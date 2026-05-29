/*
 * Copyright (C) 2024 PortaPack Mayhem
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

#ifndef __UI_TWO_TONE_PAGER_H__
#define __UI_TWO_TONE_PAGER_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_transmitter.hpp"
#include "ui_textentry.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "portapack.hpp"
#include "message.hpp"
#include "string_format.hpp"

namespace ui::external_app::two_tone_pager {

class TwoTonePagerView : public View {
   public:
    TwoTonePagerView(NavigationView& nav);
    ~TwoTonePagerView();

    TwoTonePagerView(const TwoTonePagerView&) = delete;
    TwoTonePagerView(TwoTonePagerView&&) = delete;
    TwoTonePagerView& operator=(const TwoTonePagerView&) = delete;
    TwoTonePagerView& operator=(TwoTonePagerView&&) = delete;

    void focus() override;
    std::string title() const override { return "2-Tone TX"; };

   private:
    NavigationView& nav_;

    static constexpr uint32_t SAMPLE_RATE = 1536000;
    static constexpr size_t MOTO_TONE_COUNT = 45;
    static constexpr size_t CTCSS_COUNT = 51;
    static constexpr uint32_t CUSTOM_TONE_IDX = MOTO_TONE_COUNT;  // sentinel: one past the table, use custom Hz field

    TxRadioState radio_state_{
        154280000ULL,  // 154.280 MHz — common VHF paging frequency
        1750000,
        SAMPLE_RATE};

    // Persisted settings
    uint32_t ctcss_idx{0};
    uint32_t tone_a_idx{8};   // 405.3 Hz
    uint32_t tone_b_idx{24};  // 813.9 Hz
    uint32_t dur_a{1000};
    uint32_t dur_b{3000};
    uint32_t gap_ms{0};
    uint32_t preset_slot{1};
    uint32_t custom_freq_a_hz{405};  // used when tone_a_idx == CUSTOM_TONE_IDX
    uint32_t custom_freq_b_hz{814};

    // Five preset slots — encoded tone/timing data and a user-chosen display name
    std::string preset_1{"0,8,24,1000,3000,0"};
    std::string preset_2{"0,8,24,1000,3000,0"};
    std::string preset_3{"0,8,24,1000,3000,0"};
    std::string preset_4{"0,8,24,1000,3000,0"};
    std::string preset_5{"0,8,24,1000,3000,0"};
    std::string preset_name_1{""};
    std::string preset_name_2{""};
    std::string preset_name_3{""};
    std::string preset_name_4{""};
    std::string preset_name_5{""};

    app_settings::SettingsManager settings_{
        "tx_twotone",
        app_settings::Mode::TX,
        {
            {"ctcss"sv, &ctcss_idx},
            {"tone_a"sv, &tone_a_idx},
            {"tone_b"sv, &tone_b_idx},
            {"dur_a"sv, &dur_a},
            {"dur_b"sv, &dur_b},
            {"gap"sv, &gap_ms},
            {"slot"sv, &preset_slot},
            {"custom_a"sv, &custom_freq_a_hz},
            {"custom_b"sv, &custom_freq_b_hz},
            {"preset1"sv, &preset_1},
            {"preset2"sv, &preset_2},
            {"preset3"sv, &preset_3},
            {"preset4"sv, &preset_4},
            {"preset5"sv, &preset_5},
            {"pname1"sv, &preset_name_1},
            {"pname2"sv, &preset_name_2},
            {"pname3"sv, &preset_name_3},
            {"pname4"sv, &preset_name_4},
            {"pname5"sv, &preset_name_5},
        }};

    bool start_tx();
    void stop_tx();
    void on_tx_progress(uint32_t progress, bool done);
    void apply_timing_preset(size_t idx);
    void save_preset(uint32_t slot);
    void load_preset(uint32_t slot);
    std::string encode_preset() const;
    void decode_preset(const std::string& s);
    std::string& slot_ref(uint32_t slot);
    std::string& slot_name_ref(uint32_t slot);
    uint32_t tone_delta(uint32_t freq_x10) const;
    uint32_t ms_to_samples(uint32_t ms) const;
    size_t detect_timing_preset() const;
    void update_tx_time();
    void update_slot_name_display();

    // --- Widgets ---

    Labels labels{
        {{0 * 8, 1 * 16}, "CTCSS:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 2 * 16}, "A:", Theme::getInstance()->fg_light->foreground},
        {{13 * 8, 2 * 16}, "B:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 3 * 16}, "ADur:", Theme::getInstance()->fg_light->foreground},
        {{9 * 8, 3 * 16}, "ms", Theme::getInstance()->fg_light->foreground},
        {{12 * 8, 3 * 16}, "BDur:", Theme::getInstance()->fg_light->foreground},
        {{21 * 8, 3 * 16}, "ms", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 4 * 16}, "Gap: ", Theme::getInstance()->fg_light->foreground},
        {{9 * 8, 4 * 16}, "ms", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 5 * 16}, "Timing:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 6 * 16}, "Slot: ", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 9 * 16}, "AHz:", Theme::getInstance()->fg_light->foreground},
        {{8 * 8, 9 * 16}, "Hz", Theme::getInstance()->fg_light->foreground},
        {{13 * 8, 9 * 16}, "BHz:", Theme::getInstance()->fg_light->foreground},
        {{21 * 8, 9 * 16}, "Hz", Theme::getInstance()->fg_light->foreground},
    };

    OptionsField options_ctcss{
        {7 * 8, 1 * 16},
        9,
        {}};

    OptionsField options_tone_a{
        {3 * 8, 2 * 16},
        9,
        {}};

    OptionsField options_tone_b{
        {16 * 8, 2 * 16},
        9,
        {}};

    NumberField field_dur_a{
        {5 * 8, 3 * 16},
        4,
        {100, 9900},
        100,
        ' '};

    NumberField field_dur_b{
        {17 * 8, 3 * 16},
        4,
        {100, 9900},
        100,
        ' '};

    // Digit-by-digit entry for custom tone frequency (integer Hz, 4 slots, 0100–9999)
    SymField symfield_custom_a{
        {4 * 8, 9 * 16},
        4,
        SymField::Type::Dec};

    SymField symfield_custom_b{
        {17 * 8, 9 * 16},
        4,
        SymField::Type::Dec};

    NumberField field_gap{
        {5 * 8, 4 * 16},
        4,
        {0, 9900},
        50,
        ' '};

    OptionsField options_timing{
        {8 * 8, 5 * 16},
        12,
        {{"Moto Std", 0},
         {"Short Alert", 1},
         {"Fire Std", 2},
         {"Long Alert", 3},
         {"Custom", 4}}};

    NumberField field_slot{
        {7 * 8, 6 * 16},
        1,
        {1, 5},
        1,
        ' '};

    Button button_save{
        {9 * 8, 6 * 16, 6 * 8, 20},
        "Save"};

    Button button_load{
        {16 * 8, 6 * 16, 6 * 8, 20},
        "Load"};

    // Shows the name of the currently selected preset slot (up to 8 visible chars)
    Text text_slot_name{
        {22 * 8, 6 * 16, 8 * 8, 16},
        ""};

    Text text_time{
        {0, 7 * 16, 30 * 8, 16},
        ""};

    Text text_status{
        {0, 8 * 16, 30 * 8, 16},
        ""};

    ProgressBar progressbar{
        {2 * 8, 14 * 16, UI_POS_WIDTH_REMAINING(4), 16}};

    TransmitterView tx_view{
        (int16_t)UI_POS_Y_BOTTOM(4),
        10000,
        9};

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto msg = *reinterpret_cast<const TXProgressMessage*>(p);
            this->on_tx_progress(msg.progress, msg.done);
        }};
};

}  // namespace ui::external_app::two_tone_pager

#endif /* __UI_TWO_TONE_PAGER_H__ */
