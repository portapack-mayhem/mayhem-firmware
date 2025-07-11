/*
 * Copyright (C) 2025 RocketGod
 * Copyright (C) 2025 HTotoo
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

#ifndef __UI_GRAPHEQ_H__
#define __UI_GRAPHEQ_H__

#include "ui_widget.hpp"

class GraphEq : public Widget {
   public:
    std::function<void(GraphEq&)> on_select{};

    GraphEq(Rect parent_rect, bool clickable = false);
    GraphEq(const GraphEq&) = delete;
    GraphEq(GraphEq&&) = delete;
    GraphEq& operator=(const GraphEq&) = delete;
    GraphEq& operator=(GraphEq&&) = delete;

    bool is_paused() const;
    void set_paused(bool paused);
    bool is_clickable() const;

    void paint(Painter& painter) override;
    bool on_key(const KeyEvent key) override;
    bool on_touch(const TouchEvent event) override;
    bool on_keyboard(const KeyboardEvent event) override;
    void set_parent_rect(const Rect new_parent_rect) override;

    void getAccessibilityText(std::string& result) override;
    void getWidgetName(std::string& result) override;
    void update_audio_spectrum(const AudioSpectrum& spectrum);
    void set_theme(Color base_color, Color peak_color);

   private:
    bool is_calculated{false};
    bool paused_{false};
    bool clickable_{false};
    bool needs_background_redraw{true};  // Redraw background only when needed.
    Color base_color = Color(255, 0, 255);
    Color peak_color = Color(255, 255, 255);
    std::vector<ui::Dim> bar_heights;
    std::vector<ui::Dim> prev_bar_heights;

    ui::Dim y_top = 2 * 16;
    ui::Dim RENDER_HEIGHT = 288;
    ui::Dim BAR_WIDTH = 20;
    ui::Dim HORIZONTAL_OFFSET = 2;
    static const int NUM_BARS = 11;
    static const int BAR_SPACING = 2;
    static const int SEGMENT_HEIGHT = 10;
    static constexpr std::array<int16_t, NUM_BARS + 1> FREQUENCY_BANDS = {
        375,    // Bass warmth and low rumble (e.g., deep basslines, kick drum body)
        750,    // Upper bass punch (e.g., bass guitar punch, kick drum attack)
        1500,   // Lower midrange fullness (e.g., warmth in vocals, guitar body)
        2250,   // Midrange clarity (e.g., vocal presence, snare crack)
        3375,   // Upper midrange bite (e.g., instrument definition, vocal articulation)
        4875,   // Presence and edge (e.g., guitar bite, vocal sibilance start)
        6750,   // Lower brilliance (e.g., cymbal shimmer, vocal clarity)
        9375,   // Brilliance and air (e.g., hi-hat crispness, breathy vocals)
        13125,  // High treble sparkle (e.g., subtle overtones, synth shimmer)
        16875,  // Upper treble airiness (e.g., faint harmonics, room ambiance)
        20625,  // Top-end sheen (e.g., ultra-high harmonics, noise floor)
        24375   // Extreme treble limit (e.g., inaudible overtones, signal cutoff, static)
    };

    void calculate_params();  // re calculate some parameters based on parent_rect()
};

#endif /*__UI_GRAPHEQ_H__*/