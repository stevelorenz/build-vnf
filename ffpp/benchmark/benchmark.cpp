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

#include <chrono>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

#include "ffpp/packet_engine.hpp"

using namespace ffpp;

static auto gPE = PacketEngine("/ffpp/benchmark/benchmark_config.yaml");

// MARK: Can be extended to benchmark different PMDs ?
// NOLINTBEGIN
static void bm_pe_rxtx(benchmark::State &state)
{
	std::vector<struct rte_mbuf *> vec;
	uint32_t max_num_burst = 3;
	vec.reserve(kMaxBurstSize * max_num_burst);
	for (auto _ : state) {
		auto num_rx = gPE.rx_pkts(vec, max_num_burst);
		gPE.tx_pkts(vec, std::chrono::microseconds(0));
	}
}
// NOLINTEND

// TODO: Check if this can be used for performance benchmark with different
// traffic, without using a separate pktgen ?

BENCHMARK(bm_pe_rxtx);

BENCHMARK_MAIN();
