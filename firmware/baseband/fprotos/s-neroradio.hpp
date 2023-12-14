
#ifndef __FPROTO_NERORADIO_H__
#define __FPROTO_NERORADIO_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    NeroRadioDecoderStepReset = 0,
    NeroRadioDecoderStepCheckPreambula,
    NeroRadioDecoderStepSaveDuration,
    NeroRadioDecoderStepCheckDuration,
} NeroRadioDecoderStep;

class FProtoSubGhzDNeroRadio : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDNeroRadio() {
        sensorType = FPS_NERORADIO;
        te_short = 200;
        te_long = 400;
        te_delta = 80;
        min_count_bit_for_found = 56;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case NeroRadioDecoderStepReset:
                if ((level) && (DURATION_DIFF(duration, te_short) < te_delta)) {
                    parser_step = NeroRadioDecoderStepCheckPreambula;
                    te_last = duration;
                    header_count = 0;
                }
                break;
            case NeroRadioDecoderStepCheckPreambula:
                if (level) {
                    if ((DURATION_DIFF(duration, te_short) < te_delta) ||
                        (DURATION_DIFF(duration, te_short * 4) < te_delta)) {
                        te_last = duration;
                    } else {
                        parser_step = NeroRadioDecoderStepReset;
                    }
                } else if (
                    DURATION_DIFF(duration, te_short) < te_delta) {
                    if (DURATION_DIFF(te_last, te_short) < te_delta) {
                        // Found header
                        header_count++;
                        break;
                    } else if (
                        DURATION_DIFF(te_last, te_short * 4) < te_delta) {
                        // Found start bit
                        if (header_count > 40) {
                            parser_step = NeroRadioDecoderStepSaveDuration;
                            decode_data = 0;
                            decode_count_bit = 0;
                        } else {
                            parser_step = NeroRadioDecoderStepReset;
                        }
                    } else {
                        parser_step = NeroRadioDecoderStepReset;
                    }
                } else {
                    parser_step = NeroRadioDecoderStepReset;
                }
                break;
            case NeroRadioDecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = NeroRadioDecoderStepCheckDuration;
                } else {
                    parser_step = NeroRadioDecoderStepReset;
                }
                break;
            case NeroRadioDecoderStepCheckDuration:
                if (!level) {
                    if (duration >= ((uint32_t)1250)) {
                        // Found stop bit
                        if (DURATION_DIFF(te_last, te_short) < te_delta) {
                            subghz_protocol_blocks_add_bit(0);
                        } else if (
                            DURATION_DIFF(te_last, te_long) < te_delta) {
                            subghz_protocol_blocks_add_bit(1);
                        }
                        parser_step = NeroRadioDecoderStepReset;
                        if ((decode_count_bit == min_count_bit_for_found) ||
                            (decode_count_bit == min_count_bit_for_found + 1)) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;

                            if (callback) callback(this);
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        parser_step = NeroRadioDecoderStepReset;  //-V1048
                        break;
                    } else if (
                        (DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = NeroRadioDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = NeroRadioDecoderStepSaveDuration;
                    } else {
                        parser_step = NeroRadioDecoderStepReset;
                    }
                } else {
                    parser_step = NeroRadioDecoderStepReset;
                }
                break;
        }
    }
};

#endif
