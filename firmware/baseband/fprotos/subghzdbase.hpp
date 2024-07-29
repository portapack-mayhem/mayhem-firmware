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
// default values to indicate 'no value'

class FProtoSubGhzDBase;
typedef void (*SubGhzDProtocolDecoderBaseRxCallback)(FProtoSubGhzDBase* instance);

class FProtoSubGhzDBase {
   public:
    FProtoSubGhzDBase() {}
    virtual ~FProtoSubGhzDBase() {}
    virtual void feed(bool level, uint32_t duration) = 0;                         // need to be implemented on each protocol handler.
    void setCallback(SubGhzDProtocolDecoderBaseRxCallback cb) { callback = cb; }  // this is called when there is a hit.

    // General data holder, these will be passed
    uint8_t sensorType = FPS_Invalid;
    uint16_t data_count_bit = 0;
    uint64_t data = 0;

   protected:
    // Helper functions to keep it as compatible with flipper as we can, so adding new protos will be easy.
    void subghz_protocol_blocks_add_bit(uint8_t bit) {
        decode_data = decode_data << 1 | bit;
        decode_count_bit++;
    }

    // inner logic stuff, also for flipper compatibility.
    uint32_t te_short = UINT32_MAX;
    uint32_t te_long = UINT32_MAX;
    uint32_t te_delta = UINT32_MAX;
    uint32_t min_count_bit_for_found = UINT32_MAX;

    SubGhzDProtocolDecoderBaseRxCallback callback = NULL;
    uint16_t header_count = 0;
    uint8_t parser_step = 0;
    uint32_t te_last = 0;
    uint32_t decode_count_bit = 0;
    uint64_t decode_data = 0;
    //
};

#endif