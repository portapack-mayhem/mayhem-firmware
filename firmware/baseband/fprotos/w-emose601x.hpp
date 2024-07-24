
#ifndef __FPROTO_EmosE601x_H__
#define __FPROTO_EmosE601x_H__

#include "weatherbase.hpp"

#define EmosE601xMAGIC_HEADER 0xaaa583

typedef enum {
    EmosE601xDecoderStepReset = 0,
    EmosE601xDecoderStepCheckPreamble,
    EmosE601xDecoderStepSaveDuration,
    EmosE601xDecoderStepCheckDuration,
} EmosE601xDecoderStep;

class FProtoWeatherEmosE601x : public FProtoWeatherBase {
   public:
    FProtoWeatherEmosE601x() {
        sensorType = FPW_EmosE601x;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case EmosE601xDecoderStepReset:
                if ((level) && (DURATION_DIFF(duration, te_short * 7) <
                                te_delta * 2)) {
                    parser_step = EmosE601xDecoderStepCheckPreamble;
                    te_last = duration;
                }
                break;

            case EmosE601xDecoderStepCheckPreamble:
                if (level) {
                    te_last = duration;
                } else {
                    if ((DURATION_DIFF(te_last, te_short * 7) <
                         te_delta * 2) &&
                        (DURATION_DIFF(duration, te_short) <
                         te_delta)) {
                        // Found preamble
                        parser_step = EmosE601xDecoderStepSaveDuration;
                        decode_data = 0;
                        decode_count_bit = 0;
                    } else {
                        parser_step = EmosE601xDecoderStepReset;
                    }
                }
                break;

            case EmosE601xDecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = EmosE601xDecoderStepCheckDuration;
                } else {
                    parser_step = EmosE601xDecoderStepReset;
                }
                break;

            case EmosE601xDecoderStepCheckDuration:
                if (!level) {
                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta)) {
                        subghz_protocol_blocks_add_to_128_bit(0, &upper_decode_data);
                        parser_step = EmosE601xDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_to_128_bit(1, &upper_decode_data);
                        parser_step = EmosE601xDecoderStepSaveDuration;
                    } else {
                        parser_step = EmosE601xDecoderStepReset;
                        break;
                    }

                    /* Bail out if the header doesn't match */
                    if (decode_count_bit == min_count_bit_for_found) {
                        if (decode_data != EmosE601xMAGIC_HEADER) {
                            parser_step = EmosE601xDecoderStepReset;
                            break;
                        }
                    }

                    if (decode_count_bit == 120) {
                        data_count_bit = decode_count_bit;
                        if (ws_protocol_emose601x_check()) {
                            ws_protocol_emose601x_extract_data();
                            if (callback) callback(this);
                        }
                        break;
                    }
                } else {
                    parser_step = EmosE601xDecoderStepReset;
                }
                break;
        }
    }

   protected:
    uint32_t te_short = 260;
    uint32_t te_long = 800;
    uint32_t te_delta = 100;
    uint32_t min_count_bit_for_found = 24;

    uint64_t upper_decode_data = 0;
    void subghz_protocol_blocks_add_to_128_bit(uint8_t bit, uint64_t* head_64_bit) {
        if (++decode_count_bit > 64) {
            (*head_64_bit) = ((*head_64_bit) << 1) | (decode_data >> 63);
        }
        decode_data = decode_data << 1 | bit;
    }
    bool ws_protocol_emose601x_check() {
        uint8_t msg[] = {
            static_cast<uint8_t>(upper_decode_data >> 48),
            static_cast<uint8_t>(upper_decode_data >> 40),
            static_cast<uint8_t>(upper_decode_data >> 32),
            static_cast<uint8_t>(upper_decode_data >> 24),
            static_cast<uint8_t>(upper_decode_data >> 16),
            static_cast<uint8_t>(upper_decode_data >> 8),
            static_cast<uint8_t>(upper_decode_data),
            static_cast<uint8_t>(decode_data >> 56),
            static_cast<uint8_t>(decode_data >> 48),
            static_cast<uint8_t>(decode_data >> 40),
            static_cast<uint8_t>(decode_data >> 32),
            static_cast<uint8_t>(decode_data >> 24),
            static_cast<uint8_t>(decode_data >> 16)};

        uint8_t sum = FProtoGeneral::subghz_protocol_blocks_add_bytes(msg, 13);
        return (sum == ((decode_data >> 8) & 0xff));
    }

    void ws_protocol_emose601x_extract_data() {
        id = (upper_decode_data >> 24) & 0xff;
        battery_low = (decode_data >> 10) & 1;
        int16_t temp = (decode_data >> 40) & 0xfff;
        /* Handle signed data */
        if (temp & 0x800) {
            temp |= 0xf000;
        }
        temp = (float)temp / 10.0;
        humidity = (decode_data >> 32) & 0xff;
        channel = (decode_data >> 52) & 0x03;
        data = (decode_data >> 16);
    }
};

#endif
