/*
Base class for all weather protocols.
This and most of the weather protocols uses code from Flipper XTreme codebase ( https://github.com/Flipper-XFW/Xtreme-Firmware/tree/dev/lib/subghz ). Thanks for their work!
For comments in a protocol implementation check w-nexus-th.hpp
*/

#ifndef __FPROTO_SBASE_H__
#define __FPROTO_SBASE_H__

#include "fprotogeneral.hpp"
#include "subghztypes.hpp"

#include <string>
// default walues to indicate 'no value'
#define SD_NO_ID 0xFFFFFFFF

class FProtoSubGhzDBase;
typedef void (*SubGhzDProtocolDecoderBaseRxCallback)(FProtoSubGhzDBase* instance);

class FProtoSubGhzDBase {
   public:
    FProtoSubGhzDBase() {}
    virtual ~FProtoSubGhzDBase() {}
    virtual void feed(bool level, uint32_t duration) = 0;                         // need to be implemented on each protocol handler.
    void setCallback(SubGhzDProtocolDecoderBaseRxCallback cb) { callback = cb; }  // this is called when there is a hit.

    uint8_t getSensorType() { return sensorType; }
    uint32_t getSensorId() { return id; }
    FPROTO_SUBGHZD_MODULATION modulation = FPM_AM;  // override this, if FM
   protected:
    // Helper functions to keep it as compatible with flipper as we can, so adding new protos will be easy.
    void subghz_protocol_blocks_add_bit(uint8_t bit) {
        decode_data = decode_data << 1 | bit;
        decode_count_bit++;
    }

    // General weather data holder
    uint8_t sensorType = FPS_Invalid;
    uint32_t id = SD_NO_ID;

    // inner logic stuff, also for flipper compatibility.
    SubGhzDProtocolDecoderBaseRxCallback callback = NULL;
    uint16_t header_count = 0;
    uint8_t parser_step = 0;
    uint32_t te_last = 0;
    uint64_t data = 0;
    uint64_t data_2 = 0;
    uint32_t serial = 0;
    uint16_t data_count_bit = 0;
    uint64_t decode_data = 0;
    uint32_t decode_count_bit = 0;
    uint8_t btn = 0;
    uint32_t cnt = 0;
    uint8_t cnt_2 = 0;
    uint32_t seed = 0;

    ManchesterState manchester_saved_state = ManchesterStateMid1;
};

#endif