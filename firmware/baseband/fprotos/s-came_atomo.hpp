
#ifndef __FPROTO_CAMEATOMO_H__
#define __FPROTO_CAMEATOMO_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    CameAtomoDecoderStepReset = 0,
    CameAtomoDecoderStepDecoderData,
} CameAtomoDecoderStep;

class FProtoSubGhzDCameAtomo : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDCameAtomo() {
        sensorType = FPS_CAMEATOMO;
        te_short = 600;
        te_long = 1200;
        te_delta = 250;
        min_count_bit_for_found = 62;
    }

    void feed(bool level, uint32_t duration) {
        ManchesterEvent event = ManchesterEventReset;
        switch (parser_step) {
            case CameAtomoDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_long * 60) < te_delta * 40)) {
                    // Found header CAME
                    parser_step = CameAtomoDecoderStepDecoderData;
                    decode_data = 0;
                    decode_count_bit = 1;
                    FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventReset, &manchester_saved_state, NULL);
                    FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventShortLow, &manchester_saved_state, NULL);
                }
                break;
            case CameAtomoDecoderStepDecoderData:
                if (!level) {
                    if (DURATION_DIFF(duration, te_short) < te_delta) {
                        event = ManchesterEventShortLow;
                    } else if (
                        DURATION_DIFF(duration, te_long) < te_delta) {
                        event = ManchesterEventLongLow;
                    } else if (
                        duration >= ((uint32_t)te_long * 2 + te_delta)) {
                        if (decode_count_bit ==
                            min_count_bit_for_found) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            // controller
                            data ^= 0xFFFFFFFFFFFFFFFF;
                            data <<= 4;

                            uint8_t pack[8] = {};
                            pack[0] = (data >> 56);
                            pack[1] = ((data >> 48) & 0xFF);
                            pack[2] = ((data >> 40) & 0xFF);
                            pack[3] = ((data >> 32) & 0xFF);
                            pack[4] = ((data >> 24) & 0xFF);
                            pack[5] = ((data >> 16) & 0xFF);
                            pack[6] = ((data >> 8) & 0xFF);
                            pack[7] = (data & 0xFF);

                            atomo_decrypt(pack);

                            cnt = (uint16_t)pack[1] << 8 | pack[2];
                            serial = (uint32_t)(pack[3]) << 24 | pack[4] << 16 | pack[5] << 8 | pack[6];

                            uint8_t btn_decode = (pack[7] >> 4);
                            if (btn_decode == 0x0) {
                                btn = 0x1;
                            } else if (btn_decode == 0x2) {
                                btn = 0x2;
                            } else if (btn_decode == 0x4) {
                                btn = 0x3;
                            } else if (btn_decode == 0x6) {
                                btn = 0x4;
                            }

                            if (callback) callback(this);
                        }
                        decode_data = 0;
                        decode_count_bit = 1;
                        FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventReset, &manchester_saved_state, NULL);
                        FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventShortLow, &manchester_saved_state, NULL);
                    } else {
                        parser_step = CameAtomoDecoderStepReset;
                    }
                } else {
                    if (DURATION_DIFF(duration, te_short) < te_delta) {
                        event = ManchesterEventShortHigh;
                    } else if (
                        DURATION_DIFF(duration, te_long) < te_delta) {
                        event = ManchesterEventLongHigh;
                    } else {
                        parser_step = CameAtomoDecoderStepReset;
                    }
                }
                if (event != ManchesterEventReset) {
                    bool bit;
                    bool data_ok = FProtoGeneral::manchester_advance(manchester_saved_state, event, &manchester_saved_state, &bit);

                    if (data_ok) {
                        decode_data = (decode_data << 1) | !bit;
                        decode_count_bit++;
                    }
                }
                break;
        }
    }

   protected:
    ManchesterState manchester_saved_state = ManchesterStateMid1;

    void atomo_decrypt(uint8_t* buff) {
        buff[0] = (buff[0] ^ 5) & 0x7F;
        uint8_t tmpB = (-buff[0]) & 0x7F;

        uint8_t bitCnt = 8;
        while (bitCnt < 59) {
            if ((tmpB & 0x18) && (((tmpB / 8) & 3) != 3)) {
                tmpB = ((tmpB << 1) & 0xFF) | 1;
            } else {
                tmpB = (tmpB << 1) & 0xFF;
            }

            if (tmpB & 0x80) {
                buff[bitCnt / 8] ^= (0x80 >> (bitCnt & 7));
            }
            bitCnt++;
        }
    }
};

#endif
