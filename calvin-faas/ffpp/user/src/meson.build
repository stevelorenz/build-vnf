ffpp_sources = [
  'bpf_helpers_user.c',
  'collections/mvec.c',
  'device.c',
  'handler/arp.c',
  'handler/dispatch.c',
  'io.c',
  # 'ipc.c',
  'memory.c',
  'mp.c',
  'scaling_helpers_user.c',
  'task.c',
  'utils.c',
  'general_helpers_user.c',
  ]

ffpplib_shared = shared_library('ffpp',
  ffpp_sources,
  include_directories: inc,
  dependencies: [dpdk_dep, bpf_dep, math_dep],
  install: true
)

ffpplib_static = static_library('ffpp',
  ffpp_sources,
  include_directories: inc,
  dependencies: [dpdk_dep, bpf_dep, math_dep],
  install: true
)
