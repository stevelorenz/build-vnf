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

#include <rte_ethdev.h>
#include <rte_mempool.h>

#include <fmt/core.h>
#include <fmt/format.h>
#include <glog/logging.h>
#include <yaml-cpp/yaml.h>
#include <pybind11/embed.h>

#include "ffpp/graph.hpp"
#include "ffpp/cnf.hpp"

namespace py = pybind11;

namespace ffpp
{
uint16_t kRXDescDefault = 1024;
uint16_t kTXDescDefault = 1024;

constexpr uint32_t kMaxPktBurst = 32;
// MARK
constexpr uint32_t kComputePktNum = 32;
constexpr uint32_t kMempoolCacheSize = 256;

constexpr uint8_t kDefaultVlogNum = 1;

static struct rte_mempool *pool_ = nullptr;

static struct rte_eth_conf sVdevConf = {
	.rxmode = {
		.split_hdr_size = 0,
	},
	.txmode = {
		.mq_mode = ETH_MQ_TX_NONE,
	}
};

void load_config_file(const std::string &config_file_path,
		      struct CNFConfig &cnf_config)
{
	LOG(INFO)
		<< fmt::format("Load configuration from {}", config_file_path);
	YAML::Node config = YAML::LoadFile(config_file_path);
	// Boilerplate code... Maybe there's better way of doing it...
	cnf_config.loglevel = config["loglevel"].as<std::string>();
	cnf_config.main_lcore_id = config["main_lcore_id"].as<uint32_t>();
	cnf_config.lcore_ids = config["lcore_ids"].as<std::vector<uint32_t> >();
	cnf_config.memory_mb = config["memory_mb"].as<uint32_t>();
	cnf_config.in_vdev_cfg = config["in_vdev_cfg"].as<std::string>();
	cnf_config.out_vdev_cfg = config["out_vdev_cfg"].as<std::string>();
	cnf_config.start_python_interpreter =
		config["start_python_interpreter"].as<bool>();
	cnf_config.use_null_pmd = config["use_null_pmd"].as<bool>();
	cnf_config.null_pmd_packet_size =
		config["null_pmd_packet_size"].as<uint32_t>();

	if (cnf_config.lcore_ids.size() != 1) {
		throw std::runtime_error(
			"The support for multiple worker lcores is not implemented yet!");
	}

	if (std::find(cnf_config.lcore_ids.begin(), cnf_config.lcore_ids.end(),
		      cnf_config.main_lcore_id) == cnf_config.lcore_ids.end()) {
		throw std::runtime_error(
			"Lcore IDs must contain the main lcore ID!");
	}
}

void config_glog(const std::string &loglevel)
{
	google::InitGoogleLogging("FFPP-CNF");
	FLAGS_logtostderr = true;
	FLAGS_colorlogtostderr = true;
	FLAGS_stderrthreshold = 0;

	if (loglevel == "INFO") {
		FLAGS_minloglevel = 0;
		FLAGS_v = 0;
	} else if (loglevel == "DEBUG") {
		FLAGS_minloglevel = 0;
		FLAGS_v = 3;
	} else if (loglevel == "ERROR") {
		FLAGS_minloglevel = 2;
		FLAGS_v = 0;
	} else {
		throw std::runtime_error("Unknown logging level");
	}
}

void log_cnf_config(const struct CNFConfig &cnf_config)
{
	LOG(INFO) << fmt::format("The main lcore id: {}",
				 cnf_config.main_lcore_id);
	LOG(INFO) << fmt::format("The lcore ids: {}",
				 fmt::join(cnf_config.lcore_ids, ","));
	LOG(INFO) << fmt::format("The pre-allocated memory: {} MB",
				 cnf_config.memory_mb);
	if (not cnf_config.use_null_pmd) {
		LOG(INFO) << fmt::format(
			"The ingress vdev: {}, the egress vdev: {}",
			cnf_config.in_vdev_cfg, cnf_config.out_vdev_cfg);
	} else {
		LOG(INFO) << fmt::format(
			"The null PMD (with packet size {}B) is used for local testing. in_dev_cfg and out_dev_cfg are ignored",
			cnf_config.null_pmd_packet_size);
	}
}

void init_eal(struct CNFConfig &cnf_config)
{
	LOG(INFO) << "Initialize DPDK EAL environment";
	std::vector<char *> dpdk_arg;
	auto file_prefix = cnf_config.id;

	if (cnf_config.use_null_pmd) {
		cnf_config.in_vdev_cfg.assign(fmt::format(
			"net_null0,size={}", cnf_config.null_pmd_packet_size));
		cnf_config.out_vdev_cfg.assign(fmt::format(
			"net_null1,size={}", cnf_config.null_pmd_packet_size));
	}

	// Boilerplate code... Maybe there's better way of doing it...
	const char *rte_argv[] = {
		"-l",
		fmt::format("{}", fmt::join(cnf_config.lcore_ids, ",")).c_str(),
		"--main-lcore",
		fmt::format("{}", cnf_config.main_lcore_id).c_str(), "-m",
		fmt::format("{}", cnf_config.memory_mb).c_str(),
		// Following options are enabled to make the application "cloud-native" as much as possible.
		// clang-format off
		fmt::format("--file-prefix={}", cnf_config.id).c_str(),
		"--vdev",
		cnf_config.in_vdev_cfg.c_str(),
		"--vdev",
		cnf_config.out_vdev_cfg.c_str(),
		"--no-pci",
		"--no-huge",
		nullptr,
		// clang-format on
	};
	int rte_argc =
		static_cast<int>(sizeof(rte_argv) / sizeof(rte_argv[0])) - 1;

	auto ret = rte_eal_init(rte_argc, const_cast<char **>(rte_argv));
	// MARK: It's not exception safe... Just panic and terminate...
	if (ret < 0) {
		throw std::runtime_error("Error with EAL initialization");
	}
}

void init_mempools(const std::string &id)
{
	auto nb_vdev_num = rte_eth_dev_count_avail();

	uint32_t nb_mbufs = RTE_MAX(
		nb_vdev_num * (kRXDescDefault + kTXDescDefault + kMaxPktBurst +
			       kComputePktNum + kMempoolCacheSize),
		8192U);
	LOG(INFO) << fmt::format(
		"Initialize the memory pool. Number of mbufs in the pool: {}",
		nb_mbufs);
	pool_ = rte_pktmbuf_pool_create(fmt::format("pool_{}", id).c_str(),
					nb_mbufs, kMempoolCacheSize, 0,
					RTE_MBUF_DEFAULT_BUF_SIZE,
					rte_socket_id());
	if (pool_ == nullptr) {
		throw std::runtime_error("Can not initialize the memory pool!");
	}
	LOG(INFO) << "The memory pool is successfully initialized.";
}

/**
 * The setup of vdevs/Ethernet ports are very tedious and verbose...
 */
void init_vdevs(void)
{
	LOG(INFO) << "Initialize (virtualized) network devices";
	auto avail_vdev_num = rte_eth_dev_count_avail();
	LOG(INFO) << fmt::format("Number of available vdev devices: {}",
				 avail_vdev_num);
	if (avail_vdev_num == 0 || avail_vdev_num > 2) {
		throw std::runtime_error(
			"The number of network devices must be 1 of 2!");
	}

	uint32_t vdev_id = 0;

	// Assume there's only one queue per vdev
	RTE_ETH_FOREACH_DEV(vdev_id)
	{
		struct rte_eth_dev_info dev_info;
		rte_eth_dev_info_get(vdev_id, &dev_info);
		LOG(INFO) << fmt::format("Configure vdev {} with driver: {}",
					 vdev_id, dev_info.driver_name);
		VLOG(kDefaultVlogNum) << fmt::format(
			"vdev {}: driver_name: {}, maximal RX queues: {}",
			vdev_id, dev_info.driver_name, dev_info.max_rx_queues);
		auto ret = rte_eth_dev_configure(vdev_id, 1, 1, &sVdevConf);
		if (ret < 0) {
			throw std::runtime_error(fmt::format(
				"Can not configure vdev={} with error code: {}",
				vdev_id, ret));
		}
		ret = rte_eth_dev_adjust_nb_rx_tx_desc(vdev_id, &kRXDescDefault,
						       &kTXDescDefault);
		if (ret < 0) {
			throw std::runtime_error(
				"Failed to adjust the RX/TX descriptor");
		}

		// MARK: ONLY one RX and TX queue is inited for each vdev
		auto rxq_conf = dev_info.default_rxconf;
		auto txq_conf = dev_info.default_txconf;
		ret = rte_eth_rx_queue_setup(vdev_id, 0, kRXDescDefault,
					     rte_eth_dev_socket_id(vdev_id),
					     &rxq_conf, pool_);
		if (ret < 0) {
			throw std::runtime_error(fmt::format(
				"Failed to setup the single RX queue on vdev: {}",
				vdev_id));
		}
		ret = rte_eth_tx_queue_setup(vdev_id, 0, kTXDescDefault,
					     rte_eth_dev_socket_id(vdev_id),
					     &txq_conf);
		if (ret < 0) {
			throw std::runtime_error(fmt::format(
				"Failed to setup the single TX queue on vdev: {}",
				vdev_id));
		}
		ret = rte_eth_dev_start(vdev_id);
		if (ret < 0) {
			throw std::runtime_error(fmt::format(
				"Failed to start vdev: {}", vdev_id));
		}
		ret = rte_eth_promiscuous_enable(vdev_id);
		if (ret < 0) {
			throw std::runtime_error(fmt::format(
				"Failed to enable promiscuous mode on vdev: {}",
				vdev_id));
		}
	}
	LOG(INFO) << "All vdevs are successfully configured and started.";
}

void init_embed_python(void)
{
	LOG(INFO) << "Initialize the embed Python interpreter";
	// The lifetime of the interpreter is the same of the CNF object.
	py::initialize_interpreter();
	{
		py::print("PY] The Python interpreter is started");
	}
}

CNF::CNF(std::string config_file_path)
{
	load_config_file(config_file_path, cnf_config_);
	pid_t cur_pid = getpid();
	cnf_config_.id = fmt::format("FFPP_CNF_{}", cur_pid);

	log_cnf_config(cnf_config_);
	config_glog(cnf_config_.loglevel);
	// MARK: Maybe the initilization of the EAL and others should be performed by an external global CNF manager, like OpenNetVM
	// For just fast prototyping currently, the EAL is initialized inside each container...
	init_eal(cnf_config_);
	init_mempools(cnf_config_.id);
	init_vdevs();

	if (cnf_config_.start_python_interpreter) {
		init_embed_python();
	}
}

CNF::~CNF()
{
	if (pool_ != nullptr) {
		LOG(INFO) << "Free the memory pool";
		rte_mempool_free(pool_);
	}
	LOG(INFO) << "Cleanup DPDK EAL environment";
	rte_eal_cleanup();
	if (cnf_config_.start_python_interpreter) {
		LOG(INFO) << "Finalize the embeded Python interpreter";
		py::finalize_interpreter();
	}
}

void CNF::init_graph(void)
{
	LOG(INFO) << "Initialize the packet processing graph.";
}

} // namespace ffpp