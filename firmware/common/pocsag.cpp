/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include "pocsag.hpp"

#include "baseband_api.hpp"
#include "portapack.hpp"
using namespace portapack;

#include "string_format.hpp"
#include "utility.hpp"

namespace pocsag {

std::string bitrate_str(BitRate bitrate) {
    switch (bitrate) {
        case BitRate::FSK512:
            return "512bps ";
        case BitRate::FSK1200:
            return "1200bps";
        case BitRate::FSK2400:
            return "2400bps";
        default:
            return "????";
    }
}

std::string flag_str(PacketFlag packetflag) {
    switch (packetflag) {
        case PacketFlag::NORMAL:
            return "OK";
        case PacketFlag::TIMED_OUT:
            return "TIMED OUT";
        default:
            return "";
    }
}

void insert_BCH(BCHCode& BCH_code, uint32_t* codeword) {
    uint32_t parity = 0;
    int data[21];
    int bit;
    int* bb;
    size_t c;

    for (c = 0; c < 21; c++) {
        bit = (((*codeword) << c) & 0x80000000U) ? 1 : 0;
        if (bit) parity++;
        data[c] = bit;
    }

    bb = BCH_code.encode(data);

    // Make sure ECC bits are cleared
    (*codeword) &= 0xFFFFF801;

    for (c = 0; c < 10; c++) {
        bit = bb[c];
        (*codeword) |= (bit << (10 - c));
        if (bit) parity++;
    }

    // Even parity
    (*codeword) |= (parity & 1);
}

uint32_t get_digit_code(char code) {
    if ((code >= '0') && (code <= '9')) {
        code -= '0';
    } else {
        if (code == 'S')
            code = 10;
        else if (code == 'U')
            code = 11;
        else if (code == ' ')
            code = 12;
        else if (code == '-')
            code = 13;
        else if (code == ']')
            code = 14;
        else if (code == '[')
            code = 15;
        else
            code = 12;
    }

    code = ((code & 0x0C) >> 2) | ((code & 0x03) << 2);  // ----3210 -> ----1032
    code = ((code & 0x0A) >> 1) | ((code & 0x05) << 1);  // ----1032 -> ----0123

    return code;
}

void pocsag_encode(const MessageType type, BCHCode& BCH_code, const uint32_t function, const std::string message, const uint32_t address, std::vector<uint32_t>& codewords) {
    size_t b, c, address_slot;
    size_t bit_idx, char_idx = 0;
    uint32_t codeword, digit_code;
    char ascii_char = 0;

    size_t message_size = message.size();

    // Preamble
    for (b = 0; b < (POCSAG_PREAMBLE_LENGTH / 32); b++) {
        codewords.push_back(0xAAAAAAAA);
    }

    // Address
    codeword = (address & 0x1FFFF8U) << 10;
    address_slot = (address & 7) * 2;
    // Function
    codeword |= (function << 11);

    insert_BCH(BCH_code, &codeword);

    // Address batch
    codewords.push_back(POCSAG_SYNCWORD);
    for (c = 0; c < 16; c++) {
        if (c == address_slot) {
            codewords.push_back(codeword);
            if (type != MessageType::ADDRESS_ONLY) break;
        } else
            codewords.push_back(POCSAG_IDLEWORD);
    }

    if (type == MessageType::ADDRESS_ONLY) return;  // Done.

    c++;
    codeword = 0;
    bit_idx = 20 + 11;

    // Messages batch(es)
    do {
        if (c == 0) codewords.push_back(POCSAG_SYNCWORD);

        for (; c < 16; c++) {
            // Fill up 20 bits
            if (type == MessageType::ALPHANUMERIC) {
                if ((char_idx < message_size) || (ascii_char)) {
                    do {
                        bit_idx -= 7;

                        if (char_idx < message_size)
                            ascii_char = message[char_idx] & 0x7F;
                        else
                            ascii_char = 0;  // Codeword padding

                        // Bottom's up
                        ascii_char = (ascii_char & 0xF0) >> 4 | (ascii_char & 0x0F) << 4;  // *6543210 -> 3210*654
                        ascii_char = (ascii_char & 0xCC) >> 2 | (ascii_char & 0x33) << 2;  // 3210*654 -> 103254*6
                        ascii_char = (ascii_char & 0xAA) >> 2 | (ascii_char & 0x55);       // 103254*6 -> *0123456

                        codeword |= (ascii_char << bit_idx);

                        char_idx++;

                    } while (bit_idx > 11);

                    codeword &= 0x7FFFF800;  // Trim data
                    codeword |= 0x80000000;  // Message type
                    insert_BCH(BCH_code, &codeword);

                    codewords.push_back(codeword);

                    if (bit_idx != 11) {
                        bit_idx = 20 + bit_idx;
                        codeword = ascii_char << bit_idx;
                    } else {
                        bit_idx = 20 + 11;
                        codeword = 0;
                    }
                } else {
                    codewords.push_back(POCSAG_IDLEWORD);  // Batch padding
                }
            } else if (type == MessageType::NUMERIC_ONLY) {
                if (char_idx < message_size) {
                    do {
                        bit_idx -= 4;

                        if (char_idx < message_size)
                            digit_code = get_digit_code(message[char_idx]);
                        else
                            digit_code = 3;  // Space (codeword padding)

                        codeword |= (digit_code << bit_idx);

                        char_idx++;

                    } while (bit_idx > 11);

                    codeword |= 0x80000000;  // Message type
                    insert_BCH(BCH_code, &codeword);

                    codewords.push_back(codeword);

                    bit_idx = 20 + 11;
                    codeword = 0;
                } else {
                    codewords.push_back(POCSAG_IDLEWORD);  // Batch padding
                }
            }
        }

        c = 0;

    } while (char_idx < message_size);
}

// ----------------------------------------------------------------------------
// EccContainer
// ----------------------------------------------------------------------------
EccContainer::EccContainer() {
    setup_ecc();
}

void EccContainer::setup_ecc() {
    unsigned int srr = 0x3b4;
    unsigned int i, n, j, k;

    /* calculate all information needed to implement error correction */
    // Note : this is only for 31,21 code used in pocsag & flex
    //        one should probably also make use of 32nd parity bit
    for (i = 0; i <= 20; i++) {
        ecs[i] = srr;
        if ((srr & 0x01) != 0)
            srr = (srr >> 1) ^ 0x3B4;
        else
            srr = srr >> 1;
    }

    /* bch holds a syndrome look-up table telling which bits to correct */
    // first 5 bits hold location of first error; next 5 bits hold location
    // of second error; bits 12 & 13 tell how many bits are bad
    for (i = 0; i < 1024; i++) bch[i] = 0;

    /* two errors in data */
    for (n = 0; n <= 20; n++) {
        for (i = 0; i <= 20; i++) {
            j = (i << 5) + n;
            k = ecs[n] ^ ecs[i];
            bch[k] = j + 0x2000;
        }
    }

    /* one error in data */
    for (n = 0; n <= 20; n++) {
        k = ecs[n];
        j = n + (0x1f << 5);
        bch[k] = j + 0x1000;
    }

    /* one error in data and one error in ecc portion */
    for (n = 0; n <= 20; n++) {
        for (i = 0; i < 10; i++) /* ecc screwed up bit */
        {
            k = ecs[n] ^ (1 << i);
            j = n + (0x1f << 5);
            bch[k] = j + 0x2000;
        }
    }

    /* one error in ecc */
    for (n = 0; n < 10; n++) {
        k = 1 << n;
        bch[k] = 0x3ff + 0x1000;
    }

    /* two errors in ecc */
    for (n = 0; n < 10; n++) {
        for (i = 0; i < 10; i++) {
            if (i != n) {
                k = (1 << n) ^ (1 << i);
                bch[k] = 0x3ff + 0x2000;
            }
        }
    }
}

int EccContainer::error_correct(uint32_t& val) {
    int i, synd, errl, acc, pari, ecc, b1, b2;

    errl = 0;
    pari = 0;

    ecc = 0;
    for (i = 31; i >= 11; --i) {
        if (val & (1 << i)) {
            ecc = ecc ^ ecs[31 - i];
            pari = pari ^ 0x01;
        }
    }

    acc = 0;
    for (i = 10; i >= 1; --i) {
        acc = acc << 1;
        if (val & (1 << i)) {
            acc = acc ^ 0x01;
        }
    }

    synd = ecc ^ acc;

    errl = 0;

    if (synd != 0) /* if nonzero syndrome we have error */
    {
        if (bch[synd] != 0) /* check for correctable error */
        {
            b1 = bch[synd] & 0x1f;
            b2 = bch[synd] >> 5;
            b2 = b2 & 0x1f;

            if (b2 != 0x1f) {
                val ^= 0x01 << (31 - b2);
                ecc = ecc ^ ecs[b2];
            }

            if (b1 != 0x1f) {
                val ^= 0x01 << (31 - b1);
                ecc = ecc ^ ecs[b1];
            }

            errl = bch[synd] >> 12;
        } else {
            errl = 3;
        }

        if (errl == 1) pari = pari ^ 0x01;
    }

    if (errl == 4) errl = 3;

    return errl;
}

// ----------------------------------------------------------------------------
// Numeric character table: 4-bit BCD -> ASCII
// ----------------------------------------------------------------------------
static const char numeric_chars[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'R', 'U', ' ', '-', ']', '['};

// Extract and bit-reverse a 4-bit nibble from a message codeword.
// POCSAG numeric digits are transmitted LSB first, so bit 30 (first transmitted)
// is the LSB of the digit value, not the MSB.
static uint8_t decode_nibble(uint32_t codeword, int nibble_idx) {
    int bit_pos = 30 - nibble_idx * 4;
    // bit_pos is the first transmitted bit (LSB of digit)
    // bit_pos-3 is the last transmitted bit (MSB of digit)
    uint8_t n = 0;
    n |= ((codeword >> (bit_pos - 3)) & 1) << 3;  // MSB of digit
    n |= ((codeword >> (bit_pos - 2)) & 1) << 2;
    n |= ((codeword >> (bit_pos - 1)) & 1) << 1;
    n |= ((codeword >> (bit_pos - 0)) & 1) << 0;  // LSB of digit
    return n;
}

// ----------------------------------------------------------------------------
// Heuristic message type detection (first batch only)
// ----------------------------------------------------------------------------

// Count trailing fill characters in alpha buffer (NULL or space).
static int count_alpha_fill(const std::string& data) {
    if (data.empty()) return 0;
    int fill = 0;
    for (int i = data.size() - 1; i >= 0; --i) {
        char c = data[i];
        if (c == '\0' || c == ' ')
            fill++;
        else
            break;
    }
    return fill;
}

// Count trailing fill nibbles (0xC = space) in numeric buffer.
static int count_numeric_fill(const uint8_t* nibbles, int count) {
    int fill = 0;
    for (int i = count - 1; i >= 0; --i) {
        if (nibbles[i] == 0x0C)
            fill++;
        else
            break;
    }
    return fill;
}

// Score alpha interpretation: +3 alphanumeric/space, -2 other printable, -5 control.
static int score_alpha(const std::string& data, int fill) {
    int score = 0;
    int content = 0;
    int len = data.size();

    for (int i = 0; i < len; ++i) {
        unsigned char c = data[i];

        // Skip trailing fill
        if (i >= len - fill && (c == 0 || c == ' '))
            continue;
        if (c == 0)
            continue;

        content++;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == ' ')
            score += 3;
        else if (c >= 0x20 && c <= 0x7E)
            score -= 2;
        else if (c == '\n' || c == '\r' || c == '\t' || c == 0x04)
            score += 0;
        else
            score -= 5;
    }

    if (content > 0)
        score += fill * fill * 3 + fill * 5;

    return score;
}

// Score numeric interpretation on raw nibbles.
static int score_numeric(const uint8_t* nibbles, int count, int fill) {
    int raw_score = 0;
    int scored = 0;
    int digits = 0;
    int u_count = 0;

    // Pre-scan for U nibbles
    for (int i = 0; i < count; ++i) {
        if (nibbles[i] == 0x0C && i >= count - fill)
            continue;
        if (nibbles[i] == 0x0B)
            u_count++;
    }

    bool urgent_prefix = (u_count == 1);

    for (int i = 0; i < count; ++i) {
        uint8_t n = nibbles[i];
        if (n == 0x0C && i >= count - fill)
            continue;

        scored++;
        if (n <= 0x09) {
            raw_score += 3;
            digits++;
        } else if (n == 0x0B) {
            raw_score += urgent_prefix ? -1 : -15;
        } else if (n == 0x0A) {
            raw_score -= 5;
        } else {
            raw_score -= 2;
        }
    }

    int score = scored > 0 ? raw_score * 4 / 7 : 0;

    // Phone numbers have at most ~15 digits
    if (digits > 15)
        score -= (digits - 15) * 5;

    // Fill bonus (weaker than alpha: 1/16 vs 1/128 coincidence rate)
    score += fill * fill;

    return score;
}

DetectedType detect_message_type(const std::string& alpha,
                                 const uint8_t* nibbles,
                                 uint8_t nibble_count,
                                 uint8_t msg_codewords) {
    if (alpha.empty() && nibble_count == 0)
        return DET_TONE;

    // Long messages can't be numeric (phone numbers are short)
    if (msg_codewords >= 8)
        return DET_ALPHA;

    int alpha_fill = count_alpha_fill(alpha);
    int numeric_fill = count_numeric_fill(nibbles, nibble_count);

    int sa = score_alpha(alpha, alpha_fill) + 2;  // Alpha prior bias
    int sn = score_numeric(nibbles, nibble_count, numeric_fill);

    // Short message boost for alpha (1-2 data codewords)
    if (msg_codewords <= 3)
        sa += 3;

    return (sn > sa) ? DET_NUMERIC : DET_ALPHA;
}

// ----------------------------------------------------------------------------
// Batch decoder
// ----------------------------------------------------------------------------

bool pocsag_decode_batch(const POCSAGPacket& batch, POCSAGState& state) {
    constexpr uint8_t codeword_max = 16;
    state.output.clear();

    /* Only reset numeric accumulator when starting a new message,
     * not on continuation batches — numeric_buf must be cumulative
     * across multi-batch messages. */
    const bool continuing_numeric =
        (state.mode != STATE_HAVE_ADDRESS) &&
        (state.out_type == MESSAGE) &&
        state.type_decided &&
        (state.detected == DET_NUMERIC);
    if (!continuing_numeric)
        state.numeric_len = 0;

    /* Preserve new_message across batch boundary when STATE_HAVE_ADDRESS
     * persists — the address was at the end of the previous batch and
     * we haven't displayed it yet. Otherwise reset for this batch. */
    if (state.mode != STATE_HAVE_ADDRESS)
        state.new_message = false;

    // Temporary nibble buffer for first-batch numeric decode.
    uint8_t nibbles[max_batch_nibbles];
    uint8_t nibble_count = 0;
    uint8_t msg_codewords = 0;
    // Also build raw alpha for heuristic (before non-printable replacement).
    std::string raw_alpha{};
    // Track whether any characters came from uncorrectable codewords.
    // If so, heuristic scoring is unreliable — skip it and default to alpha.
    bool has_bad_chars = false;

    while (state.codeword_index < codeword_max) {
        auto codeword = batch[state.codeword_index];
        bool is_address = (codeword & 0x80000000U) == 0;

        // Single ECC call: fix errors and get error count.
        auto error_count = state.ecc->error_correct(codeword);

        switch (state.mode) {
            case STATE_CLEAR:
                if (is_address && codeword != POCSAG_IDLEWORD) {
                    state.function = (codeword >> 11) & 3;
                    state.address = (codeword >> 10) & 0x1FFFF8U;
                    /* Frame number = lower 3 bits of RIC, derived from the
                     * address codeword's position in the batch (not the
                     * message codeword's position). codeword_index 0-15
                     * maps to frames 0-7 via index >> 1. */
                    state.address |= (state.codeword_index >> 1);
                    state.mode = STATE_HAVE_ADDRESS;
                    state.out_type = ADDRESS;
                    state.errors = error_count;
                    state.new_message = true;
                    state.type_decided = false;
                    state.detected = DET_UNKNOWN;

                    state.ascii_idx = 0;
                    state.ascii_data = 0;
                    state.prev_cw_err = 0;
                    state.cur_cw_err = 0;
                    nibble_count = 0;
                    msg_codewords = 0;
                    raw_alpha.clear();
                } else if (codeword == POCSAG_IDLEWORD) {
                    state.out_type = IDLE;
                }
                break;

            case STATE_HAVE_ADDRESS:
                if (is_address) {
                    // Got another address. Run heuristic before returning if we have pending data.
                    if (!state.type_decided && msg_codewords > 0) {
                        state.detected = has_bad_chars
                                             ? DET_ALPHA
                                             : detect_message_type(raw_alpha, nibbles, nibble_count, msg_codewords);
                        state.type_decided = true;
                        state.msg_codewords = msg_codewords;
                        if (state.detected == DET_NUMERIC) {
                            state.numeric_len = 0;
                            for (uint8_t ni = 0; ni < nibble_count && state.numeric_len < sizeof(state.numeric_buf); ++ni)
                                state.numeric_buf[state.numeric_len++] = numeric_chars[nibbles[ni] & 0x0F];
                        }
                    }
                    state.mode = STATE_CLEAR;
                    return true;
                }

                /* Frame number already applied in STATE_CLEAR.
                 * Transition to message decoding. */
                state.mode = STATE_GETTING_MSG;
                [[fallthrough]];

            case STATE_GETTING_MSG:
                if (is_address) {
                    // Message ended. Run heuristic before returning.
                    if (!state.type_decided && msg_codewords > 0) {
                        state.detected = has_bad_chars
                                             ? DET_ALPHA
                                             : detect_message_type(raw_alpha, nibbles, nibble_count, msg_codewords);
                        state.type_decided = true;
                        state.msg_codewords = msg_codewords;
                        if (state.detected == DET_NUMERIC) {
                            state.numeric_len = 0;
                            for (uint8_t ni = 0; ni < nibble_count && state.numeric_len < sizeof(state.numeric_buf); ++ni)
                                state.numeric_buf[state.numeric_len++] = numeric_chars[nibbles[ni] & 0x0F];
                        }
                    }
                    state.mode = STATE_CLEAR;
                    return true;
                }

                state.out_type = MESSAGE;
                state.errors += error_count;
                msg_codewords++;

                // Track per-codeword error level for character coloring.
                // 0=clean, 1-2=corrected, 3=uncorrectable.
                state.prev_cw_err = state.cur_cw_err;
                state.cur_cw_err = (error_count >= 3) ? 3 : error_count;

                // --- Alpha decode (always) ---
                // Bits remaining from previous codeword inherit prev_cw_err.
                // New 20 bits from this codeword use cur_cw_err.
                // Characters spanning boundary get the worst of both.
                uint32_t bits_from_prev = state.ascii_idx;  // leftover bits before adding new ones

                state.ascii_data |= ((uint64_t)((codeword >> 11) & 0xFFFFF)) << (44 - state.ascii_idx);
                state.ascii_idx += 20;

                while (state.ascii_idx >= 7) {
                    // Per-character error level from codeword error tracking.
                    // Characters spanning a codeword boundary get the worst level.
                    uint8_t char_err;
                    if (bits_from_prev >= 7) {
                        // Entire character from previous codeword's leftover bits.
                        char_err = state.prev_cw_err;
                        bits_from_prev -= 7;
                    } else if (bits_from_prev > 0) {
                        // Character spans boundary.
                        char_err = std::max(state.prev_cw_err, state.cur_cw_err);
                        bits_from_prev = 0;
                    } else {
                        // Entirely from current codeword.
                        char_err = state.cur_cw_err;
                    }

                    // Extract top 7 bits from accumulator
                    char ascii_char = (state.ascii_data >> 57) & 0x7F;
                    state.ascii_data <<= 7;
                    state.ascii_idx -= 7;

                    // Reverse bits (LSB-first encoding)
                    ascii_char = (ascii_char & 0xF0) >> 4 | (ascii_char & 0x0F) << 4;
                    ascii_char = (ascii_char & 0xCC) >> 2 | (ascii_char & 0x33) << 2;
                    ascii_char = (ascii_char & 0xAA) >> 2 | (ascii_char & 0x55);

                    // Store raw char for heuristic
                    if (!state.type_decided)
                        raw_alpha += ascii_char;

                    // Substitute '?' for characters from uncorrectable codewords
                    if (char_err >= 3) {
                        state.output += "?";
                        has_bad_chars = true;
                    } else if (ascii_char < 32 || ascii_char > 126)
                        state.output += ".";
                    else
                        state.output += ascii_char;
                }

                // --- Numeric decode ---
                // First batch: accumulate nibbles locally for heuristic scoring.
                // Continuation batches: if already decided numeric, decode directly
                // into numeric_buf for the app layer.
                if (!state.type_decided && nibble_count + 5 <= max_batch_nibbles) {
                    for (int n = 0; n < 5; ++n) {
                        nibbles[nibble_count++] = decode_nibble(codeword, n);
                    }
                } else if (state.type_decided && state.detected == DET_NUMERIC &&
                           state.numeric_len + 5 <= (uint8_t)sizeof(state.numeric_buf)) {
                    for (int n = 0; n < 5; ++n) {
                        uint8_t nib = decode_nibble(codeword, n);
                        state.numeric_buf[state.numeric_len++] = numeric_chars[nib & 0x0F];
                    }
                }

                break;
        }

        state.codeword_index++;
    }

    // End of batch. If we have message data and type not yet decided, run heuristic.
    if (state.out_type == MESSAGE && !state.type_decided && msg_codewords > 0) {
        state.detected = has_bad_chars
                             ? DET_ALPHA
                             : detect_message_type(raw_alpha, nibbles, nibble_count, msg_codewords);
        state.type_decided = true;
        state.msg_codewords = msg_codewords;
        if (state.detected == DET_NUMERIC) {
            state.numeric_len = 0;
            for (uint8_t ni = 0; ni < nibble_count && state.numeric_len < sizeof(state.numeric_buf); ++ni)
                state.numeric_buf[state.numeric_len++] = numeric_chars[nibbles[ni] & 0x0F];
        }
    }

    return false;
}

} /* namespace pocsag */
