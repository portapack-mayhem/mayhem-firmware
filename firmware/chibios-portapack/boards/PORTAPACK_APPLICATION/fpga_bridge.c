// This bridge allows calls for functions in their native C context
// HackRF headers here - C for use within the C++ board.cpp context

// Check if PRALINE was passed from CMake
#ifdef PRALINE
#include "fpga_bridge.h"

// Necessary headers
#include "lz4_blk.h"

// LIBOPENCM3 Headers (only CGU for clock setup)
#include <libopencm3/lpc43xx/cgu.h>

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

// SPIFI memory-mapped base address
// Flash is mapped starting at 0x14000000
// FPGA bitstream at flash address 0x380000 = memory address 0x14380000
// PRALINE: Moved to 1.5MB offset to allow larger base firmware
#define SPIFI_DATA_BASE 0x14000000
#define FPGA_BITSTREAM_FLASH_ADDR 0x380000  // Was 0x100000 (1MB), then 0x180000 (1.5MB), now 0x380000 (3.5MB)
#define FPGA_BITSTREAM_MEM_ADDR (SPIFI_DATA_BASE + FPGA_BITSTREAM_FLASH_ADDR)

// MMIO32 direct register access
#define MMIO32_LOCAL(addr) (*(volatile uint32_t*)(addr))

// SSP1 base address
#define SSP1_BASE_LOCAL 0x400C5000

// SSP register offsets
#define SSP_CR0_OFF 0x000
#define SSP_CR1_OFF 0x004
#define SSP_DR_OFF 0x008
#define SSP_SR_OFF 0x00C
#define SSP_CPSR_OFF 0x010

// SSP register access
#define SSP1_CR0_LOCAL MMIO32_LOCAL(SSP1_BASE_LOCAL + SSP_CR0_OFF)
#define SSP1_CR1_LOCAL MMIO32_LOCAL(SSP1_BASE_LOCAL + SSP_CR1_OFF)
#define SSP1_DR_LOCAL MMIO32_LOCAL(SSP1_BASE_LOCAL + SSP_DR_OFF)
#define SSP1_SR_LOCAL MMIO32_LOCAL(SSP1_BASE_LOCAL + SSP_SR_OFF)
#define SSP1_CPSR_LOCAL MMIO32_LOCAL(SSP1_BASE_LOCAL + SSP_CPSR_OFF)

// SSP status bits
#define SSP_SR_TNF_LOCAL (1 << 1)  // TX FIFO not full
#define SSP_SR_RNE_LOCAL (1 << 2)  // RX FIFO not empty
#define SSP_SR_BSY_LOCAL (1 << 4)  // Busy

// SSP CR0 bits
#define SSP_CR0_DSS_8BIT (0x7)  // 8-bit data
#define SSP_CR0_FRF_SPI (0x0)   // SPI frame format
#define SSP_CR0_CPOL (1 << 6)   // Clock polarity
#define SSP_CR0_CPHA (1 << 7)   // Clock phase

// SSP CR1 bits
#define SSP_CR1_SSE (1 << 1)  // SSP enable

// SCU pin configuration registers
#define PERIPH_BASE_APB0_LOCAL 0x40080000
#define SCU_BASE_LOCAL (PERIPH_BASE_APB0_LOCAL + 0x06000)
#define PIN_GROUP1_LOCAL (SCU_BASE_LOCAL + 0x080)
#define PIN_GROUP4_LOCAL (SCU_BASE_LOCAL + 0x200)
#define PIN_GROUP5_LOCAL (SCU_BASE_LOCAL + 0x280)
#define PIN3_LOCAL 0x00C
#define PIN4_LOCAL 0x010
#define PIN1_LOCAL 0x004
#define PIN2_LOCAL 0x008
#define PIN10_LOCAL 0x028
#define PIN19_LOCAL 0x04C

// SCU configuration flags
#define SCU_CONF_EPUN_DIS_PULLUP_LOCAL (1 << 4)
#define SCU_CONF_EHS_FAST_LOCAL (1 << 5)
#define SCU_CONF_EZI_EN_IN_BUFFER_LOCAL (1 << 6)
#define SCU_CONF_ZIF_DIS_IN_GLITCH_FILT_LOCAL (1 << 7)
#define SCU_GPIO_FAST_LOCAL (SCU_CONF_EPUN_DIS_PULLUP_LOCAL |  \
                             SCU_CONF_EHS_FAST_LOCAL |         \
                             SCU_CONF_EZI_EN_IN_BUFFER_LOCAL | \
                             SCU_CONF_ZIF_DIS_IN_GLITCH_FILT_LOCAL)
#define SCU_SSP_IO_LOCAL SCU_GPIO_FAST_LOCAL

// Function select values
#define SCU_CONF_FUNCTION0_LOCAL (0x0)
#define SCU_CONF_FUNCTION1_LOCAL (0x1)
#define SCU_CONF_FUNCTION4_LOCAL (0x4)
#define SCU_CONF_FUNCTION5_LOCAL (0x5)
#define SCU_GPIO_NOPULL_LOCAL (SCU_CONF_EZI_EN_IN_BUFFER_LOCAL | SCU_CONF_ZIF_DIS_IN_GLITCH_FILT_LOCAL)
#define SCU_GPIO_PUP_LOCAL (SCU_CONF_EZI_EN_IN_BUFFER_LOCAL)

// SSP1 pins (for FPGA programming)
#define SCU_SSP1_CIPO_LOCAL (PIN_GROUP1_LOCAL + PIN3_LOCAL)  // P1_3
#define SCU_SSP1_COPI_LOCAL (PIN_GROUP1_LOCAL + PIN4_LOCAL)  // P1_4
#define SCU_SSP1_SCK_LOCAL (PIN_GROUP1_LOCAL + PIN19_LOCAL)  // P1_19

// FPGA control pins
#define SCU_FPGA_CRESET_LOCAL (PIN_GROUP5_LOCAL + PIN2_LOCAL)  // P5_2 GPIO2[11]
#define SCU_FPGA_CDONE_LOCAL (PIN_GROUP4_LOCAL + PIN10_LOCAL)  // P4_10 GPIO5[14]
#define SCU_FPGA_SPI_CS_LOCAL (PIN_GROUP5_LOCAL + PIN1_LOCAL)  // P5_1 GPIO2[10]

// GPIO register addresses for direct MMIO access
#define GPIO_LPC_BASE_LOCAL 0x400F4000
#define GPIO_DIR_BASE (GPIO_LPC_BASE_LOCAL + 0x2000)  // Direction registers
#define GPIO_SET_BASE (GPIO_LPC_BASE_LOCAL + 0x2200)  // Set registers
#define GPIO_CLR_BASE (GPIO_LPC_BASE_LOCAL + 0x2280)  // Clear registers
#define GPIO_PIN_BASE (GPIO_LPC_BASE_LOCAL + 0x2100)  // Pin read registers

// GPIO port access macros
#define GPIO_DIR(port) MMIO32_LOCAL(GPIO_DIR_BASE + (port) * 4)
#define GPIO_SET(port) MMIO32_LOCAL(GPIO_SET_BASE + (port) * 4)
#define GPIO_CLR(port) MMIO32_LOCAL(GPIO_CLR_BASE + (port) * 4)
#define GPIO_PIN(port) MMIO32_LOCAL(GPIO_PIN_BASE + (port) * 4)

// FPGA control GPIO pins
// GPIO2[11] = CRESET, GPIO5[14] = CDONE, GPIO2[10] = SPI_CS
#define FPGA_CRESET_PORT 2
#define FPGA_CRESET_PIN 11
#define FPGA_CDONE_PORT 5
#define FPGA_CDONE_PIN 14
#define FPGA_SPI_CS_PORT 2
#define FPGA_SPI_CS_PIN 10

// ============================================================================
// Canonical Default Values - SINGLE SOURCE OF TRUTH
// ============================================================================
/* Define the canonical RX defaults in ONE place */
#define FPGA_RX_DEFAULT_DC_WIDTH 0x04     /* Typical for 40MHz stability */
#define FPGA_RX_DEFAULT_ADAPT_RATE 0x08   /* Typical for 40MHz stability */
#define FPGA_RX_DEFAULT_DIGITAL_GAIN 0x00 /* No shift initially */

/* Define TX defaults */
#define FPGA_TX_DEFAULT_NCO_CTRL 0x00   /* NCO disabled */
#define FPGA_TX_DEFAULT_INTERP 0x00     /* No interpolation */
#define FPGA_TX_DEFAULT_PHASE_STEP 0x00 /* Zero phase step */

// ============================================================================
// Static Variables - Declare BEFORE use
// ============================================================================
static fpga_mode_t current_mode = FPGA_MODE_OFF;
// Cached register values for debug reads (since reads may require mode switch)
static uint8_t fpga_reg_cache[6] = {0, 0x01, 0x00, 0x00, 0x00, 0x00};

// Context structure for SPIFI-based reading
struct spifi_fpga_read_ctx {
    const uint8_t* mem_ptr;  // Current read position in SPIFI memory
    size_t next_block_sz;
    uint8_t init_flag;
    uint8_t buffer[4128 + 2];  // Compressed block + next size
};

// ============================================================================
// Low-Level Helper Functions
// ============================================================================

// Simple delay loop
static void delay_cycles(volatile uint32_t count) {
    while (count--) {
        __asm__ volatile("nop");
    }
}

// Microsecond delay (approximate, assuming ~200MHz clock)
static void delay_us(uint32_t us) {
    // ~50 cycles per microsecond at 200MHz
    delay_cycles(us * 50);
}

// SSP1 transfer one byte
static uint8_t ssp1_transfer_byte(uint8_t data) {
    // Wait for TX FIFO not full
    while ((SSP1_SR_LOCAL & SSP_SR_TNF_LOCAL) == 0) {
    }
    SSP1_DR_LOCAL = data;
    // Wait for not busy
    while (SSP1_SR_LOCAL & SSP_SR_BSY_LOCAL) {
    }
    // Wait for RX FIFO not empty
    while ((SSP1_SR_LOCAL & SSP_SR_RNE_LOCAL) == 0) {
    }
    return SSP1_DR_LOCAL;
}

// Configure SSP1 for iCE40 programming (SPI mode 3: CPOL=1, CPHA=1)
static void ssp1_init_ice40(void) {
    // Disable SSP1 first
    SSP1_CR1_LOCAL = 0;

    // Configure: 8-bit, SPI mode 3 (CPOL=1, CPHA=1), master mode
    // SCR=21 for ~4MHz at 200MHz clock
    SSP1_CR0_LOCAL = SSP_CR0_DSS_8BIT | SSP_CR0_FRF_SPI | SSP_CR0_CPOL | SSP_CR0_CPHA | (21 << 8);

    // Clock prescaler = 2 (divide by 2)
    SSP1_CPSR_LOCAL = 2;

    // Enable SSP1
    SSP1_CR1_LOCAL = SSP_CR1_SSE;
}

// Configure SSP1 pins via SCU
static void configure_ssp1_pins(void) {
    // P1_3 = SSP1_MISO (function 5)
    MMIO32_LOCAL(SCU_SSP1_CIPO_LOCAL) = SCU_SSP_IO_LOCAL | SCU_CONF_FUNCTION5_LOCAL;
    // P1_4 = SSP1_MOSI (function 5)
    MMIO32_LOCAL(SCU_SSP1_COPI_LOCAL) = SCU_SSP_IO_LOCAL | SCU_CONF_FUNCTION5_LOCAL;
    // P1_19 = SSP1_SCK (function 1)
    MMIO32_LOCAL(SCU_SSP1_SCK_LOCAL) = SCU_SSP_IO_LOCAL | SCU_CONF_FUNCTION1_LOCAL;
}

// Configure FPGA control pins via SCU and GPIO
static void configure_fpga_control_pins(void) {
    // P5_2 = GPIO2[11] = CRESET (function 0, output)
    MMIO32_LOCAL(SCU_FPGA_CRESET_LOCAL) = SCU_GPIO_NOPULL_LOCAL | SCU_CONF_FUNCTION0_LOCAL;
    // P4_10 = GPIO5[14] = CDONE (function 4, input with pullup)
    MMIO32_LOCAL(SCU_FPGA_CDONE_LOCAL) = SCU_GPIO_PUP_LOCAL | SCU_CONF_FUNCTION4_LOCAL;
    // P5_1 = GPIO2[10] = SPI_CS (function 0, output)
    MMIO32_LOCAL(SCU_FPGA_SPI_CS_LOCAL) = SCU_GPIO_NOPULL_LOCAL | SCU_CONF_FUNCTION0_LOCAL;

    // Set CRESET and SPI_CS as outputs (GPIO2[11] and GPIO2[10])
    GPIO_DIR(FPGA_CRESET_PORT) |= (1 << FPGA_CRESET_PIN) | (1 << FPGA_SPI_CS_PIN);
    // Clear both initially
    GPIO_CLR(FPGA_CRESET_PORT) = (1 << FPGA_CRESET_PIN) | (1 << FPGA_SPI_CS_PIN);

    // CDONE is input (GPIO5[14])
    GPIO_DIR(FPGA_CDONE_PORT) &= ~(1 << FPGA_CDONE_PIN);
}

// GPIO control helpers
static void fpga_creset_low(void) {
    GPIO_CLR(FPGA_CRESET_PORT) = (1 << FPGA_CRESET_PIN);
}
static void fpga_creset_high(void) {
    GPIO_SET(FPGA_CRESET_PORT) = (1 << FPGA_CRESET_PIN);
}
static void fpga_cs_low(void) {
    GPIO_CLR(FPGA_SPI_CS_PORT) = (1 << FPGA_SPI_CS_PIN);
}
static void fpga_cs_high(void) {
    GPIO_SET(FPGA_SPI_CS_PORT) = (1 << FPGA_SPI_CS_PIN);
}
static bool fpga_cdone_read(void) {
    return (GPIO_PIN(FPGA_CDONE_PORT) & (1 << FPGA_CDONE_PIN)) != 0;
}

// ============================================================================
// FPGA Register Access via SPI (iCE40)
// ============================================================================
// These functions allow reading/writing FPGA internal registers via SPI.
// The FPGA bitstream implements a simple SPI register interface.
//
// FPGA Register Map:
//   Reg 1 (CTRL):    DC_BLOCK(b0), QUARTER_SHIFT_EN(b1), QUARTER_SHIFT_UP(b2), PRBS(b6), TRIGGER_EN(b7)
//   Reg 2 (RX_DECIM): Decimation ratio [2:0]
//   Reg 3 (RX/TX):    RX Digital Shift OR TX NCO Control
//   Reg 4 (RX_DC_BLOCK_WIDTH/TX_INTERP) [2:0]
//   Reg 5 (RX_DC_ADAPT_RATE/TX_PSTEP)  [7:0]
//
// SPI Protocol:
//   Read:  Send [reg & 0x7F, 0x00, 0x00] -> value in byte 3
//   Write: Send [(reg | 0x80), value, 0x00]

// ============================================================================
// SPI Mode Switching
// ============================================================================

// Configure SSP1 for iCE40 FPGA register access (Mode 3, 8-bit)
static void ssp1_set_mode_ice40(void) {
    SSP1_CR1_LOCAL = 0;  // Disable SSP1
    SSP1_CR0_LOCAL = SSP_CR0_DSS_8BIT | SSP_CR0_FRF_SPI | SSP_CR0_CPOL | SSP_CR0_CPHA | (21 << 8);
    SSP1_CPSR_LOCAL = 2;
    SSP1_CR1_LOCAL = SSP_CR1_SSE;  // Enable SSP1
}

// Configure SSP1 back to MAX2831 mode (Mode 0, 9-bit)
static void ssp1_set_mode_max2831(void) {
    SSP1_CR1_LOCAL = 0;          // Disable SSP1
    SSP1_CR0_LOCAL = (0x08) |    // 9-bit data (DSS = 0x08)
                     (0x00) |    // SPI frame format
                     (0 << 6) |  // CPOL = 0 (Mode 0)
                     (0 << 7) |  // CPHA = 0 (Mode 0)
                     (21 << 8);  // SCR = 21
    SSP1_CPSR_LOCAL = 2;
    SSP1_CR1_LOCAL = SSP_CR1_SSE;  // Enable SSP1
}

// ============================================================================
// Low-Level SPI Register Access (internal, no mode switch)
// ============================================================================

// Read an FPGA register via SPI
static uint8_t fpga_spi_read(uint8_t reg) {
    uint8_t value;
    fpga_cs_low();
    ssp1_transfer_byte(reg & 0x7F);    // Clear MSB for read
    ssp1_transfer_byte(0x00);          // Dummy byte
    value = ssp1_transfer_byte(0x00);  // Read value
    fpga_cs_high();
    return value;
}

// Write an FPGA register via SPI
static void fpga_spi_write(uint8_t reg, uint8_t value) {
    fpga_cs_low();
    ssp1_transfer_byte((reg & 0x7F) | 0x80);  // Set MSB for write
    ssp1_transfer_byte(value);
    ssp1_transfer_byte(0x00);  // Dummy byte
    fpga_cs_high();
}

// ============================================================================
// Public Debug Functions (switch SPI mode, access register, switch back)
// ============================================================================

// Public function to read FPGA register (callable from C++ application code)
// Switches SPI mode, reads register, switches back
uint8_t fpga_debug_register_read(uint8_t reg) {
    if (reg == 0 || reg > 5) return 0xFF;

    uint8_t value;
    ssp1_set_mode_ice40();
    value = fpga_spi_read(reg);
    ssp1_set_mode_max2831();

    fpga_reg_cache[reg] = value;
    return value;
}

// Public function to write FPGA register (callable from C++ application code)
void fpga_debug_register_write(uint8_t reg, uint8_t value) {
    if (reg == 0 || reg > 5) return;

    ssp1_set_mode_ice40();
    fpga_spi_write(reg, value);
    ssp1_set_mode_max2831();

    fpga_reg_cache[reg] = value;
}

// ============================================================================
// Public Register Access (wraps debug functions)
// ============================================================================

uint8_t fpga_register_read(uint8_t reg) {
    if (reg == 0 || reg > 5) return 0xFF;

    ssp1_set_mode_ice40();
    uint8_t val = fpga_spi_read(reg);
    ssp1_set_mode_max2831();

    fpga_reg_cache[reg] = val;
    return val;
}

void fpga_register_write(uint8_t reg, uint8_t value) {
    if (reg == 0 || reg > 5) return;

    ssp1_set_mode_ice40();
    fpga_spi_write(reg, value);
    ssp1_set_mode_max2831();

    fpga_reg_cache[reg] = value;
}

// ============================================================================
// Mode Management
// ============================================================================

fpga_mode_t fpga_get_mode(void) {
    return current_mode;
}

/* fpga_set_mode with consistent values */
void fpga_set_mode(fpga_mode_t mode) {
    current_mode = mode;
}

// ============================================================================
// RX Mode Functions (with mode assertion)
// ============================================================================

/* RX decimation */
void fpga_rx_set_decimation(uint8_t ratio) {
    if (current_mode != FPGA_MODE_RX) return;
    fpga_register_write(FPGA_REG_DECIM, ratio & 0x07);
}

/* RX DC block enable (bit 0 of register 1) */
void fpga_rx_enable_dc_block(bool enable) {
    if (current_mode != FPGA_MODE_RX) return;
    uint8_t ctrl = fpga_register_read(FPGA_REG_CTRL);
    if (enable)
        ctrl |= FPGA_CTRL_DC_BLOCK_EN;
    else
        ctrl &= ~FPGA_CTRL_DC_BLOCK_EN;
    fpga_register_write(FPGA_REG_CTRL, ctrl);
}

/* RX Functions with mode assertion */
void fpga_rx_set_digital_gain(uint8_t shift) {
    if (current_mode != FPGA_MODE_RX) {
        /* Log error or assert - wrong mode! */
        return;
    }
    fpga_register_write(FPGA_REG_SHARED_3, shift & FPGA_RX_GAIN_SHIFT_MASK);
}

void fpga_rx_set_dc_block_width(uint8_t width) {
    if (current_mode != FPGA_MODE_RX) return;
    fpga_register_write(FPGA_REG_SHARED_4, width & FPGA_RX_DC_WIDTH_MASK);
}

void fpga_rx_set_dc_adapt_rate(uint8_t rate) {
    if (current_mode != FPGA_MODE_RX) return;
    fpga_register_write(FPGA_REG_SHARED_5, rate);
}

// ============================================================================
// TX Mode Functions (with mode assertion)
// ============================================================================

/* TX Functions with mode assertion */
void fpga_tx_set_nco_enable(bool enable) {
    if (current_mode != FPGA_MODE_TX) return;
    uint8_t val = fpga_register_read(FPGA_REG_SHARED_3);
    if (enable)
        val |= FPGA_TX_NCO_EN;
    else
        val &= ~FPGA_TX_NCO_EN;
    fpga_register_write(FPGA_REG_SHARED_3, val);
}

void fpga_tx_set_interpolation(uint8_t ratio) {
    if (current_mode != FPGA_MODE_TX) return;
    fpga_register_write(FPGA_REG_SHARED_4, ratio & FPGA_TX_INTERP_MASK);
}

void fpga_tx_set_phase_step(uint8_t step) {
    if (current_mode != FPGA_MODE_TX) return;
    fpga_register_write(FPGA_REG_SHARED_5, step);
}

// ============================================================================
// FPGA Register Initialization (called after bitstream load)
// ============================================================================

// Initialize FPGA registers after bitstream load
// This is equivalent to fpga_init() in the reference HackRF firmware
/* fpga_register_init with consistent values */
static void fpga_register_init(void) {
    /* Boot into RX mode */
    current_mode = FPGA_MODE_RX;

    fpga_spi_write(FPGA_REG_CTRL, FPGA_CTRL_DC_BLOCK_EN);
    fpga_spi_write(FPGA_REG_DECIM, 0x00);
    fpga_spi_write(FPGA_REG_SHARED_3, FPGA_RX_DEFAULT_DIGITAL_GAIN);
    fpga_spi_write(FPGA_REG_SHARED_4, FPGA_RX_DEFAULT_DC_WIDTH);
    fpga_spi_write(FPGA_REG_SHARED_5, FPGA_RX_DEFAULT_ADAPT_RATE);

    /* Update cache */
    fpga_reg_cache[1] = FPGA_CTRL_DC_BLOCK_EN;
    fpga_reg_cache[2] = 0x00;
    fpga_reg_cache[3] = FPGA_RX_DEFAULT_DIGITAL_GAIN;
    fpga_reg_cache[4] = FPGA_RX_DEFAULT_DC_WIDTH;
    fpga_reg_cache[5] = FPGA_RX_DEFAULT_ADAPT_RATE;
}

// ============================================================================
// LZ4 Decompression for FPGA Bitstream
// ============================================================================

// SPIFI-based read callback for LZ4 decompression
// Reads from SPIFI memory-mapped address instead of using SPI flash driver
static size_t spifi_fpga_read_block_cb(void* _ctx, uint8_t* out_buffer) {
    struct spifi_fpga_read_ctx* ctx = (struct spifi_fpga_read_ctx*)_ctx;
    size_t block_sz = ctx->next_block_sz;

    // First iteration: read first block size from SPIFI memory
    if (ctx->init_flag == 0) {
        block_sz = ctx->mem_ptr[0] | (ctx->mem_ptr[1] << 8);
        ctx->mem_ptr += 2;
        ctx->init_flag = 1;
    }

    // Finish at end marker (block_sz == 0)
    if (block_sz == 0) return 0;
    if (block_sz > 4128) {
        // Flash corruption detected! Return 0 to abort programming safely.
        return 0;
    }

    // Read compressed block from SPIFI memory
    memcpy(ctx->buffer, ctx->mem_ptr, block_sz + 2);
    ctx->mem_ptr += block_sz + 2;

    // Extract next block size
    ctx->next_block_sz = ctx->buffer[block_sz] | (ctx->buffer[block_sz + 1] << 8);

    // Decompress block using LZ4
    return lz4_blk_decompress(ctx->buffer, out_buffer, block_sz);
}

// Program iCE40 FPGA using SPIFI memory-mapped data
// Based on ice40_spi_syscfg_program() from ice40_spi.c
static bool program_fpga_from_spifi(const uint8_t* bitstream_start, uint8_t* mem_base) {
    // Drive CRESET_B = 0, SPI_SS = 0
    fpga_creset_low();
    fpga_cs_low();

    // Wait minimum 200ns
    delay_us(1);

    // Release CRESET_B (drive high)
    fpga_creset_high();

    // Wait minimum 1200us (we wait 1800us to be safe)
    delay_us(1800);

    // Set SPI_SS = 1, send 8 dummy clocks
    fpga_cs_high();
    ssp1_transfer_byte(0);

    // Send configuration image
    // Use  buffers to avoid stack overflow (~8KB would be needed)
    uint8_t* out_buffer = mem_base;
    struct spifi_fpga_read_ctx* ctx = (struct spifi_fpga_read_ctx*)(out_buffer + 4096);
    memset(ctx, 0, sizeof(struct spifi_fpga_read_ctx));
    memset(out_buffer, 0, 4096);
    ctx->mem_ptr = bitstream_start;
    ctx->next_block_sz = 0;
    ctx->init_flag = 0;

    fpga_cs_low();
    // Full LZ4 decompress and send all bytes
    for (;;) {
        size_t read_sz = spifi_fpga_read_block_cb(ctx, out_buffer);
        if (read_sz == 0) break;
        for (size_t j = 0; j < read_sz; j++) {
            ssp1_transfer_byte(out_buffer[j]);
        }
    }

    // Wait for 100 clock cycles for CDONE to go high
    fpga_cs_high();
    for (size_t j = 0; j < 13; j++) {
        ssp1_transfer_byte(0);
    }

    // Check CDONE status
    bool success = fpga_cdone_read();

    // NOTE: FPGA register initialization is done later in radio::init()
    // The FPGA needs time to stabilize after configuration before accepting register writes

    // CRITICAL: Reconfigure SSP1 for MAX2831 (PRALINE RF chip) after FPGA programming
    // iCE40 uses Mode 3 (CPOL=1, CPHA=1), 8-bit
    // MAX2831 (PRALINE) uses Mode 0 (CPOL=0, CPHA=0), 9-bit (vs 16-bit for MAX283x on HackRF One)
    // Without this, RF communication will fail!
    /*SSP1_CR1_LOCAL = 0;  // Disable SSP1
    SSP1_CR0_LOCAL = (0x08) |           // 9-bit data (DSS = 0x08) for MAX2831/PRALINE
                     (0x00) |           // SPI frame format
                     (0 << 6) |         // CPOL = 0 (Mode 0)
                     (0 << 7) |         // CPHA = 0 (Mode 0)
                     (21 << 8);         // SCR = 21 (same as ssp_config_max283x for PRALINE)
    SSP1_CPSR_LOCAL = 2;                // Clock prescaler
    SSP1_CR1_LOCAL = SSP_CR1_SSE;       // Re-enable SSP1*/

    return success;
}

// ============================================================================
// Main Initialization Entry Point
// ============================================================================

int fpga_bridge_init(uint8_t* mem_base) {
    // Enable SSP1 clock for FPGA programming
    // Use PLL1 (204MHz) to match original HackRF - IRC (12MHz) is 17x too slow
    CGU_BASE_SSP1_CLK = CGU_BASE_SSP1_CLK_AUTOBLOCK(1) |
                        CGU_BASE_SSP1_CLK_CLK_SEL(CGU_SRC_PLL1);

    // Configure SSP1 pins
    configure_ssp1_pins();

    // Configure FPGA control pins
    configure_fpga_control_pins();

    // Initialize SSP1 for iCE40 programming
    ssp1_init_ice40();

    // Read FPGA bitstream header from SPIFI memory
    const uint8_t* fpga_header = (const uint8_t*)FPGA_BITSTREAM_MEM_ADDR;
    uint32_t num_bitstreams = fpga_header[0] | (fpga_header[1] << 8) |
                              (fpga_header[2] << 16) | (fpga_header[3] << 24);

    // Check if header looks valid
    if (num_bitstreams == 0 || num_bitstreams > 16 || num_bitstreams == 0xFFFFFFFF) {
        // No valid FPGA bitstream - skip programming but continue boot
        return 1;
    }

    // Get offset of first bitstream (index 0 = standard_fpga)
    uint32_t bitstream_offset = fpga_header[4] | (fpga_header[5] << 8) |
                                (fpga_header[6] << 16) | (fpga_header[7] << 24);

    // Calculate start address of first bitstream in SPIFI memory
    const uint8_t* bitstream_start = (const uint8_t*)(FPGA_BITSTREAM_MEM_ADDR + bitstream_offset);

    // Full FPGA programming
    bool success = program_fpga_from_spifi(bitstream_start, mem_base);

    // Initialize FPGA registers immediately after programming
    if (success) {
        // Give FPGA 100us to stabilize after configuration
        delay_us(100);

        // Initialize FPGA registers (DC_BLOCK, etc.)
        fpga_register_init();

        // Now switch to MAX2831 mode
        ssp1_set_mode_max2831();
    }
    return success ? 0 : 2;
}
#else
#warning "Building for HackRF_One with CPLD."
#endif
