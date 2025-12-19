#pragma once
#include "subcarbase.hpp"
#include <cstring>

typedef enum {
    KiaV2DecoderStepReset = 0,
    KiaV2DecoderStepCheckPreamble,
    KiaV2DecoderStepCollectRawBits,
} KiaV2DecoderStep;

class FProtoSubCarKiaV2 : public FProtoSubCarBase {
   public:
    FProtoSubCarKiaV2() {
        sensorType = FPC_KIAV2;
        te_short = 500;
        te_long = 1000;
        te_delta = 160;
        min_count_bit_for_found = 51;
    }

    void kia_v2_add_raw_bit(bool bit) {
        if (raw_bit_count < 160) {
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
    inline bool kia_v2_get_raw_bit(uint16_t idx) {
        uint16_t byte_idx = idx / 8;
        uint8_t bit_idx = 7 - (idx % 8);
        return (raw_bits[byte_idx] >> bit_idx) & 1;
    }
    bool kia_v2_manchester_decode() {
        if (raw_bit_count < 100) {
            return false;
        }

        uint16_t best_bits = 0;
        uint64_t best_data = 0;

        for (uint16_t offset = 0; offset < 8; offset++) {
            uint64_t data = 0;
            uint16_t decoded_bits = 0;

            for (uint16_t i = offset; i + 1 < raw_bit_count && decoded_bits < 53; i += 2) {
                bool bit1 = kia_v2_get_raw_bit(i);
                bool bit2 = kia_v2_get_raw_bit(i + 1);

                uint8_t two_bits = (bit1 << 1) | bit2;

                if (two_bits == 0x02) {
                    data = (data << 1) | 1;
                    decoded_bits++;
                } else if (two_bits == 0x01) {
                    data = (data << 1);
                    decoded_bits++;
                } else {
                    break;
                }
            }

            if (decoded_bits > best_bits) {
                best_bits = decoded_bits;
                best_data = data;
            }
        }

        decode_data = best_data;
        decode_count_bit = best_bits;

        return best_bits >= min_count_bit_for_found;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case KiaV2DecoderStepReset:
                if ((level) && (DURATION_DIFF(duration, te_long) < te_delta)) {
                    parser_step = KiaV2DecoderStepCheckPreamble;
                    te_last = duration;
                    header_count = 1;
                }
                break;

            case KiaV2DecoderStepCheckPreamble:
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
                        parser_step = KiaV2DecoderStepReset;
                    }
                } else {
                    if (DURATION_DIFF(duration, te_long) <
                        te_delta) {
                        header_count++;
                    } else if (
                        DURATION_DIFF(duration, te_short) <
                        te_delta) {
                        if (header_count > 10 &&
                            DURATION_DIFF(te_last, te_short) <
                                te_delta) {
                            parser_step = KiaV2DecoderStepCollectRawBits;
                            raw_bit_count = 0;
                            memset(raw_bits, 0, sizeof(raw_bits));
                        }
                    } else {
                        parser_step = KiaV2DecoderStepReset;
                    }
                }
                break;

            case KiaV2DecoderStepCollectRawBits:
                if (duration > 1500) {
                    if (kia_v2_manchester_decode()) {
                        /*data = decode_data;
                        data_count_bit = decode_count_bit;

                        serial = (uint32_t)((data >> 20) & 0xFFFFFFFF);
                        btn = (uint8_t)((data >> 16) & 0x0F);

                        uint16_t raw_count = (uint16_t)((data >> 4) & 0xFFF);
                        cnt = ((raw_count >> 4) | (raw_count << 8)) & 0xFFF;
                        */
                        data_count_bit = decode_count_bit;
                        if (callback)
                            callback(this);
                    }

                    parser_step = KiaV2DecoderStepReset;
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
                    parser_step = KiaV2DecoderStepReset;
                    break;
                }

                for (int i = 0; i < num_bits; i++) {
                    kia_v2_add_raw_bit(level);
                }

                break;
        }
    }

    uint8_t raw_bits[20]{0};
    uint16_t raw_bit_count = 0;
    uint16_t header_count = 0;
};
