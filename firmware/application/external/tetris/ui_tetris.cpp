/*
 * Copyright (C) 2024 Mark Thompson
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

#include "ui_tetris.hpp"

namespace ui::external_app::tetris {

#pragma GCC diagnostic push
// external code, so ignore warnings
#pragma GCC diagnostic ignored "-Weffc++"
#include "tetris.cpp"
#pragma GCC diagnostic pop

TetrisView::TetrisView(NavigationView& nav)
    : nav_(nav) {
    add_children({&dummy});
}

void TetrisView::paint(Painter& painter) {
    (void)painter;

    if (!initialized) {
        initialized = true;
        std::srand(LPC_RTC->CTIME0);
        main();
    }
}

void TetrisView::frame_sync() {
    check_fall_timer();
    set_dirty();
}

bool TetrisView::on_encoder(const EncoderEvent delta) {
    return check_encoder(delta);
}

bool TetrisView::on_key(const KeyEvent key) {
    return check_key(key);
}

}  // namespace ui::external_app::tetris
