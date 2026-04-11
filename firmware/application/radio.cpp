/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "radio.hpp"

#include "rf_path.hpp"

#include "rffc507x.hpp"

#ifdef PRALINE
#include "max2831.hpp"
extern "C" {
#include "fpga_bridge.h"
}
#else
#include "max2837.hpp"
#include "max2839.hpp"
#include "baseband_cpld.hpp"
#endif

#include "max5864.hpp"

#include "tuning.hpp"

#include "spi_arbiter.hpp"

#include "hackrf_hal.hpp"
#include "hackrf_gpio.hpp"
using namespace hackrf::one;

#include "cpld_update.hpp"

#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"

#include "hal.h"  // For LPC_SGPIO

#include <array>

/* Direct access to the radio. Setting values incorrectly can damage
 * the device. Applications should use ReceiverModel or TransmitterModel
 * instead of calling these functions directly. */
namespace radio {

static constexpr uint32_t ssp1_cpsr = 2;

static constexpr uint32_t ssp_scr(
    const float pclk_f,
    const uint32_t cpsr,
    const float spi_f) {
    return static_cast<uint8_t>(pclk_f / cpsr / spi_f - 1);
}

#ifdef PRALINE
/* MAX2831 uses 9-bit SPI transfers */
static constexpr SPIConfig ssp_config_max283x = {
    .end_cb = NULL,
    .ssport = gpio_max283x_select.port(),
    .sspad = gpio_max283x_select.pad(),
    .cr0 =
        CR0_CLOCKRATE(ssp_scr(ssp1_pclk_f, ssp1_cpsr, max283x_spi_f) + 3) | CR0_FRFSPI | CR0_DSS9BIT,
    .cpsr = ssp1_cpsr,
};

static max283x::MAX283x* transceiver = nullptr;

void set_rx_buff_vcm(const size_t v) {
    if (transceiver) {
        transceiver->set_rx_buff_vcm(v);
    }
}

#else
/* MAX2837/MAX2839 use 16-bit SPI transfers */
static constexpr SPIConfig ssp_config_max283x = {
    .end_cb = NULL,
    .ssport = gpio_max283x_select.port(),
    .sspad = gpio_max283x_select.pad(),
    .cr0 =
        CR0_CLOCKRATE(ssp_scr(ssp1_pclk_f, ssp1_cpsr, max283x_spi_f) + 3) | CR0_FRFSPI | CR0_DSS16BIT,
    .cpsr = ssp1_cpsr,
};
#endif

static constexpr SPIConfig ssp_config_max5864 = {
    .end_cb = NULL,
    .ssport = gpio_max5864_select.port(),
    .sspad = gpio_max5864_select.pad(),
    .cr0 =
        CR0_CLOCKRATE(ssp_scr(ssp1_pclk_f, ssp1_cpsr, max5864_spi_f)) | CR0_FRFSPI | CR0_DSS8BIT,
    .cpsr = ssp1_cpsr,
};

static spi::arbiter::Arbiter ssp1_arbiter(portapack::ssp1);

static spi::arbiter::Target ssp1_target_max283x{
    ssp1_arbiter,
    ssp_config_max283x};

static spi::arbiter::Target ssp1_target_max5864{
    ssp1_arbiter,
    ssp_config_max5864};

static rf::path::Path rf_path;
rffc507x::RFFC507x first_if;
max283x::MAX283x* second_if;
#ifdef PRALINE
max2831::MAX2831 second_if_max2831{ssp1_target_max283x};
#else
max2837::MAX2837 second_if_max2837{ssp1_target_max283x};
max2839::MAX2839 second_if_max2839{ssp1_target_max283x};
static baseband::CPLD baseband_cpld;
#endif
static max5864::MAX5864 baseband_codec{ssp1_target_max5864};

// load_sram() is called at boot in portapack.cpp, including verify CPLD part, so default direction is Receive
static rf::Direction direction{rf::Direction::Receive};
static bool baseband_invert = false;
static bool mixer_invert = false;

#ifdef PRALINE
static rf::Direction cached_direction = rf::Direction::Receive;
static bool cached_rf_amp = false;
static int_fast8_t cached_lna_gain = 0;
static int_fast8_t cached_vga_gain = 0;
#endif

void init() {
#ifdef PRALINE
    /* PRALINE uses MAX2831 transceiver */
    second_if = (max283x::MAX283x*)&second_if_max2831;
#else
    if (hackrf_r9) {
        gpio_r9_not_ant_pwr.write(1);
        gpio_r9_not_ant_pwr.output();
    }
    second_if = hackrf_r9
                    ? (max283x::MAX283x*)&second_if_max2839
                    : (max283x::MAX283x*)&second_if_max2837;
#endif

    rf_path.init();
    first_if.init();
    second_if->init();
    baseband_codec.init();

#ifdef PRALINE

    /* Praline-Specific Bus and Gateware Configuration */

    // SYNC SGPIO TO FPGA CLOCK:
    // Configure all 16 SGPIO slices to use the external clock (SGPIO8)
    // provided by the FPGA. This allows the MCU to stay at 40MHz
    // while the data bus scales to the RF sample rate.
    // Bit 2:1 of SGPIO_MUX_CFG = 01 (External clock from SGPIO8)
    // SYNC SGPIO TO FPGA CLOCK WITH FALLING EDGE LATCH
    for (int i = 0; i < 16; i++) {
        // (1 << 1) = External clock from SGPIO8
        // (1 << 3) = Sample on the FALLING edge of the clock
        LPC_SGPIO->SGPIO_MUX_CFG[i] = (1 << 1) | (1 << 3);
    }

    /* Initialize FPGA registers - DC_BLOCK must be enabled for RX */
    // debug::fpga::init();

    fpga_set_mode(FPGA_MODE_RX);

    // These FPGA registers control DC_BLOCK, Q-Inv, QUARTER SHIFT, and Decimation.
    fpga_debug_register_write(FPGA_REG_CTRL, FPGA_CTRL_DC_BLOCK_EN);  // DC_BLOCK=1, QUARTER_SHIFT=0, Q_INVERT=0
    fpga_debug_register_write(FPGA_REG_DECIM, 0x00);                  // RX_DECIM=No Decim

    // RX Mode: Register 3 is RX Digital Gain. Start with 0dB (no shift).
    fpga_debug_register_write(FPGA_REG_RX_DIGITAL_GAIN, FPGA_RX_DEFAULT_DIGITAL_GAIN);

    /* RX Mode: Initialize DC Block parameters to standard Praline values.
     * 0x04 Width and 0x08 Adapt Rate are typical for 40MHz stability.
     */
    fpga_debug_register_write(FPGA_REG_RX_DC_BLOCK_WIDTH, FPGA_RX_DEFAULT_DC_WIDTH);
    fpga_debug_register_write(FPGA_REG_RX_DC_ADAPT_RATE, FPGA_RX_DEFAULT_ADAPT_RATE);

    ssp1_arbiter.invalidate();
    chThdSleepMilliseconds(10);  // Let FPGA registers settle
#else
    /* HackRF One uses CPLD for Q inversion control.
     * PRALINE uses FPGA and the pin (P2_3) is used for LCD_TE on H4M. */
    baseband_cpld.init();
#endif
}

void set_direction(const rf::Direction new_direction) {
    /* TODO: Refactor all the various "Direction" enumerations into one. */
    /* TODO: Only make changes if direction changes, but beware of clock enabling. */

    // That below code line , was used to prevent RX interf ghosting when switching back to RX from any TX mode, but in recent code. it seems not necessary.
    // Deleting that load_sram_no_verify() (or the original , load_sram() ), solves random TX swap I/Q  problem in H1R1 , others OK- (and no side effects to all).
    // hackrf::cpld::load_sram_no_verify();  // After commit "removed the use of the hackrf cpld eeprom #1732", in a H1R1,  Mic App wrong SSB TX with random USB/LSB change.

#ifdef PRALINE
    cached_direction = new_direction;  // Track state for debug and potentially other purposes.

    if (new_direction == rf::Direction::Transmit) {
        fpga_set_mode(FPGA_MODE_TX);
        // TX Mode: Clear RX gain and ensure NCO is off initially
        fpga_debug_register_write(FPGA_REG_TX_CONTROL, 0x00);
        // Placeholder: Set TX-specific interpolation and phase
        fpga_debug_register_write(FPGA_REG_TX_INTERP, 0x00);
        fpga_debug_register_write(FPGA_REG_TX_PHASE_STEP, 0x00);
    } else {
        fpga_set_mode(FPGA_MODE_RX);
        // RX Mode: Ensure NCO is disabled and reset digital gain
        fpga_debug_register_write(FPGA_REG_RX_DIGITAL_GAIN, FPGA_RX_DEFAULT_DIGITAL_GAIN);
        /* RX Mode: Initialize DC Block parameters to standard Praline values.
         * 0x04 Width and 0x08 Adapt Rate are typical for 40MHz stability.
         */
        fpga_debug_register_write(FPGA_REG_RX_DC_BLOCK_WIDTH, FPGA_RX_DEFAULT_DC_WIDTH);
        fpga_debug_register_write(FPGA_REG_RX_DC_ADAPT_RATE, FPGA_RX_DEFAULT_ADAPT_RATE);
    }
#endif

    direction = new_direction;

    if (hackrf_r9) {
        /*
         * HackRF One r9 inverts analog baseband only for RX. Previous hardware
         * revisions inverted analog baseband for neither direction because of
         * compensation in the CPLD. If we ever simplify the CPLD to handle RX
         * and TX the same way, we will need to update this baseband_invert
         * logic.
         */
        baseband_invert = (direction == rf::Direction::Receive);
    } else {
        /*
         * Analog baseband is inverted in RX but not TX. The RX inversion is
         * corrected by the CPLD, but future hardware or CPLD changes may
         * change this for either or both directions. For a given hardware+CPLD
         * platform, baseband inversion is set here for RX and/or TX. Spectrum
         * inversion resulting from the mixer is tracked separately according
         * to the tuning configuration. We ask the CPLD to apply a correction
         * for the total inversion.
         */
        baseband_invert = false;
    }

#ifdef PRALINE

    // Q inversion controlled by GPIO0[13] (SGPIO12), not FPGA register
    bool q_invert = mixer_invert ^ baseband_invert;
    if (q_invert) {
        LPC_GPIO->SET[0] = (1 << 13);  // SGPIO12 = 1 (Q inverted)
    } else {
        LPC_GPIO->CLR[0] = (1 << 13);  // SGPIO12 = 0 (Q normal)
    }

    ssp1_arbiter.invalidate();
#else
    baseband_cpld.set_invert(mixer_invert ^ baseband_invert);
#endif

    second_if->set_mode((direction == rf::Direction::Transmit) ? max283x::Mode::Transmit : max283x::Mode::Receive);
    rf_path.set_direction(direction);

    baseband_codec.set_mode((direction == rf::Direction::Transmit) ? max5864::Mode::Transmit : max5864::Mode::Receive);

    if (direction == rf::Direction::Receive)
        led_rx.on();
    else
        led_tx.on();
}

bool set_tuning_frequency(const rf::Frequency frequency) {
    rf::Frequency final_frequency = frequency;
    // if converter feature is enabled
    if (portapack::persistent_memory::config_converter()) {
        // downconvert
        if (portapack::persistent_memory::config_updown_converter()) {
            final_frequency = frequency - portapack::persistent_memory::config_converter_freq();
        } else  // upconvert
        {
            final_frequency = frequency + portapack::persistent_memory::config_converter_freq();
        }
    }
    // apply frequency correction
    if (direction == rf::Direction::Transmit) {
        if (portapack::persistent_memory::config_freq_tx_correction_updown())  // tx freq correction down
            final_frequency = final_frequency - portapack::persistent_memory::config_freq_tx_correction();
        else  // tx freq correction up
            final_frequency = final_frequency + portapack::persistent_memory::config_freq_tx_correction();
    } else {
        if (portapack::persistent_memory::config_freq_rx_correction_updown())  // rx freq correction down
            final_frequency = final_frequency - portapack::persistent_memory::config_freq_rx_correction();
        else  // rx freq correction up
            final_frequency = final_frequency + portapack::persistent_memory::config_freq_rx_correction();
    }

    const auto tuning_config = tuning::config::create(final_frequency);
    if (tuning_config.is_valid()) {
        first_if.disable();

        // Program first local oscillator frequency (if there is one) into RFFC507x
        if (tuning_config.first_lo_frequency) {
            first_if.set_frequency(tuning_config.first_lo_frequency);
            first_if.enable();
#ifdef PRALINE
            first_if.flush();            // Force register write with reference clock present
            chThdSleepMilliseconds(10);  // Allow PLL to settle
#endif
        }

        // Program second local oscillator frequency into MAX283x
        const auto result_second_if = second_if->set_frequency(tuning_config.second_lo_frequency);

        rf_path.set_band(tuning_config.rf_path_band);
        mixer_invert = tuning_config.mixer_invert;

#ifdef PRALINE

        // Q inversion controlled by GPIO0[13] (SGPIO12), not FPGA register
        bool q_invert = mixer_invert ^ baseband_invert;
        if (q_invert) {
            LPC_GPIO->SET[0] = (1 << 13);  // SGPIO12 = 1 (Q inverted)
        } else {
            LPC_GPIO->CLR[0] = (1 << 13);  // SGPIO12 = 0 (Q normal)
        }

        ssp1_arbiter.invalidate();
#else
        baseband_cpld.set_invert(mixer_invert ^ baseband_invert);
#endif

        return result_second_if;
    } else {
        return false;
    }
}

void set_rf_amp(const bool rf_amp) {
#ifdef PRALINE
    cached_rf_amp = rf_amp;  // Track state for debug and potentialy other purposes.
#endif
    rf_path.set_rf_amp(rf_amp);
}

void set_lna_gain(const int_fast8_t db) {
#ifdef PRALINE
    cached_lna_gain = db;  // Track state for debug and potentially other purposes.
#endif
    second_if->set_lna_gain(db);
}

void set_vga_gain(const int_fast8_t db) {
#ifdef PRALINE
    cached_vga_gain = db;  // Track state for debug and potentially other purposes.
#endif
    second_if->set_vga_gain(db);
}

void set_tx_gain(const int_fast8_t db) {
    second_if->set_tx_vga_gain(db);
}

void set_baseband_filter_bandwidth_rx(const uint32_t bandwidth_minimum) {
    second_if->set_lpf_rf_bandwidth_rx(bandwidth_minimum);
}

void set_baseband_filter_bandwidth_tx(const uint32_t bandwidth_minimum) {
    second_if->set_lpf_rf_bandwidth_tx(bandwidth_minimum);
}

void set_baseband_rate(const uint32_t rate) {
    portapack::clock_manager.set_sampling_frequency(rate);
    // TODO: actually set baseband too?
}

void set_antenna_bias(const bool on) {
    /* Pull MOSFET gate low to turn on antenna bias. */
#ifdef PRALINE
    // Praline: P2_12 = GPIO1[12], ANT_BIAS_EN_N (active LOW)
    LPC_GPIO->CLR[1] = on ? (1 << 12) : 0;
    LPC_GPIO->SET[1] = on ? 0 : (1 << 12);
#else
    if (hackrf_r9) {
        gpio_r9_not_ant_pwr.write(on ? 0 : 1);
    } else {
        first_if.set_gpo1(on ? 0 : 1);
    }
#endif
}

void set_tx_max283x_iq_phase_calibration(const size_t v) {
    second_if->set_tx_LO_iq_phase_calibration(v);
}

void set_rx_max283x_iq_phase_calibration(const size_t v) {
    second_if->set_rx_LO_iq_phase_calibration(v);
}

void disable() {
    set_antenna_bias(false);
    baseband_codec.set_mode(max5864::Mode::Shutdown);
#ifdef PRALINE
    second_if->set_mode(max283x::Mode::Standby);
#else
    second_if->set_mode(max2837::Mode::Standby);
#endif
    first_if.disable();
    set_rf_amp(false);

    led_rx.off();
    led_tx.off();
}

#ifdef PRALINE
void invalidate_spi_config() {
    ssp1_arbiter.invalidate();
}
#endif

namespace debug {

#ifdef PRALINE
rf::Direction get_cached_direction() {
    return cached_direction;
}

bool get_cached_rf_amp() {
    return cached_rf_amp;
}

int_fast8_t get_cached_lna_gain() {
    return cached_lna_gain;
}

int_fast8_t get_cached_vga_gain() {
    return cached_vga_gain;
}
#endif

namespace first_if {

uint32_t register_read(const size_t register_number) {
    return radio::first_if.read(register_number);
}

void register_write(const size_t register_number, uint32_t value) {
    radio::first_if.write(register_number, value);
}

#ifdef PRALINE
extern "C" {
extern struct rffc507x_debug_t {
    uint32_t requested_freq_mhz;
    uint32_t calculated_vco_mhz;
    uint32_t expected_n;
    uint8_t expected_lodiv;
    uint8_t expected_presc;
    bool was_called;
    uint32_t calc_lo_freq_mhz;
    uint32_t calc_vco_inside_mhz;
    uint8_t calc_lodiv_log2;
    uint8_t calc_presc_log2;
    uint64_t calc_n_q24;
} rffc507x_debug_info;
}

/*struct TuningInfo {
    uint32_t requested_freq_mhz;
    uint32_t expected_n;
    uint8_t expected_lodiv;
    uint8_t expected_presc;
    bool was_called;
};*/

TuningInfo get_tuning_info() {
    return {
        rffc507x_debug_info.requested_freq_mhz,
        rffc507x_debug_info.calculated_vco_mhz,
        rffc507x_debug_info.expected_n,
        rffc507x_debug_info.expected_lodiv,
        rffc507x_debug_info.expected_presc,
        rffc507x_debug_info.was_called,
        rffc507x_debug_info.calc_lo_freq_mhz,
        rffc507x_debug_info.calc_vco_inside_mhz,
        rffc507x_debug_info.calc_lodiv_log2,
        rffc507x_debug_info.calc_presc_log2,
        rffc507x_debug_info.calc_n_q24,
    };
}
#endif

} /* namespace first_if */

namespace second_if {

uint32_t register_read(const size_t register_number) {
    return radio::second_if->read(register_number);
}

void register_write(const size_t register_number, uint32_t value) {
    radio::second_if->write(register_number, value);
}

int8_t temp_sense() {
    return radio::second_if->temp_sense();
}

} /* namespace second_if */

namespace rf_path_info {
rf::path::Band get_current_band() {
    return radio::rf_path.get_band();
}
} /* namespace rf_path_info */

#ifdef PRALINE
namespace fpga {

/* Use fpga_bridge.c functions for FPGA register access.
 * These properly switch SPI mode between iCE40 (Mode 3, 8-bit)
 * and MAX2831 (Mode 0, 9-bit). After each access, we must
 * invalidate the SPI arbiter's cached config since fpga_bridge.c
 * modifies SSP1 registers directly. */

uint32_t register_read(const size_t register_number) {
    uint32_t result = fpga_debug_register_read(static_cast<uint8_t>(register_number));
    ssp1_arbiter.invalidate();  // Force arbiter to reconfigure on next transfer
    return result;
}

void register_write(const size_t register_number, uint32_t value) {
    fpga_debug_register_write(static_cast<uint8_t>(register_number), static_cast<uint8_t>(value));
    ssp1_arbiter.invalidate();  // Force arbiter to reconfigure on next transfer
}

void init() {
    fpga_set_mode(FPGA_MODE_RX);

    // These FPGA registers control DC_BLOCK, Q-Inv, QUARTER SHIFT, and Decimation.
    fpga_debug_register_write(FPGA_REG_CTRL, FPGA_CTRL_DC_BLOCK_EN);  // DC_BLOCK=1, QUARTER_SHIFT=0, Q_INVERT=0
    fpga_debug_register_write(FPGA_REG_DECIM, 0x00);                  // RX_DECIM=No Decim

    // RX Mode: Register 3 is RX Digital Gain. Start with 0dB (no shift).
    fpga_debug_register_write(FPGA_REG_RX_DIGITAL_GAIN, FPGA_RX_DEFAULT_DIGITAL_GAIN);

    /* RX Mode: Initialize DC Block parameters to standard Praline values.
     * 0x04 Width and 0x08 Adapt Rate are typical for 40MHz stability.
     */
    fpga_debug_register_write(FPGA_REG_RX_DC_BLOCK_WIDTH, FPGA_RX_DEFAULT_DC_WIDTH);
    fpga_debug_register_write(FPGA_REG_RX_DC_ADAPT_RATE, FPGA_RX_DEFAULT_ADAPT_RATE);

    ssp1_arbiter.invalidate();  // Force arbiter to reconfigure on next transfer
}

} /* namespace fpga */
#endif

namespace sgpio {

/* SGPIO register map for debug viewing
 * We expose key registers for diagnosing data flow issues.
 * Register numbers map to:
 * 0: CTRL_ENABLE   - Which slices are enabled
 * 1: GPIO_INREG    - GPIO input register (data pins state)
 * 2: GPIO_OUTREG   - GPIO output register (direction, disable, etc)
 * 3: GPIO_OENREG   - GPIO output enable register
 * 4: STATUS_1      - Exchange interrupt status (slice A = bit 0)
 * 5: REG_SS[0]     - Shadow register slice A (current sample data)
 */
uint32_t register_read(const size_t register_number) {
    switch (register_number) {
        case 0:
            return LPC_SGPIO->CTRL_ENABLE;
        case 1:
            return LPC_SGPIO->GPIO_INREG;
        case 2:
            return LPC_SGPIO->GPIO_OUTREG;
        case 3:
            return LPC_SGPIO->GPIO_OENREG;
        case 4:
            return LPC_SGPIO->STATUS_1;
        case 5:
            return LPC_SGPIO->REG_SS[0];
        default:
            return 0xFFFFFFFF;
    }
}

} /* namespace sgpio */

} /* namespace debug */

} /* namespace radio */