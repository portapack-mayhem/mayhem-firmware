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

#include "ui_sd_over_usb.hpp"
#include "portapack_shared_memory.hpp"

namespace ui {

SdOverUsbView::SdOverUsbView(NavigationView& nav)
    : nav_(nav) {
    add_children({&labels,
                  &button_run});

    button_run.on_select = [this](Button&) {
        ui::Painter painter;
        painter.fill_rectangle(
            {0, 0, portapack::display.width(), portapack::display.height()},
            Theme::getInstance()->bg_darkest->background);

        painter.draw_bitmap(
            {portapack::display.width() / 2 - 8, portapack::display.height() / 2 - 8},
            bitmap_icon_hackrf,
            Theme::getInstance()->bg_darkest->foreground,
            Theme::getInstance()->bg_darkest->background);

        sdcDisconnect(&SDCD1);
        sdcStop(&SDCD1);

        portapack::shutdown(true);
        m4_init(portapack::spi_flash::image_tag_usb_sd, portapack::memory::map::m4_code, false);
        m0_halt();
        /* will not return*/
    };
}

void SdOverUsbView::focus() {
    button_run.focus();
}

} /* namespace ui */
