/*
 * Copyright (C) 2026 Pezsma
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

#include "ui_hard_reset.hpp"

#include "portapack.hpp"
#include "file.hpp"
#include "file_path.hpp"
#include "ui_touch_calibration.hpp"
#include "ui_external_items_menu_loader.hpp"
#include "app_settings.hpp"

using namespace portapack;
namespace pmem = portapack::persistent_memory;

namespace ui::external_app::hard_reset {
HardResetView::HardResetView(ui::NavigationView& nav)
    : nav_(nav) {
    add_children({&chk_settings_file,
                  &chk_bad_apps,
                  &chk_pmem,
                  &txt_settings_file_count,
                  &txt_bad_apps_count,
                  &btn_yes,
                  &btn_no,
                  &text,
                  &txt_wait});

    txt_wait.set_style(Theme::getInstance()->warning_dark);

    btn_yes.on_select = [this](Button&) {
        {  // to  free painter after draw
            Painter painter;
            txt_wait.hidden(false);
            text.hidden(true);
            painter.fill_rectangle(text.screen_rect(), this->style().background);
            txt_wait.paint(painter);
        }
        if (chk_settings_file.value()) {
            clear_settings_folder();
        }
        if (chk_bad_apps.value()) {
            delete_bad_apps(apps_dir);
        }
        if (chk_pmem.value()) {
            pmem::cache::defaults();
            StatusRefreshMessage message{};
            EventDispatcher::send_message(message);
            nav_.replace<TouchCalibrationView>();
        } else {
            nav_.pop();
        }
    };

    btn_no.on_select = [this](Button&) {
        nav_.pop();
    };
    txt_settings_file_count.set_style(Theme::getInstance()->fg_magenta);
    txt_bad_apps_count.set_style(Theme::getInstance()->fg_magenta);

    signal_token_tick_second = rtc_time::signal_tick_second += [this]() {
        this->calculate();
        rtc_time::signal_tick_second -= signal_token_tick_second;
    };
}

HardResetView::~HardResetView() {
    rtc_time::signal_tick_second -= signal_token_tick_second;
}

void HardResetView::calculate() {
    settings_file_count = count_ini_files_in_directory(settings_dir);
    txt_settings_file_count.set(to_string_dec_uint(settings_file_count));

    bad_apps_count = count_bad_apps(apps_dir);
    txt_bad_apps_count.set(to_string_dec_uint(bad_apps_count));

    if (settings_file_count > 0) {
        chk_settings_file.set_value(true);
    }
    if (bad_apps_count > 0) {
        chk_bad_apps.set_value(true);
    }
    chk_pmem.set_value(true);
    txt_wait.hidden(true);
    text.set("Warning! Checked items will beerased (bad apps, P.Mem,      settings). Uncheck to keep.   Touch calibration is required only if P.Mem is reset.");
}

void HardResetView::focus() {
    btn_no.focus();
}

uint16_t HardResetView::count_ini_files_in_directory(const std::filesystem::path& dir_path) {
    int count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(dir_path, u"*.ini")) {
        if (std::filesystem::is_regular_file(entry.status())) {
            count++;
        }
    }
    return count;
}

void HardResetView::delete_ini_files_in_directory(const std::filesystem::path& dir_path) {
    bool found_and_deleted;
    do {
        found_and_deleted = false;
        for (const auto& entry : std::filesystem::directory_iterator(dir_path, u"*.ini")) {
            if (std::filesystem::is_regular_file(entry.status())) {
                auto full_path = dir_path / entry.path().filename();
                delete_file(full_path);
                found_and_deleted = true;
                break;  // iterator can become invalid if the dir is modified, so break and restart the loop after deletion
            }
        }
    } while (found_and_deleted);
}

void HardResetView::clear_settings_folder() {
    delete_ini_files_in_directory(settings_dir);
}

bool HardResetView::is_bad_app(const std::filesystem::path& file_path, bool is_ppma) {
    File app;
    if (app.open(file_path)) {
        return true;
    }
    bool is_bad = false;
    if (is_ppma) {
        application_information_t app_info = {};
        auto readResult = app.read(&app_info, sizeof(application_information_t));
        if (!readResult || readResult.value() != sizeof(application_information_t)) {
            is_bad = true;
        } else if (app_info.header_version != CURRENT_HEADER_VERSION) {
            is_bad = true;
        } else if (VERSION_MD5 != app_info.app_version) {
            is_bad = true;
        } else {
            // --- CHECKSUM VALIDATION ---
            // Similar to run_external_app: read through the file and calculate the checksum
            app.seek(0);
            uint32_t checksum = 0;
            uint8_t buffer[512];  // 512-byte buffer to conserve RAM

            while (true) {
                auto res = app.read(buffer, sizeof(buffer));
                if (!res) break;
                checksum += simple_checksum((uint32_t)buffer, res.value());
                if (res.value() < sizeof(buffer)) break;  // End of file
            }

            if (checksum != EXT_APP_EXPECTED_CHECKSUM) {
                is_bad = true;  // The file body is corrupted / truncated!
            }
        }
    } else {
        // PPMP validation
        standalone_application_information_t app_info = {};
        auto readResult = app.read(&app_info, sizeof(standalone_application_information_t));
        if (!readResult || readResult.value() != sizeof(standalone_application_information_t)) {
            is_bad = true;
        } else if (app_info.header_version > CURRENT_STANDALONE_APPLICATION_API_VERSION) {
            is_bad = true;
        }
    }

    app.close();
    return is_bad;
}

uint16_t HardResetView::count_bad_apps(const std::filesystem::path& apps_dir) {
    int bad_count = 0;

    // Check .ppma files
    for (const auto& entry : std::filesystem::directory_iterator(apps_dir, u"*.ppma")) {
        auto file_path = apps_dir / entry.path();
        if (is_bad_app(file_path, true)) {
            bad_count++;
        }
    }

    // Check .ppmp files
    for (const auto& entry : std::filesystem::directory_iterator(apps_dir, u"*.ppmp")) {
        auto file_path = apps_dir / entry.path();
        if (is_bad_app(file_path, false)) {
            bad_count++;
        }
    }

    return bad_count;
}

void HardResetView::delete_bad_apps(const std::filesystem::path& apps_dir) {
    // Delete bad .ppma files
    bool deleted_something;
    do {
        deleted_something = false;
        for (const auto& entry : std::filesystem::directory_iterator(apps_dir, u"*.ppma")) {
            auto file_path = apps_dir / entry.path();
            if (is_bad_app(file_path, true)) {
                delete_file(file_path);
                deleted_something = true;
                break;
            }
        }
    } while (deleted_something);

    // Delete bad .ppmp files
    do {
        deleted_something = false;
        for (const auto& entry : std::filesystem::directory_iterator(apps_dir, u"*.ppmp")) {
            auto file_path = apps_dir / entry.path();
            if (is_bad_app(file_path, false)) {
                delete_file(file_path);
                deleted_something = true;
                break;
            }
        }
    } while (deleted_something);
}

}  // namespace ui::external_app::hard_reset