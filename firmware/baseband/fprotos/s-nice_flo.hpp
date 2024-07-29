
#ifndef __FPROTO_NICE_FLO_H__
#define __FPROTO_NICE_FLO_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    NiceFloDecoderStepReset = 0,
    NiceFloDecoderStepFoundStartBit,
    NiceFloDecoderStepSaveDuration,
    NiceFloDecoderStepCheckDuration,
} NiceFloDecoderStep;

class FProtoSubGhzDNiceflo : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDNiceflo() {
        sensorType = FPS_NICEFLO;
        te_short = 700;
        te_long = 1400;
        te_delta = 200;
        min_count_bit_for_found = 12;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case NiceFloDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 36) < te_delta * 36)) {
                    // Found header Nice Flo
                    parser_step = NiceFloDecoderStepFoundStartBit;
                }
                break;
            case NiceFloDecoderStepFoundStartBit:
                if (!level) {
                    break;
                } else if (
                    DURATION_DIFF(duration, te_short) < te_delta) {
                    // Found start bit Nice Flo
                    parser_step = NiceFloDecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                } else {
                    parser_step = NiceFloDecoderStepReset;
                }
                break;
            case NiceFloDecoderStepSaveDuration:
                if (!level) {  // save interval
                    if (duration >= (te_short * 4)) {
                        parser_step = NiceFloDecoderStepFoundStartBit;
                        if (decode_count_bit >= min_count_bit_for_found) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            if (callback) callback(this);
                        }
                        break;
                    }
                    te_last = duration;
                    parser_step = NiceFloDecoderStepCheckDuration;
                } else {
                    parser_step = NiceFloDecoderStepReset;
                }
                break;
            case NiceFloDecoderStepCheckDuration:
                if (level) {
                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = NiceFloDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = NiceFloDecoderStepSaveDuration;
                    } else
                        parser_step = NiceFloDecoderStepReset;
                } else {
                    parser_step = NiceFloDecoderStepReset;
                }
                break;
        }
    }
};

#endif
