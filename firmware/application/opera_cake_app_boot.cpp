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
 * Opera Cake boot restore
 * -----------------------
 * Restores the Opera Cake antenna-switch board to its last saved state
 * at PortaPack boot, without requiring the user to open the app.
 *
 * Manual mode   : the saved Port-A and Port-B selections are applied.
 * Frequency mode: A4/B4 (the catch-all fallback port) is selected because
 *                 no receiver frequency is active at boot time.
 */

#include "opera_cake_app_boot.hpp"

#include "app_settings.hpp"
#include "portapack.hpp"

using namespace portapack;
using std::literals::operator""sv;

namespace opera_cake {

// ---- PCA9557 constants (mirrors ui_opera_cake.cpp) ---------------------

static constexpr uint8_t OPERACAKE_I2C_ADDRESS = 0x18;
static constexpr uint8_t REG_OUTPUT = 0x01;
static constexpr uint8_t REG_CONFIG = 0x03;

static constexpr uint8_t PORT_A_BITS[4] = {
    0x00,  // A1
    0x20,  // A2
    0x40,  // A3
    0x60,  // A4
};
static constexpr uint8_t PORT_B_BITS[4] = {
    0x00,  // B1
    0x08,  // B2
    0x10,  // B3
    0x18,  // B4
};
static constexpr uint8_t OUTPUT_BASE = 0x03;  // /OE=0, LEDEN2=1, LEDEN=1

// Short timeout: if the board is absent or the bus is stuck, give up fast.
static constexpr systime_t I2C_TIMEOUT_TICKS = MS2ST(50);

// ---- Internal helpers --------------------------------------------------

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

// ---- Public entry point ------------------------------------------------

void restore_at_boot() {
    // Load persisted settings (same store name as the app uses).
    uint8_t setting_mode{0};
    uint8_t setting_port_a{0};
    uint8_t setting_port_b{0};
    // Frequency-range fields are loaded but not used at boot;
    // only the mode and port selections matter here.
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

    // Bail out quickly if no board is connected.
    if (!i2c0.probe(OPERACAKE_I2C_ADDRESS, I2C_TIMEOUT_TICKS))
        return;

    if (setting_mode == 0) {
        // Manual mode: restore the saved A0→Ax / B0→Bx connection.
        write_ports(setting_port_a, setting_port_b);
    } else {
        // Frequency mode: no receiver is running at boot, so select the
        // catch-all fallback port (A4/B4).
        write_ports(3, 3);
    }
}

}  // namespace opera_cake
