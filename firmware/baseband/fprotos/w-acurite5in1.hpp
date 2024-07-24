
#ifndef __FPROTO_Acurite_5in1_H__
#define __FPROTO_Acurite_5in1_H__

#include "weatherbase.hpp"

typedef enum {
    Acurite_5n1DecoderStepReset = 0,
    Acurite_5n1DecoderStepCheckPreambule,
    Acurite_5n1DecoderStepSaveDuration,
    Acurite_5n1DecoderStepCheckDuration,
} Acurite_5n1DecoderStep;

class FProtoWeatherAcurite5in1 : public FProtoWeatherBase {
   public:
    FProtoWeatherAcurite5in1() {
        sensorType = FPW_Acurite5in1;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case Acurite_5n1DecoderStepReset:
                if ((level) && (DURATION_DIFF(duration, te_short * 3) < te_delta * 2)) {
                    parser_step = Acurite_5n1DecoderStepCheckPreambule;
                    te_last = duration;
                    header_count = 0;
                }
                break;

            case Acurite_5n1DecoderStepCheckPreambule:
                if (level) {
                    te_last = duration;
                } else {
                    if ((DURATION_DIFF(te_last, te_short * 3) < te_delta * 2) &&
                        (DURATION_DIFF(duration, te_short * 3) < te_delta * 2)) {
                        // Found preambule
                        header_count++;
                    } else if ((header_count > 2) && (header_count < 5)) {
                        if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                            (DURATION_DIFF(duration, te_long) < te_delta)) {
                            decode_data = 0;
                            decode_count_bit = 0;
                            subghz_protocol_blocks_add_bit(0);
                            parser_step = Acurite_5n1DecoderStepSaveDuration;
                        } else if (
                            (DURATION_DIFF(te_last, te_long) < te_delta) &&
                            (DURATION_DIFF(duration, te_short) < te_delta)) {
                            decode_data = 0;
                            decode_count_bit = 0;
                            subghz_protocol_blocks_add_bit(1);
                            parser_step = Acurite_5n1DecoderStepSaveDuration;
                        } else {
                            parser_step = Acurite_5n1DecoderStepReset;
                        }
                    } else {
                        parser_step = Acurite_5n1DecoderStepReset;
                    }
                }
                break;

            case Acurite_5n1DecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = Acurite_5n1DecoderStepCheckDuration;
                } else {
                    parser_step = Acurite_5n1DecoderStepReset;
                }
                break;

            case Acurite_5n1DecoderStepCheckDuration:
                if (!level) {
                    if (duration >= ((uint32_t)te_short * 5)) {
                        if ((decode_count_bit == min_count_bit_for_found) &&
                            ws_protocol_acurite_5n1_check_crc() &&
                            ws_protocol_acurite_5n1_check_message_type()) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            ws_protocol_acurite_5n1_remote_controller();
                            if (callback) callback(this);
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        parser_step = Acurite_5n1DecoderStepReset;
                        break;
                    } else if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                               (DURATION_DIFF(duration, te_long) < te_delta)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = Acurite_5n1DecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = Acurite_5n1DecoderStepSaveDuration;
                    } else {
                        parser_step = Acurite_5n1DecoderStepReset;
                    }
                } else {
                    parser_step = Acurite_5n1DecoderStepReset;
                }
                break;
        }
    }

   protected:
    uint32_t te_short = 200;
    uint32_t te_long = 400;
    uint32_t te_delta = 90;
    uint32_t min_count_bit_for_found = 64;

    bool ws_protocol_acurite_5n1_check_message_type() {
        if (((decode_data >> 40) & 0x3F) == 0x38) {
            return true;
        } else {
            return false;
        }
    }

    bool ws_protocol_acurite_5n1_check_crc() {
        uint8_t msg[] = {
            (uint8_t)(decode_data >> 56),
            (uint8_t)(decode_data >> 48),
            (uint8_t)(decode_data >> 40),
            (uint8_t)(decode_data >> 32),
            (uint8_t)(decode_data >> 24),
            (uint8_t)(decode_data >> 16),
            (uint8_t)(decode_data >> 8)};

        if ((FProtoGeneral::subghz_protocol_blocks_add_bytes(msg, 7) ==
             (uint8_t)(decode_data & 0xFF)) &&
            (!FProtoGeneral::subghz_protocol_blocks_parity_bytes(&msg[2], 5))) {
            return true;
        } else {
            return false;
        }
    }

    void ws_protocol_acurite_5n1_remote_controller() {
        uint8_t channell[] = {3, 0, 2, 1};
        uint8_t channel_raw = ((data >> 62) & 0x03);
        channel = channell[channel_raw];
        id = (data >> 48) & 0x3FFF;
        battery_low = !((data >> 46) & 1);
        humidity = (data >> 8) & 0x7F;
        uint16_t temp_raw = ((data >> (24 - 7)) & 0x780) | ((data >> 16) & 0x7F);
        temp = FProtoGeneral::locale_fahrenheit_to_celsius(((float)(temp_raw)-400) / 10.0f);
    };
};

#endif
