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
#include <chrono>
#include <tuple>
#include <unistd.h>
#include <gsl/gsl>
#include <fmt/core.h>
#include <fmt/format.h>
#include <glog/logging.h>
#include <yaml-cpp/yaml.h>
#include <pybind11/embed.h> // NOLINT
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_cycles.h>

#include "ffpp/graph.hpp"
#include "ffpp/packet_engine.hpp"

namespace py = pybind11;

namespace ffpp
{
uint16_t kRXDescDefault = 1024;
uint16_t kTXDescDefault = 1024;

constexpr uint32_t kComputePktNum = 32;
constexpr uint32_t kMempoolCacheSize = 256;

constexpr uint32_t kRxTxPortID = 0;

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
		      struct PEConfig &pe_config)
{
	LOG(INFO)
		<< fmt::format("Load configuration from {}", config_file_path);
	YAML::Node config = YAML::LoadFile(config_file_path);
	// Boilerplate code... Maybe there's better way of doing it...
	pe_config.loglevel = config["loglevel"].as<std::string>();
	pe_config.main_lcore_id = config["main_lcore_id"].as<uint32_t>();
	pe_config.lcore_ids = config["lcore_ids"].as<std::vector<uint32_t> >();
	pe_config.memory_mb = config["memory_mb"].as<uint32_t>();
	pe_config.data_vdev_cfg = config["data_vdev_cfg"].as<std::string>();
	pe_config.use_null_pmd = config["use_null_pmd"].as<bool>();
	pe_config.null_pmd_packet_size =
		config["null_pmd_packet_size"].as<uint32_t>();

	if (pe_config.lcore_ids.size() != 1) {
		throw std::runtime_error(
			"The support for multiple worker lcores is not implemented yet!");
	}

	if (std::find(pe_config.lcore_ids.begin(), pe_config.lcore_ids.end(),
		      pe_config.main_lcore_id) == pe_config.lcore_ids.end()) {
		throw std::runtime_error(
			"Lcore IDs must contain the main lcore ID!");
	}
}

void config_glog(const std::string &loglevel)
{
	google::InitGoogleLogging("PacketEngine");
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

void log_config(const struct PEConfig &pe_config)
{
	LOG(INFO) << fmt::format("The main lcore id: {}",
				 pe_config.main_lcore_id);
	LOG(INFO) << fmt::format("The lcore ids: {}",
				 fmt::join(pe_config.lcore_ids, ","));
	LOG(INFO) << fmt::format("The pre-allocated hugepage memory: {} MB",
				 pe_config.memory_mb);
	if (not pe_config.use_null_pmd) {
		LOG(INFO) << fmt::format("The data plane vdev: {}",
					 pe_config.data_vdev_cfg);
	} else {
		LOG(INFO) << fmt::format(
			"The null PMD (with packet size {}B) is used for local testing. in_dev_cfg and out_dev_cfg are ignored",
			pe_config.null_pmd_packet_size);
	}
}

__attribute__((no_sanitize_address)) void init_eal(struct PEConfig &pe_config)
{
	LOG(INFO) << "Initialize DPDK EAL environment";
	std::vector<char *> dpdk_arg;
	auto file_prefix = pe_config.id;

	if (pe_config.use_null_pmd) {
		pe_config.data_vdev_cfg.assign(fmt::format(
			"net_null0,size={}", pe_config.null_pmd_packet_size));
	}

	// Boilerplate code... Maybe there's better way of doing it...
	const char *rte_argv[] = {
		fmt::format("-l {}", fmt::join(pe_config.lcore_ids, ","))
			.c_str(),
		"--main-lcore",
		fmt::format("{}", pe_config.main_lcore_id).c_str(), "-m",
		fmt::format("{}", pe_config.memory_mb).c_str(),
		// Following options are enabled to make the application "cloud-native" as much as possible.
		// clang-format off
		fmt::format("--file-prefix={}", pe_config.id).c_str(),
		"--vdev",
		pe_config.data_vdev_cfg.c_str(),
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
		nb_vdev_num * (kRXDescDefault + kTXDescDefault + kMaxBurstSize +
			       kComputePktNum + kMempoolCacheSize),
		8192U);
	std::string pool_name = fmt::format("pool_{}", id);
	LOG(INFO) << fmt::format(
		"Initialize the memory pool: {}. Number of mbufs in the pool: {}",
		pool_name, nb_mbufs);
	pool_ = rte_pktmbuf_pool_create(pool_name.c_str(), nb_mbufs,
					kMempoolCacheSize, 0,
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
	LOG(INFO) << fmt::format(
		"Number of available data plane vdev devices: {}",
		avail_vdev_num);
	if (avail_vdev_num != 1) {
		throw std::runtime_error(
			"The number of data plane network devices CURRENTLY must be 1 !");
	}

	uint32_t vdev_id = 0;

	// Assume there's only one queue per vdev.
	// This is typical for pure software NIC like a veth pair
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
		};
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

void init_all(struct PEConfig &pe_config)
{
	pid_t cur_pid = getpid();
	pe_config.id = fmt::format("pe_{}", cur_pid);
	log_config(pe_config);
	config_glog(pe_config.loglevel);
	init_eal(pe_config);
	init_mempools(pe_config.id);
	init_vdevs();

	LOG(INFO) << "Run the embeded Python interpreter.";
	py::initialize_interpreter();
	{
		auto sys = py::module::import("sys");
		py::print("[PY] interpreter is already initialized");
	}
}

PacketEngine::PacketEngine(const struct PEConfig pe_config)
{
	pe_config_ = pe_config;
	init_all(pe_config_);
}

PacketEngine::PacketEngine(const std::string &config_file_path)
{
	load_config_file(config_file_path, pe_config_);
	init_all(pe_config_);
}

PacketEngine::~PacketEngine()
{
	if (pool_ != nullptr) {
		LOG(INFO) << "Free the memory pool";
		rte_mempool_free(pool_);
	}
	LOG(INFO) << "Cleanup DPDK EAL environment";
	rte_eal_cleanup();

	LOG(INFO) << "Stop embeded Python interpreter.";
	py::finalize_interpreter();

	google::ShutdownGoogleLogging();
}

uint32_t PacketEngine::rx_pkts(packet_vector &vec, uint32_t max_num_burst)
{
	uint32_t num_pkts_rx = 0;
	uint32_t num_pkts_burst = 0;
	uint32_t i = 0;
	uint32_t j = 0;

	struct rte_mbuf *mbuf_burst[kMaxBurstSize];

	for (i = 0; i < max_num_burst; i++) {
		num_pkts_burst = rte_eth_rx_burst(kRxTxPortID, 0, mbuf_burst,
						  kMaxBurstSize);
		if (num_pkts_burst == 0) {
			continue;
		}
		for (j = 0; j < num_pkts_burst; j++) {
			vec.push_back(mbuf_burst[j]);
		}
		num_pkts_rx += num_pkts_burst;

		if (num_pkts_burst < kMaxBurstSize) {
			break;
		}
	}
	VLOG_IF(kDefaultVlogNum, (i == max_num_burst))
		<< "[RX] Hit maximal number of bursts.";

	return num_pkts_rx;
}

void PacketEngine::tx_pkts(packet_vector &vec,
			   std::chrono::microseconds burst_gap)
{
	struct rte_mbuf *mbuf_burst[kMaxBurstSize]; // on stack
	uint32_t num_full_burst = uint32_t(vec.size()) / kMaxBurstSize;
	uint32_t rest_burst = uint32_t(vec.size()) % kMaxBurstSize;
	uint32_t i = 0;
	uint32_t j = 0;

	LOG(INFO) << "Full burst:" << num_full_burst
		  << ", Rest burst:" << rest_burst;

	uint32_t num_pkts_tx = 0;
	// Send all full bursts
	for (i = 0; i < num_full_burst; i++) {
		num_pkts_tx = 0;
		for (j = 0; j < kMaxBurstSize; j++) {
			// Prepare the burst to send
			mbuf_burst[j] = vec[j + i * kMaxBurstSize];
		}
		while (num_pkts_tx < kMaxBurstSize) {
			num_pkts_tx +=
				rte_eth_tx_burst(kRxTxPortID, 0,
						 mbuf_burst + num_pkts_tx,
						 kMaxBurstSize - num_pkts_tx);
		}
		rte_delay_us_block(burst_gap.count());
	}

	// Send the last burst
	if (unlikely(rest_burst > 0)) {
		num_pkts_tx = 0;

		for (j = 0; j < rest_burst; ++j) {
			mbuf_burst[j] = vec[j + num_full_burst * kMaxBurstSize];
		}

		while (num_pkts_tx < rest_burst) {
			num_pkts_tx +=
				rte_eth_tx_burst(kRxTxPortID, 0,
						 mbuf_burst + num_pkts_tx,
						 rest_burst - num_pkts_tx);
		}
		rte_delay_us_block(burst_gap.count());
		VLOG(kDefaultVlogNum) << fmt::format(
			"[RestBurst] Number of tx packets: {}", num_pkts_tx);
	}

	vec.clear();
}

} // namespace ffpp
