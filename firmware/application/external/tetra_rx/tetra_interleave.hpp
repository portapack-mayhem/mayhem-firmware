#pragma once

#include "tetra_bits.hpp"

namespace ui::external_app::tetra_rx {

class Interleave {
   public:
    static void block_deinterleave(
        BitVector& in,
        BitVector& out,
        uint32_t K,
        uint32_t a);
};

}  // namespace ui::external_app::tetra_rx
