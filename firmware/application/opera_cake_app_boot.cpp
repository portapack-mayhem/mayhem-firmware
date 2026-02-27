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
 * Opera Cake system-level support
 * --------------------------------
 * Provides boot-time restore and automatic frequency-based antenna
 * switching, mirroring how the HackRF firmware calls
 * operacake_set_range() from tuning_set_frequency().
 *
 * Manual mode   : the saved Port-A and Port-B selections are applied.
 * Frequency mode: on_frequency_changed() is called by ReceiverModel
 *                 on every retune; it looks up the new frequency in the
 *                 stored ranges and switches ports only when the active
 *                 range changes.
 */

#include "opera_cake_app_boot.hpp"

#include "app_settings.hpp"
#include "portapack.hpp"

using namespace portapack;
using std::literals::operator""sv;

namespace opera_cake {

// ---- PCA9557 constants --------------------------------------------------

static constexpr uint8_t OPERACAKE_I2C_ADDRESS = 0x18;
static constexpr uint8_t REG_OUTPUT = 0x01;
static constexpr uint8_t REG_CONFIG = 0x03;

static constexpr uint8_t PORT_A_BITS[NUM_PORTS] = {
    0x00,  // A1
    0x20,  // A2
    0x40,  // A3
    0x60,  // A4
};
static constexpr uint8_t PORT_B_BITS[NUM_PORTS] = {
    0x00,  // B1
    0x08,  // B2
    0x10,  // B3
    0x18,  // B4
};
static constexpr uint8_t OUTPUT_BASE = 0x03;  // /OE=0, LEDEN2=1, LEDEN=1

// Short timeout: if the board is absent or the bus is stuck, give up fast.
static constexpr systime_t I2C_TIMEOUT_TICKS = MS2ST(50);

// ---- Shared state -------------------------------------------------------
// Mirrors the HackRF firmware's operacake_range array + current_range.

static uint8_t oc_mode_{0};              // 0 = manual, 1 = frequency
static bool oc_board_present_{false};
static FreqRanges oc_ranges_{{1, 30, 300, 1000}, {30, 300, 1000, 6000}};
static int8_t oc_current_port_{-1};      // -1 = not yet determined

// ---- Internal helpers ---------------------------------------------------

static bool write_ports(uint8_t port_a_idx, uint8_t port_b_idx) {
    const uint8_t cfg[] = {REG_CONFIG, 0x00};
    if (!i2c0.transmit(OPERACAKE_I2C_ADDRESS, cfg, 2, I2C_TIMEOUT_TICKS))
        return false;

    const uint8_t output = PORT_A_BITS[port_a_idx & 3] |
                           PORT_B_BITS[port_b_idx & 3] |
                           OUTPUT_BASE;
    const uint8_t out[] = {REG_OUTPUT, output};
    return i2c0.transmit(OPERACAKE_I2C_ADDRESS, out, 2, I2C_TIMEOUT_TICKS);
}

// ---- Public API ---------------------------------------------------------

void on_frequency_changed(rf::Frequency freq_hz) {
    // Fast path: nothing to do if not in frequency mode or no board.
    if (oc_mode_ != 1 || !oc_board_present_)
        return;

    const uint32_t freq_mhz =
        static_cast<uint32_t>(freq_hz / 1'000'000LL);

    // Check A1-A3 ranges in priority order; A4 is the catch-all fallback.
    // This matches HackRF firmware operacake_set_range() logic.
    uint8_t port_idx = 3;  // default: A4/B4
    for (uint8_t i = 0; i < 3; ++i) {
        if (freq_mhz >= oc_ranges_.mins[i] && freq_mhz < oc_ranges_.maxs[i]) {
            port_idx = i;
            break;
        }
    }

    // Only write I2C if the port actually changed (avoid bus traffic).
    if (static_cast<int8_t>(port_idx) == oc_current_port_)
        return;

    if (write_ports(port_idx, port_idx))
        oc_current_port_ = static_cast<int8_t>(port_idx);
}

bool update_config(
    uint8_t mode,
    uint8_t port_a,
    uint8_t port_b,
    const FreqRanges& ranges) {
    oc_mode_ = mode;
    oc_ranges_ = ranges;
    oc_current_port_ = -1;  // force re-evaluation

    // Detect board if not already known.
    if (!oc_board_present_)
        oc_board_present_ = i2c0.probe(OPERACAKE_I2C_ADDRESS, I2C_TIMEOUT_TICKS);

    if (!oc_board_present_)
        return false;

    if (mode == 0) {
        // Manual mode: apply the selected ports immediately.
        return write_ports(port_a, port_b);
    } else {
        // Frequency mode: apply based on current receiver frequency.
        on_frequency_changed(receiver_model.target_frequency());
        return true;
    }
}

bool is_board_present() {
    return oc_board_present_;
}

void restore_at_boot() {
    // Load persisted settings (same store name as the app uses).
    uint8_t setting_mode{0};
    uint8_t setting_port_a{0};
    uint8_t setting_port_b{0};
    uint32_t setting_min_a1{1}, setting_max_a1{30};
    uint32_t setting_min_a2{30}, setting_max_a2{300};
    uint32_t setting_min_a3{300}, setting_max_a3{1000};
    uint32_t setting_min_a4{1000}, setting_max_a4{6000};

    SettingBindings bindings{
        {"mode"sv, &setting_mode},
        {"port_a"sv, &setting_port_a},
        {"port_b"sv, &setting_port_b},
        {"min_a1"sv, &setting_min_a1},
        {"max_a1"sv, &setting_max_a1},
        {"min_a2"sv, &setting_min_a2},
        {"max_a2"sv, &setting_max_a2},
        {"min_a3"sv, &setting_min_a3},
        {"max_a3"sv, &setting_max_a3},
        {"min_a4"sv, &setting_min_a4},
        {"max_a4"sv, &setting_max_a4},
    };
    load_settings("opera_cake"sv, bindings);

    // Clamp to valid ranges (guard against corrupted settings).
    if (setting_mode > 1) setting_mode = 0;
    if (setting_port_a > 3) setting_port_a = 0;
    if (setting_port_b > 3) setting_port_b = 0;

    // Populate shared state so on_frequency_changed() works from boot.
    oc_mode_ = setting_mode;
    oc_ranges_.mins[0] = static_cast<uint16_t>(setting_min_a1);
    oc_ranges_.maxs[0] = static_cast<uint16_t>(setting_max_a1);
    oc_ranges_.mins[1] = static_cast<uint16_t>(setting_min_a2);
    oc_ranges_.maxs[1] = static_cast<uint16_t>(setting_max_a2);
    oc_ranges_.mins[2] = static_cast<uint16_t>(setting_min_a3);
    oc_ranges_.maxs[2] = static_cast<uint16_t>(setting_max_a3);
    oc_ranges_.mins[3] = static_cast<uint16_t>(setting_min_a4);
    oc_ranges_.maxs[3] = static_cast<uint16_t>(setting_max_a4);
    oc_current_port_ = -1;

    // Bail out quickly if no board is connected.
    oc_board_present_ = i2c0.probe(OPERACAKE_I2C_ADDRESS, I2C_TIMEOUT_TICKS);
    if (!oc_board_present_)
        return;

    if (setting_mode == 0) {
        // Manual mode: restore the saved A0->Ax / B0->Bx connection.
        write_ports(setting_port_a, setting_port_b);
    } else {
        // Frequency mode: no receiver is running at boot, so select the
        // catch-all fallback port (A4/B4).  Automatic switching will kick
        // in as soon as a receiver app starts tuning.
        write_ports(3, 3);
        oc_current_port_ = 3;
    }
}

}  // namespace opera_cake
