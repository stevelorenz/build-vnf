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
#include <string>
#include <algorithm>

#include <unistd.h>

#include <rte_eal.h>
#include <rte_ethdev.h>

#include <fmt/core.h>
#include <fmt/format.h>

#include <glog/logging.h>
#include <yaml-cpp/yaml.h>

#include "ffpp/cnf.hpp"

namespace py = pybind11;

namespace ffpp
{
static void init_eal(const struct CNFConfig &config)
{
	LOG(INFO) << "Initialize DPDK EAL environment";
	std::vector<char *> dpdk_arg;
	// Avoid the conflict between multiple DPDK applications running inside containers
	pid_t cur_pid = getpid();
	auto file_prefix = fmt::format("FFPP_CNF_{}", cur_pid);
	// Boilerplate code... Maybe there's better way of doing it...
	std::string vdev_conf = "net_af_packet0,iface=eth0";
	const char *rte_argv[] = {
		"-l",
		fmt::format("{}", fmt::join(config.lcore_ids, ",")).c_str(),
		"--main-lcore",
		fmt::format("{}", config.main_lcore_id).c_str(),
		"-m",
		fmt::format("{}", config.memory_mb).c_str(),
		"--no-pci",
		fmt::format("--file-prefix={}", file_prefix).c_str(),
		"--vdev",
		config.in_vdev_cfg.c_str(),
		"--vdev",
		config.out_vdev_cfg.c_str(),
		nullptr
	};
	int rte_argc =
		static_cast<int>(sizeof(rte_argv) / sizeof(rte_argv[0])) - 1;

	auto ret = rte_eal_init(rte_argc, const_cast<char **>(rte_argv));
	// MARK: It's not exception safe... Just panic and terminate...
	if (ret < 0) {
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
	}
}

CNF::CNF(std::string config_file_path) : config_file_path_(config_file_path)
{
	// TODO: Move logging stuff somewhere else
	FLAGS_logtostderr = true;
	FLAGS_colorlogtostderr = true;
	FLAGS_minloglevel = 0;
	FLAGS_stderrthreshold = 0;

	google::InitGoogleLogging("FFPP-CNF");

	load_config_file(config_file_path);
	// MARK: Maybe the initilization of the EAL should be performed by an external global CNF manager, like OpenNetVM
	// For just fast prototyping currently, the EAL is initialized inside each container...
	init_eal(cnf_config_);

	// The lifetime of the interpreter is the same of the CNF object.
	if (cnf_config_.start_python_interpreter) {
		LOG(INFO) << "Start the embeded Python interpreter";
		py::initialize_interpreter();
		{
			py::print("Hello, World!");
		}
	}

	auto avail_vdev_num = rte_eth_dev_count_avail();
	LOG(INFO) << fmt::format("Number of available vdev devices: {}",
				 avail_vdev_num);

	LOG(INFO) << "Initialize DPDK's graph library";
}

CNF::~CNF()
{
	LOG(INFO) << "Cleanup DPDK EAL environment";
	rte_eal_cleanup();
	if (cnf_config_.start_python_interpreter) {
		LOG(INFO) << "Finalize the embeded Python interpreter";
		py::finalize_interpreter();
	}
}

void CNF::load_config_file(const std::string &config_file_path)
{
	LOG(INFO)
		<< fmt::format("Load configuration from {}", config_file_path);
	YAML::Node config = YAML::LoadFile(config_file_path);
	// Boilerplate code... Maybe there's better way of doing it...
	auto main_lcore_id = config["main_lcore_id"].as<uint32_t>();
	auto lcore_ids = config["lcore_ids"].as<std::vector<uint32_t> >();
	auto memory_mb = config["memory_mb"].as<uint32_t>();
	auto in_vdev_cfg = config["in_vdev_cfg"].as<std::string>();
	auto out_vdev_cfg = config["out_vdev_cfg"].as<std::string>();
	auto start_python_interpreter =
		config["start_python_interpreter"].as<bool>();

	if (std::find(lcore_ids.begin(), lcore_ids.end(), main_lcore_id) ==
	    lcore_ids.end()) {
		throw std::runtime_error(
			"Lcore IDs must contain the main lcore ID!");
	}

	LOG(INFO) << fmt::format("The main lcore id: {}", main_lcore_id);
	LOG(INFO)
		<< fmt::format("The lcore ids: {}", fmt::join(lcore_ids, ","));
	LOG(INFO) << fmt::format("The pre-allocated memory: {} MB", memory_mb);
	LOG(INFO) << fmt::format("The ingress vdev: {}, the egress vdev: {}",
				 in_vdev_cfg, out_vdev_cfg);

	cnf_config_.main_lcore_id = main_lcore_id;
	cnf_config_.lcore_ids = lcore_ids;
	cnf_config_.memory_mb = memory_mb;
	cnf_config_.in_vdev_cfg = in_vdev_cfg;
	cnf_config_.out_vdev_cfg = out_vdev_cfg;
	cnf_config_.start_python_interpreter = start_python_interpreter;
}

} // namespace ffpp