
#ifndef __FPROTO_HORMANNBISECURE_H__
#define __FPROTO_HORMANNBISECURE_H__

#include "subghzdbase.hpp"

typedef enum {
    HormannBiSecurDecoderStepReset = 0,
    HormannBiSecurDecoderStepFoundPreambleAlternatingShort,
    HormannBiSecurDecoderStepFoundPreambleHighVeryLong,
    HormannBiSecurDecoderStepFoundPreambleAlternatingLong,
    HormannBiSecurDecoderStepFoundData,
} HormannBiSecurDecoderStep;

class FProtoSubGhzDHormannBiSecure : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDHormannBiSecure() {
        sensorType = FPS_HORMANN;
        te_short = 208;
        te_long = 416;
        te_delta = 104;
        min_count_bit_for_found = 176;
    }

    void subghz_protocol_decoder_hormann_bisecur_reset() {
        parser_step = HormannBiSecurDecoderStepReset;
        data = 0;
        for (uint8_t i = 0; i < 22; ++i) dataa[i] = 0;
        data_count_bit = 0;
        manchester_saved_state = ManchesterStateStart1;
    }

    void feed(bool level, uint32_t duration) {
        ManchesterEvent event = ManchesterEventReset;

        switch (parser_step) {
            case HormannBiSecurDecoderStepReset:
                if (!level && DURATION_DIFF(duration, duration_short + duration_half_short) < te_delta) {
                    parser_step = HormannBiSecurDecoderStepFoundPreambleAlternatingShort;
                }
                break;
            case HormannBiSecurDecoderStepFoundPreambleAlternatingShort:
                if (DURATION_DIFF(duration, duration_short) < te_delta) {
                    // stay on the same step, the pattern repeats around 21 times
                    break;
                }

                if (level && DURATION_DIFF(duration, duration_long * 4) < te_delta) {
                    parser_step = HormannBiSecurDecoderStepFoundPreambleHighVeryLong;
                    break;
                }

                parser_step = HormannBiSecurDecoderStepReset;
                break;
            case HormannBiSecurDecoderStepFoundPreambleHighVeryLong:
                if (!level && DURATION_DIFF(duration, duration_long) < te_delta) {
                    sync_cnt = 3;
                    parser_step = HormannBiSecurDecoderStepFoundPreambleAlternatingLong;
                    break;
                }

                parser_step = HormannBiSecurDecoderStepReset;
                break;
            case HormannBiSecurDecoderStepFoundPreambleAlternatingLong:
                if (level == (sync_cnt-- & 1) &&
                    DURATION_DIFF(duration, duration_long) < te_delta) {
                    if (!sync_cnt) {
                        FProtoGeneral::manchester_advance_alt(manchester_saved_state, event, &manchester_saved_state, NULL);
                        parser_step = HormannBiSecurDecoderStepFoundData;
                    }

                    // stay on the same step, or advance to the next if enough transitions are found
                    break;
                }

                parser_step = HormannBiSecurDecoderStepReset;
                break;
            case HormannBiSecurDecoderStepFoundData:
                if (DURATION_DIFF(duration, duration_short) < te_delta ||
                    (
                        // the last bit can be arbitrary long, but it is parsed as a short
                        data_count_bit == min_count_bit_for_found - 1 &&
                        duration > duration_short)) {
                    event = !level ? ManchesterEventShortHigh : ManchesterEventShortLow;
                }

                if (DURATION_DIFF(duration, duration_long) < te_delta) {
                    event = !level ? ManchesterEventLongHigh : ManchesterEventLongLow;
                }

                if (event == ManchesterEventReset) {
                    subghz_protocol_decoder_hormann_bisecur_reset();
                } else {
                    bool new_level;

                    if (manchester_advance_alt(instance->manchester_saved_state, event, &instance->manchester_saved_state, &new_level)) {
                        subghz_protocol_decoder_hormann_bisecur_add_bit(instance, new_level);
                    }
                }
                break;
        }
    }

    void subghz_protocol_decoder_hormann_bisecur_add_bit(bool level) {
        if (data_count_bit >= min_count_bit_for_found) {
            return;
        }

        if (level) {
            uint8_t byte_index = data_count_bit / 8;
            uint8_t bit_index = data_count_bit % 8;
            dataa[byte_index] |= 1 << (7 - bit_index);
        }
        data_count_bit++;
        if (data_count_bit >= min_count_bit_for_found) {
            if (callback) {
                callback(this);
            } else {
                subghz_protocol_decoder_hormann_bisecur_reset();
            }
        }
    }

   protected:
    ManchesterState manchester_saved_state = ManchesterStateMid1;
    uint8_t dataa[22] = {0};
    uint8_t sync_cnt = 0;
};

#endif
