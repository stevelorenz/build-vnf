#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
This file extends the rtp_packet.py from [1] for the COIN-YOLO prototype.
The performance is not the focus of this prototype
It maybe better to use libraries like scapy or dpkt, but I'm in a hurry now...

[1] https://github.com/gabrieljablonski/rtsp-rtp-stream

Main references: RFC 3550,2435

0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|V=2|P|X|  CC   |M|     PT      |       sequence number         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           timestamp                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|           synchronization source (SSRC) identifier            |
+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
|            contributing source (CSRC) identifiers             |
|                             ....                              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Fragment Offset                            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      Type     |       Q       |     Width     |     Height    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
"""

import struct
import timeit


class JPEGHeader(object):

    """JPEGHeader
    No getters and setters, just assume all attributes are public
    """

    HEADER_SIZE = 8

    def __init__(self, fragment_offset, type_=0, q=0, width=0, height=0):
        self.header = bytearray(self.HEADER_SIZE)

        self.fragment_offset = fragment_offset
        self.type_ = type_
        self.q = q
        self.width = width
        self.height = height

    def pack(self):
        struct.pack_into(
            "!IBBBB",
            self.header,
            0,
            self.fragment_offset,
            self.type_,
            self.q,
            self.width,
            self.height,
        )
        return bytes(self.header)

    @classmethod
    def unpack(cls, header: bytes):
        assert len(header) == cls.HEADER_SIZE
        fragment_offset, type_, q, width, height = struct.unpack("!IBBBB", header)
        return cls(fragment_offset, type_, q, width, height)


class InvalidPacketException(Exception):
    pass


class RTPJPEGPacket:

    """RTP packet with JPEG format payload"""

    FIXED_HEADER_SIZE = 16  # bytes
    HEADER_SIZE = FIXED_HEADER_SIZE + JPEGHeader.HEADER_SIZE

    def __init__(
        self,
        version,
        padding,
        extension,
        cc,
        marker,
        payload_type,
        sequence_number,
        timestamp,
        ssrc,
        csrc,
        fragment_offset,
        payload: bytes,
    ):
        self.header = bytearray(self.HEADER_SIZE)

        self.version = version
        self.padding = padding
        self.extension = extension
        self.cc = cc
        self.marker = marker
        self.payload_type = payload_type
        self.sequence_number = sequence_number
        self.timestamp = timestamp
        self.ssrc = ssrc
        self.csrc = csrc

        self.fragment_offset = fragment_offset
        self.jpeg_header = JPEGHeader(self.fragment_offset, 0, 0, 0, 0)
        self.payload = payload

    def get_v_p_e_cc(self):
        return (
            (self.version << 6) | (self.padding << 5) | (self.extension << 4) | self.cc
        )

    def get_m_pt(self):
        return (self.marker << 7) | (self.payload_type & 0x7F)

    def pack(self):
        struct.pack_into(
            "!BBHIII",
            self.header,
            0,
            self.get_v_p_e_cc(),
            self.get_m_pt(),
            self.sequence_number,
            self.timestamp,
            self.ssrc,
            self.csrc,
        )
        # Add JPEGHeader
        self.header[
            self.FIXED_HEADER_SIZE : self.FIXED_HEADER_SIZE + JPEGHeader.HEADER_SIZE
        ] = self.jpeg_header.pack()
        return b"".join((self.header, self.payload))

    @classmethod
    def unpack(cls, packet: bytes):
        """This looks bad..."""
        fix_header_data = packet[: cls.FIXED_HEADER_SIZE]
        _, _, sequence_number, timestamp, ssrc, csrc = struct.unpack(
            "!BBHIII", fix_header_data
        )
        version = (fix_header_data[0] >> 6) & 0x03
        padding = (fix_header_data[0] >> 5) & 0x01
        extension = (fix_header_data[0] >> 4) & 0x01
        cc = fix_header_data[0] & 0x03
        marker = (fix_header_data[1] >> 7) & 0x01
        payload_type = (fix_header_data[1]) & 0x7F
        payload_offset = cls.FIXED_HEADER_SIZE + JPEGHeader.HEADER_SIZE
        jpeg_header_data = packet[cls.FIXED_HEADER_SIZE : payload_offset]
        jpeg_header = JPEGHeader.unpack(jpeg_header_data)
        payload = packet[payload_offset:]
        return cls(
            version,
            padding,
            extension,
            cc,
            marker,
            payload_type,
            sequence_number,
            timestamp,
            ssrc,
            csrc,
            jpeg_header.fragment_offset,
            payload,
        )


class RTPFragmenter:
    def __init__(self) -> None:
        pass

    @staticmethod
    def fragmentize(data, sequence_number, timestamp, max_fragment_size):
        fragments = []
        for offset in range(0, len(data), max_fragment_size):
            fragments.append(
                RTPJPEGPacket(
                    2,
                    0,
                    0,
                    0,
                    0,
                    26,
                    sequence_number,
                    timestamp,
                    0,
                    0,
                    offset,
                    data[offset : offset + max_fragment_size],
                )
            )
        # The last fragment MUST has the marker 1
        fragments[-1].marker = 1
        return fragments


class RTPReassembler:
    def __init__(self) -> None:
        self.fragments = []
        self.cur_sequence_number = 0
        self.next_fragment_offset = 0

    def reset(self):
        """Reset the reassembler"""
        self.fragments.clear()
        self.cur_sequence_number = 0
        self.next_fragment_offset = 0

    def add_fragment(self, fragment: RTPJPEGPacket):
        # Add the first fragment of a new frame
        if len(self.fragments) == 0:
            if fragment.fragment_offset != 0:
                self.reset()
                return "BAD_NEW_FRAME"
            else:
                self.cur_sequence_number = fragment.sequence_number
                self.next_fragment_offset = self.next_fragment_offset + len(
                    fragment.payload
                )
                self.fragments.append(fragment)
                return "GOOD_NEW_FRAME"
        # Add further fragments
        else:
            if (self.cur_sequence_number == fragment.sequence_number) and (
                self.next_fragment_offset == fragment.fragment_offset
            ):
                self.fragments.append(fragment)
                self.next_fragment_offset = self.next_fragment_offset + len(
                    fragment.payload
                )
                if fragment.marker == 1:
                    return "HAS_ENTIRE_FRAME"
                else:
                    return "GOOD_CUR_FRAME"
            else:
                self.reset()
                return "BAD_CUR_FRAME"

    def get_frame(self):
        """Get the entire frame"""
        frame = b""
        for f in self.fragments:
            frame = b"".join((frame, f.payload))
        return frame


def test():
    print("* Run tests")
    jpeg_header = JPEGHeader(1, 0, 0, 0, 0)
    jpeg_header.fragment_offset = 2
    jpeg_header_data = jpeg_header.pack()
    assert len(jpeg_header_data) == 8
    new_jpeg_header = JPEGHeader.unpack(jpeg_header_data)
    assert new_jpeg_header.fragment_offset == 2

    test_payload = b"test"
    rtp_packet = RTPJPEGPacket(2, 0, 0, 0, 1, 26, 123, 321, 0, 0, 1, test_payload)
    assert rtp_packet.fragment_offset == 1
    rtp_packet_data = rtp_packet.pack()
    assert len(rtp_packet_data) == 24 + len(test_payload)

    new_rtp_packet = RTPJPEGPacket.unpack(rtp_packet_data)
    assert new_rtp_packet.marker == 1
    assert new_rtp_packet.fragment_offset == 1
    assert new_rtp_packet.payload == test_payload

    test_payload = b"t" * 48000
    fragmenter = RTPFragmenter()
    fragments = fragmenter.fragmentize(test_payload, 0, 0, 1400)
    assert len(fragments) == 35
    assert fragments[0].marker == 0
    assert len(fragments[-1].payload) == 400
    assert fragments[-1].marker == 1

    reassembler = RTPReassembler()
    ret = reassembler.add_fragment(fragments[1])
    assert ret == "BAD_NEW_FRAME"
    ret = reassembler.add_fragment(fragments[0])
    assert ret == "GOOD_NEW_FRAME"

    for f in fragments[1:-1]:
        ret = reassembler.add_fragment(f)
        assert ret == "GOOD_CUR_FRAME"
    ret = reassembler.add_fragment(fragments[-1])
    assert ret == "HAS_ENTIRE_FRAME"

    frame = reassembler.get_frame()
    assert test_payload == frame

    print("All tests passed !")


def benchmark():
    print("* Run benchmarks")
    setup = """
from __main__ import RTPJPEGPacket
test_payload = b"t" * 48000
p = RTPJPEGPacket(2, 0, 0, 0, 1, 26, 123, 321, 0, 0, 1, test_payload)
    """
    stmt = """
d = p.pack()
new_p = RTPJPEGPacket.unpack(d)
    """
    number = 1000
    ret = timeit.timeit(stmt=stmt, setup=setup, number=number)
    ret = (ret / number) * 1e3
    print(f"Pack and unpack time for one RTPJPEGPacket with size 48000B: {ret}ms")

    setup = """
from __main__ import RTPFragmenter
test_payload = b"t" * 48000
f = RTPFragmenter()
    """
    stmt = """
fragments = f.fragmentize(test_payload, 0, 0, 1400)
    """
    number = 1000
    ret = timeit.timeit(stmt=stmt, setup=setup, number=number)
    ret = (ret / number) * 1e3
    print(
        f"RTP fragmentize time with payload length 48000B, max fragment size: 1400B: {ret}ms"
    )

    setup = """
from __main__ import RTPFragmenter, RTPReassembler
test_payload = b"t" * 48000
f = RTPFragmenter()
fragments = f.fragmentize(test_payload, 0, 0, 1400)
reassembler = RTPReassembler()
    """
    stmt = """
for f in fragments:
    ret = reassembler.add_fragment(f)
frame = reassembler.get_frame()
    """
    number = 1000
    ret = timeit.timeit(stmt=stmt, setup=setup, number=number)
    ret = (ret / number) * 1e3
    print(
        f"RTP reassemble with payload length 48000B, max fragment size: 1400B: {ret}ms"
    )


if __name__ == "__main__":
    test()
    benchmark()
