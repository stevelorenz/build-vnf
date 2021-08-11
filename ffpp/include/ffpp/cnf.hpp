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

#ifndef FFPP_CNF_HPP
#define FFPP_CNF_HPP

#include <memory>
#include <string>
#include <vector>

namespace ffpp
{
/**
 * CNF configuration
 */
struct CNFConfig {
	std::string id;
	// Important EAL parameters
	uint32_t main_lcore_id;
	std::vector<uint32_t> lcore_ids;
	uint32_t memory_mb;

	std::string proce_type;

	// Assume each CNF only handles two virtual interfaces.
	std::string in_vdev_cfg;
	std::string out_vdev_cfg;

	std::string eal_log_level;

	bool start_python_interpreter;
	bool use_null_pmd;
	uint32_t null_pmd_packet_size;

	std::string loglevel;
};
/**
 * Containerized (or Cloud-native) Network Function (CNF)
 */
class CNF {
    public:
	/**
	 * Initialize a CNF with the given configuration file.
	 * The initilization process contains the initlization of EAL, memory pools, network ports.
	 * 
	 * @param config_file_path 
	 */
	CNF(std::string config_file_path);
	~CNF();

	/**
	 * Try/learn how to use rte_graph correctly. Then corresponded functionalities should be moved to ffpp/graph.
	 */
	void init_graph(void);

    private:
	struct CNFConfig cnf_config_;
};

} // namespace ffpp

#endif /* FFPP_CNF_HPP */
