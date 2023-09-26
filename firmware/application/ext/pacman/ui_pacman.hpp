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

#ifndef __UI_PACMAN_H__
#define __UI_PACMAN_H__

#include "ui_navigation.hpp"
#include "event_m0.hpp"
#include "message.hpp"

namespace ui::external_app::pacman {

class PacmanView : public View {
   public:
    PacmanView(NavigationView& nav);

    void focus() override;

    std::string title() const override { return "Pacman"; };

    void paint(Painter& painter) override;
    void frame_sync();

   private:
    NavigationView& nav_;

    bool is_blue = false;
    Labels labels{
        {{3 * 8, 2 * 16}, "Click Run to start the", Color::white()},
        {{3 * 8, 3 * 16}, "Example.", Color::white()},
    };

    Button button_run{
        {9 * 8, 15 * 16, 12 * 8, 3 * 16},
        "Run"};

    MessageHandlerRegistration message_handler_sample{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->frame_sync();
        }};
};

}  // namespace ui::external_app::pacman

#endif /*__UI_PACMAN_H__*/
