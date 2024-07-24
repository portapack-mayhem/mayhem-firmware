
#ifndef __FPROTO_AuriolAhfl_H__
#define __FPROTO_AuriolAhfl_H__

#include "weatherbase.hpp"

#define AURIOL_AHFL_CONST_DATA 0b0100

typedef enum {
    auriol_AHFLDecoderStepReset = 0,
    auriol_AHFLDecoderStepSaveDuration,
    auriol_AHFLDecoderStepCheckDuration,
} auriol_AHFLDecoderStep;

class FProtoWeatherAuriolAhfl : public FProtoWeatherBase {
   public:
    FProtoWeatherAuriolAhfl() {
        sensorType = FPW_AuriolAhfl;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case auriol_AHFLDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 18) <
                                 te_delta)) {
                    // Found syncPrefix
                    parser_step = auriol_AHFLDecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                }
                break;
            case auriol_AHFLDecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = auriol_AHFLDecoderStepCheckDuration;
                } else {
                    parser_step = auriol_AHFLDecoderStepReset;
                }
                break;
            case auriol_AHFLDecoderStepCheckDuration:
                if (!level) {
                    if (DURATION_DIFF(te_last, te_short) <
                        te_delta) {
                        if (DURATION_DIFF(duration, te_short * 18) <
                            te_delta * 8) {
                            // Found syncPostfix
                            parser_step = auriol_AHFLDecoderStepReset;
                            if ((decode_count_bit ==
                                 min_count_bit_for_found) &&
                                ws_protocol_auriol_ahfl_check()) {
                                data = decode_data;
                                data_count_bit = decode_count_bit;
                                ws_protocol_auriol_ahfl_remote_controller();
                                if (callback) {
                                    callback(this);
                                }
                            } else if (decode_count_bit == 1) {
                                parser_step = auriol_AHFLDecoderStepSaveDuration;
                            }
                            decode_data = 0;
                            decode_count_bit = 0;
                        } else if (
                            DURATION_DIFF(duration, te_long) <
                            te_delta * 2) {
                            subghz_protocol_blocks_add_bit(0);
                            parser_step = auriol_AHFLDecoderStepSaveDuration;
                        } else if (
                            DURATION_DIFF(duration, te_long * 2) <
                            te_delta * 4) {
                            subghz_protocol_blocks_add_bit(1);
                            parser_step = auriol_AHFLDecoderStepSaveDuration;
                        } else {
                            parser_step = auriol_AHFLDecoderStepReset;
                        }
                    } else {
                        parser_step = auriol_AHFLDecoderStepReset;
                    }
                } else {
                    parser_step = auriol_AHFLDecoderStepReset;
                }
                break;
        }
    }

   protected:
    uint32_t te_short = 500;
    uint32_t te_long = 2000;
    uint32_t te_delta = 150;
    uint32_t min_count_bit_for_found = 42;

    void ws_protocol_auriol_ahfl_remote_controller() {
        id = data >> 34;
        battery_low = (data >> 33) & 1;
        // btn = (data >> 32) & 1;
        channel = ((data >> 30) & 0x3) + 1;
        if (!((data >> 29) & 1)) {
            temp = (float)((data >> 18) & 0x07FF) / 10.0f;
        } else {
            temp = (float)((~(data >> 18) & 0x07FF) + 1) / -10.0f;
        }
        humidity = (data >> 11) & 0x7F;
    }

    bool ws_protocol_auriol_ahfl_check() {
        uint8_t type = (decode_data >> 6) & 0x0F;
        if (type != AURIOL_AHFL_CONST_DATA) {
            // Fail const data check
            return false;
        }
        uint64_t payload = decode_data >> 6;
        // Checksum is the last 6 bits of data
        uint8_t checksum_received = decode_data & 0x3F;
        uint8_t checksum_calculated = 0;
        for (uint8_t i = 0; i < 9; i++) {
            checksum_calculated += (payload >> (i * 4)) & 0xF;
        }
        return checksum_received == checksum_calculated;
    }
};

#endif
