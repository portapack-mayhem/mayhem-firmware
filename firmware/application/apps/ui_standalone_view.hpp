/*
 * Copyright (C) 2024 Bernd Herzog
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

#ifndef __UI_STANDALONE_VIEW_H__
#define __UI_STANDALONE_VIEW_H__

#include "ui_navigation.hpp"
#include "event_m0.hpp"
#include "message.hpp"
#include "standalone_app.hpp"

namespace ui {

class StandaloneView : public View {
   public:
    StandaloneView(NavigationView& nav, uint8_t* app_image);
    virtual ~StandaloneView() override {
    }

    std::string title() const override { return (const char*)get_application_information()->app_name; };

    void focus() override;
    void paint(Painter& painter) override;

    void on_focus() override;
    bool on_key(const KeyEvent key) override;
    bool on_encoder(const EncoderEvent event) override;
    bool on_touch(const TouchEvent event) override;
    bool on_keyboard(const KeyboardEvent event) override;

    void on_after_attach() override;
    void on_before_detach() override;

    void frame_sync();

    void exit();

   private:
    bool initialized = false;
    NavigationView& nav_;
    uint8_t& _app_image;
    standalone_application_information_t* get_application_information() const {
        return reinterpret_cast<standalone_application_information_t*>(&_app_image);
    }

    MessageHandlerRegistration message_handler_sample{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->frame_sync();
        }};
};

}  // namespace ui

#endif /*__UI_STANDALONE_VIEW_H__*/
