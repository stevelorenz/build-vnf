if get_option('ffpppy')
	ffpppy_lib_shared = shared_library('ffpppy',
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