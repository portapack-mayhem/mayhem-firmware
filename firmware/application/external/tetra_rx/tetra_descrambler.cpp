#include "tetra_descrambler.hpp"

namespace ui::external_app::tetra_rx {

static inline uint8_t lfsr_step(uint32_t& s) {
    uint8_t b = 0;

    b ^= (s >> (32 - 32)) & 1;
    b ^= (s >> (32 - 26)) & 1;
    b ^= (s >> (32 - 23)) & 1;
    b ^= (s >> (32 - 22)) & 1;
    b ^= (s >> (32 - 16)) & 1;
    b ^= (s >> (32 - 12)) & 1;
    b ^= (s >> (32 - 11)) & 1;
    b ^= (s >> (32 - 10)) & 1;
    b ^= (s >> (32 - 8)) & 1;
    b ^= (s >> (32 - 7)) & 1;
    b ^= (s >> (32 - 5)) & 1;
    b ^= (s >> (32 - 4)) & 1;
    b ^= (s >> (32 - 2)) & 1;
    b ^= (s >> (32 - 1)) & 1;

    s = (s >> 1) | (uint32_t(b) << 31);

    return b;
}

void Descrambler::generate(
    uint32_t init,
    uint8_t* seq,
    size_t bits) {
    uint32_t s = init;

    for (size_t i = 0; i < bits; i++)
        seq[i] = lfsr_step(s);
}

void Descrambler::descramble(
    BitVector& bits,
    uint32_t init) {
    uint32_t s = init;

    for (size_t i = 0; i < bits.size(); i++)
        bits.xor_bit(i, lfsr_step(s));
}

uint32_t Descrambler::dmo_scramb_init(
    uint32_t mni,
    uint32_t src) {
    uint32_t init =
        (src & 0xFFFFFF) |
        ((mni & 0x3F) << 24);

    init <<= 2;
    init |= 3;

    return init;
}

}  // namespace ui::external_app::tetra_rx
