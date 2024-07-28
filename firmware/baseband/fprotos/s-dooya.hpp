
#ifndef __FPROTO_DOOYA_H__
#define __FPROTO_DOOYA_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    DooyaDecoderStepReset = 0,
    DooyaDecoderStepFoundStartBit,
    DooyaDecoderStepSaveDuration,
    DooyaDecoderStepCheckDuration,
} DooyaDecoderStep;

class FProtoSubGhzDDooya : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDDooya() {
        sensorType = FPS_DOOYA;
        te_short = 366;
        te_long = 733;
        te_delta = 120;
        min_count_bit_for_found = 40;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case DooyaDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_long * 12) < te_delta * 20)) {
                    parser_step = DooyaDecoderStepFoundStartBit;
                }
                break;

            case DooyaDecoderStepFoundStartBit:
                if (!level) {
                    if (DURATION_DIFF(duration, te_long * 2) < te_delta * 3) {
                        parser_step = DooyaDecoderStepSaveDuration;
                        decode_data = 0;
                        decode_count_bit = 0;
                    } else {
                        parser_step = DooyaDecoderStepReset;
                    }
                } else if (
                    DURATION_DIFF(duration, te_short * 13) < te_delta * 5) {
                    break;
                } else {
                    parser_step = DooyaDecoderStepReset;
                }
                break;

            case DooyaDecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = DooyaDecoderStepCheckDuration;
                } else {
                    parser_step = DooyaDecoderStepReset;
                }
                break;

            case DooyaDecoderStepCheckDuration:
                if (!level) {
                    if (duration >= (te_long * 4)) {
                        // add last bit
                        if (DURATION_DIFF(te_last, te_short) < te_delta) {
                            subghz_protocol_blocks_add_bit(0);
                        } else if (
                            DURATION_DIFF(te_last, te_long) <
                            te_delta * 2) {
                            subghz_protocol_blocks_add_bit(1);
                        } else {
                            parser_step = DooyaDecoderStepReset;
                            break;
                        }
                        parser_step = DooyaDecoderStepFoundStartBit;
                        if (decode_count_bit ==
                            min_count_bit_for_found) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            if (callback) callback(this);
                        }
                        break;
                    } else if (
                        (DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta * 2)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = DooyaDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta * 2) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = DooyaDecoderStepSaveDuration;
                    } else {
                        parser_step = DooyaDecoderStepReset;
                    }
                } else {
                    parser_step = DooyaDecoderStepReset;
                }
                break;
        }
    }
};

#endif
