#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About : UDP RTT measurement tool
        Use multi-threading, the server use a queue to avoid packet dropping.
        Indexes are used to check if packets are in order.

Issue : - Directly bounced packets will be redirected into the chain even if the
        chain is set to asymmetric.

Email : xianglinks@gmail.com
"""

import argparse
import logging
import queue
import socket
import struct
import subprocess
import sys
import threading
import time

# --- Logging ----

fmt_str = '%(asctime)s %(levelname)-8s %(threadName)s %(message)s'
level = {
    'INFO': logging.INFO,
    'DEBUG': logging.DEBUG,
    'ERROR': logging.ERROR
}
logger = logging.getLogger(__name__)

# --- Global ----

MAX_SEND_SLOW_NUMBER = 10
BUFFER_SIZE = 1024
RECV_SHORT_SLEEP = 1e-5  # 10us
THD_JOIN_CHECK_PERIOD = 0.5  # second
PUSH_PACK_NUM = 50

CSV_FILE_PATH = './udp_rtt.csv'


class Base(object):

    def __init__(self, name):
        self._name = name
        self._send_thd = threading.Thread(daemon=True,
                                          target=self._send_pack,
                                          name=self._name+'_send_thd')
        self._recv_thd = threading.Thread(daemon=True,
                                          target=self._recv_pack,
                                          name=self._name + '_recv_thd')
        self._send_sock = socket.socket(family=socket.AF_INET,
                                        type=socket.SOCK_DGRAM)
        self._recv_sock = socket.socket(family=socket.AF_INET,
                                        type=socket.SOCK_DGRAM)

    def _send_pack(self):
        raise NotImplementedError

    def _recv_pack(self):
        raise NotImplementedError

    def _cleanup(self):
        raise NotImplementedError

    def run(self):
        """Run until detect KeyboardInterrupt"""
        logger.info('UDP %s runs.' % self._name)
        self._recv_thd.start()
        self._send_thd.start()

        quit = False
        try:
            while not quit:
                quit = True
                for thd in (self._send_thd, self._recv_thd):
                    if thd.is_alive():
                        quit = False
                        thd.join(THD_JOIN_CHECK_PERIOD)
        except KeyboardInterrupt:
            logger.info('[%s] KeyboardInterrupt detected, run cleanups.',
                        self._name)

        finally:
            self._cleanup()

# ---- UDP Client ---


class UDPClient(Base):

    """UDP Client"""

    def __init__(self, send_addr, recv_ip, pack_nb, ipd, pld_size, b_addr,
                 push=False):
        super(UDPClient, self).__init__('client')
        self._send_ip, self._send_port = send_addr
        self._recv_ip = recv_ip
        self._recv_port = self._send_port + 1
        self._pld_size = pld_size
        self._ipd = ipd
        self._pack_nb = pack_nb
        self._b_addr = b_addr
        self._push = push

        self._recv_info_lst = list()
        self._rtt_result = list()
        self._flush_csv = True

    def _send_pack(self):
        if self._b_addr:
            self._send_sock.bind(self._b_addr)
        logger.info('[Client] Start sending UDP packets to %s:%s' %
                    (self._send_ip, self._send_port))

        send_data = bytearray(b'a' * self._pld_size)
        send_idx = 0
        send_slow_nb = 0
        while send_idx < self._pack_nb:
            send_data[0:16] = struct.pack('!qQ', send_idx,
                                          int(time.time() * 1e6))
            st_send_ts = time.time()
            self._send_sock.sendto(send_data[0:self._pld_size],
                                   (self._send_ip, self._send_port))
            send_dur = time.time() - st_send_ts
            logger.debug('[Client] send index:%d, send duration: %f' %
                         (send_idx, send_dur))
            if send_dur <= self._ipd:
                time.sleep(self._ipd - send_dur)
                send_slow_nb = 0
            else:
                send_slow_nb += 1
                if send_slow_nb >= MAX_SEND_SLOW_NUMBER:
                    logger.error(
                        '[Client] Client sends too slow, the IDP may be too small.')
                    self._cleanup()
            send_idx += 1
        logger.debug(
            '[Client] The client finishes sending {:d} packets.'.format(self._pack_nb))

        if self._push:
            logger.info('[Client] Send additional %d pushing packets.',
                        PUSH_PACK_NUM)
            send_idx = 0
            while send_idx < PUSH_PACK_NUM:
                send_data[0:16] = struct.pack('!qQ', -1,
                                              int(time.time() * 1e6))
                self._send_sock.sendto(send_data[0:self._pld_size],
                                       (self._send_ip, self._send_port))
                time.sleep(self._ipd * 2)
                send_idx += 1

        # Raise exception to main thread

    def _recv_pack(self):
        self._recv_sock.setblocking(False)
        self._recv_sock.bind((self._recv_ip, self._recv_port))
        logger.info('[Client] Start receiving UDP packets from %s:%s' %
                    (self._recv_ip, self._recv_port))
        recv_idx = 0
        while recv_idx < self._pack_nb:
            try:
                data = self._recv_sock.recv(BUFFER_SIZE)
            except socket.error:
                time.sleep(RECV_SHORT_SLEEP)
                continue
            recv_ts = int(time.time() * 1e6)
            self._recv_info_lst.append((data, recv_ts))
            logger.debug('[Client] recv index:%d', recv_idx)
            recv_idx += 1

        logger.info(
            '[Client] Receive all bounced packets, start calculating RTTs.')
        self._flush_csv = False
        self._calc_rtt()

    def _calc_rtt(self):
        # Check order and calculate RTT
        for idx, recv_info in enumerate(self._recv_info_lst):
            send_idx, send_ts = struct.unpack('!qQ', recv_info[0][0:16])
            if idx != send_idx:
                logger.error('The order of packets is not correct!')
                self._cleanup()
            rtt = recv_info[1] - send_ts
            if rtt < 0:
                raise RuntimeError('Error: Get negative RTT!')
            self._rtt_result.append(rtt)

        # Write into csv file
        with open(CSV_FILE_PATH, 'a+') as csv_file:
            csv_file.write(
                ','.join(map(str, self._rtt_result))
            )
            csv_file.write('\n')

    def _cleanup(self):
        logger.info('[Client] Run cleanups. Flush CSV:%s', self._flush_csv)
        if self._flush_csv:
            self._calc_rtt()
        self._send_sock.close()
        self._recv_sock.close()
        sys.exit(0)


# --- UDP Server ---

class UDPServer(Base):

    """UDP Server"""

    def __init__(self, queue_size, recv_addr, send_delay, pld_size, b_addr):
        super(UDPServer, self).__init__('server')
        self.buffer = queue.Queue(maxsize=queue_size)
        self._recv_ip, self._recv_port = recv_addr
        # ...
        self._send_delay = send_delay
        self._pld_size = pld_size
        self._b_addr = b_addr

    def _recv_pack(self):
        self._recv_sock.bind((self._recv_ip, self._recv_port))
        self._recv_sock.setblocking(0)
        recv_idx = 0
        while True:
            try:
                data, addr = self._recv_sock.recvfrom(BUFFER_SIZE)
            except socket.error:
                time.sleep(RECV_SHORT_SLEEP)
                continue
            # Put the data into queue, check if queue is full
            if self.buffer.full():
                logger.error(
                    '[Server] Receive queue is full. SHOULD Use a bigger queue number.')
                logger.info('[Server] Capture 100 packets for debugging.')
                pcap_file_name = 'queue_overflow_{}.pcap'.format(
                    int(time.time()))
                subprocess.check_call(
                    'sudo tcpdump -nn dst {} and udp -c 100 -w {}'.format(
                        self._recv_ip, pcap_file_name),
                    shell=True)
                self._cleanup()
            else:
                # Timestamp for start queuing packets in the buffer
                # MARK: No print before st_queue_ts
                st_queue_ts = int(time.time() * 1e6)
                logger.debug('[Server] Recv index: %d', recv_idx)
                self.buffer.put_nowait((data, addr, st_queue_ts))
                recv_idx += 1

    def _send_pack(self):
        if self._b_addr:
            self._send_sock.bind(self._b_addr)
        bounce_idx = 0
        while True:
            if not self.buffer.empty():
                data, addr, st_queue_ts = self.buffer.get_nowait()
                data_arr = bytearray(data)
                queue_time = int(time.time() * 1e6) - st_queue_ts
                # Update the send ts in the packet with queuing time on the
                # server side
                send_idx, send_ts = struct.unpack('!qQ', data[0:16])
                if send_idx < 0:
                    logger.debug('[Server] Recv a pushing packet')
                    bounce_idx = 0
                    continue
                elif send_idx == 0:
                    logger.info(
                        # Only delayed for the first packet
                        '[Server] Delay %f seconds before bouncing the first packet', self._send_delay)
                    time.sleep(self._send_delay)
                    bounce_idx = 0
                send_ts += queue_time  # This is the delta
                data_arr[0:16] = struct.pack('!qQ', send_idx, send_ts)
                self._send_sock.sendto(
                    data_arr[:], (addr[0], (self._recv_port + 1)))
                logger.debug(
                    '[Server] Bounce index: %d, queue time: %dus', bounce_idx,
                    queue_time)
                bounce_idx += 1
            else:
                time.sleep(RECV_SHORT_SLEEP)

    def _cleanup(self):
        logger.info('[Server] Run cleanups.')
        self._send_sock.close()
        self._recv_sock.close()
        sys.exit(0)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='UDP RTT measurement tool.',
        formatter_class=argparse.RawTextHelpFormatter
    )
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-s', metavar='ADDRESS', type=str,
                       help='Run as UDP server.')
    parser.add_argument('--srv_queue_len', type=int, default=100,
                        help='Length of receiving queue for server.')
    parser.add_argument('-d', '--delay', type=float, default=10,
                        help="""Start sending delay for server. This is used to
avoid the backward traffic being redirected into the
chain. (Bug of neutron sfc extension)""")

    group.add_argument('-c', metavar='ADDRESS', type=str,
                       help='Run as UDP client. ADDRESS format: ip:port')
    parser.add_argument('-l', metavar='IP', type=str,
                        help='IP used by client to receive packets.'
                        + 'Listen port = send port + 1'
                        )

    parser.add_argument('-n', type=int, default=3,
                        help='Number of to be sent packets.')
    parser.add_argument('--ipd', type=float, default=1.0,
                        help='Inter-packet delay in seconds.')
    parser.add_argument('--payload_size', type=int, default=256,
                        help='UDP Payload size in bytes.')
    parser.add_argument('--csv_path', type=str, default='./udp_rtt.csv',
                        help='Path to store CSV files.')
    parser.add_argument('--bind', metavar='ADDRESS', type=str, default='',
                        help='To be binded address for sending packets.')
    parser.add_argument('--push', action='store_true', default=False,
                        help='Send additional pushing packets.')

    parser.add_argument('--log', type=str, default='INFO',
                        help='Logging level, e.g. INFO, DEBUG')
    args = parser.parse_args()

    logging.basicConfig(level=args.log,
                        handlers=[logging.StreamHandler()],
                        format=fmt_str)

    b_addr = None
    if args.bind:
        s_ip, s_port = args.bind.split(':')
        s_port = int(s_port)
        logger.info('Bind send socket to address: %s:%d', s_ip, s_port)
        b_addr = (s_ip, s_port)

    if args.c:
        logger.info('# Run in UDP client mode:')
        ip, port = args.c.split(':')
        port = int(port)
        udp_clt = UDPClient((ip, port), args.l, args.n,
                            args.ipd, args.payload_size,
                            b_addr, args.push)
        logger.info("IPD: %f, Payload size: %d, Push: %s",
                    args.ipd, args.payload_size, args.push)
        CSV_FILE_PATH = args.csv_path
        udp_clt.run()

    if args.s:
        logger.info('# Run in UDP server mode:')
        ip, port = args.s.split(':')
        port = int(port)
        logger.info('Server queue length: {}, send delay: {}s'.format(
            args.srv_queue_len, args.delay))
        udp_srv = UDPServer(args.srv_queue_len, (ip, port), args.delay,
                            args.payload_size, b_addr)
        udp_srv.run()
