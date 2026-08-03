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
#include <stdbool.h>

#ifdef PRALINE

/*
 * RX path.
 *
 * NOTE: the register map below is the one the loaded bitstream actually
 * implements. Ground truth is hackrf/firmware/fpga/top/standard.py plus
 * hackrf/firmware/common/fpga_regs.def; Mayhem, hackrf_usb and debug_for_adsb
 * all load a byte-identical praline_fpga.bin, so that map applies here too.
 *
 * An earlier "legacy PRALINE software map" claimed register 0x03 was an RX
 * digital gain and 0x04/0x05 were DC-block width / adaptation rate. The
 * gateware has none of those: 0x03 is rx_pstep (whose top two bits are the
 * quarter-rate shift) and 0x04/0x05 are TX registers. Writing the old "RX DC
 * width" value of 0x01 to 0x04 actually set tx_ctrl[0] and switched the TX NCO
 * on, and every write of the fictional digital gain to 0x03 cleared the
 * quarter shift.
 */
#define FPGA_REG_RX_PSTEP 0x03 /* RX phase step; bits [7:6] = quarter shift */

/*
 * TX path must match the currently built PRALINE standard gateware in
 * hackrf/firmware/fpga/top/standard.py:
 *   0x04 tx_ctrl
 *   0x05 tx_intrp
 *   0x06 tx_pstep
 */
#define FPGA_REG_TX_CONTROL 0x04
#define FPGA_REG_TX_INTERP 0x05
#define FPGA_REG_TX_PHASE_STEP 0x06

/*
 * FPGA Operating Mode
 */
typedef enum {
    FPGA_MODE_OFF = 0,
    FPGA_MODE_RX,
    FPGA_MODE_TX
} fpga_mode_t;

/*
 * FPGA Register Addresses
 *
 * From hackrf/firmware/fpga/top/standard.py (spi_regs.add_register):
 *   0x01 ctrl      8 bits
 *   0x02 rx_decim  3 bits
 *   0x03 rx_pstep  8 bits
 *   0x04 tx_ctrl   1 bit
 *   0x05 tx_intrp  3 bits
 *   0x06 tx_pstep  8 bits
 *
 * There is no RX gain register: rx_decim selects half-band FIR stages
 * (hbfir1..hbfir5), which are unity-gain, so there is no CIC bit growth to
 * renormalise.
 */
#define FPGA_REG_CTRL 0x01     /* Control register */
#define FPGA_REG_DECIM 0x02    /* RX decimation (log2, bits [2:0]) */
#define FPGA_REG_SHARED_3 0x03 /* rx_pstep */
#define FPGA_REG_SHARED_4 0x04 /* tx_ctrl */
#define FPGA_REG_SHARED_5 0x05 /* tx_intrp */
#define FPGA_REG_SHARED_6 0x06 /* tx_pstep */

/*
 * Register 1 (CTRL) Bit Definitions
 * standard.py: ctrl[0] -> dc_block.enable, ctrl[6] -> prbs, ctrl[7] -> trigger_en.
 * Nothing else in this register is decoded.
 */
#define FPGA_CTRL_DC_BLOCK_EN (1 << 0) /* DC block enable */
#define FPGA_CTRL_PRBS_EN (1 << 6)     /* PRBS test mode */
#define FPGA_CTRL_TRIGGER_EN (1 << 7)  /* External trigger enable */

/*
 * Register 3 (RX_PSTEP) Bit Definitions
 * standard.py: rx_pstep[6] -> quarter_shift.enable, rx_pstep[7] -> quarter_shift.up.
 * Same encoding as fpga_quarter_shift_mode_t << 6 in hackrf/firmware/common/fpga.c.
 */
#define FPGA_RX_QUARTER_SHIFT_SHIFT 6
#define FPGA_RX_QUARTER_SHIFT_MASK 0xC0
#define FPGA_RX_QUARTER_SHIFT_NONE 0x00 /* mode 0b00 */
#define FPGA_RX_QUARTER_SHIFT_UP 0xC0   /* mode 0b11 */
#define FPGA_RX_QUARTER_SHIFT_DOWN 0x40 /* mode 0b01 */

/*
 * Register 4 (TX_CTRL) / 5 (TX_INTRP) / 6 (TX_PSTEP) Bit Definitions
 */
#define FPGA_REG3_TX_NCO_CTRL 0x04 /* tx_ctrl holds the NCO enable */
#define FPGA_TX_NCO_EN (1 << 0)    /* NCO enable */
#define FPGA_REG4_TX_INTERP 0x05
#define FPGA_TX_INTERP_MASK 0x07 /* Bits [2:0] */
#define FPGA_REG5_TX_PHASE_STEP 0x06
#define FPGA_TX_PHASE_STEP_MASK 0xFF /* Bits [7:0] */

/*
 * Core Functions
 */

/* Initialize the FPGA - loads bitstream from SPIFI flash
 * Returns: 0 on success, non-zero on failure */
int fpga_bridge_init(uint8_t* mem_base);

/* Set operating mode - MUST be called before using mode-specific functions
 * This ensures registers 3-5 are interpreted correctly */
void fpga_set_mode(fpga_mode_t mode);

/* Get current operating mode */
fpga_mode_t fpga_get_mode(void);

/*
 * Low-Level Register Access (use with caution)
 */
uint8_t fpga_register_read(uint8_t reg);
void fpga_register_write(uint8_t reg, uint8_t value);

/*
 * RX Mode Functions (only valid when mode == FPGA_MODE_RX)
 */
void fpga_rx_set_decimation(uint8_t ratio);
/* mode is the gateware encoding: 0b00 none, 0b11 up, 0b01 down. */
void fpga_rx_set_quarter_shift_mode(uint8_t mode);
void fpga_rx_enable_dc_block(bool enable);

/*
 * TX Mode Functions (only valid when mode == FPGA_MODE_TX)
 */
void fpga_tx_set_interpolation(uint8_t ratio);
void fpga_tx_set_nco_enable(bool enable);
void fpga_tx_set_phase_step(uint8_t step);

/*
 * Debug Functions
 */

/*
 * Read an FPGA register via SPI
 * reg: Register number (1-5)
 * Returns: Register value, or 0xFF if invalid register
 *
 * FPGA Register Map (hackrf/firmware/fpga/top/standard.py):
 *   Reg 1 (CTRL):     DC_BLOCK(b0), PRBS(b6), TRIGGER_EN(b7)
 *   Reg 2 (RX_DECIM): Decimation ratio, log2 [2:0]
 *   Reg 3 (RX_PSTEP): QUARTER_SHIFT_EN(b6), QUARTER_SHIFT_UP(b7)
 *   Reg 4 (TX_CTRL):  NCO_EN(b0)
 *   Reg 5 (TX_INTRP): Interpolation ratio, log2 [2:0]
 *   Reg 6 (TX_PSTEP): NCO phase step [7:0]
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
