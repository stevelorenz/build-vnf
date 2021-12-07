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

#pragma once

#include <rte_byteorder.h>
#include <rte_memcpy.h>

#include <cstdint>
#include <string>

#include <tins/tins.h>

namespace ffpp
{

constexpr uint16_t kRtpHdrSize = 16; // bytes
constexpr uint16_t kRtpJpegHdrSize = 8; // bytes

/**
 * Ref: RFC 3550
 */
struct rtp_hdr {
	rte_be16_t version : 2;
	rte_be16_t pad_bit : 1;
	rte_be16_t ext_bit : 1;
	rte_be16_t cc : 4;
	rte_be16_t mark_bit : 1;
	rte_be16_t payload_type : 7;
	// MARK: pay attention to endianness!
	rte_be16_t seq_number;
	rte_be32_t timestamp;
	rte_be32_t ssrc;
	rte_be32_t csrc;
} __rte_packed;

/**
 * Ref: RFC 2435
 */
struct rtp_jpeg_hdr {
	rte_be32_t type_specific : 8;

	// MARK: pay attention to endianness!
	rte_be32_t fragment_offset : 24;
	uint8_t type;
	uint8_t q;
	uint8_t width;
	uint8_t height;
} __rte_packed;

// I know, it's very C-style. I use C + STL + class ;)
// Maybe it's not a good idea to mix C-style struct and libtins's PDU design...
// Naja, just for prototyping...

/**
 * Pack RTP, JPEG payload headers and the payload into a new RawPDU.
 *
 * @param rtp_h
 * @param rtp_jpeg_h
 * @param payload
 *
 * @return A new RawPDU contains all RTP headers and payload.
 */
Tins::RawPDU rtp_pack_jpeg(const struct rtp_hdr &rtp_h,
			   const struct rtp_jpeg_hdr &rtp_jpeg_h,
			   std::string payload);
} // namespace ffpp
