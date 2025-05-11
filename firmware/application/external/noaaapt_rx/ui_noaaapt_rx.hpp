/*
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

#ifndef __UI_noaaapt_RX_H__
#define __UI_noaaapt_RX_H__

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
#include "ui_fileman.hpp"
#include "bmpfile.hpp"
#include "file_path.hpp"

using namespace ui;

namespace ui::external_app::noaaapt_rx {

#define NOAAAPT_PX_SIZE 2080

#define NOAAAPT_FREQ_OFFSET 0

#define NOAA_IMG_START_ROW 4

class NoaaAptRxView : public View {
   public:
    NoaaAptRxView(NavigationView& nav);
    ~NoaaAptRxView();

    void focus() override;

    std::string title() const override { return "NOAA APT"; };

   private:
    void on_settings_changed();
    void on_status(NoaaAptRxStatusDataMessage msg);
    void on_image(NoaaAptRxImageDataMessage msg);

    bool stopping = false;

    uint16_t line_num = 0;      // nth line
    uint16_t line_in_part = 0;  // got multiple parts of a line, so keep track of it
    uint8_t delayer = 0;
    ui::Color line_buffer[240];
    std::filesystem::path filetohandle = "";

    bool paused = false;  // when freq field is shown for example, we need to pause

    BMPFile bmp{};

    NavigationView& nav_;
    RxRadioState radio_state_{};
    app_settings::SettingsManager settings_{
        "rx_noaaapt",
        app_settings::Mode::RX,
        {}};

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

    RecordView record_view{
        {0 * 8, 2 * 16, 30 * 8, 1 * 16},
        u"AUD",
        u"AUDIO",
        RecordView::FileType::WAV,
        4096,
        4};

    Checkbox check_wav{
        {0 * 8, 2 * 16},
        12,
        "Save WAV too",
        true};

    /*Labels labels{
        {{1 * 8, 1 * 16}, "LPM:", Theme::getInstance()->fg_light->foreground},
        {{13 * 8, 1 * 16}, "IOC:", Theme::getInstance()->fg_light->foreground},
    };*/

    Text txt_status{
        {0 * 8, 1 * 16, 20 * 8, 16},
    };

    Button button_ss{
        {190, 1 * 16, 5 * 8, 16},
        LanguageHelper::currentMessages[LANG_START]};

    MessageHandlerRegistration message_handler_stats{
        Message::ID::NoaaAptRxStatusData,
        [this](const Message* const p) {
            if (stopping || paused) return;
            const auto message = *reinterpret_cast<const NoaaAptRxStatusDataMessage*>(p);
            on_status(message);
        }};

    MessageHandlerRegistration message_handler_image{
        Message::ID::NoaaAptRxImageData,
        [this](const Message* const p) {
            if (stopping || paused) return;
            const auto message = *reinterpret_cast<const NoaaAptRxImageDataMessage*>(p);
            on_image(message);
        }};
};

}  // namespace ui::external_app::noaaapt_rx

#endif /*__UI_noaaapt_RX_H__*/
