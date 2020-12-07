project('ebpf_map_rw_latency', 'cpp',
  version : '0.1',
  default_options : ['warning_level=2', 'cpp_std=c++2a'])

ffpp_dep = dependency('libffpp', required: true)

dep_list = [
  ffpp_dep
]

all_deps = declare_dependency(
  dependencies: dep_list,
)

executable('ebpf_map_rw_latency',
           'ebpf_map_rw_latency.cpp',
           dependencies:all_deps,
           install : true)
