#ifndef __KEELOQ_COMMON__
#define __KEELOQ_COMMON__

#include <cstdint>

#define bit(x, n) (((x) >> (n)) & 1)
#define g5(x, a, b, c, d, e) \
    (bit(x, a) + bit(x, b) * 2 + bit(x, c) * 4 + bit(x, d) * 8 + bit(x, e) * 16)

#define KEELOQ_NLF 0x3A5C742E

uint32_t keeloq_encrypt(const uint32_t data, const uint64_t key);
uint32_t keeloq_decrypt(const uint32_t data, const uint64_t key);
uint64_t keeloq_normal_learning(uint32_t data, const uint64_t key);

#endif
