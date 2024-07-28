
#ifndef __FPROTO_KEELOQ_H__
#define __FPROTO_KEELOQ_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    KeeloqDecoderStepReset = 0,
    KeeloqDecoderStepCheckPreambula,
    KeeloqDecoderStepSaveDuration,
    KeeloqDecoderStepCheckDuration,
} KeeloqDecoderStep;

class FProtoSubGhzDKeeLoq : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDKeeLoq() {
        sensorType = FPS_KEELOQ;
        te_short = 400;
        te_long = 800;
        te_delta = 140;
        min_count_bit_for_found = 64;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case KeeloqDecoderStepReset:
                if ((level) && DURATION_DIFF(duration, te_short) < te_delta) {
                    parser_step = KeeloqDecoderStepCheckPreambula;
                    header_count++;
                }
                break;
            case KeeloqDecoderStepCheckPreambula:
                if ((!level) && (DURATION_DIFF(duration, te_short) < te_delta)) {
                    parser_step = KeeloqDecoderStepReset;
                    break;
                }
                if ((header_count > 2) && (DURATION_DIFF(duration, te_short * 10) < te_delta * 10)) {
                    // Found header
                    parser_step = KeeloqDecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                } else {
                    parser_step = KeeloqDecoderStepReset;
                    header_count = 0;
                }
                break;
            case KeeloqDecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = KeeloqDecoderStepCheckDuration;
                }
                break;
            case KeeloqDecoderStepCheckDuration:
                if (!level) {
                    if (duration >= ((uint32_t)te_short * 2 + te_delta)) {
                        // Found end TX
                        parser_step = KeeloqDecoderStepReset;
                        if ((decode_count_bit >= min_count_bit_for_found) &&
                            (decode_count_bit <= min_count_bit_for_found + 2)) {
                            if (data != decode_data) {
                                data = decode_data;
                                data_count_bit = min_count_bit_for_found;
                                if (callback) callback(this);
                            }
                            decode_data = 0;
                            decode_count_bit = 0;
                            header_count = 0;
                        }
                        break;
                    } else if (
                        (DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta * 2)) {
                        if (decode_count_bit < min_count_bit_for_found) {
                            subghz_protocol_blocks_add_bit(1);
                        } else {
                            decode_count_bit++;
                        }
                        parser_step = KeeloqDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta * 2) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        if (decode_count_bit < min_count_bit_for_found) {
                            subghz_protocol_blocks_add_bit(0);
                        } else {
                            decode_count_bit++;
                        }
                        parser_step = KeeloqDecoderStepSaveDuration;
                    } else {
                        parser_step = KeeloqDecoderStepReset;
                        header_count = 0;
                    }
                } else {
                    parser_step = KeeloqDecoderStepReset;
                    header_count = 0;
                }
                break;
        }
    }
};

#endif
