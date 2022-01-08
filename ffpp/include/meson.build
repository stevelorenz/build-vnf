# Maybe, somewhen, I can use modules and avoid hpp headers files for C++ code ;)

ffpp_headers = files(
	'ffpp/bpf_defines_user.h',
	'ffpp/bpf_helpers_user.h',
	'ffpp/config.h',
	'ffpp/general_helpers_user.h',
	'ffpp/global_stats_user.h',
	'ffpp/scaling_defines_user.h',
	'ffpp/scaling_helpers_user.h',
	'ffpp/utils.h',

    'ffpp/ffpp.hpp',

	'ffpp/graph.hpp',
	'ffpp/mbuf_pdu.hpp',
    'ffpp/data_processor.hpp',
    'ffpp/packet_engine.hpp',
    'ffpp/packet_ring.hpp',
    'ffpp/rtp.hpp',
)

install_headers(ffpp_headers, subdir: 'ffpp')
