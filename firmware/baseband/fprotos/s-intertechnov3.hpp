
#ifndef __FPROTO_INTERTECHNOV3_H__
#define __FPROTO_INTERTECHNOV3_H__

#include "subghzdbase.hpp"

#define INTERTECHNO_V3_DIMMING_COUNT_BIT 36

typedef enum : uint8_t {
    IntertechnoV3DecoderStepReset = 0,
    IntertechnoV3DecoderStepStartSync,
    IntertechnoV3DecoderStepFoundSync,
    IntertechnoV3DecoderStepStartDuration,
    IntertechnoV3DecoderStepSaveDuration,
    IntertechnoV3DecoderStepCheckDuration,
    IntertechnoV3DecoderStepEndDuration,
} IntertechnoV3DecoderStep;

class FProtoSubGhzDIntertechnoV3 : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDIntertechnoV3() {
        sensorType = FPS_INTERTECHNOV3;
        te_short = 275;
        te_long = 1375;
        te_delta = 150;
        min_count_bit_for_found = 32;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case IntertechnoV3DecoderStepReset:
                if ((!level) &&
                    (DURATION_DIFF(duration, te_short * 37) < te_delta * 15)) {
                    parser_step = IntertechnoV3DecoderStepStartSync;
                }
                break;
            case IntertechnoV3DecoderStepStartSync:
                if (level && (DURATION_DIFF(duration, te_short) < te_delta)) {
                    parser_step = IntertechnoV3DecoderStepFoundSync;
                } else {
                    parser_step = IntertechnoV3DecoderStepReset;
                }
                break;

            case IntertechnoV3DecoderStepFoundSync:
                if (!level && (DURATION_DIFF(duration, te_short * 10) < te_delta * 3)) {
                    parser_step = IntertechnoV3DecoderStepStartDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                } else {
                    parser_step = IntertechnoV3DecoderStepReset;
                }
                break;

            case IntertechnoV3DecoderStepStartDuration:
                if (level && (DURATION_DIFF(duration, te_short) < te_delta)) {
                    parser_step = IntertechnoV3DecoderStepSaveDuration;
                } else {
                    parser_step = IntertechnoV3DecoderStepReset;
                }
                break;

            case IntertechnoV3DecoderStepSaveDuration:
                if (!level) {  // save interval
                    if (duration >= (te_short * 11)) {
                        parser_step = IntertechnoV3DecoderStepStartSync;
                        if ((decode_count_bit == min_count_bit_for_found) ||
                            (decode_count_bit == INTERTECHNO_V3_DIMMING_COUNT_BIT)) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            if (callback) callback(this);
                        }
                        break;
                    }
                    te_last = duration;
                    parser_step = IntertechnoV3DecoderStepCheckDuration;
                } else {
                    parser_step = IntertechnoV3DecoderStepReset;
                }
                break;
            case IntertechnoV3DecoderStepCheckDuration:
                if (level) {
                    // Add 0 bit
                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = IntertechnoV3DecoderStepEndDuration;
                    } else if (
                        // Add 1 bit
                        (DURATION_DIFF(te_last, te_long) < te_delta * 2) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = IntertechnoV3DecoderStepEndDuration;

                    } else if (
                        // Add dimm_state
                        (DURATION_DIFF(te_last, te_short) < te_delta * 2) &&
                        (DURATION_DIFF(duration, te_short) < te_delta) &&
                        (decode_count_bit == 27)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = IntertechnoV3DecoderStepEndDuration;

                    } else
                        parser_step = IntertechnoV3DecoderStepReset;
                } else {
                    parser_step = IntertechnoV3DecoderStepReset;
                }
                break;

            case IntertechnoV3DecoderStepEndDuration:
                if (!level && ((DURATION_DIFF(duration, te_short) < te_delta) ||
                               (DURATION_DIFF(duration, te_long) < te_delta * 2))) {
                    parser_step = IntertechnoV3DecoderStepStartDuration;
                } else {
                    parser_step = IntertechnoV3DecoderStepReset;
                }
                break;
        }
    }

   protected:
};

#endif
