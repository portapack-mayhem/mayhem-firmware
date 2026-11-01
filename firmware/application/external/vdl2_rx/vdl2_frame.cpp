/*
 * Copyright (C) 2026 Xmoo26
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

#include "vdl2_frame.hpp"

namespace ui::external_app::vdl2_rx {

namespace {

constexpr uint8_t STX = 0x02;
constexpr uint8_t ETX = 0x03;
constexpr uint8_t ETB = 0x17;
constexpr uint8_t DEL = 0x7F;

// 4 octets, 7 bits each (the lowest is the address extension bit), sent reversed:
// 24 bits of address, 3 of type, 1 of command / response.
void parse_address(const uint8_t* p, uint32_t& address, uint8_t& type) {
    uint32_t raw = 0;
    for (size_t i = 0; i < 4; i++)
        raw |= (uint32_t)(p[i] >> 1) << (7 * i);
    uint32_t v = 0;
    for (size_t i = 0; i < 28; i++)
        v |= ((raw >> i) & 1) << (27 - i);
    address = v & 0xFFFFFF;
    type = (v >> 24) & 7;
}

std::string text_of(const uint8_t* p, const size_t n) {
    std::string s{};
    for (size_t i = 0; i < n; i++) {
        const char c = p[i] & 0x7F;
        s += (c >= 32 && c < 127) ? c : ' ';
    }
    return s;
}

std::string trim(std::string s) {
    while (!s.empty() && s.back() == ' ')
        s.pop_back();
    size_t start = 0;
    while (start < s.size() && (s[start] == ' ' || s[start] == '.'))
        start++;
    return s.substr(start);
}

// ACARS block: mode, registration (7), technical ack, label (2), block id, [STX text], ETX or
// ETB, CRC (2), DEL. Characters carry an odd parity bit. Downlinks start their text with the
// message number (4) and the flight (6).
bool parse_acars(const uint8_t* p, size_t n, Frame& f) {
    if (n >= 1 && (p[n - 1] & 0x7F) == DEL)
        n--;
    if (n < 15)
        return false;
    n -= 2;  // CRC
    if ((p[n - 1] & 0x7F) == ETX || (p[n - 1] & 0x7F) == ETB)
        n--;

    f.kind = Frame::Kind::ACARS;
    f.mode = p[0] & 0x7F;
    f.reg = trim(text_of(p + 1, 7));
    f.label = text_of(p + 9, 2);
    if ((p[10] & 0x7F) == DEL)
        f.label[1] = 'd';  // "_" + DEL: a general response
    f.block_id = p[11] & 0x7F;
    if (n <= 12 || (p[12] & 0x7F) != STX)
        return true;

    const uint8_t* t = p + 13;
    size_t len = n - 13;
    if (f.block_id >= '0' && f.block_id <= '9' && len >= 10) {
        f.msg_num = text_of(t, 4);
        f.flight = trim(text_of(t + 4, 6));
        t += 10;
        len -= 10;
    }
    f.text = trim(text_of(t, len));
    return true;
}

}  // namespace

bool parse_frame(const uint8_t* d, const size_t n, Frame& f) {
    if (n < 9)
        return false;
    parse_address(d, f.dst, f.dst_type);
    parse_address(d + 4, f.src, f.src_type);
    const uint8_t control = d[8];
    if (control & 1) {
        f.kind = Frame::Kind::Link;  // supervisory or unnumbered (XID and the like)
        return true;
    }
    const uint8_t* info = d + 9;
    const size_t len = n - 9;
    if (len > 3 && info[0] == 0xFF && info[1] == 0xFF && info[2] == 0x01)
        return parse_acars(info + 3, len - 3, f);
    f.kind = Frame::Kind::Data;  // X.25 / ATN
    return true;
}

}  // namespace ui::external_app::vdl2_rx
