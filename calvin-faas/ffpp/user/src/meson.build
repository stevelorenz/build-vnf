ffpp_sources = [
  'aes.c',
  'bpf_helpers_user.c',
  'collections/mvec.c',
  'device.c',
  'general_helpers_user.c',
  'io.c',
  'memory.c',
  'scaling_helpers_user.c',
  'task.c',
  'utils.c',
  'general_helpers_user.c',
  ]

ffpplib_shared = shared_library('ffpp',
  ffpp_sources,
  include_directories: inc,
  dependencies: [dpdk_dep, bpf_dep, math_dep, json_dep, zmq_dep],
  install: true
)

ffpplib_static = static_library('ffpp',
  ffpp_sources,
  include_directories: inc,
  dependencies: [dpdk_dep, bpf_dep, math_dep, json_dep, zmq_dep],
  install: true
)
