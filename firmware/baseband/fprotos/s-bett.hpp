
#ifndef __FPROTO_BETT_H__
#define __FPROTO_BETT_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    BETTDecoderStepReset = 0,
    BETTDecoderStepSaveDuration,
    BETTDecoderStepCheckDuration,
} BETTDecoderStep;

class FProtoSubGhzDBett : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDBett() {
        sensorType = FPS_BETT;
        te_short = 340;
        te_long = 2000;
        te_delta = 150;
        min_count_bit_for_found = 18;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case BETTDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 44) <
                                 (te_delta * 15))) {
                    decode_data = 0;
                    decode_count_bit = 0;
                    parser_step = BETTDecoderStepCheckDuration;
                }
                break;
            case BETTDecoderStepSaveDuration:
                if (!level) {
                    if (DURATION_DIFF(duration, te_short * 44) <
                        (te_delta * 15)) {
                        if (decode_count_bit ==
                            min_count_bit_for_found) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            // dip decoder needed
                            if (callback) callback(this);
                        } else {
                            parser_step = BETTDecoderStepReset;
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        break;
                    } else {
                        if ((DURATION_DIFF(duration, te_short) <
                             te_delta) ||
                            (DURATION_DIFF(duration, te_long) <
                             te_delta * 3)) {
                            parser_step = BETTDecoderStepCheckDuration;
                        } else {
                            parser_step = BETTDecoderStepReset;
                        }
                    }
                }
                break;
            case BETTDecoderStepCheckDuration:
                if (level) {
                    if (DURATION_DIFF(duration, te_long) <
                        te_delta * 3) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = BETTDecoderStepSaveDuration;
                    } else if (
                        DURATION_DIFF(duration, te_short) <
                        te_delta) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = BETTDecoderStepSaveDuration;
                    } else {
                        parser_step = BETTDecoderStepReset;
                    }
                } else {
                    parser_step = BETTDecoderStepReset;
                }
                break;
        }
    }
};

#endif
