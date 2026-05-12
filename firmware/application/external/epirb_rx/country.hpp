/*
 * Copyright (C) 2026 Frederic BORRY - ADRASEC 31
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

#ifndef __COUNTRY_RX_H__
#define __COUNTRY_RX_H__

#include "string_format.hpp"
#include "file_reader.hpp"
#include "file_path.hpp"

namespace ui::external_app::epirb_rx {

#define DISABLE_COUNTRY_CACHE

// Country struct
struct Country {
    int16_t code;
    char alphaCode[4];   // 3 chars + null
    char shortName[11];  // 10 chars + null
};

class CountryManager {
   public:
    static void get_country(int code, Country& out) {
#ifndef DISABLE_COUNTRY_CACHE
        // Check cache
        for (int i = 0; i < cache_count; i++) {
            if (cache[i].code == code) {
                out = cache[i];
                if (i > 0) {  // Promotion
                    Country tmp = cache[i];
                    for (int j = i; j > 0; j--) cache[j] = cache[j - 1];
                    cache[0] = tmp;
                }
                return;
            }
        }
#endif
        // Cache miss => load from file
        if (load_from_file(code, out)) {
#ifndef DISABLE_COUNTRY_CACHE
            // Insert in cache
            int limit = (cache_count < 16) ? cache_count : 15;
            for (int j = limit; j > 0; j--) cache[j] = cache[j - 1];
            cache[0] = out;
            if (cache_count < 16) cache_count++;
#endif
        } else {
            out.code = code;
            out.alphaCode[0] = '\0';
            out.shortName[0] = '\0';
        }
    }

   private:
#ifndef DISABLE_COUNTRY_CACHE
    static Country cache[16];
    static int cache_count;
#endif

    static bool load_from_file(int target_code, Country& out) {
        FIL file;
        if (f_open(&file, (epirb_dir / u"RES/COUNTRY.RES").tchar(), FA_READ) != FR_OK) return false;

        TCHAR line[48];
        bool found = false;

        while (f_gets(line, sizeof(line) / sizeof(TCHAR), &file)) {
            // Light parsing method
            TCHAR* p = line;

            // Parse CODE
            int c = 0;
            while (*p >= '0' && *p <= '9') c = c * 10 + (*p++ - '0');
            if (c != target_code || *p != ':') continue;
            p++;  // Skip ':'

            out.code = (int16_t)c;

            // Parse ALPHA CODE (3 chars)
            for (int i = 0; i < 3 && *p != ':'; i++) out.alphaCode[i] = (char)*p++;
            out.alphaCode[3] = '\0';
            if (*p != ':') continue;
            p++;  // Skip ':'

            // Parse SHORT NAME
            int i = 0;
            while (i < static_cast<int>(sizeof(out.shortName) - 1) && *p != '\r' && *p != '\n' && *p != '\0') {
                out.shortName[i++] = (char)*p++;
            }
            out.shortName[i] = '\0';

            found = true;
            break;
        }

        f_close(&file);
        return found;
    }
};

}  // namespace ui::external_app::epirb_rx

#endif  // __COUNTRY_RX_H__
