
#ifndef __FPROTO_VAUNO_EN8822_H__
#define __FPROTO_VAUNO_EN8822_H__

#include "weatherbase.hpp"

typedef enum {
    VaunoEN8822CDecoderStepReset = 0,
    VaunoEN8822CDecoderStepSaveDuration,
    VaunoEN8822CDecoderStepCheckDuration,
} VaunoEN8822CDecoderStep;

class FProtoWeatherVaunoEN8822 : public FProtoWeatherBase {
   public:
    FProtoWeatherVaunoEN8822() {
        sensorType = FPW_Vauno_EN8822;
    }

    void feed(bool level, uint32_t duration) override {
        switch (parser_step) {
            case VaunoEN8822CDecoderStepReset:
                if ((!level) && DURATION_DIFF(duration, te_long * 4) < te_delta) {
                    parser_step = VaunoEN8822CDecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                }
                break;

            case VaunoEN8822CDecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = VaunoEN8822CDecoderStepCheckDuration;
                } else {
                    parser_step = VaunoEN8822CDecoderStepReset;
                }
                break;

            case VaunoEN8822CDecoderStepCheckDuration:
                if (!level) {
                    if (DURATION_DIFF(te_last, te_short) < te_delta) {
                        if (DURATION_DIFF(duration, te_long * 2) < te_delta) {
                            subghz_protocol_blocks_add_bit(1);
                            parser_step = VaunoEN8822CDecoderStepSaveDuration;
                        } else if (DURATION_DIFF(duration, te_long) < te_delta) {
                            subghz_protocol_blocks_add_bit(0);
                            parser_step = VaunoEN8822CDecoderStepSaveDuration;
                        } else if (DURATION_DIFF(duration, te_long * 4) < te_delta) {
                            parser_step = VaunoEN8822CDecoderStepReset;
                            if (decode_count_bit == min_count_bit_for_found && ws_protocol_vauno_en8822c_check()) {
                                // ws_protocol_vauno_en8822c_extract_data(&instance->generic);

                                if (callback) {
                                    callback(this);
                                }
                            } else if (decode_count_bit == 1) {
                                parser_step = VaunoEN8822CDecoderStepSaveDuration;
                            }
                            decode_data = 0;
                            decode_count_bit = 0;
                        } else
                            parser_step = VaunoEN8822CDecoderStepReset;
                    } else
                        parser_step = VaunoEN8822CDecoderStepReset;
                } else
                    parser_step = VaunoEN8822CDecoderStepReset;
                break;
        }
    }

   protected:
    // timing values
    uint32_t te_short = 500;
    uint32_t te_long = 1940;
    uint32_t te_delta = 150;
    uint32_t min_count_bit_for_found = 42;

    bool ws_protocol_vauno_en8822c_check() {
        if (!decode_data) return false;
        // The sum of all nibbles should match the last 6 bits
        uint8_t sum = 0;
        for (uint8_t i = 6; i <= 38; i += 4) {
            sum += ((decode_data >> i) & 0x0f);
        }
        return sum != 0 && (sum & 0x3f) == (decode_data & 0x3f);
    }
};

#endif
