/*
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

#ifndef __UI_SUBGHZ_CHAT_H__
#define __UI_SUBGHZ_CHAT_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "ui_rssi.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "log_file.hpp"
#include "message.hpp"
#include "chat_crypto.hpp"

#include <array>
#include <deque>
#include <string>

namespace ui::external_app::subghz_chat {

// The chat, newest line at the bottom. With the focus on it, the encoder and Up/Down scroll
// back through it; Up at the oldest line hands the focus back. The Console can't scroll back
// and loses its lines when the keyboard has been over it.
class ChatLog : public Widget {
   public:
    enum class Kind { Received,
                      Own,
                      Info };

    ChatLog(Rect parent_rect);

    void add(const Kind kind, const std::string& text);

    void paint(Painter& painter) override;
    bool on_key(const KeyEvent key) override;
    bool on_encoder(const EncoderEvent delta) override;
    void on_focus() override { set_dirty(); }
    void on_blur() override { set_dirty(); }

   private:
    struct Entry {
        Kind kind;
        std::string text;
    };
    static constexpr size_t HISTORY_BYTES = 2048;
    std::deque<Entry> entries_{};
    size_t bytes_{0};
    size_t scroll_{0};  // rows scrolled back from the newest

    size_t columns() const;
    size_t rows_of(const Entry& e) const;
    size_t visible_rows() const;
    size_t max_scroll() const;
    bool scroll_by(const int32_t rows);
};

// Options in the Config screen, kept in the app settings.
struct ChatSettings {
    std::string nickname{"Mayhem"};
    uint8_t tx_gain{10};  // dB; keep it low, these are short range bands
    bool tx_amp{false};
    bool enable_log{false};  // the chat to LOGS/SUBGHZCHAT.TXT
    bool small_font{false};
    bool announce{true};  // "Name joined chat." on start, "Name left chat." on leaving

    // Not saved: entered again each time, as on the Flipper.
    uint8_t crypto_mode{CRYPTO_OFF};
    std::string secret{};  // password, or the key in hex
    bool key_changed{false};

    static constexpr uint8_t CRYPTO_OFF = 0;
    static constexpr uint8_t CRYPTO_PASSWORD = 1;
    static constexpr uint8_t CRYPTO_HEX_KEY = 2;
};

class ChatSettingsView : public View {
   public:
    ChatSettingsView(NavigationView& nav, ChatSettings& settings);
    ~ChatSettingsView();

    std::string title() const override { return "Chat Config"; };
    void focus() override { button_save.focus(); }

   private:
    ChatSettings& settings_;
    std::string nickname_;
    std::string secret_;

    void update_key_button();

    Labels labels{
        {{UI_POS_X(2), UI_POS_Y(1)}, "Name:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(2), UI_POS_Y(3)}, "TX gain:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(2), UI_POS_Y(11)}, "Encrypt:", Theme::getInstance()->fg_light->foreground},
    };
    Button button_nickname{
        {UI_POS_X(11), UI_POS_Y(1) - 4, UI_POS_WIDTH(14), UI_POS_HEIGHT(1) + 8},
        ""};
    NumberField field_tx_gain{
        {UI_POS_X(11), UI_POS_Y(3)},
        2,
        {0, 47},
        1,
        ' '};
    Checkbox check_tx_amp{
        {UI_POS_X(15), UI_POS_Y(3) - 4},
        9,
        "TX RF amp"};
    Checkbox check_log{
        {UI_POS_X(2), UI_POS_Y(5)},
        10,
        "Enable Log"};
    Checkbox check_small_font{
        {UI_POS_X(2), UI_POS_Y(7)},
        14,
        "Use Small Font"};
    Checkbox check_announce{
        {UI_POS_X(2), UI_POS_Y(9)},
        19,
        "Announce join/leave"};
    OptionsField options_crypto{
        {UI_POS_X(11), UI_POS_Y(11)},
        8,
        {{"Off", ChatSettings::CRYPTO_OFF},
         {"Password", ChatSettings::CRYPTO_PASSWORD},
         {"Hex key", ChatSettings::CRYPTO_HEX_KEY}}};
    Button button_key{
        {UI_POS_X(2), UI_POS_Y(13) - 4, UI_POS_WIDTH(23), UI_POS_HEIGHT(1) + 8},
        ""};
    Button button_save{
        {UI_POS_X_CENTER(10), UI_POS_Y_BOTTOM(4), UI_POS_WIDTH(10), UI_POS_HEIGHT(2)},
        "Save"};
};

// Chat on Sub-GHz in the Flipper Zero's format (CC1101 GFSK 9.99 kbaud, packets of 60 bytes):
// receive all the time, transmit a typed line, back to receiving.
class SubGhzChatView : public View {
   public:
    SubGhzChatView(NavigationView& nav);
    ~SubGhzChatView();

    void focus() override;

    std::string title() const override { return "SubGhz Chat"; };

   private:
    NavigationView& nav_;
    RxRadioState radio_state_{433920000 /* frequency */, 1750000 /* bandwidth */, 3072000 /* sampling rate */};
    ChatSettings chat_settings_{};
    app_settings::SettingsManager settings_{
        "rx_subghz_chat",
        app_settings::Mode::RX,
        {
            {"nickname"sv, &chat_settings_.nickname},
            {"tx_gain"sv, &chat_settings_.tx_gain},
            {"tx_amp"sv, &chat_settings_.tx_amp},
            {"log"sv, &chat_settings_.enable_log},
            {"small_font"sv, &chat_settings_.small_font},
            {"announce"sv, &chat_settings_.announce},
        }};

    LogFile log_file_{};
    std::string draft_{};
    std::string received_{};  // packets of the message coming in
    systime_t last_packet_{0};
    bool transmitting_{false};
    bool joined_{false};
    systime_t opened_{0};
    chat_crypto::ChatCipher cipher_{};
    systime_t tx_started_{0};
    systime_t tx_timeout_{0};  // when no TX done has come by then, back to receiving anyway
    std::array<uint8_t, 512> tx_bits_{};

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
        {UI_POS_X(21), 0, UI_POS_WIDTH_REMAINING(25), 4}};
    Channel channel{
        {UI_POS_X(21), 5, UI_POS_WIDTH_REMAINING(25), 4}};
    // What the radio is doing right now, as in Meshtastic: " RX " or " TX ".
    Text text_rxtx{
        {UI_POS_X_RIGHT(4), UI_POS_Y(0), UI_POS_WIDTH(4), UI_POS_HEIGHT(1)},
        " RX "};

    Button button_send{
        {UI_POS_X(0), UI_POS_Y(1), UI_POS_WIDTH(6), UI_POS_HEIGHT(1)},
        "Send"};
    Text text_status{
        {UI_POS_X(7), UI_POS_Y(1), UI_POS_WIDTH_REMAINING(16), UI_POS_HEIGHT(1)},
        ""};
    Button button_config{
        {UI_POS_X_RIGHT(8), UI_POS_Y(1), UI_POS_WIDTH(8), UI_POS_HEIGHT(1)},
        "Config"};

    ChatLog chat_log{
        {UI_POS_X(0), UI_POS_Y(2), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(3)}};

    void refresh_ui();
    void apply_key();
    void set_rxtx(const bool tx);
    void send(const std::string& text);
    void announce(const char* what);
    size_t build_packets(const std::string& line);
    std::string noise() const;
    void start_rx();
    void start_tx(const size_t bits);
    void on_packet(const SubGhzChatPacketMessage& message);
    void on_frame_sync();
    void flush_received();
    void show(const ChatLog::Kind kind, const std::string& line);

    MessageHandlerRegistration message_handler_packet{
        Message::ID::SubGhzChatPacket,
        [this](Message* const p) {
            this->on_packet(*static_cast<const SubGhzChatPacketMessage*>(p));
        }};
    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](Message* const p) {
            if (static_cast<const TXProgressMessage*>(p)->done)
                this->start_rx();
        }};
    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->on_frame_sync();
        }};
};

}  // namespace ui::external_app::subghz_chat

#endif /*__UI_SUBGHZ_CHAT_H__*/
