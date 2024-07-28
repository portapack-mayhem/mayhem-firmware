
#ifndef __FPROTO_POWER_SMART_H__
#define __FPROTO_POWER_SMART_H__

#include "subghzdbase.hpp"

#define POWER_SMART_PACKET_HEADER 0xFD000000AA000000
#define POWER_SMART_PACKET_HEADER_MASK 0xFF000000FF000000

typedef enum : uint8_t {
    PowerSmartDecoderStepReset = 0,
    PowerSmartDecoderFoundHeader,
    PowerSmartDecoderStepDecoderData,
} PowerSmartDecoderStep;

class FProtoSubGhzDPowerSmart : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDPowerSmart() {
        sensorType = FPS_POWERSMART;
        te_short = 225;
        te_long = 450;
        te_delta = 100;
        min_count_bit_for_found = 64;
    }

    void feed(bool level, uint32_t duration) {
        ManchesterEvent event = ManchesterEventReset;
        if (!level) {
            if (DURATION_DIFF(duration, te_short) < te_delta) {
                event = ManchesterEventShortLow;
            } else if (
                DURATION_DIFF(duration, te_long) < te_delta * 2) {
                event = ManchesterEventLongLow;
            }
        } else {
            if (DURATION_DIFF(duration, te_short) < te_delta) {
                event = ManchesterEventShortHigh;
            } else if (
                DURATION_DIFF(duration, te_long) < te_delta * 2) {
                event = ManchesterEventLongHigh;
            }
        }
        if (event != ManchesterEventReset) {
            bool bit_val;
            bool data_ok = FProtoGeneral::manchester_advance(manchester_saved_state, event, &manchester_saved_state, &bit_val);

            if (data_ok) {
                decode_data = (decode_data << 1) | !bit_val;
            }
            if ((decode_data & POWER_SMART_PACKET_HEADER_MASK) == POWER_SMART_PACKET_HEADER) {
                if (subghz_protocol_power_smart_chek_valid(decode_data)) {
                    data = decode_data;
                    data_count_bit = min_count_bit_for_found;

                    if (callback) callback(this);
                    decode_data = 0;
                    decode_count_bit = 0;
                }
            }
        } else {
            decode_data = 0;
            decode_count_bit = 0;
            FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventReset, &manchester_saved_state, NULL);
        }
    }

   protected:
    ManchesterState manchester_saved_state = ManchesterStateMid1;

    bool subghz_protocol_power_smart_chek_valid(uint64_t packet) {
        uint32_t data_1 = (uint32_t)((packet >> 40) & 0xFFFF);
        uint32_t data_2 = (uint32_t)((~packet >> 8) & 0xFFFF);
        uint8_t data_3 = (uint8_t)(packet >> 32) & 0xFF;
        uint8_t data_4 = (uint8_t)(((~packet) & 0xFF) - 1);
        return (data_1 == data_2) && (data_3 == data_4);
    }
};

#endif
