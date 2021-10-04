project('coin_yolo', 'cpp',
	version : '0.0.1',
	default_options : [
		'warning_level=2',
		'cpp_std=c++2a',
		'buildtype=release',
		'werror=true',
	]
)

host_system = host_machine.system()
cppc = meson.get_compiler('cpp')

cpp_flags = [
	'-Wno-missing-field-initializers',
    '-Wno-deprecated-declarations',
    '-Wno-unused-but-set-variable',
    '-Wno-unused-parameter',
    '-Wno-unused-value',
    '-Wno-unused-variable',
]

add_project_arguments(
    cpp_flags,
    language: 'cpp',
)

ffpp_dep = dependency('libffpp', required: true)
boost_dep = dependency('boost')
boost_dep_modules = dependency('boost', modules : ['program_options'], required: true)
glog_dep = dependency('libglog', required: true)
fmt_dep = dependency('fmt', required: true)
libfccp_inc = include_directories('/usr/include/libfccp')
libfccp_dep = declare_dependency(include_directories : libfccp_inc)

dep_list = [
	boost_dep,
	boost_dep_modules,
	ffpp_dep,
	fmt_dep,
	glog_dep,
	libfccp_dep,
]

all_deps = declare_dependency(
	dependencies: dep_list,
)

executable('coin_yolo',
	'coin_yolo.cpp',
	dependencies:all_deps,
	install : false
)
