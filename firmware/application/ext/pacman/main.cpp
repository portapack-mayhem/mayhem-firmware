/*
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

#include "ui_pacman.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::pacman {
__attribute__((noinline)) void initialize_app(ui::NavigationView& nav, void** p) {
    auto obj = (void*)new PacmanView(nav);
    *p = obj;
}
}  // namespace ui::external_app::pacman

extern "C" {

__attribute__((section(".external_app.app_pacman"), used, noinline)) void app_pacman(ui::NavigationView& nav, void** p) {
    ui::external_app::pacman::initialize_app(nav, p);
}

__attribute__((section(".external_app.app_pacman.ExternalAppEntry_pacman"), used, noinline)) void ExternalAppEntry_pacman(ui::NavigationView& nav, void** p) {
    app_pacman(nav, p);
}

__attribute__((section(".external_app.app_pacman.application_information"), used, noinline)) application_information_t _application_information_pacman = {
    0,
    ExternalAppEntry_pacman,
    /*&__flash_start__ */ (void*)0x10080000};
}
