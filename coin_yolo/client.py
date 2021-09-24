#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: YOLOv2 client
"""

import argparse
import csv
import socket
import sys
import time
import typing

import meica_host

# The duration of each message.
DURATION_OF_EACH_MSG: typing.Final[float] = 1  # seconds


class Client(meica_host.MEICAHost):
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
        self.sock_data.bind(self.client_address_data)

        if verbose:
            self.logger = meica_host.get_logger("debug")
        else:
            self.logger = meica_host.get_logger("info")

    def generate_data(self, wav_range, source_number):
        folder_address = "/in-network_bss/google_dataset/32000_wav_factory"
        # Use the fixed combination of source files which only depends on
        # source_number.
        _, _, X = pyfbss_tb.generate_matrix_S_A_X_fixed_sources(
            folder_address, wav_range, source_number
        )
        return X

    def start_session(self, wav_range, source_number, total_msg_num):
        """Start a new session for messages with specific parameters."""
        self.sock_control.connect(self.server_address_control)
        context = ",".join((str(wav_range), str(source_number), str(total_msg_num)))
        self.sock_control.sendall(context.encode())
        ack = self.sock_control.recv(1024).decode()
        if ack != "INIT_ACK":
            raise RuntimeError("Failed to get the INIT ACK from server!")
        self.sock_control.sendall("CONTROL_CLOSE".encode())

    def run(
        self,
        mode,
        wav_range,
        source_number,
        total_msg_num,
        chunk_gap,
        service_latency_csv,
    ):
        # TODO: Perform data pre-processing for mode == compute_forward
        print(f"* Client runs. Mode: {mode}")
        print(
            f"- Wav range: {wav_range}, source number: {source_number}, total message number: {total_msg_num}."
        )
        print(f"- Chunk gap: {chunk_gap} seconds.")
        service_latency_csv = service_latency_csv + "_" + mode + ".csv"
        print(f"- Service latency CSV file: {service_latency_csv}")

        self.start_session(wav_range, source_number, total_msg_num)

        # MARK: Use the same X for all messages.
        X = self.generate_data(wav_range, source_number)
        service_latencies = list()
        for msg_num in range(total_msg_num):
            start_ts = time.time()
            chunks, msg_len = self.fragment(
                X, msg_type=0, total_msg_num=total_msg_num, msg_num=msg_num
            )
            self.logger.debug(
                f"Message number: {msg_num} ,size: {msg_len}. Start sending {len(chunks)} chunks to the server..."
            )
            total_chunk_num = len(chunks)
            if total_chunk_num > meica_host.MAX_CHUNK_NUM:
                raise RuntimeError(
                    f"Number of chunks {total_chunk_num} is larger than the maximal allowed chunks: {meica_host.MAX_CHUNK_NUM}."
                )
            counter = 0
            start_ts = time.time()
            for chunk in chunks:
                payload = b"".join(chunk)
                self.sock_data.sendto(payload, self.server_address_data)
                time.sleep(chunk_gap)
                counter += 1
            self.logger.debug(f"{counter} chunks are sent successfully.")
            self.logger.info("Wait for the ACK from the server...")
            data, _ = self.sock_data.recvfrom(1024)
            latency = time.time() - start_ts
            self.logger.debug(f"Total service latency: {latency} seconds.")
            service_latencies.append(latency)
            elapsed_time = time.time() - start_ts
            if elapsed_time < wav_range:
                # Wait for next message to be ready.
                time.sleep(wav_range - elapsed_time)

        assert len(service_latencies) > 0
        service_latencies.insert(0, source_number)
        with open(service_latency_csv, "a+") as csvfile:
            writer = csv.writer(csvfile, delimiter=",")
            writer.writerow(service_latencies)

    def test(self, chunk_gap):
        # 2 for fast break (msg_type: 2) and 8 for without fast break (msg_type: 1)
        for source_number in [2, 8]:
            X = self.generate_data(DURATION_OF_EACH_MSG, source_number)
            chunks, msg_len = self.fragment(X, msg_type=0, total_msg_num=1, msg_num=0)
            data_chunk_num = len(chunks)
            total_chunk_num = data_chunk_num
            if total_chunk_num > meica_host.MAX_CHUNK_NUM:
                raise RuntimeError(
                    f"Number of chunks {total_chunk_num} is larger than the maximal allowed chunks: {meica_host.MAX_CHUNK_NUM}."
                )
            self.logger.debug(
                f"Message size: {msg_len}, data chunk number: {data_chunk_num}, Start sending {total_chunk_num} chunks to the server..."
            )
            couter = 0
            for chunk in chunks:
                payload = b"".join(chunk)
                self.sock_data.sendto(payload, self.server_address_data)
                time.sleep(chunk_gap)
                couter += 1
            self.logger.info(f"{couter} chunks are sent.")

    def probe(self, total_msg_num, chunk_gap):
        """Send probing packets for measurements of the baseline performance."""
        self.logger.info("Send probing packets.")
        probe_data = b"P" * meica_host.MEICA_IP_TOTAL_LEN
        for _ in range(total_msg_num):
            self.sock_data.sendto(probe_data, self.server_address_data)
            time.sleep(chunk_gap)

        self.logger.info("All probing packets are sent.")

    def cleanup(self):
        self.sock_control.close()
        self.sock_data.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="MEICA client.")
    parser.add_argument(
        "--wav_range", type=int, default=1, help="Time range to generate data."
    )
    parser.add_argument(
        "--source_number", type=int, default=2, help="Number of source node."
    )
    parser.add_argument(
        "--total_msg_num", type=int, default=1, help="Number of messages to send."
    )
    parser.add_argument(
        "--chunk_gap",
        type=float,
        default=0.01,  # seconds
        help="Sleep time (seconds) between each chunk in a message.",
    )
    parser.add_argument(
        "-m",
        "--mode",
        type=str,
        default="store_forward",
        choices=["store_forward", "compute_forward", "no_compute"],
        help="The working mode of the client.",
    )
    parser.add_argument(
        "--service_latency_csv",
        type=str,
        default="client_service_latency",
        help="Basename of the CSV file (without extension name) to store service latency results.",
    )
    parser.add_argument(
        "--use_fastica",
        action="store_true",
        help="Run FastICA for comparision.",
        default=False,
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="Enable verbose mode."
    )
    parser.add_argument("--probe", action="store_true", help="Send probing packets.")
    parser.add_argument(
        "--test",
        action="store_true",
        help="Generate test data for development purposes.",
    )
    args = parser.parse_args()
    wav_range = args.wav_range
    source_number = args.source_number
    total_msg_num = args.total_msg_num
    chunk_gap = args.chunk_gap
    service_latency_csv = args.service_latency_csv

    if args.use_fastica:
        service_latency_csv = "fastica_" + service_latency_csv
        print(f"Use FastICA as comparison. CSV file: {service_latency_csv}")

    client_address_data = ("10.0.1.11", 9999)
    server_address_data = ("10.0.3.11", 9999)
    server_address_control = (server_address_data[0], server_address_data[1] + 1)

    client = Client(
        server_address_control,
        client_address_data,
        server_address_data,
        args.verbose,
    )

    try:
        if args.test:
            print("* Client generates test traffic.")
            client.test(chunk_gap)
        elif args.probe:
            client.probe(total_msg_num, chunk_gap)
        else:
            client.run(
                args.mode,
                wav_range,
                source_number,
                total_msg_num,
                chunk_gap,
                service_latency_csv,
            )
    except KeyboardInterrupt:
        print("Client stops.")
    finally:
        client.cleanup()
