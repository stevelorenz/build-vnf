# Available suits:
# - unit: Unit tests
# - no-leak: There's no memory leaking from FFPP's code, but there's problems in the dependencies...
#            So the leak information is manually checked and make sure there's no-leak in FFPP's code.
# - dev: Tests currently under development and debugging. So a subset of all tests.

# Common tests do not require special environment setup

test_common_sources = files('''
	test_dummy.cpp
'''.split())

test_common_executable = executable('test_common',
	sources: test_common_sources,
	include_directories: inc,
	dependencies: [ffpp_deps, gtest_withmain_dep], link_with: [ffpplib_shared])

test('test_common', [test_common_executable], is_parallel: true, suite: ['unit'],
	workdir : meson.source_root()
)

# Following tests are defined separately mainly because of some environment have to be initialized only ONCE... e.g. DPDK's EAL,
# So they need to run in different/isolated processes.

test_cnf_exe = executable('test_cnf',
	sources: ['test_cnf.cpp'],
	include_directories: inc,
	dependencies: [ffpp_deps, gtest_withmain_dep], link_with: [ffpplib_shared])

test('test_cnf', test_cnf_exe, is_parallel: false, suite: ['unit'],
	workdir : meson.source_root()
)

test_mbuf_helpers_exe = executable('test_mbuf_helpers',
	sources: ['test_mbuf_helpers.cpp'],
	include_directories: inc,
	dependencies: [ffpp_deps, gtest_withmain_dep], link_with: [ffpplib_shared])

test('test_mbuf_helpers', test_mbuf_helpers_exe, is_parallel: false, suite: ['unit', 'dev'],
	workdir : meson.source_root()
)

test_cnf_exe = executable('test_rte_graph',
	sources: ['test_rte_graph.cpp'],
	include_directories: inc,
	dependencies: [ffpp_deps, gtest_withmain_dep], link_with: [ffpplib_shared])

test('test_rte_graph', test_cnf_exe, is_parallel: false, suite: ['unit', 'dev'],
	workdir : meson.source_root()
)
