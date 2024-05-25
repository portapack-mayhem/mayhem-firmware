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

#include "ui_flash_utility.hpp"
#include "portapack_shared_memory.hpp"
#include "file_path.hpp"

namespace ui {

// Firmware image validation
static const char* hackrf_magic = "HACKRFFW";
#define FIRMWARE_INFO_AREA_OFFSET 0x400
#define FIRST_CHECKSUM_NIGHTLY 240125

Thread* FlashUtilityView::thread{nullptr};
static constexpr size_t max_filename_length = 26;

bool valid_firmware_file(std::filesystem::path::string_type path) {
    File firmware_file;
    bool require_checksum{false};
    uint32_t read_buffer[128];
    uint32_t checksum{FLASH_CHECKSUM_ERROR};  // initializing to invalid checksum in case file can't be read

    static_assert((FIRMWARE_INFO_AREA_OFFSET % sizeof(read_buffer)) == 0, "Read buffer size must divide evenly into FIRMWARE_INFO_AREA_OFFSET");

    // test read of the whole file just to validate checksum (baseband flash code will re-read when flashing)
    auto result = firmware_file.open(path.c_str());
    if (!result.is_valid()) {
        checksum = 0;
        for (uint32_t offset = 0; offset < FLASH_ROM_SIZE; offset += sizeof(read_buffer)) {
            auto readResult = firmware_file.read(&read_buffer, sizeof(read_buffer));

            if ((!readResult) || (readResult.value() != sizeof(read_buffer))) {
                // File was smaller than 1MB:
                // If version is such that the file SHOULD have been 1MB, call it a checksum error (otherwise say it's OK).
                checksum = (require_checksum) ? FLASH_CHECKSUM_ERROR : FLASH_EXPECTED_CHECKSUM;
                break;
            }

            // Did we just read the firmware info area?
            if (offset == FIRMWARE_INFO_AREA_OFFSET) {
                // If there's no info area (missing HACKRFFW signature), it could be an ancient FW version (so skipping check)
                if (memcmp(read_buffer, hackrf_magic, 8) == 0) {
                    char* version_string = (char*)&read_buffer[4];

                    // Require a 1MB firmware image with a valid checksum if release version >=v2.x or nightly >n_240125
                    if (((version_string[0] == 'v') && (std::atoi(&version_string[1]) >= 2)) ||
                        ((version_string[0] == 'n') && (version_string[1] == '_') && (std::atoi(&version_string[2]) >= FIRST_CHECKSUM_NIGHTLY)))
                        require_checksum = true;
                }
            }

            checksum += simple_checksum((uint32_t)read_buffer, sizeof(read_buffer));
        }
    }
    return (checksum == FLASH_EXPECTED_CHECKSUM);
}

FlashUtilityView::FlashUtilityView(NavigationView& nav)
    : nav_(nav) {
    add_children({&labels,
                  &menu_view});

    menu_view.set_parent_rect({0, 3 * 8, 240, 33 * 8});

    ensure_directory(apps_dir);
    ensure_directory(firmware_dir);

    auto add_firmware_items = [&](
                                  const std::filesystem::path& folder_path,
                                  const std::filesystem::path& wild,
                                  ui::Color color) {
        for (const auto& entry : std::filesystem::directory_iterator(folder_path, wild)) {
            auto filename = entry.path().filename();
            auto path = entry.path().native();

            menu_view.add_item({filename.string().substr(0, max_filename_length),
                                color,
                                &bitmap_icon_temperature,
                                [this, path](KeyEvent) {
                                    this->firmware_selected(path);
                                }});
        }
    };

    add_firmware_items(firmware_dir, u"*.bin", ui::Theme::getInstance()->fg_red->foreground);
    add_firmware_items(firmware_dir, u"*.tar", ui::Theme::getInstance()->fg_cyan->foreground);

    // add_firmware_items(user_firmware_folder,u"*.bin", ui::Theme::getInstance()->fg_cyan->foreground);
}

void FlashUtilityView::firmware_selected(std::filesystem::path::string_type path) {
    nav_.push<ModalMessageView>(
        "Warning!",
        "This will replace your\ncurrent firmware.\n\nIf things go wrong you are\nrequired to flash manually\nwith dfu.",
        YESNO,
        [this, path](bool choice) {
            if (choice) {
                std::filesystem::path::string_type full_path = firmware_dir.native() + u"/" + path;
                this->flash_firmware(full_path);
            }
        });
}

bool FlashUtilityView::endsWith(const std::u16string& str, const std::u16string& suffix) {
    if (str.length() >= suffix.length()) {
        std::u16string endOfString = str.substr(str.length() - suffix.length());
        return endOfString == suffix;
    } else {
        return false;
    }
}

std::filesystem::path FlashUtilityView::extract_tar(std::filesystem::path::string_type path, ui::Painter& painter) {
    //
    painter.fill_rectangle(
        {0, 0, portapack::display.width(), portapack::display.height()},
        Theme::getInstance()->bg_darkest->background);
    painter.draw_string({12, 24}, this->nav_.style(), "Unpacking TAR file...");

    auto res = UnTar::untar(path, [this](const std::string fileName) {
        ui::Painter painter;
        painter.fill_rectangle({0, 50, portapack::display.width(), 90}, Theme::getInstance()->bg_darkest->background);
        painter.draw_string({0, 60}, this->nav_.style(), fileName);
    });
    return res;
}

bool FlashUtilityView::flash_firmware(std::filesystem::path::string_type path) {
    ui::Painter painter;
    if (endsWith(path, u".tar")) {
        // extract, then update
        path = extract_tar(u'/' + path, painter).native();
    }

    if (path.empty() || !valid_firmware_file(path.c_str())) {
        painter.fill_rectangle({0, 50, portapack::display.width(), 90}, Theme::getInstance()->bg_darkest->background);
        painter.draw_string({0, 60}, *Theme::getInstance()->fg_red, "BAD FIRMWARE FILE OR W/R ERR");
        chThdSleepMilliseconds(5000);
        return false;
    }
    painter.fill_rectangle(
        {0, 0, portapack::display.width(), portapack::display.height()},
        Theme::getInstance()->bg_darkest->background);

    painter.draw_string({12, 24}, this->nav_.style(), "This will take 15 seconds.");
    painter.draw_string({12, 64}, this->nav_.style(), "Please wait while LED RX");
    painter.draw_string({12, 84}, this->nav_.style(), "is on and TX is flashing.");
    painter.draw_string({12, 124}, this->nav_.style(), "Device will then restart.");

    std::memcpy(&shared_memory.bb_data.data[0], path.c_str(), (path.length() + 1) * 2);
    m4_init(portapack::spi_flash::image_tag_flash_utility, portapack::memory::map::m4_code, false);
    m0_halt();
    return true;  // fixes compiler warning (line should not be reached due to halt)
}

void FlashUtilityView::focus() {
    menu_view.focus();
}

} /* namespace ui */