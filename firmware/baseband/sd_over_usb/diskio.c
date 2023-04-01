#include "diskio.h"

#include "ch.h"
#include "hal.h"

uint32_t get_capacity(void) {
    return mmcsdGetCardCapacity(&SDCD1);
}

bool_t read_block(uint32_t startblk, uint8_t *buf, uint32_t n) {
    return sdcRead(&SDCD1, startblk, buf, n);
}
