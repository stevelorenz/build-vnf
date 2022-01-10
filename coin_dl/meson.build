project('coin_dl', 'cpp',
  version : '0.1.0',
  default_options : [
    'warning_level=3',
    'cpp_std=c++2a',
    'buildtype=release',
    'werror=true',
    ]
  )

host_system = host_machine.system()
cppc = meson.get_compiler('cpp')

cpp_flags = [
  '-Wno-implicit-fallthrough',
  '-Wno-missing-field-initializers',
  '-Wno-deprecated-declarations',
  # They will be used in 6G and beyond :)
  '-Wno-unused-value',
  '-Wno-unused-variable',
  '-Wno-unused-parameter',
  '-Wno-unused-but-set-variable',
  '-Wno-missing-field-initializers',
  ]

add_project_arguments(
  cpp_flags,
  language: 'cpp',
  )

boost_dep = dependency('boost', modules : ['program_options'], required: true)
boost_header_only_dep = dependency('boost', required: true)
ffpp_dep = dependency('libffpp', required: true)
fmt_dep = dependency('fmt', required: true)
glog_dep = dependency('libglog', required: true)

dep_list = [
  boost_dep,
  boost_header_only_dep,
  ffpp_dep,
  fmt_dep,
  glog_dep,
  ]

all_deps = declare_dependency(
  dependencies: dep_list,
  )

executable('coin_dl',
  'coin_dl.cpp',
  dependencies:all_deps,
  install : false
  )

#
# Log and Message
#

message('\n'.join([
  '',
  'Build Info:',
  '        Host system:                  ' + host_system,
  '        Source code location:         ' + meson.source_root(),
  '        Build directory location:     ' + meson.build_root(),
  '        CPP compiler:                 ' + cppc.get_id(),
  '        Build type:                   ' + get_option('buildtype'),
  '',
  ]))
