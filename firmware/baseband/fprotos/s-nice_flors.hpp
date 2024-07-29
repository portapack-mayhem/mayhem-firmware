
#ifndef __FPROTO_NICE_FLORS_H__
#define __FPROTO_NICE_FLORS_H__

#include "subghzdbase.hpp"

#define NICE_ONE_COUNT_BIT 72

typedef enum : uint8_t {
    NiceFlorSDecoderStepReset = 0,
    NiceFlorSDecoderStepCheckHeader,
    NiceFlorSDecoderStepFoundHeader,
    NiceFlorSDecoderStepSaveDuration,
    NiceFlorSDecoderStepCheckDuration,
} NiceFlorSDecoderStep;

class FProtoSubGhzDNiceflors : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDNiceflors() {
        sensorType = FPS_NICEFLORS;
        te_short = 500;
        te_long = 1000;
        te_delta = 300;
        min_count_bit_for_found = 52;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case NiceFlorSDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 38) < te_delta * 38)) {
                    // Found start header Nice Flor-S
                    parser_step = NiceFlorSDecoderStepCheckHeader;
                }
                break;
            case NiceFlorSDecoderStepCheckHeader:
                if ((level) && (DURATION_DIFF(duration, te_short * 3) < te_delta * 3)) {
                    // Found next header Nice Flor-S
                    parser_step = NiceFlorSDecoderStepFoundHeader;
                } else {
                    parser_step = NiceFlorSDecoderStepReset;
                }
                break;
            case NiceFlorSDecoderStepFoundHeader:
                if ((!level) && (DURATION_DIFF(duration, te_short * 3) < te_delta * 3)) {
                    // Found header Nice Flor-S
                    parser_step = NiceFlorSDecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                } else {
                    parser_step = NiceFlorSDecoderStepReset;
                }
                break;
            case NiceFlorSDecoderStepSaveDuration:
                if (level) {
                    if (DURATION_DIFF(duration, te_short * 3) < te_delta) {
                        // Found STOP bit
                        parser_step = NiceFlorSDecoderStepReset;
                        if ((decode_count_bit == min_count_bit_for_found) || (decode_count_bit == NICE_ONE_COUNT_BIT)) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;

                            if (callback) callback(this);
                        }
                        break;
                    } else {
                        // save interval
                        te_last = duration;
                        parser_step = NiceFlorSDecoderStepCheckDuration;
                    }
                }
                break;
            case NiceFlorSDecoderStepCheckDuration:
                if (!level) {
                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = NiceFlorSDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = NiceFlorSDecoderStepSaveDuration;
                    } else
                        parser_step = NiceFlorSDecoderStepReset;
                } else {
                    parser_step = NiceFlorSDecoderStepReset;
                }
                if (decode_count_bit == min_count_bit_for_found) {
                    data = decode_data;
                    decode_data = 0;
                }
                break;
        }
    }
};

#endif
