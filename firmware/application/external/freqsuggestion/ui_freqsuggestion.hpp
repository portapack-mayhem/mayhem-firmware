/*
 * Copyright (C) 2026
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

#ifndef __UI_FREQSUGGESTION_H__
#define __UI_FREQSUGGESTION_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "ui_navigation.hpp"
#include "string_format.hpp"
#include "freqman_db.hpp"
#include <vector>
#include <string>

using namespace ui;

namespace ui::external_app::freqsuggestion {

// Extended frequency band entry - reuses freqman_entry fields
struct FrequencyBand {
    rf::Frequency freq_start;        // Starting frequency in Hz (maps to frequency_a)
    rf::Frequency freq_end;          // Ending frequency in Hz (maps to frequency_b)
    std::string band_name;           // Short band name (stored separately from description)
    std::string description;         // Additional info (maps to freqman_entry.description)
    freqman_index_t primary_mode;    // Primary demodulation mode (reuses freqman modulation)
    freqman_index_t secondary_mode;  // Secondary mode (reuses freqman modulation)
    uint32_t suggested_bw_min;       // Min bandwidth in Hz
    uint32_t suggested_bw_max;       // Max bandwidth in Hz
    std::string region;              // e.g., "US", "EU", "GLOBAL"
    std::string usage;               // e.g., "Amateur Radio", "Commercial", "Aviation"
    uint8_t suggested_lna;           // Suggested LNA gain (0-40)
    uint8_t suggested_vga;           // Suggested VGA gain (0-62)
    bool rf_amp_suggested;           // Whether to enable RF amp
};

// Antenna recommendation
struct AntennaRecommendation {
    std::string antenna_name;
    std::string recommendation;  // e.g., "Good match", "Extend to 3 elements", "Out of range"
};

class FreqSuggestionView : public View {
   public:
    FreqSuggestionView(NavigationView& nav);
    void focus() override;
    std::string title() const override { return "Freq Suggestion"; };

   private:
    NavigationView& nav_;
    std::vector<FrequencyBand> frequency_db{};
    std::vector<std::string> antenna_names{};

    void update_suggestions();
    void load_frequency_db();
    void load_antenna_list();
    void add_default_bands();
    FrequencyBand* find_band(rf::Frequency freq);
    std::string modulation_index_to_string(freqman_index_t mode);
    std::string format_bandwidth(uint32_t bw_min, uint32_t bw_max);
    std::string format_frequency_range(rf::Frequency start, rf::Frequency end);
    AntennaRecommendation get_antenna_recommendation(rf::Frequency freq, const std::string& antenna_name);

    // UI Components
    Labels labels{
        {{1 * 8, 1 * 16}, "Frequency:", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 3 * 16}, "Band Name:", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 4 * 16}, "Region:", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 5 * 16}, "Usage:", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 6 * 16}, "Demod Mode:", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 7 * 16}, "Bandwidth:", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 8 * 16}, "LNA / VGA:", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 9 * 16}, "RF Amp:", Theme::getInstance()->fg_light->foreground}};

    RxFrequencyField field_frequency{
        {13 * 8, 1 * 16},
        nav_};

    // Display fields for suggestions
    Text text_band_name{
        {13 * 8, 3 * 16, 17 * 8, 16},
        "-"};

    Text text_region{
        {13 * 8, 4 * 16, 17 * 8, 16},
        "-"};

    Text text_usage{
        {13 * 8, 5 * 16, 17 * 8, 16},
        "-"};

    Text text_demod_mode{
        {13 * 8, 6 * 16, 17 * 8, 16},
        "-"};

    Text text_bandwidth{
        {13 * 8, 7 * 16, 17 * 8, 16},
        "-"};

    Text text_gains{
        {13 * 8, 8 * 16, 17 * 8, 16},
        "-"};

    Text text_rf_amp{
        {13 * 8, 9 * 16, 17 * 8, 16},
        "-"};

    // Additional info section
    Labels labels_info{
        {{1 * 8, 11 * 16}, "Info:", Theme::getInstance()->fg_light->foreground}};

    Console console{
        {0, 12 * 16, screen_width, 120}};

    Button button_copy_settings{
        {UI_POS_X(0), UI_POS_Y_BOTTOM(3), 96, 32},
        "Apply"};

    Button button_exit{
        {UI_POS_X_RIGHT(12), UI_POS_Y_BOTTOM(3), 96, 32},
        "Back"};
};

}  // namespace ui::external_app::freqsuggestion

#endif /*__UI_FREQSUGGESTION_H__*/
