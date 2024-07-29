
#ifndef __FPROTO_HOLTEKTH12X_H__
#define __FPROTO_HOLTEKTH12X_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    Holtek_HT12XDecoderStepReset = 0,
    Holtek_HT12XDecoderStepFoundStartBit,
    Holtek_HT12XDecoderStepSaveDuration,
    Holtek_HT12XDecoderStepCheckDuration,
} Holtek_HT12XDecoderStep;

class FProtoSubGhzDHoltekHt12x : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDHoltekHt12x() {
        sensorType = FPS_HOLTEKHT12X;
        te_short = 320;
        te_long = 640;
        te_delta = 200;
        min_count_bit_for_found = 12;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case Holtek_HT12XDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 36) < te_delta * 36)) {
                    // Found Preambula
                    parser_step = Holtek_HT12XDecoderStepFoundStartBit;
                }
                break;
            case Holtek_HT12XDecoderStepFoundStartBit:
                if ((level) && (DURATION_DIFF(duration, te_short) < te_delta)) {
                    // Found StartBit
                    parser_step = Holtek_HT12XDecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                } else {
                    parser_step = Holtek_HT12XDecoderStepReset;
                }
                break;
            case Holtek_HT12XDecoderStepSaveDuration:
                // save duration
                if (!level) {
                    if (duration >= ((uint32_t)te_short * 10 + te_delta)) {
                        if (decode_count_bit == min_count_bit_for_found) {
                            if (data != decode_data) {
                                data = decode_data;
                                data_count_bit = decode_count_bit;
                                if (callback) callback(this);
                            }
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        parser_step = Holtek_HT12XDecoderStepFoundStartBit;
                        break;
                    } else {
                        te_last = duration;
                        parser_step = Holtek_HT12XDecoderStepCheckDuration;
                    }
                } else {
                    parser_step = Holtek_HT12XDecoderStepReset;
                }
                break;
            case Holtek_HT12XDecoderStepCheckDuration:
                if (level) {
                    if ((DURATION_DIFF(te_last, te_long) < te_delta * 2) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = Holtek_HT12XDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta * 2)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = Holtek_HT12XDecoderStepSaveDuration;
                    } else {
                        parser_step = Holtek_HT12XDecoderStepReset;
                    }
                } else {
                    parser_step = Holtek_HT12XDecoderStepReset;
                }
                break;
        }
    }
};

#endif
