/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
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

#ifndef __UI_DEBUG_PMEM_APP_H__
#define __UI_DEBUG_PMEM_APP_H__

#include "portapack_persistent_memory.hpp"
#include "ui_flash_utility.hpp"

using namespace portapack;
using namespace portapack::persistent_memory;

namespace ui::external_app::debug_pmem {

class DebugDumpView : public View {
   public:
    DebugDumpView(NavigationView& nav);

    void focus() override;

    std::string title() const override { return "DebugPMem"; };
    bool debug_dump_func();

   private:
    NavigationView& nav_;

    Text dump_output{
        {UI_POS_X(0), 19 * 8, screen_width, 16},
        ""};

    Button button_exit{
        {UI_POS_X_CENTER(8), UI_POS_Y_BOTTOM(4), 8 * 8, 32},
        "Exit"};
};

}  // namespace ui::external_app::debug_pmem

#endif /*__UI_DEBUG_PMEM_APP_H__*/
