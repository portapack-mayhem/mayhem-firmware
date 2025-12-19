#pragma once
#include "subcarbase.hpp"
#include <cstring>

typedef enum {
    KiaV1DecoderStepReset = 0,
    KiaV1DecoderStepCheckPreamble,
    KiaV1DecoderStepFoundShortLow,
    KiaV1DecoderStepCollectRawBits,
} KiaV1DecoderStep;

class FProtoSubCarKiaV1 : public FProtoSubCarBase {
   public:
    FProtoSubCarKiaV1() {
        sensorType = FPC_KIAV1;
        te_short = 800;
        te_long = 1600;
        te_delta = 200;
        min_count_bit_for_found = 56;
    }
    void kia_v1_add_raw_bit(bool bit) {
        if (raw_bit_count < 192) {
            uint16_t byte_idx = raw_bit_count / 8;
            uint8_t bit_idx = 7 - (raw_bit_count % 8);
            if (bit) {
                raw_bits[byte_idx] |= (1 << bit_idx);
            } else {
                raw_bits[byte_idx] &= ~(1 << bit_idx);
            }
            raw_bit_count++;
        }
    }
    inline bool kia_v1_get_raw_bit(uint16_t idx) {
        uint16_t byte_idx = idx / 8;
        uint8_t bit_idx = 7 - (idx % 8);
        return (raw_bits[byte_idx] >> bit_idx) & 1;
    }
    bool kia_v1_manchester_decode() {
        if (raw_bit_count < 113) {
            // FURI_LOG_D(TAG, "Not enough raw bits: %u", raw_bit_count);
            return false;
        }
        // Try different offsets to find best alignment (RTL-433 uses -1 bit offset)
        uint16_t best_bits = 0;
        uint64_t best_data = 0;
        // uint16_t best_offset = 0;

        for (uint16_t offset = 0; offset < 8; offset++) {
            uint64_t data = 0;
            uint16_t decoded_bits = 0;

            for (uint16_t i = offset; i + 1 < raw_bit_count && decoded_bits < 56; i += 2) {
                bool bit1 = kia_v1_get_raw_bit(i);
                bool bit2 = kia_v1_get_raw_bit(i + 1);

                uint8_t two_bits = (bit1 << 1) | bit2;

                // V1 uses: 10=1, 01=0
                if (two_bits == 0x02) {  // 10 = decoded 1
                    data = (data << 1) | 1;
                    decoded_bits++;
                } else if (two_bits == 0x01) {  // 01 = decoded 0
                    data = (data << 1);
                    decoded_bits++;
                } else {
                    break;
                }
            }

            if (decoded_bits > best_bits) {
                best_bits = decoded_bits;
                best_data = data;
                // best_offset = offset;
            }
        }

        // FURI_LOG_I(TAG, "Best: offset=%u bits=%u data=%014llX", best_offset, best_bits, best_data);

        decode_data = best_data;
        decode_count_bit = best_bits;

        return best_bits >= min_count_bit_for_found;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case KiaV1DecoderStepReset:
                // Preamble 0xCCCCCCCD produces alternating LONG pulses
                if ((level) && (DURATION_DIFF(duration, te_long) <
                                te_delta)) {
                    parser_step = KiaV1DecoderStepCheckPreamble;
                    te_last = duration;
                    header_count = 1;
                }
                break;

            case KiaV1DecoderStepCheckPreamble:
                if (level) {
                    if (DURATION_DIFF(duration, te_long) <
                        te_delta) {
                        te_last = duration;
                        header_count++;
                    } else if (
                        DURATION_DIFF(duration, te_short) <
                        te_delta) {
                        te_last = duration;
                    } else {
                        parser_step = KiaV1DecoderStepReset;
                    }
                } else {
                    // LOW pulse
                    if (DURATION_DIFF(duration, te_long) <
                        te_delta) {
                        header_count++;
                    } else if (
                        DURATION_DIFF(duration, te_short) <
                        te_delta) {
                        // Short LOW - this is the start of sync (0xCD ends: ...long H, short L, short H)
                        if (header_count > 12) {
                            parser_step = KiaV1DecoderStepFoundShortLow;
                        }
                    } else {
                        parser_step = KiaV1DecoderStepReset;
                    }
                }
                break;

            case KiaV1DecoderStepFoundShortLow:
                // Expecting SHORT HIGH to complete sync
                if (level && (DURATION_DIFF(duration, te_short) <
                              te_delta)) {
                    // FURI_LOG_I(TAG, "Sync! hdr=%u", header_count);
                    parser_step = KiaV1DecoderStepCollectRawBits;
                    raw_bit_count = 0;
                    memset(raw_bits, 0, sizeof(raw_bits));
                    // Add the sync short HIGH as first raw bit
                    kia_v1_add_raw_bit(true);
                } else {
                    parser_step = KiaV1DecoderStepReset;
                }
                break;

            case KiaV1DecoderStepCollectRawBits:
                if (duration > 2400) {
                    // FURI_LOG_I(TAG, "End! raw_bits=%u", raw_bit_count);

                    if (kia_v1_manchester_decode()) {
                        // instance->generic.data = decode_data;
                        data_count_bit = decode_count_bit;

                        // Extract fields from 56-bit data per RTL-433:
                        // Serial: bits 55-24 (32 bits)
                        // Btn: bits 23-16 (8 bits)
                        // Count: bits 15-8 (8 bits)
                        // CRC: bits 7-0 (8 bits)
                        // instance->generic.serial = (uint32_t)((instance->generic.data >> 24) & 0xFFFFFFFF);
                        // instance->generic.btn = (uint8_t)((instance->generic.data >> 16) & 0xFF);
                        // instance->generic.cnt = (uint8_t)((instance->generic.data >> 8) & 0xFF);

                        if (callback)
                            callback(this);
                    }

                    parser_step = KiaV1DecoderStepReset;
                    break;
                }

                int num_bits = 0;
                if (DURATION_DIFF(duration, te_short) <
                    te_delta) {
                    num_bits = 1;
                } else if (
                    DURATION_DIFF(duration, te_long) <
                    te_delta) {
                    num_bits = 2;
                } else {
                    parser_step = KiaV1DecoderStepReset;
                    break;
                }

                for (int i = 0; i < num_bits; i++) {
                    kia_v1_add_raw_bit(level);
                }

                break;
        }
    }

    uint8_t raw_bits[24]{0};
    uint16_t raw_bit_count = 0;
    uint16_t header_count = 0;
};
