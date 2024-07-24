
#ifndef __FPROTO_GTWT03_H__
#define __FPROTO_GTWT03_H__

#include "weatherbase.hpp"

#define AURIOL_AHFL_CONST_DATA 0b0100

typedef enum {
    GT_WT03DecoderStepReset = 0,
    GT_WT03DecoderStepCheckPreambule,
    GT_WT03DecoderStepSaveDuration,
    GT_WT03DecoderStepCheckDuration,
} GT_WT03DecoderStep;

class FProtoWeatherGTWT03 : public FProtoWeatherBase {
   public:
    FProtoWeatherGTWT03() {
        sensorType = FPW_GTWT03;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case GT_WT03DecoderStepReset:
                if ((level) && (DURATION_DIFF(duration, te_short * 3) <
                                te_delta * 2)) {
                    parser_step = GT_WT03DecoderStepCheckPreambule;
                    te_last = duration;
                    header_count = 0;
                }
                break;

            case GT_WT03DecoderStepCheckPreambule:
                if (level) {
                    te_last = duration;
                } else {
                    if ((DURATION_DIFF(te_last, te_short * 3) < te_delta * 2) &&
                        (DURATION_DIFF(duration, te_short * 3) < te_delta * 2)) {
                        // Found preambule
                        header_count++;
                    } else if (header_count == 4) {
                        if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                            (DURATION_DIFF(duration, te_long) < te_delta)) {
                            decode_data = 0;
                            decode_count_bit = 0;
                            subghz_protocol_blocks_add_bit(0);
                            parser_step = GT_WT03DecoderStepSaveDuration;
                        } else if (
                            (DURATION_DIFF(te_last, te_long) < te_delta) &&
                            (DURATION_DIFF(duration, te_short) < te_delta)) {
                            decode_data = 0;
                            decode_count_bit = 0;
                            subghz_protocol_blocks_add_bit(1);
                            parser_step = GT_WT03DecoderStepSaveDuration;
                        } else {
                            parser_step = GT_WT03DecoderStepReset;
                        }
                    } else {
                        parser_step = GT_WT03DecoderStepReset;
                    }
                }
                break;

            case GT_WT03DecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = GT_WT03DecoderStepCheckDuration;
                } else {
                    parser_step = GT_WT03DecoderStepReset;
                }
                break;

            case GT_WT03DecoderStepCheckDuration:
                if (!level) {
                    if (((DURATION_DIFF(te_last, te_short * 3) < te_delta * 2) &&
                         (DURATION_DIFF(duration, te_short * 3) < te_delta * 2))) {
                        if ((decode_count_bit ==
                             min_count_bit_for_found) &&
                            ws_protocol_gt_wt_03_check_crc()) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            ws_protocol_gt_wt_03_remote_controller();
                            if (callback) callback(this);
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        header_count = 1;
                        parser_step = GT_WT03DecoderStepCheckPreambule;
                        break;
                    } else if (
                        (DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = GT_WT03DecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = GT_WT03DecoderStepSaveDuration;
                    } else {
                        parser_step = GT_WT03DecoderStepReset;
                    }
                } else {
                    parser_step = GT_WT03DecoderStepReset;
                }
                break;
        }
    }

   protected:
    uint32_t te_short = 285;
    uint32_t te_long = 570;
    uint32_t te_delta = 120;
    uint32_t min_count_bit_for_found = 41;

    void ws_protocol_gt_wt_03_remote_controller() {
        id = data >> 33;
        humidity = (data >> 25) & 0xFF;

        if (humidity <= 10) {  // actually the sensors sends 10 below working range of 20%
            humidity = 0;
        } else if (humidity > 95) {  // actually the sensors sends 110 above working range of 90%
            humidity = 100;
        }

        battery_low = (data >> 24) & 1;
        // (data >> 23) & 1;
        channel = ((data >> 21) & 0x03) + 1;

        if (!((data >> 20) & 1)) {
            temp = (float)((data >> 9) & 0x07FF) / 10.0f;
        } else {
            temp = (float)((~(data >> 9) & 0x07FF) + 1) / -10.0f;
        }
    }

    bool ws_protocol_gt_wt_03_check_crc() {
        uint8_t msg[] = {
            static_cast<uint8_t>(decode_data >> 33),
            static_cast<uint8_t>(decode_data >> 25),
            static_cast<uint8_t>(decode_data >> 17),
            static_cast<uint8_t>(decode_data >> 9)};

        uint8_t sum = 0;
        for (unsigned k = 0; k < sizeof(msg); ++k) {
            uint8_t data = msg[k];
            uint16_t key = 0x3100;
            for (int i = 7; i >= 0; --i) {
                // XOR key into sum if data bit is set
                if ((data >> i) & 1) sum ^= key & 0xff;
                // roll the key right
                key = (key >> 1);
            }
        }
        return ((sum ^ (uint8_t)((decode_data >> 1) & 0xFF)) == 0x2D);
    }
};

#endif
