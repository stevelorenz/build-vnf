ffpp_sources = [
  	# C sources
	'bpf_helpers_user.c',
  	'device.c',
  	'general_helpers_user.c',
  	'memory.c',
  	'scaling_helpers_user.c',
  	'utils.c',
	
	# CPP sources
	'graph.cpp',
	'main_thread.cpp',
]

ffpplib_shared = shared_library('ffpp',
  	ffpp_sources,
  	include_directories: inc,
  	dependencies: ffpp_deps,
  	install: true
)

ffpplib_static = static_library('ffpp',
  	ffpp_sources,
  	include_directories: inc,
  	dependencies: ffpp_deps,
  	install: true
)
