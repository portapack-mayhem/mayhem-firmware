
#ifndef __FPROTO_KIA_H__
#define __FPROTO_KIA_H__

#include "subghzdbase.hpp"

typedef enum {
    KIADecoderStepReset = 0,
    KIADecoderStepCheckPreambula,
    KIADecoderStepSaveDuration,
    KIADecoderStepCheckDuration,
} KIADecoderStep;

class FProtoSubGhzDKia : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDKia() {
        sensorType = FPS_KIA;
        modulation = FPM_FM;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case KIADecoderStepReset:
                if ((level) && (DURATION_DIFF(duration, te_short) < te_delta)) {
                    parser_step = KIADecoderStepCheckPreambula;
                    te_last = duration;
                    header_count = 0;
                }
                break;
            case KIADecoderStepCheckPreambula:
                if (level) {
                    if ((DURATION_DIFF(duration, te_short) < te_delta) ||
                        (DURATION_DIFF(duration, te_long) < te_delta)) {
                        te_last = duration;
                    } else {
                        parser_step = KIADecoderStepReset;
                    }
                } else if (
                    (DURATION_DIFF(duration, te_short) < te_delta) &&
                    (DURATION_DIFF(te_last, te_short) < te_delta)) {
                    // Found header
                    header_count++;
                    break;
                } else if (
                    (DURATION_DIFF(duration, te_long) < te_delta) &&
                    (DURATION_DIFF(te_last, te_long) < te_delta)) {
                    // Found start bit
                    if (header_count > 15) {
                        parser_step = KIADecoderStepSaveDuration;
                        decode_data = 0;
                        decode_count_bit = 1;
                        subghz_protocol_blocks_add_bit(1);
                    } else {
                        parser_step = KIADecoderStepReset;
                    }
                } else {
                    parser_step = KIADecoderStepReset;
                }
                break;
            case KIADecoderStepSaveDuration:
                if (level) {
                    if (duration >=
                        (te_long + te_delta * 2UL)) {
                        // Found stop bit
                        parser_step = KIADecoderStepReset;
                        if (decode_count_bit == min_count_bit_for_found) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            // controller
                            serial = (uint32_t)((data >> 12) & 0x0FFFFFFF);
                            btn = (data >> 8) & 0x0F;
                            cnt = (data >> 40) & 0xFFFF;
                            if (callback) callback(this);
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        break;
                    } else {
                        te_last = duration;
                        parser_step = KIADecoderStepCheckDuration;
                    }

                } else {
                    parser_step = KIADecoderStepReset;
                }
                break;
            case KIADecoderStepCheckDuration:
                if (!level) {
                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = KIADecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = KIADecoderStepSaveDuration;
                    } else {
                        parser_step = KIADecoderStepReset;
                    }
                } else {
                    parser_step = KIADecoderStepReset;
                }
                break;
        }
    }

   protected:
    uint32_t te_short = 250;
    uint32_t te_long = 500;
    uint32_t te_delta = 100;
    uint32_t min_count_bit_for_found = 61;
};

#endif
