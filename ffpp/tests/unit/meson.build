# MARK: EAL primary process is used to run the tests, so parallel need to be disabled.

# Available suits:
# - unit: Unit tests
# - no-leak: There's no memory leaking from FFPP's code, but there's problems in the dependencies...
#            So the leak information is manually checked and make sure there's no-leak in FFPP's code.
# - dev: Tests currently under development and debugging. So a subset of all tests.

# Tests managed by google-test
unit_test_gtest_sources = files('''
	test_gtest_dummy.cpp
	test_packet_vector.cpp
'''.split())

unit_test_gtest_exe = executable('unit-gtest',
	sources: unit_test_gtest_sources,
	dependencies: [ffpp_deps, gtest_withmain_dep])

test('unit-gtest-all', unit_test_gtest_exe, is_parallel: false, suite: ['unit', 'no-leak'])

# Other special tests
test_pye_pybind11 = executable(
  	'test_pye_pybind11', 'test_pye_pybind11.cpp',
  	include_directories: inc,
  	dependencies: ffpp_deps,
  	link_with: [ffpplib_shared])

test('test_pye_pybind11', test_pye_pybind11, is_parallel : false, suite: ['unit', 'no-leak'])