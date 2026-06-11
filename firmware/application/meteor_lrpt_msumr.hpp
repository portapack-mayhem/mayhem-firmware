/*
 * Copyright (C) 2026
 *
 * CCSDS Space Packet primary header helpers for MSU-MR / G4
 * (SatDump: plugins/meteor_support/meteor/instruments/msumr/).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef APPLICATION_METEOR_LRPT_MSUMR_HPP
#define APPLICATION_METEOR_LRPT_MSUMR_HPP

#include <cstddef>
#include <cstdint>

namespace meteor_lrpt {

/** CCSDS CADU attachment sync marker (big-endian) after RS + derandomization (Mayhem REC body). */
constexpr uint8_t kCcsdsCaduAsm[4] = {0x1A, 0xCF, 0xFC, 0x1D};

/** CADU payload bytes written per Mayhem `RecordView` row (ASM + 1016 B transfer zone fields). */
constexpr size_t kMeteorCaduRecBytes = 1020;

/**
 * SatDump MSU-MR LRPT CADU path (`module_meteor_msumr_lrpt.cpp` + `ccsds_aos/mpdu.cpp`):
 * `Demuxer(882, true)` — MPDU data field is 882 B; insert zone 2 B after the 6-byte VCDU primary
 * (`parseVCDU` reads bytes [4..9] of the CADU buffer). First Header Pointer is 11 bits at bytes [12..13];
 * space-packet octets start at offset **14** from the CADU origin (ASM still at [0..3]).
 */
constexpr size_t kMsumrLrptMpduDataOffsetFromCadu = 14;
constexpr size_t kMsumrMpduDataBytes = 882;
constexpr uint16_t kMsumrMpduIdleFhp = 2047;

/**
 * Parse M-PDU FHP and data pointer like SatDump `parseMPDU(cadu, true, 2)`.
 * Returns false if `cadu_len` is too short or ASM is missing.
 */
bool msumr_lrpt_mpdu_field(
    const uint8_t* cadu,
    size_t cadu_len,
    uint16_t& first_header_pointer_out,
    const uint8_t*& mpdu_data_out,
    size_t& mpdu_data_len_out);

/**
 * LRPT virtual channel ID from first VCDU primary header octet after ASM (SatDump `vcdu.cpp` byte 5, low 6 bits).
 * Only meaningful when the 4-byte ASM prefix matches `kCcsdsCaduAsm`.
 */
inline uint8_t lrpt_vcid_from_cadu(const uint8_t* cadu1020) {
    if (!cadu1020)
        return 0xFF;
    return (uint8_t)(cadu1020[5] & 0x3Fu);
}

/**
 * CCSDS Space Packet **Packet Error Control** (CRC-16-CCITT, SatDump `crcCheckCCITT` in `ccsds.cpp`):
 * CRC is computed over the whole space packet except the last two octets; if it equals those octets (big-endian),
 * `ccsds_try_parse_space_packet` strips the PEC from the returned user field length.
 */
uint16_t ccsds_space_packet_pec_crc16_ccitt(const uint8_t* packet, size_t total_octets);

/** Primary header length (CCSDS Space Packet). */
constexpr size_t kCcsdsSpacePacketPrimaryBytes = 6;

/** 11-bit APID from first two bytes of CCSDS Space Packet primary header (big-endian wire order). */
inline uint16_t ccsds_space_packet_apid(const uint8_t hdr_be[2]) {
    const unsigned w = ((unsigned)hdr_be[0] << 8) | (unsigned)hdr_be[1];
    return (uint16_t)(w & 0x07FFu);
}

/** Version field (3 bits) — expect 0 for telemetry. */
inline uint8_t ccsds_primary_version(const uint8_t ph_be[kCcsdsSpacePacketPrimaryBytes]) {
    return (uint8_t)(ph_be[0] >> 5u);
}

/** Secondary header flag (bit 3 of first octet). */
inline bool ccsds_primary_has_secondary(const uint8_t ph_be[kCcsdsSpacePacketPrimaryBytes]) {
    return (ph_be[0] & 0x08u) != 0;
}

/** Sequence flags: bits 14–15 of 16-bit field formed by bytes 2–3 (big-endian). */
inline uint8_t ccsds_primary_sequence_flags(const uint8_t ph_be[kCcsdsSpacePacketPrimaryBytes]) {
    const unsigned w = ((unsigned)ph_be[2] << 8) | (unsigned)ph_be[3];
    return (uint8_t)((w >> 14) & 3u);
}

/** `data_length` field (last 16 bits of primary header, big-endian). CCSDS: value = (user data octets − 1). */
inline uint16_t ccsds_primary_data_length_field(const uint8_t ph_be[kCcsdsSpacePacketPrimaryBytes]) {
    return (uint16_t)(((unsigned)ph_be[4] << 8) | (unsigned)ph_be[5]);
}

/** Byte offset from `buf` start to first user-data octet after primary + assumed secondary header. */
size_t ccsds_payload_offset(
    const uint8_t* buf,
    size_t len,
    size_t primary_offset,
    bool use_fixed_secondary_len);

/** Total space-packet size in octets (primary + secondary + user data), or 0 if invalid. */
size_t ccsds_total_length(
    const uint8_t* buf,
    size_t len,
    size_t primary_offset,
    bool use_fixed_secondary_len);

/**
 * Try to interpret a Space Packet starting at `start`.
 * On success: `total_out` whole packet size, `apid_out`, `user_off` offset to user field, `user_len` user field length.
 */
bool ccsds_try_parse_space_packet(
    const uint8_t* buf,
    size_t len,
    size_t start,
    bool use_fixed_secondary_len,
    size_t& total_out,
    uint16_t& apid_out,
    size_t& user_off,
    size_t& user_len);

}  // namespace meteor_lrpt

#endif
