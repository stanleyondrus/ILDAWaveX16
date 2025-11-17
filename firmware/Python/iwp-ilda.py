#!/usr/bin/env python3

# IWP-ILDA
# Parse an ILDA file and stream it via UDP to an ILDAWaveX16 using IWP
# StanleyProjects 2025

# Usage:
# python iwp-ilda.py --file "animation.ild" --ip 192.168.1.123 --scan 1000 --repeat 0
# E.g.: python iwp-ilda.py --file "anim1.ild" --ip 192.168.1.142 --scan 100000 --fps 0 --repeat 100

from __future__ import annotations
import argparse
import socket
import struct
import sys
import time
from dataclasses import dataclass
from typing import List, Optional, Tuple

# ------------------------
# Protocol constants
# ------------------------
IW_TYPE_0 = 0x00  # Turn off / end frame
IW_TYPE_1 = 0x01  # Period (us)
IW_TYPE_2 = 0x02  # 16b X/Y + 8b R/G/B
IW_TYPE_3 = 0x03  # 16b X/Y + 16b R/G/B

# ------------------------
# ILDA structures / helpers
# ------------------------
ILDA_HEADER_SIZE = 32

# Status bit (per ILDA spec): bit 6 = blanked (1 means laser off)
STATUS_BLANKED_MASK = 0b0100_0000

@dataclass
class IldaHeader:
    format: int
    frame_name: str
    company_name: str
    records: int
    frame_number: int
    total_frames: int
    projector_number: int

@dataclass
class IldaFrame:
    format: int
    points: List[Tuple[int, int, int, int, int, int, int]]


def read_ilda_header(buf: bytes, offset: int) -> Tuple[Optional[IldaHeader], int]:
    head = buf[offset:offset + ILDA_HEADER_SIZE]
    if len(head) < ILDA_HEADER_SIZE:
        return None, offset

    if head[0:4] != b"ILDA":
        return None, offset

    format_code = head[7]
    frame_name = head[8:16].rstrip(b"\x00").decode(errors="ignore")
    company_name = head[16:24].rstrip(b"\x00").decode(errors="ignore")
    records = struct.unpack(">H", head[24:26])[0]
    frame_number = struct.unpack(">H", head[26:28])[0]
    total_frames = struct.unpack(">H", head[28:30])[0]
    projector_number = head[30]

    hdr = IldaHeader(
        format=format_code,
        frame_name=frame_name,
        company_name=company_name,
        records=records,
        frame_number=frame_number,
        total_frames=total_frames,
        projector_number=projector_number,
    )
    return hdr, offset + ILDA_HEADER_SIZE


def parse_ilda(filename: str) -> Tuple[List[IldaFrame], List[Tuple[int, int, int]]]:
    with open(filename, "rb") as f:
        data = f.read()

    offset = 0
    frames: List[IldaFrame] = []
    palette: List[Tuple[int, int, int]] = [(255, 255, 255)] * 256

    while offset < len(data):
        hdr, offset = read_ilda_header(data, offset)
        if hdr is None:
            break

        fmt = hdr.format
        recs = hdr.records

        if fmt == 0:
            rec_size = 8
            points = []
            for _ in range(recs):
                rec = data[offset:offset + rec_size]
                if len(rec) < rec_size:
                    break
                x, y, z, status, color_index = struct.unpack(">hhhBB", rec)
                r, g, b = palette[color_index]
                points.append((x, y, z, status, r, g, b))
                offset += rec_size
            frames.append(IldaFrame(format=fmt, points=points))

        elif fmt == 1:
            rec_size = 6
            points = []
            for _ in range(recs):
                rec = data[offset:offset + rec_size]
                if len(rec) < rec_size:
                    break
                x, y, status, color_index = struct.unpack(">hhBB", rec)
                r, g, b = palette[color_index]
                points.append((x, y, 0, status, r, g, b))
                offset += rec_size
            frames.append(IldaFrame(format=fmt, points=points))

        elif fmt == 2:
            rec_size = 3
            for i in range(recs):
                rec = data[offset:offset + rec_size]
                if len(rec) < rec_size:
                    break
                r, g, b = struct.unpack(">BBB", rec)
                if i < 256:
                    palette[i] = (r, g, b)
                offset += rec_size

        elif fmt == 4:
            rec_size = 10
            points = []
            for _ in range(recs):
                rec = data[offset:offset + rec_size]
                if len(rec) < rec_size:
                    break
                x, y, z, status, b, g, r = struct.unpack(">hhhBBBB", rec)
                points.append((x, y, z, status, r, g, b))
                offset += rec_size
            frames.append(IldaFrame(format=fmt, points=points))

        elif fmt == 5:
            rec_size = 8
            points = []
            for _ in range(recs):
                rec = data[offset:offset + rec_size]
                if len(rec) < rec_size:
                    break
                x, y, status, b, g, r = struct.unpack(">hhBBBB", rec)
                points.append((x, y, 0, status, r, g, b))
                offset += rec_size
            frames.append(IldaFrame(format=fmt, points=points))

        else:
            break

    return frames, palette


# ------------------------
# UDP sender
# ------------------------
class ProjectorSender:
    def __init__(self, ip: str, scan_rate: int = 1000, point_delay: float = 0.0):
        self.ip = ip
        self.port = 7200   
        self.scan_period = max(1, min(4294967295, int(1000000/int(scan_rate))))
        self.point_delay = point_delay
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.sendto(struct.pack(">B I", IW_TYPE_1, self.scan_period), (self.ip, self.port))

    @staticmethod
    def _u16(x: int) -> int:
        return x & 0xFFFF

    @staticmethod
    def _to_u16_from_u8(c: int) -> int:
        return (c & 0xFF) * 257

    def _transform_xy(self, x: int, y: int) -> Tuple[int, int]:
        xn = (x + 0x8000)
        yn = (-y + 0x8000)
        return self._u16(xn), self._u16(yn)

    def send_frame(self, points: List[Tuple[int, int, int, int, int, int, int]]):
        max_packet_size = 1023
        point_size = struct.calcsize(">B H H H H H")  # 11 bytes
        max_points_per_packet = max_packet_size // point_size
        
        samples = []
        for (x, y, _z, status, r8, g8, b8) in points:
            blanked = (status & STATUS_BLANKED_MASK) != 0
        
            x16, y16 = self._transform_xy(x, y)
            if blanked:
                r16 = g16 = b16 = 0
            else:
                r16 = self._to_u16_from_u8(r8)
                g16 = self._to_u16_from_u8(g8)
                b16 = self._to_u16_from_u8(b8)
        
            samples.append(struct.pack(
                ">B H H H H H",
                IW_TYPE_3,
                x16, y16, r16, g16, b16
            ))
        
        # Chunk into packets
        for i in range(0, len(samples), max_points_per_packet):
            chunk = b"".join(samples[i:i + max_points_per_packet])
            if chunk:
                self.sock.sendto(chunk, (self.ip, self.port))
                if self.point_delay > 0:
                    time.sleep(self.point_delay)

def main():
    ap = argparse.ArgumentParser(description="ILDA to IWP Sender")
    ap.add_argument("--file", required=True, help="Path to ILDA .ild file")
    ap.add_argument("--ip", required=True, help="Projector IP")
    ap.add_argument("--scan", type=int, default=1000, help="Scan rate in Hz")
    ap.add_argument("--fps", type=float, default=0, help="Frame rate. If >0, used to time points; else send as fast as possible.")
    ap.add_argument("--repeat", type=int, default=1, help="How many times to play the file. 0 = infinite")
    args = ap.parse_args()

    frames, palette = parse_ilda(args.file)
    if not frames:
        print("No frames parsed or unsupported file.")
        sys.exit(1)

    point_delay = 0.0
    if args.fps > 0:
        point_delay = 1.0 / args.fps
        
    sender = ProjectorSender(args.ip, args.scan, point_delay=point_delay)

    try:
        loops = 0
        while args.repeat == 0 or loops < args.repeat:
            for fr in frames:
                sender.send_frame(fr.points)
            loops += 1
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
