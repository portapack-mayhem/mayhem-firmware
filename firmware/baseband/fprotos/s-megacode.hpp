
#ifndef __FPROTO_MEGACODE_H__
#define __FPROTO_MEGACODE_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    MegaCodeDecoderStepReset = 0,
    MegaCodeDecoderStepFoundStartBit,
    MegaCodeDecoderStepSaveDuration,
    MegaCodeDecoderStepCheckDuration,
} MegaCodeDecoderStep;

class FProtoSubGhzDMegacode : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDMegacode() {
        sensorType = FPS_MEGACODE;
        te_short = 1000;
        te_long = 1000;
        te_delta = 200;
        min_count_bit_for_found = 24;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case MegaCodeDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 13) < te_delta * 17)) {  // 10..16ms
                    // Found header MegaCode
                    parser_step = MegaCodeDecoderStepFoundStartBit;
                }
                break;
            case MegaCodeDecoderStepFoundStartBit:
                if (level && (DURATION_DIFF(duration, te_short) < te_delta)) {
                    // Found start bit MegaCode
                    parser_step = MegaCodeDecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                    subghz_protocol_blocks_add_bit(1);
                    last_bit = 1;

                } else {
                    parser_step = MegaCodeDecoderStepReset;
                }
                break;
            case MegaCodeDecoderStepSaveDuration:
                if (!level) {  // save interval
                    if (duration >= (te_short * 10)) {
                        parser_step = MegaCodeDecoderStepReset;
                        if (decode_count_bit ==
                            min_count_bit_for_found) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;

                            if (callback) callback(this);
                        }
                        break;
                    }

                    if (!last_bit) {
                        te_last = duration - te_short * 3;
                    } else {
                        te_last = duration;
                    }
                    parser_step = MegaCodeDecoderStepCheckDuration;
                } else {
                    parser_step = MegaCodeDecoderStepReset;
                }
                break;
            case MegaCodeDecoderStepCheckDuration:
                if (level) {
                    if ((DURATION_DIFF(te_last, te_short * 5) < te_delta * 5) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        last_bit = 1;
                        parser_step = MegaCodeDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_short * 2) < te_delta * 2) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(0);
                        last_bit = 0;
                        parser_step = MegaCodeDecoderStepSaveDuration;
                    } else
                        parser_step = MegaCodeDecoderStepReset;
                } else {
                    parser_step = MegaCodeDecoderStepReset;
                }
                break;
        }
    }

   protected:
    uint8_t last_bit = false;
};

#endif
