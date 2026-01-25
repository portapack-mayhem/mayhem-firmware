
#ifndef __FPROTO_HOLTEKHT6P20B_H__
#define __FPROTO_HOLTEKHT6P20B_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    Holtek_HT6P20BDecoderStepReset = 0,
    Holtek_HT6P20BDecoderStepFoundStartBit,
    Holtek_HT6P20BDecoderStepSaveDuration,
    Holtek_HT6P20BDecoderStepCheckDuration,
} Holtek_HT6P20BDecoderStep;

class FProtoSubGhzDHoltekHt6p20b : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDHoltekHt6p20b() {
        sensorType = FPS_HOLTEKHT6P20B;
        te_short = 450;
        te_long = 900;
        te_delta = 270;
        min_count_bit_for_found = 28;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case Holtek_HT6P20BDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 23) < te_delta * 23)) {
                    // Found Preambula
                    parser_step = Holtek_HT6P20BDecoderStepFoundStartBit;
                }
                break;
            case Holtek_HT6P20BDecoderStepFoundStartBit:
                if ((level) && (DURATION_DIFF(duration, te_short) < te_delta)) {
                    // Found StartBit
                    parser_step = Holtek_HT6P20BDecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                } else {
                    parser_step = Holtek_HT6P20BDecoderStepReset;
                }
                break;
            case Holtek_HT6P20BDecoderStepSaveDuration:
                // save duration
                if (!level) {
                    if (duration >= ((uint32_t)te_short * 10 + te_delta)) {
                        if (decode_count_bit == min_count_bit_for_found) {
                            data_count_bit = decode_count_bit;
                            if (callback) callback(this);
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        parser_step = Holtek_HT6P20BDecoderStepFoundStartBit;
                        break;
                    } else {
                        te_last = duration;
                        parser_step = Holtek_HT6P20BDecoderStepCheckDuration;
                    }
                } else {
                    parser_step = Holtek_HT6P20BDecoderStepReset;
                }
                break;
            case Holtek_HT6P20BDecoderStepCheckDuration:
                if (level) {
                    if ((DURATION_DIFF(te_last, te_long) < te_delta * 2) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = Holtek_HT6P20BDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta * 2)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = Holtek_HT6P20BDecoderStepSaveDuration;
                    } else {
                        parser_step = Holtek_HT6P20BDecoderStepReset;
                    }
                } else {
                    parser_step = Holtek_HT6P20BDecoderStepReset;
                }
                break;
        }
    }
};

#endif
