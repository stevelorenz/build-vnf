# Copyright (C) 2016 Nippon Telegraph and Telephone Corporation.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
About: Ryu SDN controller application for the dumbbell topology.

TODO:
    - Add monitoring
    - Add QoS for bandwidth shaping

MARK: I'm not an expert for SDN controller programming :-)
"""

import json
import shlex
import subprocess

import ryu.lib.packet as packet_lib
import ryu.topology.api as topo_api

from ryu.base import app_manager
from ryu.controller import ofp_event
from ryu.controller.handler import CONFIG_DISPATCHER, MAIN_DISPATCHER, set_ev_cls
from ryu.lib import dpid as dpid_lib
from ryu.lib.ovs import vsctl
from ryu.ofproto import ofproto_v1_3

# Assume the controller knows the IPs of end-hosts.
CLIENT_IP = "10.0.1.11"
SERVER_IP = "10.0.2.11"


class MultiHopRest(app_manager.RyuApp):

    OFP_VERSIONS = [ofproto_v1_3.OFP_VERSION]

    def __init__(self, *args, **kwargs):
        super(MultiHopRest, self).__init__(*args, **kwargs)

        self.switches = {}
        self.nodes = {}

        self.mac_to_port = {}
        self.ip_to_port = {}
        # Map specific interface names to port.
        self.vnf_iface_to_port = {}

        # Load topology information
        with open("./dumbbell.json", "r", encoding="ascii") as f:
            self.topo_params = json.load(f)

    # ISSUE: This is a temp workaround to get openflow port of each VNF instance
    # without adding a service discovery system.
    @staticmethod
    def _get_ofport(ifce: str):
        """Get the openflow port based on the iterface name.

        :param ifce (str): Name of the interface.
        """
        try:
            # MARK: root privilege is required to access the OVS DB.
            ret = int(
                subprocess.check_output(
                    shlex.split(f"sudo ovs-vsctl get Interface {ifce} ofport")
                )
                .decode("utf-8")
                .strip()
            )
        except Exception as e:
            print(e)
            import pdb

            pdb.set_trace()

        return ret

    @set_ev_cls(ofp_event.EventOFPSwitchFeatures, CONFIG_DISPATCHER)
    def switch_features_handler(self, ev):

        datapath = ev.msg.datapath
        dpid = datapath.id
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser

        self.logger.info(f"*** Connect with datapath with ID: {datapath.id}")

        # Install table-miss flow entry
        #
        # We specify NO BUFFER to max_len of the output action due to
        # OVS bug. At this moment, if we specify a lesser number, e.g.,
        # 128, OVS will send Packet-In with invalid buffer_id and
        # truncated packet data. In that case, we cannot output packets
        # correctly.  The bug has been fixed in OVS v2.1.0.
        match = parser.OFPMatch()
        actions = [
            parser.OFPActionOutput(ofproto.OFPP_CONTROLLER, ofproto.OFPCML_NO_BUFFER)
        ]
        self.add_flow(datapath, 0, match, actions)

        self.switches[dpid] = datapath

        self.mac_to_port.setdefault(dpid, {})
        self.ip_to_port.setdefault(dpid, {})

        # Get port number of the specific interface for VNF processing.
        port_num = self._get_ofport(f"s{datapath.id}-vnf{datapath.id}")
        self.vnf_iface_to_port[datapath.id] = port_num

    def add_flow(self, datapath, priority, match, actions, buffer_id=None):
        """Add a new entry in the flow table of the datapath.

        WARN: This step takes some time even if controller and switch are connected locally.
        """
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser

        inst = [parser.OFPInstructionActions(ofproto.OFPIT_APPLY_ACTIONS, actions)]
        if buffer_id:
            mod = parser.OFPFlowMod(
                datapath=datapath,
                buffer_id=buffer_id,
                priority=priority,
                match=match,
                instructions=inst,
            )
        else:
            mod = parser.OFPFlowMod(
                datapath=datapath,
                priority=priority,
                match=match,
                instructions=inst,
            )
        datapath.send_msg(mod)

    def add_l2fwd(self, msg, pkt, add_flow=False):
        datapath = msg.datapath
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser
        in_port = msg.match["in_port"]
        dpid = datapath.id
        eth = pkt.get_protocol(packet_lib.ethernet.ethernet)
        dst = eth.dst
        src = eth.src

        ip = pkt.get_protocol(packet_lib.ipv4.ipv4)
        if ip:
            self.ip_to_port.setdefault(dpid, {})
            self.ip_to_port[dpid][ip.src] = in_port

        self.logger.debug(
            f"""<Packet-In>[L2FWD]: DPID:{dpid}, l2_src:{src}, l2_dst:{dst}, in_port: {in_port}"""
        )

        self.mac_to_port[dpid][src] = in_port

        if dst in self.mac_to_port[dpid]:
            out_port = self.mac_to_port[dpid][dst]
        else:
            out_port = ofproto.OFPP_FLOOD

        actions = [parser.OFPActionOutput(out_port)]

        if add_flow:
            if out_port != ofproto.OFPP_FLOOD:
                match = parser.OFPMatch(
                    in_port=in_port, eth_dst=dst, eth_src=src, eth_type=eth.ethertype
                )
                # verify if we have a valid buffer_id, if yes avoid to send both
                # flow_mod & packet_out
                if msg.buffer_id != ofproto.OFP_NO_BUFFER:
                    self.add_flow(datapath, 1, match, actions, msg.buffer_id)
                    return
                else:
                    self.add_flow(datapath, 1, match, actions)

        data = None
        if msg.buffer_id == ofproto.OFP_NO_BUFFER:
            data = msg.data

        out = parser.OFPPacketOut(
            datapath=datapath,
            buffer_id=msg.buffer_id,
            in_port=in_port,
            actions=actions,
            data=data,
        )
        datapath.send_msg(out)

    def check_sfc_flow(self, pkt):
        """Check if the given packet belongs to flows that should be managed by SFC.

        :param pkt:
        """
        ip = pkt.get_protocol(packet_lib.ipv4.ipv4)
        udp = pkt.get_protocol(packet_lib.udp.udp)

        if (
            ip.src == CLIENT_IP
            and ip.dst == SERVER_IP
            and udp.dst_port == self.topo_params["sfc_port"]
        ):
            return True

        return False

    def handle_udp(self, msg, pkt):
        if not self.check_sfc_flow(pkt):
            self.logger.info(
                "<Packet-In>[UDP]: Receive un-known UDP flows. Action: L2FWD."
            )
            self.add_l2fwd(msg, pkt, add_flow=True)
            return

        datapath = msg.datapath
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser
        in_port = msg.match["in_port"]
        dpid = datapath.id
        eth = pkt.get_protocol(packet_lib.ethernet.ethernet)
        ip = pkt.get_protocol(packet_lib.ipv4.ipv4)
        udp = pkt.get_protocol(packet_lib.udp.udp)

        l3_info = f"<Packet-In>[UDP]: DPID:{dpid}, l3_src:{ip.src}, l3_dst:{ip.dst}, in_port: {in_port}"
        l4_info = f"l4_src:{udp.src_port}, l4_dst: {udp.dst_port}, total_len: {udp.total_length}"

        # --- Handle upstream flows.
        vnf_out_port = self.vnf_iface_to_port[datapath.id]

        if in_port == vnf_out_port:
            out_port = self.mac_to_port[dpid].get(eth.dst, None)
            if not out_port:
                self.logger.error(
                    f"Can not find the output port of upstream UDP flows for vnf port of datapath {datapath.id}"
                )
                return
            actions = [parser.OFPActionOutput(out_port)]
            action_info = f"out_port: {out_port}"
        else:
            actions = [parser.OFPActionOutput(vnf_out_port)]
            action_info = f"out_port: {vnf_out_port}"

        self.logger.info(", ".join((l3_info, l4_info, action_info)))

        # Add forwarding rules for these specific UDP flows.
        match = parser.OFPMatch(
            in_port=in_port,
            eth_src=eth.src,
            eth_dst=eth.dst,
            eth_type=0x0800,
            ip_proto=0x11,
            udp_dst=udp.dst_port,
        )
        # Use a higher priority to avoid overriding.
        if msg.buffer_id != ofproto.OFP_NO_BUFFER:
            self.add_flow(datapath, 17, match, actions, msg.buffer_id)
            return
        else:
            self.add_flow(datapath, 17, match, actions)

        data = None
        if msg.buffer_id == ofproto.OFP_NO_BUFFER:
            data = msg.data

        out = parser.OFPPacketOut(
            datapath=datapath,
            buffer_id=msg.buffer_id,
            in_port=in_port,
            actions=actions,
            data=data,
        )
        datapath.send_msg(out)

    @set_ev_cls(ofp_event.EventOFPPacketIn, MAIN_DISPATCHER)
    def _packet_in_handler(self, ev):
        # If you hit this you might want to increase
        # the "miss_send_length" of your switch
        if ev.msg.msg_len < ev.msg.total_len:
            self.logger.debug(
                "- Packet truncated: only %s of %s bytes",
                ev.msg.msg_len,
                ev.msg.total_len,
            )
        msg = ev.msg
        datapath = msg.datapath
        parser = datapath.ofproto_parser

        pkt = packet_lib.packet.Packet(msg.data)
        eth = pkt.get_protocol(packet_lib.ethernet.ethernet)
        arp = pkt.get_protocol(packet_lib.arp.arp)
        ip = pkt.get_protocol(packet_lib.ipv4.ipv4)
        udp = pkt.get_protocol(packet_lib.udp.udp)

        # Ignore LLDP packets
        if eth.ethertype == packet_lib.ether_types.ETH_TYPE_LLDP:
            return

        # Currently drop IPv6 packets
        if pkt.get_protocol(packet_lib.ipv6.ipv6):
            match = parser.OFPMatch(eth_type=eth.ethertype)
            actions = []
            self.add_flow(datapath, 1, match, actions)
            return None

        if arp:
            self.add_l2fwd(msg, pkt, add_flow=False)
        elif ip:
            if udp:
                self.handle_udp(msg, pkt)
            else:
                self.add_l2fwd(msg, pkt)
        else:
            # Ignore other packets.
            return
