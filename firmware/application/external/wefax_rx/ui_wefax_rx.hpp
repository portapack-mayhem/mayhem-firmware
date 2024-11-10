/*
 * Copyright (C) 2024 HTotoo
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

#ifndef __UI_WEFAX_RX_H__
#define __UI_WEFAX_RX_H__

#include "ui.hpp"
#include "ui_language.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_geomap.hpp"
#include "ui_freq_field.hpp"
#include "ui_record_view.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "log_file.hpp"
#include "utility.hpp"

using namespace ui;

namespace ui::external_app::wefax_rx {

class WeFaxRxView : public View {
   public:
    WeFaxRxView(NavigationView& nav);
    ~WeFaxRxView();

    void focus() override;

    std::string title() const override { return "WeFax"; };

   private:
    void on_settings_changed();
    void on_status(WeFaxRxStatusDataMessage msg);
    void on_image(WeFaxRxImageDataMessage msg);

    uint8_t ioc_index{0};
    uint8_t lpm_index{0};
    uint16_t line_num = 0;
    uint8_t txtDec = 0;

    NavigationView& nav_;
    RxRadioState radio_state_{};
    app_settings::SettingsManager settings_{
        "rx_wefax",
        app_settings::Mode::RX,
        {
            {"ioc_index"sv, &ioc_index},
            {"lpm_index"sv, &lpm_index},
        }};

    RFAmpField field_rf_amp{
        {13 * 8, 0 * 16}};
    LNAGainField field_lna{
        {15 * 8, 0 * 16}};
    VGAGainField field_vga{
        {18 * 8, 0 * 16}};
    RSSI rssi{
        {21 * 8, 0, 6 * 8, 4}};
    AudioVolumeField field_volume{
        {28 * 8, 0 * 16}};

    RxFrequencyField field_frequency{
        {0 * 8, 0 * 16},
        nav_};

    Labels labels{
        {{1 * 8, 1 * 16}, "LPM:", Theme::getInstance()->fg_light->foreground},
        {{13 * 8, 1 * 16}, "IOC:", Theme::getInstance()->fg_light->foreground},
    };

    OptionsField options_lpm{
        {6 * 8, 1 * 16},
        4,
        {
            {"60", 60},
            {"90", 90},
            {"100", 100},
            {"120", 120},
            {"180", 180},
            {"240", 240},
        }};

    OptionsField options_ioc{
        {18 * 8, 1 * 16},
        4,
        {
            {"576", 0},
            {"228", 1},
        }};

    Text txt_status{
        {0 * 8, 2 * 16, 30 * 8, 16},
    };

    MessageHandlerRegistration message_handler_stats{
        Message::ID::WeFaxRxStatusData,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const WeFaxRxStatusDataMessage*>(p);
            on_status(message);
        }};

    MessageHandlerRegistration message_handler_image{
        Message::ID::WeFaxRxImageData,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const WeFaxRxImageDataMessage*>(p);
            on_image(message);
        }};
};

}  // namespace ui::external_app::wefax_rx

#endif /*__UI_WEFAX_RX_H__*/
