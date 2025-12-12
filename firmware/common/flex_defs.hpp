#ifndef __FLEX_DEFS_H__
#define __FLEX_DEFS_H__

#include <cstdint>
#include <array>
#include "baseband.hpp"

namespace flex {

enum class FlexMode : uint8_t {
    FLEX_1600_2FSK,
    FLEX_3200_2FSK,
    FLEX_3200_4FSK,
    FLEX_6400_4FSK
};

struct FlexStats {
    uint32_t symbols_processed;
    uint32_t total_frames;
    uint32_t correct_frames;
};

struct FlexPacket {
    uint32_t bitrate;  // 1600, 3200, 6400
    uint32_t capcode;
    uint32_t function;  // 0-3
    uint32_t type;      // Message type (e.g. ALN, NUM, etc - could use enum)
    char message[128];  // Decoded message text
    uint32_t status;    // 0=OK, other=Errors
};

} /* namespace flex */

#endif /*__FLEX_DEFS_H__*/
