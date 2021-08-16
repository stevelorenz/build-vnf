project('ffpp', 'c', 'cpp',
	version: run_command(find_program('cat', 'more'),files('./VERSION')).stdout().strip(),
	license: 'MIT',
	meson_version: '>= 0.53.0',
	default_options: [
		'c_std=gnu18',
		# C++20 is used for C-style designated initializers.
		'cpp_std=c++2a',
		'buildtype=release',
		'warning_level=2',
		'default_library=both',
		'werror=true',
		'b_lundef=true',
	],
)

host_system = host_machine.system()

cc = meson.get_compiler('c')
cppc = meson.get_compiler('cpp')

c_flags = [
	# ABI stability of DPDK is not considered here.
	# Because the test container always uses a specific DPDK version.
	'-Wno-deprecated-declarations',
	# They will be used in 6G and beyond :)
	'-Wno-unused-value',
	'-Wno-unused-variable',
	'-Wno-unused-but-set-variable',
]

cpp_flags = [
	# When including DPDK headers, the following warnings will be disabled to
	# allow the code to compile...
	'-Wno-implicit-fallthrough',
	# ABI stability of DPDK is not considered here.
	# Because the test container always uses a specific DPDK version.
	'-Wno-deprecated-declarations',
	# They will be used in 6G and beyond :)
	'-Wno-unused-value',
	'-Wno-unused-variable',
	'-Wno-unused-parameter',
	'-Wno-unused-but-set-variable',
	'-Wno-missing-field-initializers',
]


add_project_arguments(
	c_flags,
	language: 'c',
)

add_project_arguments(
	cpp_flags,
	language: 'cpp',
)

if get_option('buildtype').startswith('debug')
	add_project_arguments('-DDEBUG', language : ['c', 'cpp'])
endif

inc = include_directories('include')

# Dependencies

math_dep = cc.find_library('m', required: true)

thread_dep = dependency('threads')
bpf_dep = dependency('libbpf', required: true)
dpdk_dep = dependency('libdpdk', required: true)
json_dep = dependency('jansson', required: true)
python_embed_dep = dependency('python3-embed', required: true)
zmq_dep = dependency('libzmq', required: true)
libtins_dep = dependency('libtins', required: true)
glog_dep = dependency('libglog', required: true)
fmt_dep = dependency('fmt', required: true)
yaml_dep = dependency('yaml-cpp', required: true)
boost_dep = dependency('boost', required: true)
boost_dep_modules = dependency('boost', modules : ['program_options'], required: true)
gtest_nomain_dep = dependency('gtest', main: false)
gtest_withmain_dep = dependency('gtest', main: true)

pybind11_inc = include_directories('/usr/include/pybind11')
pybind11_dep = declare_dependency(include_directories : pybind11_inc)
libfccp_inc = include_directories('/usr/include/libfccp')
libfccp_dep = declare_dependency(include_directories : libfccp_inc)
ms_gsl_inc = include_directories('/usr/include/gsl')
ms_gsl_dep = declare_dependency(include_directories: ms_gsl_inc)

ffpp_dep_list = [
	bpf_dep,
	dpdk_dep,
	json_dep,
	math_dep,
	thread_dep,
	python_embed_dep,
	zmq_dep,
	pybind11_dep,
	libtins_dep,
	boost_dep,
	boost_dep_modules,
	glog_dep,
	fmt_dep,
	yaml_dep,
	libfccp_dep,
	ms_gsl_dep,
]

ffpp_deps = declare_dependency(
	dependencies: ffpp_dep_list,
)

subdir('include')
subdir('src')
# TODO <Zuo>: Manage ./kern sources with Meson's custom target or native method.

if get_option('examples')
	subdir('examples')
endif

if get_option('tests')
	subdir('tests')
endif

if get_option('related_works')
	subdir('related_works')
endif

pkg_mod = import('pkgconfig')
pkg_mod.generate(
	name: meson.project_name(),
	filebase: 'lib' + meson.project_name().to_lower(),
	libraries : [ffpplib_shared, ffpplib_static],
	version: meson.project_version(),
	description : 'A Library for Fast Functional Packet Processing.'
)

run_target('cppcheck', command: [
	'cppcheck','--enable=warning,style,performance,information',
	'--project=' + join_paths(meson.source_root(), 'build', 'compile_commands.json')
])

message('\n'.join([
	'',
	'        Host system:                  ' + host_system,
	'        Source code location:         ' + meson.source_root(),
	'        C compiler:                   ' + cc.get_id(),
	'        CPP compiler:                 ' + cppc.get_id(),
	'        Debugging support:            ' + get_option('buildtype'),
	'',
]))
