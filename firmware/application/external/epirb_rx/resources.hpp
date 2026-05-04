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

#ifndef __RESOUCES_H__
#define __RESOUCES_H__

#include <cstring>
#include <vector>
#include "file_reader.hpp"
#include "file_path.hpp"

namespace ui::external_app::epirb_rx {

#define UNKNOWN_LABEL "Unk."

extern std::vector<std::string> beacon_res;
extern std::vector<std::string> freq;

class ResourceManager {
   public:
    static ResourceManager* getInstance();

    static ResourceManager* current;
    static void destroy();  // used from standalone app, to prevent memleak

    static const char* get_beacon_resource(uint8_t line) {
        ResourceManager* rm = getInstance();
        if (rm->beacon_res.empty()) {
            load_file((epirb_dir / u"RES/BEACON.RES"), rm->beacon_res);
        }
        if (line < rm->beacon_res.size()) {
            return rm->beacon_res[line].c_str();
        }
        return UNKNOWN_LABEL;
    }

    static const std::vector<std::string>& get_frequencies() {
        ResourceManager* rm = getInstance();
        if (rm->freq.empty()) {
            load_file((epirb_dir / u"FREQ.TXT"), rm->freq);
        }
        return rm->freq;
    }

   private:
    std::vector<std::string> beacon_res{};
    std::vector<std::string> freq{};

    static void load_file(const std::filesystem::path& path, std::vector<std::string>& vect) {
        FIL file;
        if (f_open(&file, path.tchar(), FA_READ) == FR_OK) {
            TCHAR line_buffer[32];
            while (f_gets(line_buffer, sizeof(line_buffer) / sizeof(TCHAR), &file)) {
                std::string s;
                for (int i = 0; line_buffer[i] != '\0' && line_buffer[i] != '\n'; i++) {
                    s += (char)line_buffer[i];
                }
                vect.push_back(std::move(s));
            }
            f_close(&file);
        }
        if (vect.empty()) vect.push_back(UNKNOWN_LABEL);
    }
};

}  // namespace ui::external_app::epirb_rx

#endif  // __RESOUCES_H__