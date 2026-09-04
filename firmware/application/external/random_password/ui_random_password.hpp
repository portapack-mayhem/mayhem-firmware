/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
 * Copyright (C) 2024 zxkmm
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

#ifndef __UI_RANDOM_PASSWORD_H__
#define __UI_RANDOM_PASSWORD_H__

#define MAX_DIGITS 30

#include "ui.hpp"
#include "ui_language.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "ui_record_view.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "log_file.hpp"
#include "utility.hpp"
#include "ui_qrcode.hpp"
#include "usb_serial_asyncmsg.hpp"
#include "sha256.h"

#include <array>
#include <cstdint>
#include <string>

using namespace ui;

namespace ui::external_app::random_password {

class RandomPasswordLogger {
   public:
    Optional<File::Error> append(const std::filesystem::path& filename) {
        return log_file.append(filename);
    }

    void log_raw_data(const std::string& data);

   private:
    LogFile log_file{};
};

class RandomPasswordView : public View {
   public:
    RandomPasswordView(NavigationView& nav);
    ~RandomPasswordView();

    void focus() override;

    std::string title() const override { return "R.passwd"; };

   private:
    /* ---- pipeline constants; see the header comment in the .cpp ---- */

    /* Min-entropy that must be credited before a password may be generated.
     * Fixed, not a function of length: the most this app can emit is
     * 30 * log2(67) = 182 bits, so 256 always covers the output with margin,
     * and a knob here would only invite explaining. */
    static constexpr uint32_t required_mbits = 256 * 1000;

    static constexpr uint32_t harvest_sampling_rate = 2000000;
    /* Widest the MAX283x offers. Deliberate: noise bandwidth far wider than
     * the sample rate means consecutive samples see fresh noise instead of a
     * filtered, correlated process. */
    static constexpr uint32_t harvest_bandwidth = 28000000;
    static constexpr uint32_t harvest_buffer_decimation = 16;
    static constexpr uint32_t harvest_settle_buffers = 2;

    /* Consecutive failing/passing blocks needed to latch or clear the alarm.
     * Latching avoids flapping the UI on a single unlucky block. */
    static constexpr uint32_t health_latch_blocks = 8;

    /* Retune after this much fresh credit, so that no single emitter can
     * dominate a pool. */
    static constexpr uint32_t hop_interval_mbits = 64 * 1000;

    static constexpr size_t log_ring_size = 16;

    struct LoggedBlock {
        uint8_t data[EntropyBlockMessage::block_bytes];
        uint32_t freq_hz;
        uint16_t h_mbits;
        uint8_t flags;
    };

    /* ---- entropy pool state ---- */

    /* The pool is a running hash context, not a buffer. Every block ever
     * received is absorbed into it and nothing is ever dropped, which is what
     * makes "every collected byte influences every output character" true. */
    SHA256 pool_{};
    uint32_t pool_credit_mbits_{0};
    uint32_t mbits_since_hop_{0};

    bool source_failed_{false};
    uint32_t consecutive_bad_{0};
    uint32_t consecutive_good_{0};
    uint16_t last_h_mbits_{0};
    uint8_t last_flags_{0};
    uint32_t ui_throttle_{0};

    /* Non-secret state used only to choose hop frequencies. Frequency choice
     * is not an entropy source and is not claimed to be one. */
    uint8_t hop_state_[SHA256::digest_size]{};

    std::array<LoggedBlock, log_ring_size> log_ring_{};
    size_t log_ring_count_{0};

    std::string password{};
    bool auto_generated_{false};
    bool flooding{false};
    bool logging{false};
    bool async_prev_val{false};

    void on_entropy_block(const EntropyBlockMessage& message);
    void absorb_block(const EntropyBlockMessage& message);
    void update_status_text();
    void new_password();
    void reset_pool(const uint8_t* carry);
    bool pool_ready() const;

    std::string build_charset() const;
    uint32_t hop_random();
    void hop_frequency();
    void retune(uint32_t freq_hz);
    void on_frequency_changed();

    std::string generate_log_line() const;
    void paint_password_hints();

    static bool islower(char c);
    static bool isupper(char c);
    static bool isdigit(char c);

    NavigationView& nav_;
    RxRadioState radio_state_{
        0,
        harvest_bandwidth,
        harvest_sampling_rate,
        ReceiverModel::Mode::SpectrumAnalysis};
    app_settings::SettingsManager settings_{
        "rx_passgen", app_settings::Mode::RX};

    Labels labels{
        {{UI_POS_X_CENTER(30), UI_POS_Y(0)}, "---------entropy pool---------", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 7 * 16 - 2}, "digits:", Theme::getInstance()->fg_light->foreground},
    };

    RFAmpField field_rf_amp{
        {13 * 8, 1 * 16}};
    LNAGainField field_lna{
        {15 * 8, 1 * 16}};
    VGAGainField field_vga{
        {18 * 8, 1 * 16}};

    RSSI rssi{
        {UI_POS_X(21), 1 * 16, UI_POS_WIDTH_REMAINING(21), 8}};

    RxFrequencyField field_frequency{
        {UI_POS_X(0), 1 * 16},
        nav_};

    Text text_pool{
        {0, 2 * 16, 13 * 8, 16},
        "0/256 bit"};

    ProgressBar progressbar{
        {13 * 8 + 2, 2 * 16, screen_width - 13 * 8 - 4, 16}};

    Text text_health{
        {0, 3 * 16, screen_width, 16},
        "waiting for radio"};

    Text text_generated_passwd{
        {0, 4 * 16, screen_width, 16},
        ""};

    Text text_char_type_hints{
        {0, 5 * 16 + 4, screen_width, 16},
        ""};

    NumberField field_digits{
        {0 + (sizeof("digits:") - 1) * 8, 7 * 16 - 2},
        2,
        {1, MAX_DIGITS},
        1,
        ' '};

    Checkbox check_auto_send{
        {1 * 8, 8 * 16},
        20,
        "Auto send"};

    Checkbox check_hop{
        {17 * 8, 8 * 16},
        6,
        "Hop freq"};

    Checkbox check_allow_confusable_chars{
        {1 * 8, 10 * 16},
        20,
        "0 O o 1 l"};

    Checkbox check_log{
        {17 * 8, 10 * 16},
        3,
        LanguageHelper::currentMessages[LANG_SAVE]};

    Checkbox check_digits{
        {1 * 8, 12 * 16},
        3,
        "123"};

    Checkbox check_punctuation{
        {17 * 8, 12 * 16},
        6,
        ".,-!?"};

    Checkbox check_latin_lower{
        {1 * 8, 14 * 16},
        3,
        "abc"};

    Checkbox check_latin_upper{
        {17 * 8, 14 * 16},
        3,
        "ABC"};

    Button button_flood{
        {UI_POS_X(0), 15 * 16 + 18, screen_width / 2, 22},
        LanguageHelper::currentMessages[LANG_FLOOD]};

    Button button_send{
        {screen_width / 2 + 2, 15 * 16 + 18, screen_width / 2 - 2, 22},
        "Send pwd"};

    Button button_refresh{
        {UI_POS_X(0), 17 * 16 + 10, screen_width / 2, 22},
        "Generate"};

    Button button_show_qr{
        {screen_width / 2 + 2, 17 * 16 + 10, screen_width / 2 - 2, 22},
        LanguageHelper::currentMessages[LANG_SHOWQR]};

    std::unique_ptr<RandomPasswordLogger> logger{};

    MessageHandlerRegistration message_handler_entropy{
        Message::ID::EntropyBlock,
        [this](Message* const p) {
            this->on_entropy_block(*static_cast<const EntropyBlockMessage*>(p));
        }};

    MessageHandlerRegistration message_handler_freqchg{
        Message::ID::FreqChangeCommand,
        [this](Message* const p) {
            const auto message = static_cast<const FreqChangeCommandMessage*>(p);
            this->field_frequency.set_value(message->freq);
        }};
};

}  // namespace ui::external_app::random_password

#endif /*__UI_RANDOM_PASSWORD_H__*/
