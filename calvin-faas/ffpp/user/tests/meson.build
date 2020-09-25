subdir('unit')

# All auto unit tests
# MARK: Primary process is used to run the tests, so parallel need to be disabled.
test('test_dummy', test_dummy, args:['-l 0'], is_parallel : false, suite: 'no-leak')
test('test_mvec', test_mvec, args:['-l 0'], is_parallel : false, suite: 'no-leak')
test('test_munf', test_munf,
  args:['-l 0', '--no-pci','--proc-type', 'primary',
    '--vdev=net_null0', '--vdev=net_null1'],
  is_parallel : false, suite: 'no-leak'
)
test('test_pye', test_pye, is_parallel : false, suite: 'leak')

# Extra non-auto and non-unit tests
extra_tests = [
  # 'test_power',
  # 'test_ring',
]

foreach extra_test: extra_tests
  executable(extra_test, files(extra_test+'.c'),
    dependencies: [dpdk_dep],
    link_with : [ffpplib_shared])
endforeach
