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

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include <rte_mbuf.h>

namespace ffpp
{
constexpr uint32_t kMaxBurstSize = 32;

struct PEConfig {
	std::string id;
	// Important EAL parameters
	uint32_t main_lcore_id;
	std::vector<uint32_t> lcore_ids;
	uint32_t memory_mb;

	std::string proce_type;

	std::string data_vdev_cfg;

	std::string eal_log_level;

	bool use_null_pmd;
	uint32_t null_pmd_packet_size;

	std::string loglevel;
};

class PacketEngine {
    public:
	using packet_vector = std::vector<struct rte_mbuf *>;

	PacketEngine(const struct PEConfig pe_config);
	PacketEngine(const std::string &config_file_path);
	~PacketEngine();

	PacketEngine(const PacketEngine &) = delete;
	PacketEngine &operator=(const PacketEngine &) = delete;

	PacketEngine(const PacketEngine &&) = delete;
	PacketEngine &operator=(const PacketEngine &&) = delete;

	/**
	 * @brief rx_pkts 
	 *
	 * @param vec
	 * @param max_num_burst
	 *
	 * @return 
	 */
	uint32_t rx_pkts(packet_vector &vec, uint32_t max_num_burst = 3);

	/**
	 * @brief tx_pkts 
	 *
	 * @param vec
	 * @param burst_gap
	 */
	void tx_pkts(packet_vector &vec, std::chrono::microseconds burst_gap);

	/**
	 * Try/learn how to use rte_graph correctly. Then corresponded functionalities should be moved to ffpp/graph.
	 * MARK: I tried to learn rte_graph through documentation and l3fwd-graph example. I feel that more time is needed
	 * for me to adopt it into the codebase... So currently for quick prototyping, I'll "hard-code" the processing methods...
	 * Later I'll learn try to refactor them using rte_graph 
	 */
	// void init_graph(void);

    private:
	PacketEngine();
	struct PEConfig pe_config_;
};

} // namespace ffpp
