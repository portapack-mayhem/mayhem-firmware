#pragma once
#include "subcarbase.hpp"

#define SUZUKI_GAP_TIME 2000
#define SUZUKI_GAP_DELTA 400

typedef enum {
    SuzukiDecoderStepReset = 0,
    SuzukiDecoderStepCountPreamble = 1,
    SuzukiDecoderStepDecodeData = 2,
} SuzukiDecoderStep;

class FProtoSubCarSuzuki : public FProtoSubCarBase {
   public:
    FProtoSubCarSuzuki() {
        sensorType = FPC_SUZUKI;
        te_short = 250;
        te_long = 500;
        te_delta = 110;
        min_count_bit_for_found = 64;
    }
    void suzuki_add_bit(uint32_t bit) {
        decode_data = (decode_data << 1) | bit;
        decode_count_bit++;
    }
    void subghz_protocol_decoder_suzuki_reset() {
        parser_step = SuzukiDecoderStepReset;
        header_count = 0;
        data_count_bit = 0;
        decode_data = 0;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case SuzukiDecoderStepReset:
                // Wait for HIGH pulse (~250µs) to start preamble
                if (!level) {
                    return;
                }
                if (DURATION_DIFF(duration, te_short) > te_delta) {
                    return;
                }

                decode_data = 0;
                decode_count_bit = 0;
                parser_step = SuzukiDecoderStepCountPreamble;
                header_count = 0;
                break;

            case SuzukiDecoderStepCountPreamble:
                if (level) {
                    // HIGH pulse

                    if (header_count >= 300) {
                        if (DURATION_DIFF(duration, te_long) <= te_delta) {
                            parser_step = SuzukiDecoderStepDecodeData;
                            suzuki_add_bit(1);
                        }
                    }
                } else {
                    if (DURATION_DIFF(duration, te_short) <= te_delta) {
                        te_last = duration;
                        header_count++;
                    } else {
                        parser_step = SuzukiDecoderStepReset;
                    }
                }
                break;

            case SuzukiDecoderStepDecodeData:

                if (level) {
                    // HIGH pulse - determines bit value
                    if (duration < te_long) {
                        uint32_t diff_long = 500 - duration;
                        if (diff_long > 99) {
                            uint32_t diff_short;
                            if (duration < 250) {
                                diff_short = 250 - duration;
                            } else {
                                diff_short = duration - 250;
                            }

                            if (diff_short <= 99) {
                                suzuki_add_bit(0);
                            }
                        } else {
                            suzuki_add_bit(1);
                        }
                    } else {
                        uint32_t diff_long = duration - 500;
                        if (diff_long <= 99) {
                            suzuki_add_bit(1);
                        }
                    }
                } else {
                    // LOW pulse - check for gap (end of transmission)
                    uint32_t diff_gap;
                    if (duration < SUZUKI_GAP_TIME) {
                        diff_gap = SUZUKI_GAP_TIME - duration;
                    } else {
                        diff_gap = duration - SUZUKI_GAP_TIME;
                    }

                    if (diff_gap <= SUZUKI_GAP_DELTA) {
                        if (decode_count_bit == 64) {
                            // instance->generic.data = decode_data;
                            data_count_bit = 64;

                            /*uint64_t data = instance->generic.data;
                            uint32_t data_high = (uint32_t)(data >> 32);
                            uint32_t data_low = (uint32_t)data;

                            instance->generic.serial = ((data_high & 0xFFF) << 16) | (data_low >> 16);

                            instance->generic.btn = (data_low >> 12) & 0xF;
                            instance->generic.cnt = (data_high << 4) >> 16;
*/
                            if (callback) {
                                callback(this);
                            }
                        }

                        decode_data = 0;
                        decode_count_bit = 0;
                        parser_step = SuzukiDecoderStepReset;
                    }
                }
                break;
        }
    }

    uint16_t header_count = 0;
    uint8_t data_count_bit = 0;
};
