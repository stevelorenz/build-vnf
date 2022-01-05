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

uint8_t get_m_pt(uint8_t mark_bit)
{
	return ((mark_bit & 0x01) << 7) + (kRtpJpegPayloadType & 0x7f);
}

void get_rtp_hdr(struct rtp_hdr &hdr, uint8_t mark_bit, uint16_t seq_number,
		 uint16_t timestamp)
{
	uint8_t v_p_e_cc = 0;
	// I'm ...
	v_p_e_cc = (2 & 0xc0) + (0 & 0x20) + (0 & 0x10) + (0 & 0x0f);

	uint8_t m_pt = get_m_pt(mark_bit);

	hdr = {
		.v_p_e_cc = v_p_e_cc,
		.m_pt = m_pt,
		.seq_number = rte_cpu_to_be_16(seq_number),
		.timestamp = rte_cpu_to_be_32(timestamp),
		// Now I don't fully understand usage of these two fields.
		.ssrc = rte_cpu_to_be_32(0),
		.csrc = rte_cpu_to_be_32(0),
	};
}

void get_rtp_jpeg_hdr(struct rtp_jpeg_hdr &hdr, uint32_t fragment_offset)
{
	hdr = {
		.fragment_offset = rte_be_to_cpu_32(fragment_offset),
		.type = 0,
		.q = 0,
		.width = 0,
		.height = 0,
	};
}

// I know, I know, the implementation looks ( And it really is ;) ) very inefficient...
// I'm just a researcher :)
RTPJPEG::RTPJPEG(uint8_t mark_bit, uint16_t seq_number, uint16_t timestamp,
		 uint32_t fragment_offset, const std::string payload)
{
	get_rtp_hdr(rtp_hdr_, mark_bit, seq_number, timestamp);
	get_rtp_jpeg_hdr(rtp_jpeg_hdr_, fragment_offset);
	uint8_t buf[kRtpHdrSize + kRtpJpegHdrSize];
	payload_ = rtp_payload_type(payload.begin(), payload.end());
}

// I'm definitely not sure this is the right way to go...
RTPJPEG::RTPJPEG(const Tins::RawPDU &raw_pdu)
{
	// vector of uint8_t (bytes)
	rtp_payload_type raw_payload = raw_pdu.payload();

	rte_memcpy(&rtp_hdr_, raw_payload.data(), kRtpHdrSize);
	rte_memcpy(&rtp_jpeg_hdr_, raw_payload.data() + kRtpHdrSize,
		   kRtpJpegHdrSize);

	payload_ = rtp_payload_type();
	auto hdr_size = (kRtpHdrSize + kRtpJpegHdrSize);
	std::copy(raw_payload.begin() + hdr_size, raw_payload.end(),
		  std::back_inserter(payload_));
}

uint8_t RTPJPEG::mark_bit() const
{
	return (rtp_hdr_.m_pt & 0x80) >> 7;
}

void RTPJPEG::mark_bit(uint8_t mark_bit)
{
	rtp_hdr_.m_pt = get_m_pt(mark_bit);
}

Tins::RawPDU RTPJPEG::pack_to_rawpdu()
{
	using namespace Tins;

	uint8_t buf[kRtpHdrSize + kRtpJpegHdrSize];
	rte_memcpy(buf, &rtp_hdr_, kRtpHdrSize);
	rte_memcpy(buf + kRtpHdrSize, &rtp_jpeg_hdr_, kRtpJpegHdrSize);

	RawPDU::payload_type pdu_data;
	// Add RTP header and JPEG payload header
	pdu_data.insert(pdu_data.begin(), buf,
			buf + (kRtpHdrSize + kRtpJpegHdrSize));
	// Append the RTP payload data
	for (const uint8_t &b : payload_) {
		pdu_data.push_back(b);
	}

	RawPDU raw_pdu = RawPDU(pdu_data);
	return raw_pdu;
}

} // namespace ffpp
