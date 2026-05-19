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

#ifndef __UI_HARD_RESET_H__
#define __UI_HARD_RESET_H__

#include "ui_navigation.hpp"
#include "signal.hpp"
using namespace ui;

namespace ui::external_app::hard_reset {

class HardResetView : public ui::View {
   public:
    HardResetView(ui::NavigationView& nav);
    ~HardResetView();

    std::string title() const override { return "Hard Reset"; }
    void focus() override;

   private:
    ui::NavigationView& nav_;

    SignalToken signal_token_tick_second{};
    ui::Button btn_yes{{UI_POS_X(1), UI_POS_Y_BOTTOM(4), UI_POS_WIDTH(11), UI_POS_HEIGHT(2)}, "Erase sel."};
    ui::Button btn_no{{UI_POS_X_RIGHT(11), UI_POS_Y_BOTTOM(4), UI_POS_WIDTH(10), UI_POS_HEIGHT(2)}, "No"};
    ui::Text text{{UI_POS_X(0), UI_POS_Y(6), UI_POS_MAXWIDTH, UI_POS_HEIGHT(5)}, ""};
    Checkbox chk_settings_file{{UI_POS_X(1), UI_POS_Y(1)}, 14, "Settings file:", true};
    Checkbox chk_bad_apps{{UI_POS_X(1), UI_POS_Y(2.5)}, 9, "Bad apps:", true};
    Checkbox chk_pmem{{UI_POS_X(1), UI_POS_Y(4)}, 11, "P.mem reset", true};

    Text txt_settings_file_count{{UI_POS_X(19), UI_POS_Y(1), UI_POS_WIDTH(5), UI_POS_HEIGHT(1)}, "?"};
    Text txt_bad_apps_count{{UI_POS_X(14), UI_POS_Y(2.5), UI_POS_WIDTH(5), UI_POS_HEIGHT(1)}, "?"};

    Text txt_wait{{UI_POS_X_CENTER(14), UI_POS_Y(8), UI_POS_WIDTH(14), UI_POS_HEIGHT(1)}, "Please wait..."};

    uint16_t count_ini_files_in_directory(const std::filesystem::path& dir_path);
    void delete_ini_files_in_directory(const std::filesystem::path& dir_path);
    void clear_settings_folder();
    void calculate();
    bool is_bad_app(const std::filesystem::path& file_path, bool is_ppma);
    uint16_t count_bad_apps(const std::filesystem::path& apps_dir);
    void delete_bad_apps(const std::filesystem::path& apps_dir);

    uint16_t settings_file_count = 0;
    uint16_t bad_apps_count = 0;
};

}  // namespace ui::external_app::hard_reset

#endif  // __UI_HARD_RESET_H__
