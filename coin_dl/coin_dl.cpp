/*
 * Packet engine component for COIN YOLO
 */

#include <chrono>
#include <csignal>
#include <iostream>
#include <vector>

#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <fmt/core.h>
#include <glog/logging.h>
#include <pybind11/embed.h>
#include <tins/tins.h>

#include <ffpp/data_processor.hpp>
#include <ffpp/mbuf_pdu.hpp>
#include <ffpp/packet_engine.hpp>
#include <ffpp/rtp.hpp>

namespace ba = boost::asio;
namespace po = boost::program_options;
namespace py = pybind11;

bool gExit = false;

static constexpr uint64_t kSFCPort = 9999;
static constexpr uint64_t kMaxUDSMTU = 65000;

void exit_handler(int signal)
{
	LOG(INFO) << "SIGINT is received. Exit the program.";
	gExit = true;
}

void run_store_forward(ffpp::PEConfig pe_config)
{
	using namespace Tins;
	using namespace ffpp;
	using namespace std;

	// Check the usage of the DPDK's traffic management APIs
	PacketEngine pe = PacketEngine(pe_config);

	vector<struct rte_mbuf *> vec;
	uint32_t max_num_burst = 3;
	vec.reserve(kMaxBurstSize * max_num_burst);

	uint32_t num_rx = 0;
	uint32_t i = 0;

	LOG(INFO) << "Run store-and-forward loop!";
	while (not gExit) {
		num_rx = pe.rx_pkts(vec, max_num_burst);
		for (i = 0; i < num_rx; ++i) {
			// Touch the packet and update the checksums using libtins.
			auto eth = read_mbuf_to_eth(vec[i]);
			UDP &udp = eth.rfind_pdu<UDP>();
			if (udp.dport() != uint16_t(kSFCPort)) {
				LOG(ERROR) << "LOL!";
			}
			write_eth_to_mbuf(eth, vec[i]);
			rte_delay_us_block(1e3);
		}
		pe.tx_pkts(vec, chrono::microseconds(3));
	}

} // MARK: pe is out of scope, RAII

std::tuple<std::string, ffpp::RTPJPEG>
reassemble_frame(const std::vector<struct rte_mbuf *> &vec,
		 std::vector<Tins::EthernetII> &eths)
{
	using namespace Tins;
	using namespace ffpp;
	using namespace std;

	vector<RTPJPEG> fragments{};
	for (auto eth : eths) {
		UDP &udp = eth.rfind_pdu<UDP>();
		RawPDU &raw = udp.rfind_pdu<RawPDU>();
		RTPJPEG j = RTPJPEG(raw);
		fragments.push_back(j);
	}

	auto reassmbler = RTPReassembler();
	for (auto f : fragments) {
		auto ret = reassmbler.add_fragment(&f);
	}

	// BUG!!! The size of reassmbled_frame seems to be not fully correct...
	// Currently don't have the time to figure out why... A bad hot-fix is used...
	string reassmbled_frame = reassmbler.get_frame();

	return std::make_tuple(reassmbled_frame, fragments.back());
}

/**
 * Oops! What a freaking terrible implementation...
 * I know... Time is limited... My PhD contract already ends, LOL.
 * I'll refactor it soon.
 *
 * @param pe_config
 */
void run_compute_forward(ffpp::PEConfig pe_config)
{
	using namespace Tins;
	using namespace ffpp;
	using namespace std;

	// Unfortunately... This is a workaround... The embeded Python interpreter currently
	// DOES NOT work with Anaconda environment...
	ba::io_service io_service;
	ba::local::datagram_protocol::socket uds(io_service);
	ba::local::datagram_protocol::endpoint server_ep("/tmp/coin_dl_server");
	uds.open();

	ba::local::datagram_protocol::endpoint client_ep("/tmp/coin_dl_client");
	uds.bind(client_ep);

	PacketEngine pe = PacketEngine(pe_config);

	static constexpr uint64_t num_fragments = 167;
	vector<struct rte_mbuf *> vec;
	// MARK: Hard-code here...
	vec.reserve(num_fragments);

	uint64_t i;
	uint32_t num_rx;
	std::array<char, 140000> recv_buf;
	auto fragmenter = RTPFragmenter();

	LOG(INFO) << "Run compute-and-forward loop!";
	// Very naive prototype...
	while (not gExit) {
		num_rx = pe.rx_one_pkt(vec);
		// Messy code... Needs refactoring
		if (vec.size() == num_fragments) {
			vector<EthernetII> eths;
			for (auto m : vec) {
				eths.push_back(read_mbuf_to_eth(m));
			}
			auto [raw_frame, base] = reassemble_frame(vec, eths);
			// Send the raw frame to Python preprocessor
			string fake_frame(140000, 'a');
			uds.send_to(ba::buffer(fake_frame), server_ep);
			// Receive the preprocessed image
			auto ret = uds.receive_from(ba::buffer(recv_buf),
						    client_ep);
			string recv_str(std::begin(recv_buf),
					std::begin(recv_buf) + ret);
			assert(recv_str.size() == 135279);

			auto fragments =
				fragmenter.fragmentize(recv_str, base, 1400);

			auto num_pop = vec.size() - fragments.size();
			for (i = 0; i < num_pop; ++i) {
				rte_pktmbuf_free(vec.back());
				vec.pop_back();
				eths.pop_back();
			}
			// Replace the old UDP payload with new one!
			for (i = 0; i < vec.size(); ++i) {
				auto eth = eths[i];
				UDP *udp = eth.find_pdu<UDP>();
				auto inner_pdu = udp->release_inner_pdu();
				delete inner_pdu;
				RawPDU fragment_pdu =
					fragments[i].pack_to_rawpdu();
				udp = udp / fragment_pdu;
				write_eth_to_mbuf(eth, vec[i]);
			}
			// TX the preprocessed frame
			pe.tx_pkts(vec, chrono::microseconds(1000));
			eths.clear();
			fragments.clear();
		}
	}
}

int main(int argc, char *argv[])
{
	using namespace Tins;
	using namespace ffpp;
	using namespace std;

	FLAGS_logtostderr = 1;

	string mode = "store_forward";
	string host_name = ba::ip::host_name();
	string dev = host_name + "-s" + host_name.back();
	uint32_t lcore_id = 3;
	uint64_t num_fragments = 164;

	try {
		po::options_description desc("COIN YOLO");
		// clang-format off
		desc.add_options()
			("dev", po::value<string>(), fmt::format("The name of the IO network device. Default: {}", dev).c_str())
			("help,h", "Produce help message")
			("lcore_id", po::value<uint64_t>(), fmt::format("The lcore id to run on. Default: {}", lcore_id).c_str())
			("mode", po::value<string>(), "Set working mode [store_forward, compute_forward]. Default: store_forward.")
			("num_fragments", po::value<uint64_t>(), fmt::format("Number of fragments in a frame").c_str());
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help")) {
			cout << desc << "\n";
			return 1;
		}
		if (vm.count("dev")) {
			dev = vm["dev"].as<string>();
		}
		/* TODO:  <25-10-21, Zuo> Add a mode to measure batch sizes ? */
		if (vm.count("mode")) {
			mode = vm["mode"].as<string>();
		}

		if (vm.count("lcore_id")) {
			lcore_id = vm["lcore_id"].as<uint64_t>();
		}

	} catch (exception &e) {
		LOG(ERROR) << e.what();
		return 1;
	}

	LOG(INFO) << fmt::format("Run COIN-DL on lcore: {}, mode: {}, io device: {}", lcore_id,mode, dev);

	signal(SIGINT, exit_handler);

	// ISSUE: lcores are hard coded here.
	vector<uint32_t> lcore_ids = {lcore_id};
	struct PEConfig pe_config = {
		.main_lcore_id = lcore_ids[0],
		.lcore_ids = lcore_ids,
		.memory_mb = 256,
		.data_vdev_cfg= fmt::format("eth_af_packet0,iface={}", dev),
		.loglevel = "ERROR",
	};

	if (mode == "store_forward") {
		run_store_forward(pe_config);
	} else if(mode == "compute_forward") {
		run_compute_forward(pe_config);
	} else {
		throw std::runtime_error("Unknown mode. Use store_forward or compute_forward!");
	}

	return 0;
}
