
#ifndef __FPROTO_HORMANN_H__
#define __FPROTO_HORMANN_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    HormannDecoderStepReset = 0,
    HormannDecoderStepFoundStartHeader,
    HormannDecoderStepFoundHeader,
    HormannDecoderStepFoundStartBit,
    HormannDecoderStepSaveDuration,
    HormannDecoderStepCheckDuration,
} HormannDecoderStep;

#define HORMANN_HSM_PATTERN 0xFF000000003

class FProtoSubGhzDHormann : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDHormann() {
        sensorType = FPS_HORMANN;
        te_short = 500;
        te_long = 1000;
        te_delta = 200;
        min_count_bit_for_found = 44;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case HormannDecoderStepReset:
                if ((level) && (DURATION_DIFF(duration, te_short * 24) < te_delta * 24)) {
                    parser_step = HormannDecoderStepFoundStartBit;
                }
                break;
            case HormannDecoderStepFoundStartBit:
                if ((!level) && (DURATION_DIFF(duration, te_short) < te_delta)) {
                    parser_step = HormannDecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                } else {
                    parser_step = HormannDecoderStepReset;
                }
                break;
            case HormannDecoderStepSaveDuration:
                if (level) {  // save interval
                    if (duration >= (te_short * 5) && (decode_data & HORMANN_HSM_PATTERN) == HORMANN_HSM_PATTERN) {
                        parser_step = HormannDecoderStepFoundStartBit;
                        if (decode_count_bit >=
                            min_count_bit_for_found) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            if (callback) callback(this);
                        }
                        break;
                    }
                    te_last = duration;
                    parser_step = HormannDecoderStepCheckDuration;
                } else {
                    parser_step = HormannDecoderStepReset;
                }
                break;
            case HormannDecoderStepCheckDuration:
                if (!level) {
                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = HormannDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = HormannDecoderStepSaveDuration;
                    } else
                        parser_step = HormannDecoderStepReset;
                } else {
                    parser_step = HormannDecoderStepReset;
                }
                break;
        }
    }
};

#endif
