
#ifndef __FPROTO_MASTERCODE_H__
#define __FPROTO_MASTERCODE_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    MastercodeDecoderStepReset = 0,
    MastercodeDecoderStepSaveDuration,
    MastercodeDecoderStepCheckDuration,
} MastercodeDecoderStep;

class FProtoSubGhzDMastercode : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDMastercode() {
        sensorType = FPS_MASTERCODE;
        te_short = 1072;
        te_long = 2145;
        te_delta = 150;
        min_count_bit_for_found = 36;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case MastercodeDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 15) < te_delta * 15)) {
                    parser_step = MastercodeDecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                }
                break;

            case MastercodeDecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = MastercodeDecoderStepCheckDuration;
                } else {
                    parser_step = MastercodeDecoderStepReset;
                }
                break;

            case MastercodeDecoderStepCheckDuration:
                if (!level) {
                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta * 8)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = MastercodeDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta * 8) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = MastercodeDecoderStepSaveDuration;
                    } else if (
                        DURATION_DIFF(duration, te_short * 15) < te_delta * 15) {
                        if ((DURATION_DIFF(te_last, te_short) < te_delta)) {
                            subghz_protocol_blocks_add_bit(0);
                        } else if ((DURATION_DIFF(te_last, te_long) < te_delta * 8)) {
                            subghz_protocol_blocks_add_bit(1);
                        } else {
                            parser_step = MastercodeDecoderStepReset;
                        }

                        if (decode_count_bit == min_count_bit_for_found) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;

                            if (callback) callback(this);
                        }
                        parser_step = MastercodeDecoderStepSaveDuration;
                        decode_data = 0;
                        decode_count_bit = 0;

                    } else {
                        parser_step = MastercodeDecoderStepReset;
                    }
                } else {
                    parser_step = MastercodeDecoderStepReset;
                }
                break;
        }
    }
};

#endif
