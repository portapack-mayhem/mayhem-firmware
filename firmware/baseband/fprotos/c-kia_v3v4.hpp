#pragma once
#include "subcarbase.hpp"
#include <cstring>

typedef enum {
    KiaV3V4DecoderStepReset = 0,
    KiaV3V4DecoderStepCheckPreamble,
    KiaV3V4DecoderStepCollectRawBits,
} KiaV3V4DecoderStep;

class FProtoSubCarKiaV3V4 : public FProtoSubCarBase {
   public:
    FProtoSubCarKiaV3V4() {
        sensorType = FPC_KIAV3V4;
        te_short = 400;
        te_long = 800;
        te_delta = 150;
        min_count_bit_for_found = 64;
    }
    uint8_t reverse8(uint8_t byte) {
        byte = (byte & 0xF0) >> 4 | (byte & 0x0F) << 4;
        byte = (byte & 0xCC) >> 2 | (byte & 0x33) << 2;
        byte = (byte & 0xAA) >> 1 | (byte & 0x55) << 1;
        return byte;
    }
    void kia_v3_v4_add_raw_bit(bool bit) {
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
    bool kia_v3_v4_process_buffer() {
        if (raw_bit_count < 64) {
            return false;
        }

        uint8_t* b = raw_bits;

        // For V3-style (long LOW sync), data is inverted
        if (is_v3_sync) {
            uint16_t num_bytes = (raw_bit_count + 7) / 8;
            for (uint16_t i = 0; i < num_bytes; i++) {
                b[i] = ~b[i];
            }
        }

        // Extract fields
        // uint32_t encrypted = ((uint32_t)reverse8(b[3]) << 24) | ((uint32_t)reverse8(b[2]) << 16) | ((uint32_t)reverse8(b[1]) << 8) | (uint32_t)reverse8(b[0]);
        uint32_t serial = ((uint32_t)reverse8(b[7] & 0xF0) << 24) | ((uint32_t)reverse8(b[6]) << 16) | ((uint32_t)reverse8(b[5]) << 8) | (uint32_t)reverse8(b[4]);

        uint8_t btn = (reverse8(b[7]) & 0xF0) >> 4;
        decode_data = serial;
        decode_count_bit = 64;
        decode_data2 = btn;
        data_count_bit = decode_count_bit;
        if (callback)
            callback(this);
        // uint8_t our_serial_lsb = serial & 0xFF;

        // Decrypt --skipped, no keeloq decoding
        /* uint32_t decrypted = keeloq_common_decrypt(encrypted, kia_mf_key);
         uint8_t dec_btn = (decrypted >> 28) & 0x0F;
         uint8_t dec_serial_lsb = (decrypted >> 16) & 0xFF;

         // Validate
         if (dec_btn != btn || dec_serial_lsb != our_serial_lsb) {
             return false;
         }

         // Valid decode - version determined by sync type
         instance->encrypted = encrypted;
         instance->decrypted = decrypted;
         instance->generic.serial = serial;
         instance->generic.btn = btn;
         instance->generic.cnt = decrypted & 0xFFFF;
         instance->version = is_v3_sync ? 1 : 0;

         uint64_t key_data = ((uint64_t)b[0] << 56) | ((uint64_t)b[1] << 48) | ((uint64_t)b[2] << 40) |
                             ((uint64_t)b[3] << 32) | ((uint64_t)b[4] << 24) | ((uint64_t)b[5] << 16) |
                             ((uint64_t)b[6] << 8) | (uint64_t)b[7];
         instance->generic.data = key_data;
         instance->generic.data_count_bit = 64;
        */
        return true;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case KiaV3V4DecoderStepReset:
                if (level && DURATION_DIFF(duration, te_short) <
                                 te_delta) {
                    parser_step = KiaV3V4DecoderStepCheckPreamble;
                    te_last = duration;
                    header_count = 1;
                }
                break;

            case KiaV3V4DecoderStepCheckPreamble:
                if (level) {
                    if (DURATION_DIFF(duration, te_short) <
                        te_delta) {
                        te_last = duration;
                    } else if (duration > 1000 && duration < 1500) {
                        // V4 style: Sync is LONG HIGH
                        if (header_count >= 8) {
                            parser_step = KiaV3V4DecoderStepCollectRawBits;
                            raw_bit_count = 0;
                            is_v3_sync = false;
                            memset(raw_bits, 0, sizeof(raw_bits));
                        } else {
                            parser_step = KiaV3V4DecoderStepReset;
                        }
                    } else {
                        parser_step = KiaV3V4DecoderStepReset;
                    }
                } else {
                    if (duration > 1000 && duration < 1500) {
                        // V3 style: Sync is LONG LOW
                        if (header_count >= 8) {
                            parser_step = KiaV3V4DecoderStepCollectRawBits;
                            raw_bit_count = 0;
                            is_v3_sync = true;
                            memset(raw_bits, 0, sizeof(raw_bits));
                        } else {
                            parser_step = KiaV3V4DecoderStepReset;
                        }
                    } else if (
                        DURATION_DIFF(duration, te_short) <
                            te_delta &&
                        DURATION_DIFF(te_last, te_short) <
                            te_delta) {
                        header_count++;
                    } else if (duration > 1500) {
                        parser_step = KiaV3V4DecoderStepReset;
                    }
                }
                break;

            case KiaV3V4DecoderStepCollectRawBits:
                if (level) {
                    if (duration > 1000 && duration < 1500) {
                        // Next sync pulse (V4 style) - end this packet
                        kia_v3_v4_process_buffer();
                        parser_step = KiaV3V4DecoderStepReset;
                    } else if (
                        DURATION_DIFF(duration, te_short) <
                        te_delta) {
                        kia_v3_v4_add_raw_bit(false);
                    } else if (
                        DURATION_DIFF(duration, te_long) <
                        te_delta) {
                        kia_v3_v4_add_raw_bit(true);
                    } else {
                        parser_step = KiaV3V4DecoderStepReset;
                    }
                } else {
                    if (duration > 1000 && duration < 1500) {
                        // Next sync pulse (V3 style) - end this packet
                        kia_v3_v4_process_buffer();
                        parser_step = KiaV3V4DecoderStepReset;
                    } else if (duration > 1500) {
                        // Long gap - end of transmission
                        kia_v3_v4_process_buffer();
                        parser_step = KiaV3V4DecoderStepReset;
                    }
                }
                break;
        }
    }

    bool is_v3_sync = false;  // true = V3 (long LOW sync), false = V4 (long HIGH sync)
    uint8_t version = 0;      // 0 = V4, 1 = V3
    uint8_t raw_bits[32]{0};
    uint16_t raw_bit_count = 0;
    uint16_t header_count = 0;

    // uint32_t encrypted;
    //  uint32_t decrypted;
};
