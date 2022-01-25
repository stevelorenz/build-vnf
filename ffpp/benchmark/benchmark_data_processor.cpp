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
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>
#include <pybind11/embed.h>

#include "ffpp/packet_engine.hpp"
#include "ffpp/data_processor.hpp"

namespace py = pybind11;

static auto gPE = ffpp::PacketEngine("/ffpp/benchmark/benchmark_config.yaml");

// According the benchmark on my dev VM: The Python based implementation is at least
// 15 times slower to just append a string...
static void bm_embeded_py(benchmark::State &state)
{
	ffpp::py_insert_sys_path("/ffpp/tests/unit/", 0);
	auto test_module = py::module::import("test_py_data_processor");
	auto append_test_str = test_module.attr("append_test_str");
	py::object ret;
	std::string new_str = "";
	for (auto _ : state) {
		ret = append_test_str("zuozuo");
		new_str = ret.cast<std::string>();
	}
}

static void bm_embeded_py_cpp_ref(benchmark::State &state)
{
	std::string str = "zuozuo";
	std::string new_str = "";
	for (auto _ : state) {
		new_str = str + "_fanfan";
	}
}

static void bm_py_bytearray(benchmark::State &state)
{
	ffpp::py_insert_sys_path("/ffpp/tests/unit/", 0);
	auto test_module = py::module::import("test_py_data_processor");
	auto do_nothing = test_module.attr("do_nothing");
	std::string test_data(int(std::pow(2, 20)), 'A');
	for (auto _ : state) {
		// Great! bytearray costs less than 50% time than std::string
		do_nothing(py::bytearray(test_data));
	}
}

BENCHMARK(bm_embeded_py);
BENCHMARK(bm_embeded_py_cpp_ref);
BENCHMARK(bm_py_bytearray);

BENCHMARK_MAIN();
