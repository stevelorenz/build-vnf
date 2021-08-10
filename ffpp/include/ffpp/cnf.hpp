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

#include <vector>
#include <string>

#include <pybind11/embed.h>
#include <pybind11/stl.h>

namespace py = pybind11;

namespace ffpp
{
struct CNFConfig {
	// Important EAL parameters
	uint32_t main_lcore_id;
	std::vector<uint32_t> lcore_ids;
	uint32_t memory_mb;

	std::string proce_type;

	// Assume each CNF only handles two virtual interfaces.
	std::string in_vdev_cfg;
	std::string out_vdev_cfg;

	std::string eal_log_level;
	std::string file_prefix;

	bool start_python_interpreter;
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

    private:
	std::string config_file_path_;
	struct CNFConfig cnf_config_;

	void load_config_file(const std::string &config_file_path);
};

} // namespace ffpp

#endif /* FFPP_CNF_HPP */
