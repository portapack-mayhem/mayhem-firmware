
#ifndef __FPROTO_DOITRAND_H__
#define __FPROTO_DOITRAND_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    DoitrandDecoderStepReset = 0,
    DoitrandDecoderStepFoundStartBit,
    DoitrandDecoderStepSaveDuration,
    DoitrandDecoderStepCheckDuration,
} DoitrandDecoderStep;

class FProtoSubGhzDDoitrand : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDDoitrand() {
        sensorType = FPS_DOITRAND;
        te_short = 400;
        te_long = 1100;
        te_delta = 150;
        min_count_bit_for_found = 37;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case DoitrandDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 62) < te_delta * 30)) {
                    // Found Preambula
                    parser_step = DoitrandDecoderStepFoundStartBit;
                }
                break;
            case DoitrandDecoderStepFoundStartBit:
                if (level && ((DURATION_DIFF(duration, (te_short * 2)) < te_delta * 3))) {
                    // Found start bit
                    parser_step = DoitrandDecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                } else {
                    parser_step = DoitrandDecoderStepReset;
                }
                break;
            case DoitrandDecoderStepSaveDuration:
                if (!level) {
                    if (duration >= ((uint32_t)te_short * 10 + te_delta)) {
                        parser_step = DoitrandDecoderStepFoundStartBit;
                        if (decode_count_bit == min_count_bit_for_found) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            if (callback) callback(this);
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        break;
                    } else {
                        te_last = duration;
                        parser_step = DoitrandDecoderStepCheckDuration;
                    }
                }
                break;
            case DoitrandDecoderStepCheckDuration:
                if (level) {
                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta * 3)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = DoitrandDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta * 3) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = DoitrandDecoderStepSaveDuration;
                    } else {
                        parser_step = DoitrandDecoderStepReset;
                    }
                } else {
                    parser_step = DoitrandDecoderStepReset;
                }
                break;
        }
    }
};

#endif
