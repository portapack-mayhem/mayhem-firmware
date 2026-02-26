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
 *   Bit 7: /OE     — Output Enable, active-low (0 = switches active)
 *   Bit 6: U2CTRL1 — Port-A mux select bit 1
 *   Bit 5: U2CTRL0 — Port-A mux select bit 0
 *   Bit 4: U3CTRL1 — Port-B mux select bit 1
 *   Bit 3: U3CTRL0 — Port-B mux select bit 0
 *   Bit 2: U1CTRL  — Through-switch (A-side to B-side, for 1×8 mode)
 *   Bit 1: LEDEN2  — LED enable 2
 *   Bit 0: LEDEN   — LED enable 1
 *
 * Port A0 → A1-A4  controlled by U2CTRL1:U2CTRL0 = 00, 01, 10, 11
 * Port B0 → B1-B4  controlled by U3CTRL1:U3CTRL0 = 00, 01, 10, 11
 *
 * Supported modes:
 *   Manual    — user directly chooses Port A0 and Port B0 connections.
 *   Frequency — four frequency bands are mapped to ports A1-A4; A4 is the
 *               fallback for unmatched frequencies.  Whenever A0 switches to
 *               Ax, B0 mirrors it to Bx (original Opera Cake behaviour).
 *               With "Monitor: On" the app re-evaluates the current receiver
 *               frequency once per second and switches automatically.
 *
 * Time-based switching is NOT supported: the required timer GPIO pins conflict
 * with PortaPack hardware.
 */

#include "ui_opera_cake.hpp"
#include "portapack.hpp"
#include "string_format.hpp"

using namespace portapack;

namespace ui::external_app::opera_cake {

// ---- PCA9557 register addresses ----------------------------------------
static constexpr uint8_t REG_OUTPUT = 0x01;
static constexpr uint8_t REG_CONFIG = 0x03;

// ---- Port bit patterns -------------------------------------------------
// Port A0→A1-A4: U2CTRL1 (bit 6) and U2CTRL0 (bit 5)
static constexpr uint8_t PORT_A_BITS[4] = {
    0x00,  // A1: U2CTRL1=0 U2CTRL0=0
    0x20,  // A2: U2CTRL1=0 U2CTRL0=1
    0x40,  // A3: U2CTRL1=1 U2CTRL0=0
    0x60,  // A4: U2CTRL1=1 U2CTRL0=1
};

// Port B0→B1-B4: U3CTRL1 (bit 4) and U3CTRL0 (bit 3)
static constexpr uint8_t PORT_B_BITS[4] = {
    0x00,  // B1: U3CTRL1=0 U3CTRL0=0
    0x08,  // B2: U3CTRL1=0 U3CTRL0=1
    0x10,  // B3: U3CTRL1=1 U3CTRL0=0
    0x18,  // B4: U3CTRL1=1 U3CTRL0=1
};

// Base output byte: /OE=0 (enabled), U1CTRL=0, LEDEN2=1, LEDEN=1
static constexpr uint8_t OUTPUT_BASE = 0x03;

// ---- Constructor -------------------------------------------------------

OperaCakeView::OperaCakeView(NavigationView& nav)
    : nav_{nav} {
    add_children({
        &labels,
        &text_status,
        &options_mode,
        &field_min_a1, &field_max_a1,
        &field_min_a2, &field_max_a2,
        &field_min_a3, &field_max_a3,
        &field_min_a4, &field_max_a4,
        &options_port_a,
        &options_port_b,
        &options_monitor,
        &button_apply,
        &button_rescan,
        &text_result,
    });

    // Clamp loaded settings to valid ranges
    if (setting_mode > 1) setting_mode = 0;
    if (setting_port_a > 3) setting_port_a = 0;
    if (setting_port_b > 3) setting_port_b = 0;
    if (setting_monitor > 1) setting_monitor = 0;

    // Apply loaded settings to widgets
    options_mode.set_selected_index(setting_mode, false);
    options_port_a.set_selected_index(setting_port_a, false);
    options_port_b.set_selected_index(setting_port_b, false);
    options_monitor.set_selected_index(setting_monitor, false);

    field_min_a1.set_value(static_cast<int32_t>(setting_min_a1), false);
    field_max_a1.set_value(static_cast<int32_t>(setting_max_a1), false);
    field_min_a2.set_value(static_cast<int32_t>(setting_min_a2), false);
    field_max_a2.set_value(static_cast<int32_t>(setting_max_a2), false);
    field_min_a3.set_value(static_cast<int32_t>(setting_min_a3), false);
    field_max_a3.set_value(static_cast<int32_t>(setting_max_a3), false);
    field_min_a4.set_value(static_cast<int32_t>(setting_min_a4), false);
    field_max_a4.set_value(static_cast<int32_t>(setting_max_a4), false);

    detect_board();

    // Wire up callbacks to keep settings in sync with widget state
    options_mode.on_change = [this](size_t idx, int32_t) {
        setting_mode = static_cast<uint8_t>(idx);
    };
    options_port_a.on_change = [this](size_t idx, int32_t) {
        setting_port_a = static_cast<uint8_t>(idx);
    };
    options_port_b.on_change = [this](size_t idx, int32_t) {
        setting_port_b = static_cast<uint8_t>(idx);
    };
    options_monitor.on_change = [this](size_t idx, int32_t) {
        setting_monitor = static_cast<uint8_t>(idx);
    };

    field_min_a1.on_change = [this](int32_t v) { setting_min_a1 = static_cast<uint32_t>(v); };
    field_max_a1.on_change = [this](int32_t v) { setting_max_a1 = static_cast<uint32_t>(v); };
    field_min_a2.on_change = [this](int32_t v) { setting_min_a2 = static_cast<uint32_t>(v); };
    field_max_a2.on_change = [this](int32_t v) { setting_max_a2 = static_cast<uint32_t>(v); };
    field_min_a3.on_change = [this](int32_t v) { setting_min_a3 = static_cast<uint32_t>(v); };
    field_max_a3.on_change = [this](int32_t v) { setting_max_a3 = static_cast<uint32_t>(v); };
    field_min_a4.on_change = [this](int32_t v) { setting_min_a4 = static_cast<uint32_t>(v); };
    field_max_a4.on_change = [this](int32_t v) { setting_max_a4 = static_cast<uint32_t>(v); };

    button_apply.on_select = [this](Button&) {
        if (setting_mode == 0)
            apply_manual();
        else
            apply_frequency();
    };

    button_rescan.on_select = [this](Button&) {
        text_result.set("");
        detect_board();
    };
}

// ---- Board detection ---------------------------------------------------

void OperaCakeView::detect_board() {
    if (i2c0.probe(OPERACAKE_I2C_ADDRESS)) {
        text_status.set("Found at 0x18");
    } else {
        text_status.set("Not detected");
    }
}

// ---- Low-level I2C write -----------------------------------------------

bool OperaCakeView::write_ports(uint8_t port_a_idx, uint8_t port_b_idx) {
    // Configure all PCA9557 pins as outputs
    const uint8_t cfg[] = {REG_CONFIG, 0x00};
    if (!i2c0.transmit(OPERACAKE_I2C_ADDRESS, cfg, 2))
        return false;

    // Build and write output byte
    const uint8_t output = PORT_A_BITS[port_a_idx & 3] |
                           PORT_B_BITS[port_b_idx & 3] |
                           OUTPUT_BASE;
    const uint8_t out[] = {REG_OUTPUT, output};
    return i2c0.transmit(OPERACAKE_I2C_ADDRESS, out, 2);
}

// ---- Manual mode -------------------------------------------------------

void OperaCakeView::apply_manual() {
    if (write_ports(setting_port_a, setting_port_b))
        text_result.set("Applied OK");
    else
        text_result.set("Err: board not found");
}

// ---- Frequency mode ----------------------------------------------------

void OperaCakeView::apply_frequency() {
    // Read current receiver frequency (Hz) and convert to MHz
    const uint32_t freq_mhz =
        static_cast<uint32_t>(receiver_model.target_frequency() / 1'000'000ULL);

    // Check A1–A3 ranges in priority order; A4 is the catch-all fallback
    uint8_t port_idx = 3;  // default: A4/B4

    const uint32_t mins[4] = {setting_min_a1, setting_min_a2, setting_min_a3, setting_min_a4};
    const uint32_t maxs[4] = {setting_max_a1, setting_max_a2, setting_max_a3, setting_max_a4};

    for (uint8_t i = 0; i < 3; ++i) {
        if (freq_mhz >= mins[i] && freq_mhz < maxs[i]) {
            port_idx = i;
            break;
        }
    }

    // In frequency mode B mirrors A: A1↔B1, A2↔B2, A3↔B3, A4↔B4
    if (write_ports(port_idx, port_idx)) {
        const char port_name[] = {'A', static_cast<char>('1' + port_idx), '\0'};
        text_result.set(std::string("Freq→") + port_name +
                        " (" + to_string_dec_uint(freq_mhz) + "MHz)");
    } else {
        text_result.set("Err: board not found");
    }
}

// ---- Frame-sync handler (auto-monitor) ---------------------------------

void OperaCakeView::on_frame_sync() {
    if (setting_monitor == 0 || setting_mode != 1)
        return;

    // Fire at ~1 Hz (DisplayFrameSync fires at ~60 Hz)
    if (++frame_counter_ < 60)
        return;

    frame_counter_ = 0;
    apply_frequency();
}

// ---- Focus -------------------------------------------------------------

void OperaCakeView::focus() {
    options_mode.focus();
}

}  // namespace ui::external_app::opera_cake
