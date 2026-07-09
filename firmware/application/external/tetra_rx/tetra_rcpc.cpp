#include "tetra_rcpc.hpp"
#include <string.h>

namespace ui::external_app::tetra_rx {

static constexpr uint8_t P_RATE_2_3[3] = {1, 2, 5};

static constexpr uint32_t T_RATE_2_3 = 3;
static constexpr uint32_t PERIOD = 8;

void RCPC::depuncture_2_3(BitVector& in, uint8_t* out, uint32_t in_bits) {
    const uint32_t mother_bits = ((in_bits + T_RATE_2_3 - 1) / T_RATE_2_3) * PERIOD + PERIOD;
    memset(out, ERASED, mother_bits);
    for (uint32_t j = 1; j <= in_bits; j++) {
        uint32_t i = j;
        uint32_t k = PERIOD * ((i - 1) / T_RATE_2_3);
        k += P_RATE_2_3[(i - 1) % T_RATE_2_3];
        if (k >= 1 && k <= mother_bits) {
            out[k - 1] = in.get(j - 1);
        }
    }
}

}  // namespace ui::external_app::tetra_rx