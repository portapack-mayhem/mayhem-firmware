
#ifndef __FPROTO_SUPERROLLO_H__
#define __FPROTO_SUPERROLLO_H__

#include "subghzdbase.hpp"

// Superrollo GW60 (HCS361) 67-bit OOK decoder. Reports FPS_SUPERROLLO; SubGhzD
// treats FPS_SUPERROLLO like KeeLoq for decrypt/display (see ui_subghzd.cpp), so
// the "Superrollo" keystore key decrypts it while it stays identifiable in logs.

typedef enum : uint8_t {
    SuperrolloStepReset = 0,
    SuperrolloStepPreambleLow,
    SuperrolloStepSyncLow,
    SuperrolloStepSaveDuration,
    SuperrolloStepCheckDuration,
} SuperrolloDecoderStep;

class FProtoSubGhzDSuperrollo : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDSuperrollo() {
        sensorType = FPS_SUPERROLLO;
        te_short = 450;
        te_long = 900;
        te_delta = 200;
        min_count_bit_for_found = 64;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case SuperrolloStepReset:
                if (level) {
                    if (DURATION_DIFF(duration, te_short) < te_delta) {
                        header_count++;
                        parser_step = SuperrolloStepPreambleLow;
                    } else if (
                        (header_count >= 4) &&
                        (DURATION_DIFF(duration, te_short * 10) < te_delta * 10)) {
                        parser_step = SuperrolloStepSyncLow;
                    } else {
                        header_count = 0;
                    }
                } else {
                    header_count = 0;
                }
                break;

            case SuperrolloStepPreambleLow:
                if ((!level) && (DURATION_DIFF(duration, te_long) < te_delta)) {
                    parser_step = SuperrolloStepReset;
                } else {
                    parser_step = SuperrolloStepReset;
                    header_count = 0;
                }
                break;

            case SuperrolloStepSyncLow:
                if ((!level) && (DURATION_DIFF(duration, te_short * 10) < te_delta * 10)) {
                    parser_step = SuperrolloStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                } else {
                    parser_step = SuperrolloStepReset;
                    header_count = 0;
                }
                break;

            case SuperrolloStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = SuperrolloStepCheckDuration;
                } else if (duration >= te_short * 12) {
                    endFrame();
                }
                break;

            case SuperrolloStepCheckDuration:
                if (!level) {
                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta)) {
                        if (decode_count_bit < min_count_bit_for_found)
                            subghz_protocol_blocks_add_bit(1);
                        else
                            decode_count_bit++;
                        parser_step = SuperrolloStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        if (decode_count_bit < min_count_bit_for_found)
                            subghz_protocol_blocks_add_bit(0);
                        else
                            decode_count_bit++;
                        parser_step = SuperrolloStepSaveDuration;
                    } else if (duration >= te_short * 12) {
                        endFrame();
                    } else {
                        parser_step = SuperrolloStepReset;
                        header_count = 0;
                    }
                } else {
                    parser_step = SuperrolloStepReset;
                    header_count = 0;
                }
                break;
        }
    }

    uint16_t header_count = 0;

   private:
    void endFrame() {
        if ((decode_count_bit >= min_count_bit_for_found) &&
            (decode_count_bit <= min_count_bit_for_found + 3)) {
            data_count_bit = min_count_bit_for_found;
            if (callback) callback(this);
        }
        parser_step = SuperrolloStepReset;
        decode_data = 0;
        decode_count_bit = 0;
        header_count = 0;
    }
};

#endif
