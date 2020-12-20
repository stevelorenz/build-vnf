subdir('unit')

# All auto unit tests
# MARK: Primary process is used to run the tests, so parallel need to be disabled.
test('test_dummy', test_dummy, args:['-l 0', '--no-pci'], is_parallel : false, suite: ['no-leak'])

test('test_munf', test_munf,
  args:['-l 0', '--no-pci','--proc-type', 'primary',
    '--vdev=net_null0', '--vdev=net_null1'],
  is_parallel : false, suite: ['no-leak', 'dev']
)
test('test_pye_pybind11', test_pye_pybind11, is_parallel : false, suite: ['no-leak', 'dev'])

test('test_mbuf_generation', test_mbuf_generation,
  args:['-l 0', '--no-pci','--proc-type', 'primary',
    '--vdev', 'net_pcap0,rx_pcap=/ffpp/user/tests/data/udp_3pkts.pcap,tx_pcap=/tmp/test_mbuf_generation.pcap'],
  is_parallel : false, suite: ['no-leak', 'dev'])

# TODO: Extra non-auto and non-unit tests
extra_tests = [
]

foreach extra_test: extra_tests
  executable(extra_test, files(extra_test+'.c'),
    dependencies: [dpdk_dep],
    link_with : [ffpplib_shared])
endforeach
