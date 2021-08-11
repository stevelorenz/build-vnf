# MARK: EAL primary process is used to run the tests, so parallel need to be disabled.

# Available suits:
# - unit: Unit tests
# - no-leak: There's no memory leaking from FFPP's code, but there's problems in the dependencies...
#            So the leak information is manually checked and make sure there's no-leak in FFPP's code.
# - dev: Tests currently under development and debugging. So a subset of all tests.

dummy_test_sources = files('''
	test_dummy.cpp
'''.split())

dummy_test_exe = executable('dummy_test',
	sources: dummy_test_sources,
	include_directories: inc,
	dependencies: [ffpp_deps, gtest_withmain_dep], link_with: [ffpplib_shared])

test('dummy_test', dummy_test_exe, is_parallel: false, suite: ['unit', 'no-leak'],
	workdir : meson.source_root()
)

# Tests are defined separately mainly because of some environment have to be initialized only ONCE... e.g. DPDK's EAL,
# So they need to run in different/isolated processes.
test_cnf_sources = files('''
	test_cnf.cpp
'''.split())

test_cnf_exe = executable('test_cnf',
	sources: test_cnf_sources,
	include_directories: inc,
	dependencies: [ffpp_deps, gtest_withmain_dep], link_with: [ffpplib_shared])

test('test_cnf', test_cnf_exe, is_parallel: false, suite: ['unit', 'no-leak', 'dev'],
	workdir : meson.source_root()
)

# Other special tests
test_pye_pybind11 = executable(
  	'test_pye_pybind11', 'test_pye_pybind11.cpp',
  	include_directories: inc,
  	dependencies: ffpp_deps,
  	link_with: [ffpplib_shared])

test('test_pye_pybind11', test_pye_pybind11, is_parallel : false, suite: ['unit', 'no-leak'])