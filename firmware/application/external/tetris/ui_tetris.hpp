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

#ifndef __UI_TETRIS_H__
#define __UI_TETRIS_H__

#include "ui_navigation.hpp"
#include "event_m0.hpp"
#include "message.hpp"
#include "irq_controls.hpp"
#include "random.hpp"
#include "lpc43xx_cpp.hpp"
#include "limits.h"

namespace ui::external_app::tetris {

class TetrisView : public View {
   public:
    TetrisView(NavigationView& nav);

    std::string title() const override { return "Tetris"; };

    void focus() override { dummy.focus(); };
    void paint(Painter& painter) override;
    void frame_sync();
    bool on_encoder(const EncoderEvent event) override;
    bool on_key(KeyEvent key) override;

   private:
    bool initialized = false;
    NavigationView& nav_;

    Button dummy{
        {240, 0, 0, 0},
        ""};

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->frame_sync();
        }};
};

}  // namespace ui::external_app::tetris

#endif /*__UI_TETRIS_H__*/
