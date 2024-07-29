
#ifndef __FPROTO_IDO_H__
#define __FPROTO_IDO_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    IDoDecoderStepReset = 0,
    IDoDecoderStepFoundPreambula,
    IDoDecoderStepSaveDuration,
    IDoDecoderStepCheckDuration,
} IDoDecoderStep;

class FProtoSubGhzDIdo : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDIdo() {
        sensorType = FPS_IDO;
        te_short = 450;
        te_long = 1450;
        te_delta = 150;
        min_count_bit_for_found = 48;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case IDoDecoderStepReset:
                if ((level) && (DURATION_DIFF(duration, te_short * 10) < te_delta * 5)) {
                    parser_step = IDoDecoderStepFoundPreambula;
                }
                break;
            case IDoDecoderStepFoundPreambula:
                if ((!level) && (DURATION_DIFF(duration, te_short * 10) < te_delta * 5)) {
                    // Found Preambula
                    parser_step = IDoDecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                } else {
                    parser_step = IDoDecoderStepReset;
                }
                break;
            case IDoDecoderStepSaveDuration:
                if (level) {
                    if (duration >= ((uint32_t)te_short * 5 + te_delta)) {
                        parser_step = IDoDecoderStepFoundPreambula;
                        if (decode_count_bit >=
                            min_count_bit_for_found) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            if (callback) callback(this);
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        break;
                    } else {
                        te_last = duration;
                        parser_step = IDoDecoderStepCheckDuration;
                    }

                } else {
                    parser_step = IDoDecoderStepReset;
                }
                break;
            case IDoDecoderStepCheckDuration:
                if (!level) {
                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta * 3)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = IDoDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_short) < te_delta * 3) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = IDoDecoderStepSaveDuration;
                    } else {
                        parser_step = IDoDecoderStepReset;
                    }
                } else {
                    parser_step = IDoDecoderStepReset;
                }
                break;
        }
    }
};

#endif
