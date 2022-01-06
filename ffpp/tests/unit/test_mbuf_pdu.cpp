/**
 *  Copyright (C) 2020 Zuo Xiang
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

#include <iostream>
#include <chrono>

#include <gtest/gtest.h>
#include <tins/tins.h>
#include <rte_mbuf.h>

#include "ffpp/mbuf_pdu.hpp"
#include "ffpp/packet_engine.hpp"

using namespace ffpp;
using namespace Tins;

// ISSUE: The EAL can be only initialized once...
static auto gPE = PacketEngine("/ffpp/tests/unit/test_config.yaml");

EthernetII create_sample_ethernet_frame()
{
	NetworkInterface iface = NetworkInterface::default_interface();
	NetworkInterface::Info info = iface.addresses();
	EthernetII eth = EthernetII("17:17:17:17:17:17", info.hw_addr) /
			 IP("192.168.17.17", info.ip_addr) / UDP(8888, 8888) /
			 RawPDU("Detective Conan: One truth prevails !");

	return eth;
}

TEST(UnitTest, TestPacketParsing)
{
	PacketEngine::packet_ring_type vec;
	uint32_t max_num_burst = 3;
	vec.reserve(kMaxBurstSize * max_num_burst);

	auto num_rx = gPE.rx_pkts(vec, max_num_burst); // NOLINT

	EthernetII eth = create_sample_ethernet_frame();
	IP &ip = eth.rfind_pdu<IP>();
	UDP &udp = eth.rfind_pdu<UDP>();
	ASSERT_EQ(eth.trailer_size(), uint32_t(0));
	write_eth_to_mbuf(eth, vec[0]);

	auto new_eth = read_mbuf_to_eth(vec[0]);
	IP &new_ip = new_eth.rfind_pdu<IP>();
	UDP &new_udp = new_eth.rfind_pdu<UDP>();

	ASSERT_EQ(ip.tot_len(), new_ip.tot_len());
	ASSERT_EQ(udp.dport(), new_udp.dport());

	gPE.tx_pkts(vec, std::chrono::microseconds(0));
}
