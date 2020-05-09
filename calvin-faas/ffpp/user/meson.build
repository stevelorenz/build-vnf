project('ffpp', 'c',
  version: '0.0.1',
  license: 'MIT',
  meson_version: '>= 0.47.1',
  default_options: ['buildtype=release'],
)

# Find DPDK dependency using pkg-config (by default.)
dpdk_dep = dependency('libdpdk')
bpf_dep = dependency('libbpf')

inc = include_directories('include')

subdir('include')
subdir('src')
if get_option('tests')
  subdir('tests/unit')
endif

pkg_mod = import('pkgconfig')
pkg_mod.generate(
  name: meson.project_name(),
  filebase: 'lib' + meson.project_name().to_lower(),
  libraries : [ffpplib_shared, ffpplib_static],
  version: meson.project_version(),
  description : 'A Library for Fast Functional Packet Processing.'
)
