/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#ifndef __UI_TOAST_H__
#define __UI_TOAST_H__

#include "ui.hpp"
#include "ui_navigation.hpp"

namespace ui {

class ToastView : public View {
   public:
    std::function<void(std::string&)> on_changed{};

    void focus() override;
    std::string title() const override { return "Text entry"; };

    void set_cursor(uint32_t pos);

   protected:
    ToastView(NavigationView& nav, std::string& str, size_t max_length);
    ~ToastView();


    ToastView(const ToastView&) = delete;
    ToastView(ToastView&&) = delete;
    ToastView& operator=(const ToastView&) = delete;
    ToastView& operator=(ToastView&&) = delete;

    void on_frame_sync();

    Painter painter;

    size_t target_cycle{0};
    size_t current_cycle{0};

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->on_frame_sync();
        }};
};

// Show the TextEntry view to receive keyboard input.
// NB: This function returns immediately. 'str' is taken
// by reference and its lifetime must be ensured by the
// caller until the TextEntry view is dismissed.
void toast(
    NavigationView& nav,
    std::string& str,
    size_t cycle);

} /* namespace ui */

#endif /*__UI_TOAST_H__*/
