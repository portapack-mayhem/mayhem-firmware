
#ifndef __FPROTO_AuriolTH_H__
#define __FPROTO_AuriolTH_H__

#include "weatherbase.hpp"

#define AURIOL_TH_CONST_DATA 0b1110

typedef enum {
    auriol_THDecoderStepReset = 0,
    auriol_THDecoderStepSaveDuration,
    auriol_THDecoderStepCheckDuration,
} auriol_THDecoderStep;

class FProtoWeatherAuriolTh : public FProtoWeatherBase {
   public:
    FProtoWeatherAuriolTh() {
        sensorType = FPW_AuriolTH;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case auriol_THDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 8) < te_delta)) {
                    // Found sync
                    parser_step = auriol_THDecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                }
                break;

            case auriol_THDecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = auriol_THDecoderStepCheckDuration;
                } else {
                    parser_step = auriol_THDecoderStepReset;
                }
                break;

            case auriol_THDecoderStepCheckDuration:
                if (!level) {
                    if (DURATION_DIFF(duration, te_short * 8) <
                        te_delta) {
                        // Found sync
                        parser_step = auriol_THDecoderStepReset;
                        if ((decode_count_bit ==
                             min_count_bit_for_found) &&
                            ws_protocol_auriol_th_check()) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            ws_protocol_auriol_th_remote_controller();
                            if (callback) callback(this);
                            parser_step = auriol_THDecoderStepCheckDuration;
                        }
                        decode_data = 0;
                        decode_count_bit = 0;

                        break;
                    } else if (
                        (DURATION_DIFF(te_last, te_short) <
                         te_delta) &&
                        (DURATION_DIFF(duration, te_short * 2) <
                         te_delta)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = auriol_THDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_short) <
                         te_delta) &&
                        (DURATION_DIFF(duration, te_short * 4) <
                         te_delta * 2)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = auriol_THDecoderStepSaveDuration;
                    } else {
                        parser_step = auriol_THDecoderStepReset;
                    }
                } else {
                    parser_step = auriol_THDecoderStepReset;
                }
                break;
        }
    }

   protected:
    uint32_t te_short = 500;
    uint32_t te_long = 2000;
    uint32_t te_delta = 150;
    uint32_t min_count_bit_for_found = 37;

    void ws_protocol_auriol_th_remote_controller() {
        id = (data >> 31) & 0xFF;
        battery_low = ((data >> 30) & 1);
        channel = ((data >> 25) & 0x03) + 1;
        if (!((data >> 23) & 1)) {
            temp = (float)((data >> 13) & 0x07FF) / 10.0f;
        } else {
            temp = (float)((~(data >> 13) & 0x07FF) + 1) / -10.0f;
        }

        humidity = (data >> 1) & 0x7F;
    }
    bool ws_protocol_auriol_th_check() {
        uint8_t type = (decode_data >> 8) & 0x0F;

        if ((type == AURIOL_TH_CONST_DATA) && ((decode_data >> 4) != 0xffffffff)) {
            return true;
        } else {
            return false;
        }
        return true;
    }
};

#endif
