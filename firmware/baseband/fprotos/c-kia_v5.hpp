#pragma once
#include "subcarbase.hpp"
#include <cstring>

typedef enum {
    KiaV5DecoderStepReset = 0,
    KiaV5DecoderStepCheckPreamble,
    KiaV5DecoderStepData,
} KiaV5DecoderStep;

class FProtoSubCarKiaV5 : public FProtoSubCarBase {
   public:
    FProtoSubCarKiaV5() {
        sensorType = FPC_KIAV5;
        te_short = 400;
        te_long = 800;
        te_delta = 150;
        min_count_bit_for_found = 64;
    }

    void kia_v5_add_bit(bool bit) {
        decode_data = (decode_data << 1) | (bit ? 1 : 0);
        decode_count_bit++;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case KiaV5DecoderStepReset:
                if ((level) && (DURATION_DIFF(duration, te_short) < te_delta)) {
                    parser_step = KiaV5DecoderStepCheckPreamble;
                    te_last = duration;
                    header_count = 1;
                    decode_count_bit = 0;
                    decode_data = 0;
                    FProtoGeneral::manchester_advance(manchester_state, ManchesterEventReset, &manchester_state, NULL);
                }
                break;

            case KiaV5DecoderStepCheckPreamble:
                if (level) {
                    if (DURATION_DIFF(duration, te_long) < te_delta) {
                        if (header_count > 40) {
                            parser_step = KiaV5DecoderStepData;
                            decode_count_bit = 0;
                            decode_data = 0;
                            decode_data2 = 0;
                            header_count = 0;
                        } else {
                            te_last = duration;
                        }
                    } else if (DURATION_DIFF(duration, te_short) < te_delta) {
                        te_last = duration;
                    } else {
                        parser_step = KiaV5DecoderStepReset;
                    }
                } else {
                    if ((DURATION_DIFF(duration, te_short) <
                         te_delta) &&
                        (DURATION_DIFF(te_last, te_short) <
                         te_delta)) {
                        header_count++;
                    } else if (
                        (DURATION_DIFF(duration, te_long) <
                         te_delta) &&
                        (DURATION_DIFF(te_last, te_short) <
                         te_delta)) {
                        header_count++;
                    } else if (
                        DURATION_DIFF(te_last, te_long) <
                        te_delta) {
                        header_count++;
                    } else {
                        parser_step = KiaV5DecoderStepReset;
                    }
                    te_last = duration;
                }
                break;

            case KiaV5DecoderStepData: {
                ManchesterEvent event;

                if (DURATION_DIFF(duration, te_short) < te_delta) {
                    event = level ? ManchesterEventShortHigh : ManchesterEventShortLow;
                } else if (DURATION_DIFF(duration, te_long) < te_delta) {
                    event = level ? ManchesterEventLongHigh : ManchesterEventLongLow;
                } else {
                    if (decode_count_bit >= min_count_bit_for_found) {
                        // instance->generic.data = instance->decode_data2;
                        data_count_bit = (decode_count_bit > 67) ? 67 : decode_count_bit;
                        if (callback)
                            callback(this);
                    }

                    parser_step = KiaV5DecoderStepReset;
                    break;
                }

                bool data_bit;
                if (decode_count_bit <= 66 && FProtoGeneral::manchester_advance(manchester_state, event, &manchester_state, &data_bit)) {
                    kia_v5_add_bit(data_bit);

                    if (decode_count_bit == 64) {
                        decode_data2 = decode_data;
                        decode_data = 0;
                    }
                }

                te_last = duration;
                break;
            }
        }
    }

    uint16_t header_count = 0;
    ManchesterState manchester_state = ManchesterStateMid1;
};
