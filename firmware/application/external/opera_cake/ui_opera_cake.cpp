/*
 * copyleft spammingdramaqueen
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

/*
 * Opera Cake is an antenna-switching add-on board for HackRF One.
 * It uses a PCA9557 I/O expander (8-bit output port) at I2C address 0x18.
 *
 * PCA9557 register map:
 *   0x00 - Input port   (read-only)
 *   0x01 - Output port  (controls the RF switches)
 *   0x02 - Polarity inversion
 *   0x03 - Configuration (0 = output, 1 = input; default all inputs)
 *
 * Output port bit assignments:
 *   Bit 7: /OE  — Output Enable, active-low (0 = switches active)
 *   Bit 6: U2CTRL1 — Port-A mux select bit 1
 *   Bit 5: U2CTRL0 — Port-A mux select bit 0
 *   Bit 4: U3CTRL1 — Port-B mux select bit 1
 *   Bit 3: U3CTRL0 — Port-B mux select bit 0
 *   Bit 2: U1CTRL  — Through-switch (connects A-side to B-side for 1×8 mode)
 *   Bit 1: LEDEN2  — LED enable 2
 *   Bit 0: LEDEN   — LED enable 1
 *
 * Port A0 → A1-A4 (U2CTRL1:U2CTRL0 = 00, 01, 10, 11)
 * Port B0 → B1-B4 (U3CTRL1:U3CTRL0 = 00, 01, 10, 11)
 *
 * Note: time-based switching is NOT supported on PortaPack because the
 * required GPIO pins conflict with PortaPack hardware.
 */

#include "ui_opera_cake.hpp"
#include "portapack.hpp"
#include "string_format.hpp"

using namespace portapack;

namespace ui::external_app::opera_cake {

// PCA9557 register addresses
static constexpr uint8_t REG_OUTPUT = 0x01;
static constexpr uint8_t REG_CONFIG = 0x03;

// Port A0→A1-A4: U2CTRL1 (bit6) and U2CTRL0 (bit5)
static constexpr uint8_t PORT_A_BITS[4] = {
    0x00,  // A1: U2CTRL1=0 U2CTRL0=0
    0x20,  // A2: U2CTRL1=0 U2CTRL0=1
    0x40,  // A3: U2CTRL1=1 U2CTRL0=0
    0x60,  // A4: U2CTRL1=1 U2CTRL0=1
};

// Port B0→B1-B4: U3CTRL1 (bit4) and U3CTRL0 (bit3)
static constexpr uint8_t PORT_B_BITS[4] = {
    0x00,  // B1: U3CTRL1=0 U3CTRL0=0
    0x08,  // B2: U3CTRL1=0 U3CTRL0=1
    0x10,  // B3: U3CTRL1=1 U3CTRL0=0
    0x18,  // B4: U3CTRL1=1 U3CTRL0=1
};

// Base flags: /OE=0 (enabled), U1CTRL=0, LEDEN2=1, LEDEN=1
static constexpr uint8_t OUTPUT_BASE = 0x03;

OperaCakeView::OperaCakeView(NavigationView& nav)
    : nav_{nav} {
    add_children({
        &labels,
        &text_status,
        &options_port_a,
        &options_port_b,
        &button_apply,
        &button_rescan,
        &text_result,
    });

    // Clamp loaded values to valid range before applying to selectors
    if (setting_port_a > 3) setting_port_a = 0;
    if (setting_port_b > 3) setting_port_b = 0;

    options_port_a.set_selected_index(setting_port_a, false);
    options_port_b.set_selected_index(setting_port_b, false);

    // Probe for Opera Cake hardware at startup
    detect_board();

    options_port_a.on_change = [this](size_t idx, int32_t) {
        setting_port_a = static_cast<uint8_t>(idx);
    };

    options_port_b.on_change = [this](size_t idx, int32_t) {
        setting_port_b = static_cast<uint8_t>(idx);
    };

    button_apply.on_select = [this](Button&) {
        apply_settings();
    };

    button_rescan.on_select = [this](Button&) {
        text_result.set("");
        detect_board();
    };
}

void OperaCakeView::detect_board() {
    if (i2c0.probe(OPERACAKE_I2C_ADDRESS)) {
        text_status.set("Found at 0x18");
    } else {
        text_status.set("Not detected");
    }
}

void OperaCakeView::apply_settings() {
    // Step 1: Configure all PCA9557 pins as outputs (config register = 0x00)
    const uint8_t cfg_data[] = {REG_CONFIG, 0x00};
    if (!i2c0.transmit(OPERACAKE_I2C_ADDRESS, cfg_data, 2)) {
        text_result.set("Err: board not found");
        return;
    }

    // Step 2: Write output byte (port A bits | port B bits | base flags)
    const uint8_t pa = PORT_A_BITS[setting_port_a & 3];
    const uint8_t pb = PORT_B_BITS[setting_port_b & 3];
    const uint8_t output = pa | pb | OUTPUT_BASE;
    const uint8_t out_data[] = {REG_OUTPUT, output};
    if (!i2c0.transmit(OPERACAKE_I2C_ADDRESS, out_data, 2)) {
        text_result.set("Err: write failed");
        return;
    }

    text_result.set("Applied OK");
}

void OperaCakeView::focus() {
    options_port_a.focus();
}

}  // namespace ui::external_app::opera_cake
