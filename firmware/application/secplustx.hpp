#ifndef __SECPLUSTX__
#define __SECPLUSTX__

#include <cstdint>

namespace secplustx {
int8_t encode_v2(const uint32_t rolling, const uint64_t fixed, uint32_t data, const uint8_t frame_type, uint8_t* packet1, uint8_t* packet2);
};

#endif
