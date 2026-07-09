#pragma once

#include <stdint.h>
#include "tetra_bits.hpp"

namespace ui::external_app::tetra_rx {

class RCPC {
   public:
    static constexpr uint8_t ERASED = 0xff;

    static void depuncture_2_3(
        BitVector& in,
        uint8_t* out,
        uint32_t in_bits);
};

}  // namespace ui::external_app::tetra_rx
