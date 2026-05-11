/* Host sanity check for NRZ-M byte decode (same algorithm as meteor_nrzm.cpp). */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void decode(uint8_t* data, int length, uint8_t* last_bit) {
    for (int i = 0; i < length; i++) {
        uint8_t mask = (uint8_t)(((data[i] >> 1) & 0x7F) | ((*last_bit) << 7));
        *last_bit = (uint8_t)(data[i] & 1);
        data[i] ^= mask;
    }
}

int main(void) {
    uint8_t buf[] = {0x55, 0xaa, 0x33};
    uint8_t copy[sizeof buf];
    memcpy(copy, buf, sizeof buf);
    uint8_t lb = 0;
    decode(copy, (int)sizeof copy, &lb);
    printf("nrzm_smoketest_ok len=%zu last_bit=%u out0=%02x\n", sizeof copy, (unsigned)lb, (unsigned)copy[0]);
    return 0;
}
