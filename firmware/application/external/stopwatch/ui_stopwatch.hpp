/*
 * Copyright 2025 Mark Thompson
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

#ifndef __UI_STOPWATCH_H__
#define __UI_STOPWATCH_H__

#include "ui_navigation.hpp"

namespace ui::external_app::stopwatch {

class StopwatchView : public View {
   public:
    StopwatchView(NavigationView& nav);
    void focus() override;
    void paint(Painter& painter) override;
    void frame_sync();
    std::string title() const override { return "Stopwatch"; };

   private:
    void run();
    void stop();
    void reset();
    void lap();

    bool running{false};
    long long start_time{0};
    long long end_time{0};
    long long lap_time{0};
    long long previously_elapsed{0};

    Labels labels{
        {{0 * 8, 1 * 16}, "TOTAL:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 7 * 16}, "LAP:", Theme::getInstance()->fg_light->foreground},
    };

    BigFrequency big_display{
        {4, 2 * 16 + 4, 28 * 8, 52},
        0};

    BigFrequency lap_display{
        {4, 8 * 16 + 4, 28 * 8, 52},
        0};

    Button button_run_stop{
        {72, 210, 96, 24},
        "START"};

    Button button_reset_lap{
        {72, 240, 96, 24},
        "RESET"};

    Button button_done{
        {72, 270, 96, 24},
        "EXIT"};

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->frame_sync();
        }};
};

}  // namespace ui::external_app::stopwatch

#endif /*__UI_STOPWATCH_H__*/