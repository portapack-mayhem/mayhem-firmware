
#ifndef __FPROTO_ANSONIC_H__
#define __FPROTO_ANSONIC_H__

#include "subghzdbase.hpp"

#define ANSONICDIP_PATTERN "%c%c%c%c%c%c%c%c%c%c"
#define ANSONICCNT_TO_DIP(dip)                                                              \
    (dip & 0x0800 ? '1' : '0'), (dip & 0x0400 ? '1' : '0'), (dip & 0x0200 ? '1' : '0'),     \
        (dip & 0x0100 ? '1' : '0'), (dip & 0x0080 ? '1' : '0'), (dip & 0x0040 ? '1' : '0'), \
        (dip & 0x0020 ? '1' : '0'), (dip & 0x0010 ? '1' : '0'), (dip & 0x0001 ? '1' : '0'), \
        (dip & 0x0008 ? '1' : '0')

typedef enum {
    AnsonicDecoderStepReset = 0,
    AnsonicDecoderStepFoundStartBit,
    AnsonicDecoderStepSaveDuration,
    AnsonicDecoderStepCheckDuration,
} AnsonicDecoderStep;

class FProtoSubGhzDAnsonic : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDAnsonic() {
        sensorType = FPS_ANSONIC;
        modulation = FPM_FM;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case AnsonicDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 35) < te_delta * 35)) {
                    // Found header Ansonic
                    parser_step = AnsonicDecoderStepFoundStartBit;
                }
                break;
            case AnsonicDecoderStepFoundStartBit:
                if (!level) {
                    break;
                } else if (
                    DURATION_DIFF(duration, te_short) < te_delta) {
                    // Found start bit Ansonic
                    parser_step = AnsonicDecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                } else {
                    parser_step = AnsonicDecoderStepReset;
                }
                break;
            case AnsonicDecoderStepSaveDuration:
                if (!level) {  // save interval
                    if (duration >= (te_short * 4)) {
                        parser_step = AnsonicDecoderStepFoundStartBit;
                        if (decode_count_bit >=
                            min_count_bit_for_found) {
                            serial = 0x0;
                            btn = 0x0;
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            if (callback) callback(this);
                        }
                        break;
                    }
                    te_last = duration;
                    parser_step = AnsonicDecoderStepCheckDuration;
                } else {
                    parser_step = AnsonicDecoderStepReset;
                }
                break;
            case AnsonicDecoderStepCheckDuration:
                if (level) {
                    if ((DURATION_DIFF(te_last, te_short) <
                         te_delta) &&
                        (DURATION_DIFF(duration, te_long) <
                         te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = AnsonicDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) <
                         te_delta) &&
                        (DURATION_DIFF(duration, te_short) <
                         te_delta)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = AnsonicDecoderStepSaveDuration;
                    } else
                        parser_step = AnsonicDecoderStepReset;
                } else {
                    parser_step = AnsonicDecoderStepReset;
                }
                break;
        }
    }
    void subghz_protocol_ansonic_check_remote_controller() {
        /*
         *        12345678(10) k   9
         * AAA => 10101010 1   01  0
         *
         * 1...10 - DIP
         * k- KEY
         */
        cnt = data & 0xFFF;
        btn = ((data >> 1) & 0x3);
    }
    void get_string(char* output, size_t outSize) {
        subghz_protocol_ansonic_check_remote_controller();
        snprintf(
            output, outSize,
            "%dbit\r\n"
            "Key:%03lX\r\n"
            "Btn:%X\r\n"
            "DIP:" ANSONICDIP_PATTERN "\r\n",
            data_count_bit,
            (uint32_t)(data & 0xFFFFFFFF),
            btn,
            ANSONICCNT_TO_DIP(cnt));
    }

   protected:
    uint32_t te_short = 555;
    uint32_t te_long = 1111;
    uint32_t te_delta = 120;
    uint32_t min_count_bit_for_found = 12;

    uint32_t crc = 0;
};

#endif
