
#ifndef __FPROTO_MARANTEC24_H__
#define __FPROTO_MARANTEC24_H__

#include "subghzdbase.hpp"

typedef enum {
    Marantec24DecoderStepReset = 0,
    Marantec24DecoderStepSaveDuration,
    Marantec24DecoderStepCheckDuration,
} Marantec24DecoderStep;

class FProtoSubGhzDMarantec24 : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDMarantec24() {
        sensorType = FPS_MARANTEC24;
        te_short = 800;
        te_long = 1600;
        te_delta = 200;
        min_count_bit_for_found = 24;
    }

    void feed(bool level, uint32_t duration) {
        // Key samples
        // 101011000000010111001000 = AC05C8
        // 101011000000010111000100 = AC05C4
        // 101011000000010111001100 = AC05CC
        // 101011000000010111000000 = AC05C0

        switch (parser_step) {
            case Marantec24DecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_long * 9) < te_delta * 4)) {
                    // Found GAP
                    decode_data = 0;
                    decode_count_bit = 0;
                    parser_step = Marantec24DecoderStepSaveDuration;
                }
                break;
            case Marantec24DecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = Marantec24DecoderStepCheckDuration;
                } else {
                    parser_step = Marantec24DecoderStepReset;
                }
                break;
            case Marantec24DecoderStepCheckDuration:
                if (!level) {
                    // Bit 0 is long and short x2 timing = 1600us HIGH (te_last) and 2400us LOW
                    if ((DURATION_DIFF(te_last, te_long) < te_delta) &&
                        (DURATION_DIFF(duration, te_short * 3) < te_delta)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = Marantec24DecoderStepSaveDuration;
                        // Bit 1 is short and long x2 timing = 800us HIGH (te_last) and 3200us LOW
                    } else if (
                        (DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long * 2) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = Marantec24DecoderStepSaveDuration;
                    } else if (
                        // End of the key
                        DURATION_DIFF(duration, te_long * 9) < te_delta * 4) {
                        // Found next GAP and add bit 0 or 1 (only bit 0 was found on the remotes)
                        if ((DURATION_DIFF(te_last, te_long) < te_delta) &&
                            (DURATION_DIFF(duration, te_long * 9) < te_delta * 4)) {
                            subghz_protocol_blocks_add_bit(0);
                        }
                        if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                            (DURATION_DIFF(duration, te_long * 9) < te_delta * 4)) {
                            subghz_protocol_blocks_add_bit(1);
                        }
                        // If got 24 bits key reading is finished
                        if (decode_count_bit == min_count_bit_for_found) {
                            data_count_bit = decode_count_bit;
                            if (callback) callback(this);
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        parser_step = Marantec24DecoderStepReset;
                    } else {
                        parser_step = Marantec24DecoderStepReset;
                    }
                } else {
                    parser_step = Marantec24DecoderStepReset;
                }
                break;
        }
    }

   protected:
};

#endif
