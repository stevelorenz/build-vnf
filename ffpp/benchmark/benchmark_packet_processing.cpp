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

#include <chrono>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

#include "ffpp/mbuf_pdu.hpp"
#include "ffpp/packet_engine.hpp"
#include "ffpp/rtp.hpp"

using namespace ffpp;

static void bm_rtp_pack_jpeg(benchmark::State &state)
{
	struct rtp_hdr rtp_h;
	struct rtp_jpeg_hdr rtp_jpeg_h;

	std::string payload = "test";

	for (auto _ : state) {
		auto rtp_pdu = rtp_pack_jpeg(rtp_h, rtp_jpeg_h, payload);
	}
}

BENCHMARK(bm_rtp_pack_jpeg);

BENCHMARK_MAIN();
