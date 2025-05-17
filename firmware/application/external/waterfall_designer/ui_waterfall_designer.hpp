/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
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

#ifndef __WATERFALL_DESIGNER_APP_HPP__
#define __WATERFALL_DESIGNER_APP_HPP__

#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "ui_record_view.hpp"
#include "ui_spectrum.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "file_path.hpp"

namespace ui::external_app::waterfall_designer {

enum class ColorComponent {
    RED,
    GREEN,
    BLUE
};

class WaterfallDesignerColorPickerView : public View {
   public:
    std::function<void(std::string)> on_save{};

    WaterfallDesignerColorPickerView(NavigationView& nav, std::string color_str);

    std::string title() const override { return "Color Picker"; };
    void focus() override;
    void paint(Painter& painter) override;

   private:
    NavigationView& nav_;
    std::string color_str_;
    uint8_t index_{0};
    uint8_t red_{0};
    uint8_t green_{0};
    uint8_t blue_{0};

    void update_color_index();
    std::string build_color_str();

    Labels labels{
        {{0 * 8, 0 * 16}, "Index", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 2 * 16}, "Red", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 4 * 16}, "Green", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 6 * 16}, "Blue", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 8 * 16}, "Step", Theme::getInstance()->fg_light->foreground}};

    NumberField field_index{
        {0 * 8, 1 * 16},
        3,
        {0, 255},
        1,
        ' '};

    NumberField field_red{
        {0 * 8, 3 * 16},
        3,
        {0, 255},
        1,
        ' '};

    NumberField field_green{
        {0 * 8, 5 * 16},
        3,
        {0, 255},
        1,
        ' '};

    NumberField field_blue{
        {0 * 8, 7 * 16},
        3,
        {0, 255},
        1,
        ' '};

    NumberField field_step{
        {0 * 8, 9 * 16},
        3,
        {0, 255},
        1,
        ' '};

    ProgressBar progressbar{
        {0 * 8,
         screen_height - 4 * 16 - 2 * 16 - 1 * 16,
         screen_width,
         2 * 16}};

    Button button_save{
        {0, screen_height - 4 * 16, screen_width, 4 * 16},
        "Save"};
};

class WaterfallDesignerView : public View {
   public:
    WaterfallDesignerView(NavigationView& nav);
    WaterfallDesignerView(NavigationView& nav, ReceiverModel::settings_t override);
    ~WaterfallDesignerView();

    void focus() override;
    void set_parent_rect(const Rect new_parent_rect) override;

    std::string title() const override { return "Wtf Design"; };

   private:
    uint32_t capture_rate{500000};
    uint32_t file_format{0};
    uint32_t highlighted_index_{0};
    bool trim{false};
    std::filesystem::path current_profile_path = "";
    NavigationView& nav_;
    RxRadioState radio_state_{ReceiverModel::Mode::Capture};
    app_settings::SettingsManager settings_{
        "rx_wtf_designer",
        app_settings::Mode::RX,
        {
            {"capture_rate"sv, &capture_rate},
            {"file_format"sv, &file_format},
            {"trim"sv, &trim},
        }};

    Labels labels{
        {{0 * 8, 1 * 16}, "Rate:", Theme::getInstance()->fg_light->foreground},
        {{11 * 8, 1 * 16}, "Format:", Theme::getInstance()->fg_light->foreground},
    };

    RxFrequencyField field_frequency{
        {0 * 8, 0 * 16},
        nav_};

    FrequencyStepView field_frequency_step{
        {10 * 8, 0 * 16}};

    RFAmpField field_rf_amp{
        {16 * 8, 0 * 16}};

    LNAGainField field_lna{
        {18 * 8, 0 * 16}};

    VGAGainField field_vga{
        {21 * 8, 0 * 16}};

    OptionsField option_bandwidth{
        {24 * 8, 0 * 16},
        5,
        {}};

    MenuView menu_view{};

    NewButton button_new{
        {0 * 8, 8 * 16, 4 * 8, 32},
        {},
        &bitmap_icon_file,
        Theme::getInstance()->fg_blue->foreground};

    NewButton button_open{
        {4 * 8, 8 * 16, 4 * 8, 32},
        {},
        &bitmap_icon_load,
        Theme::getInstance()->fg_blue->foreground};

    NewButton button_save{
        {8 * 8, 8 * 16, 4 * 8, 32},
        {},
        &bitmap_icon_save,
        Theme::getInstance()->fg_blue->foreground};

    NewButton button_add_level{
        {12 * 8, 8 * 16, 4 * 8, 32},
        {},
        &bitmap_icon_add,
        Theme::getInstance()->fg_blue->foreground};

    NewButton button_remove_level{
        {16 * 8, 8 * 16, 4 * 8, 32},
        {},
        &bitmap_icon_delete,
        Theme::getInstance()->fg_blue->foreground};

    NewButton button_edit_color{
        {20 * 8, 8 * 16, 4 * 8, 32},
        {},
        &bitmap_icon_notepad,
        Theme::getInstance()->fg_blue->foreground};

    NewButton button_apply_setting{
        {24 * 8, 8 * 16, 4 * 8, 32},
        {},
        &bitmap_icon_replay,
        Theme::getInstance()->fg_blue->foreground};

    void backup_current_profile();
    void restore_current_profile();
    void on_create_new_profile();
    void on_open_profile();
    void on_profile_changed(std::filesystem::path new_profile_path);
    void on_save_profile();
    void on_add_level();
    void on_remove_level();
    void on_edit_color();

    void refresh_menu_view();

    void on_apply_current_to_wtf();  // will restore if didn't apple, when distruct
    void on_apply_setting();         // apply set

    bool if_apply_setting{false};
    /*NB:
    this works as:
    each time you change color, it apply as file realtime
    however if you don't push the apply (play) btn, it would resotore in distructor,
    if you push apply, it would apply and exit*/

    std::vector<std::string> profile_levels{};

    static constexpr ui::Dim header_height = 10 * 16;

    RecordView record_view{// we still need it cuz it make waterfall correct
                           {screen_width, screen_height, 30 * 8, 1 * 16},
                           u"BBD_????.*",
                           captures_dir,
                           RecordView::FileType::RawS16,
                           16384,
                           3};

    std::unique_ptr<spectrum::WaterfallView> waterfall{};
    std::string file_name_buffer{};  // needed by text_prompy

    MessageHandlerRegistration message_handler_freqchg{
        Message::ID::FreqChangeCommand,
        [this](Message* const p) {
            const auto message = static_cast<const FreqChangeCommandMessage*>(p);
            this->on_freqchg(message->freq);
        }};

    void on_freqchg(int64_t freq);
};

} /* namespace ui::external_app::waterfall_designer */

#endif /*__WATERFALL_DESIGNER_APP_HPP__*/
