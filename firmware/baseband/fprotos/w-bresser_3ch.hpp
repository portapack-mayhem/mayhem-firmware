
#ifndef __FPROTO_Bresser3ch_H__
#define __FPROTO_Bresser3ch_H__

#include "weatherbase.hpp"

#define BRESSER_V0_DATA 36
#define BRESSER_V0_DATA_AND_TAIL 52
#define BRESSER_V1_DATA 40

typedef enum {
    Bresser3chDecoderStepReset = 0,
    Bresser3chDecoderStepV0SaveDuration,
    Bresser3chDecoderStepV0CheckDuration,
    Bresser3chDecoderStepV0TailCheckDuration,
    Bresser3chDecoderStepV1PreambleDn,
    Bresser3chDecoderStepV1PreambleUp,
    Bresser3chDecoderStepV1SaveDuration,
    Bresser3chDecoderStepV1CheckDuration,
} Bresser3chDecoderStepV1;

class FProtoWeatheBresser3CH : public FProtoWeatherBase {
   public:
    FProtoWeatheBresser3CH() {
        sensorType = FPW_Bresser3CH;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case Bresser3chDecoderStepReset:
                if (level && DURATION_DIFF(duration, te_short * 3) < te_delta) {
                    te_last = duration;
                    parser_step = Bresser3chDecoderStepV1PreambleDn;
                    decode_data = 0;
                    decode_count_bit = 0;
                } else if ((!level) && duration >= te_long) {
                    parser_step = Bresser3chDecoderStepV0SaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                }
                break;

            case Bresser3chDecoderStepV0SaveDuration:
                if (level) {
                    te_last = duration;
                    if (decode_count_bit < BRESSER_V0_DATA) {
                        parser_step = Bresser3chDecoderStepV0CheckDuration;
                    } else {
                        parser_step = Bresser3chDecoderStepV0TailCheckDuration;
                    }
                } else {
                    parser_step = Bresser3chDecoderStepReset;
                }
                break;

            case Bresser3chDecoderStepV0CheckDuration:
                if (!level) {
                    if (DURATION_DIFF(te_last, te_short) < te_delta) {
                        if (DURATION_DIFF(duration, te_short * 2) < te_delta) {
                            subghz_protocol_blocks_add_bit(0);
                            parser_step = Bresser3chDecoderStepV0SaveDuration;
                        } else if (
                            DURATION_DIFF(duration, te_short * 4) < te_delta) {
                            subghz_protocol_blocks_add_bit(1);
                            parser_step = Bresser3chDecoderStepV0SaveDuration;
                        } else
                            parser_step = Bresser3chDecoderStepReset;
                    } else
                        parser_step = Bresser3chDecoderStepReset;
                } else
                    parser_step = Bresser3chDecoderStepReset;
                break;

            case Bresser3chDecoderStepV0TailCheckDuration:
                if (!level) {
                    if (duration >= te_long) {
                        if (decode_count_bit == BRESSER_V0_DATA_AND_TAIL &&
                            ws_protocol_bresser_3ch_check_v0()) {
                            decode_count_bit = BRESSER_V0_DATA;
                            sensorType = FPW_Bresser3CH;
                            // ws_protocol_bresser_3ch_extract_data_v0();
                            if (callback) {
                                callback(this);
                            }
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        parser_step = Bresser3chDecoderStepReset;
                    } else if (
                        decode_count_bit < BRESSER_V0_DATA_AND_TAIL &&
                        DURATION_DIFF(te_last, te_short) < te_delta &&
                        DURATION_DIFF(duration, te_short * 2) < te_delta) {
                        decode_count_bit++;
                        parser_step = Bresser3chDecoderStepV0SaveDuration;
                    } else
                        parser_step = Bresser3chDecoderStepReset;
                } else
                    parser_step = Bresser3chDecoderStepReset;
                break;

            case Bresser3chDecoderStepV1PreambleDn:
                if ((!level) && DURATION_DIFF(duration, te_short_v1 * 3) < te_delta) {
                    if (DURATION_DIFF(te_last, te_short_v1 * 12) < te_delta * 2) {
                        // End of sync after 4*750 (12*250) high values, start reading the message
                        parser_step = Bresser3chDecoderStepV1SaveDuration;
                    } else {
                        parser_step = Bresser3chDecoderStepV1PreambleUp;
                    }
                } else {
                    parser_step = Bresser3chDecoderStepReset;
                }
                break;

            case Bresser3chDecoderStepV1PreambleUp:
                if (level && DURATION_DIFF(duration, te_short_v1 * 3) < te_delta) {
                    te_last = te_last + duration;
                    parser_step = Bresser3chDecoderStepV1PreambleDn;
                } else {
                    parser_step = Bresser3chDecoderStepReset;
                }
                break;

            case Bresser3chDecoderStepV1SaveDuration:
                if (decode_count_bit == BRESSER_V1_DATA) {
                    if (ws_protocol_bresser_3ch_check_v1()) {
                        // ws_protocol_bresser_3ch_extract_data_v1(&instance->generic);
                        sensorType = FPW_Bresser3CH_V1;
                        if (callback) {
                            callback(this);
                        }
                    }
                    decode_data = 0;
                    decode_count_bit = 0;
                    parser_step = Bresser3chDecoderStepReset;
                } else if (level) {
                    te_last = duration;
                    parser_step = Bresser3chDecoderStepV1CheckDuration;
                } else {
                    parser_step = Bresser3chDecoderStepReset;
                }
                break;

            case Bresser3chDecoderStepV1CheckDuration:
                if (!level) {
                    if (DURATION_DIFF(te_last, te_short_v1) < te_delta && DURATION_DIFF(duration, te_long_v1) < te_delta) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = Bresser3chDecoderStepV1SaveDuration;
                    } else if (
                        DURATION_DIFF(te_last, te_long_v1) < te_delta && DURATION_DIFF(duration, te_short_v1) < te_delta) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = Bresser3chDecoderStepV1SaveDuration;
                    } else
                        parser_step = Bresser3chDecoderStepReset;
                } else
                    parser_step = Bresser3chDecoderStepReset;
                break;
        }
    }

   protected:
    uint32_t te_short = 475;
    uint32_t te_long = 3900;
    uint32_t te_delta = 150;

    uint32_t te_short_v1 = 250;
    uint32_t te_long_v1 = 500;

    bool ws_protocol_bresser_3ch_check_v0() {
        if (!decode_data) return false;
        // No CRC, so better sanity checks here
        if (((decode_data >> 8) & 0x0f) != 0x0f) return false;   // separator not 0xf
        if (((decode_data >> 28) & 0xff) == 0xff) return false;  // ID only ones?
        if (((decode_data >> 28) & 0xff) == 0x00) return false;  // ID only zeroes?
        if (((decode_data >> 25) & 0x0f) == 0x0f) return false;  // flags only ones?
        if (((decode_data >> 25) & 0x0f) == 0x00) return false;  // flags only zeroes?
        if (((decode_data >> 12) & 0x0fff) == 0x0fff)
            return false;  // temperature maxed out?
        if ((decode_data & 0xff) < 20)
            return false;  // humidity percentage less than 20?
        if ((decode_data & 0xff) > 95)
            return false;  // humidity percentage more than 95?
        return true;
    }
    bool ws_protocol_bresser_3ch_check_v1() {
        if (!decode_data) return false;

        uint8_t sum = (((decode_data >> 32) & 0xff) +
                       ((decode_data >> 24) & 0xff) +
                       ((decode_data >> 16) & 0xff) +
                       ((decode_data >> 8) & 0xff)) &
                      0xff;

        return (decode_data & 0xff) == sum;
    }
};

#endif
