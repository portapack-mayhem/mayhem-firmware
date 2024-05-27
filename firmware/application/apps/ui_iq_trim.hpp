/*
 * Copyright (C) 2023 Kyle Reed
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

#ifndef __UI_IQ_TRIM_H__
#define __UI_IQ_TRIM_H__

#include "file.hpp"
#include "iq_trim.hpp"
#include "optional.hpp"
#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_widget.hpp"

#include <array>

namespace ui {

/* Helper for drawing progress related to IQ trimming. */
/* TODO: Ideally this would all be done on second thread. */
class TrimProgressUI {
   public:
    void show_reading() {
        clear();
        p.draw_string({6 * 8, 5 * 16}, *Theme::getInstance()->fg_yellow, "Reading Capture...");
    }

    void show_trimming() {
        clear();
        p.draw_string({5 * 8, 5 * 16}, *Theme::getInstance()->fg_yellow, "Trimming Capture...");
    }

    void show_progress(uint8_t percent) {
        auto width = percent * screen_width / 100;
        p.draw_hline({0, 6 * 16 + 2}, width, Theme::getInstance()->fg_yellow->foreground);
    }

    void clear() {
        p.fill_rectangle({0 * 8, 4 * 16, screen_width, 3 * 16}, Theme::getInstance()->bg_darkest->background);
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
    IQTrimView(NavigationView& nav, const std::filesystem::path& path);

    std::string title() const override { return "IQ Trim"; }
    void paint(Painter& painter) override;
    void focus() override;

   private:
    void open_file(const std::filesystem::path& path);

    /* Update controls with latest values. */
    void refresh_ui();

    /* Update the start/end controls with trim range info. */
    void update_range_controls(iq::TrimRange trim_range);

    /* Collect capture info and samples to draw the UI. */
    void profile_capture();

    /* Determine the start and end buckets based on the cutoff. */
    void compute_range();

    /* Trims the capture file based on the settings. */
    bool trim_capture();

    NavigationView& nav_;

    std::filesystem::path path_{};
    Optional<iq::CaptureInfo> info_{};
    std::array<iq::PowerBuckets::Bucket, screen_width> power_buckets_{};
    TrimProgressUI progress_ui{};

    Labels labels{
        {{0 * 8, 0 * 16}, "Capture File:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 6 * 16}, "Start  :", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 7 * 16}, "End    :", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 8 * 16}, "Samples:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 9 * 16}, "Max Pwr:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 10 * 16}, "Cutoff :", Theme::getInstance()->fg_light->foreground},
        {{12 * 8, 10 * 16}, "%", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 12 * 16}, "Amplify:", Theme::getInstance()->fg_light->foreground},
        {{10 * 8, 12 * 16}, "x", Theme::getInstance()->fg_light->foreground},
    };

    TextField field_path{
        {0 * 8, 1 * 16, 30 * 8, 1 * 16},
        "Open File..."};

    Point pos_lines{0 * 8, 4 * 16};
    Dim height_lines{2 * 16};

    NumberField field_start{
        {9 * 8, 6 * 16},
        10,
        {0, 0},
        1,
        ' '};

    NumberField field_end{
        {9 * 8, 7 * 16},
        10,
        {0, 0},
        1,
        ' '};

    Text text_samples{
        {9 * 8, 8 * 16, 10 * 8, 1 * 16},
        "0"};

    Text text_max{
        {9 * 8, 9 * 16, 20 * 8, 1 * 16},
        "0"};

    NumberField field_cutoff{
        {9 * 8, 10 * 16},
        3,
        {1, 100},
        1,
        ' '};

    NumberField field_amplify{
        {9 * 8, 12 * 16},
        1,
        {1, 9},
        1,
        ' '};

    Button button_trim{
        {20 * 8, 16 * 16, 8 * 8, 2 * 16},
        "Trim"};
};

} /* namespace ui */

#endif /*__UI_IQ_TRIM_H__*/
