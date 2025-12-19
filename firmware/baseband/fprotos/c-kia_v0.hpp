#pragma once
#include "subcarbase.hpp"
#include <cstring>

typedef enum {
    KIADecoderStepReset = 0,
    KIADecoderStepCheckPreambula,
    KIADecoderStepSaveDuration,
    KIADecoderStepCheckDuration,
} KIADecoderStep;

class FProtoSubCarKiaV0 : public FProtoSubCarBase {
   public:
    FProtoSubCarKiaV0() {
        sensorType = FPC_KIAV0;
        te_short = 250;
        te_long = 500;
        te_delta = 100;
        min_count_bit_for_found = 61;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case KIADecoderStepReset:
                if ((level) && (DURATION_DIFF(duration, te_short) < te_delta)) {
                    parser_step = KIADecoderStepCheckPreambula;
                    te_last = duration;
                    header_count = 0;
                }
                break;
            case KIADecoderStepCheckPreambula:
                if (level) {
                    if ((DURATION_DIFF(duration, te_short) < te_delta) ||
                        (DURATION_DIFF(duration, te_long) < te_delta)) {
                        te_last = duration;
                    } else {
                        parser_step = KIADecoderStepReset;
                    }
                } else if (
                    (DURATION_DIFF(duration, te_short) < te_delta) &&
                    (DURATION_DIFF(te_last, te_short) < te_delta)) {
                    header_count++;
                    break;
                } else if (
                    (DURATION_DIFF(duration, te_long) < te_delta) &&
                    (DURATION_DIFF(te_last, te_long) < te_delta)) {
                    if (header_count > 15) {
                        parser_step = KIADecoderStepSaveDuration;
                        decode_data = 0;
                        decode_count_bit = 1;
                        subghz_protocol_blocks_add_bit(1);
                        // FURI_LOG_I(TAG, "Starting data decode after %u header pulses", header_count);
                    } else {
                        parser_step = KIADecoderStepReset;
                    }
                } else {
                    parser_step = KIADecoderStepReset;
                }
                break;
            case KIADecoderStepSaveDuration:
                if (level) {
                    if (duration >=
                        (te_long + te_delta * 2UL)) {
                        // Signal ended too early!
                        // FURI_LOG_W(TAG, "Signal ended at %u bits (expected 61). Duration: %lu", decode_count_bit, duration);

                        parser_step = KIADecoderStepReset;
                        if (decode_count_bit == min_count_bit_for_found) {
                            // instance->generic.data = decode_data;
                            data_count_bit = decode_count_bit;
                            if (callback)
                                callback(this);
                        } else {
                            // FURI_LOG_E(TAG, "Incomplete signal: only %u bits", decode_count_bit);
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        break;
                    } else {
                        te_last = duration;
                        parser_step = KIADecoderStepCheckDuration;
                    }
                } else {
                    parser_step = KIADecoderStepReset;
                }
                break;
            case KIADecoderStepCheckDuration:
                if (!level) {
                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(0);
                        if (decode_count_bit % 10 == 0) {
                            // FURI_LOG_D(TAG, "Decoded %u bits so far", decode_count_bit);
                        }
                        parser_step = KIADecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        if (decode_count_bit % 10 == 0) {
                            // FURI_LOG_D(TAG, "Decoded %u bits so far", decode_count_bit);
                        }
                        parser_step = KIADecoderStepSaveDuration;
                    } else {
                        // FURI_LOG_W(TAG, "Timing mismatch at bit %u. Last: %lu, Current: %lu", decode_count_bit, te_last, duration);
                        parser_step = KIADecoderStepReset;
                    }
                } else {
                    parser_step = KIADecoderStepReset;
                }
                break;
        }
    }

    bool is_running = false;
    size_t preamble_count = 0;
    size_t data_bit_index = 0;
    uint8_t last_bit = 0;
    bool send_high = false;
    uint16_t header_count = 0;
};
