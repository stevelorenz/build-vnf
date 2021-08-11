ffpp_sources = [
  	# C sources
	'bpf_helpers_user.c',
  	'general_helpers_user.c',
  	'scaling_helpers_user.c',
  	'utils.c',
	
	# CPP sources
	'cnf.cpp',
	'graph.cpp',
	'packet_vector.cpp',
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
