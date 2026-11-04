/*
 * Copyright (C) 2026 TehRabbitt, Rabbit-Labs LLC
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

#include "lcd_tuning_file.hpp"

#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"
#include "file.hpp"
#include "file_path.hpp"
#include "string_format.hpp"

#include <cstring>

namespace lcd_tuning_file {

#define LCD_TUNING_FILE u"lcd_gamma.txt"

static std::string hex_row(const uint8_t* p, size_t n) {
    std::string s;
    for (size_t i = 0; i < n; i++) s += to_string_hex(p[i], 2);
    return s;
}

static int hex_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static bool parse_hex_key(const std::string& s, const char* key, uint8_t* out, size_t n) {
    const auto pos = s.find(key);
    if (pos == std::string::npos) return false;
    size_t i = pos + strlen(key);
    for (size_t b = 0; b < n; b++, i += 2) {
        if (i + 1 >= s.size()) return false;
        const int h = hex_nibble(s[i]), l = hex_nibble(s[i + 1]);
        if (h < 0 || l < 0) return false;
        out[b] = (h << 4) | l;
    }
    return true;
}

bool read(lcd::panel_tuning_t& t, bool& ips) {
    auto r = File::read_file(settings_dir / LCD_TUNING_FILE);
    if (!r) return false;
    const std::string& s = *r;
    lcd::panel_tuning_t n{};
    if (!parse_hex_key(s, "E0=", n.gamma_pos, lcd::GAMMA_LEN) ||
        !parse_hex_key(s, "E1=", n.gamma_neg, lcd::GAMMA_LEN) ||
        !parse_hex_key(s, "C5=", n.vcom1, 2) ||
        !parse_hex_key(s, "C7=", &n.vcom2, 1))
        return false;
    if (s.find("BGR=1") != std::string::npos) n.flags |= lcd::TUNING_BGR_SWAP;
    // Files without a MODE line (older exports) are treated as IPS.
    ips = s.find("MODE=TN") == std::string::npos;
    t = n;
    return true;
}

bool write(const lcd::panel_tuning_t& t, const bool ips) {
    ensure_directory(settings_dir);
    File f;
    if (f.create(settings_dir / LCD_TUNING_FILE)) return false;
    f.write_line("# Mayhem LCD tuning (ILI9341 240x320)");
    f.write_line(std::string("MODE=") + (ips ? "IPS" : "TN"));
    f.write_line("E0=" + hex_row(t.gamma_pos, lcd::GAMMA_LEN));
    f.write_line("E1=" + hex_row(t.gamma_neg, lcd::GAMMA_LEN));
    f.write_line("C5=" + hex_row(t.vcom1, 2));
    f.write_line("C7=" + to_string_hex(t.vcom2, 2));
    f.write_line(std::string("BGR=") + ((t.flags & lcd::TUNING_BGR_SWAP) ? "1" : "0"));
    return true;
}

void remove() {
    delete_file(settings_dir / LCD_TUNING_FILE);
}

void apply(const bool force) {
    if (portapack::device_type != portapack::DEV_PORTAPACK) return;
    const bool had_override = lcd::ILI9341::has_override();
    lcd::panel_tuning_t t;
    bool ips = false;
    const bool use_file = read(t, ips) && ips == portapack::persistent_memory::config_lcd_ips();
    lcd::ILI9341::set_override(use_file ? &t : nullptr);
    if (force || use_file || had_override) portapack::display.apply_panel_config();
}

}  // namespace lcd_tuning_file
