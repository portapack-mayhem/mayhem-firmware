#pragma once
#include "subcarbase.hpp"
#include <cstring>

typedef enum {
    FiatV0DecoderStepReset = 0,
    FiatV0DecoderStepPreamble = 1,
    FiatV0DecoderStepData = 2,
} FiatV0DecoderStep;

class FProtoSubCarFiatV0 : public FProtoSubCarBase {
   public:
    FProtoSubCarFiatV0() {
        sensorType = FPC_FIATV0;
        te_short = 200;
        te_long = 400;
        te_delta = 100;
        min_count_bit_for_found = 64;
    }

    void feed(bool level, uint32_t duration) {
        uint32_t gap_threshold = 800;
        uint32_t diff;
        switch (decoder_state) {
            case FiatV0DecoderStepReset:
                if (!level) {
                    return;
                }
                if (duration < te_short) {
                    diff = te_short - duration;
                } else {
                    diff = duration - te_short;
                }
                if (diff < te_delta) {
                    data_low = 0;
                    data_high = 0;
                    decoder_state = FiatV0DecoderStepPreamble;
                    te_last = duration;
                    preamble_count = 0;
                    bit_count = 0;
                    FProtoGeneral::manchester_advance(
                        manchester_state,
                        ManchesterEventReset,
                        &manchester_state,
                        NULL);
                }
                break;
            case FiatV0DecoderStepPreamble:
                if (level) {
                    return;
                }
                if (duration < te_short) {
                    diff = te_short - duration;
                    if (diff < te_delta) {
                        preamble_count++;
                        te_last = duration;
                        if (preamble_count >= 0x96) {
                            if (duration < gap_threshold) {
                                diff = gap_threshold - duration;
                            } else {
                                diff = duration - gap_threshold;
                            }
                            if (diff < te_delta) {
                                decoder_state = FiatV0DecoderStepData;
                                preamble_count = 0;
                                data_low = 0;
                                data_high = 0;
                                bit_count = 0;
                                te_last = duration;
                                return;
                            }
                        }
                    } else {
                        decoder_state = FiatV0DecoderStepReset;
                        if (preamble_count >= 0x96) {
                            if (duration < gap_threshold) {
                                diff = gap_threshold - duration;
                            } else {
                                diff = duration - gap_threshold;
                            }
                            if (diff < te_delta) {
                                decoder_state = FiatV0DecoderStepData;
                                preamble_count = 0;
                                data_low = 0;
                                data_high = 0;
                                bit_count = 0;
                                te_last = duration;
                                return;
                            }
                        }
                    }
                } else {
                    diff = duration - te_short;
                    if (diff < te_delta) {
                        preamble_count++;
                        te_last = duration;
                    } else {
                        decoder_state = FiatV0DecoderStepReset;
                    }
                    if (preamble_count >= 0x96) {
                        if (duration >= 799) {
                            diff = duration - gap_threshold;
                        } else {
                            diff = gap_threshold - duration;
                        }
                        if (diff < te_delta) {
                            decoder_state = FiatV0DecoderStepData;
                            preamble_count = 0;
                            data_low = 0;
                            data_high = 0;
                            bit_count = 0;
                            te_last = duration;
                            return;
                        }
                    }
                }
                break;
            case FiatV0DecoderStepData:
                ManchesterEvent event = ManchesterEventReset;
                if (duration < te_short) {
                    diff = te_short - duration;
                    if (diff < te_delta) {
                        event = level ? ManchesterEventShortLow : ManchesterEventShortHigh;
                    }
                } else {
                    diff = duration - te_short;
                    if (diff < te_delta) {
                        event = level ? ManchesterEventShortLow : ManchesterEventShortHigh;
                    } else {
                        if (duration < te_long) {
                            diff = te_long - duration;
                        } else {
                            diff = duration - te_long;
                        }
                        if (diff < te_delta) {
                            event = level ? ManchesterEventLongLow : ManchesterEventLongHigh;
                        }
                    }
                }

                if (event != ManchesterEventReset) {
                    bool data_bit_bool;
                    if (FProtoGeneral::manchester_advance(
                            manchester_state,
                            event,
                            &manchester_state,
                            &data_bit_bool)) {
                        uint32_t new_bit = data_bit_bool ? 1 : 0;

                        uint32_t carry = (data_low >> 31) & 1;
                        data_low = (data_low << 1) | new_bit;
                        data_high = (data_high << 1) | carry;

                        bit_count++;

                        if (bit_count == 0x40) {
                            fix = data_low;
                            hop = data_high;
                            data_low = 0;
                            data_high = 0;
                        }

                        if (bit_count > 0x46) {
                            final_count = bit_count;

                            endbyte = (uint8_t)data_low;
                            /*
                                                        generic.data = ((uint64_t)hop << 32) | fix;
                                                        generic.data_count_bit = 64;
                                                        generic.serial = fix;
                                                        generic.btn = endbyte;  // still exported as btn for UI compatibility
                                                        generic.cnt = hop;
                            */
                            decode_data = ((uint64_t)hop << 32) | fix;  // this is my own data passer, not the original
                            decode_data2 = endbyte;
                            data_count_bit = 64;
                            if (callback) {
                                callback(this);
                            }

                            data_low = 0;
                            data_high = 0;
                            bit_count = 0;
                            decoder_state = FiatV0DecoderStepReset;
                        }
                    }
                }
                te_last = duration;
                break;
        }
    }

    ManchesterState manchester_state = ManchesterStateMid1;
    uint8_t decoder_state = 0;
    uint16_t preamble_count = 0;
    uint32_t data_low = 0;
    uint32_t data_high = 0;
    uint8_t bit_count = 0;
    uint32_t hop = 0;
    uint32_t fix = 0;
    uint8_t endbyte = 0;
    uint8_t final_count = 0;
    uint32_t te_last = 0;
};
