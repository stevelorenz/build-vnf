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

	std::string payload = "test";
	Tins::RawPDU rtp_pdu = rtp_pack_jpeg(rtp_h, rtp_jpeg_h, payload);

	ASSERT_EQ(
		rtp_pdu.payload_size(),
		(unsigned int)(kRtpHdrSize + kRtpJpegHdrSize + payload.size()));
}

TEST(UnitTest, TestRtpFrameProcessor)
{
	ASSERT_EQ(0, 0);
}
