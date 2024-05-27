/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2023 Bernd Herzog
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

#ifndef __UI_FLSH_UITILTY_H__
#define __UI_FLSH_UITILTY_H__

#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "string_format.hpp"
#include "ff.h"
#include "baseband_api.hpp"
#include "core_control.hpp"
#include "untar.hpp"
#include <cstdint>

#define FLASH_ROM_SIZE 1048576
#define FLASH_STARTING_ADDRESS 0x00000000
#define FLASH_EXPECTED_CHECKSUM 0x00000000
#define FLASH_CHECKSUM_ERROR 0xFFFFFFFF

namespace ui {

bool valid_firmware_file(std::filesystem::path::string_type path);

class FlashUtilityView : public View {
   public:
    FlashUtilityView(NavigationView& nav);

    void focus() override;

    std::string title() const override { return "Flash Utility"; };
    bool flash_firmware(std::filesystem::path::string_type path);

   private:
    NavigationView& nav_;

    bool confirmed = false;
    static Thread* thread;

    Labels labels{
        {{4, 4}, "Select firmware to flash:", Theme::getInstance()->bg_darkest->foreground}};

    MenuView menu_view{
        {0, 2 * 8, 240, 26 * 8},
        true};

    std::filesystem::path extract_tar(std::filesystem::path::string_type path, ui::Painter& painter);  // extracts the tar file, and returns the firmware.bin path from it. empty string if no fw
    void firmware_selected(std::filesystem::path::string_type path);

    bool endsWith(const std::u16string& str, const std::u16string& suffix);
};

} /* namespace ui */

#endif /*__UI_FLSH_UITILTY_H__*/
