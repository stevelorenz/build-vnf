/**
 *  Copyright (C) 2021 Zuo Xiang
 *  All rights reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 *  deal in the Software without restriction, including without limitation the
 *  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 *  sell copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 */

#include <chrono>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>
#include <tins/tins.h>

#include "ffpp/mbuf_pdu.hpp"
#include "ffpp/packet_engine.hpp"
#include "ffpp/rtp.hpp"

static auto gPE = ffpp::PacketEngine("/ffpp/benchmark/benchmark_config.yaml");

Tins::EthernetII create_sample_ethernet_frame()
{
	using namespace ffpp;
	using namespace Tins;
	NetworkInterface iface = NetworkInterface::default_interface();
	NetworkInterface::Info info = iface.addresses();
	EthernetII eth = EthernetII("17:17:17:17:17:17", info.hw_addr) /
			 IP("192.168.17.17", info.ip_addr) / UDP(8888, 8888) /
			 RawPDU("Detective Conan: One truth prevails !");

	return eth;
}

static void bm_eth_pdu_serialise(benchmark::State &state)
{
	using namespace Tins;
	EthernetII eth = create_sample_ethernet_frame();
	for (auto _ : state) {
		// According to the profiling(valgrind callgraph), the most
		// time-consuming operation is the checksum calculation.
		// This can not fixed by software, hardware offloading is the direction.
		eth.serialize();
	}
}

static void bm_eth_pdu_to_mbuf(benchmark::State &state)
{
	using namespace ffpp;
	using namespace Tins;
	PacketEngine::packet_vector vec;
	uint32_t max_num_burst = 3;
	vec.reserve(kMaxBurstSize * max_num_burst);

	gPE.rx_pkts(vec, max_num_burst);

	EthernetII eth = create_sample_ethernet_frame();
	for (auto _ : state) {
		// It's really slow here... Takes more than 100 ns to write eth
		// to mbuf ...
		write_eth_to_mbuf(eth, vec[0]);
	}
	gPE.tx_pkts(vec, std::chrono::microseconds(0));
}

static void bm_eth_mbuf_to_pdu(benchmark::State &state)
{
	using namespace ffpp;
	using namespace Tins;
	PacketEngine::packet_vector vec;
	uint32_t max_num_burst = 3;
	vec.reserve(kMaxBurstSize * max_num_burst);

	gPE.rx_pkts(vec, max_num_burst);

	EthernetII eth = create_sample_ethernet_frame();
	write_eth_to_mbuf(eth, vec[0]);
	for (auto _ : state) {
		read_mbuf_to_eth(vec[0]);
	}
	gPE.tx_pkts(vec, std::chrono::microseconds(0));
}

/* TODO: Add benchmarks for RTPJPEG unpack and pack <09-01-22, Zuo> */

static void bm_rtp_jpeg_fragmentize(benchmark::State &state)
{
	using namespace ffpp;
	auto fragmenter = RTPFragmenter();
	std::string test_data(48000, 'A');
	RTPJPEG base = RTPJPEG(0, 123, 321, 0, "");
	for (auto _ : state) {
		fragmenter.fragmentize(test_data, base, 1400);
	}
}

static void bm_rtp_jpeg_reassemble(benchmark::State &state)
{
	using namespace ffpp;

	auto reassembler = RTPReassembler();
	auto fragmenter = RTPFragmenter();
	std::string test_data(48000, 'A');
	RTPJPEG base = RTPJPEG(0, 123, 321, 0, "");
	auto fragments = fragmenter.fragmentize(test_data, base, 1400);

	for (auto _ : state) {
		auto ret = reassembler.add_fragment(&fragments[0]);
		for (auto it = fragments.begin() + 1; it != fragments.end() - 1;
		     ++it) {
			ret = reassembler.add_fragment(&(*it));
		}
		ret = reassembler.add_fragment(&(fragments.back()));

		auto frame = reassembler.get_frame();
	}
}

BENCHMARK(bm_eth_pdu_serialise);
BENCHMARK(bm_eth_pdu_to_mbuf);
BENCHMARK(bm_eth_mbuf_to_pdu);
BENCHMARK(bm_rtp_jpeg_fragmentize);
BENCHMARK(bm_rtp_jpeg_reassemble);

BENCHMARK_MAIN();
