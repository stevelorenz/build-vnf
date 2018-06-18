#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About : UDP RTT measurement tool
        Use multi-threading, the server use a queue to avoid packet dropping.

Email : xianglinks@gmail.com
"""

import argparse
import logging
import queue
import socket
import struct
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
                        thd.join(1)
        except KeyboardInterrupt:
            logger.info('[%s] KeyboardInterrupt detected, run cleanups.',
                        self._name)
            self._cleanup()

        finally:
            self._cleanup()

# ---- UDP Client ---


class UDPClient(Base):

    """UDP Client"""

    def __init__(self, send_addr, recv_ip, pack_nb, ipd, pld_size):
        super(UDPClient, self).__init__('client')
        self._send_ip, self._send_port = send_addr
        self._recv_ip = recv_ip
        self._recv_port = self._send_port + 1
        self._pld_size = pld_size
        self._ipd = ipd
        self._pack_nb = pack_nb

        self._send_info_lst = list()
        self._recv_info_lst = list()

    def _send_pack(self):
        logger.info('[Client] Start sending UDP packets to %s:%s' %
                    (self._send_ip, self._send_port))
        send_data = bytearray(self._pld_size)
        send_data[8:self._pld_size] = b'a' * (self._pld_size - 8)
        send_idx = 0
        send_slow_nb = 0
        while send_idx < self._pack_nb:
            send_data[0:7] = struct.pack('!Q', send_idx)
            st_send_ts = time.time()
            self._send_info_lst.append((send_idx, int(st_send_ts * 1e6)))
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

    def _recv_pack(self):
        self._recv_sock.setblocking(False)
        self._recv_sock.bind((self._recv_ip, self._recv_port))
        logger.info('[Client] Start receiving UDP packets from %s:%s' %
                    (self._recv_ip, self._recv_port))

        recv_idx = 0
        while recv_idx < self._pack_nb:
            # Here is blocking IO
            try:
                data = self._recv_sock.recv(BUFFER_SIZE)
            except socket.error:
                time.sleep(RECV_SHORT_SLEEP)
                continue
            recv_ts = int(time.time() * 1e6)
            self._recv_info_lst.append((data, recv_ts))
            logger.debug('[Client] recv index:%d', recv_idx)
            recv_idx += 1

        self._calc_rtt()

    def _calc_rtt(self):
        rtt_result = list()
        # Check order and calculate RTT
        for send, recv in zip(self._send_info_lst, self._recv_info_lst):
            if send[0] != struct.unpack('!Q', recv[0][0:8])[0]:
                logger.error('The packet order is not correct!')
            else:
                rtt_result.append(recv[1] - send[1])

        # Write into csv file
        f_name = 'rtt_%s_%s.csv' % (self._ipd, self._pld_size)
        with open('./%s' % f_name, 'a+') as csv_file:
            csv_file.write(
                ','.join(map(str, rtt_result))
            )
            csv_file.write('\n')

    def _cleanup(self):
        logger.info('[Client] Run cleanups.')
        self._send_sock.close()
        self._recv_sock.close()
        sys.exit(0)


# --- UDP Server ---

class UDPServer(Base):

    """UDP Server"""

    def __init__(self, queue_size, recv_addr, send_delay):
        super(UDPServer, self).__init__('server')
        self.buffer = queue.Queue(maxsize=queue_size)
        self._recv_ip, self._recv_port = recv_addr
        self._st_send_evt = threading.Event()
        self._st_send_evt.clear()
        self._send_delay = send_delay

    def _recv_pack(self):
        self._recv_sock.bind((self._recv_ip, self._recv_port))
        self._recv_sock.setblocking(0)
        recv_idx = 0
        while True:
            try:
                data, addr = self._recv_sock.recvfrom(BUFFER_SIZE)
                logger.debug('[Server] Recv index: %d', recv_idx)
                # Put the data into queue, check if queue is full
                if self.buffer.full():
                    logger.error('[Server] Receive queue is full.')
                    self._cleanup()
                else:
                    self.buffer.put_nowait((data, addr))
                    recv_idx += 1
            except socket.error:
                time.sleep(RECV_SHORT_SLEEP)
                continue

    def _send_pack(self):
        self._st_send_evt.wait(timeout=self._send_delay)
        send_idx = 0
        while True:
            if not self.buffer.empty():
                data, addr = self.buffer.get_nowait()
                self._send_sock.sendto(data, (addr[0], (self._recv_port + 1)))
                logger.debug('[Server] Send index: %d', send_idx)
                send_idx += 1
            else:
                time.sleep(RECV_SHORT_SLEEP)

    def _cleanup(self):
        logger.info('[Client] Run cleanups.')
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

    parser.add_argument('--log', type=str, default='INFO',
                        help='Logging level, e.g. INFO, DEBUG')
    args = parser.parse_args()

    logging.basicConfig(level=args.log,
                        handlers=[logging.StreamHandler()],
                        format=fmt_str)

    if args.c:
        logger.info('# Run in UDP client mode:')
        ip, port = args.c.split(':')
        port = int(port)
        udp_clt = UDPClient((ip, port), args.l, args.n,
                            args.ipd, args.payload_size)
        udp_clt.run()

    if args.s:
        logger.info('# Run in UDP server mode:')
        ip, port = args.s.split(':')
        port = int(port)
        logger.info('Server queue length: {}, send delay: {}s'.format(
            args.srv_queue_len, args.delay))
        udp_srv = UDPServer(args.srv_queue_len, (ip, port), args.delay)
        udp_srv.run()
