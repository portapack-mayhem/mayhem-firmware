
#ifndef __FPROTO_KINGGATES_STYLO_4K_H__
#define __FPROTO_KINGGATES_STYLO_4K_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    KingGates_stylo_4kDecoderStepReset = 0,
    KingGates_stylo_4kDecoderStepCheckPreambula,
    KingGates_stylo_4kDecoderStepCheckStartBit,
    KingGates_stylo_4kDecoderStepSaveDuration,
    KingGates_stylo_4kDecoderStepCheckDuration,
} KingGates_stylo_4kDecoderStep;

class FProtoSubGhzDKinggatesStylo4K : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDKinggatesStylo4K() {
        sensorType = FPS_KINGGATESSTYLO4K;
        te_short = 400;
        te_long = 1100;
        te_delta = 140;
        min_count_bit_for_found = 89;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case KingGates_stylo_4kDecoderStepReset:
                if ((level) && DURATION_DIFF(duration, te_short) < te_delta) {
                    parser_step = KingGates_stylo_4kDecoderStepCheckPreambula;
                    header_count++;
                }
                break;
            case KingGates_stylo_4kDecoderStepCheckPreambula:
                if ((!level) &&
                    (DURATION_DIFF(duration, te_short) < te_delta)) {
                    parser_step = KingGates_stylo_4kDecoderStepReset;
                    break;
                }
                if ((header_count > 2) &&
                    (DURATION_DIFF(duration, te_long * 2) < te_delta * 2)) {
                    // Found header
                    parser_step = KingGates_stylo_4kDecoderStepCheckStartBit;
                } else {
                    parser_step = KingGates_stylo_4kDecoderStepReset;
                    header_count = 0;
                }
                break;
            case KingGates_stylo_4kDecoderStepCheckStartBit:
                if ((level) &&
                    DURATION_DIFF(duration, te_short * 2) < te_delta * 2) {
                    parser_step = KingGates_stylo_4kDecoderStepSaveDuration;
                    decode_data = 0;
                    data_2 = 0;
                    decode_count_bit = 0;
                    header_count = 0;
                }
                break;
            case KingGates_stylo_4kDecoderStepSaveDuration:
                if (!level) {
                    if (duration >= ((uint32_t)te_long * 3)) {
                        if (decode_count_bit ==
                            min_count_bit_for_found) {
                            data = data_2;
                            data_2 = decode_data;  // TODO DATA2
                            data_count_bit = decode_count_bit;
                            if (callback) callback(this);
                        }

                        parser_step = KingGates_stylo_4kDecoderStepReset;
                        decode_data = 0;
                        data_2 = 0;
                        decode_count_bit = 0;
                        header_count = 0;
                        break;
                    } else {
                        te_last = duration;
                        parser_step = KingGates_stylo_4kDecoderStepCheckDuration;
                    }
                } else {
                    parser_step = KingGates_stylo_4kDecoderStepReset;
                    header_count = 0;
                }
                break;
            case KingGates_stylo_4kDecoderStepCheckDuration:
                if (level) {
                    if ((DURATION_DIFF(
                             te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta * 2)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = KingGates_stylo_4kDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(
                             te_last, te_long) <
                         te_delta * 2) &&
                        (DURATION_DIFF(duration, te_short) <
                         te_delta)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = KingGates_stylo_4kDecoderStepSaveDuration;
                    } else {
                        parser_step = KingGates_stylo_4kDecoderStepReset;
                        header_count = 0;
                    }
                    if (decode_count_bit == 53) {
                        data_2 = decode_data;
                        decode_data = 0;
                    }
                } else {
                    parser_step = KingGates_stylo_4kDecoderStepReset;
                    header_count = 0;
                }
                break;
        }
    }

   protected:
    uint64_t data_2 = 0;
};

#endif
