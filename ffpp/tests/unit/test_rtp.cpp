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
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <tins/tins.h>

#include "ffpp/packet_engine.hpp"
#include "ffpp/rtp.hpp"

std::string kTestPayload = "Detective Conan: One truth prevails !";

TEST(UnitTest, TestRtpPackUnpack)
{
	using namespace ffpp;
	using namespace Tins;

	struct rtp_hdr rtp_h;
	struct rtp_jpeg_hdr rtp_jpeg_h;
	ASSERT_EQ(sizeof(rtp_h), kRtpHdrSize);
	ASSERT_EQ(sizeof(rtp_jpeg_h), kRtpJpegHdrSize);

	RTPJPEG rtp_jpeg = RTPJPEG(1, 123, 321, 2048, kTestPayload);

	// Test getters and setters
	ASSERT_EQ(rtp_jpeg.mark_bit(), 1);
	rtp_jpeg.mark_bit(0);
	ASSERT_EQ(rtp_jpeg.mark_bit(), 0);
	ASSERT_EQ(rtp_jpeg.seq_number(), (uint16_t)(123));
	rtp_jpeg.seq_number(321);
	ASSERT_EQ(rtp_jpeg.seq_number(), (uint16_t)(321));
	ASSERT_EQ(rtp_jpeg.timestamp(), (uint32_t)(321));
	rtp_jpeg.timestamp(123);
	ASSERT_EQ(rtp_jpeg.timestamp(), (uint32_t)(123));
	ASSERT_EQ(rtp_jpeg.fragment_offset(), (uint32_t)(2048));
	rtp_jpeg.fragment_offset(8402);
	ASSERT_EQ(rtp_jpeg.fragment_offset(), (uint32_t)(8402));

	RawPDU rtp_jpeg_pdu = rtp_jpeg.pack_to_rawpdu();
	ASSERT_EQ(rtp_jpeg_pdu.payload_size(),
		  (kRtpHdrSize + kRtpJpegHdrSize + kTestPayload.size()));

	RTPJPEG rtp_jpeg_new = RTPJPEG(rtp_jpeg_pdu);
	ASSERT_EQ(rtp_jpeg_new.mark_bit(), 0);
	ASSERT_EQ(rtp_jpeg_new.seq_number(), (uint16_t)(321));
	ASSERT_EQ(rtp_jpeg_new.timestamp(), (uint32_t)(123));
	ASSERT_EQ(rtp_jpeg_new.fragment_offset(), (uint32_t)(8402));

	RTPJPEG::rtp_payload_type payload = rtp_jpeg_new.payload();
	std::string unpacked_payload(payload.begin(), payload.end());
	ASSERT_EQ(kTestPayload, unpacked_payload);
}

TEST(UnitTest, TestReleaseUdpPdu)
{
	using namespace ffpp;
	using namespace Tins;

	// MARK: The EthernetII can add automatically padding bytes when size < 60!
	EthernetII eth = EthernetII("aa:aa:aa:aa:aa:aa", "aa:aa:aa:aa:aa:aa") /
			 IP("127.0.0.1", "127.0.0.1") / UDP(1, 0) /
			 RawPDU(kTestPayload);
	ASSERT_TRUE(eth.size() > 60);
	UDP *udp = eth.find_pdu<UDP>();
	auto inner_pdu = udp->release_inner_pdu();
	ASSERT_NE(inner_pdu, nullptr);
	// The inner_pdu now owns the resource, it's our responsibility to free
	// the memory. This inner_pdu is no longer belonged by the parent eth
	// and not managed by eth's RAII.
	delete inner_pdu;
	ASSERT_EQ(eth.size(), (unsigned int)(60));

	RTPJPEG rtp_jpeg = RTPJPEG(1, 123, 321, 2048, kTestPayload);
	RawPDU rtp_jpeg_pdu = rtp_jpeg.pack_to_rawpdu();
	udp = udp / rtp_jpeg_pdu;
	unsigned int eth_new_size = 14 + 20 + 8 + rtp_jpeg_pdu.size();
	ASSERT_EQ(eth.size(), eth_new_size);
}

TEST(UnitTest, TestRTPFragmenter)
{
	using namespace ffpp;

	auto fragmenter = RTPFragmenter();
	std::string test_data(48000, 'A');
	RTPJPEG base = RTPJPEG(0, 123, 321, 0, kTestPayload);
	auto fragments = fragmenter.fragmentize(test_data, base, 1400);

	ASSERT_TRUE(fragments.size() == 35);
	ASSERT_TRUE(fragments[0].fragment_offset() == 0);
	ASSERT_TRUE(fragments[1].fragment_offset() == 1400);
	ASSERT_TRUE(fragments[1].mark_bit() == 0);
	ASSERT_TRUE(fragments.back().fragment_offset() == 47600);
	ASSERT_TRUE(fragments.back().payload().size() == 400);
	ASSERT_TRUE(fragments.back().mark_bit() == 1);
}

TEST(UnitTest, TestRTPReassembler)
{
	using namespace ffpp;

	auto reassembler = RTPReassembler();

	auto fragmenter = RTPFragmenter();
	// std::string test_data(48000, 'A');
	std::string test_data(1500, 'A');
	RTPJPEG base = RTPJPEG(0, 123, 321, 0, kTestPayload);
	auto fragments = fragmenter.fragmentize(test_data, base, 1400);

	auto ret = reassembler.add_fragment(&fragments[1]);
	ASSERT_TRUE(ret == RTPReassembler::BAD_NEW_FRAME);
	ret = reassembler.add_fragment(&fragments[0]);
	ASSERT_TRUE(ret == RTPReassembler::GOOD_NEW_FRAME);
	ret = reassembler.add_fragment(&fragments[2]);
	ASSERT_TRUE(ret == RTPReassembler::BAD_CUR_FRAME);
	ASSERT_TRUE(reassembler.fragment_vec_size() == 0);

	ret = reassembler.add_fragment(&fragments[0]);
	ASSERT_TRUE(ret == RTPReassembler::GOOD_NEW_FRAME);
	for (auto it = fragments.begin() + 1; it != fragments.end() - 1; ++it) {
		ret = reassembler.add_fragment(&(*it));
		ASSERT_TRUE(ret == RTPReassembler::GOOD_CUR_FRAME);
	}
	ret = reassembler.add_fragment(&(fragments.back()));
	ASSERT_TRUE(ret == RTPReassembler::HAS_ENTIRE_FRAME);
	ASSERT_TRUE(reassembler.next_fragment_offset() == test_data.size());

	auto reassembled_data = reassembler.get_frame();
	ASSERT_TRUE(reassembler.fragment_vec_size() == 0);
	ASSERT_TRUE(reassembled_data == test_data);
}
