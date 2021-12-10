project('ffpp', 'c', 'cpp',
  version: run_command(find_program('cat', 'more'), files('./VERSION')).stdout().strip(),
  license: 'MIT',
  meson_version: '>= 0.53.0',
  default_options: [
    'c_std=gnu18',
    # C++20 is required mainly for C-style designated initializers.
    'cpp_std=c++2a',
    'buildtype=debugoptimized',
    # Come on! For me, all warnings are errors :)
    'warning_level=3',
    'werror=true',
    'default_library=both',
    'b_lundef=true',
    'b_ndebug=if-release',
    ],
  )

host_system = host_machine.system()

#
# Compiler Flags
#

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
  # Some DPDK C sources perform implicit pointer casting from (void*) to another pointer.
  '-fpermissive',
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

#
# Dependencies
#

# Build and runtime dependencies

# TODO: Remove unused dependencies. Use a RPC library.
# the runtime deps.
math_dep = cc.find_library('m', required: true)

boost_dep = dependency('boost', modules : ['program_options'], required: true)
boost_header_only_dep = dependency('boost', required: true)
dpdk_dep = dependency('libdpdk', required: true)
fmt_dep = dependency('fmt', required: true)
gbenchmark_dep = dependency('benchmark', required:true)
glog_dep = dependency('libglog', required: true)
json_dep = dependency('jansson', required: true)
libbpf_dep = dependency('libbpf', required: true)
libtins_dep = dependency('libtins', required: true)
libxdp_dep = dependency('libxdp', required: true)
python_embed_dep = dependency('python3-embed', required: true)
thread_dep = dependency('threads', required: true)
yaml_dep = dependency('yaml-cpp', required: true)
zmq_dep = dependency('libzmq', required: true)

cppc.has_header('gsl/gsl', required : true)
cppc.has_header('pybind11/pybind11.h', required : true)

subdir('deps')

ffpp_dep_list = [
  boost_dep,
  boost_header_only_dep,
  dpdk_dep,
  fmt_dep,
  gbenchmark_dep,
  glog_dep,
  json_dep,
  libbpf_dep,
  libtins_dep,
  math_dep,
  python_embed_dep,
  thread_dep,
  yaml_dep,
  zmq_dep,
  ]

ffpp_deps = declare_dependency(
  dependencies: ffpp_dep_list,
  )

# TODO: Consider adding Rust toolchain and FFI support DPDK and eBPF programming.

# Development and test dependencies

gtest_nomain_dep = dependency('gtest', main: false, required: false)
gtest_withmain_dep = dependency('gtest', main: true, required: false)


#
# FFPP Source Tree
#

inc = include_directories('include')
subdir('include')
subdir('src')

subdir('benchmark')
subdir('kernel')

if get_option('examples')
  subdir('examples')
endif

if get_option('tests')
  subdir('tests')
endif

if get_option('related_works')
  subdir('related_works')
endif

#
# Pkg-Config
#

pkg_mod = import('pkgconfig')
pkg_mod.generate(
  name: meson.project_name(),
  filebase: 'lib' + meson.project_name().to_lower(),
  libraries : [ffpplib_shared, ffpplib_static],
  version: meson.project_version(),
  description : 'Library for Fast Forward Packet Processing.'
  )

#
# Run Targets
#

run_target('cppcheck', command: [
  'cppcheck','--enable=warning,style,performance,information',
  '--project=' + join_paths(meson.source_root(), 'build', 'compile_commands.json')
  ])

#
# Log and Message
#

message('\n'.join([
  '',
  'Build Info:',
  '        Host system:                  ' + host_system,
  '        Source code location:         ' + meson.source_root(),
  '        Build directory location:     ' + meson.build_root(),
  '        C compiler:                   ' + cc.get_id(),
  '        CPP compiler:                 ' + cppc.get_id(),
  '        Build type:                   ' + get_option('buildtype'),
  '',
  ]))
