/*
 * ebpf_map_rw_latency.cpp
 */

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#include <argparse.hpp>
#include <ffpp/bpf_helpers_user.h>

using namespace std;

static void stats_poll(const vector<int> &map_fds)
{
	struct stats_record record = { 0 };
	uint64_t rx_packets = 0;
	for (auto fd : map_fds) {
		map_collect(fd, 0, &record.stats);
		rx_packets = record.stats.total.rx_packets;
		rx_packets++;
	}
}

int main(int argc, char *argv[])
{
	argparse::ArgumentParser program("ebpf_map_rw_latency");
	program.add_argument("veth_num")
		.help("Number of veth pairs.")
		.action([](const std::string &value) {
			return std::stoi(value);
		});
	program.add_argument("test_rounds")
		.help("Number of test rounds.")
		.action([](const std::string &value) {
			return std::stoi(value);
		});

	try {
		program.parse_args(argc, argv);
	} catch (const std::runtime_error &err) {
		std::cout << err.what() << std::endl;
		std::cout << program;
		exit(0);
	}

	int veth_num = program.get<int>("veth_num");
	int test_rounds = program.get<int>("test_rounds");

	vector<string> veth_names;
	cout << "Veth pair lists: ";
	for (auto i = 0; i < veth_num; ++i) {
		veth_names.push_back("r" + to_string(i));
	}
	for (auto v : veth_names) {
		cout << v << ",";
	}
	cout << endl;

	struct bpf_map_info xdp_stats_map_info = { 0 };
	const struct bpf_map_info xdp_stats_map_expect = {
		.key_size = sizeof(__u32),
		.value_size = sizeof(struct datarec),
		.max_entries = 1,
	};

	int xdp_stats_map_fd;
	vector<int> xdp_stats_map_fds;
	for (auto v : veth_names) {
		string pin_dir = "/sys/fs/bpf/" + v;
		xdp_stats_map_fd = open_bpf_map_file(
			pin_dir.c_str(), "xdp_stats_map", &xdp_stats_map_info);
		if (xdp_stats_map_fd < 0) {
			cerr << "Can not open the XDP stats map.\n";
			return -1;
		}
		xdp_stats_map_fds.push_back(xdp_stats_map_fd);
		int err = 0;
		err = check_map_fd_info(&xdp_stats_map_info,
					&xdp_stats_map_expect);
		if (err) {
			cerr << "XDP stats map via FD not compatible.\n";
			return -1;
		}
		cout << "Successfully open the map file on interface: " << v
		     << endl;
	}

	vector<double> monitor_latencies;
	for (auto r = 0; r < test_rounds; r++) {
		auto t1 = chrono::high_resolution_clock::now();
		stats_poll(xdp_stats_map_fds);
		auto t2 = chrono::high_resolution_clock::now();

		auto duration =
			chrono::duration_cast<std::chrono::microseconds>(t2 -
									 t1)
				.count();
		monitor_latencies.push_back(duration);
	}

	fstream csv_file;
	string csv_file_name =
		"./monitor_latencies_" + to_string(veth_num) + ".csv";
	csv_file.open(csv_file_name, fstream::out | fstream::app);
	cout << fixed << setprecision(17) << endl;
	csv_file << fixed << setprecision(17) << endl;

	for (auto t : monitor_latencies) {
		csv_file << t << ",";
	}

	csv_file << endl;
	csv_file.close();

	return 0;
}
