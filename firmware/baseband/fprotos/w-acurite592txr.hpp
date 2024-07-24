
#ifndef __FPROTO_Acurite_592TXR_H__
#define __FPROTO_Acurite_592TXR_H__

#include "weatherbase.hpp"

typedef enum {
    Acurite_592TXRDecoderStepReset = 0,
    Acurite_592TXRDecoderStepCheckPreambule,
    Acurite_592TXRDecoderStepSaveDuration,
    Acurite_592TXRDecoderStepCheckDuration,
} Acurite_592TXRDecoderStep;

class FProtoWeatherAcurite592TXR : public FProtoWeatherBase {
   public:
    FProtoWeatherAcurite592TXR() {
        sensorType = FPW_Acurite592TXR;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case Acurite_592TXRDecoderStepReset:
                if ((level) && (DURATION_DIFF(duration, te_short * 3) <
                                te_delta * 2)) {
                    parser_step = Acurite_592TXRDecoderStepCheckPreambule;
                    te_last = duration;
                    header_count = 0;
                }
                break;

            case Acurite_592TXRDecoderStepCheckPreambule:
                if (level) {
                    te_last = duration;
                } else {
                    if ((DURATION_DIFF(
                             te_last, te_short * 3) <
                         te_delta * 2) &&
                        (DURATION_DIFF(duration, te_short * 3) <
                         te_delta * 2)) {
                        // Found preambule
                        header_count++;
                    } else if ((header_count > 2) && (header_count < 5)) {
                        if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                            (DURATION_DIFF(duration, te_long) < te_delta)) {
                            decode_data = 0;
                            decode_count_bit = 0;
                            subghz_protocol_blocks_add_bit(0);
                            parser_step = Acurite_592TXRDecoderStepSaveDuration;
                        } else if (
                            (DURATION_DIFF(
                                 te_last, te_long) <
                             te_delta) &&
                            (DURATION_DIFF(duration, te_short) <
                             te_delta)) {
                            decode_data = 0;
                            decode_count_bit = 0;
                            subghz_protocol_blocks_add_bit(1);
                            parser_step = Acurite_592TXRDecoderStepSaveDuration;
                        } else {
                            parser_step = Acurite_592TXRDecoderStepReset;
                        }
                    } else {
                        parser_step = Acurite_592TXRDecoderStepReset;
                    }
                }
                break;

            case Acurite_592TXRDecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = Acurite_592TXRDecoderStepCheckDuration;
                } else {
                    parser_step = Acurite_592TXRDecoderStepReset;
                }
                break;

            case Acurite_592TXRDecoderStepCheckDuration:
                if (!level) {
                    if (duration >= ((uint32_t)te_short * 5)) {
                        if ((decode_count_bit ==
                             min_count_bit_for_found) &&
                            ws_protocol_acurite_592txr_check_crc()) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            ws_protocol_acurite_592txr_remote_controller();
                            if (callback) callback(this);
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        parser_step = Acurite_592TXRDecoderStepReset;
                        break;
                    } else if (
                        (DURATION_DIFF(
                             te_last, te_short) <
                         te_delta) &&
                        (DURATION_DIFF(duration, te_long) <
                         te_delta)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = Acurite_592TXRDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(
                             te_last, te_long) <
                         te_delta) &&
                        (DURATION_DIFF(duration, te_short) <
                         te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = Acurite_592TXRDecoderStepSaveDuration;
                    } else {
                        parser_step = Acurite_592TXRDecoderStepReset;
                    }
                } else {
                    parser_step = Acurite_592TXRDecoderStepReset;
                }
                break;
        }
    }

   protected:
    uint32_t te_short = 200;
    uint32_t te_long = 400;
    uint32_t te_delta = 90;
    uint32_t min_count_bit_for_found = 56;

    bool ws_protocol_acurite_592txr_check_crc() {
        uint8_t msg[] = {
            static_cast<uint8_t>(decode_data >> 48),
            static_cast<uint8_t>(decode_data >> 40),
            static_cast<uint8_t>(decode_data >> 32),
            static_cast<uint8_t>(decode_data >> 24),
            static_cast<uint8_t>(decode_data >> 16),
            static_cast<uint8_t>(decode_data >> 8)};

        if ((FProtoGeneral::subghz_protocol_blocks_add_bytes(msg, 6) ==
             (uint8_t)(decode_data & 0xFF)) &&
            (!FProtoGeneral::subghz_protocol_blocks_parity_bytes(&msg[2], 4))) {
            return true;
        } else {
            return false;
        }
    }

    void ws_protocol_acurite_592txr_remote_controller() {
        uint8_t channel_[] = {3, 0, 2, 1};
        uint8_t channel_raw = ((data >> 54) & 0x03);
        channel = channel_[channel_raw];
        id = (data >> 40) & 0x3FFF;
        battery_low = !((data >> 38) & 1);
        humidity = (data >> 24) & 0x7F;
        uint16_t temp_raw = ((data >> 9) & 0xF80) | ((data >> 8) & 0x7F);
        temp = ((float)(temp_raw)-1000) / 10.0f;
    }
};

#endif
