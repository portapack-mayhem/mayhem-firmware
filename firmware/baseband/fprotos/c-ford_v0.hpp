#pragma once
#include "subcarbase.hpp"
#include <cstring>

typedef enum {
    FordV0DecoderStepReset = 0,
    FordV0DecoderStepPreamble,
    FordV0DecoderStepPreambleCheck,
    FordV0DecoderStepGap,
    FordV0DecoderStepData,
} FordV0DecoderStep;

class FProtoSubCarFordV0 : public FProtoSubCarBase {
   public:
    FProtoSubCarFordV0() {
        sensorType = FPC_FORDV0;
        te_short = 250;
        te_long = 500;
        te_delta = 100;
        min_count_bit_for_found = 64;
    }
    void ford_v0_add_bit(bool bit) {
        uint32_t low = (uint32_t)data_low;
        data_low = (data_low << 1) | (bit ? 1 : 0);
        data_high = (data_high << 1) | ((low >> 31) & 1);
        bit_count++;
    }

    bool ford_v0_process_data() {
        if (bit_count == 64) {
            uint64_t combined = ((uint64_t)data_high << 32) | data_low;
            key1 = ~combined;
            data_low = 0;
            data_high = 0;
            return false;
        }

        if (bit_count == 80) {
            uint16_t key2_raw = (uint16_t)(data_low & 0xFFFF);
            uint16_t key2 = ~key2_raw;
            decode_data = key1;
            decode_data2 = key2;
            // decode_ford_v0(key1, key2, &serial, &button, &count);
            return true;
        }

        return false;
    }

    void feed(bool level, uint32_t duration) {
        uint32_t gap_threshold = 3500;

        switch (parser_step) {
            case FordV0DecoderStepReset:
                if (level && (DURATION_DIFF(duration, te_short) < te_delta)) {
                    data_low = 0;
                    data_high = 0;
                    parser_step = FordV0DecoderStepPreamble;
                    te_last = duration;
                    header_count = 0;
                    bit_count = 0;
                    FProtoGeneral::manchester_advance(manchester_state, ManchesterEventReset, &manchester_state, NULL);
                }
                break;

            case FordV0DecoderStepPreamble:
                if (!level) {
                    if (DURATION_DIFF(duration, te_long) < te_delta) {
                        te_last = duration;
                        parser_step = FordV0DecoderStepPreambleCheck;
                    } else {
                        parser_step = FordV0DecoderStepReset;
                    }
                }
                break;

            case FordV0DecoderStepPreambleCheck:
                if (level) {
                    if (DURATION_DIFF(duration, te_long) < te_delta) {
                        header_count++;
                        te_last = duration;
                        parser_step = FordV0DecoderStepPreamble;
                    } else if (DURATION_DIFF(duration, te_short) < te_delta) {
                        parser_step = FordV0DecoderStepGap;
                    } else {
                        parser_step = FordV0DecoderStepReset;
                    }
                }
                break;

            case FordV0DecoderStepGap:
                if (!level && (DURATION_DIFF(duration, gap_threshold) < 250)) {
                    data_low = 1;
                    data_high = 0;
                    bit_count = 1;
                    parser_step = FordV0DecoderStepData;
                } else if (!level && duration > gap_threshold + 250) {
                    parser_step = FordV0DecoderStepReset;
                }
                break;

            case FordV0DecoderStepData: {
                ManchesterEvent event;

                if (DURATION_DIFF(duration, te_short) < te_delta) {
                    event = level ? ManchesterEventShortLow : ManchesterEventShortHigh;
                } else if (DURATION_DIFF(duration, te_long) < te_delta) {
                    event = level ? ManchesterEventLongLow : ManchesterEventLongHigh;
                } else {
                    parser_step = FordV0DecoderStepReset;
                    break;
                }

                bool data_bit;
                if (FProtoGeneral::manchester_advance(manchester_state, event, &manchester_state, &data_bit)) {
                    ford_v0_add_bit(data_bit);
                    if (ford_v0_process_data()) {
                        /* instance->generic.data = instance->key1;
                         instance->generic.data_count_bit = 64;
                         instance->generic.serial = instance->serial;
                         instance->generic.btn = instance->button;
                         instance->generic.cnt = instance->count;
                         */
                        data_count_bit = 64;
                        if (callback) {
                            callback(this);
                        }

                        data_low = 0;
                        data_high = 0;
                        bit_count = 0;
                        parser_step = FordV0DecoderStepReset;
                    }
                }

                te_last = duration;
                break;
            }
        }
    }

    ManchesterState manchester_state = ManchesterStateMid1;

    uint64_t data_low = 0;
    uint64_t data_high = 0;
    uint8_t bit_count = 0;

    uint16_t header_count = 0;
    uint64_t key1 = 0;
    uint16_t key2 = 0;
};
