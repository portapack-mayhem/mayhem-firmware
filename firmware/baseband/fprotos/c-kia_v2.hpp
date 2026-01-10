#pragma once
#include "subcarbase.hpp"
#include <cstring>

typedef enum {
    KiaV2DecoderStepReset = 0,
    KiaV2DecoderStepCheckPreamble,
    KiaV2DecoderStepCollectRawBits,
} KiaV2DecoderStep;

class FProtoSubCarKiaV2 : public FProtoSubCarBase {
   public:
    FProtoSubCarKiaV2() {
        sensorType = FPC_KIAV2;
        te_short = 500;
        te_long = 1000;
        te_delta = 160;
        min_count_bit_for_found = 53;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case KiaV2DecoderStepReset:
                if ((level) && (DURATION_DIFF(duration, te_long) < te_delta)) {
                    parser_step = KiaV2DecoderStepCheckPreamble;
                    te_last = duration;
                    header_count = 0;
                    FProtoGeneral::manchester_advance(manchester_state, ManchesterEventReset, &manchester_state, NULL);
                }
                break;
            case KiaV2DecoderStepCheckPreamble:
                if (level)  // HIGH pulse
                {
                    if (DURATION_DIFF(duration, te_long) < te_delta) {
                        te_last = duration;
                        header_count++;
                    } else if (
                        DURATION_DIFF(duration, te_short) < te_delta) {
                        if (header_count >= 100) {
                            header_count = 0;
                            decode_data = 0;
                            decode_count_bit = 1;
                            parser_step = KiaV2DecoderStepCollectRawBits;
                            subghz_protocol_blocks_add_bit(1);
                        } else {
                            te_last = duration;
                        }
                    } else {
                        parser_step = KiaV2DecoderStepReset;
                    }
                } else {
                    if (DURATION_DIFF(duration, te_long) < te_delta) {
                        header_count++;
                        te_last = duration;
                    } else if (
                        DURATION_DIFF(duration, te_short) < te_delta) {
                        te_last = duration;
                    } else {
                        parser_step = KiaV2DecoderStepReset;
                    }
                }
                break;

            case KiaV2DecoderStepCollectRawBits: {
                ManchesterEvent event;

                if (DURATION_DIFF(duration, te_short) < te_delta) {
                    event = level ? ManchesterEventShortLow : ManchesterEventShortHigh;
                } else if (
                    DURATION_DIFF(duration, te_long) < te_delta) {
                    event = level ? ManchesterEventLongLow : ManchesterEventLongHigh;
                } else {
                    parser_step = KiaV2DecoderStepReset;
                    break;
                }

                bool data_bit;
                if (FProtoGeneral::manchester_advance(manchester_state, event, &manchester_state, &data_bit)) {
                    decode_data = (decode_data << 1) | data_bit;
                    decode_count_bit++;

                    if (decode_count_bit == 53) {
                        // instance->generic.data = decode_data;
                        data_count_bit = decode_count_bit;

                        // instance->generic.serial = (uint32_t)((instance->generic.data >> 20) & 0xFFFFFFFF);
                        // instance->generic.btn = (uint8_t)((instance->generic.data >> 16) & 0x0F);

                        // uint16_t raw_count = (uint16_t)((instance->generic.data >> 4) & 0xFFF);
                        // instance->generic.cnt = ((raw_count >> 4) | (raw_count << 8)) & 0xFFF;

                        if (callback)
                            callback(this);

                        decode_data = 0;
                        decode_count_bit = 0;
                        header_count = 0;
                        parser_step = KiaV2DecoderStepReset;
                    }
                }
                break;
            }
        }
    }

    uint16_t header_count = 0;
    ManchesterState manchester_state = ManchesterStateMid1;
};
