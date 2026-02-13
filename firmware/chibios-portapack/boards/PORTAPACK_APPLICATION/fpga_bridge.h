/*
 * FPGA Bridge Header - PRALINE iCE40 FPGA interface
 *
 * Provides functions for initializing and accessing the FPGA on HackRF Pro (PRALINE).
 */

#ifndef __FPGA_BRIDGE_H__
#define __FPGA_BRIDGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifdef PRALINE

/*
 * Initialize the FPGA - loads bitstream from SPIFI flash
 * Returns: 0 on success, non-zero on failure
 */
int fpga_bridge_init(void);

/*
 * Read an FPGA register via SPI
 * reg: Register number (1-5)
 * Returns: Register value, or 0xFF if invalid register
 *
 * FPGA Register Map:
 *   Reg 1 (CTRL):     DC_BLOCK(b0), QUARTER_SHIFT_EN(b1), QUARTER_SHIFT_UP(b2), PRBS(b6), TRIGGER_EN(b7)
 *   Reg 2 (RX_DECIM): Decimation ratio [2:0]
 *   Reg 3 (TX_CTRL):  NCO_EN(b0)
 *   Reg 4 (TX_INTRP): Interpolation ratio [2:0]
 *   Reg 5 (TX_PSTEP): NCO phase step [7:0]
 */
uint8_t fpga_debug_register_read(uint8_t reg);

/*
 * Write an FPGA register via SPI
 * reg: Register number (1-5)
 * value: Value to write
 */
void fpga_debug_register_write(uint8_t reg, uint8_t value);

#endif /* PRALINE */

#ifdef __cplusplus
}
#endif

#endif /* __FPGA_BRIDGE_H__ */
