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

#include <vector>
#include <stdexcept>

#include <rte_eal.h>
#include <fmt/core.h>
#include <glog/logging.h>

#include "ffpp/cnf.hpp"

namespace ffpp
{
CNF::CNF()
{
	FLAGS_logtostderr = true;
	FLAGS_colorlogtostderr = true;
	FLAGS_minloglevel = 0;
	FLAGS_stderrthreshold = 0;
	FLAGS_v = 3;
	google::InitGoogleLogging("FFPP-CNF");

	LOG(INFO) << "Initialize DPDK EAL environment.";
	std::vector<char *> dpdk_arg;
	dpdk_arg.push_back((char *)(new std::string("-l 0"))->c_str());
	dpdk_arg.push_back((char *)(new std::string("--no-pci"))->c_str());

	auto ret = rte_eal_init(dpdk_arg.size(), dpdk_arg.data());
	if (ret < 0) {
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
	}
}

CNF::~CNF()
{
	LOG(INFO) << "Cleanup DPDK EAL environment.";
	rte_eal_cleanup();
}

void CNF::init()
{
}

} // namespace ffpp