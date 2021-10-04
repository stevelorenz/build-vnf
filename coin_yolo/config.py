#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Config
"""

DEFAULT_PORT = 11111
SFC_PORT = 9999
PORT = DEFAULT_PORT

client_address_data = ("10.0.1.11", PORT)
server_address_data = ("10.0.3.11", PORT)
# control_port = data_port + 1
server_address_control = (server_address_data[0], server_address_data[1] + 1)


end_host_cpu_args = {
    "client": {"cpuset_cpus": "0", "nano_cpus": int(1e9)},
    "server": {"cpuset_cpus": "0", "nano_cpus": int(1e9)},
}

# TODO: Check if different link parameters should be tested ?
link_params = {
    "host-sw": {"bw": 1000, "delay": "1ms"},
    "sw-sw": {"bw": 1000, "delay": "1ms"},
    "sw-vnf": {"bw": 1000, "delay": "0ms"},
}

RTP_FRAGMENT_LEN = 1400
