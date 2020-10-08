project('ffpp', 'c', 'cpp',
  version: run_command(find_program('cat', 'more'),files('../VERSION')).stdout().strip(),
  license: 'MIT',
  meson_version: '>= 0.53.0',
  default_options: [
    'c_std=gnu18',
    'buildtype=release',
    'warning_level=3',
    'default_library=both',
    'werror=true',
    'b_lundef=true',
  ],
)

host_system = host_machine.system()

cc = meson.get_compiler('c')
cppc = meson.get_compiler('cpp')

cc_flags = [
    # From open5gs project.
    '-Wlogical-op',
    '-Werror=missing-include-dirs',
    '-Werror=pointer-arith',
    '-Werror=init-self',
    '-Wfloat-equal',
    '-Wsuggest-attribute=noreturn',
    # '-Werror=missing-prototypes',
    # '-Werror=missing-declarations',
    '-Werror=implicit-function-declaration',
    '-Werror=return-type',
    '-Werror=incompatible-pointer-types',
    '-Werror=format=2',
    # '-Wstrict-prototypes',
    # '-Wredundant-decls',
    '-Wimplicit-fallthrough=5',
    '-Wendif-labels',
    '-Wstrict-aliasing=3',
    '-Wwrite-strings',
    '-Werror=overflow',
    '-Werror=shift-count-overflow',
    '-Werror=shift-overflow=2',
    '-Wdate-time',
    '-Wnested-externs',
    '-Wunused',
    '-Wduplicated-branches',
    '-Wmisleading-indentation',

    '-Wno-sign-compare',
    '-Wno-unused-parameter',

    ## '-fvisibility=hidden',
    '-ffast-math',
    '-fdiagnostics-show-option',
    '-fstack-protector',
    '-fstack-protector-strong',
    '--param=ssp-buffer-size=4',

    # ABI stability of DPDK is not considered here.
    # Because the test container always uses a specific DPDK version.
    '-Wno-deprecated-declarations',
    # They should be used in 6G and beyond :)
    '-Wno-unused-value',
    '-Wno-unused-variable',
    '-Wno-unused-but-set-variable',
]

cpp_flags = [
    # From open5gs project.
    '-Wlogical-op',
    '-Werror=missing-include-dirs',
    '-Werror=pointer-arith',
    '-Werror=init-self',
    '-Wfloat-equal',
    '-Wsuggest-attribute=noreturn',
    # '-Werror=missing-prototypes',
    # '-Werror=missing-declarations',
    '-Werror=return-type',
    '-Werror=format=2',
    # '-Wstrict-prototypes',
    # '-Wredundant-decls',
    '-Wimplicit-fallthrough=5',
    '-Wendif-labels',
    '-Wstrict-aliasing=3',
    '-Wwrite-strings',
    '-Werror=overflow',
    '-Werror=shift-count-overflow',
    '-Werror=shift-overflow=2',
    '-Wdate-time',
    '-Wnested-externs',
    '-Wunused',
    '-Wduplicated-branches',
    '-Wmisleading-indentation',

    '-Wno-sign-compare',
    '-Wno-unused-parameter',

    ## '-fvisibility=hidden',
    '-ffast-math',
    '-fdiagnostics-show-option',
    '-fstack-protector',
    '-fstack-protector-strong',
    '--param=ssp-buffer-size=4',

    '-Wno-deprecated-declarations',
    '-Wno-unused-value',
    '-Wno-unused-variable',
    '-Wno-unused-but-set-variable',
]

add_project_arguments(
    cc.get_supported_arguments(cc_flags),
    language: 'c',
)

add_project_arguments(
    cppc.get_supported_arguments(cpp_flags),
    language: 'cpp',
)

if get_option('buildtype').startswith('debug')
  add_project_arguments('-DDEBUG', language : 'c')
endif

inc = include_directories('include')

math_dep = cc.find_library('m', required: true)

bpf_dep = dependency('libbpf', required: false)
dpdk_dep = dependency('libdpdk', required: true)
json_dep = dependency('jansson', required: true)
python_embed_dep = dependency('python3-embed', required: true)
zmq_dep = dependency('libzmq', required: true)

ffpp_dep_list = [
  bpf_dep,
  dpdk_dep,
  json_dep,
  math_dep,
  python_embed_dep,
  zmq_dep,
]

ffpp_deps = declare_dependency(
  dependencies: ffpp_dep_list,
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

message('\n'.join([
  '',
  '        Host system:                  ' + host_system,
  '        Source code location:         ' + meson.source_root(),
  '        C compiler:                   ' + cc.get_id(),
  '        CPP compiler:                 ' + cppc.get_id(),
  '        Debugging support:            ' + get_option('buildtype'),
  '',
]))
