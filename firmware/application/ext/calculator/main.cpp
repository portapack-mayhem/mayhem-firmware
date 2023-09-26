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

#include "ui_calculator.hpp"
#include "ui_navigation.hpp"

namespace ui::external_app::calculator {
__attribute__((noinline)) void initialize_app(ui::NavigationView& nav, void** p) {
    auto obj = (void*)new CalculatorView(nav);
    *p = obj;
}
}  // namespace ui::external_app::calculator

extern "C" __attribute__((section(".external_app.app_calculator"), used)) void app_calculator(void* nav, void** p) {
    ui::external_app::calculator::initialize_app(reinterpret_cast<ui::NavigationView&>(nav), p);
}
