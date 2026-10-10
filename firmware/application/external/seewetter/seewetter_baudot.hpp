/*
 * Copyright (C) 2026 justin080
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

/* ITA2 (CCITT No. 2) decoder, as used by the DWD RTTY broadcasts. Header only and
 * free of PortaPack dependencies, so it can be unit tested on a PC. */

#ifndef __SEEWETTER_BAUDOT_HPP__
#define __SEEWETTER_BAUDOT_HPP__

#include <cstdint>

namespace ui::external_app::seewetter {

class Ita2Decoder {
   public:
    static constexpr uint8_t CODE_FIGS = 27;
    static constexpr uint8_t CODE_LTRS = 31;
    static constexpr uint8_t CODE_SPACE = 4;

    void set_usos(bool enable) { usos_ = enable; }
    void reset() { figures_ = false; }
    bool figures() const { return figures_; }

    /* Returns the printable character ('\n' for line feed) or 0 for nothing to print. */
    char decode(uint8_t code) {
        code &= 0x1F;
        if (code == CODE_FIGS) {
            figures_ = true;
            return 0;
        }
        if (code == CODE_LTRS) {
            figures_ = false;
            return 0;
        }
        if (code == CODE_SPACE && usos_) figures_ = false;
        const char c = figures_ ? figs_[code] : ltrs_[code];
        return c;
    }

    /* Encoder for tests: appends codes for an ASCII string, returns the count. */
    template <typename Sink>
    void encode(const char* s, Sink sink) {
        bool figs = false;
        sink(CODE_LTRS);
        for (; *s; s++) {
            char ch = *s;
            if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
            if (ch == '\n') {
                sink(8);  // CR
                sink(2);  // LF
                continue;
            }
            int idx = -1;
            bool in_figs = false;
            for (int i = 0; i < 32 && idx < 0; i++) {
                if (ltrs_[i] == ch && ch != 0) idx = i;
            }
            if (idx < 0) {
                for (int i = 0; i < 32 && idx < 0; i++) {
                    if (figs_[i] == ch && ch != 0) {
                        idx = i;
                        in_figs = true;
                    }
                }
            }
            if (idx < 0) continue;
            const bool shared = (idx == 2 || idx == 4 || idx == 8);
            if (!shared && in_figs != figs) {
                sink(in_figs ? CODE_FIGS : CODE_LTRS);
                figs = in_figs;
            }
            sink((uint8_t)idx);
        }
    }

   private:
    bool figures_{false};
    bool usos_{false};

    static constexpr char ltrs_[32] = {
        0, 'E', '\n', 'A', ' ', 'S', 'I', 'U',
        0, 'D', 'R', 'J', 'N', 'F', 'C', 'K',
        'T', 'Z', 'L', 'W', 'H', 'Y', 'P', 'Q',
        'O', 'B', 'G', 0, 'M', 'X', 'V', 0};

    /* ITA2 figures; the national positions (F, G, H) use the common ! & # */
    static constexpr char figs_[32] = {
        0, '3', '\n', '-', ' ', '\'', '8', '7',
        0, 0, '4', 0, ',', '!', ':', '(',
        '5', '+', ')', '2', '#', '6', '0', '1',
        '9', '?', '&', 0, '.', '/', '=', 0};
};

}  // namespace ui::external_app::seewetter

#endif /*__SEEWETTER_BAUDOT_HPP__*/
