
#ifndef __FPROTO_NEXUSTH_H__
#define __FPROTO_NEXUSTH_H__

#include "weatherbase.hpp"

// For the state machine
typedef enum {
    Nexus_THDecoderStepReset = 0,
    Nexus_THDecoderStepSaveDuration,
    Nexus_THDecoderStepCheckDuration,
} Nexus_THDecoderStep;

// we will look for this value
#define NEXUS_TH_CONST_DATA 0b1111

class FProtoWeatherNexusTH : public FProtoWeatherBase {
   public:
    FProtoWeatherNexusTH() {
        // must set it's value from the "weathertypes.hpp". getWeatherSensorTypeName() will work with this.
        sensorType = FPW_NexusTH;
    }

    // Here we will got a level and duration. eg HIGH (true) for 500. This function must be as fast as possible, to keep the core happy.
    // From this function we will call the "callback", that is responsible for transmitting the parsed data to the ui. Before calling it, we must populate the base class's variables, like temp, humidity, ...
    void feed(bool level, uint32_t duration) override {
        switch (parser_step) {
            case Nexus_THDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 8) <
                                 te_delta * 4)) {
                    // Found sync
                    parser_step = Nexus_THDecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                }
                break;

            case Nexus_THDecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = Nexus_THDecoderStepCheckDuration;
                } else {
                    parser_step = Nexus_THDecoderStepReset;
                }
                break;

            case Nexus_THDecoderStepCheckDuration:
                if (!level) {
                    if (DURATION_DIFF(duration, te_short * 8) < te_delta * 4) {
                        // Found sync
                        parser_step = Nexus_THDecoderStepReset;
                        if ((decode_count_bit == min_count_bit_for_found) &&
                            ws_protocol_nexus_th_check()) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            ws_protocol_nexus_th_remote_controller();
                            if (callback) callback((FProtoWeatherBase*)this);
                            parser_step = Nexus_THDecoderStepCheckDuration;
                        }
                        decode_data = 0;
                        decode_count_bit = 0;

                        break;
                    } else if ((DURATION_DIFF(te_last, te_short) <
                                te_delta) &&
                               (DURATION_DIFF(duration, te_short * 2) < te_delta * 2)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = Nexus_THDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_short * 4) < te_delta * 4)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = Nexus_THDecoderStepSaveDuration;
                    } else {
                        parser_step = Nexus_THDecoderStepReset;
                    }
                } else {
                    parser_step = Nexus_THDecoderStepReset;
                }
                break;
        }
    }

   protected:
    // timing values
    uint32_t te_short = 490;
    uint32_t te_long = 1980;
    uint32_t te_delta = 150;
    uint32_t min_count_bit_for_found = 36;

    // sanity check
    bool ws_protocol_nexus_th_check() {
        uint8_t type = (decode_data >> 8) & 0x0F;
        if ((type == NEXUS_TH_CONST_DATA) && ((decode_data >> 4) != 0xffffffff)) {
            return true;
        } else {
            return false;
        }
        return true;
    }

    // fill the base class's variables before calling the callback
    void ws_protocol_nexus_th_remote_controller() {
        id = (data >> 28) & 0xFF;
        battery_low = !((data >> 27) & 1);
        channel = ((data >> 24) & 0x03) + 1;
        if (!((data >> 23) & 1)) {
            temp = (float)((data >> 12) & 0x07FF) / 10.0f;
        } else {
            temp = (float)((~(data >> 12) & 0x07FF) + 1) / -10.0f;
        }
        humidity = data & 0xFF;
        if (humidity > 95)
            humidity = 95;
        else if (humidity < 20)
            humidity = 20;
    }
};

#endif
