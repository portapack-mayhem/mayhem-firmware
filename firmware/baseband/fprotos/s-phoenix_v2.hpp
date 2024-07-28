
#ifndef __FPROTO_PHOENIX_V2_H__
#define __FPROTO_PHOENIX_V2_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    Phoenix_V2DecoderStepReset = 0,
    Phoenix_V2DecoderStepFoundStartBit,
    Phoenix_V2DecoderStepSaveDuration,
    Phoenix_V2DecoderStepCheckDuration,
} Phoenix_V2DecoderStep;

class FProtoSubGhzDPhoenixV2 : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDPhoenixV2() {
        sensorType = FPS_PHOENIXV2;
        te_short = 427;
        te_long = 853;
        te_delta = 100;
        min_count_bit_for_found = 52;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case Phoenix_V2DecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 60) < te_delta * 30)) {
                    // Found Preambula
                    parser_step = Phoenix_V2DecoderStepFoundStartBit;
                }
                break;
            case Phoenix_V2DecoderStepFoundStartBit:
                if (level && ((DURATION_DIFF(duration, (te_short * 6)) < te_delta * 4))) {
                    // Found start bit
                    parser_step = Phoenix_V2DecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                } else {
                    parser_step = Phoenix_V2DecoderStepReset;
                }
                break;
            case Phoenix_V2DecoderStepSaveDuration:
                if (!level) {
                    if (duration >= ((uint32_t)te_short * 10 + te_delta)) {
                        parser_step = Phoenix_V2DecoderStepFoundStartBit;
                        if (decode_count_bit ==
                            min_count_bit_for_found) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            if (callback) callback(this);
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        break;
                    } else {
                        te_last = duration;
                        parser_step = Phoenix_V2DecoderStepCheckDuration;
                    }
                }
                break;
            case Phoenix_V2DecoderStepCheckDuration:
                if (level) {
                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta * 3)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = Phoenix_V2DecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta * 3) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = Phoenix_V2DecoderStepSaveDuration;
                    } else {
                        parser_step = Phoenix_V2DecoderStepReset;
                    }
                } else {
                    parser_step = Phoenix_V2DecoderStepReset;
                }
                break;
        }
    }
};

#endif
