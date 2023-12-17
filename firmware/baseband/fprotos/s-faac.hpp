
#ifndef __FPROTO_FAAC_H__
#define __FPROTO_FAAC_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    FaacSLHDecoderStepReset = 0,
    FaacSLHDecoderStepFoundPreambula,
    FaacSLHDecoderStepSaveDuration,
    FaacSLHDecoderStepCheckDuration,
} FaacSLHDecoderStep;

class FProtoSubGhzDFaac : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDFaac() {
        sensorType = FPS_FAAC;
        te_short = 255;
        te_long = 595;
        te_delta = 100;
        min_count_bit_for_found = 64;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case FaacSLHDecoderStepReset:
                if ((level) && (DURATION_DIFF(duration, te_long * 2) < te_delta * 3)) {
                    parser_step = FaacSLHDecoderStepFoundPreambula;
                }
                break;
            case FaacSLHDecoderStepFoundPreambula:
                if ((!level) && (DURATION_DIFF(duration, te_long * 2) < te_delta * 3)) {
                    // Found Preambula
                    parser_step = FaacSLHDecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                } else {
                    parser_step = FaacSLHDecoderStepReset;
                }
                break;
            case FaacSLHDecoderStepSaveDuration:
                if (level) {
                    if (duration >= ((uint32_t)te_short * 3 + te_delta)) {
                        parser_step = FaacSLHDecoderStepFoundPreambula;
                        if (decode_count_bit == min_count_bit_for_found) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            // remark controller skipped
                            if (callback) callback(this);
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        break;
                    } else {
                        te_last = duration;
                        parser_step = FaacSLHDecoderStepCheckDuration;
                    }

                } else {
                    parser_step = FaacSLHDecoderStepReset;
                }
                break;
            case FaacSLHDecoderStepCheckDuration:
                if (!level) {
                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = FaacSLHDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = FaacSLHDecoderStepSaveDuration;
                    } else {
                        parser_step = FaacSLHDecoderStepReset;
                    }
                } else {
                    parser_step = FaacSLHDecoderStepReset;
                }
                break;
        }
    }
};

#endif
