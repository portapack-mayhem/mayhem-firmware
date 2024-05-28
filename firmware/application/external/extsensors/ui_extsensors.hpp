/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

// Code from https://github.com/Flipper-XFW/Xtreme-Apps/tree/04c3a60093e2c2378e79498b4505aa8072980a42/ble_spam/protocols
// Thanks for the work of the original creators!
// Saying thanks in the main view!

#ifndef __UI_EXTSENSORS_H__
#define __UI_EXTSENSORS_H__

#include "ui.hpp"
#include "ui_language.hpp"
#include "ui_navigation.hpp"
#include "ui_record_view.hpp"
#include "app_settings.hpp"
#include "utility.hpp"

using namespace ui;

namespace ui::external_app::extsensors {

class ExtSensorsView : public View {
   public:
    ExtSensorsView(NavigationView& nav);
    ~ExtSensorsView();

    void focus() override;

    std::string title() const override {
        return "ExtSensor";
    };

   private:
    NavigationView& nav_;

    bool has_data = false;

    Labels labels{
        {{0 * 8, 3 * 16}, "GPS:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 5 * 16}, "ORI:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 7 * 16}, "ENV:", Theme::getInstance()->fg_light->foreground}};

    Text text_info{{0 * 8, 0 * 8, 30 * 8, 16 * 1}, "Connect a compatible module..."};
    Text text_gps{{5 * 8, 3 * 16, 24 * 8, 16}, "-"};
    Text text_orientation{{5 * 8, 5 * 16, 24 * 8, 16}, "-"};
    Text text_envl1{{5 * 8, 7 * 16, 24 * 8, 16}, "-"};
    Text text_envl2{{1 * 8, 9 * 16, 24 * 8, 16}, "-"};

    void on_any();

    void on_gps(const GPSPosDataMessage* msg);
    void on_orientation(const OrientationDataMessage* msg);
    void on_environment(const EnvironmentDataMessage* msg);

    MessageHandlerRegistration message_handler_gps{
        Message::ID::GPSPosData,
        [this](Message* const p) {
            const auto message = static_cast<const GPSPosDataMessage*>(p);
            this->on_gps(message);
        }};
    MessageHandlerRegistration message_handler_orientation{
        Message::ID::OrientationData,
        [this](Message* const p) {
            const auto message = static_cast<const OrientationDataMessage*>(p);
            this->on_orientation(message);
        }};

    MessageHandlerRegistration message_handler_environment{
        Message::ID::EnvironmentData,
        [this](Message* const p) {
            const auto message = static_cast<const EnvironmentDataMessage*>(p);
            this->on_environment(message);
        }};
};
};  // namespace ui::external_app::extsensors

#endif /*__UI_EXTSENSORS_H__*/
