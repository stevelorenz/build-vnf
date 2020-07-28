project('ffpp', 'c',
  version: run_command(find_program('cat', 'more'),files('../VERSION')).stdout().strip(),
  license: 'MIT',
  meson_version: '>= 0.53.0',
  default_options: [
    'c_std=gnu99',
    'buildtype=release',
    'warning_level=3',
    'default_library=both',
    ],
)

add_project_arguments(
    [
        '-Wall',
        '-Wextra',
        '-Wundef',
        '-Wvla',

        # ABI stability of DPDK is not considered here.
        # Because the test container always uses a specific DPDK version.
        '-Wno-deprecated-declarations',
    ],
    language: 'c',
)

if get_option('buildtype').startswith('debug')
  add_project_arguments('-DDEBUG', language : 'c')
endif

cc = meson.get_compiler('c')

# Find math.h dependency
math_dep = cc.find_library('m')

# Find DPDK dependency using pkg-config (by default.)
dpdk_dep = dependency('libdpdk')

bpf_dep = dependency('libbpf')
zmq_dep = dependency('libzmq')
json_dep = dependency('jansson')
inc = include_directories('include')

ffpp_deps = [
  bpf_dep,
  dpdk_dep,
  json_dep,
  math_dep,
  zmq_dep,
]

ffpp_dep = declare_dependency(
  dependencies: ffpp_deps,
)

subdir('include')
subdir('src')

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
