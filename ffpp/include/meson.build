ffpp_headers = files(
  	# C sources, most are related to eBPF/XDP
  	'ffpp/bpf_defines_user.h',
  	'ffpp/bpf_helpers_user.h',
  	'ffpp/config.h',
  	'ffpp/general_helpers_user.h',
  	'ffpp/global_stats_user.h',
  	'ffpp/scaling_defines_user.h',
  	'ffpp/scaling_helpers_user.h',
  	'ffpp/utils.h',

  	# CPP sources
	'ffpp/cnf.hpp',
	'ffpp/graph.hpp',
	'ffpp/packet_vector.hpp',
	'ffpp/utils.hpp',
)

install_headers(ffpp_headers, subdir: 'ffpp')
