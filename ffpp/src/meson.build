ffpp_sources = [
	'bpf_helpers_user.c',
	'general_helpers_user.c',
	'scaling_helpers_user.c',
	'utils.c',

	'graph.cpp',
	'mbuf_pdu.cpp',
	'packet_engine.cpp',
    'data_processor.cpp',
    'rtp.cpp',
]

ffpplib_static = static_library('ffpp',
	ffpp_sources,
	include_directories: inc,
	dependencies: ffpp_deps,
	install: true
)

ffpplib_shared = shared_library('ffpp',
	ffpp_sources,
	include_directories: inc,
	dependencies: ffpp_deps,
	install: true
)
