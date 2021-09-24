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

#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <functional>

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
	PacketEngine(const std::string &config_file_path);
	~PacketEngine();

	// WARNING: The std::vector is used here for prototyping... Maybe a io_ring based on rte_ring would be added for
	// better performance.
	uint32_t rx_pkts(std::vector<struct rte_mbuf *> &vec,
			 uint32_t max_num_burst = 3);

	void tx_pkts(std::vector<struct rte_mbuf *> &vec,
		     std::chrono::microseconds burst_gap);

	/**
	 * Try/learn how to use rte_graph correctly. Then corresponded functionalities should be moved to ffpp/graph.
	 * MARK: I tried to learn rte_graph through documentation and l3fwd-graph example. I feel that more time is needed
	 * for me to adopt it into the codebase... So currently for quick prototyping, I'll "hard-code" the processing methods...
	 * Later I'll learn try to refactor them using rte_graph 
	 */
	// void init_graph(void);

	void
	register_read_apu_cb(std::function<std::string(struct rte_mbuf *)> cb)
	{
		cb_read_apu_ = cb;
	}

    private:
	PacketEngine();
	struct PEConfig pe_config_;
	std::function<std::string(struct rte_mbuf *)> cb_read_apu_;
};

} // namespace ffpp
