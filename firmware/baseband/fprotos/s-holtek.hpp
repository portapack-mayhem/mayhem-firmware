
#ifndef __FPROTO_HOLTEK_H__
#define __FPROTO_HOLTEK_H__

#include "subghzdbase.hpp"

#define HOLTEK_HEADER_MASK 0xF000000000
#define HOLTEK_HEADER 0x5000000000

typedef enum : uint8_t {
    HoltekDecoderStepReset = 0,
    HoltekDecoderStepFoundStartBit,
    HoltekDecoderStepSaveDuration,
    HoltekDecoderStepCheckDuration,
} HoltekDecoderStep;

class FProtoSubGhzDHoltek : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDHoltek() {
        sensorType = FPS_HOLTEK;
        te_short = 430;
        te_long = 870;
        te_delta = 100;
        min_count_bit_for_found = 40;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case HoltekDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 36) < te_delta * 36)) {
                    // Found Preambula
                    parser_step = HoltekDecoderStepFoundStartBit;
                }
                break;
            case HoltekDecoderStepFoundStartBit:
                if ((level) && (DURATION_DIFF(duration, te_short) < te_delta)) {
                    // Found StartBit
                    parser_step = HoltekDecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                } else {
                    parser_step = HoltekDecoderStepReset;
                }
                break;
            case HoltekDecoderStepSaveDuration:
                // save duration
                if (!level) {
                    if (duration >= ((uint32_t)te_short * 10 + te_delta)) {
                        if (decode_count_bit ==
                            min_count_bit_for_found) {
                            if ((decode_data & HOLTEK_HEADER_MASK) == HOLTEK_HEADER) {
                                data = decode_data;
                                data_count_bit = decode_count_bit;
                                if (callback) callback(this);
                            }
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        parser_step = HoltekDecoderStepFoundStartBit;
                        break;
                    } else {
                        te_last = duration;
                        parser_step = HoltekDecoderStepCheckDuration;
                    }
                } else {
                    parser_step = HoltekDecoderStepReset;
                }
                break;
            case HoltekDecoderStepCheckDuration:
                if (level) {
                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta * 2)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = HoltekDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta * 2) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = HoltekDecoderStepSaveDuration;
                    } else {
                        parser_step = HoltekDecoderStepReset;
                    }
                } else {
                    parser_step = HoltekDecoderStepReset;
                }
                break;
        }
    }
};

#endif
