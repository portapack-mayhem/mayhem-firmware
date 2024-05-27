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

#ifndef __CAPTURE_APP_HPP__
#define __CAPTURE_APP_HPP__

#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "ui_record_view.hpp"
#include "ui_spectrum.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "file_path.hpp"

namespace ui {

class CaptureAppView : public View {
   public:
    CaptureAppView(NavigationView& nav);
    CaptureAppView(NavigationView& nav, ReceiverModel::settings_t override);
    ~CaptureAppView();

    void focus() override;
    void set_parent_rect(const Rect new_parent_rect) override;

    std::string title() const override { return "Capture"; };

   private:
    static constexpr ui::Dim header_height = 3 * 16;
    uint32_t previous_bandwidth{500000};

    NavigationView& nav_;
    RxRadioState radio_state_{ReceiverModel::Mode::Capture};
    app_settings::SettingsManager settings_{
        "rx_capture", app_settings::Mode::RX};

    Labels labels{
        {{0 * 8, 1 * 16}, "Rate:", Theme::getInstance()->fg_light->foreground},
        {{11 * 8, 1 * 16}, "Format:", Theme::getInstance()->fg_light->foreground},
    };

    RSSI rssi{
        {24 * 8, 0, 6 * 8, 4}};

    Channel channel{
        {24 * 8, 5, 6 * 8, 4}};

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
        {5 * 8, 1 * 16},
        5,
        {}};

    OptionsField option_format{
        {18 * 8, 1 * 16},
        3,
        {{"C16", RecordView::FileType::RawS16},
         {"C8", RecordView::FileType::RawS8}}};

    Checkbox check_trim{
        {23 * 8, 1 * 16},
        4,
        "Trim",
        /*small*/ true};

    RecordView record_view{
        {0 * 8, 2 * 16, 30 * 8, 1 * 16},
        u"BBD_????.*",
        captures_dir,
        RecordView::FileType::RawS16,
        16384,
        3};

    spectrum::WaterfallView waterfall{};
};

} /* namespace ui */

#endif /*__CAPTURE_APP_HPP__*/
