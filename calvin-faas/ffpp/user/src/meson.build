ffpp_sources = [
  'aes.c',
  'bpf_helpers_user.c',
  'collections/mvec.c',
  'device.c',
  'general_helpers_user.c',
  'general_helpers_user.c',
  'io.c',
  'memory.c',
  'munf.c',
  'packet_processors.c',
  'python.c',
  'scaling_helpers_user.c',
  'task.c',
  'utils.c',
  ]

ffpplib_shared = shared_library('ffpp',
  ffpp_sources,
  include_directories: inc,
  dependencies: ffpp_deps,
  install: true
)

ffpplib_static = static_library('ffpp',
  ffpp_sources,
  include_directories: inc,
  dependencies: ffpp_deps,
  install: true
)
