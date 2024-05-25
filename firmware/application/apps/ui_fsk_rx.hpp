/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
 * Copyright (C) 2023 gullradriel, Nilorea Studio Inc.
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

#ifndef __UI_FSK_RX_H__
#define __UI_FSK_RX_H__

#include "ui_widget.hpp"
#include "ui_freq_field.hpp"
#include "ui_receiver.hpp"
#include "ui_record_view.hpp"
#include "ui_rssi.hpp"
#include "ui_spectrum.hpp"
#include "ui_tabview.hpp"

#include "app_settings.hpp"
#include "log_file.hpp"
#include "radio_state.hpp"
#include "pocsag_app.hpp"

#include <functional>

class FskRxLogger {
   public:
    Optional<File::Error> append(const std::filesystem::path& filename) {
        return log_file.append(filename);
    }

    void log_raw_data(const std::string& data, const uint32_t frequency);
    void log_decoded(Timestamp timestamp, const std::string& text);

   private:
    LogFile log_file{};
};

namespace ui {
class FskRxAppConsoleView : public View {
   public:
    FskRxAppConsoleView(NavigationView& nav, Rect parent_rec);
    ~FskRxAppConsoleView();

    std::string title() const override { return "FSK RX Data"; };

    void on_packet(uint32_t value, bool is_data);

    void on_show() override;
    void on_hide() override;

   private:
    NavigationView& nav_;

    Console console{
        {0, 0, 240, 224}};
};

class FskRxAppView : public View {
   public:
    FskRxAppView(NavigationView& nav, Rect parent_rect);
    ~FskRxAppView();

    void focus() override;
    void on_show() override;
    void on_hide() override;

    spectrum::WaterfallView waterfall{};

    std::string title() const override { return "FSK RX Stream"; };

   private:
    NavigationView& nav_;
    RxRadioState radio_state_{};
};

class FskxRxMainView : public View {
   public:
    FskxRxMainView(NavigationView& nav);
    ~FskxRxMainView();

    void focus() override;
    void set_parent_rect(const Rect new_parent_rect) override;

    std::string title() const override { return "FSK RX"; };

   private:
    static constexpr uint32_t initial_target_frequency = 902'075'000;
    static constexpr ui::Dim header_height = (5 * 16);

    uint32_t initial_deviation{3750};

    bool logging() const { return false; };
    bool logging_raw() const { return false; };

    NavigationView& nav_;
    Rect view_rect = {0, header_height, 240, 224};

    FskRxAppView view_stream{nav_, view_rect};
    FskRxAppConsoleView view_data{nav_, view_rect};

    TabView tab_view{
        {"Data", Theme::getInstance()->fg_yellow->foreground, &view_data},
        {"Stream", Theme::getInstance()->fg_cyan->foreground, &view_stream}};

    void refresh_ui(rf::Frequency f);
    void on_packet(uint32_t value, bool is_data);
    void handle_decoded(Timestamp timestamp, const std::string& prefix);

    uint32_t last_address = 0;
    FskRxLogger logger{};
    uint16_t packet_count = 0;

    RxFrequencyField field_frequency{
        {0 * 8, 4 * 8},
        nav_};

    RFAmpField field_rf_amp{
        {11 * 8, 2 * 16}};

    LNAGainField field_lna{
        {13 * 8, 2 * 16}};

    VGAGainField field_vga{
        {16 * 8, 2 * 16}};

    RSSI rssi{
        {19 * 8 - 4, 35, 6 * 8, 4}};

    Channel channel{
        {19 * 8 - 4, 40, 6 * 8, 4}};

    Labels labels{
        {{0 * 8, 3 * 16}, "Deviation:", Theme::getInstance()->fg_light->foreground},
    };

    FrequencyField deviation_frequency{
        {10 * 8, 3 * 16},
        {3750, 500000},
    };

    // DEBUG
    RecordView record_view{
        {0 * 8, 4 * 16, 30 * 8, 1 * 16},
        u"FSKRX_????.C16",
        u"FSKRX",
        RecordView::FileType::RawS16,
        16384,
        3};

    MessageHandlerRegistration message_handler_packet{
        Message::ID::AFSKData,
        [this](Message* const p) {
            const auto message = static_cast<const AFSKDataMessage*>(p);
            this->view_data.on_packet(message->value, message->is_data);
        }};
};

} /* namespace ui */

#endif /*__POCSAG_APP_H__*/