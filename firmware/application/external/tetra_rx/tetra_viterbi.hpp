#pragma once

#include <stdint.h>
#include <string.h>

namespace ui::external_app::tetra_rx {

class Viterbi {
   public:
    int decode_cch(
        const uint8_t* mother,
        uint32_t n_info,
        uint8_t* bits,
        uint8_t* trace_buffer);
};

}  // namespace ui::external_app::tetra_rx