#pragma once

#include <stdint.h>

namespace ui::external_app::tetra_rx {

class CRC {
   public:
    static constexpr uint16_t CRC_OK = 0x1d0f;

    static uint16_t crc16_itut_bits(
        const uint8_t* bits,
        uint32_t n_bits,
        uint16_t init = 0xffff);
};

}  // namespace ui::external_app::tetra_rx
