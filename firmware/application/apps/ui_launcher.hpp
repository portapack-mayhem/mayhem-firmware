/*
 * Copyright (C) 2023 Kyle Reed
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

#ifndef __UI_LAUNCHER_H__
#define __UI_LAUNCHER_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_painter.hpp"
#include "ui_styles.hpp"
#include "ui_widget.hpp"

#include "file_wrapper.hpp"
#include "optional.hpp"

#include <memory>
#include <string>

namespace ui {

class AppLoader {};

class AppLauncherView : public View {
   public:
    using app_start_fn = int(*)(void);

    AppLauncherView(NavigationView& nav);
    AppLauncherView(
        NavigationView& nav,
        const std::filesystem::path& path);

    std::string title() const override {
        return "App Launcher";
    };

   private:
    void run_application(const std::filesystem::path& path);
    NavigationView& nav_;

    Button button_open{
      {1 * 8, 2 * 16, 8 * 8, 2 * 16},
      "Open"};

    Text text_result{
      {1 * 8, 5 * 15, 8 * 8, 1 * 16}, ""};
};

}  // namespace ui

#endif  // __UI_LAUNCHER_H__
