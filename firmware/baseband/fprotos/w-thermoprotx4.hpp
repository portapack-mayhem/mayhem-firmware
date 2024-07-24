
#ifndef __FPROTO_THERMOPROTX4_H__
#define __FPROTO_THERMOPROTX4_H__

#include "weatherbase.hpp"
#define THERMO_PRO_TX4_TYPE_1 0b1001
#define THERMO_PRO_TX4_TYPE_2 0b0110

typedef enum {
    ThermoPRO_TX4DecoderStepReset = 0,
    ThermoPRO_TX4DecoderStepSaveDuration,
    ThermoPRO_TX4DecoderStepCheckDuration,
} ThermoPRO_TX4DecoderStep;

class FProtoWeatherThermoProTx4 : public FProtoWeatherBase {
   public:
    FProtoWeatherThermoProTx4() {
        sensorType = FPW_THERMOPROTX4;
    }

    void feed(bool level, uint32_t duration) override {
        switch (parser_step) {
            case ThermoPRO_TX4DecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 18) < te_delta * 10)) {
                    // Found sync
                    parser_step = ThermoPRO_TX4DecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                }
                break;

            case ThermoPRO_TX4DecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = ThermoPRO_TX4DecoderStepCheckDuration;
                } else {
                    parser_step = ThermoPRO_TX4DecoderStepReset;
                }
                break;

            case ThermoPRO_TX4DecoderStepCheckDuration:
                if (!level) {
                    if (DURATION_DIFF(duration, te_short * 18) < te_delta * 10) {
                        // Found sync
                        parser_step = ThermoPRO_TX4DecoderStepReset;
                        if ((decode_count_bit == min_count_bit_for_found) &&
                            ws_protocol_thermopro_tx4_check()) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            ws_protocol_thermopro_tx4_remote_controller();
                            if (callback) callback(this);
                            parser_step = ThermoPRO_TX4DecoderStepCheckDuration;
                        }
                        decode_data = 0;
                        decode_count_bit = 0;

                        break;
                    } else if (
                        (DURATION_DIFF(
                             te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta * 2)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = ThermoPRO_TX4DecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(
                             te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long * 2) < te_delta * 4)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = ThermoPRO_TX4DecoderStepSaveDuration;
                    } else {
                        parser_step = ThermoPRO_TX4DecoderStepReset;
                    }
                } else {
                    parser_step = ThermoPRO_TX4DecoderStepReset;
                }
                break;
        }
    }

   protected:
    // timing values
    uint32_t te_short = 500;
    uint32_t te_long = 2000;
    uint32_t te_delta = 150;
    uint32_t min_count_bit_for_found = 37;

    bool ws_protocol_thermopro_tx4_check() {
        uint8_t type = decode_data >> 33;
        if ((type == THERMO_PRO_TX4_TYPE_1) || (type == THERMO_PRO_TX4_TYPE_2)) {
            return true;
        } else {
            return false;
        }
    }

    void ws_protocol_thermopro_tx4_remote_controller() {
        id = (data >> 25) & 0xFF;
        battery_low = (data >> 24) & 1;
        // btn = (data >> 23) & 1;
        channel = ((data >> 21) & 0x03) + 1;
        if (!((data >> 20) & 1)) {
            temp = (float)((data >> 9) & 0x07FF) / 10.0f;
        } else {
            temp = (float)((~(data >> 9) & 0x07FF) + 1) / -10.0f;
        }
        humidity = (data >> 1) & 0xFF;
    }
};

#endif
