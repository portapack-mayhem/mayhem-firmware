#pragma once

#include <stdint.h>
#include "tetra_bits.hpp"

namespace ui::external_app::tetra_rx {

class Descrambler {
   public:
    static constexpr uint32_t SCRAMB_INIT_BSCH = 0x00000003;

    static void descramble(
        BitVector& bits,
        uint32_t init = SCRAMB_INIT_BSCH);

    static void generate(
        uint32_t init,
        uint8_t* sequence,
        size_t bits);

    static uint32_t dmo_scramb_init(
        uint32_t mni,
        uint32_t src);
};

}  // namespace ui::external_app::tetra_rx
