test_dummy = executable(
  'test_dummy', 'test_dummy.c',
  include_directories : inc,
  dependencies: [dpdk_dep, bpf_dep],
  link_with : [ffpplib_shared])

test_mvec = executable(
  'test_mvec', 'test_mvec.c',
  include_directories : inc,
  dependencies: [dpdk_dep],
  link_with : [ffpplib_shared])

test_munf = executable(
  'test_munf', 'test_munf.c',
  include_directories : inc,
  dependencies: [dpdk_dep],
  link_with : [ffpplib_shared])
