#pragma once
#include "subcarbase.hpp"
#include <cstring>

typedef enum {
    KiaV1DecoderStepReset = 0,
    KiaV1DecoderStepCheckPreamble,
    KiaV1DecoderStepDecodeData,
} KiaV1DecoderStep;

class FProtoSubCarKiaV1 : public FProtoSubCarBase {
   public:
    FProtoSubCarKiaV1() {
        sensorType = FPC_KIAV1;
        te_short = 800;
        te_long = 1600;
        te_delta = 200;
        min_count_bit_for_found = 57;
    }

    void feed(bool level, uint32_t duration) {
        ManchesterEvent event = ManchesterEventReset;

        switch (parser_step) {
            case KiaV1DecoderStepReset:
                if ((level) && (DURATION_DIFF(duration, te_long) < te_delta)) {
                    parser_step = KiaV1DecoderStepCheckPreamble;
                    te_last = duration;
                    header_count = 0;
                    decode_data = 0;
                    decode_count_bit = 0;
                    FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventReset, &manchester_saved_state, NULL);
                }
                break;

            case KiaV1DecoderStepCheckPreamble:
                if (!level) {
                    if ((DURATION_DIFF(duration, te_long) < te_delta) && (DURATION_DIFF(te_last, te_long) < te_delta)) {
                        header_count++;
                        te_last = duration;
                    } else {
                        parser_step = KiaV1DecoderStepReset;
                    }
                }
                if (header_count > 70) {
                    if ((!level) && (DURATION_DIFF(duration, te_short) < te_delta) && (DURATION_DIFF(te_last, te_long) < te_delta)) {
                        decode_count_bit = 1;
                        subghz_protocol_blocks_add_bit(1);
                        header_count = 0;
                        parser_step = KiaV1DecoderStepDecodeData;
                    }
                }
                break;

            case KiaV1DecoderStepDecodeData:
                if ((DURATION_DIFF(duration, te_short) < te_delta)) {
                    event = level ? ManchesterEventShortLow : ManchesterEventShortHigh;
                } else if ((DURATION_DIFF(duration, te_long) <
                            te_delta)) {
                    event = level ? ManchesterEventLongLow : ManchesterEventLongHigh;
                }

                if (event != ManchesterEventReset) {
                    bool data;
                    bool data_ok = FProtoGeneral::manchester_advance(manchester_saved_state, event, &manchester_saved_state, &data);
                    if (data_ok) {
                        decode_data = (decode_data << 1) | data;
                        decode_count_bit++;
                    }
                }

                if (decode_count_bit == min_count_bit_for_found) {
                    // instance->generic.data = decode_data;
                    data_count_bit = decode_count_bit;
                    if (callback)
                        callback(this);

                    decode_data = 0;
                    decode_count_bit = 0;
                    parser_step = KiaV1DecoderStepReset;
                }
                break;
        }
    }

    uint8_t header_count = 0;
    ManchesterState manchester_saved_state = ManchesterStateStart1;
};
