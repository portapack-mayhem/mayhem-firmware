/* Packed soft bits. The byte trellis reduces every int8 value to
 * clamp(v >> 5, +-3) anyway, so reducing at the demapper's copy loses nothing
 * and saves ~12.7 KB at 72 CU, enough for a 96 kbit/s subchannel. A code is the
 * level as 4-bit two's complement, 0 the erasure, packed two a byte (index i in
 * byte i / 2, low nibble first); the trellis takes a uint16 per step. */
#ifndef __DAB_SOFT_H__
#define __DAB_SOFT_H__

#include <cstddef>
#include <cstdint>

namespace dab {

constexpr int kSoftShift = 5;
constexpr int kSoftMax = 3;

inline uint8_t soft_code(int8_t v) {
    int32_t q = static_cast<int32_t>(v) >> kSoftShift;
    if (q < -kSoftMax)
        q = -kSoftMax;
    if (q > kSoftMax)
        q = kSoftMax;
    return static_cast<uint8_t>(q) & 0x0Fu;
}

inline int32_t soft_level(uint32_t code) {
    return static_cast<int32_t>(code << 28) >> 28;
}

inline uint8_t soft_get(const uint8_t* run, size_t i) {
    return (i & 1) ? static_cast<uint8_t>(run[i >> 1] >> 4) : static_cast<uint8_t>(run[i >> 1] & 0x0Fu);
}

/* Four int8 soft values (byte k of `w`) to four codes (nibble k), all lanes at
 * once. A byte's top three bits are v >> 5 as a 3-bit code; the one clamp,
 * -4 to -3, is a zero-lane test on t ^ 4 (x + 7 sets bit 3 unless x is 0);
 * bit 2 copied to bit 3 sign-extends; one shift folds lanes 1, 3 into 0, 2.
 * packcheck.cpp checks it against soft_code() on all 2^32 words. */
inline uint32_t soft_codes4(uint32_t w) {
    static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__, "byte k of the word is value k");
    uint32_t t = (w >> 5) & 0x07070707u;
    const uint32_t live = ((t ^ 0x04040404u) + 0x07070707u) & 0x08080808u;
    t |= (~live & 0x08080808u) >> 3;
    t |= (t & 0x04040404u) << 1;
    t |= t >> 4;
    return (t & 0xFFu) | ((t >> 8) & 0xFF00u);
}

/* `values` int8 soft values (a multiple of 4) to a packed code run: the copy
 * every CIF symbol of the subchannel takes into msc_in_. */
inline void pack_codes(const int8_t* src, uint8_t* dst, size_t values) {
    for (size_t n = values / 4; n != 0; --n, src += 4, dst += 2) {
        uint32_t w;
        __builtin_memcpy(&w, src, sizeof(w));
        const uint32_t r = soft_codes4(w);
        dst[0] = static_cast<uint8_t>(r);
        dst[1] = static_cast<uint8_t>(r >> 8);
    }
}

} /* namespace dab */

#endif /*__DAB_SOFT_H__*/
