/*
 * Copyright (C) 2026 Frederic BORRY - ADRASEC 31
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#ifndef __BEACON_H__
#define __BEACON_H__

#include "portapack.hpp"
#include "ui_epirb_tx.hpp"
#include "location.hpp"

namespace ui::external_app::epirb_tx {

/**
 * Get bits frop the provided frame
 * @param data the frame data
 * @param startBit the start bit number (one based to match documentation)
 * @param endBit the end bit number (one based to match documentation)
 * @return the selected bits (max 64)
 */
static uint64_t get_bits(uint8_t* data, int startBit, int endBit) {
    uint64_t result = 0;
    // 0 bases bit count
    startBit--;
    int numBits = endBit - startBit;

    // get a pointer to the starting byte...
    const uint8_t* pData = &(data[startBit / 8]);
    uint8_t b = *pData;

    // calculate the starting bit within that byte...
    int bitOffset = 7 - (startBit % 8);

    // iterate for the desired number of bits...
    for (int i = 0; i < numBits; ++i) {
        // make room for the next bit...
        result <<= 1;
        // copy the bit...
        result |= ((b >> bitOffset) & 0x01);
        // reached the end of the current byte?
        if (--bitOffset < 0) {
            b = *(++pData);  // go to the next byte...
            bitOffset = 7;   // restart at the first bit in that byte...
        }
    }

    // all done...
    return result;
}

/**
 * Compute a BCH code
 * @param frame the frame data
 * @param startBit the bit to start BCH calculation
 * @param endBit the bit to stop calculation
 * @param poly the BCH polynome
 * @param polyLength the BCH polynome length
 * @return the BCH code
 */
static uint64_t compute_bch(uint8_t* frame, int startBit, int endBit, unsigned long poly, int polyLength) {  // Length of data to be checked (not including the BCH code)
    int dataLength = endBit - startBit + 1;
    // Total lengh (including the BCH code that will be padded to zeros (BCH code length is polyLengh-1))
    int totalLength = dataLength + polyLength - 1;
    // Start with the first polyLength bits
    uint64_t result = get_bits(frame, startBit, startBit + polyLength - 1);
    for (int i = polyLength; i <= totalLength; i++) {  // Iterate on each bit after the first polyLength batch
        bool firstBit = result >> (polyLength - 1);
        if (firstBit) {  // We have a leading 1 => xor the result with the poly
            result = result ^ poly;
        }
        if (i < totalLength) {  // Move to next bit
            result = result << 1;
            if (i < dataLength) {  // Append next bit
                result |= get_bits(frame, startBit + i, startBit + i);
            }  // else : 0 padding after data length
        }
    }
    return result;
}

// 21 bits BCH polynomial
#define BCH_21_POLYNOMIAL 0b1001101101100111100011UL
#define BCH_21_POLY_LENGTH 22
// 12 bits BCH polynomial
#define BCH_12_POLYNOMIAL 0b1010100111001UL
#define BCH_12_POLY_LENGTH 13

/**
 * Combyte BCH1 code for the provided frame
 */
static uint64_t compute_bch1(uint8_t* frame) {
    return compute_bch(frame, 25, 85, BCH_21_POLYNOMIAL, BCH_21_POLY_LENGTH);
}

/**
 * Combyte BCH2 code for the provided frame
 */
static uint64_t compute_bch2(uint8_t* frame) {
    return compute_bch(frame, 107, 132, BCH_12_POLYNOMIAL, BCH_12_POLY_LENGTH);
}

/**
 * Set a bit in the provided frame
 * @param buf the frame
 * @param bit the position of the bit to set (0 based)
 * @param v the bit value
 */
static void set_bit(uint8_t* buf, int bit, bool v) {
    int byte = bit >> 3;
    int off = 7 - (bit & 7);

    if (v)
        buf[byte] |= (1 << off);
    else
        buf[byte] &= ~(1 << off);
}

/**
 * Push bits to the provided frame
 * @param buf the frame
 * @param pos the current frame possition (in/out, will be incremented during the opreration)
 * @param v the bits to push (max 64)
 * @param n the number of bits to push
 */
static void push_bits(uint8_t* buf, int& pos, uint64_t v, int n) {
    for (int i = n - 1; i >= 0; i--)
        set_bit(buf, pos++, (v >> i) & 1);
}

/**
 * Generate a beacon in the provided buffer
 * @param frame the buffer to generate the frame in
 * @param params the beacon parameters
 * @return the size of the generated frame
 */
size_t generate_beacon(uint8_t* frame, const BeaconParams& params) {
    // Clear the content of the frame
    memset(frame, 0, 18);

    int pos = 0;
    uint32_t deg, min, sec;

    // bit sync
    for (int i = 0; i < 15; i++)
        push_bits(frame, pos, 1, 1);

    // frame sync
    push_bits(frame, pos, params.is_test ? 0b011010000 : 0b000101111, 9);

    // PDF-1 (Protexted Data field 1)
    int pdf1_start = pos;

    push_bits(frame, pos, 1, 1);  // format flag (long)
    bool is_user = (params.protocol == BeaconProtocol::USER);
    bool is_standard = (params.protocol == BeaconProtocol::STANDARD);
    push_bits(frame, pos, is_user, 1);          // protocol flag
    push_bits(frame, pos, params.country, 10);  // country code
    switch (params.type) {
        case BeaconType::EPIRB:
            if (is_user)
                push_bits(frame, pos, 0b010, 3);
            else if (is_standard)
                push_bits(frame, pos, 0b0010, 4);
            else
                push_bits(frame, pos, 0b1010, 4);
            break;
        case BeaconType::PLB:
            if (is_user)
                push_bits(frame, pos, 0b011, 3);
            else if (is_standard)
                push_bits(frame, pos, 0b0111, 4);
            else
                push_bits(frame, pos, 0b1011, 4);
            break;
        default:
        case BeaconType::ELT:
            if (is_user)
                push_bits(frame, pos, 0b001, 3);
            else if (is_standard)
                push_bits(frame, pos, 0b0011, 4);
            else
                push_bits(frame, pos, 0b1000, 4);
            break;
    }

    // Fill the rest of PDF 1 with zeros
    while (pos < pdf1_start + 61)
        push_bits(frame, pos, 0, 1);

    if (is_user) {
        // User Location Protocol: no position in PDF1
        if (params.type == BeaconType::PLB) {
            // Set bits for PLB
            set_bit(frame, 40 - 1, 1);
            set_bit(frame, 41 - 1, 1);
            set_bit(frame, 42 - 1, 0);
        }

        set_bit(frame, 85 - 1, params.has_121_5);
    } else if (is_standard) {
        // Standard  Location Protocol
        // North / south
        set_bit(frame, 65 - 1, params.location.south);
        // E / W
        set_bit(frame, 75 - 1, params.location.west);
        pos = 66 - 1;
        // Latitude 1/4 degrees (9 bits)
        deg = (params.location.lat_deg << 2) + (params.location.lat_min / 15);
        push_bits(frame, pos, deg, 9);
        pos = 76 - 1;
        // Longitude 1/4 degrees (10 bits)
        deg = (params.location.long_deg << 2) + (params.location.long_min / 15);
        push_bits(frame, pos, deg, 10);
        pos = pdf1_start + 61;
    } else {
        // National Location Protocol
        // North / south
        set_bit(frame, 59 - 1, params.location.south);
        // E / W
        set_bit(frame, 72 - 1, params.location.west);
        pos = 60 - 1;
        // Latitude degrees (7 bits)
        push_bits(frame, pos, params.location.lat_deg, 7);
        // Latitude min (5 bits, 2 min increment)
        min = params.location.lat_min / 2;
        push_bits(frame, pos, min, 5);
        pos = 73 - 1;
        // Longitude degrees (8 bits)
        push_bits(frame, pos, params.location.long_deg, 8);
        // Longiitude min (5 bits, 2 min increment)
        min = params.location.long_min / 2;
        push_bits(frame, pos, min, 5);
        pos = pdf1_start + 61;
    }

    // Set BCH1
    uint64_t bch1 = compute_bch1(frame);
    push_bits(frame, pos, bch1, 21);

    // PDF-2 (Protexted Data Field 2)
    int pdf2_start = pos;
    if (is_user) {
        // User Location Protocol
        push_bits(frame, pos, params.is_internal, 1);
        // Latitude N/S
        push_bits(frame, pos, params.location.south, 1);
        // Latitude degrees (7 bits)
        push_bits(frame, pos, params.location.lat_deg, 7);
        // Latitude minutes (4 bits, 4 minutes precision)
        min = params.location.lat_min / 4;
        push_bits(frame, pos, min, 4);
        // Longitude E/W
        push_bits(frame, pos, params.location.west, 1);
        // Longitude degrees (8 bits)
        push_bits(frame, pos, params.location.long_deg, 8);
        // Longitude minutes (4 bits, 4 minutes precision)
        min = params.location.long_min / 4;
        push_bits(frame, pos, min, 4);
    } else if (is_standard) {
        // Standard Location Protocol
        push_bits(frame, pos, 0b1101, 4);
        push_bits(frame, pos, params.is_internal, 1);
        push_bits(frame, pos, params.has_121_5, 1);
        // Latiitude
        // +
        push_bits(frame, pos, 1, 1);
        // Min
        min = params.location.lat_min % 15;
        push_bits(frame, pos, min, 5);
        // Sec (4 bits, 4 sec precision)
        sec = params.location.lat_sec / 4;
        push_bits(frame, pos, sec, 4);
        // Longitude
        // +
        push_bits(frame, pos, 1, 1);
        // Min
        min = params.location.long_min % 15;
        push_bits(frame, pos, min, 5);
        // Sec (4 bits, 4 sec precision)
        sec = params.location.long_sec / 4;
        push_bits(frame, pos, sec, 4);
    } else {
        // National Location Protocol
        push_bits(frame, pos, 0b1101, 4);
        push_bits(frame, pos, params.is_internal, 1);
        push_bits(frame, pos, params.has_121_5, 1);
        // Lat
        // +
        push_bits(frame, pos, 1, 1);
        // Min
        push_bits(frame, pos, (params.location.lat_min % 2), 2);
        // Sec (4 bits, 4 sec precision)
        sec = params.location.lat_sec / 4;
        push_bits(frame, pos, sec, 4);
        // LLon
        // +
        push_bits(frame, pos, 1, 1);
        // Min
        push_bits(frame, pos, (params.location.long_min % 2), 2);
        // Sec (4 bits, 4 sec precision)
        sec = params.location.long_sec / 4;
        push_bits(frame, pos, sec, 4);
    }

    while (pos < pdf2_start + 26)
        push_bits(frame, pos, 0, 1);

    // Compute BCH 2
    uint64_t bch2 = compute_bch2(frame);
    push_bits(frame, pos, bch2, 12);
    return 18;
}

/**
 * Convert a beacon hex string representation (generic range)
 * @param frame the frame data
 * @param offset_bytes starting byte offset
 * @param count_bytes number of bytes to convert
 * @return the hex string representation
 */
std::string beacon_to_hex_string_range(const uint8_t* frame, int offset_bytes, int count_bytes) {
    static const char hex[] = "0123456789ABCDEF";
    std::string out;
    out.resize(count_bytes * 2);
    for (int i = 0; i < count_bytes; i++) {
        uint8_t b = frame[offset_bytes + i];
        out[i * 2] = hex[b >> 4];
        out[i * 2 + 1] = hex[b & 0x0F];
    }
    return out;
}

// ---------------------------------------------------------------------------
// SGB (Second Generation Beacon) — T.018 Rev 7, March 2021
// ---------------------------------------------------------------------------

// Reference values from T.018 canonical test vector (Appendix B.1)
#define SGB_TAC_NUMBER 230     // TAC number (16 bits, 0–65535)
#define SGB_SERIAL_NUMBER 573  // Serial number within TAC (14 bits, 0–16383)
#define SGB_HOMING_DEVICE 1    // 1 = beacon has 121.5 / 243 MHz homing device
#define SGB_RLS_FUNCTION 0     // 0 = no Return Link Service function

// BCH(250,202) generator polynomial — lower 48 bits (without leading X^48 term)
// Full g(x): X^48+X^47+X^46+X^42+X^41+X^40+X^39+X^38+X^37+X^35+X^33+X^32+X^31
//            +X^26+X^24+X^23+X^22+X^20+X^19+X^18+X^17+X^16+X^13+X^12+X^11+X^10
//            +X^7+X^4+X^2+X+1
// Binary MSB-first: 1110001111110101110000101110111110011110010010111 (T.018 App. B.1)
#define SGB_BCH_G_LOWER UINT64_C(0xC7EB85DF3C97)

/**
 * Encode latitude for SGB GNSS location protocol (T.018 Appendix C)
 * Pushes 23 bits: 1-bit N/S flag + 7-bit degrees + 15-bit decimal fraction
 * @param frame the frame buffer
 * @param pos current bit position (modified in-place)
 * @param south true if southern hemisphere
 * @param lat_mag absolute latitude in decimal degrees (0..90)
 */
static void push_sgb_latitude(uint8_t* frame, int& pos, bool south, float lat_mag) {
    push_bits(frame, pos, south ? 1 : 0, 1);
    uint32_t deg = (uint32_t)lat_mag;
    if (deg > 90) deg = 90;
    float frac = lat_mag - (float)deg;
    uint32_t frac_n = (uint32_t)(frac * 32768.0f + 0.5f);
    if (frac_n >= 32768) {
        frac_n = 0;
        if (deg < 90) deg++;
    }
    push_bits(frame, pos, deg, 7);
    push_bits(frame, pos, frac_n, 15);
}

/**
 * Encode longitude for SGB GNSS location protocol (T.018 Appendix C)
 * Pushes 24 bits: 1-bit E/W flag + 8-bit degrees + 15-bit decimal fraction
 * @param frame the frame buffer
 * @param pos current bit position (modified in-place)
 * @param west true if western hemisphere
 * @param lon_mag absolute longitude in decimal degrees (0..180)
 */
static void push_sgb_longitude(uint8_t* frame, int& pos, bool west, float lon_mag) {
    push_bits(frame, pos, west ? 1 : 0, 1);
    uint32_t deg = (uint32_t)lon_mag;
    if (deg > 180) deg = 180;
    float frac = lon_mag - (float)deg;
    uint32_t frac_n = (uint32_t)(frac * 32768.0f + 0.5f);
    if (frac_n >= 32768) {
        frac_n = 0;
        if (deg < 180) deg++;
    }
    push_bits(frame, pos, deg, 8);
    push_bits(frame, pos, frac_n, 15);
}

/**
 * Compute BCH(250,202) parity using LFSR method (T.018 Appendix B.1)
 * Reads 202 message bits from buffer positions 6..207 and writes
 * 48 BCH parity bits to buffer positions 208..255.
 * @param frame the 32-byte SGB buffer (message already written at bits 6..207)
 */
static void compute_sgb_bch(uint8_t* frame) {
    uint64_t reg = 0;
    for (int i = 0; i < 202; i++) {
        int bp = i + 6;  // buffer bit position (6 = start of message after padding)
        bool msg_bit = (frame[bp >> 3] >> (7 - (bp & 7))) & 1;
        bool feedback = ((reg >> 47) & 1) ^ (msg_bit ? 1u : 0u);
        reg = (reg << 1) & UINT64_C(0xFFFFFFFFFFFF);
        if (feedback) reg ^= SGB_BCH_G_LOWER;
    }
    // Append 48-bit remainder at buffer positions 208..255
    int pos = 208;
    push_bits(frame, pos, reg, 48);
}

/**
 * Map BeaconType to SGB 3-bit beacon type code (T.018 Table 3.1, bits 138-140)
 * 000=ELT, 001=EPIRB, 010=PLB
 */
static uint8_t sgb_beacon_type_code(BeaconType t) {
    switch (t) {
        case BeaconType::EPIRB:
            return 0b001;
        case BeaconType::PLB:
            return 0b010;
        default:
        case BeaconType::ELT:
            return 0b000;
    }
}

/**
 * Generate a Second Generation Beacon (SGB / T.018) frame.
 * The 250-bit frame is stored in 32 bytes with 6 leading padding bits
 * (bits 0..4 = 0, bit 5 = 1).
 * The rotating field is Type 0 — G.008 Objective Requirements (Table 3.3),
 * with elapsed_hours and time_last_loc_min updated from elapsed_s.
 * @param frame     32-byte output buffer
 * @param params    beacon parameters
 * @param elapsed_s seconds elapsed since beacon activation (drives rotating field)
 * @return 32 (size of the generated frame in bytes)
 */
size_t generate_sgb_beacon(uint8_t* frame, const BeaconParams& params, uint32_t elapsed_s) {
    memset(frame, 0, 32);

    // 6 padding bits at start of 32-byte buffer: [0,0,0,0,1,0]  (5th bit = 1 to indicate self-test mode)
    set_bit(frame, 4, 1);

    // Message bits start at buffer bit position 6 (= SGB message bit 1)
    int pos = 6;

    // Bits  1-16:  TAC number (16 bits) — T.018 Table 3.1
    push_bits(frame, pos, SGB_TAC_NUMBER, 16);
    // Bits 17-30:  Serial number within TAC (14 bits)
    push_bits(frame, pos, SGB_SERIAL_NUMBER, 14);
    // Bits 31-40:  Country code (10 bits, ITU MID)
    push_bits(frame, pos, params.country & 0x3FFU, 10);
    // Bit  41:     Status of homing device (1 = has 121.5/243 MHz homing device)
    push_bits(frame, pos, params.has_121_5 ? 1 : 0, 1);
    // Bit  42:     RLS function flag (0 = no RLS)
    push_bits(frame, pos, SGB_RLS_FUNCTION, 1);
    // Bit  43:     Test protocol flag
    push_bits(frame, pos, params.is_test ? 1 : 0, 1);

    // Bits 44-66:  Encoded GNSS latitude (23 bits) — T.018 Appendix C
    float lat_mag = params.location.latitude < 0.0f ? -params.location.latitude : params.location.latitude;
    push_sgb_latitude(frame, pos, params.location.south, lat_mag);
    // Bits 67-90:  Encoded GNSS longitude (24 bits)
    float lon_mag = params.location.longitude < 0.0f ? -params.location.longitude : params.location.longitude;
    push_sgb_longitude(frame, pos, params.location.west, lon_mag);

    // Bits 91-137: Vessel ID (47 bits = 3-bit type selector + 44-bit body)
    // Type 000 = No aircraft/maritime identity; body all zeros
    push_bits(frame, pos, 0, 47);

    // Bits 138-140: Beacon type (3 bits)
    push_bits(frame, pos, sgb_beacon_type_code(params.type), 3);

    // Bits 141-154: Spare (14 bits, all 1s in normal operation)
    push_bits(frame, pos, 0x3FFFU, 14);

    // Bits 155-202: Rotating Field Type 0 — G.008 Objective Requirements (Table 3.3)
    // Bits 155-158: Identifier (4 bits = 0000)
    push_bits(frame, pos, 0b0000U, 4);
    // Bits 159-164: Elapsed time since activation (6 bits, 0-63 h, truncated at 63)
    uint32_t elapsed_h = elapsed_s / 3600;
    if (elapsed_h > 63) elapsed_h = 63;
    push_bits(frame, pos, elapsed_h, 6);
    // Bits 165-175: Time from last encoded location (11 bits, 0-2046 min; 2047 = no fix)
    uint32_t loc_age_min = elapsed_s / 60;
    if (loc_age_min > 2046) loc_age_min = 2046;
    push_bits(frame, pos, loc_age_min, 11);
    // Bits 176-185: Altitude of encoded location (10 bits; 0x3FF = no altitude)
    push_bits(frame, pos, 0x3FFU, 10);
    // Bits 186-193: Dilution of Precision (4-bit HDOP + 4-bit VDOP; 0 = best HDOP, 0 = best VDOP)
    push_bits(frame, pos, 0b00000000U, 8);
    // Bits 194-195: Activation notification (00 = manual by user)
    push_bits(frame, pos, 0b00U, 2);
    // Bits 196-198: Remaining battery capacity (101 = >75%)
    push_bits(frame, pos, 0b101U, 3);
    // Bits 199-200: GNSS status (10 = 3D fix)
    push_bits(frame, pos, 0b10U, 2);
    // Bits 201-202: Spare (00)
    push_bits(frame, pos, 0b00U, 2);

    // pos == 208: 6 padding + 202 message bits consumed

    // Bits 203-250: BCH(250,202) parity (48 bits) computed and written at positions 208-255
    compute_sgb_bch(frame);

    return 32;
}

}  // namespace ui::external_app::epirb_tx

#endif /*__BEACON_H__*/