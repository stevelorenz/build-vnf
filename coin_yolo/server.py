#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: YOLOv2 server
"""

import argparse
import csv
import socket
import sys
import time

import meica_host
import utils

sys.path.insert(0, "../")

from pyfastbss_core import pyfbss
from pyfastbss_testbed import pyfbss_tb


class Server(meica_host.MEICAHost):
    def __init__(
        self,
        server_address_control,
        client_address_data,
        server_address_data,
        verbose,
    ):
        self.server_address_control = server_address_control
        self.sock_control = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        self.client_address_data = client_address_data
        self.server_address_data = server_address_data
        self.sock_data = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock_data.bind(self.server_address_data)
        if verbose:
            self.logger = meica_host.get_logger("debug")
        else:
            self.logger = meica_host.get_logger("info")

        self.extraction_base = 2

    def recv_chunks(self) -> list:
        """Receive all chunks in order."""
        meica_header_len = meica_host.ServiceHeader.length
        # TODO: Add timeout.
        recv_timeout = False

        chunk_bytes, _ = self.sock_data.recvfrom(4096)
        hdr = meica_host.ServiceHeader.parse(chunk_bytes[:meica_header_len])
        total_chunk_num = hdr.total_chunk_num
        self.logger.debug(
            f"Received the first chunk, total chunk number: {total_chunk_num}"
        )
        chunks = [(chunk_bytes[:meica_header_len], chunk_bytes[meica_header_len:])]
        chunk_counter = 1

        if total_chunk_num == 1:
            return chunks

        while not recv_timeout:
            chunk_bytes, _ = self.sock_data.recvfrom(4096)
            chunk_counter += 1
            hdr = meica_host.ServiceHeader.parse(chunk_bytes[:meica_header_len])
            self.logger.debug(
                f"Recv {chunk_counter} chunks, msg_type: {hdr.msg_type}, number: {hdr.chunk_num}, length: {hdr.chunk_len}"
            )
            chunks.append(
                (chunk_bytes[:meica_header_len], chunk_bytes[meica_header_len:])
            )

            if hdr.chunk_num == total_chunk_num - 1:
                self.logger.debug("Received all chunks.")
                break

        return chunks

    def send_ack(self):
        self.sock_data.sendto(b"OK", self.client_address_data)

    @staticmethod
    def get_chunk_sort_key(chunk):
        hdr = meica_host.ServiceHeader.parse(chunk[0])
        return hdr.chunk_num

    def fix_error_chunks(self, chunks: list) -> bool:
        """Fix error chunks (out-of-order or lost) with basic heuristic approaches.

        :param chunks (mutable): A list of chunks of a message.

        :return: True if chunks are successfully fixed.
        """
        hdr = meica_host.ServiceHeader.parse(chunks[0][0])
        if len(chunks) != hdr.total_chunk_num:
            return False
        else:
            # Chunks are only out-of-order, just sort them.
            chunks.sort(key=self.get_chunk_sort_key)
            return True

    def handle_chunks_store_foward(self, chunks):
        if not self.check_chunks(chunks):
            if not self.fix_error_chunks(chunks):
                return False

        hdr = meica_host.ServiceHeader.parse(chunks[0][0])
        msg_type = hdr.msg_type
        # iter_num = hdr.iter_num

        if msg_type == 0:
            self.logger.debug("Start running centralized MEICA.")
            X = self.defragment(chunks)
            pyfbss.meica(X, ext_multi_ica=self.extraction_base)
            self.send_ack()
        else:
            raise RuntimeError(f"Invalid or unknown message type {msg_type}!")

        return True

    def handle_chunks_compute_foward(self, X_chunks, uW_chunks):
        if not self.check_chunks(X_chunks):
            if not self.fix_error_chunks(X_chunks):
                return False

        X = self.defragment(X_chunks)
        uW = self.defragment(uW_chunks)
        uW_hdr = meica_host.ServiceHeader.parse(uW_chunks[0][0])
        msg_flags = uW_hdr.msg_flags
        if msg_flags == 0:
            iter_num = uW_hdr.iter_num
            self.logger.debug(
                f"Start running distributed MEICA. Compute from the {iter_num}-th MEICA iteration."
            )

            uW_prev = uW
            uXs = pyfbss.meica_generate_uxs(X, ext_multi_ica=self.extraction_base)
            assert iter_num < len(uXs)
            for index in range(iter_num, len(uXs), 1):
                uW_next, break_by_tol = pyfbss.meica_dist_get_uw(uXs[index], uW_prev)
                uW_prev = uW_next
                if break_by_tol:
                    break
            pyfbss.meica_dist_get_hat_s(uW_next, X)
            self.send_ack()

        elif msg_flags == 1:
            self.logger.debug("Get the final uW. Calculate hat_S directly.")
            pyfbss.meica_dist_get_hat_s(uW, X)
            self.send_ack()

        else:
            raise RuntimeError("Unknown message flags!")

        return True

    def handle_chunks_fastica(self, chunks):
        if not self.check_chunks(chunks):
            if not self.fix_error_chunks(chunks):
                return False

        hdr = meica_host.ServiceHeader.parse(chunks[0][0])
        msg_type = hdr.msg_type
        # iter_num = hdr.iter_num

        if msg_type == 0:
            self.logger.debug("Start running centralized FastICA.")
            X = self.defragment(chunks)
            pyfbss.fastica(X, max_iter=100, tol=1e-04)
            self.send_ack()
        else:
            raise RuntimeError(f"Invalid or unknown message type {msg_type}!")

        return True

    def handle_chunks_no_compute(self, chunks):
        """Do nothing on the chunks and just send ACK directly"""
        self.send_ack()

    def handle_session(self, mode, source_number, total_msg_num, compute_latency_csv):
        if mode == "no_compute":
            for _ in range(total_msg_num):
                chunks = self.recv_chunks()
                self.handle_chunks_no_compute(chunks)
            # There is no any compute latency
            return

        compute_latencies = list()

        # Broken messages are now ignored.
        if mode == "store_forward":
            for _ in range(total_msg_num):
                chunks = self.recv_chunks()
                start_ts = time.time()
                if self.handle_chunks_store_foward(chunks):
                    elapsed_time = time.time() - start_ts
                    compute_latencies.append(elapsed_time)

        elif mode == "compute_forward":
            # WARN: Currently, it is assumed that the chunks of X arrive firstly
            # and there are no any conflict with the chunks of uW
            for i in range(total_msg_num):
                X_chunks = self.recv_chunks()
                self.logger.debug(f"Message number: {i}, received all chunks of X.")
                uW_chunks = self.recv_chunks()
                self.logger.debug(f"Message number: {i}, received all chunks of uW.")
                start_ts = time.time()
                if self.handle_chunks_compute_foward(X_chunks, uW_chunks):
                    elapsed_time = time.time() - start_ts
                    compute_latencies.append(elapsed_time)
        else:
            raise RuntimeError("Unknown working mode!")

        assert len(compute_latencies) > 0
        compute_latencies.insert(0, source_number)
        with open(compute_latency_csv, "a+") as csvfile:
            writer = csv.writer(csvfile, delimiter=",")
            writer.writerow(compute_latencies)

    def handle_session_fastica(
        self, mode, source_number, total_msg_num, compute_latency_csv
    ):
        compute_latencies = list()
        self.logger.debug("Use FastICA algorithm.")
        for _ in range(total_msg_num):
            chunks = self.recv_chunks()
            start_ts = time.time()
            if self.handle_chunks_fastica(chunks):
                elapsed_time = time.time() - start_ts
                compute_latencies.append(elapsed_time)

        assert len(compute_latencies) > 0
        compute_latencies.insert(0, source_number)
        with open(compute_latency_csv, "a+") as csvfile:
            writer = csv.writer(csvfile, delimiter=",")
            writer.writerow(compute_latencies)

    def run(self, mode, compute_latency_csv, use_fastica):
        print(f"* Server starts. Mode: {mode}")
        compute_latency_csv = compute_latency_csv + "_" + mode + ".csv"
        self.sock_control.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock_control.bind(self.server_address_control)
        self.sock_control.listen(1)

        while True:
            conn, client_address_control = self.sock_control.accept()
            self.logger.info(
                "Get a connection from {}:{}".format(*client_address_control)
            )
            context = conn.recv(1024).decode()
            wav_range, source_number, total_msg_num = list(map(int, context.split(",")))
            conn.sendall("INIT_ACK".encode())
            ack = conn.recv(1024).decode()
            if ack != "CONTROL_CLOSE":
                raise RuntimeError("Failed to handle control close message!")
            conn.close()

            if use_fastica:
                self.handle_session_fastica(
                    mode, source_number, total_msg_num, compute_latency_csv
                )
            else:
                self.handle_session(
                    mode, source_number, total_msg_num, compute_latency_csv
                )
            self.logger.info(
                "Finish the session from {}:{}".format(*client_address_control)
            )

    def probe(self):
        """Receive probing packets for measurements of the baseline performance."""
        self.logger.info("Receive probing packets.")
        probe_latencies = list()
        for _ in range(100 + 1):
            start_ts = time.time()
            self.sock_data.recvfrom(4096)
            elapsed_time = time.time() - start_ts
            probe_latencies.append(elapsed_time)

        with open("./network_latency.csv", "a+") as csvfile:
            writer = csv.writer(csvfile, delimiter=",")
            writer.writerow(probe_latencies[1:])

    def cleanup(self):
        self.sock_control.close()
        self.sock_data.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="MEICA server.")
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="Print debug information"
    )
    parser.add_argument(
        "-m",
        "--mode",
        type=str,
        default="store_forward",
        choices=["store_forward", "compute_forward", "no_compute"],
        help="Working mode of the server.",
    )
    parser.add_argument("--probe", action="store_true", help="Receive probing packets.")
    parser.add_argument(
        "--compute_latency_csv",
        type=str,
        default="server_compute_latency",
        help="Basename of the CSV file to store compute latency results on the server.",
    )
    parser.add_argument(
        "--use_fastica",
        action="store_true",
        help="Run FastICA for comparision.",
        default=False,
    )
    args = parser.parse_args()
    compute_latency_csv = args.compute_latency_csv

    if args.use_fastica:
        compute_latency_csv = "fastica_" + compute_latency_csv
        print(f"Use FastICA as comparison. CSV file: {compute_latency_csv}")

    client_address_data = ("10.0.1.11", 9999)
    server_address_data = ("10.0.3.11", 9999)
    server_address_control = (server_address_data[0], server_address_data[1] + 1)

    server = Server(
        server_address_control,
        client_address_data,
        server_address_data,
        args.verbose,
    )

    if args.probe:
        server.probe()
    else:
        try:
            server.run(args.mode, compute_latency_csv, args.use_fastica)
        except KeyboardInterrupt:
            print("Server stops.")
        except RuntimeError as e:
            print("Server stops with error: " + str(e))
        finally:
            server.cleanup()
