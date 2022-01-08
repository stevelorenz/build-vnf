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
#include <vector>

#include <tins/tins.h>

namespace ffpp
{

constexpr uint16_t kRtpHdrSize = 16; // bytes
constexpr uint16_t kRtpJpegHdrSize = 8; // bytes
constexpr uint16_t kRtpJpegPayloadType = 26;

/**
 * Ref: RFC 3550
 */
struct rtp_hdr {
	uint8_t v_p_e_cc;
	uint8_t m_pt;
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
	// MARK: pay attention to endianness!
	rte_be32_t fragment_offset;
	uint8_t type;
	uint8_t q;
	uint8_t width;
	uint8_t height;
} __rte_packed;

// Following functions are very "slow" prototypes, compared to direct operations
// on rte_mbuf...
// A proper zero-copy and efficient C++ wrapper for mbuf operations should be
// used. I have not yet found the proper library.

/**
 * RTP with JPEG payload
 *
 * ISSUE: Many implicit memory copies can happen when using std::vector<uint8_t> as payload type...
 * libtins's current design does not care about the memory alloc/free and copies when process packets.
 * So this is really an INEFFICIENT prototyping
 */
class RTPJPEG {
    public:
	using rtp_payload_type = std::vector<uint8_t>;

	RTPJPEG(uint8_t mark_bit, uint16_t seq_number, uint16_t timestamp,
		uint32_t fragment_offset, const std::string payload);

	RTPJPEG(const Tins::RawPDU &raw_pdu);

	uint8_t mark_bit() const;

	void mark_bit(uint8_t mark_bit);

	uint16_t seq_number() const
	{
		return rte_be_to_cpu_16(rtp_hdr_.seq_number);
	}

	void seq_number(uint16_t seq_number)
	{
		rtp_hdr_.seq_number = rte_cpu_to_be_16(seq_number);
	}

	uint32_t timestamp() const
	{
		return rte_be_to_cpu_32(rtp_hdr_.timestamp);
	}

	void timestamp(uint32_t timestamp)
	{
		rtp_hdr_.timestamp = rte_cpu_to_be_32(timestamp);
	}

	uint32_t fragment_offset() const
	{
		return rte_be_to_cpu_32(rtp_jpeg_hdr_.fragment_offset);
	}

	void fragment_offset(uint32_t fragment_offset)
	{
		rtp_jpeg_hdr_.fragment_offset =
			rte_cpu_to_be_32(fragment_offset);
	}

	rtp_payload_type payload() const
	{
		return payload_;
	}

	// Check assign method of vector and templates for ForwardIterator
	void payload(rtp_payload_type payload)
	{
		payload_ = payload;
	}

	/**
	 * pack_to_rawpdu
	 *
	 * @return 
	 */
	Tins::RawPDU pack_to_rawpdu();

    private:
	// I know, pimpl can be used to build the compiler firewall. This is just toy research tool code
	struct rtp_hdr rtp_hdr_;
	struct rtp_jpeg_hdr rtp_jpeg_hdr_;
	// The name of the variable and type needs more consideration
	rtp_payload_type payload_;
};

/**
 * RTP fragmenter
 */
class RTPFragmenter {
    public:
	RTPFragmenter(){};

	std::vector<RTPJPEG> fragmentize(const std::string &data,
					 const RTPJPEG &base,
					 uint64_t max_fragment_size);

    private:
	/* data */
};

/**
 * RTP reassembler
 *
 * This is currently JUST an OVER-SIMPLIFIED prototype without considering
 * out-of-order packets and fixing the packet losses.
 *
 * Compared to fragmenter, reassembler is more complicated since
 * input packets could be lost or out-of-order.
 * So the reassembler feels like a RLNC recoder.
 * This should be the virtual class for different RTPReassembler.
 * Currently only RTPJPEG payload type is implemented.
 */
class RTPReassembler {
    public:
	RTPReassembler()
		: fragment_vec_({}), cur_seq_number_(0),
		  next_fragment_offset_(0){};

	void reset()
	{
		fragment_vec_.clear();
		cur_seq_number_ = 0;
		next_fragment_offset_ = 0;
	}

	enum AddResult {
		GOOD_NEW_FRAME,
		BAD_NEW_FRAME,
		GOOD_CUR_FRAME,
		BAD_CUR_FRAME,
		// get_frame() can be used to retrieve the entire frame
		HAS_ENTIRE_FRAME,
	};

	// I feel like the design of add_fragment is not fully correct...
	AddResult add_fragment(const RTPJPEG *fragment);

	/**
	 * Get the reassembled I-frame data
	 *
	 * @return 
	 */
	std::string get_frame();

	uint64_t fragment_vec_size() const
	{
		return fragment_vec_.size();
	}

	uint64_t cur_seq_number() const
	{
		return cur_seq_number_;
	}

    private:
	// Use reference ? But reference_wrapper is needed !
	// Somehow STL containers does not support store references directly
	std::vector<const RTPJPEG *> fragment_vec_;
	uint64_t cur_seq_number_;
	uint64_t next_fragment_offset_;
};

} // namespace ffpp
