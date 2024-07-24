
#ifndef __FPROTO_Acurite_606TX_H__
#define __FPROTO_Acurite_606TX_H__

#include "weatherbase.hpp"

typedef enum {
    Acurite_606TXDecoderStepReset = 0,
    Acurite_606TXDecoderStepSaveDuration,
    Acurite_606TXDecoderStepCheckDuration,
} Acurite_606TXDecoderStep;

class FProtoWeatherAcurite606TX : public FProtoWeatherBase {
   public:
    FProtoWeatherAcurite606TX() {
        sensorType = FPW_Acurite606TX;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case Acurite_606TXDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 17) < te_delta * 8)) {
                    // Found syncPrefix
                    parser_step = Acurite_606TXDecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                }
                break;

            case Acurite_606TXDecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = Acurite_606TXDecoderStepCheckDuration;
                } else {
                    parser_step = Acurite_606TXDecoderStepReset;
                }
                break;

            case Acurite_606TXDecoderStepCheckDuration:
                if (!level) {
                    if (DURATION_DIFF(te_last, te_short) <
                        te_delta) {
                        if ((DURATION_DIFF(duration, te_short) <
                             te_delta) ||
                            (duration > te_long * 3)) {
                            // Found syncPostfix
                            parser_step = Acurite_606TXDecoderStepReset;
                            if ((decode_count_bit ==
                                 min_count_bit_for_found) &&
                                ws_protocol_acurite_606tx_check()) {
                                data = decode_data;
                                data_count_bit = decode_count_bit;
                                ws_protocol_acurite_606tx_remote_controller();
                                if (callback) callback(this);
                            }
                            decode_data = 0;
                            decode_count_bit = 0;
                        } else if (
                            DURATION_DIFF(duration, te_long) <
                            te_delta * 2) {
                            subghz_protocol_blocks_add_bit(0);
                            parser_step = Acurite_606TXDecoderStepSaveDuration;
                        } else if (
                            DURATION_DIFF(duration, te_long * 2) < te_delta * 4) {
                            subghz_protocol_blocks_add_bit(1);
                            parser_step = Acurite_606TXDecoderStepSaveDuration;
                        } else {
                            parser_step = Acurite_606TXDecoderStepReset;
                        }
                    } else {
                        parser_step = Acurite_606TXDecoderStepReset;
                    }
                } else {
                    parser_step = Acurite_606TXDecoderStepReset;
                }
                break;
        }
    }

   protected:
    uint32_t te_short = 500;
    uint32_t te_long = 2000;
    uint32_t te_delta = 150;
    uint32_t min_count_bit_for_found = 32;

    void ws_protocol_acurite_606tx_remote_controller() {
        id = (data >> 24) & 0xFF;
        battery_low = (data >> 23) & 1;
        channel = WS_NO_CHANNEL;
        if (!((data >> 19) & 1)) {
            temp = (float)((data >> 8) & 0x07FF) / 10.0f;
        } else {
            temp = (float)((~(data >> 8) & 0x07FF) + 1) / -10.0f;
        }
        humidity = WS_NO_HUMIDITY;
    }
    bool ws_protocol_acurite_606tx_check() {
        if (!decode_data) return false;
        uint8_t msg[] = {
            static_cast<uint8_t>(decode_data >> 24),
            static_cast<uint8_t>(decode_data >> 16),
            static_cast<uint8_t>(decode_data >> 8)};

        uint8_t crc = FProtoGeneral::subghz_protocol_blocks_lfsr_digest8(msg, 3, 0x98, 0xF1);
        return (crc == (decode_data & 0xFF));
    }
};

#endif
