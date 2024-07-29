
#ifndef __FPROTO_LEGRAND_H__
#define __FPROTO_LEGRAND_H__

#include "subghzdbase.hpp"

typedef enum {
    LegrandDecoderStepReset = 0,
    LegrandDecoderStepFirstBit,
    LegrandDecoderStepSaveDuration,
    LegrandDecoderStepCheckDuration,
} LegrandDecoderStep;

class FProtoSubGhzDLegrand : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDLegrand() {
        sensorType = FPS_LEGRAND;
        te_short = 375;
        te_long = 1125;
        te_delta = 150;
        min_count_bit_for_found = 18;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case LegrandDecoderStepReset:
                if (!level && DURATION_DIFF(duration, te_short * 16) < te_delta * 8) {
                    parser_step = LegrandDecoderStepFirstBit;
                    decode_data = 0;
                    decode_count_bit = 0;
                    te = 0;
                }
                break;
            case LegrandDecoderStepFirstBit:
                if (level) {
                    if (DURATION_DIFF(duration, te_short) < te_delta) {
                        subghz_protocol_blocks_add_bit(0);
                        te += duration * 4;  // long low that is part of sync, then short high
                    }

                    if (DURATION_DIFF(duration, te_long) < te_delta * 3) {
                        subghz_protocol_blocks_add_bit(1);
                        te += duration / 3 * 4;  // short low that is part of sync, then long high
                    }

                    if (decode_count_bit > 0) {
                        // advance to the next step if either short or long is found
                        parser_step = LegrandDecoderStepSaveDuration;
                        break;
                    }
                }

                parser_step = LegrandDecoderStepReset;
                break;
            case LegrandDecoderStepSaveDuration:
                if (!level) {
                    te_last = duration;
                    te += duration;
                    parser_step = LegrandDecoderStepCheckDuration;
                    break;
                }

                parser_step = LegrandDecoderStepReset;
                break;
            case LegrandDecoderStepCheckDuration:
                if (level) {
                    uint8_t found = 0;

                    if (DURATION_DIFF(te_last, te_long) < te_delta * 3 && DURATION_DIFF(duration, te_short) < te_delta) {
                        found = 1;
                        subghz_protocol_blocks_add_bit(0);
                    }

                    if (DURATION_DIFF(te_last, te_short) < te_delta && DURATION_DIFF(duration, te_long) < te_delta * 3) {
                        found = 1;
                        subghz_protocol_blocks_add_bit(1);
                    }

                    if (found) {
                        te += duration;

                        if (decode_count_bit <
                            min_count_bit_for_found) {
                            parser_step = LegrandDecoderStepSaveDuration;
                            break;
                        }

                        // enough bits for a packet found, save it only if there was a previous packet
                        // with the same data
                        if (data && (data != decode_data)) {
                            te /= decode_count_bit * 4;

                            data = decode_data;
                            data_count_bit = decode_count_bit;

                            if (callback) {
                                callback(this);
                            }
                        }
                        // fallthrough to reset, the next bit is expected to be a sync
                        // it also takes care of resetting the decoder state
                    }
                }
                parser_step = LegrandDecoderStepReset;
                break;
        }
    }

   protected:
    uint32_t te = 0;
};

#endif
