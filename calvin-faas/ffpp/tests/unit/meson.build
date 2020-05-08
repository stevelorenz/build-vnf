test_dummy = executable(
  'test_dummy', 'test_dummy.c',
  include_directories : inc,
  dependencies: [dpdk_dep, bpf_dep],
  link_with : [ffpplib_shared])

test('FFPP Unit Tests', [test_dummy])
