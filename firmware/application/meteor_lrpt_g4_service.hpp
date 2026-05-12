/*
 * Copyright (C) 2026
 *
 * G4 product path (MSU-MR demux, JPEG via tjpgd, RGB BMP to SD). Heavy decode stays off M4.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef APPLICATION_METEOR_LRPT_G4_SERVICE_HPP
#define APPLICATION_METEOR_LRPT_G4_SERVICE_HPP

#include <cstdint>

/** LRPT UI / settings: bit6 enables M0 G4 worker; bit7 enables M4→M0 live CADU ring (no second REC file). */
constexpr uint8_t kMeteorLrptFlagG4Decode = 1u << 6;
constexpr uint8_t kMeteorLrptFlagG4LiveRing = 1u << 7;

/** `SharedMemory::MeteorLrptG4Ipc::drop_bits` — sticky OR across the session. */
enum meteor_lrpt_g4_drop_bit : uint32_t {
    G4_DROP_BAD_ASM = 1u << 0,
    G4_DROP_DEMUX_SPARSE = 1u << 1,
    G4_DROP_PACKET_PARSE = 1u << 2,
    G4_DROP_REASSEMBLY_OVERFLOW = 1u << 3,
    G4_DROP_JPEG_PREPARE = 1u << 4,
    G4_DROP_JPEG_DECOMP = 1u << 5,
    G4_DROP_BMP = 1u << 6,
    G4_DROP_PROGRESSIVE_JPEG = 1u << 7,
    /** Live ring full: M4 dropped an **incoming** post-RS CADU (M0 did not consume slots fast enough). */
    G4_DROP_RING_OVERFLOW = 1u << 8,
    /** MPDU reassembly saw a different LRPT VCID than the segment that opened the bucket (byte 5 of CADU, low 6 bits). */
    G4_DROP_VCID_MISMATCH = 1u << 9,
    /** Reserved: strict CCSDS PEC (CRC-16) mismatch vs trailer — see `ccsds_try_parse_space_packet` strip-on-match path. */
    G4_DROP_PACKET_CRC = 1u << 10,
    /** CADU tail file seek/read/size mismatch (not BMP encode). */
    G4_DROP_CADU_IO = 1u << 11,
};

/** One-time M0 init: starts the G4 worker thread (idempotent). */
void meteor_lrpt_g4_init();

/** Called whenever LRPT RX is configured; mirrors `flags` into shared G4 IPC. */
void meteor_lrpt_g4_configure(uint8_t lrpt_flags);

/** M0 only: copy UTF-8 path (ASCII SD paths). Empty clears to default tail file. */
void meteor_lrpt_g4_set_input_path_utf8(const char* path_utf8);

/** M0 only: `debug_flags` bit0 = write `.ppm` next to BMP; other bits reserved. */
void meteor_lrpt_g4_set_trace_flags(uint8_t trace_flags);

#endif
