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

#include <queue>
#include <chrono>

#include <gtest/gtest.h>

#include "ffpp/packet_engine.hpp"

using namespace ffpp;

// ISSUE: The EAL can be only initialized once...
// I need to look deeper into gtest if there's a way to put EAL inside its fixtures.
static auto gPE = PacketEngine("/ffpp/tests/unit/test_config.yaml");

TEST(UnitTest, TestPERxTx)
{
	PacketEngine::packet_ring_type vec;
	uint32_t max_num_burst = 3;
	vec.reserve(kMaxBurstSize * max_num_burst);

	auto num_rx = gPE.rx_pkts(vec, max_num_burst);
	ASSERT_EQ(num_rx, (unsigned int)(kMaxBurstSize * max_num_burst));
	ASSERT_EQ(num_rx, vec.size());
	gPE.tx_pkts(vec, std::chrono::microseconds(3));
	ASSERT_EQ((unsigned int)(0), vec.size());
	ASSERT_EQ((unsigned int)(kMaxBurstSize * max_num_burst),
		  vec.capacity());
}
