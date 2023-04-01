#ifndef __DISKIO_H__
#define __DISKIO_H__

#include <stdint.h>
#include <chtypes.h>

uint32_t get_capacity(void);
bool_t read_block(uint32_t startblk, uint8_t *buf, uint32_t n);
bool_t write_block(uint32_t startblk, uint8_t *buf, uint32_t n);

#endif /* __DISKIO_H__ */