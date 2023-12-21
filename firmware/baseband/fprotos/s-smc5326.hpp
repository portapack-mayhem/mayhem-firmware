
#ifndef __FPROTO_SMC5326_H__
#define __FPROTO_SMC5326_H__

#include "subghzdbase.hpp"

typedef enum {
    SMC5326DecoderStepReset = 0,
    SMC5326DecoderStepSaveDuration,
    SMC5326DecoderStepCheckDuration,
} SMC5326DecoderStep;

class FProtoSubGhzDSmc5326 : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDSmc5326() {
        sensorType = FPS_SECPLUSV2;
        te_short = 300;
        te_long = 900;
        te_delta = 200;
        min_count_bit_for_found = 25;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case SMC5326DecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 24) < te_delta * 12)) {
                    // Found Preambula
                    parser_step = SMC5326DecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                }
                break;
            case SMC5326DecoderStepSaveDuration:
                // save duration
                if (level) {
                    te_last = duration;
                    parser_step = SMC5326DecoderStepCheckDuration;
                }
                break;
            case SMC5326DecoderStepCheckDuration:
                if (!level) {
                    if (duration >= ((uint32_t)te_long * 2)) {
                        parser_step = SMC5326DecoderStepSaveDuration;
                        if (decode_count_bit == min_count_bit_for_found) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            if (callback) callback(this);
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        break;
                    }

                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta * 3)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = SMC5326DecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta * 3) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = SMC5326DecoderStepSaveDuration;
                    } else {
                        parser_step = SMC5326DecoderStepReset;
                    }
                } else {
                    parser_step = SMC5326DecoderStepReset;
                }
                break;
        }
    }
};

#endif
