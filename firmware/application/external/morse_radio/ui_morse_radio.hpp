/*
 * Copyright (C) 2026 Pezsma
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

#ifndef __MORSE_RADIO_H__
#define __MORSE_RADIO_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "string_format.hpp"
#include "external_app.hpp"
#include "ui_freq_field.hpp"
#include "radio_state.hpp"
#include "portapack.hpp"
#include "message.hpp"
#include "audio.hpp"
#include "baseband_api.hpp"
#include "receiver_model.hpp"
#include "tone_key.hpp"
#include "log_file.hpp"
#include "file_path.hpp"
#include "file.hpp"
#include "rtc_time.hpp"
#include "morsedecoder.hpp"

#include <cstdint>

namespace ui::external_app::morse_radio {

class MorseRadioView;

class MorseLogger {
   public:
    Optional<File::Error> append(const std::filesystem::path& filename) {
        return log_file.append(filename);
    }

    void init_daily_log(const std::filesystem::path& log_dir);
    bool on_packet(const std::string& content, bool time, const std::string& morse_mode);
    void radio_set_log(const std::string& morse_mode);

   private:
    LogFile log_file{};
    uint8_t char_count{0};  // log file line break
};

class MorseRadioView : public ui::View {
   public:
    MorseRadioView(ui::NavigationView& nav);
    ~MorseRadioView();
    std::string title() const override {
        return "Morse";
    }
    void focus() override;

   private:
    void writeCharToConsole(const std::string& ch, double confidence);
    void on_data(const MorseRXDataMessage* message);
    void on_freq(const MorseRXfreqMessage* message);
    void on_message(const Message* const p);
    void check_for_timeout();
    int32_t ProcessSignal(int32_t sig_time_us);
    ui::NavigationView& nav_;
    MorseDecoder morse_decoder_{};
    RxRadioState radio_state_{};
    std::unique_ptr<MorseLogger> logger{};

    enum morse_modes : uint8_t {
        MORSE_AM_CW = 0,
        MORSE_NFM,
        MORSE_AM_DSB,
        MORSE_AM_USB,
        MORSE_AM_LSB,
    };
    uint8_t saved_mode = MORSE_AM_CW;

    app_settings::SettingsManager settings_{
        "rx_morse_radio",
        app_settings::Mode::RX,
        {
            {"cwmode"sv, &saved_mode},
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

    NumberField field_squelch{
        {UI_POS_X(10), UI_POS_Y(1)},
        2,
        {0, 99},
        1,
        ' ',
    };
    ui::Text txt_speed{{UI_POS_X(23), UI_POS_Y(1), UI_POS_WIDTH(3), UI_POS_HEIGHT(1)}, "??"};
    ui::Text txt_last{{UI_POS_X(12), UI_POS_Y(4), UI_POS_WIDTH_REMAINING(12), UI_POS_HEIGHT(1)}, ""};
    ui::Text txt_freq{{UI_POS_X(21), UI_POS_Y(2), UI_POS_WIDTH(5), UI_POS_HEIGHT(1)}, "??"};
    ui::Text txt_clip{{UI_POS_X(15), UI_POS_Y(3), UI_POS_WIDTH(5), UI_POS_HEIGHT(1)}, "clipping"};
    ui::Console console_text{{UI_POS_X(0), UI_POS_Y(5), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(7)}};
    ui::Labels labels{
        {{UI_POS_X(0), UI_POS_Y(1)}, "Squelch:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(15), UI_POS_Y(1)}, "Speed:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(26), UI_POS_Y(1)}, "wpm", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(4)}, "Last seq.:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(15), UI_POS_Y(2)}, "Tone:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(27), UI_POS_Y(2)}, "Hz", Theme::getInstance()->fg_light->foreground}};

    ui::OptionsField options_mode{
        {UI_POS_X(8), UI_POS_Y(2) + 4},  // +4 to align with 'Log' checkbox text
        6,
        {
            {"AM/CW", MORSE_AM_CW},
            {"NFM", MORSE_NFM},
            {"AM/DSB", MORSE_AM_DSB},
            {"AM/USB", MORSE_AM_USB},
            {"AM/LSB", MORSE_AM_LSB},
        }};

    Checkbox chk_log{{UI_POS_X(0), UI_POS_Y(2)}, 12, "Log", false};
    ui::Button btn_clear{{UI_POS_X(0), UI_POS_Y_BOTTOM(2), UI_POS_WIDTH(6), UI_POS_HEIGHT(1)}, "CLR"};

    std::string arr_color[4] = {STR_COLOR_WHITE, STR_COLOR_RED, STR_COLOR_YELLOW, STR_COLOR_GREEN};

    bool long_pause_sent_{false};
    bool save_log{false};
    bool reset_triggered_{false};
    bool time_stamp{false};
    uint8_t last_color_id{255};
    uint8_t color_id{255};
    int8_t last_sign_{0};
    uint8_t space_timer{0};
    int32_t accumulator_us_{0};
    uint64_t last_activity_time{0};

    MessageHandlerRegistration message_handler_packet{
        Message::ID::MorseRXData,
        [this](Message* const p) {
            const auto message = static_cast<const MorseRXDataMessage*>(p);
            this->on_data(message);
        }};

    MessageHandlerRegistration message_handler_freq{
        Message::ID::MorseRXfreq,
        [this](Message* const p) {
            const auto message = static_cast<const MorseRXfreqMessage*>(p);
            this->on_freq(message);
        }};

    MessageHandlerRegistration message_handler_framesync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const p) {
            (void)p;
            this->check_for_timeout();
        }};
};

}  // namespace ui::external_app::morse_radio

#endif  // __MORSE_RADIO_H__
