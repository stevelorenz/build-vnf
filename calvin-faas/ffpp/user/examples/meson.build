all_examples = [
  'frequency_manager',
  'frequency_switcher',
  'power_manager',
  'zmq',
  'traffic_monitor',
  'c1_manager',
  'feedback_manager',
]

foreach example: all_examples
  name = example.split('/')[-1]
  sources=[]
  subdir(example)
  executable('ffpp_' + name, sources,
    include_directories: [inc, include_directories(example)],
    dependencies: [ffpp_dep],
    link_with : [ffpplib_shared],
    link_args : '-lcpufreq')
endforeach
