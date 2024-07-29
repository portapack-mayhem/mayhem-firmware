
#ifndef __FPROTO_CAMETWEE_H__
#define __FPROTO_CAMETWEE_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    CameTweeDecoderStepReset = 0,
    CameTweeDecoderStepDecoderData,
} CameTweeDecoderStep;

class FProtoSubGhzDCameTwee : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDCameTwee() {
        sensorType = FPS_CAMETWEE;
        te_short = 500;
        te_long = 1000;
        te_delta = 250;
        min_count_bit_for_found = 54;
    }

    void feed(bool level, uint32_t duration) {
        ManchesterEvent event = ManchesterEventReset;
        switch (parser_step) {
            case CameTweeDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_long * 51) < te_delta * 20)) {
                    // Found header CAME
                    parser_step = CameTweeDecoderStepDecoderData;
                    decode_data = 0;
                    decode_count_bit = 0;
                    FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventLongLow, &manchester_saved_state, NULL);
                    FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventLongHigh, &manchester_saved_state, NULL);
                    FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventShortLow, &manchester_saved_state, NULL);
                }
                break;
            case CameTweeDecoderStepDecoderData:
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
                        decode_data = 0;
                        decode_count_bit = 0;
                        FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventLongLow, &manchester_saved_state, NULL);
                        FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventLongHigh, &manchester_saved_state, NULL);
                        FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventShortLow, &manchester_saved_state, NULL);
                    } else {
                        parser_step = CameTweeDecoderStepReset;
                    }
                } else {
                    if (DURATION_DIFF(duration, te_short) < te_delta) {
                        event = ManchesterEventShortHigh;
                    } else if (
                        DURATION_DIFF(duration, te_long) < te_delta) {
                        event = ManchesterEventLongHigh;
                    } else {
                        parser_step = CameTweeDecoderStepReset;
                    }
                }
                if (event != ManchesterEventReset) {
                    bool bit;
                    if (FProtoGeneral::manchester_advance(manchester_saved_state, event, &manchester_saved_state, &bit)) {
                        decode_data = (decode_data << 1) | !bit;
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
