
#ifndef __FPROTO_CLEMSA_H__
#define __FPROTO_CLEMSA_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    ClemsaDecoderStepReset = 0,
    ClemsaDecoderStepSaveDuration,
    ClemsaDecoderStepCheckDuration,
} ClemsaDecoderStep;

class FProtoSubGhzDClemsa : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDClemsa() {
        sensorType = FPS_CLEMSA;
        te_short = 385;
        te_long = 2695;
        te_delta = 150;
        min_count_bit_for_found = 18;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case ClemsaDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 51) < te_delta * 25)) {
                    parser_step = ClemsaDecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                }
                break;

            case ClemsaDecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = ClemsaDecoderStepCheckDuration;
                } else {
                    parser_step = ClemsaDecoderStepReset;
                }
                break;

            case ClemsaDecoderStepCheckDuration:
                if (!level) {
                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta * 3)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = ClemsaDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta * 3) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = ClemsaDecoderStepSaveDuration;
                    } else if (
                        DURATION_DIFF(duration, te_short * 51) < te_delta * 25) {
                        if ((DURATION_DIFF(te_last, te_short) < te_delta)) {
                            subghz_protocol_blocks_add_bit(0);
                        } else if ((DURATION_DIFF(te_last, te_long) < te_delta * 3)) {
                            subghz_protocol_blocks_add_bit(1);
                        } else {
                            parser_step = ClemsaDecoderStepReset;
                        }

                        if (decode_count_bit ==
                            min_count_bit_for_found) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            if (callback) callback(this);
                        }
                        parser_step = ClemsaDecoderStepSaveDuration;
                        decode_data = 0;
                        decode_count_bit = 0;

                    } else {
                        parser_step = ClemsaDecoderStepReset;
                    }
                } else {
                    parser_step = ClemsaDecoderStepReset;
                }
                break;
        }
    }
};

#endif
