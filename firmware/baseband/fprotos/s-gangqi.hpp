
#ifndef __FPROTO_GANGQI_H__
#define __FPROTO_GANGQI_H__

#include "subghzdbase.hpp"

typedef enum {
    GangQiDecoderStepReset = 0,
    GangQiDecoderStepSaveDuration,
    GangQiDecoderStepCheckDuration,
} GangQiDecoderStep;

class FProtoSubGhzDGangqi : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDGangqi() {
        sensorType = FPS_GANGQI;
        te_short = 500;
        te_long = 1200;
        te_delta = 200;
        min_count_bit_for_found = 34;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case GangQiDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_long * 2) < te_delta * 3)) {
                    // Found GAP
                    decode_data = 0;
                    decode_count_bit = 0;
                    parser_step = GangQiDecoderStepSaveDuration;
                }
                break;
            case GangQiDecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = GangQiDecoderStepCheckDuration;
                } else {
                    parser_step = GangQiDecoderStepReset;
                }
                break;
            case GangQiDecoderStepCheckDuration:
                if (!level) {
                    // Bit 0 is short and long timing
                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = GangQiDecoderStepSaveDuration;
                        // Bit 1 is long and short timing
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = GangQiDecoderStepSaveDuration;
                    } else if (
                        // End of the key
                        DURATION_DIFF(duration, te_short * 4) <
                        te_delta) {
                        // Found next GAP and add bit 0 or 1 (only bit 0 was found on the remotes)
                        if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                            (DURATION_DIFF(duration, te_short * 4) < te_delta)) {
                            subghz_protocol_blocks_add_bit(0);
                        }
                        if ((DURATION_DIFF(te_last, te_long) < te_delta) &&
                            (DURATION_DIFF(duration, te_short * 4) < te_delta)) {
                            subghz_protocol_blocks_add_bit(1);
                        }
                        // If got 34 bits key reading is finished
                        if (decode_count_bit == min_count_bit_for_found) {
                            data_count_bit = decode_count_bit;
                            if (callback) callback(this);
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        parser_step = GangQiDecoderStepReset;
                    } else {
                        parser_step = GangQiDecoderStepReset;
                    }
                } else {
                    parser_step = GangQiDecoderStepReset;
                }
                break;
        }
    }
};

#endif
