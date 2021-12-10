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

/**
 * @file
 * RTP protocol related operations.
 *
 * This is currently JUST the quick PROTOTYPE and super-slow version using libtins's
 * data structures.
 * The reason for using a slow implementation is that it's easy to get research stuff done ;)
 *
 * Check dpdk/examples/ip_reassembly for a DPDK-native reassembly implementation
 *
 */

#include <rte_byteorder.h>
#include <rte_memcpy.h>

#include <cstdint>
#include <string>
#include <tins/rawpdu.h>
#include <vector>

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

// Following functions are very "slow" prototypes, compared to direct operations
// on rte_mbuf...
// A proper zero-copy and efficient C++ wrapper for mbuf operations should be
// used. I have not yet found the proper library.

// I know, it's very C-style. I use C + STL + class ;)
// Maybe it's not a good idea to mix C-style struct and libtins's PDU design...
// Naja, just for prototyping... I don't want to modify libtins's source code to
// add new protocols currently.

Tins::RawPDU rtp_pack_jpeg(const struct rtp_hdr &rtp_h,
			   const struct rtp_jpeg_hdr &rtp_jpeg_h,
			   const Tins::RawPDU &payload_pdu);

Tins::RawPDU rtp_unpack_jpeg(Tins::RawPDU rtp_pdu, struct rtp_hdr &rtp_h,
			     struct rtp_jpeg_hdr &rtp_jpeg_hdr);

/**
 * RTP PDU as RawPDU
 */
class RTP {
    public:
	using rtp_payload_type = std::vector<uint8_t>;
	RTP(const Tins::RawPDU &raw_pdu);
	RTP(uint16_t mark_bit, uint16_t payload_type, uint16_t seq_number,
	    uint16_t timestamp, uint32_t ssrc, uint32_t csrc,
	    const std::string payload);

	RTP(uint16_t mark_bit, uint16_t payload_type, uint16_t seq_number,
	    uint16_t timestamp, uint32_t ssrc, uint32_t csrc,
	    const std::vector<uint8_t> &payload);

	uint16_t mark_bit() const
	{
		return rte_be_to_cpu_16(rtp_hdr_.mark_bit);
	}

	uint16_t seq_number() const
	{
		return rte_be_to_cpu_16(rtp_hdr_.seq_number);
	}

	uint16_t payload_type() const
	{
		return rte_be_to_cpu_16(rtp_hdr_.payload_type);
	}

    private:
	struct rtp_hdr rtp_hdr_;
	struct rtp_jpeg_hdr rtp_jpeg_hdr_;
	rtp_payload_type payload_;
};

/**
 * @brief
 */
class RTPReassembler {
    public:
	RTPReassembler();

    private:
};

/**
 * @brief
 */
class RTPFragmenter {
    public:
	RTPFragmenter();

    private:
	/* data */
};

} // namespace ffpp
