
#ifndef __FPROTO_HONEYWELLWDB_H__
#define __FPROTO_HONEYWELLWDB_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    Honeywell_WDBDecoderStepReset = 0,
    Honeywell_WDBDecoderStepFoundStartBit,
    Honeywell_WDBDecoderStepSaveDuration,
    Honeywell_WDBDecoderStepCheckDuration,
} Honeywell_WDBDecoderStep;

class FProtoSubGhzDHoneywellWdb : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDHoneywellWdb() {
        sensorType = FPS_HONEYWELLWDB;
        te_short = 160;
        te_long = 320;
        te_delta = 61;
        min_count_bit_for_found = 48;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case Honeywell_WDBDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 3) < te_delta)) {
                    // Found header Honeywell_WDB
                    decode_count_bit = 0;
                    decode_data = 0;
                    parser_step = Honeywell_WDBDecoderStepSaveDuration;
                }
                break;
            case Honeywell_WDBDecoderStepSaveDuration:
                if (level) {  // save interval
                    if (DURATION_DIFF(duration, te_short * 3) < te_delta) {
                        if ((decode_count_bit == min_count_bit_for_found) &&
                            ((decode_data & 0x01) == FProtoGeneral::subghz_protocol_blocks_get_parity(decode_data >> 1, min_count_bit_for_found - 1))) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            // controller has too much, should be done on ui side
                            if (callback) callback(this);
                        }
                        parser_step = Honeywell_WDBDecoderStepReset;
                        break;
                    }
                    te_last = duration;
                    parser_step = Honeywell_WDBDecoderStepCheckDuration;
                } else {
                    parser_step = Honeywell_WDBDecoderStepReset;
                }
                break;
            case Honeywell_WDBDecoderStepCheckDuration:
                if (!level) {
                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = Honeywell_WDBDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = Honeywell_WDBDecoderStepSaveDuration;
                    } else
                        parser_step = Honeywell_WDBDecoderStepReset;
                } else {
                    parser_step = Honeywell_WDBDecoderStepReset;
                }
                break;
        }
    }
};

#endif
