project('ffpp', 'c',
  version: run_command(find_program('cat', 'more'),files('../VERSION')).stdout().strip(),
  license: 'MIT',
  meson_version: '>= 0.53.0',
  default_options: [
    'c_std=gnu99',
    'buildtype=release',
    'warning_level=3',
    'default_library=both',
    # TODO: Enable it to avoid any warnings.
    # 'werror=true',
  ],
)

add_project_arguments(
    [
        '-Wformat-nonliteral',
        '-Wformat-security',
        '-Wnested-externs',
        '-Wpointer-arith',
        '-Wsign-compare',
        '-Wundef',
        '-Wvla',
        '-Wwrite-strings',

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

inc = include_directories('include')

math_dep = cc.find_library('m', required: true)

bpf_dep = dependency('libbpf', required: true)
dpdk_dep = dependency('libdpdk', required: true)
json_dep = dependency('jansson', required: false)
zmq_dep = dependency('libzmq', required: false)

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
