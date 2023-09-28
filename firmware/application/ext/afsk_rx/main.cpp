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

#include "ui_afsk_rx.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::afsk_rx {
__attribute__((noinline)) void initialize_app(ui::NavigationView& nav, void** p) {
    auto obj = (void*)new AFSKRxView(nav);
    *p = obj;
}
}  // namespace ui::external_app::afsk_rx

extern "C" {

__attribute__((section(".external_app.app_afsk_rx"), used, noinline)) void app_afsk_rx(ui::NavigationView& nav, void** p) {
    ui::external_app::afsk_rx::initialize_app(nav, p);
}

__attribute__((section(".external_app.app_afsk_rx.ExternalAppEntry_afsk_rx"), used, noinline)) void ExternalAppEntry_afsk_rx(ui::NavigationView& nav, void** p) {
    app_afsk_rx(nav, p);
}

__attribute__((section(".external_app.app_afsk_rx.application_information"), used, noinline)) application_information_t _application_information_afsk_rx = {
    0,
    ExternalAppEntry_afsk_rx,
    /*&__flash_start__ */ (void*)0x10086000};
}
