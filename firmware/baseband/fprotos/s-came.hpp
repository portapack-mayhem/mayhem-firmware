
#ifndef __FPROTO_CAME_H__
#define __FPROTO_CAME_H__

#include "subghzdbase.hpp"

#define CAME_12_COUNT_BIT 12
#define CAME_24_COUNT_BIT 24
#define PRASTEL_COUNT_BIT 25
#define AIRFORCE_COUNT_BIT 18

typedef enum : uint8_t {
    CameDecoderStepReset = 0,
    CameDecoderStepFoundStartBit,
    CameDecoderStepSaveDuration,
    CameDecoderStepCheckDuration,
} CameDecoderStep;

class FProtoSubGhzDCame : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDCame() {
        sensorType = FPS_CAME;
        te_short = 320;
        te_long = 640;
        te_delta = 150;
        min_count_bit_for_found = 12;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case CameDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 56) < te_delta * 47)) {
                    // Found header CAME
                    parser_step = CameDecoderStepFoundStartBit;
                }
                break;
            case CameDecoderStepFoundStartBit:
                if (!level) {
                    break;
                } else if (
                    DURATION_DIFF(duration, te_short) < te_delta) {
                    // Found start bit CAME
                    parser_step = CameDecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                } else {
                    parser_step = CameDecoderStepReset;
                }
                break;
            case CameDecoderStepSaveDuration:
                if (!level) {  // save interval
                    if (duration >= (te_short * 4)) {
                        parser_step = CameDecoderStepFoundStartBit;
                        if ((decode_count_bit == min_count_bit_for_found) || (decode_count_bit == AIRFORCE_COUNT_BIT) ||
                            (decode_count_bit == PRASTEL_COUNT_BIT) || (decode_count_bit == CAME_24_COUNT_BIT)) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            // if flippa hacky, i hacky
                            sensorType = FPS_CAME;
                            if (decode_count_bit == PRASTEL_COUNT_BIT) sensorType = FPS_PRASTEL;
                            if (decode_count_bit == AIRFORCE_COUNT_BIT) sensorType = FPS_AIRFORCE;
                            if (callback) callback(this);
                        }
                        break;
                    }
                    te_last = duration;
                    parser_step = CameDecoderStepCheckDuration;
                } else {
                    parser_step = CameDecoderStepReset;
                }
                break;
            case CameDecoderStepCheckDuration:
                if (level) {
                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = CameDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = CameDecoderStepSaveDuration;
                    } else
                        parser_step = CameDecoderStepReset;
                } else {
                    parser_step = CameDecoderStepReset;
                }
                break;
        }
    }
};

#endif
