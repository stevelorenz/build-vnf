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

#include <cstdint>
#include <iostream>

#include <gtest/gtest.h>
#include <string>
#include <tins/tins.h>

#include "ffpp/packet_engine.hpp"
#include "ffpp/rtp.hpp"

using namespace ffpp;

TEST(UnitTest, TestRtpPackUnpack)
{
	struct rtp_hdr rtp_h;
	// RTP header is 16 bytes
	ASSERT_EQ(sizeof(rtp_h), kRtpHdrSize);

	struct rtp_jpeg_hdr rtp_jpeg_h;
	ASSERT_EQ(sizeof(rtp_jpeg_h), kRtpJpegHdrSize);

	std::string payload = "FanFan";
	Tins::RawPDU payload_pdu = Tins::RawPDU(payload);
	Tins::RawPDU rtp_pdu = rtp_pack_jpeg(rtp_h, rtp_jpeg_h, payload_pdu);

	ASSERT_EQ(
		rtp_pdu.payload_size(),
		(unsigned int)(kRtpHdrSize + kRtpJpegHdrSize + payload.size()));

	RTP rtp = RTP(0, 0, 0, 0, 0, 0, "AAA");
}

TEST(UnitTest, TestReleaseUdpPdu)
{
	using namespace Tins;
	struct rtp_hdr rtp_h;
	struct rtp_jpeg_hdr rtp_jpeg_h;
	std::string payload = "FanFan";
	Tins::RawPDU payload_pdu = Tins::RawPDU(payload);
	Tins::RawPDU rtp_pdu = rtp_pack_jpeg(rtp_h, rtp_jpeg_h, payload_pdu);

	// MARK: The EthernetII can add automatically padding bytes when size < 60!
	EthernetII eth = EthernetII("aa:aa:aa:aa:aa:aa", "aa:aa:aa:aa:aa:aa") /
			 IP("127.0.0.1", "127.0.0.1") / UDP(1, 0) /
			 RawPDU("Detective Conan: One truth prevails !");
	ASSERT_TRUE(eth.size() > 60);
	UDP *udp = eth.find_pdu<UDP>();
	auto inner_pdu = udp->release_inner_pdu();
	ASSERT_NE(inner_pdu, nullptr);
	// The inner_pdu now owns the resource, it's our responsibility to free
	// the memory. This inner_pdu is no longer belonged by the parent eth
	// and not managed by eth's RAII.
	delete inner_pdu;
	ASSERT_EQ(eth.size(), (unsigned int)(60));
	udp = udp / rtp_pdu;
	unsigned int eth_new_size = 14 + 20 + 8 + rtp_pdu.size();
	ASSERT_EQ(eth.size(), eth_new_size);
}

TEST(UnitTest, TestRtpFrameProcessor)
{
	ASSERT_EQ(0, 0);
}
