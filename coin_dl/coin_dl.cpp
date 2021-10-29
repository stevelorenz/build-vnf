/*
 * Packet engine component for COIN YOLO
 */

#include <chrono>
#include <csignal>
#include <iostream>
#include <vector>

#include <boost/asio/ip/host_name.hpp>
#include <boost/program_options.hpp>
#include <fmt/core.h>
#include <glog/logging.h>
#include <pybind11/embed.h>
#include <tins/tins.h>

#include <ffpp/mbuf_pdu.hpp>
#include <ffpp/packet_engine.hpp>

using namespace Tins;
using namespace ffpp;
using namespace std;

namespace po = boost::program_options;
namespace py = pybind11;

bool gExit = false;

void exit_handler(int signal)
{
	LOG(INFO) << "Exit the program.";
	gExit = true;
}

void run_store_forward(PEConfig pe_config)
{
	// Check the usage of the DPDK's traffic management APIs
	LOG(INFO) << "Run store-and-forward loop";
	PacketEngine pe = PacketEngine(pe_config);

	vector<struct rte_mbuf *> vec;
	uint32_t max_num_burst = 3;
	vec.reserve(kMaxBurstSize * max_num_burst);

	uint32_t num_rx = 0;
	uint32_t i = 0;
	while (not gExit) {
		num_rx = pe.rx_pkts(vec, max_num_burst);
		// ONLY for debug
		// if (num_rx > 0) {
		//	cout << num_rx << endl;
		//}
		for (i = 0; i < num_rx; ++i) {
			// Touch the packet and update the checksums using libtins.
			auto eth = read_mbuf_to_eth(vec[i]);
			UDP &udp = eth.rfind_pdu<UDP>();
			if (udp.dport() != uint16_t(9999)) {
				LOG(ERROR) << "LOL!";
			}
			write_eth_to_mbuf(eth, vec[i]);
		}
		pe.tx_pkts(vec, chrono::microseconds(3));
	}

} // MARK: pe is out of scope, RAII

int main(int argc, char *argv[])
{
	FLAGS_logtostderr = 1;

	string mode = "store_forward";
	string host_name = boost::asio::ip::host_name();
	string iface = host_name + "-s" + host_name.back();

	try {
		po::options_description desc("COIN YOLO");
		// clang-format off
		desc.add_options()
			("help,h", "Produce help message")
			("iface,i", po::value<string>(), "The name of the IO interface.")
			("mode,m", po::value<string>(), "Set mode [store_forward, compute_forward]. The default is store_forward.");
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help")) {
			cout << desc << "\n";
			return 1;
		}
		if (vm.count("iface")) {
			iface = vm["iface"].as<string>();
		}
		/* TODO:  <25-10-21, Zuo> Add a mode to measure batch sizes ? */
		if (vm.count("mode")) {
			mode = vm["mode"].as<string>();
		}
	} catch (exception &e) {
		LOG(ERROR) << e.what();
		return 1;
	}

	LOG(INFO) << fmt::format("Run COIN YOLO, mode: {}, interface: {}", mode, iface);

	signal(SIGINT, exit_handler);

	// ISSUE: lcores are hard coded here.
	vector<uint32_t> lcore_ids = {2};
	struct PEConfig pe_config = {
		.main_lcore_id = lcore_ids[0],
		.lcore_ids = lcore_ids,
		.memory_mb = 256,
		.data_vdev_cfg= fmt::format("eth_af_packet0,iface={}", iface),
		.loglevel = "ERROR",
	};

	if (mode == "store_forward") {
		run_store_forward(pe_config);
	}

	return 0;
}
