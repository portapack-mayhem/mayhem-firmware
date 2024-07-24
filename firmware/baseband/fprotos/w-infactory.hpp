
#ifndef __FPROTO_Infactory_H__
#define __FPROTO_Infactory_H__

#include "weatherbase.hpp"

typedef enum {
    InfactoryDecoderStepReset = 0,
    InfactoryDecoderStepCheckPreambule,
    InfactoryDecoderStepSaveDuration,
    InfactoryDecoderStepCheckDuration,
} InfactoryDecoderStep;

class FProtoWeatherInfactory : public FProtoWeatherBase {
   public:
    FProtoWeatherInfactory() {
        sensorType = FPW_INFACTORY;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case InfactoryDecoderStepReset:
                if ((level) && (DURATION_DIFF(duration, te_short * 2) <
                                te_delta * 2)) {
                    parser_step = InfactoryDecoderStepCheckPreambule;
                    te_last = duration;
                    header_count = 0;
                }
                break;

            case InfactoryDecoderStepCheckPreambule:
                if (level) {
                    te_last = duration;
                } else {
                    if ((DURATION_DIFF(te_last, te_short * 2) <
                         te_delta * 2) &&
                        (DURATION_DIFF(duration, te_short * 2) <
                         te_delta * 2)) {
                        // Found preambule
                        header_count++;
                    } else if (
                        (DURATION_DIFF(te_last, te_short) <
                         te_delta) &&
                        (DURATION_DIFF(duration, te_short * 16) <
                         te_delta * 8)) {
                        // Found syncPrefix
                        if (header_count > 3) {
                            parser_step = InfactoryDecoderStepSaveDuration;
                            decode_data = 0;
                            decode_count_bit = 0;
                        }
                    } else {
                        parser_step = InfactoryDecoderStepReset;
                    }
                }
                break;

            case InfactoryDecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = InfactoryDecoderStepCheckDuration;
                } else {
                    parser_step = InfactoryDecoderStepReset;
                }
                break;

            case InfactoryDecoderStepCheckDuration:
                if (!level) {
                    if (duration >= ((uint32_t)te_short * 30)) {
                        // Found syncPostfix
                        if ((decode_count_bit ==
                             min_count_bit_for_found) &&
                            ws_protocol_infactory_check_crc()) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            ws_protocol_infactory_remote_controller();
                            if (callback) callback(this);
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        parser_step = InfactoryDecoderStepReset;
                        break;
                    } else if (
                        (DURATION_DIFF(te_last, te_short) <
                         te_delta) &&
                        (DURATION_DIFF(duration, te_long) <
                         te_delta * 2)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = InfactoryDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_short) <
                         te_delta) &&
                        (DURATION_DIFF(duration, te_long * 2) <
                         te_delta * 4)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = InfactoryDecoderStepSaveDuration;
                    } else {
                        parser_step = InfactoryDecoderStepReset;
                    }
                } else {
                    parser_step = InfactoryDecoderStepReset;
                }
                break;
        }
    }

   protected:
    uint32_t te_short = 500;
    uint32_t te_long = 2000;
    uint32_t te_delta = 150;
    uint32_t min_count_bit_for_found = 40;

    bool ws_protocol_infactory_check_crc() {
        uint8_t msg[] = {
            static_cast<uint8_t>(decode_data >> 32),
            static_cast<uint8_t>(((decode_data >> 24) & 0x0F) | (decode_data & 0x0F) << 4),
            static_cast<uint8_t>(decode_data >> 16),
            static_cast<uint8_t>(decode_data >> 8),
            static_cast<uint8_t>(decode_data)};

        uint8_t crc = FProtoGeneral::subghz_protocol_blocks_crc4(msg, 4, 0x13, 0);  // Koopmann 0x9, CCITT-4; FP-4; ITU-T G.704
        crc ^= msg[4] >> 4;                                                         // last nibble is only XORed
        return (crc == ((decode_data >> 28) & 0x0F));
    }
    void ws_protocol_infactory_remote_controller() {
        id = data >> 32;
        battery_low = (data >> 26) & 1;
        temp = FProtoGeneral::locale_fahrenheit_to_celsius(((float)((data >> 12) & 0x0FFF) - 900.0f) / 10.0f);
        humidity = (((data >> 8) & 0x0F) * 10) + ((data >> 4) & 0x0F);  // BCD, 'A0'=100%rH
        channel = data & 0x03;
    }
};

#endif
