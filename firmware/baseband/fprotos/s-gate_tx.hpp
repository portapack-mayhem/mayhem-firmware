
#ifndef __FPROTO_GATETX_H__
#define __FPROTO_GATETX_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    GateTXDecoderStepReset = 0,
    GateTXDecoderStepFoundStartBit,
    GateTXDecoderStepSaveDuration,
    GateTXDecoderStepCheckDuration,
} GateTXDecoderStep;

class FProtoSubGhzDGateTx : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDGateTx() {
        sensorType = FPS_GATETX;
        te_short = 350;
        te_long = 700;
        te_delta = 100;
        min_count_bit_for_found = 24;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case GateTXDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 47) < te_delta * 47)) {
                    // Found Preambula
                    parser_step = GateTXDecoderStepFoundStartBit;
                }
                break;
            case GateTXDecoderStepFoundStartBit:
                if (level && ((DURATION_DIFF(duration, te_long) < te_delta * 3))) {
                    // Found start bit
                    parser_step = GateTXDecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                } else {
                    parser_step = GateTXDecoderStepReset;
                }
                break;
            case GateTXDecoderStepSaveDuration:
                if (!level) {
                    if (duration >= ((uint32_t)te_short * 10 + te_delta)) {
                        parser_step = GateTXDecoderStepFoundStartBit;
                        if (decode_count_bit == min_count_bit_for_found) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            if (callback) callback(this);
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        break;
                    } else {
                        te_last = duration;
                        parser_step = GateTXDecoderStepCheckDuration;
                    }
                }
                break;
            case GateTXDecoderStepCheckDuration:
                if (level) {
                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta * 3)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = GateTXDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta * 3) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = GateTXDecoderStepSaveDuration;
                    } else {
                        parser_step = GateTXDecoderStepReset;
                    }
                } else {
                    parser_step = GateTXDecoderStepReset;
                }
                break;
        }
    }
};

#endif
