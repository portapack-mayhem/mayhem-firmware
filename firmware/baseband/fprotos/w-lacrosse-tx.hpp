
#ifndef __FPROTO_LACROSSE_TX_H__
#define __FPROTO_LACROSSE_TX_H__

#include "weatherbase.hpp"

#define LACROSSE_TX_GAP 1000
#define LACROSSE_TX_BIT_SIZE 44
#define LACROSSE_TX_SUNC_PATTERN 0x0A000000000
#define LACROSSE_TX_SUNC_MASK 0x0F000000000
#define LACROSSE_TX_MSG_TYPE_TEMP 0x00
#define LACROSSE_TX_MSG_TYPE_HUM 0x0E
typedef enum {
    LaCrosse_TXDecoderStepReset = 0,
    LaCrosse_TXDecoderStepCheckPreambule,
    LaCrosse_TXDecoderStepSaveDuration,
    LaCrosse_TXDecoderStepCheckDuration,
} LaCrosse_TXDecoderStep;

class FProtoWeatherLaCrosseTx : public FProtoWeatherBase {
   public:
    FProtoWeatherLaCrosseTx() {
        sensorType = FPW_LACROSSETX;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case LaCrosse_TXDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, LACROSSE_TX_GAP) < te_delta * 2)) {
                    parser_step = LaCrosse_TXDecoderStepCheckPreambule;
                    header_count = 0;
                }
                break;

            case LaCrosse_TXDecoderStepCheckPreambule:

                if (level) {
                    if ((DURATION_DIFF(duration, te_short) < te_delta) &&
                        (header_count > 1)) {
                        parser_step = LaCrosse_TXDecoderStepCheckDuration;
                        decode_data = 0;
                        decode_count_bit = 0;
                        te_last = duration;
                    } else if (duration > (te_long * 2)) {
                        parser_step = LaCrosse_TXDecoderStepReset;
                    }
                } else {
                    if (DURATION_DIFF(duration, LACROSSE_TX_GAP) < te_delta * 2) {
                        te_last = duration;
                        header_count++;
                    } else {
                        parser_step = LaCrosse_TXDecoderStepReset;
                    }
                }

                break;

            case LaCrosse_TXDecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = LaCrosse_TXDecoderStepCheckDuration;
                } else {
                    parser_step = LaCrosse_TXDecoderStepReset;
                }
                break;

            case LaCrosse_TXDecoderStepCheckDuration:

                if (!level) {
                    if (duration > LACROSSE_TX_GAP * 3) {
                        if (DURATION_DIFF(te_last, te_short) < te_delta) {
                            subghz_protocol_blocks_add_bit(1);
                            parser_step = LaCrosse_TXDecoderStepSaveDuration;
                        } else if (
                            DURATION_DIFF(te_last, te_long) < te_delta) {
                            subghz_protocol_blocks_add_bit(0);
                            parser_step = LaCrosse_TXDecoderStepSaveDuration;
                        }
                        if ((decode_data & LACROSSE_TX_SUNC_MASK) ==
                            LACROSSE_TX_SUNC_PATTERN) {
                            if (ws_protocol_lacrosse_tx_check_crc()) {
                                data = decode_data;
                                data_count_bit = LACROSSE_TX_BIT_SIZE;
                                ws_protocol_lacrosse_tx_remote_controller();
                                if (callback) callback(this);
                            }
                        }

                        decode_data = 0;
                        decode_count_bit = 0;
                        header_count = 0;
                        parser_step = LaCrosse_TXDecoderStepReset;
                        break;
                    } else if (
                        (DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, LACROSSE_TX_GAP) < te_delta * 2)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = LaCrosse_TXDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta) &&
                        (DURATION_DIFF(duration, LACROSSE_TX_GAP) < te_delta * 2)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = LaCrosse_TXDecoderStepSaveDuration;
                    } else {
                        parser_step = LaCrosse_TXDecoderStepReset;
                    }

                } else {
                    parser_step = LaCrosse_TXDecoderStepReset;
                }

                break;
        }
    }

   protected:
    uint32_t te_short = 550;
    uint32_t te_long = 1300;
    uint32_t te_delta = 120;
    uint32_t min_count_bit_for_found = 40;

    void ws_protocol_lacrosse_tx_remote_controller() {
        uint8_t msg_type = (data >> 32) & 0x0F;
        id = (((data >> 28) & 0x0F) << 3) | (((data >> 24) & 0x0F) >> 1);

        float msg_value = (float)((data >> 20) & 0x0F) * 10.0f +
                          (float)((data >> 16) & 0x0F) +
                          (float)((data >> 12) & 0x0F) * 0.1f;

        if (msg_type == LACROSSE_TX_MSG_TYPE_TEMP) {  //-V1051
            temp = msg_value - 50.0f;
            humidity = WS_NO_HUMIDITY;
        } else if (msg_type == LACROSSE_TX_MSG_TYPE_HUM) {
            // ToDo for verification, records are needed with sensors maintaining temperature and temperature for this standard
            humidity = (uint8_t)msg_value;
        } else {
            // furi_crash("WS: WSProtocolLaCrosse_TX incorrect msg_type.");
        }

        battery_low = WS_NO_BATT;
        channel = WS_NO_CHANNEL;
    }

    bool ws_protocol_lacrosse_tx_check_crc() {
        if (!decode_data) return false;
        uint8_t msg[] = {
            static_cast<uint8_t>((decode_data >> 36) & 0x0F),
            static_cast<uint8_t>((decode_data >> 32) & 0x0F),
            static_cast<uint8_t>((decode_data >> 28) & 0x0F),
            static_cast<uint8_t>((decode_data >> 24) & 0x0F),
            static_cast<uint8_t>((decode_data >> 20) & 0x0F),
            static_cast<uint8_t>((decode_data >> 16) & 0x0F),
            static_cast<uint8_t>((decode_data >> 12) & 0x0F),
            static_cast<uint8_t>((decode_data >> 8) & 0x0F),
            static_cast<uint8_t>((decode_data >> 4) & 0x0F)};

        uint8_t crc = FProtoGeneral::subghz_protocol_blocks_add_bytes(msg, 9);
        return ((crc & 0x0F) == (decode_data & 0x0F));
    }
};

#endif
