# MARK: EAL primary process is used to run the tests, so parallel need to be disabled.

# Tests managed by google-test
unit_test_gtest_sources = files('''
	test_gtest_dummy.cpp
'''.split())

unit_test_gtest_exe = executable('unit-gtest',
	sources: unit_test_gtest_sources,
	dependencies: [ffpp_deps, gtest_withmain_dep])

test('unit-gtest', unit_test_gtest_exe, is_parallel: false, suite: ['no-leak', 'unit', 'dev'])

# Other special tests
test_pye_pybind11 = executable(
  	'test_pye_pybind11', 'test_pye_pybind11.cpp',
  	include_directories: inc,
  	dependencies: ffpp_deps,
  	link_with: [ffpplib_shared])

test('test_pye_pybind11', test_pye_pybind11, is_parallel : false, suite: ['no-leak', 'unit', 'dev'])