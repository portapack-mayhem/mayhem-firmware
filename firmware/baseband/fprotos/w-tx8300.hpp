
#ifndef __FPROTO_TX8300_H__
#define __FPROTO_TX8300_H__

#include "weatherbase.hpp"
typedef enum {
    TX_8300DecoderStepReset = 0,
    TX_8300DecoderStepCheckPreambule,
    TX_8300DecoderStepSaveDuration,
    TX_8300DecoderStepCheckDuration,
} TX_8300DecoderStep;
#define TX_8300_PACKAGE_SIZE 32

class FProtoWeatherTX8300 : public FProtoWeatherBase {
   public:
    FProtoWeatherTX8300() {
        sensorType = FPW_TX_8300;
    }

    void feed(bool level, uint32_t duration) override {
        switch (parser_step) {
            case TX_8300DecoderStepReset:
                if ((level) && (DURATION_DIFF(duration, te_short * 2) < te_delta)) {
                    parser_step = TX_8300DecoderStepCheckPreambule;
                }
                break;

            case TX_8300DecoderStepCheckPreambule:
                if ((!level) && ((DURATION_DIFF(duration, te_short * 2) < te_delta) ||
                                 (DURATION_DIFF(duration, te_short * 3) < te_delta))) {
                    parser_step = TX_8300DecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 1;
                    package_1 = 0;
                    package_2 = 0;
                } else {
                    parser_step = TX_8300DecoderStepReset;
                }
                break;

            case TX_8300DecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = TX_8300DecoderStepCheckDuration;
                } else {
                    parser_step = TX_8300DecoderStepReset;
                }
                break;

            case TX_8300DecoderStepCheckDuration:
                if (!level) {
                    if (duration >= ((uint32_t)te_short * 5)) {
                        // Found syncPostfix
                        if ((decode_count_bit ==
                             min_count_bit_for_found) &&
                            ws_protocol_tx_8300_check_crc()) {
                            data = package_1;
                            data_count_bit = decode_count_bit;
                            ws_protocol_tx_8300_remote_controller();
                            if (callback) callback(this);
                        }
                        decode_data = 0;
                        decode_count_bit = 1;
                        parser_step = TX_8300DecoderStepReset;
                        break;
                    } else if (
                        (DURATION_DIFF(te_last, te_short) <
                         te_delta) &&
                        (DURATION_DIFF(duration, te_long) <
                         te_delta * 2)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = TX_8300DecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_short) <
                         te_delta) &&
                        (DURATION_DIFF(duration, te_short) <
                         te_delta)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = TX_8300DecoderStepSaveDuration;
                    } else {
                        parser_step = TX_8300DecoderStepReset;
                    }

                    if (decode_count_bit == TX_8300_PACKAGE_SIZE) {
                        package_1 = decode_data;
                        decode_data = 0;
                    } else if (decode_count_bit == TX_8300_PACKAGE_SIZE * 2) {
                        package_2 = decode_data;
                        decode_data = 0;
                    }

                } else {
                    parser_step = TX_8300DecoderStepReset;
                }
                break;
        }
    }

   protected:
    // timing values
    uint32_t te_short = 1940;
    uint32_t te_long = 3880;
    uint32_t te_delta = 250;
    uint32_t min_count_bit_for_found = 72;

    uint32_t package_1{0};
    uint32_t package_2{0};
    bool ws_protocol_tx_8300_check_crc() {
        if (!package_2) return false;
        if (package_1 != ~package_2) return false;

        uint16_t x = 0;
        uint16_t y = 0;
        for (int i = 0; i < 32; i += 4) {
            x += (package_1 >> i) & 0x0F;
            y += (package_1 >> i) & 0x05;
        }
        uint8_t crc = (~x & 0xF) << 4 | (~y & 0xF);
        return (crc == (decode_data & 0xFF));
    }
    void ws_protocol_tx_8300_remote_controller() {
        humidity = (((data >> 28) & 0x0F) * 10) + ((data >> 24) & 0x0F);
        if (!((data >> 22) & 0x03))
            battery_low = 0;
        else
            battery_low = 1;
        channel = (data >> 20) & 0x03;
        id = (data >> 12) & 0x7F;

        float temp_raw = ((data >> 8) & 0x0F) * 10.0f + ((data >> 4) & 0x0F) +
                         (data & 0x0F) * 0.1f;
        if (!((data >> 19) & 1)) {
            temp = temp_raw;
        } else {
            temp = -temp_raw;
        }
    }
};

#endif
