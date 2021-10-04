#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Config
"""

client_address_data = ("10.0.1.11", 9999)
server_address_data = ("10.0.3.11", 9999)
# control_port = data_port + 1
server_address_control = (server_address_data[0], server_address_data[1] + 1)
