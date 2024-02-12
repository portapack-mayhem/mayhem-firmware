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
#include "max2837.hpp"
#include "max2839.hpp"
#include "max5864.hpp"
#include "baseband_cpld.hpp"

#include "tuning.hpp"

#include "spi_arbiter.hpp"

#include "hackrf_hal.hpp"
#include "hackrf_gpio.hpp"
using namespace hackrf::one;

#include "cpld_update.hpp"

#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"

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

static constexpr SPIConfig ssp_config_max283x = {
    .end_cb = NULL,
    .ssport = gpio_max283x_select.port(),
    .sspad = gpio_max283x_select.pad(),
    .cr0 =
        CR0_CLOCKRATE(ssp_scr(ssp1_pclk_f, ssp1_cpsr, max283x_spi_f) + 3) | CR0_FRFSPI | CR0_DSS16BIT,
    .cpsr = ssp1_cpsr,
};

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
max2837::MAX2837 second_if_max2837{ssp1_target_max283x};
max2839::MAX2839 second_if_max2839{ssp1_target_max283x};
static max5864::MAX5864 baseband_codec{ssp1_target_max5864};
static baseband::CPLD baseband_cpld;

// load_sram() is called at boot in portapack.cpp, including verify CPLD part, so default direction is Receive
static rf::Direction direction{rf::Direction::Receive};
static bool baseband_invert = false;
static bool mixer_invert = false;

void init() {
    if (hackrf_r9) {
        gpio_r9_not_ant_pwr.write(1);
        gpio_r9_not_ant_pwr.output();
    }
    rf_path.init();
    first_if.init();
    second_if = hackrf_r9
                    ? (max283x::MAX283x*)&second_if_max2839
                    : (max283x::MAX283x*)&second_if_max2837;
    second_if->init();
    baseband_codec.init();
    baseband_cpld.init();
}

void set_direction(const rf::Direction new_direction) {
    /* TODO: Refactor all the various "Direction" enumerations into one. */
    /* TODO: Only make changes if direction changes, but beware of clock enabling. */

    // That below code line , was used to prevent RX interf ghosting when switching back to RX from any TX mode, but in recent code. it seems not necessary.
    // Deleting that load_sram_no_verify() (or the original , load_sram() ), solves random TX swap I/Q  problem in H1R1 , others OK- (and no side effects to all).
    // hackrf::cpld::load_sram_no_verify();  // After commit "removed the use of the hackrf cpld eeprom #1732", in a H1R1,  Mic App wrong SSB TX with random USB/LSB change.

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
    baseband_cpld.set_invert(mixer_invert ^ baseband_invert);

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
        }

        // Program second local oscillator frequency into MAX283x
        const auto result_second_if = second_if->set_frequency(tuning_config.second_lo_frequency);

        rf_path.set_band(tuning_config.rf_path_band);
        mixer_invert = tuning_config.mixer_invert;
        baseband_cpld.set_invert(mixer_invert ^ baseband_invert);

        return result_second_if;
    } else {
        return false;
    }
}

void set_rf_amp(const bool rf_amp) {
    rf_path.set_rf_amp(rf_amp);

    if (direction == rf::Direction::Transmit) {
        if (rf_amp)
            led_tx.on();
        else
            led_tx.off();
    }
}

void set_lna_gain(const int_fast8_t db) {
    second_if->set_lna_gain(db);
}

void set_vga_gain(const int_fast8_t db) {
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
    if (hackrf_r9) {
        gpio_r9_not_ant_pwr.write(on ? 0 : 1);
    } else {
        first_if.set_gpo1(on ? 0 : 1);
    }
}

void set_tx_max283x_iq_phase_calibration(const size_t v) {
    second_if->set_tx_LO_iq_phase_calibration(v);
}

/*void enable(Configuration configuration) {
    configure(configuration);
}

void configure(Configuration configuration) {
    set_tuning_frequency(configuration.tuning_frequency);
    set_rf_amp(configuration.rf_amp);
    set_lna_gain(configuration.lna_gain);
    set_vga_gain(configuration.vga_gain);
    set_baseband_rate(configuration.baseband_rate);
    set_baseband_filter_bandwidth(configuration.baseband_filter_bandwidth);
    set_direction(configuration.direction);
}*/

void disable() {
    set_antenna_bias(false);
    baseband_codec.set_mode(max5864::Mode::Shutdown);
    second_if->set_mode(max2837::Mode::Standby);
    first_if.disable();
    set_rf_amp(false);

    led_rx.off();
    led_tx.off();
}

namespace debug {

namespace first_if {

uint32_t register_read(const size_t register_number) {
    return radio::first_if.read(register_number);
}

void register_write(const size_t register_number, uint32_t value) {
    radio::first_if.write(register_number, value);
}

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

} /* namespace debug */

} /* namespace radio */
