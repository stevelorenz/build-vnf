# Available suts:
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

test_packet_engine_exe = executable('test_packet_engine',
	sources: ['test_packet_engine.cpp'],
	include_directories: inc,
	dependencies: [ffpp_deps, gtest_withmain_dep], link_with: [ffpplib_shared])

test('test_packet_engine', test_packet_engine_exe, is_parallel: false, suite: ['unit'],
	workdir : meson.source_root()
)

test_mbuf_pdu_exe = executable('test_mbuf_pdu',
	sources: ['test_mbuf_pdu.cpp'],
	include_directories: inc,
	dependencies: [ffpp_deps, gtest_withmain_dep], link_with: [ffpplib_shared])

test('test_mbuf_pdu', test_mbuf_pdu_exe, is_parallel: false, suite: ['unit', 'dev'],
	workdir : meson.source_root()
)
