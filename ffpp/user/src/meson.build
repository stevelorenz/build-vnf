ffpp_sources = [
  	# C sources
	'bpf_helpers_user.c',
  	'general_helpers_user.c',
  	'scaling_helpers_user.c',
  	'utils.c',
	
	# CPP sources
	'controller.cpp',
	'graph.cpp',
	'packet_vector.cpp',
	'utils.cpp',
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

if get_option('pyffpp')
	pyffpplib_shared = shared_library('pyffpp',
		link_with : ffpplib_static,
		# This is required to install ffpp.so instead of libffpp.so,
		# so it can be `import ffpp`
		name_prefix : '',
		sources : [
			'python/bindings.cpp',
		],
		dependencies : [
			ffpp_deps,
		],
		install : true, install_dir : import('python3').sysconfig_path('platlib'))
endif
