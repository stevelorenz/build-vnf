ffpp_sources = [
  'collections/vec.c',
  'device.c',
  'handler/arp.c',
  'handler/dispatch.c',
  'io.c',
  'ipc.c',
  'master.c',
  'memory.c',
  'mp.c',
  'task.c',
  'utils.c',
]

ffpplib_shared = shared_library('ffpp',
  ffpp_sources,
  include_directories: inc,
  dependencies: [dpdk_dep, bpf_dep],
  install: true
)

ffpplib_static = static_library('ffpp',
  ffpp_sources,
  include_directories: inc,
  dependencies: [dpdk_dep, bpf_dep],
  install: true
)
