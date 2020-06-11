subdir('unit')

ffpp_dep = dependency('libffpp')

# All auto unit tests
test('FFPP Unit Tests', [test_dummy])

# Extra non-auto and non-unit tests
extra_tests = [
  'test_power',
  'test_ring',
]

foreach extra_test: extra_tests
  executable(extra_test, files(extra_test+'.c'),
    dependencies: [dpdk_dep, bpf_dep, ffpp_dep],
    link_with : [ffpplib_shared])
endforeach
