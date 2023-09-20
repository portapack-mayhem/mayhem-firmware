/*
 * Copyright (C) 2023 Kyle Reed
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

#ifndef __UI_IQ_TRIM_H__
#define __UI_IQ_TRIM_H__

#include "file.hpp"
#include "iq_trim.hpp"
#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_styles.hpp"
#include "ui_widget.hpp"

#include <array>

namespace ui {

/* Helper for drawing progress related to IQ trimming. */
/* TODO: Ideally this would all be done on second thread. */
class TrimProgressUI {
   public:
    void show_reading() {
        clear();
        p.draw_string({6 * 8, 5 * 16}, Styles::yellow, "Reading Capture...");
    }

    void show_trimming() {
        clear();
        p.draw_string({5 * 8, 5 * 16}, Styles::yellow, "Trimming Capture...");
    }

    void show_progress(uint8_t percent) {
        auto width = percent * screen_width / 100;
        p.draw_hline({0, 6 * 16}, width, Color::yellow());
    }

    void clear() {
        p.fill_rectangle({0 * 8, 4 * 16, screen_width, 3 * 16}, Color::black());
    }

    auto get_callback() {
        return [this](uint8_t percent) { show_progress(percent); };
    }

   private:
    Painter p{};
};

class IQTrimView : public View {
   public:
    IQTrimView(NavigationView& nav);

    std::string title() const override { return "IQ Trim"; }
    void paint(Painter& painter) override;
    void focus() override;

   private:
    void refresh_ui();
    bool read_capture(const std::filesystem::path& path);

    std::filesystem::path path_{};
    TrimRange trim_range_{};
    std::array<PowerBuckets::Bucket, screen_width> power_buckets_{};
    uint8_t amp_threshold = 5;
    TrimProgressUI progress_ui{};

    Labels labels{
        {{0 * 8, 0 * 16}, "Capture File:", Color::light_grey()},
        {{0 * 8, 6 * 16}, "Range:", Color::light_grey()},
    };

    TextField field_path{
        {0 * 8, 1 * 16, 30 * 8, 1 * 16},
        "Open File..."};

    Point pos_lines{0 * 8, 4 * 16};
    Dim height_lines{2 * 16};

    Text text_range{
        {7 * 8, 6 * 16, 20 * 8, 1 * 16},
        {}};

    Button button_trim{
        {11 * 8, 9 * 16, 8 * 8, 3 * 16},
        "Trim"};
};

} /* namespace ui */

#endif /*__UI_IQ_TRIM_H__*/
