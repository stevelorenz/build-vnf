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

#include <iostream>
#include <rte_memcpy.h>
#include <string>

#include <tins/tins.h>

#include "ffpp/rtp.hpp"

namespace ffpp
{

Tins::RawPDU rtp_pack_jpeg(const struct rtp_hdr &rtp_h,
			   const struct rtp_jpeg_hdr &rtp_jpeg_h,
			   std::string payload)
{
	using namespace Tins;

	static_assert(sizeof(rtp_h) == kRtpHdrSize);
	static_assert(sizeof(rtp_jpeg_hdr) == kRtpJpegHdrSize);

	// I know, I know, the implementation looks very inefficient...
	// I'm just a researcher :)
	uint8_t buf[kRtpHdrSize + kRtpJpegHdrSize];
	rte_memcpy(buf, &rtp_h, kRtpHdrSize);
	rte_memcpy(buf + kRtpHdrSize, &rtp_jpeg_h, kRtpJpegHdrSize);
	RawPDU::payload_type rtp_pdu_payload;
	// Add rtp_hdr and rtp_jpeg_hdr
	rtp_pdu_payload.insert(rtp_pdu_payload.begin(), buf,
			       buf + (kRtpHdrSize + kRtpJpegHdrSize));
	// Add payload
	std::vector<uint8_t> payload_vec(payload.begin(), payload.end());
	for (const uint8_t &b : payload_vec) {
		rtp_pdu_payload.push_back(b);
	}

	RawPDU rtp_pdu = RawPDU(rtp_pdu_payload);

	return rtp_pdu;
}
} // namespace ffpp
