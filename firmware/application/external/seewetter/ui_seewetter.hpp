/*
 * Copyright (C) 2026 justin080
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

#ifndef __UI_SEEWETTER_H__
#define __UI_SEEWETTER_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "ui_rssi.hpp"
#include "ui_widget.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "receiver_model.hpp"
#include "message.hpp"
#include "log_file.hpp"
#include "seewetter_protocol.hpp"
#include "seewetter_baudot.hpp"

namespace ui::external_app::seewetter {

/* Setting names as named constants: string literals used in a constant
 * expression ("..."sv) land in a shared .rodata.str1.1 section, which the
 * external app linker rule does not catch, so they would end up in the main
 * firmware. Named objects get their own (app) section. */
inline constexpr char setting_baud[] = "baud";
inline constexpr char setting_shift[] = "shift";
inline constexpr char setting_polarity[] = "polarity";
inline constexpr char setting_afc[] = "afc";
inline constexpr char setting_squelch[] = "squelch";
inline constexpr char setting_usos[] = "usos";
inline constexpr char setting_log[] = "log";
inline constexpr char settings_name[] = "rx_seewetter";

/* Small tuning display: +-1250 Hz around the receive frequency, with the two
 * tone positions the decoder listens on. */
class SeewetterSpectrum : public Widget {
   public:
    SeewetterSpectrum(Rect parent_rect);

    void update(const uint8_t* bins, int16_t centre_hz, uint16_t shift_hz, bool locked);
    void paint(Painter& painter) override;

   private:
    uint8_t bins_[::seewetter::SPECTRUM_BINS]{};
    int16_t centre_hz_{0};
    uint16_t shift_hz_{450};
    bool locked_{false};

    int hz_to_x(int32_t hz, int x0, int bin_w) const;
};

class SeewetterView : public View {
   public:
    SeewetterView(NavigationView& nav);
    ~SeewetterView();

    SeewetterView(const SeewetterView&) = delete;
    SeewetterView& operator=(const SeewetterView&) = delete;

    void focus() override;
    std::string title() const override { return "Seewetter"; }

   private:
    static constexpr uint32_t sampling_rate = 3072000;
    static constexpr uint32_t baseband_bandwidth = 1750000;

    NavigationView& nav_;

    RxRadioState radio_state_{
        4583000 /* DWD Pinneberg DDK2, centre frequency */,
        baseband_bandwidth,
        sampling_rate};

    uint32_t baud_ = 5000;
    uint32_t shift_ = 450;
    uint32_t polarity_ = 0;
    uint32_t afc_ = 1;
    uint32_t squelch_ = 2;
    uint32_t usos_ = 0;
    uint32_t log_ = 0;

    app_settings::SettingsManager settings_{
        settings_name,
        app_settings::Mode::RX,
        {
            {setting_baud, &baud_},
            {setting_shift, &shift_},
            {setting_polarity, &polarity_},
            {setting_afc, &afc_},
            {setting_squelch, &squelch_},
            {setting_usos, &usos_},
            {setting_log, &log_},
        }};

    RxFrequencyField field_frequency{
        {UI_POS_X(0), UI_POS_Y(0)},
        nav_};
    RFAmpField field_rf_amp{
        {UI_POS_X(13), UI_POS_Y(0)}};
    LNAGainField field_lna{
        {UI_POS_X(15), UI_POS_Y(0)}};
    VGAGainField field_vga{
        {UI_POS_X(18), UI_POS_Y(0)}};
    RSSI rssi{
        {UI_POS_X(21), UI_POS_Y(0), UI_POS_WIDTH_REMAINING(24), 4}};
    AudioVolumeField field_volume{
        {UI_POS_X_RIGHT(2), UI_POS_Y(0)}};

    Labels labels{
        {{UI_POS_X(14), UI_POS_Y(1)}, "Bd", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(23), UI_POS_Y(1)}, "Sh", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(2)}, "Pol", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(9), UI_POS_Y(2)}, "AFC", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(17), UI_POS_Y(2)}, "Sq", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(3)}, "USOS", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(9), UI_POS_Y(3)}, "Log", Theme::getInstance()->fg_light->foreground},
    };

    /* DWD Pinneberg RTTY, published centre frequencies (F1B, 50 Bd, +-225 Hz).
     * The first entry must mean "no preset": set_by_value falls back to it. */
    OptionsField options_station{
        {UI_POS_X(0), UI_POS_Y(1)},
        13,
        {{"Manuell", 0},
         {"DDK2  4583.0", 4583000},
         {"DDH7  7646.0", 7646000},
         {"DDK9 10100.8", 10100800},
         {"DDH9 11039.0", 11039000},
         {"DDH8 14467.3", 14467300}}};

    OptionsField options_baud{
        {UI_POS_X(17), UI_POS_Y(1)},
        5,
        {{"45.45", 4545},
         {"50", 5000},
         {"75", 7500},
         {"100", 10000}}};

    OptionsField options_shift{
        {UI_POS_X(26), UI_POS_Y(1)},
        3,
        {{"85", 85},
         {"170", 170},
         {"425", 425},
         {"450", 450},
         {"850", 850}}};

    OptionsField options_polarity{
        {UI_POS_X(4), UI_POS_Y(2)},
        4,
        {{"Auto", 0},
         {"Norm", 1},
         {"Inv", 2}}};

    OptionsField options_afc{
        {UI_POS_X(13), UI_POS_Y(2)},
        3,
        {{"Aus", 0},
         {"Ein", 1}}};

    OptionsField options_squelch{
        {UI_POS_X(20), UI_POS_Y(2)},
        6,
        {{"Aus", 0},
         {"Leicht", 1},
         {"Mittel", 2},
         {"Stark", 3}}};

    OptionsField options_usos{
        {UI_POS_X(5), UI_POS_Y(3)},
        3,
        {{"Aus", 0},
         {"Ein", 1}}};

    OptionsField options_log{
        {UI_POS_X(13), UI_POS_Y(3)},
        3,
        {{"Aus", 0},
         {"Ein", 1}}};

    Button button_clear{
        {UI_POS_X(19), UI_POS_Y(3), UI_POS_WIDTH(10), UI_POS_HEIGHT(1)},
        "Leeren"};

    SeewetterSpectrum spectrum{
        {UI_POS_X(0), UI_POS_Y(4), UI_POS_MAXWIDTH, UI_POS_HEIGHT(2)}};

    Text text_status{
        {UI_POS_X(0), UI_POS_Y(6), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)},
        "Suche Signal..."};

    /* View y = 0 is screen y = 16 (status bar), so the console must end at
     * screen_height - 16 (see the note in the FT8 app). */
    Console console{
        {UI_POS_X(0), UI_POS_Y(7), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(8)}};

    Ita2Decoder decoder{};
    LogFile log_file{};
    bool log_file_ready{false};
    bool log_open{false};
    std::string log_line{};

    void send_config();
    void set_logging(bool enable);
    void on_message(const RTTYDataMessage* message);
    void on_text(const RTTYDataMessage* message);
    void on_status(const RTTYDataMessage* message);

    MessageHandlerRegistration message_handler{
        Message::ID::RTTYData,
        [this](const Message* const p) {
            this->on_message(static_cast<const RTTYDataMessage*>(p));
        }};
};

}  // namespace ui::external_app::seewetter

#endif /*__UI_SEEWETTER_H__*/
