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

#include "ui_subghz_chat.hpp"

#include "audio.hpp"
#include "baseband_api.hpp"
#include "cc1101_packet.hpp"
#include "portapack.hpp"
#include "portapack_shared_memory.hpp"
#include "rtc_time.hpp"
#include "string_format.hpp"
#include "file_path.hpp"
#include "ui_textentry.hpp"

#include <algorithm>
#include <cstring>

using namespace portapack;

namespace ui::external_app::subghz_chat {

namespace {

// The Flipper sends a message in packets of 60 bytes and takes it as whole after this much quiet.
constexpr systime_t MESSAGE_GAP = MS2ST(500);
constexpr size_t MAX_RECEIVED = 1024;
constexpr size_t MAX_TEXT = 200;
// The FSK transmitter runs at 2.28 MHz and holds each bit samples_per_bit + 1 samples:
// 227 gives 10000 baud, within the CC1101's tolerance of 9992.6.
constexpr uint32_t TX_SAMPLE_RATE = 2280000;
constexpr uint32_t TX_SAMPLES_PER_BIT = 227;
// Between the packets of one message: idle preamble, so the receiver can re-arm.
constexpr size_t GAP_BYTES = 12;
// Before the first packet: the receiver gets time to find the level and the bit clock.
constexpr size_t LEAD_IN_BYTES = 24;
// The radio settles after switching to TX before the first bit goes out.
constexpr uint32_t TX_SETTLE_MS = 20;
// On top of the time the bits take: a TX done that never comes doesn't keep the radio in TX.
constexpr systime_t TX_MARGIN = MS2ST(1000);
constexpr systime_t JOIN_DELAY = MS2ST(500);
constexpr const char* JOINED = " joined chat.";
constexpr const char* LEFT = " left chat.";

bool ends_with(const std::string& s, const char* end) {
    const size_t n = strlen(end);
    return s.size() >= n && s.compare(s.size() - n, n, end) == 0;
}

}  // namespace

ChatLog::ChatLog(Rect parent_rect)
    : Widget{parent_rect} {
    set_focusable(true);
}

void ChatLog::add(const Kind kind, const std::string& text) {
    entries_.push_back({kind, text});
    bytes_ += text.size();
    while (bytes_ > HISTORY_BYTES && entries_.size() > 1) {
        bytes_ -= entries_.front().text.size();
        entries_.pop_front();
    }
    // Scrolled back: the lines in view stay put, the marker says there's more below.
    if (scroll_)
        scroll_ = std::min(scroll_ + rows_of(entries_.back()), max_scroll());
    set_dirty();
}

size_t ChatLog::columns() const {
    return std::max<size_t>(1, screen_rect().width() / style().font.char_width());
}

size_t ChatLog::rows_of(const Entry& e) const {
    return std::max<size_t>(1, (e.text.size() + columns() - 1) / columns());
}

size_t ChatLog::visible_rows() const {
    return screen_rect().height() / style().font.line_height();
}

size_t ChatLog::max_scroll() const {
    size_t rows = 0;
    for (const auto& e : entries_)
        rows += rows_of(e);
    return rows > visible_rows() ? rows - visible_rows() : 0;
}

bool ChatLog::scroll_by(const int32_t rows) {
    const int32_t target = std::clamp<int32_t>((int32_t)scroll_ + rows, 0, max_scroll());
    if ((size_t)target == scroll_)
        return false;
    scroll_ = target;
    set_dirty();
    return true;
}

bool ChatLog::on_key(const KeyEvent key) {
    if (key == KeyEvent::Up)
        return scroll_by(1);  // at the oldest line: not handled, so the focus moves up
    if (key == KeyEvent::Down) {
        scroll_by(-1);
        return true;
    }
    return false;
}

bool ChatLog::on_encoder(const EncoderEvent delta) {
    scroll_by(-delta);
    return true;
}

void ChatLog::paint(Painter& painter) {
    const auto r = screen_rect();
    const auto& s = style();
    const auto line_height = s.font.line_height();
    const size_t cols = columns();
    painter.fill_rectangle(r, s.background);

    // From the bottom up, each entry wrapped to the width; the first scroll_ rows are skipped.
    Coord y = r.top() + (visible_rows() - 1) * line_height;
    size_t skip = scroll_;
    for (auto e = entries_.rbegin(); e != entries_.rend() && y >= r.top(); ++e) {
        const Color color = e->kind == Kind::Own    ? Color::green()
                            : e->kind == Kind::Info ? Color::grey()
                                                    : s.foreground;
        for (size_t row = rows_of(*e); row-- > 0 && y >= r.top();) {
            if (skip) {
                skip--;
                continue;
            }
            painter.draw_string({r.left(), y}, s.font, color, s.background,
                                std::string_view{e->text}.substr(row * cols, cols));
            y -= line_height;
        }
    }

    if (scroll_) {
        const std::string more = " more below ";
        const Coord w = more.size() * s.font.char_width();
        painter.draw_string({r.right() - w, r.top() + (Coord)(visible_rows() - 1) * line_height},
                            s.font, s.background, s.foreground, more);
    }
    if (has_focus())
        painter.draw_rectangle(r, Theme::getInstance()->fg_light->foreground);
}

ChatSettingsView::ChatSettingsView(NavigationView& nav, ChatSettings& settings)
    : settings_{settings}, nickname_{settings.nickname}, secret_{settings.secret} {
    add_children({&labels, &button_nickname, &field_tx_gain, &check_tx_amp, &check_log,
                  &check_small_font, &check_announce, &options_crypto, &button_key, &button_save});
    button_nickname.set_text(nickname_);
    button_nickname.on_select = [this, &nav](Button&) {
        text_prompt(nav, nickname_, 12, ENTER_KEYBOARD_MODE_ALPHA, [this](std::string& s) {
            button_nickname.set_text(s);
        });
    };
    field_tx_gain.set_value(settings_.tx_gain);
    check_tx_amp.set_value(settings_.tx_amp);
    check_log.set_value(settings_.enable_log);
    check_small_font.set_value(settings_.small_font);
    check_announce.set_value(settings_.announce);

    options_crypto.set_by_value(settings_.crypto_mode);
    options_crypto.on_change = [this](size_t, OptionsField::value_t) {
        // a password doesn't make a hex key or the other way round
        chat_crypto::wipe(&secret_[0], secret_.size());
        secret_.clear();
        update_key_button();
    };
    button_key.on_select = [this, &nav](Button&) {
        const bool hex = options_crypto.selected_index_value() == ChatSettings::CRYPTO_HEX_KEY;
        text_prompt(nav, secret_, 64, hex ? ENTER_KEYBOARD_MODE_HEX : ENTER_KEYBOARD_MODE_ALPHA,
                    [this](std::string&) { update_key_button(); });
    };
    update_key_button();

    button_save.on_select = [this, &nav](Button&) {
        const uint8_t mode = options_crypto.selected_index_value();
        uint8_t key[chat_crypto::KEY_BYTES];
        if (mode != ChatSettings::CRYPTO_OFF && secret_.empty()) {
            button_key.set_text("Enter a key first");
            return;
        }
        if (mode == ChatSettings::CRYPTO_HEX_KEY && !chat_crypto::parse_hex_key(secret_, key)) {
            button_key.set_text("Key: 64 hex digits");
            return;
        }
        chat_crypto::wipe(key, sizeof(key));
        settings_.key_changed = mode != settings_.crypto_mode || secret_ != settings_.secret;
        settings_.crypto_mode = mode;
        chat_crypto::wipe(&settings_.secret[0], settings_.secret.size());
        settings_.secret = mode == ChatSettings::CRYPTO_OFF ? "" : secret_;

        if (!nickname_.empty())
            settings_.nickname = nickname_;
        settings_.tx_gain = field_tx_gain.value();
        settings_.tx_amp = check_tx_amp.value();
        settings_.enable_log = check_log.value();
        settings_.small_font = check_small_font.value();
        settings_.announce = check_announce.value();
        nav.pop();
    };
}

ChatSettingsView::~ChatSettingsView() {
    chat_crypto::wipe(&secret_[0], secret_.size());
}

void ChatSettingsView::update_key_button() {
    const auto mode = options_crypto.selected_index_value();
    button_key.set_text(mode == ChatSettings::CRYPTO_OFF ? "No key needed"
                        : secret_.empty()                ? "Enter key"
                                                         : "Key entered (change)");
    button_key.set_focusable(mode != ChatSettings::CRYPTO_OFF);
}

SubGhzChatView::SubGhzChatView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_subghz_chat);

    add_children({&field_frequency, &field_rf_amp, &field_lna, &field_vga, &rssi, &channel, &text_rxtx,
                  &button_send, &text_status, &button_config, &chat_log});

    field_frequency.set_step(25000);

    button_send.on_select = [this](Button&) {
        if (transmitting_)
            return;
        draft_.clear();
        text_prompt(nav_, draft_, MAX_TEXT, ENTER_KEYBOARD_MODE_ALPHA, [this](std::string& s) {
            if (!s.empty())
                send(s);
        });
    };
    button_config.on_select = [this](Button&) {
        nav_.push<ChatSettingsView>(chat_settings_);
        nav_.set_on_pop([this]() {
            apply_key();
            refresh_ui();
        });
    };

    log_file_.append(logs_dir / u"SUBGHZCHAT.TXT");
    if (!settings_.loaded()) {
        // Two radios close together overload each other's receiver.
        field_rf_amp.set_value(0);
        field_lna.set_value(8);
        field_vga.set_value(10);
        show(ChatLog::Kind::Info, "Close by? RX amp off, LNA/VGA low");
    }
    show(ChatLog::Kind::Info, "Not encrypted, see Config");
    refresh_ui();
    receiver_model.enable();
    opened_ = chTimeNow();
}

SubGhzChatView::~SubGhzChatView() {
    // As the Flipper: say goodbye. The view is going, so wait here for the bits to go out.
    if (chat_settings_.announce && joined_ && !transmitting_) {
        const size_t bits = build_packets(chat_settings_.nickname + LEFT);
        if (bits) {
            start_tx(bits);
            chThdSleepMilliseconds(bits / 10 + 50);  // 10 bits per ms
        }
    }
    if (transmitting_)
        transmitter_model.disable();
    else
        receiver_model.disable();
    baseband::shutdown();
    cipher_.clear();
    chat_crypto::wipe(&chat_settings_.secret[0], chat_settings_.secret.size());
}

void SubGhzChatView::focus() {
    button_send.focus();
}

void SubGhzChatView::refresh_ui() {
    chat_log.set_style(chat_settings_.small_font ? Theme::getInstance()->bg_darkest_small
                                                 : Theme::getInstance()->bg_darkest);
    text_status.set(chat_settings_.nickname + (cipher_.enabled() ? " ENC" : "") +
                    (chat_settings_.enable_log ? " LOG" : ""));
}

// The key from Config: SHA-256 of the password as on the Flipper, or the hex key itself.
void SubGhzChatView::apply_key() {
    if (!chat_settings_.key_changed)
        return;
    chat_settings_.key_changed = false;
    const bool was_enabled = cipher_.enabled();
    if (chat_settings_.crypto_mode == ChatSettings::CRYPTO_OFF) {
        cipher_.clear();
        if (was_enabled)
            show(ChatLog::Kind::Info, "Not encrypted");
        return;
    }
    uint8_t key[chat_crypto::KEY_BYTES];
    const auto& secret = chat_settings_.secret;
    if (chat_settings_.crypto_mode == ChatSettings::CRYPTO_PASSWORD)
        chat_crypto::sha256(reinterpret_cast<const uint8_t*>(secret.data()), secret.size(), key);
    else if (!chat_crypto::parse_hex_key(secret, key))
        return;
    // the run id: random (receiver noise), with the name and time on top
    std::string seed = noise();
    seed += chat_settings_.nickname + to_string_dec_uint(chTimeNow()) + to_string_timestamp(rtc_time::now());
    cipher_.set_key(key, reinterpret_cast<const uint8_t*>(seed.data()), seed.size());
    chat_crypto::wipe(key, sizeof(key));
    show(ChatLog::Kind::Info, "Encrypted (AES-256-GCM)");
    // the Flipper joins once its key is set: those with the same key see it now
    if (joined_)
        announce(JOINED);
}

// Only the indicator, not a set_dirty() of the whole view: no repaint of the chat on every switch.
void SubGhzChatView::set_rxtx(const bool tx) {
    text_rxtx.set(tx ? " TX " : " RX ");
}

void SubGhzChatView::show(const ChatLog::Kind kind, const std::string& line) {
    chat_log.add(kind, line);
    if (chat_settings_.enable_log)
        log_file_.write_entry(line);
}

// Receiver noise, kept up by the chat image at the start of bb_data (only while receiving).
std::string SubGhzChatView::noise() const {
    return std::string(reinterpret_cast<const char*>(shared_memory.bb_data.data),
                       SubGhzChatPacketMessage::NOISE_POOL_BYTES) +
           to_string_dec_uint(chTimeNow());
}

void SubGhzChatView::send(const std::string& text) {
    const std::string line = chat_settings_.nickname + ": " + text;
    const size_t bits = build_packets(line);
    if (!bits) {
        text_status.set("Too long");
        return;
    }
    // What came in before this goes above it, even if its quiet time hasn't run out yet.
    if (!received_.empty())
        flush_received();
    show(ChatLog::Kind::Own, line);
    start_tx(bits);
}

// "Name joined chat." and "Name left chat.", as the Flipper says it.
void SubGhzChatView::announce(const char* what) {
    if (!chat_settings_.announce || transmitting_)
        return;
    const std::string line = chat_settings_.nickname + what;
    const size_t bits = build_packets(line);
    if (!bits)
        return;
    show(ChatLog::Kind::Info, line);
    start_tx(bits);
}

// A line in packets of up to 60 bytes into tx_bits_, as the Flipper's chat sends it; the
// number of bits, 0 when it doesn't fit.
size_t SubGhzChatView::build_packets(const std::string& line) {
    // as the Flipper: plain text ends in CR LF, encrypted doesn't
    std::string payload;
    if (cipher_.enabled()) {
        const std::string random = noise();
        payload = cipher_.encrypt(line, reinterpret_cast<const uint8_t*>(random.data()), random.size());
    } else {
        payload = line + "\r\n";
    }

    tx_bits_.fill(0);
    // The transmitter reads 32 bits past the end; leave room for them.
    cc1101::BitWriter writer{tx_bits_.data(), (tx_bits_.size() - 4) * 8};
    for (size_t i = 0; i < LEAD_IN_BYTES; i++)
        writer.byte(0xAA);
    for (size_t pos = 0; pos < payload.size(); pos += cc1101::MAX_PAYLOAD) {
        if (pos)
            for (size_t i = 0; i < GAP_BYTES; i++)
                writer.byte(0xAA);
        const size_t n = std::min(cc1101::MAX_PAYLOAD, payload.size() - pos);
        if (!writer.packet((const uint8_t*)payload.data() + pos, n))
            return 0;
    }
    return writer.bits();
}

void SubGhzChatView::start_tx(const size_t bits) {
    transmitting_ = true;
    set_rxtx(true);
    receiver_model.disable();
    baseband::shutdown();
    chThdSleepMilliseconds(50);

    baseband::run_image(portapack::spi_flash::image_tag_fsktx);
    chThdSleepMilliseconds(50);
    memcpy(shared_memory.bb_data.data, tx_bits_.data(), tx_bits_.size());

    transmitter_model.set_target_frequency(receiver_model.target_frequency());
    transmitter_model.set_sampling_rate(TX_SAMPLE_RATE);
    transmitter_model.set_baseband_bandwidth(1750000);
    transmitter_model.set_rf_amp(chat_settings_.tx_amp);
    transmitter_model.set_tx_gain(chat_settings_.tx_gain);
    // The FSK image sends nothing until it is configured: first the radio, then the bits, or
    // the start of the packet goes out while the synthesizer is still locking.
    transmitter_model.enable();
    chThdSleepMilliseconds(TX_SETTLE_MS);
    baseband::set_fsk_data(bits, TX_SAMPLES_PER_BIT, cc1101::DEVIATION_HZ, 1000);
    tx_started_ = chTimeNow();
    tx_timeout_ = MS2ST(bits / 10) + TX_MARGIN;  // 10 bits per ms
}

void SubGhzChatView::start_rx() {
    if (!transmitting_)
        return;
    transmitter_model.disable();
    baseband::shutdown();
    chThdSleepMilliseconds(50);

    baseband::run_image(portapack::spi_flash::image_tag_subghz_chat);
    chThdSleepMilliseconds(50);
    receiver_model.set_sampling_rate(3072000);
    receiver_model.set_baseband_bandwidth(1750000);
    receiver_model.enable();
    transmitting_ = false;
    set_rxtx(false);
}

void SubGhzChatView::on_packet(const SubGhzChatPacketMessage& message) {
    if (received_.size() + message.length <= MAX_RECEIVED)
        received_.append((const char*)message.data, message.length);
    last_packet_ = chTimeNow();
}

void SubGhzChatView::on_frame_sync() {
    // Join once the receiver has run a moment: what's on air now comes first.
    if (!joined_ && !transmitting_ && chTimeNow() - opened_ >= JOIN_DELAY) {
        joined_ = true;
        announce(JOINED);
    }
    if (transmitting_ && chTimeNow() - tx_started_ >= tx_timeout_)
        start_rx();
    if (!received_.empty() && chTimeNow() - last_packet_ >= MESSAGE_GAP)
        flush_received();
}

// The message that has come in, as one line.
void SubGhzChatView::flush_received() {
    std::string text{};
    std::string line{};
    const bool decrypted = cipher_.decrypt(received_, text);
    if (!decrypted)
        text = received_;
    bool printable = true;
    for (const char c : text) {
        if (c == '\r' || c == '\n')
            continue;
        printable &= c >= 32 && c < 127;
        line += (c >= 32 && c < 127) ? c : '?';
    }
    const bool encrypted_looking = !printable && received_.size() > chat_crypto::ChatCipher::OVERHEAD;
    chat_crypto::wipe(&text[0], text.size());
    received_.clear();

    const bool presence = ends_with(line, JOINED) || ends_with(line, LEFT);
    if (decrypted || (printable && !cipher_.enabled()))
        show(presence ? ChatLog::Kind::Info : ChatLog::Kind::Received, line);
    else if (printable)
        show(ChatLog::Kind::Received, "(not encrypted) " + line);
    else if (encrypted_looking)
        show(ChatLog::Kind::Info, cipher_.enabled() ? "Can't decrypt: other key?" : "Encrypted message: set the key in Config");
    else
        show(ChatLog::Kind::Received, line);
}

}  // namespace ui::external_app::subghz_chat
