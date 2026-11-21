/*
 * LoRa RX - MeshCore receiver.
 *
 * Channels: Public (built in) + lines of /LORA/CHANNELS.TXT: "#name" (hashtag channel)
 * or "Name=<32 hex digits>" (private channel key). Log: /LOGS/LORA_RX.TXT.
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

// Keep everything inside this app's namespace (no constprop clones in the firmware image).
#pragma GCC optimize("no-ipa-cp")

#include "ui_lora_rx.hpp"

#include "baseband_api.hpp"
#include "file.hpp"
#include "portapack.hpp"
#include "rtc_time.hpp"
#include "string_format.hpp"
#include "ui_font_fixed_5x8.hpp"
#include "ui_font_fixed_8x16.hpp"

#include <cstring>

using namespace portapack;

namespace ui::external_app::lora_rx {

namespace {
constexpr uint32_t default_freq = 869618000;  // MeshCore EU/UK narrow
const uint8_t public_key[16] = {0x8b, 0x33, 0x87, 0xe9, 0xc5, 0xcd, 0xea, 0x6a, 0xc9, 0xe5, 0xed, 0xba, 0xa1, 0x15, 0xcd, 0x72};

// Small fixed-size string builder (the M0 stack is small: keep N to what each line needs).
template <int N>
struct Buf {
    char s[N];
    int n{0};
    Buf& str(const char* p) {
        while (*p && n < N - 2) s[n++] = *p++;
        s[n] = 0;
        return *this;
    }
    Buf& num(int32_t v) {
        char t[12];
        int k = 0;
        bool neg = v < 0;
        uint32_t u = neg ? -v : v;
        do {
            t[k++] = '0' + u % 10;
            u /= 10;
        } while (u);
        if (neg) t[k++] = '-';
        while (k && n < N - 2) s[n++] = t[--k];
        s[n] = 0;
        return *this;
    }
};

int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
}  // namespace

LoraRxView::LoraRxView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_t{'P', 'L', 'R', 'A'});

    add_children({&labels, &field_lna, &field_vga, &field_rf_amp, &rssi,
                  &field_frequency, &field_agc, &text_status, &messages});
    // the baseband image must be running before the waterfall is added
    waterfall = new spectrum::WaterfallView(true);
    add_child(waterfall);
    layout({0, 0, screen_width, screen_height - 16});  // set_parent_rect() redoes it with the real size

    if (field_frequency.value() < 860000000 || field_frequency.value() > 930000000)
        field_frequency.set_value(default_freq);
    field_frequency.set_step(1000);

    receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);
    receiver_model.set_sampling_rate(2000000);
    receiver_model.set_baseband_bandwidth(1750000);
    receiver_model.enable();

    field_agc.set_by_value(1);
    field_agc.on_change = [this](size_t, OptionsField::value_t v) {
        agc_on = v != 0;
        quiet_reports = 0;
        if (agc_on) apply_gain();
        update_status();
    };
    apply_gain();

    load_channels();
    Buf<160> b;
    for (int i = 0; i < nchannels; i++) b.str(i ? ", " : "").str(channels[i].name);
    if (channels_ignored) b.str("  (+").num(channels_ignored).str(" ignored: 16 max)");
    messages.add("", "", 0, MessageList::Kind::Info, "Channels", b.s);
    update_status();
}

LoraRxView::~LoraRxView() {
    if (waterfall) {
        remove_child(waterfall);
        Widget* w = waterfall;
        delete w;  // through the virtual destructor
    }
    receiver_model.disable();
    baseband::shutdown();
}

void LoraRxView::layout(Rect r) {
    const int top = UI_POS_Y(3);
    const int wf_h = r.height() / 4;
    messages.set_parent_rect({0, top, r.width(), r.height() - wf_h - top});
    if (waterfall) waterfall->set_parent_rect({0, r.height() - wf_h, r.width(), wf_h});
}

void LoraRxView::set_parent_rect(const Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);
    layout(new_parent_rect);
}

void LoraRxView::focus() {
    field_frequency.focus();
}

void LoraRxView::load_channels() {
    memcpy(channels[0].key, public_key, 16);
    strcpy(channels[0].name, "Public");
    channel_set_key(channels[0]);
    nchannels = 1;

    File f;
    if (f.open(u"/LORA/CHANNELS.TXT")) return;  // Optional<Error>: true on error
    // whole file on the heap (not the small M0 stack), up to 2 KB
    uint32_t n = (uint32_t)f.size();
    if (n > 2048) n = 2048;
    std::string text;
    text.resize(n);
    auto r = f.read(&text[0], n);
    f.close();
    if (r.is_error()) return;

    char* p = &text[0];
    while (*p) {
        char* line = p;
        while (*p && *p != '\n') p++;
        char* e = p;
        if (*p) p++;
        while (e > line && (e[-1] == '\r' || e[-1] == ' ')) e--;
        *e = 0;
        const bool is_channel = (line[0] == '#' && line[1]) || strchr(line, '=');
        if (is_channel && nchannels >= MAX_CHANNELS) {
            channels_ignored++;
            continue;
        }
        Channel& ch = channels[nchannels];
        if (line[0] == '#' && line[1]) {
            channel_from_name(line, ch);
            nchannels++;
        } else if (char* eq = strchr(line, '=')) {
            *eq = 0;
            const char* h = eq + 1;
            bool ok = strlen(h) >= 32;
            for (int i = 0; ok && i < 16; i++) {
                int a = hexval(h[2 * i]), b = hexval(h[2 * i + 1]);
                if (a < 0 || b < 0) ok = false;
                ch.key[i] = a << 4 | b;
            }
            if (ok && line[0]) {
                strncpy(ch.name, line, 16);
                ch.name[16] = 0;
                channel_set_key(ch);
                nchannels++;
            }
        }
    }
}

void LoraRxView::update_status() {
    Buf<48> b;
    b.str("Packets ").num(count_all).str(" ok ").num(count_ok);
    b.str(agc_on ? "  AGC " : "  Gain ").num(field_lna.value() + field_vga.value()).str("dB");
    if (overruns) b.str(" !").num(overruns);  // decoder fell behind (should stay 0)
    text_status.set(std::string_view{b.s, (size_t)b.n});
}

void LoraRxView::apply_gain() {
    // total gain T = LNA + VGA: VGA goes down first (30 -> 14 dB), then LNA (32 -> 0, 8 dB steps)
    int t = gain_db();
    int lna = t - 14 > 0 ? (t - 14) / 8 * 8 : 0;
    if (lna > 32) lna = 32;
    int vga = t - lna;
    if (vga > 30) vga = 30;
    field_rf_amp.set_value(0);
    field_lna.set_value(lna);
    field_vga.set_value(vga & ~1);
}

void LoraRxView::on_level(const LoraLevelMessage& m) {
    if (m.overruns != overruns) {
        overruns = m.overruns;
        update_status();
    }
    if (!agc_on) return;
    if (m.overload) {
        // sustained in-channel clipping: -12 dB at once, again after each 16 ms until it stops
        if (gain_idx < GAIN_STEPS - 1) {
            if (gain_before_packet < 0) {
                gain_before_packet = gain_idx;
                packet_seen = false;
                preambles_at_drop = m.preambles;
            }
            reports_since_drop = 0;
            gain_idx += 3;
            if (gain_idx > GAIN_STEPS - 1) gain_idx = GAIN_STEPS - 1;
            apply_gain();
            update_status();
        }
        quiet_reports = 0;
        return;
    }
    if (gain_before_packet >= 0) {
        // after a drop: back to the previous gain as soon as the packet is over (decoded
        // packets restore it in on_packet()). A strong signal that is not LoRa (no packet
        // seen within 2 s) keeps the slow climb instead, so the gain does not oscillate.
        if (m.busy || m.preambles != preambles_at_drop) packet_seen = true;
        if (!m.busy) {
            if (packet_seen) {
                restore_gain();
                return;
            }
            if (++reports_since_drop >= 8) gain_before_packet = -1;
        }
    }
    if (!m.busy && m.loud_pct < 10 && gain_idx > 0) {
        // mostly weak (brief bursts allowed) and idle for 2 s (8 reports): +4 dB
        if (++quiet_reports >= 8) {
            gain_idx--;
            quiet_reports = 0;
            apply_gain();
            update_status();
        }
    } else {
        quiet_reports = 0;
    }
}

void LoraRxView::restore_gain() {
    if (gain_before_packet < 0) return;
    gain_idx = gain_before_packet;
    gain_before_packet = -1;
    quiet_reports = 0;
    apply_gain();
    update_status();
}

// Three steps that never share the stack: the M0 stack is small, decryption needs ~800 bytes
// and the log file (FatFS object with its sector buffer) ~600.
void LoraRxView::on_packet(const LoraPacketMessage& m) {
    const bool crc_ok = m.has_crc && m.crc_ok;
    if (agc_on) restore_gain();  // the packet that lowered the gain is over
    count_all++;
    if (crc_ok) count_ok++;
    update_status();

    if (!crc_ok) {
        strcpy(title_, "bad CRC");
        text_[0] = 0;
        kind_ = MessageList::Kind::BadCrc;
    } else if (meshcore_decode(m.data, m.length, channels, nchannels, title_, sizeof(title_), text_, sizeof(text_))) {
        kind_ = MessageList::Kind::Channel;
    } else {
        kind_ = strncmp(title_, "advert", 6) == 0 ? MessageList::Kind::Advert : MessageList::Kind::Other;
    }
    snr_db_ = m.snr_db;
    if (m.snr_db > 20)
        strcpy(snr_, ">20dB");
    else {
        Buf<8> b;
        b.str(m.snr_db > 0 ? "+" : "").num(m.snr_db).str("dB");
        strcpy(snr_, b.s);
    }
    show_packet();
    log_packet(m);
}

void LoraRxView::show_packet() {
    rtc::RTC now;
    rtc_time::now(now);
    messages.add(to_string_datetime(now, HMS).c_str(), snr_, snr_db_, kind_, title_, text_);
}

void LoraRxView::log_packet(const LoraPacketMessage& m) {
    // date, CRC, SNR, CFO, gain, decoded text, then the raw packet in hex
    rtc::RTC now;
    rtc_time::now(now);
    ensure_directory(u"/LOGS");
    File f;
    if (f.append(u"/LOGS/LORA_RX.TXT")) return;  // Optional<Error>: error
    Buf<240> lg;
    lg.str(to_string_datetime(now).c_str()).str(m.has_crc && m.crc_ok ? " ok SNR " : " CRC SNR ").str(snr_);
    lg.str(" ").num(m.cfo_hz).str("Hz G").num(field_lna.value() + field_vga.value()).str(" ").str(title_);
    if (text_[0]) lg.str(" | ").str(text_);
    f.write_line(std::string{lg.s});
    std::string hex;
    static const char hd[] = "0123456789abcdef";
    for (int i = 0; i < m.length; i++) {
        hex += hd[m.data[i] >> 4];
        hex += hd[m.data[i] & 15];
    }
    f.write_line(hex);
}

void MessageList::add(const char* time, const char* snr, int snr_db, Kind kind, const char* title, const char* text) {
    newest_ = (newest_ + 1) % MAX_ENTRIES;
    Entry& e = entries_[newest_];
    strncpy(e.time, time, sizeof(e.time) - 1);
    e.time[sizeof(e.time) - 1] = 0;
    strncpy(e.snr, snr, sizeof(e.snr) - 1);
    e.snr[sizeof(e.snr) - 1] = 0;
    e.snr_db = (int8_t)snr_db;
    e.kind = kind;
    strncpy(e.title, title, sizeof(e.title) - 1);
    e.title[sizeof(e.title) - 1] = 0;
    strncpy(e.text, text, sizeof(e.text) - 1);
    e.text[sizeof(e.text) - 1] = 0;
    if (count_ < MAX_ENTRIES) count_++;
    set_dirty();
}

void MessageList::paint(Painter& painter) {
    const Rect r = screen_rect();
    const Color bg = Theme::getInstance()->bg_darkest->background;
    painter.fill_rectangle(r, bg);
    const auto& small = ui::font::fixed_5x8;
    const auto& big = ui::font::fixed_8x16;
    const int small_h = small.line_height();
    const int big_h = big.line_height();
    const int cols = r.width() / big.char_width();
    int y = r.top();

    for (int k = 0; k < count_; k++) {
        const Entry& e = entries_[(newest_ - k + MAX_ENTRIES) % MAX_ENTRIES];
        if (y + small_h > r.bottom()) break;

        // metadata: time (grey), SNR (green / yellow / orange), title (by kind)
        int x = r.left();
        if (e.time[0]) {
            x += painter.draw_string({x, y}, small, Color::grey(), bg, e.time);
            x += small.char_width();
        }
        if (e.snr[0]) {
            const Color c = e.snr_db >= 5 ? Color::green() : e.snr_db >= -3 ? Color::yellow()
                                                                            : Color::orange();
            x += painter.draw_string({x, y}, small, c, bg, e.snr);
            x += small.char_width();
        }
        // (if chain rather than switch: no lookup table in a shared section)
        Color title_color = Color::light_grey();
        if (e.kind == Kind::Channel)
            title_color = Color::cyan();
        else if (e.kind == Kind::Advert)
            title_color = Color::magenta();
        else if (e.kind == Kind::Other)
            title_color = Color::grey();
        else if (e.kind == Kind::BadCrc)
            title_color = Color::red();
        painter.draw_string({x, y}, small, title_color, bg, e.title);
        y += small_h + 1;

        // word-wrapped text; for channel messages "sender:" in yellow
        int sender_len = 0;
        if (e.kind == Kind::Channel) {
            for (int i = 0; e.text[i] && i < 40; i++)
                if (e.text[i] == ':' && e.text[i + 1] == ' ') {
                    sender_len = i + 1;
                    break;
                }
        }
        const char* p = e.text;
        while (*p) {
            if (y + big_h > r.bottom()) return;
            int n = (int)strlen(p);
            if (n > cols) {
                n = cols;
                for (int i = cols; i > cols / 2; i--)
                    if (p[i] == ' ') {
                        n = i;
                        break;
                    }
            }
            const int offset = p - e.text;
            int head = sender_len - offset;
            head = head < 0 ? 0 : (head > n ? n : head);
            int tx = r.left();
            if (head) tx += painter.draw_string({tx, y}, big, Color::yellow(), bg, std::string_view{p, (size_t)head});
            if (n > head) painter.draw_string({tx, y}, big, Color::white(), bg, std::string_view{p + head, (size_t)(n - head)});
            y += big_h;
            p += n;
            while (*p == ' ') p++;
        }
        y += 3;
    }
}

void MessageList::getAccessibilityText(std::string& result) {
    result = "";
    if (newest_ < 0) return;
    const Entry& e = entries_[newest_];
    result = e.time;
    result += " ";
    result += e.snr;
    result += " ";
    result += e.title;
    result += " | ";
    result += e.text;
}

void MessageList::getWidgetName(std::string& result) {
    result = "MessageList";
}

}  // namespace ui::external_app::lora_rx
