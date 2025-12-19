#pragma once
#include "subcarbase.hpp"
#include <cstring>
typedef enum {
    KiaV5DecoderStepReset = 0,
    KiaV5DecoderStepCheckPreamble,
    KiaV5DecoderStepCollectRawBits,
} KiaV5DecoderStep;

class FProtoSubCarKiaV5 : public FProtoSubCarBase {
   public:
    FProtoSubCarKiaV5() {
        sensorType = FPC_KIAV5;
        te_short = 400;
        te_long = 800;
        te_delta = 150;
        min_count_bit_for_found = 64;
    }

    inline bool kia_v5_get_raw_bit(uint16_t idx) {
        uint16_t byte_idx = idx / 8;
        uint8_t bit_idx = 7 - (idx % 8);
        return (raw_bits[byte_idx] >> bit_idx) & 1;
    }

    void kia_v5_add_raw_bit(bool bit) {
        if (raw_bit_count < 256) {
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

    bool kia_v5_manchester_decode() {
        if (raw_bit_count < 130) {
            return false;
        }

        decode_data = 0;
        decode_count_bit = 0;

        // Start at offset 2 for proper Manchester alignment
        const uint16_t start_bit = 2;

        for (uint16_t i = start_bit;
             i + 1 < raw_bit_count && decode_count_bit < 64;
             i += 2) {
            bool bit1 = kia_v5_get_raw_bit(i);
            bool bit2 = kia_v5_get_raw_bit(i + 1);

            uint8_t two_bits = (bit1 << 1) | bit2;

            if (two_bits == 0x01) {  // 01 = decoded 1
                decode_data = (decode_data << 1) | 1;
                decode_count_bit++;
            } else if (two_bits == 0x02) {  // 10 = decoded 0
                decode_data = (decode_data << 1);
                decode_count_bit++;
            } else {
                break;
            }
        }
        return decode_count_bit >= min_count_bit_for_found;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case KiaV5DecoderStepReset:
                if ((level) && (DURATION_DIFF(duration, te_short) <
                                te_delta)) {
                    parser_step = KiaV5DecoderStepCheckPreamble;
                    te_last = duration;
                    header_count = 1;
                }
                break;

            case KiaV5DecoderStepCheckPreamble:
                if (level) {
                    if ((DURATION_DIFF(duration, te_short) <
                         te_delta) ||
                        (DURATION_DIFF(duration, te_long) <
                         te_delta)) {
                        te_last = duration;
                    } else {
                        parser_step = KiaV5DecoderStepReset;
                    }
                } else {
                    if ((DURATION_DIFF(duration, te_short) <
                         te_delta) &&
                        (DURATION_DIFF(te_last, te_short) <
                         te_delta)) {
                        header_count++;
                    } else if (
                        (DURATION_DIFF(duration, te_long) <
                         te_delta) &&
                        (DURATION_DIFF(te_last, te_short) <
                         te_delta)) {
                        if (header_count > 40) {
                            parser_step = KiaV5DecoderStepCollectRawBits;
                            raw_bit_count = 0;
                            memset(raw_bits, 0, sizeof(raw_bits));
                        } else {
                            header_count++;
                        }
                    } else if (
                        DURATION_DIFF(te_last, te_long) <
                        te_delta) {
                        header_count++;
                    } else {
                        parser_step = KiaV5DecoderStepReset;
                    }
                }
                break;

            case KiaV5DecoderStepCollectRawBits:
                if (duration > 1200) {
                    if (kia_v5_manchester_decode()) {
                        // generic.data = decode_data;
                        // generic.data_count_bit = decode_count_bit;
                        data_count_bit = decode_count_bit;
                        // Compute yek (bit-reverse each byte)
                        uint64_t yek = 0;
                        for (int i = 0; i < 8; i++) {
                            uint8_t byte = (decode_data >> (i * 8)) & 0xFF;
                            uint8_t reversed = 0;
                            for (int b = 0; b < 8; b++) {
                                if (byte & (1 << b))
                                    reversed |= (1 << (7 - b));
                            }
                            yek |= ((uint64_t)reversed << ((7 - i) * 8));
                        }
                        decode_data = yek;

                        // Shift serial right by 1 to correct alignment
                        // generic.serial = (uint32_t)(((yek >> 32) & 0x0FFFFFFF) >> 1);
                        // generic.btn = (uint8_t)((yek >> 61) & 0x07);  // Shift btn too
                        // generic.cnt = (uint16_t)(yek & 0xFFFF);

                        if (callback)
                            callback(this);
                    }

                    parser_step = KiaV5DecoderStepReset;
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
                    parser_step = KiaV5DecoderStepReset;
                    break;
                }

                for (int i = 0; i < num_bits; i++) {
                    kia_v5_add_raw_bit(level);
                }

                break;
        }
    }

    uint8_t raw_bits[32]{};
    uint16_t raw_bit_count = 0;
    uint16_t header_count = 0;
};
