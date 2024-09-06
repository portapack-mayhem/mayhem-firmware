
#ifndef __FPROTO_SOLIGHT_TE44_H__
#define __FPROTO_SOLIGHT_TE44_H__

#include "weatherbase.hpp"

typedef enum {
    SolightTE44DecoderStepReset = 0,
    SolightTE44DecoderStepSaveDuration,
    SolightTE44DecoderStepCheckDuration,
} SolightTE44DecoderStep;

class FProtoWeatherSolightTE44 : public FProtoWeatherBase {
   public:
    FProtoWeatherSolightTE44() {
        sensorType = FPW_SolightTE44;
    }

    void feed(bool level, uint32_t duration) override {
        switch (parser_step) {
            case SolightTE44DecoderStepReset:
                if ((!level) && duration >= te_long) {
                    parser_step = SolightTE44DecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                }
                break;

            case SolightTE44DecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = SolightTE44DecoderStepCheckDuration;
                } else {
                    parser_step = SolightTE44DecoderStepReset;
                }
                break;

            case SolightTE44DecoderStepCheckDuration:
                if (!level) {
                    if (DURATION_DIFF(duration, te_short) < te_delta) {
                        if (decode_count_bit == min_count_bit_for_found &&
                            ws_protocol_solight_te44_check()) {
                            if (callback) {
                                callback(this);
                            }
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        parser_step = SolightTE44DecoderStepReset;
                    } else if (
                        DURATION_DIFF(te_last, te_short) < te_delta) {
                        if (DURATION_DIFF(duration, te_short * 2) < te_delta) {
                            subghz_protocol_blocks_add_bit(0);
                            parser_step = SolightTE44DecoderStepSaveDuration;
                        } else if (
                            DURATION_DIFF(duration, te_short * 4) < te_delta) {
                            subghz_protocol_blocks_add_bit(1);
                            parser_step = SolightTE44DecoderStepSaveDuration;
                        } else
                            parser_step = SolightTE44DecoderStepReset;
                    } else
                        parser_step = SolightTE44DecoderStepReset;
                } else
                    parser_step = SolightTE44DecoderStepReset;
                break;
        }
    }

   protected:
    // timing values
    uint32_t te_short = 490;
    uint32_t te_long = 3000;
    uint32_t te_delta = 150;
    uint32_t min_count_bit_for_found = 36;

    bool ws_protocol_solight_te44_check() {
        if (((decode_data >> 8) & 0x0f) != 0x0f) return false;  // const not 1111
        // Rubicson CRC check
        uint8_t msg_rubicson_crc[] = {
            (uint8_t)(decode_data >> 28),
            (uint8_t)(decode_data >> 20),
            (uint8_t)(decode_data >> 12),
            0xf0,
            (uint8_t)(((uint8_t)decode_data & 0xf0) | ((uint8_t)decode_data & 0x0f))};

        return FProtoGeneral::subghz_protocol_blocks_crc8(msg_rubicson_crc, 5, 0x31, 0x6c) == 0;
    }
};

#endif
