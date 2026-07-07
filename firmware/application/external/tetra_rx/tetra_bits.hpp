#pragma once

#include <stdint.h>
#include <stddef.h>

namespace ui::external_app::tetra_rx {

class BitVector {
   public:
    BitVector(uint8_t* p, size_t bits)
        : data_(p),
          bits_(bits) {
    }

    inline bool get(size_t bit) const {
        return (data_[bit >> 3] >>
                (7 - (bit & 7))) &
               1;
    }

    inline void set(size_t bit, bool value) {
        if (value)
            data_[bit >> 3] |=
                1 << (7 - (bit & 7));
        else
            data_[bit >> 3] &=
                ~(1 << (7 - (bit & 7)));
    }

    inline void xor_bit(size_t bit, bool value) {
        if (value)
            data_[bit >> 3] ^=
                1 << (7 - (bit & 7));
    }

    inline size_t size() const {
        return bits_;
    }

   private:
    uint8_t* data_;
    size_t bits_;
};

}  // namespace ui::external_app::tetra_rx
