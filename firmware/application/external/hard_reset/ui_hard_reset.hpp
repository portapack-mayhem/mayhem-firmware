/*
 * Copyright (C) 2025 Pezsma
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

#ifndef __HARD_RESET_H__
#define __HARD_RESET_H__

#include "ui_navigation.hpp"

namespace ui::external_app::hard_reset {

class HardResetView : public ui::View {
   public:
    HardResetView(ui::NavigationView& nav);
    ~HardResetView();

    std::string title() const override { return "Hard Reset"; }
    void focus() override;
    void on_show() override;

   private:
    ui::NavigationView& nav_;
    ui::Button btn_yes{{UI_POS_X(1), UI_POS_Y_BOTTOM(4), UI_POS_WIDTH(10), UI_POS_HEIGHT(2)}, "Erase All"};
    ui::Button btn_no{{UI_POS_X_RIGHT(11), UI_POS_Y_BOTTOM(4), UI_POS_WIDTH(10), UI_POS_HEIGHT(2)}, "No"};
    ui::Console console_text{{UI_POS_X(0), UI_POS_Y(2), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(6)}};

    void delete_all_files_in_directory(const std::filesystem::path& dir_path);
    void clear_settings_folder();
};

}  // namespace ui::external_app::hard_reset

#endif  // __HARD_RESET_H__
