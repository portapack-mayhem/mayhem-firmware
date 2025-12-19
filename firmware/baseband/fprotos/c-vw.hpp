#pragma once
#include "subcarbase.hpp"

typedef enum {
    VwDecoderStepReset = 0,
    VwDecoderStepFoundSync,
    VwDecoderStepFoundStart1,
    VwDecoderStepFoundStart2,
    VwDecoderStepFoundStart3,
    VwDecoderStepFoundData,
} VwDecoderStep;

class FProtoSubCarVW : public FProtoSubCarBase {
   public:
    FProtoSubCarVW() {
        sensorType = FPC_VW;
        te_short = 500;
        te_long = 1000;
        te_delta = 130;
        min_count_bit_for_found = 80;
    }
    uint8_t vw_get_bit_index(uint8_t bit) {
        uint8_t bit_index = 0;
        if (bit < 72 && bit >= 8) {
            // use generic.data (bytes 1-8)
            bit_index = bit - 8;
        } else {
            // use data_2
            if (bit >= 72) {
                bit_index = bit - 64;  // byte 0 = type
            }
            if (bit < 8) {
                bit_index = bit;  // byte 9 = check digit
            }
            bit_index |= 0x80;  // mark for data_2
        }
        return bit_index;
    }

    void vw_add_bit(bool level) {
        if (data_count_bit >= min_count_bit_for_found) {
            return;
        }
        uint8_t bit_index_full = min_count_bit_for_found - 1 - data_count_bit;
        uint8_t bit_index_masked = vw_get_bit_index(bit_index_full);
        uint8_t bit_index = bit_index_masked & 0x7F;
        if (bit_index_masked & 0x80) {
            // use data_2
            if (level) {
                decode_data2 |= (1ULL << bit_index);
            } else {
                decode_data2 &= ~(1ULL << bit_index);
            }
        } else {
            // use data
            if (level) {
                decode_data |= (1ULL << bit_index);
            } else {
                decode_data &= ~(1ULL << bit_index);
            }
        }

        data_count_bit++;
        if (data_count_bit >= min_count_bit_for_found) {
            if (callback) {
                callback(this);
            }
        }
    }

    void subghz_protocol_decoder_vw_reset() {
        parser_step = VwDecoderStepReset;
        data_count_bit = 0;
        decode_data = 0;
        decode_data2 = 0;
        manchester_state = ManchesterStateMid1;
    }

    bool vw_manchester_advance(
        ManchesterState state,
        ManchesterEvent event,
        ManchesterState* next_state,
        bool* data) {
        bool result = false;
        ManchesterState new_state = ManchesterStateMid1;

        if (event == ManchesterEventReset) {
            new_state = ManchesterStateMid1;
        } else if (state == ManchesterStateMid0 || state == ManchesterStateMid1) {
            if (event == ManchesterEventShortHigh) {
                new_state = ManchesterStateStart1;
            } else if (event == ManchesterEventShortLow) {
                new_state = ManchesterStateStart0;
            } else {
                new_state = ManchesterStateMid1;
            }
        } else if (state == ManchesterStateStart1) {
            if (event == ManchesterEventShortLow) {
                new_state = ManchesterStateMid1;
                result = true;
                if (data)
                    *data = true;
            } else if (event == ManchesterEventLongLow) {
                new_state = ManchesterStateStart0;
                result = true;
                if (data)
                    *data = true;
            } else {
                new_state = ManchesterStateMid1;
            }
        } else if (state == ManchesterStateStart0) {
            if (event == ManchesterEventShortHigh) {
                new_state = ManchesterStateMid0;
                result = true;
                if (data)
                    *data = false;
            } else if (event == ManchesterEventLongHigh) {
                new_state = ManchesterStateStart1;
                result = true;
                if (data)
                    *data = false;
            } else {
                new_state = ManchesterStateMid1;
            }
        }

        *next_state = new_state;
        return result;
    }

    void feed(bool level, uint32_t duration) {
        uint32_t te_med = (te_long + te_short) / 2;
        uint32_t te_end = te_long * 5;

        ManchesterEvent event = ManchesterEventReset;

        switch (parser_step) {
            case VwDecoderStepReset:
                if (DURATION_DIFF(duration, te_short) < te_delta) {
                    parser_step = VwDecoderStepFoundSync;
                }
                break;

            case VwDecoderStepFoundSync:
                if (DURATION_DIFF(duration, te_short) < te_delta) {
                    // Stay - sync pattern repeats ~43 times
                    break;
                }

                if (level && DURATION_DIFF(duration, te_long) < te_delta) {
                    parser_step = VwDecoderStepFoundStart1;
                    break;
                }

                parser_step = VwDecoderStepReset;
                break;

            case VwDecoderStepFoundStart1:
                if (!level && DURATION_DIFF(duration, te_short) < te_delta) {
                    parser_step = VwDecoderStepFoundStart2;
                    break;
                }

                parser_step = VwDecoderStepReset;
                break;

            case VwDecoderStepFoundStart2:
                if (level && DURATION_DIFF(duration, te_med) < te_delta) {
                    parser_step = VwDecoderStepFoundStart3;
                    break;
                }

                parser_step = VwDecoderStepReset;
                break;

            case VwDecoderStepFoundStart3:
                if (DURATION_DIFF(duration, te_med) < te_delta) {
                    // Stay - med pattern repeats
                    break;
                }

                if (level && DURATION_DIFF(duration, te_short) < te_delta) {
                    // Start data collection
                    vw_manchester_advance(
                        manchester_state,
                        ManchesterEventReset,
                        &manchester_state,
                        NULL);
                    vw_manchester_advance(
                        manchester_state,
                        ManchesterEventShortHigh,
                        &manchester_state,
                        NULL);
                    data_count_bit = 0;
                    decode_data = 0;
                    decode_data2 = 0;
                    parser_step = VwDecoderStepFoundData;
                    break;
                }

                parser_step = VwDecoderStepReset;
                break;

            case VwDecoderStepFoundData:
                if (DURATION_DIFF(duration, te_short) < te_delta) {
                    event = level ? ManchesterEventShortHigh : ManchesterEventShortLow;
                }

                if (DURATION_DIFF(duration, te_long) < te_delta) {
                    event = level ? ManchesterEventLongHigh : ManchesterEventLongLow;
                }

                // Last bit can be arbitrarily long
                if (data_count_bit == min_count_bit_for_found - 1 &&
                    !level && duration > te_end) {
                    event = ManchesterEventShortLow;
                }

                if (event == ManchesterEventReset) {
                    subghz_protocol_decoder_vw_reset();
                } else {
                    bool new_level;
                    if (vw_manchester_advance(
                            manchester_state,
                            event,
                            &manchester_state,
                            &new_level)) {
                        vw_add_bit(new_level);
                    }
                }
                break;
        }
    }

    ManchesterState manchester_state = ManchesterStateMid1;
};
