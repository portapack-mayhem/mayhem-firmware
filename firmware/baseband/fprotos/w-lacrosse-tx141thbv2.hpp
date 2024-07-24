
#ifndef __FPROTO_LACROSSE_TX141thbv2_H__
#define __FPROTO_LACROSSE_TX141thbv2_H__

#include "weatherbase.hpp"

#define LACROSSE_TX141TH_BV2_BIT_COUNT 41

typedef enum {
    LaCrosse_TX141THBv2DecoderStepReset = 0,
    LaCrosse_TX141THBv2DecoderStepCheckPreambule,
    LaCrosse_TX141THBv2DecoderStepSaveDuration,
    LaCrosse_TX141THBv2DecoderStepCheckDuration,
} LaCrosse_TX141THBv2DecoderStep;

class FProtoWeatherLaCrosseTx141thbv2 : public FProtoWeatherBase {
   public:
    FProtoWeatherLaCrosseTx141thbv2() {
        sensorType = FPW_LACROSSETX141thbv2;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case LaCrosse_TX141THBv2DecoderStepReset:
                if ((level) && (DURATION_DIFF(duration, te_short * 4) < te_delta * 2)) {
                    parser_step = LaCrosse_TX141THBv2DecoderStepCheckPreambule;
                    te_last = duration;
                    header_count = 0;
                }
                break;

            case LaCrosse_TX141THBv2DecoderStepCheckPreambule:
                if (level) {
                    te_last = duration;
                } else {
                    if ((DURATION_DIFF(te_last, te_short * 4) < te_delta * 2) &&
                        (DURATION_DIFF(duration, te_short * 4) < te_delta * 2)) {
                        // Found preambule
                        header_count++;
                    } else if (header_count == 4) {
                        if (ws_protocol_decoder_lacrosse_tx141thbv2_add_bit(te_last, duration)) {
                            decode_data = decode_data & 1;
                            decode_count_bit = 1;
                            parser_step = LaCrosse_TX141THBv2DecoderStepSaveDuration;
                        } else {
                            parser_step = LaCrosse_TX141THBv2DecoderStepReset;
                        }
                    } else {
                        parser_step = LaCrosse_TX141THBv2DecoderStepReset;
                    }
                }
                break;

            case LaCrosse_TX141THBv2DecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = LaCrosse_TX141THBv2DecoderStepCheckDuration;
                } else {
                    parser_step = LaCrosse_TX141THBv2DecoderStepReset;
                }
                break;

            case LaCrosse_TX141THBv2DecoderStepCheckDuration:
                if (!level) {
                    if (((DURATION_DIFF(te_last, te_short * 3) < te_delta * 2) &&
                         (DURATION_DIFF(duration, te_short * 4) < te_delta * 2))) {
                        if ((decode_count_bit == min_count_bit_for_found) ||
                            (decode_count_bit == LACROSSE_TX141TH_BV2_BIT_COUNT)) {
                            if (ws_protocol_lacrosse_tx141thbv2_check_crc()) {
                                data = decode_data;
                                data_count_bit = decode_count_bit;
                                ws_protocol_lacrosse_tx141thbv2_remote_controller();
                                if (callback) callback(this);
                            }
                            decode_data = 0;
                            decode_count_bit = 0;
                            header_count = 1;
                            parser_step = LaCrosse_TX141THBv2DecoderStepCheckPreambule;
                            break;
                        }
                    } else if (ws_protocol_decoder_lacrosse_tx141thbv2_add_bit(te_last, duration)) {
                        parser_step = LaCrosse_TX141THBv2DecoderStepSaveDuration;
                    } else {
                        parser_step = LaCrosse_TX141THBv2DecoderStepReset;
                    }
                } else {
                    parser_step = LaCrosse_TX141THBv2DecoderStepReset;
                }
                break;
        }
    }

   protected:
    uint32_t te_short = 208;
    uint32_t te_long = 417;
    uint32_t te_delta = 120;
    uint32_t min_count_bit_for_found = 40;

    bool ws_protocol_lacrosse_tx141thbv2_check_crc() {
        if (!decode_data) return false;
        uint64_t data = decode_data;
        if (decode_count_bit == LACROSSE_TX141TH_BV2_BIT_COUNT) {
            data >>= 1;
        }
        uint8_t msg[] = {static_cast<uint8_t>(data >> 32), static_cast<uint8_t>(data >> 24), static_cast<uint8_t>(data >> 16), static_cast<uint8_t>(data >> 8)};

        uint8_t crc = FProtoGeneral::subghz_protocol_blocks_lfsr_digest8_reflect(msg, 4, 0x31, 0xF4);
        return (crc == (data & 0xFF));
    }
    void ws_protocol_lacrosse_tx141thbv2_remote_controller() {
        if (data_count_bit == LACROSSE_TX141TH_BV2_BIT_COUNT) {
            data >>= 1;
        }
        id = data >> 32;
        battery_low = (data >> 31) & 1;
        // btn = (data >> 30) & 1;
        channel = ((data >> 28) & 0x03) + 1;
        temp = ((float)((data >> 16) & 0x0FFF) - 500.0f) / 10.0f;
        humidity = (data >> 8) & 0xFF;
    }
    bool ws_protocol_decoder_lacrosse_tx141thbv2_add_bit(uint32_t te_last, uint32_t te_current) {
        bool ret = false;
        if (DURATION_DIFF(te_last + te_current, te_short + te_long) < te_delta * 2) {
            if (te_last > te_current) {
                subghz_protocol_blocks_add_bit(1);
            } else {
                subghz_protocol_blocks_add_bit(0);
            }
            ret = true;
        }
        return ret;
    }
};

#endif
