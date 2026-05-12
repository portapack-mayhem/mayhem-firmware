/*
 * Copyright (C) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "meteor_lrpt_msumr.hpp"

#include <algorithm>
#include <cstring>

namespace meteor_lrpt {

namespace {

constexpr size_t kMaxUserField = 8192;

size_t secondary_assumed_len(const uint8_t ph_be[kCcsdsSpacePacketPrimaryBytes]) {
    return ccsds_primary_has_secondary(ph_be) ? 10u : 0u;
}

}  // namespace

bool msumr_lrpt_mpdu_field(
    const uint8_t* cadu,
    const size_t cadu_len,
    uint16_t& first_header_pointer_out,
    const uint8_t*& mpdu_data_out,
    size_t& mpdu_data_len_out) {
    first_header_pointer_out = kMsumrMpduIdleFhp;
    mpdu_data_out = nullptr;
    mpdu_data_len_out = 0;
    if (!cadu || cadu_len < kMsumrLrptMpduDataOffsetFromCadu)
        return false;
    if (std::memcmp(cadu, kCcsdsCaduAsm, 4) != 0)
        return false;
    first_header_pointer_out = (uint16_t)((((unsigned)cadu[12] & 7u) << 8) | (unsigned)cadu[13]);
    mpdu_data_out = cadu + kMsumrLrptMpduDataOffsetFromCadu;
    const size_t rem = cadu_len - kMsumrLrptMpduDataOffsetFromCadu;
    mpdu_data_len_out = std::min(rem, kMsumrMpduDataBytes);
    return true;
}

size_t ccsds_payload_offset(
    const uint8_t* buf,
    size_t len,
    size_t primary_offset,
    const bool use_fixed_secondary_len) {
    if (primary_offset + kCcsdsSpacePacketPrimaryBytes > len)
        return 0;
    const uint8_t* ph = buf + primary_offset;
    const size_t sec = use_fixed_secondary_len ? secondary_assumed_len(ph) : 0u;
    const size_t off = primary_offset + kCcsdsSpacePacketPrimaryBytes + sec;
    return off > len ? 0 : off;
}

size_t ccsds_total_length(
    const uint8_t* buf,
    size_t len,
    size_t primary_offset,
    const bool use_fixed_secondary_len) {
    if (primary_offset + kCcsdsSpacePacketPrimaryBytes > len)
        return 0;
    const uint8_t* ph = buf + primary_offset;
    const uint32_t dlen_field = ccsds_primary_data_length_field(ph);
    const size_t user_len = (size_t)dlen_field + 1u;
    if (user_len > kMaxUserField)
        return 0;
    const size_t sec = use_fixed_secondary_len ? secondary_assumed_len(ph) : 0u;
    const size_t total = kCcsdsSpacePacketPrimaryBytes + sec + user_len;
    if (primary_offset + total > len)
        return 0;
    (void)buf;
    return total;
}

uint16_t ccsds_space_packet_pec_crc16_ccitt(const uint8_t* packet, const size_t total_octets) {
    if (!packet || total_octets < 8)
        return 0;
    constexpr int CCITT_CRC_GEN = 0x1021;
    uint16_t crc = 0xFFFF;
    const size_t n = total_octets - 2u;
    for (size_t j = 0; j < n; j++) {
        uint16_t data_byte = (uint16_t)packet[j] << 8;
        for (int i = 8; i > 0; i--) {
            if ((data_byte ^ crc) & 0x8000)
                crc = (uint16_t)((crc << 1) ^ CCITT_CRC_GEN);
            else
                crc = (uint16_t)(crc << 1);
            data_byte <<= 1;
        }
    }
    return crc;
}

bool ccsds_try_parse_space_packet(
    const uint8_t* buf,
    const size_t len,
    const size_t start,
    const bool use_fixed_secondary_len,
    size_t& total_out,
    uint16_t& apid_out,
    size_t& user_off,
    size_t& user_len) {
    total_out = 0;
    apid_out = 0;
    user_off = 0;
    user_len = 0;
    if (start + kCcsdsSpacePacketPrimaryBytes > len)
        return false;
    const uint8_t* ph = buf + start;
    if (ccsds_primary_version(ph) != 0u)
        return false;
    const size_t total = ccsds_total_length(buf, len, start, use_fixed_secondary_len);
    if (total == 0)
        return false;
    const size_t pay_off = ccsds_payload_offset(buf, len, start, use_fixed_secondary_len);
    if (pay_off == 0)
        return false;
    apid_out = ccsds_space_packet_apid(ph);
    user_off = pay_off;
    user_len = total - (pay_off - start);
    if (total >= 8 && user_len >= 2) {
        const uint16_t rx = (uint16_t)(
            ((unsigned)buf[start + total - 2] << 8) | (unsigned)buf[start + total - 1]);
        const uint16_t cv = ccsds_space_packet_pec_crc16_ccitt(buf + start, total);
        if (cv == rx)
            user_len -= 2;
    }
    total_out = total;
    return true;
}

}  // namespace meteor_lrpt
