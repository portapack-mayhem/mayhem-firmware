
#ifndef __FPROTO_STARLINE_H__
#define __FPROTO_STARLINE_H__

#include "subghzdbase.hpp"

typedef enum {
    StarLineDecoderStepReset = 0,
    StarLineDecoderStepCheckPreambula,
    StarLineDecoderStepSaveDuration,
    StarLineDecoderStepCheckDuration,
} StarLineDecoderStep;

class FProtoSubGhzDStarLine : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDStarLine() {
        sensorType = FPS_STARLINE;
        te_short = 250;
        te_long = 500;
        te_delta = 120;
        min_count_bit_for_found = 64;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case StarLineDecoderStepReset:
                if (level) {
                    if (DURATION_DIFF(duration, te_long * 2) < te_delta * 2) {
                        parser_step = StarLineDecoderStepCheckPreambula;
                        header_count++;
                    } else if (header_count > 4) {
                        decode_data = 0;
                        decode_count_bit = 0;
                        te_last = duration;
                        parser_step = StarLineDecoderStepCheckDuration;
                    }
                } else {
                    header_count = 0;
                }
                break;
            case StarLineDecoderStepCheckPreambula:
                if ((!level) && (DURATION_DIFF(duration, te_long * 2) < te_delta * 2)) {
                    // Found Preambula
                    parser_step = StarLineDecoderStepReset;
                } else {
                    header_count = 0;
                    parser_step = StarLineDecoderStepReset;
                }
                break;
            case StarLineDecoderStepSaveDuration:
                if (level) {
                    if (duration >= (te_long + te_delta)) {
                        parser_step = StarLineDecoderStepReset;
                        if ((decode_count_bit >= min_count_bit_for_found) &&
                            (decode_count_bit <= min_count_bit_for_found + 2)) {
                            if (data != decode_data) {
                                data = decode_data;
                                data_count_bit = min_count_bit_for_found;
                                if (callback) callback(this);
                            }
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        header_count = 0;
                        break;
                    } else {
                        te_last = duration;
                        parser_step = StarLineDecoderStepCheckDuration;
                    }

                } else {
                    parser_step = StarLineDecoderStepReset;
                }
                break;
            case StarLineDecoderStepCheckDuration:
                if (!level) {
                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        if (decode_count_bit < min_count_bit_for_found) {
                            subghz_protocol_blocks_add_bit(0);
                        } else {
                            decode_count_bit++;
                        }
                        parser_step = StarLineDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta)) {
                        if (decode_count_bit <
                            min_count_bit_for_found) {
                            subghz_protocol_blocks_add_bit(1);
                        } else {
                            decode_count_bit++;
                        }
                        parser_step = StarLineDecoderStepSaveDuration;
                    } else {
                        parser_step = StarLineDecoderStepReset;
                    }
                } else {
                    parser_step = StarLineDecoderStepReset;
                }
                break;
        }
    }
};

#endif
