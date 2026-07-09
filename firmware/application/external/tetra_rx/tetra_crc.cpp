#include "tetra_crc.hpp"

namespace ui::external_app::tetra_rx {

uint16_t CRC::crc16_itut_bits(
    const uint8_t* bits,
    uint32_t n_bits,
    uint16_t init) {
    uint16_t crc = init;

    for (uint32_t i = 0; i < n_bits; i++) {
        uint8_t bit =
            (bits[i >> 3] >>
             (7 - (i & 7))) &
            1;

        crc ^= uint16_t(bit) << 15;

        if (crc & 0x8000) {
            crc =
                (crc << 1) ^
                0x1021;
        } else {
            crc <<= 1;
        }

        crc &= 0xffff;
    }

    return crc;
}

}  // namespace ui::external_app::tetra_rx
