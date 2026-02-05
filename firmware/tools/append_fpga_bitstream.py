#!/usr/bin/env python3
#
# Append FPGA bitstream to PRALINE firmware
#
# This script takes a PortaPack firmware.bin and appends the PRALINE FPGA
# bitstream at offset 0x100000 (1MB), then recalculates the checksum.
#
# Usage: append_fpga_bitstream.py <firmware.bin> <fpga.bin> <output.bin> <total_size>
#

import sys
import os

def read_image(path):
    with open(path, 'rb') as f:
        return bytearray(f.read())

def write_image(data, path):
    with open(path, 'wb') as f:
        f.write(data)

def main():
    if len(sys.argv) != 5:
        print("Usage: append_fpga_bitstream.py <firmware.bin> <fpga.bin> <output.bin> <total_size>")
        print("  firmware.bin: Input PortaPack firmware (1MB with checksum)")
        print("  fpga.bin:     PRALINE FPGA bitstream (LZ4 compressed)")
        print("  output.bin:   Output combined firmware")
        print("  total_size:   Total flash size in bytes (e.g., 2097152 for 2MB)")
        sys.exit(1)

    firmware_path = sys.argv[1]
    fpga_path = sys.argv[2]
    output_path = sys.argv[3]
    total_size = int(sys.argv[4], 0)

    # FPGA bitstream offset in flash
    # PRALINE: Moved to 3.5MB to allow larger base firmware
    FPGA_OFFSET = 0x380000  # 3.5MB (was 0x180000 = 1.5MB)

    # Read input files
    firmware = read_image(firmware_path)
    fpga = read_image(fpga_path)

    print(f"Firmware size: {len(firmware)} bytes")
    print(f"FPGA bitstream size: {len(fpga)} bytes")
    print(f"Target total size: {total_size} bytes")

    # The firmware has a 4-byte checksum at the end
    # Remove it - we'll recalculate after adding FPGA bitstream
    firmware_content = firmware[:-4]

    # Check that firmware content fits before FPGA offset
    if len(firmware_content) > FPGA_OFFSET:
        print(f"ERROR: Firmware content ({len(firmware_content)} bytes) exceeds FPGA offset ({FPGA_OFFSET})")
        sys.exit(1)

    # Create output image
    output = bytearray()

    # Add firmware content (padded to FPGA_OFFSET with 0xFF)
    output.extend(firmware_content)
    pad_size = FPGA_OFFSET - len(firmware_content)
    output.extend(bytes([0xFF] * pad_size))

    print(f"Padded firmware to {len(output)} bytes (FPGA offset)")

    # Add FPGA bitstream
    output.extend(fpga)
    print(f"Added FPGA bitstream, total now {len(output)} bytes")

    # Check total size
    if len(output) > total_size - 4:
        print(f"ERROR: Combined image ({len(output)} bytes) exceeds max size ({total_size - 4} bytes)")
        sys.exit(1)

    # Pad to total_size - 4 (leaving room for checksum)
    pad_size = total_size - 4 - len(output)
    output.extend(bytes([0xFF] * pad_size))
    print(f"Padded to {len(output)} bytes (before checksum)")

    # Calculate checksum (same algorithm as make_spi_image.py)
    checksum = 0
    for i in range(0, len(output), 4):
        snippet = output[i:i + 4]
        if len(snippet) == 4:
            val = int.from_bytes(snippet, byteorder='little')
            checksum += val

    final_checksum = 0
    checksum = (final_checksum - checksum) & 0xFFFFFFFF
    output.extend(checksum.to_bytes(4, 'little'))

    print(f"Final image size: {len(output)} bytes")
    print(f"Checksum: 0x{checksum:08X}")

    # Write output
    write_image(output, output_path)
    print(f"Written to {output_path}")

    # Report space usage
    fpga_end = FPGA_OFFSET + len(fpga)
    space_after_fpga = total_size - 4 - fpga_end
    print(f"\nSpace usage:")
    print(f"  Firmware: 0x000000 - 0x{len(firmware_content):06X} ({len(firmware_content)} bytes)")
    print(f"  FPGA:     0x{FPGA_OFFSET:06X} - 0x{fpga_end:06X} ({len(fpga)} bytes)")
    print(f"  Free:     0x{fpga_end:06X} - 0x{total_size-4:06X} ({space_after_fpga} bytes)")

if __name__ == "__main__":
    main()
