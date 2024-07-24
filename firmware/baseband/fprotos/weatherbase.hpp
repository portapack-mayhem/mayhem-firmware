/*
Base class for all weather protocols.
This and most of the weather protocols uses code from Flipper XTreme codebase ( https://github.com/Flipper-XFW/Xtreme-Firmware/tree/dev/lib/subghz ). Thanks for their work!
For comments in a protocol implementation check w-nexus-th.hpp
*/

#ifndef __FPROTO_BASE_H__
#define __FPROTO_BASE_H__

#include "fprotogeneral.hpp"
#include "weathertypes.hpp"

#include <string>
// default walues to indicate 'no value'

class FProtoWeatherBase;
typedef void (*SubGhzProtocolDecoderBaseRxCallback)(FProtoWeatherBase* instance);

class FProtoWeatherBase {
   public:
    FProtoWeatherBase() {}
    virtual ~FProtoWeatherBase() {}
    virtual void feed(bool level, uint32_t duration) = 0;                        // need to be implemented on each protocol handler.
    void setCallback(SubGhzProtocolDecoderBaseRxCallback cb) { callback = cb; }  // this is called when there is a hit.

    uint8_t getSensorType() { return sensorType; }
    uint8_t getHumidity() { return humidity; }
    uint8_t getBattLow() { return battery_low; }
    uint8_t getChannel() { return channel; }
    uint32_t getSensorId() { return id; }
    float getTemp() { return temp; }

   protected:
    // Helper functions to keep it as compatible with flipper as we can, so adding new protos will be easy.
    void subghz_protocol_blocks_add_bit(uint8_t bit) {
        decode_data = decode_data << 1 | bit;
        decode_count_bit++;
    }

    // needs to be in this chaotic order, to save flash!
    //  General weather data holder
    uint8_t sensorType = FPW_Invalid;
    uint8_t humidity = WS_NO_HUMIDITY;
    uint8_t battery_low = WS_NO_BATT;
    uint8_t channel = WS_NO_CHANNEL;
    // inner logic stuff
    uint8_t parser_step = 0;
    // inner logic stuff, also for flipper compatibility.
    uint16_t header_count = 0;
    // General weather data holder
    float temp = WS_NO_TEMPERATURE;
    uint32_t id = WS_NO_ID;
    // inner logic stuff,

    uint32_t te_last = 0;
    uint32_t data_count_bit = 0;
    uint32_t decode_count_bit = 0;
    uint64_t decode_data = 0;
    uint64_t data = 0;
    SubGhzProtocolDecoderBaseRxCallback callback = NULL;
};

#endif