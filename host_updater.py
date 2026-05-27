#!/usr/bin/env python3
"""
Host-side firmware updater — simulates what a PC tool does over UART.
Packages a firmware binary into framed packets and writes to /tmp/firmware_update.bin
On real hardware this would send bytes over pyserial to the STM32 UART port.
"""

import struct
import os
import sys

PACKET_MAGIC     = 0xAA55
PACKET_DATA_SIZE = 128
OUTPUT_FILE      = "/tmp/firmware_update.bin"

def crc16(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc

def package_firmware(firmware_bytes: bytes) -> list:
    packets = []
    offset  = 0
    seq     = 0
    while offset < len(firmware_bytes):
        chunk = firmware_bytes[offset:offset + PACKET_DATA_SIZE]
        # Pad to PACKET_DATA_SIZE
        padded = chunk + b'\xFF' * (PACKET_DATA_SIZE - len(chunk))
        crc    = crc16(chunk)
        # Pack: magic(2) + seq(2) + len(2) + crc16(2) + data(128)
        pkt = struct.pack('<HHHH', PACKET_MAGIC, seq, len(chunk), crc) + padded
        packets.append(pkt)
        offset += len(chunk)
        seq    += 1
    return packets

def main():
    # Generate a fake firmware binary (simulates motor control app)
    print("[HOST] Generating synthetic firmware image...")
    firmware = bytearray()
    # Fake ARM Cortex-M vector table entries
    firmware += struct.pack('<I', 0x20020000)  # initial stack pointer
    firmware += struct.pack('<I', 0x08009009)  # reset handler address
    # Fill rest with a recognizable pattern
    for i in range(512 - 8):
        firmware += bytes([i & 0xFF])
    firmware = bytes(firmware)

    print(f"[HOST] Firmware size: {len(firmware)} bytes")

    packets = package_firmware(firmware)
    print(f"[HOST] Packaged into {len(packets)} packets ({PACKET_DATA_SIZE} bytes each)")

    with open(OUTPUT_FILE, 'wb') as f:
        for pkt in packets:
            f.write(pkt)

    print(f"[HOST] Written to {OUTPUT_FILE}")
    print(f"[HOST] Now run ./bootloader_sim with update trigger active")

if __name__ == "__main__":
    main()