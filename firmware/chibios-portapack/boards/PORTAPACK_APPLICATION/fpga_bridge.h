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

/* FPGA Register Map Address 0x03 (Dual Purpose) */
#define FPGA_REG_RX_DIGITAL_GAIN 0x03 /* Digital Shift / scaling (RX Mode) */
#define FPGA_REG_TX_CONTROL 0x03      /* NCO_EN and TX flags (TX Mode) */

/* FPGA Register Map Address 0x04 (Shared) */
#define FPGA_REG_RX_DC_BLOCK_WIDTH 0x04 /* Notch filter cutoff (RX Mode) */
#define FPGA_REG_TX_INTERP 0x04         /* Interpolation ratio (TX Mode) */

/* FPGA Register Map Address 0x05 (Shared) */
#define FPGA_REG_RX_DC_ADAPT_RATE 0x05 /* Settle time/Integration (RX Mode) */
#define FPGA_REG_TX_PHASE_STEP 0x05    /* NCO frequency step (TX Mode) */

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
 * Note: Registers 3-5 are dual-purpose (meaning depends on RX/TX mode)
 */
#define FPGA_REG_CTRL 0x01     /* Control register */
#define FPGA_REG_DECIM 0x02    /* Decimation (RX) / unused (TX) */
#define FPGA_REG_SHARED_3 0x03 /* Dual-purpose register */
#define FPGA_REG_SHARED_4 0x04 /* Dual-purpose register */
#define FPGA_REG_SHARED_5 0x05 /* Dual-purpose register */

/*
 * Register 1 (CTRL) Bit Definitions
 */
#define FPGA_CTRL_DC_BLOCK_EN (1 << 0)      /* DC block enable */
#define FPGA_CTRL_QUARTER_SHIFT_EN (1 << 1) /* Quarter-rate shift enable */
#define FPGA_CTRL_QUARTER_SHIFT_UP (1 << 2) /* Shift direction: 1=up, 0=down */
#define FPGA_CTRL_TX_MODE (1 << 5)          /* TX mode indicator (if applicable) */
#define FPGA_CTRL_PRBS_EN (1 << 6)          /* PRBS test mode */
#define FPGA_CTRL_TRIGGER_EN (1 << 7)       /* External trigger enable */

/*
 * Register 3 Dual-Purpose Definitions
 */
/* RX Mode: Digital gain/shift */
#define FPGA_REG3_RX_DIGITAL_GAIN 0x03
#define FPGA_RX_GAIN_SHIFT_MASK 0x0F /* Bits [3:0] - shift amount */

/* TX Mode: NCO control */
#define FPGA_REG3_TX_NCO_CTRL 0x03
#define FPGA_TX_NCO_EN (1 << 0)     /* NCO enable */
#define FPGA_TX_NCO_INVERT (1 << 1) /* Invert spectrum */

/*
 * Register 4 Dual-Purpose Definitions
 */
/* RX Mode: DC block notch width */
#define FPGA_REG4_RX_DC_WIDTH 0x04
#define FPGA_RX_DC_WIDTH_MASK 0x07 /* Bits [2:0] */

/* TX Mode: Interpolation ratio */
#define FPGA_REG4_TX_INTERP 0x04
#define FPGA_TX_INTERP_MASK 0x07 /* Bits [2:0] */

/*
 * Register 5 Dual-Purpose Definitions
 */
/* RX Mode: DC block adaptation rate */
#define FPGA_REG5_RX_DC_RATE 0x05
#define FPGA_RX_DC_RATE_MASK 0xFF /* Bits [7:0] */

/* TX Mode: NCO phase step (frequency) */
#define FPGA_REG5_TX_PHASE_STEP 0x05
#define FPGA_TX_PHASE_STEP_MASK 0xFF /* Bits [7:0] */

/* Export default values so other methods can use them */
#define FPGA_RX_DEFAULT_DIGITAL_GAIN 0x00
#define FPGA_RX_DEFAULT_DC_WIDTH 0x04
#define FPGA_RX_DEFAULT_ADAPT_RATE 0x08

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
void fpga_rx_set_digital_gain(uint8_t shift);
void fpga_rx_set_dc_block_width(uint8_t width);
void fpga_rx_set_dc_adapt_rate(uint8_t rate);
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
 * FPGA Register Map:
 *   Reg 1 (CTRL):     DC_BLOCK(b0), QUARTER_SHIFT_EN(b1), QUARTER_SHIFT_UP(b2), PRBS(b6), TRIGGER_EN(b7)
 *   Reg 2 (RX_DECIM): Decimation ratio [2:0]
 *   Reg 3 (RX/TX):    RX Digital Shift OR TX NCO Control
 *   Reg 4 (RX_DC_BLOCK_WIDTH/TX_INTERP) [2:0]
 *   Reg 5 (RX_DC_ADAPT_RATE/TX_PSTEP)  [7:0]
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
