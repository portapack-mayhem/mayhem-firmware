
#ifndef __FPROTO_PRINCETON_H__
#define __FPROTO_PRINCETON_H__

#include "subghzdbase.hpp"

typedef enum {
    PrincetonDecoderStepReset = 0,
    PrincetonDecoderStepSaveDuration,
    PrincetonDecoderStepCheckDuration,
} PrincetonDecoderStep;

class FProtoSubGhzDPrinceton : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDPrinceton() {
        sensorType = FPS_PRINCETON;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case PrincetonDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 36) < te_delta * 36)) {
                    // Found Preambula
                    parser_step = PrincetonDecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                    te = 0;
                }
                break;
            case PrincetonDecoderStepSaveDuration:
                // save duration
                if (level) {
                    te_last = duration;
                    te += duration;
                    parser_step = PrincetonDecoderStepCheckDuration;
                }
                break;
            case PrincetonDecoderStepCheckDuration:
                if (!level) {
                    if (duration >= ((uint32_t)te_long * 2)) {
                        parser_step = PrincetonDecoderStepSaveDuration;
                        if (decode_count_bit ==
                            min_count_bit_for_found) {
                            if ((last_data == decode_data) &&
                                last_data) {
                                te /= (decode_count_bit * 4 + 1);

                                data = decode_data;
                                data_count_bit = decode_count_bit;
                                subghz_protocol_princeton_check_remote_controller();
                                if (callback) callback(this);
                            }
                            last_data = decode_data;
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        te = 0;
                        break;
                    }

                    te += duration;

                    if ((DURATION_DIFF(te_last, te_short) <
                         te_delta) &&
                        (DURATION_DIFF(duration, te_long) <
                         te_delta * 3)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = PrincetonDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) <
                         te_delta * 3) &&
                        (DURATION_DIFF(duration, te_short) <
                         te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = PrincetonDecoderStepSaveDuration;
                    } else {
                        parser_step = PrincetonDecoderStepReset;
                    }
                } else {
                    parser_step = PrincetonDecoderStepReset;
                }
                break;
        }
    }
    void subghz_protocol_princeton_check_remote_controller() {
        serial = data >> 4;
        btn = data & 0xF;
        // te = te
        // key = (uint32_t)(data & 0xFFFFFF) --the whole packet.
    }

   protected:
    uint32_t te_short = 390;
    uint32_t te_long = 1170;
    uint32_t te_delta = 300;
    uint32_t min_count_bit_for_found = 24;

    uint32_t te = 0;
    uint32_t last_data = 0;
};

#endif
