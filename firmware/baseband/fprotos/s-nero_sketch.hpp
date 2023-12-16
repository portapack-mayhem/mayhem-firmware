
#ifndef __FPROTO_NEROSKETCH_H__
#define __FPROTO_NEROSKETCH_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    NeroSketchDecoderStepReset = 0,
    NeroSketchDecoderStepCheckPreambula,
    NeroSketchDecoderStepSaveDuration,
    NeroSketchDecoderStepCheckDuration,
} NeroSketchDecoderStep;

class FProtoSubGhzDNeroSketch : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDNeroSketch() {
        sensorType = FPS_NERO_SKETCH;
        te_short = 330;
        te_long = 660;
        te_delta = 150;
        min_count_bit_for_found = 40;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case NeroSketchDecoderStepReset:
                if ((level) && (DURATION_DIFF(duration, te_short) < te_delta)) {
                    parser_step = NeroSketchDecoderStepCheckPreambula;
                    te_last = duration;
                    header_count = 0;
                }
                break;
            case NeroSketchDecoderStepCheckPreambula:
                if (level) {
                    if ((DURATION_DIFF(duration, te_short) < te_delta) ||
                        (DURATION_DIFF(duration, te_short * 4) < te_delta)) {
                        te_last = duration;
                    } else {
                        parser_step = NeroSketchDecoderStepReset;
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
                            parser_step = NeroSketchDecoderStepSaveDuration;
                            decode_data = 0;
                            decode_count_bit = 0;
                        } else {
                            parser_step = NeroSketchDecoderStepReset;
                        }
                    } else {
                        parser_step = NeroSketchDecoderStepReset;
                    }
                } else {
                    parser_step = NeroSketchDecoderStepReset;
                }
                break;
            case NeroSketchDecoderStepSaveDuration:
                if (level) {
                    if (duration >= (te_short * 2 + te_delta * 2)) {
                        // Found stop bit
                        parser_step = NeroSketchDecoderStepReset;
                        if (decode_count_bit == min_count_bit_for_found) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            if (callback) callback(this);
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        break;
                    } else {
                        te_last = duration;
                        parser_step = NeroSketchDecoderStepCheckDuration;
                    }

                } else {
                    parser_step = NeroSketchDecoderStepReset;
                }
                break;
            case NeroSketchDecoderStepCheckDuration:
                if (!level) {
                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = NeroSketchDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = NeroSketchDecoderStepSaveDuration;
                    } else {
                        parser_step = NeroSketchDecoderStepReset;
                    }
                } else {
                    parser_step = NeroSketchDecoderStepReset;
                }
                break;
        }
    }
};

#endif
