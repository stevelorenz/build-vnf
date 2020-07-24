ffpp_dep = dependency('libffpp')

all_examples = [
  'frequency_manager',
  'frequency_switcher',
  'power_manager',
  'zmq'
]

foreach example: all_examples
  name = example.split('/')[-1]
  sources=[]
  subdir(example)
  executable('ffpp_' + name, sources,
    include_directories: [inc, include_directories(example)],
    dependencies: [dpdk_dep, bpf_dep, ffpp_dep, zmq_dep, json_dep],
    link_with : [ffpplib_shared],
    link_args : '-lcpufreq')
endforeach
