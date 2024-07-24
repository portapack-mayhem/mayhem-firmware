
#ifndef __FPROTO_GTWT02_H__
#define __FPROTO_GTWT02_H__

#include "weatherbase.hpp"

#define AURIOL_AHFL_CONST_DATA 0b0100

typedef enum {
    GT_WT02DecoderStepReset = 0,
    GT_WT02DecoderStepSaveDuration,
    GT_WT02DecoderStepCheckDuration,
} GT_WT02DecoderStep;

class FProtoWeatherGTWT02 : public FProtoWeatherBase {
   public:
    FProtoWeatherGTWT02() {
        sensorType = FPW_GTWT02;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case GT_WT02DecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 18) <
                                 te_delta * 8)) {
                    // Found syncPrefix
                    parser_step = GT_WT02DecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                }
                break;

            case GT_WT02DecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = GT_WT02DecoderStepCheckDuration;
                } else {
                    parser_step = GT_WT02DecoderStepReset;
                }
                break;

            case GT_WT02DecoderStepCheckDuration:
                if (!level) {
                    if (DURATION_DIFF(te_last, te_short) <
                        te_delta) {
                        if (DURATION_DIFF(duration, te_short * 18) <
                            te_delta * 8) {
                            // Found syncPostfix
                            parser_step = GT_WT02DecoderStepReset;
                            if ((decode_count_bit ==
                                 min_count_bit_for_found) &&
                                ws_protocol_gt_wt_02_check()) {
                                data = decode_data;
                                data_count_bit = decode_count_bit;
                                ws_protocol_gt_wt_02_remote_controller();
                                if (callback) callback(this);
                            } else if (decode_count_bit == 1) {
                                parser_step = GT_WT02DecoderStepSaveDuration;
                            }
                            decode_data = 0;
                            decode_count_bit = 0;
                        } else if (
                            DURATION_DIFF(duration, te_long) <
                            te_delta * 2) {
                            subghz_protocol_blocks_add_bit(0);
                            parser_step = GT_WT02DecoderStepSaveDuration;
                        } else if (
                            DURATION_DIFF(duration, te_long * 2) <
                            te_delta * 4) {
                            subghz_protocol_blocks_add_bit(1);
                            parser_step = GT_WT02DecoderStepSaveDuration;
                        } else {
                            parser_step = GT_WT02DecoderStepReset;
                        }
                    } else {
                        parser_step = GT_WT02DecoderStepReset;
                    }
                } else {
                    parser_step = GT_WT02DecoderStepReset;
                }
                break;
        }
    }

   protected:
    uint32_t te_short = 500;
    uint32_t te_long = 2000;
    uint32_t te_delta = 150;
    uint32_t min_count_bit_for_found = 37;

    void ws_protocol_gt_wt_02_remote_controller() {
        id = (data >> 29) & 0xFF;
        battery_low = (data >> 28) & 1;
        // btn = (data >> 27) & 1;
        channel = ((data >> 25) & 0x3) + 1;

        if (!((data >> 24) & 1)) {
            temp = (float)((data >> 13) & 0x07FF) / 10.0f;
        } else {
            temp = (float)((~(data >> 13) & 0x07FF) + 1) / -10.0f;
        }

        humidity = (data >> 6) & 0x7F;
        if (humidity <= 10)  // actually the sensors sends 10 below working range of 20%
            humidity = 0;
        else if (humidity > 90)  // actually the sensors sends 110 above working range of 90%
            humidity = 100;
    }

    bool ws_protocol_gt_wt_02_check() {
        if (!decode_data) return false;
        uint8_t sum = (decode_data >> 5) & 0xe;
        uint64_t temp_data = decode_data >> 9;
        for (uint8_t i = 0; i < 7; i++) {
            sum += (temp_data >> (i * 4)) & 0xF;
        }
        return ((uint8_t)(decode_data & 0x3F) == (sum & 0x3F));
    }
};

#endif
