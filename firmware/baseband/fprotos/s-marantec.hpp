
#ifndef __FPROTO_MARANTEC_H__
#define __FPROTO_MARANTEC_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    MarantecDecoderStepReset = 0,
    MarantecDecoderFoundHeader,
    MarantecDecoderStepDecoderData,
} MarantecDecoderStep;

class FProtoSubGhzDMarantec : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDMarantec() {
        sensorType = FPS_MARANTEC;
        te_short = 1000;
        te_long = 2000;
        te_delta = 200;
        min_count_bit_for_found = 49;
    }

    void feed(bool level, uint32_t duration) {
        ManchesterEvent event = ManchesterEventReset;

        switch (parser_step) {
            case MarantecDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_long * 5) < te_delta * 8)) {
                    // Found header marantec
                    parser_step = MarantecDecoderStepDecoderData;
                    decode_data = 1;
                    decode_count_bit = 1;
                    FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventReset, &manchester_saved_state, NULL);
                }
                break;
            case MarantecDecoderStepDecoderData:
                if (!level) {
                    if (DURATION_DIFF(duration, te_short) < te_delta) {
                        event = ManchesterEventShortLow;
                    } else if (
                        DURATION_DIFF(duration, te_long) < te_delta) {
                        event = ManchesterEventLongLow;
                    } else if (
                        duration >= ((uint32_t)te_long * 2 + te_delta)) {
                        if (decode_count_bit == min_count_bit_for_found) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            if (callback) callback(this);
                        }
                        decode_data = 1;
                        decode_count_bit = 1;
                        FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventReset, &manchester_saved_state, NULL);
                    } else {
                        parser_step = MarantecDecoderStepReset;
                    }
                } else {
                    if (DURATION_DIFF(duration, te_short) < te_delta) {
                        event = ManchesterEventShortHigh;
                    } else if (
                        DURATION_DIFF(duration, te_long) < te_delta) {
                        event = ManchesterEventLongHigh;
                    } else {
                        parser_step = MarantecDecoderStepReset;
                    }
                }
                if (event != ManchesterEventReset) {
                    bool bitstate;
                    bool data_ok = FProtoGeneral::manchester_advance(manchester_saved_state, event, &manchester_saved_state, &bitstate);

                    if (data_ok) {
                        decode_data = (decode_data << 1) | bitstate;
                        decode_count_bit++;
                    }
                }
                break;
        }
    }

   protected:
    ManchesterState manchester_saved_state = ManchesterStateMid1;
};

#endif
