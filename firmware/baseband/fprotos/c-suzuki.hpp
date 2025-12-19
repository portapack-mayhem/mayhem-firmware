#pragma once
#include "subcarbase.hpp"

#define SUZUKI_GAP_TIME 2000
#define SUZUKI_GAP_DELTA 400

typedef enum {
    SuzukiDecoderStepReset = 0,
    SuzukiDecoderStepFoundStartPulse,
    SuzukiDecoderStepSaveDuration,
} SuzukiDecoderStep;

class FProtoSubCarSuzuki : public FProtoSubCarBase {
   public:
    FProtoSubCarSuzuki() {
        sensorType = FPC_SUZUKI;
        te_short = 250;
        te_long = 500;
        te_delta = 110;
        min_count_bit_for_found = 64;
    }
    void suzuki_add_bit(uint32_t bit) {
        uint32_t carry = data_low >> 31;
        data_low = (data_low << 1) | bit;
        data_high = (data_high << 1) | carry;
        data_count_bit++;
    }
    void subghz_protocol_decoder_suzuki_reset() {
        parser_step = SuzukiDecoderStepReset;
        header_count = 0;
        data_count_bit = 0;
        data_low = 0;
        data_high = 0;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case SuzukiDecoderStepReset:
                // Wait for short HIGH pulse (~250µs) to start preamble
                if (!level)
                    return;

                if (DURATION_DIFF(duration, te_short) > te_delta) {
                    return;
                }

                data_low = 0;
                data_high = 0;
                parser_step = SuzukiDecoderStepFoundStartPulse;
                header_count = 0;
                data_count_bit = 0;
                break;

            case SuzukiDecoderStepFoundStartPulse:
                if (level) {
                    // HIGH pulse
                    if (header_count < 257) {
                        // Still in preamble - just count
                        return;
                    }

                    // After preamble, look for long HIGH to start data
                    if (DURATION_DIFF(duration, te_long) < te_delta) {
                        parser_step = SuzukiDecoderStepSaveDuration;
                        suzuki_add_bit(1);
                    }
                    // Ignore short HIGHs after preamble until we see a long one
                } else {
                    // LOW pulse - count as header if short
                    if (DURATION_DIFF(duration, te_short) < te_delta) {
                        te_last = duration;
                        header_count++;
                    } else {
                        parser_step = SuzukiDecoderStepReset;
                    }
                }
                break;

            case SuzukiDecoderStepSaveDuration:
                if (level) {
                    // HIGH pulse - determines bit value
                    // Long HIGH (~500µs) = 1, Short HIGH (~250µs) = 0
                    if (DURATION_DIFF(duration, te_long) < te_delta) {
                        suzuki_add_bit(1);
                    } else if (DURATION_DIFF(duration, te_short) < te_delta) {
                        suzuki_add_bit(0);
                    } else {
                        parser_step = SuzukiDecoderStepReset;
                    }
                    // Stay in this state for next bit
                } else {
                    // LOW pulse - check for gap (end of transmission)
                    if (DURATION_DIFF(duration, SUZUKI_GAP_TIME) < SUZUKI_GAP_DELTA) {
                        // Gap found - end of transmission
                        if (data_count_bit == 64) {
                            data_count_bit = 64;
                            decode_data = ((uint64_t)data_high << 32) | (uint64_t)data_low;
                            // Check manufacturer nibble (should be 0xF)
                            uint8_t manufacturer = (data_high >> 28) & 0xF;
                            if (manufacturer == 0xF) {
                                // Extract fields
                                decode_data2 = 0;  // Not used
                                if (callback) {
                                    callback(this);
                                }
                            }
                        }
                        parser_step = SuzukiDecoderStepReset;
                    }
                    // Short LOW pulses are ignored - stay in this state
                }
                break;
        }
    }

    uint16_t header_count = 0;
    uint32_t data_high = 0;
    uint32_t data_low = 0;
    uint8_t data_count_bit = 0;
};
