#include "tetra_interleave.hpp"

namespace ui::external_app::tetra_rx {

void Interleave::block_deinterleave(
    BitVector& in,
    BitVector& out,
    uint32_t K,
    uint32_t a) {
    for (uint32_t i = 1; i <= K; i++) {
        uint32_t k =
            1 +
            ((a * i) % K);

        out.set(
            i - 1,
            in.get(k - 1));
    }
}

}  // namespace ui::external_app::tetra_rx
