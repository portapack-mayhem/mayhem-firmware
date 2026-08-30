/*
 * Copyright (C) 2026 Alexey Verhogladov
 *
 * This file is part of PortaPack.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ui_meshtastic.hpp"
#include "ui_qrcode.hpp"
#include "debug.hpp"
#include "ch.h"

#include "baseband_api.hpp"
#include "portapack.hpp"
#include "audio.hpp"  // codec output for RX beeps
#include "event_m0.hpp"
#include "battery.hpp"
#include "radio.hpp"
#include "string_format.hpp"
#include "rtc_time.hpp"
#include "ui_textentry.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "file_path.hpp"
#include "file.hpp"

#include <cstring>
#include <algorithm>
#include <cmath>

using namespace portapack;

namespace ui {

namespace {

// One packet's worth of scratch, shared by everything that assembles a frame.
// Every one of these used to be a stack local - 255 bytes apiece, and on_timer alone
// held six of them, which is most of why its frame came to 968 bytes out of the M0's
// 4 KB process stack. The buffer is only ever live between building a packet and
// handing it to queue_tx (which copies it), and the UI runs on one thread, so a single
// buffer serves them all.
uint8_t pkt_scratch[meshtastic::PKT_MAX_SIZE];

// The alert bell as it travels. Clients key on the ASCII character; the phone app
// writes the emoji beside it so the message reads as something as well as ringing.
// Kept as separate pieces because a hex escape swallows every hex digit after it -
// "\x07Bell" once compiled to the single byte 0xBE plus "ll", and the bell never
// reached the air.
const char BELL_EMOJI[] = "\xF0\x9F\x94\x94";  // U+1F514 BELL, UTF-8
const char BELL_WORDS[] = " Alert Bell Character! ";
constexpr char BELL_CHAR = '\x07';

// Either half counts: a phone sends both, but the character alone is the old
// convention and the emoji alone still means someone wanted attention.
bool has_bell(const std::string& s) {
    return s.find(BELL_CHAR) != std::string::npos ||
           s.find(BELL_EMOJI) != std::string::npos;
}

void strip_bell(std::string& s) {
    s.erase(std::remove(s.begin(), s.end(), BELL_CHAR), s.end());
    for (size_t i; (i = s.find(BELL_EMOJI)) != std::string::npos;)
        s.erase(i, sizeof(BELL_EMOJI) - 1);
}

// Value of one hex digit, or -1. Four copies of this lambda had accumulated across the
// key, channel and settings parsers.
int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

}  // namespace

// Per-node chat colours. Index 0 means "no mark"; the rest are picked in a node's
// detail page and drawn as a square to the left of every message from that node.
static Color node_palette(uint8_t idx) {
    switch (idx) {
        case 1:
            return Color::red();
        case 2:
            return Color::green();
        case 3:
            return Color::blue();
        case 4:
            return Color::yellow();
        case 5:
            return Color::cyan();
        case 6:
            return Color::magenta();
        case 7:
            return Color::orange();
        default:
            return Color::black();
    }
}

using namespace meshtastic;

// ============================================================================
// Helpers
// ============================================================================

// Real wall-clock time as unix UTC seconds (0 if the RTC looks unset).
// Meshtastic packets carry a unix timestamp; sending uptime instead made
// everything show up dated 1970 on peers.
// The LPC RTC powers up at 1980 and stays there until something sets it. That is a
// perfectly good unix time, so the old "u > 0" test let it through and every position,
// telemetry and NodeInfo we sent went out stamped 1980 - which is exactly what a phone's
// position log showed. No clock is better than a wrong one: every encoder here omits the
// time field when it is zero, and a receiver then treats the reading as undated.
static bool rtc_is_set() {
    return rtc_time::now().year() >= 2024;
}

static uint32_t now_unix() {
    if (!rtc_is_set()) return 0;
    auto t = rtc_time::now();
    const time_t u = rtc_time::rtcToUnixUTC(t);
    return (u > 0) ? static_cast<uint32_t>(u) : 0;
}

// unix seconds -> calendar date, Howard Hinnant's civil_from_days. Nothing on this
// device sets the clock - no GPS, no network - but every Meshtastic peer puts its own
// wall time in POSITION (field 4) and Telemetry (field 1), so the mesh can supply it.
static void unix_to_rtc(uint32_t unix_s, rtc::RTC& out) {
    const uint32_t secs = unix_s % 86400u;
    uint32_t days = unix_s / 86400u + 719468u;  // shift the epoch to 0000-03-01
    const uint32_t era = days / 146097u;
    const uint32_t doe = days % 146097u;
    const uint32_t yoe = (doe - doe / 1460u + doe / 36524u - doe / 146096u) / 365u;
    uint32_t y = yoe + era * 400u;
    const uint32_t doy = doe - (365u * yoe + yoe / 4u - yoe / 100u);
    const uint32_t mp = (5u * doy + 2u) / 153u;
    const uint32_t d = doy - (153u * mp + 2u) / 5u + 1u;
    const uint32_t m = mp + ((mp < 10u) ? 3u : -9u);
    if (m <= 2u) y++;
    out = rtc::RTC(y, m, d, secs / 3600u, (secs / 60u) % 60u, secs % 60u);
}

// Format frequency Hz -> "868.825 MHz" for display
static std::string fmt_freq_mhz(uint32_t hz) {
    if (hz == 0) return "Auto (Region)";
    const uint32_t whole = hz / 1000000;
    const uint32_t frac = (hz % 1000000) / 1000;  // kHz portion -> 3 decimal digits
    std::string s = to_string_dec_uint(whole) + ".";
    // Zero-pad the fractional part to 3 digits
    if (frac < 10)
        s += "00";
    else if (frac < 100)
        s += "0";
    s += to_string_dec_uint(frac) + " MHz";
    return s;
}

// ============================================================================
// MeshConsole - UTF-8 chat console (Cyrillic from SD, emoji tokens)
// ============================================================================

namespace {

// Decode one UTF-8 sequence at s[i], advance i past it, return false at end.
// Truncated/invalid sequences (possible on a partially-decoded RX packet) yield
// U+FFFD and advance one byte so we never loop or read out of bounds.
bool utf8_next(const std::string& s, size_t& i, uint32_t& cp) {
    if (i >= s.size()) return false;
    const uint8_t c = static_cast<uint8_t>(s[i]);
    int n;
    if (c < 0x80) {
        cp = c;
        i += 1;
        return true;
    } else if ((c & 0xE0) == 0xC0) {
        cp = c & 0x1F;
        n = 1;
    } else if ((c & 0xF0) == 0xE0) {
        cp = c & 0x0F;
        n = 2;
    } else if ((c & 0xF8) == 0xF0) {
        cp = c & 0x07;
        n = 3;
    } else {
        cp = 0xFFFD;
        i += 1;
        return true;
    }
    for (int k = 1; k <= n; ++k) {
        if (i + k >= s.size() || (static_cast<uint8_t>(s[i + k]) & 0xC0) != 0x80) {
            cp = 0xFFFD;
            i += 1;
            return true;  // truncated
        }
        cp = (cp << 6) | (static_cast<uint8_t>(s[i + k]) & 0x3F);
    }
    i += n + 1;
    return true;
}

// Append the UTF-8 encoding of cp (<= U+FFFF for our use) to out.
void utf8_encode(uint32_t cp, std::string& out) {
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

// Common emoji -> short ASCII token. nullptr = not mapped (-> placeholder). Color
// emoji is impossible (1bpp font, 2-colour glyph draw), so tokens are the sane
// substitute for a text chat.
const char* emoji_token(uint32_t cp) {
    switch (cp) {
        case 0x1F44D:
            return "+1";
        case 0x1F44E:
            return "-1";
        case 0x2764:
        case 0x2665:
        case 0x1F496:
        case 0x1F49B:
        case 0x1F493:
            return "<3";
        case 0x1F600:
        case 0x1F601:
        case 0x1F603:
        case 0x1F604:
        case 0x1F642:
        case 0x1F60A:
            return ":)";
        case 0x1F609:
            return ";)";
        case 0x1F602:
        case 0x1F923:
            return ":D";
        case 0x1F612:
        case 0x1F614:
        case 0x1F61E:
        case 0x1F622:
        case 0x1F62D:
            return ":(";
        case 0x1F60E:
            return "B)";
        case 0x1F618:
        case 0x1F617:
            return ":*";
        case 0x1F525:
            return "[fire]";
        case 0x2705:
        case 0x2714:
            return "[ok]";
        case 0x274C:
        case 0x274E:
            return "[x]";
        case 0x26A1:
            return "[!]";
        case 0x1F4CD:
        case 0x1F4CC:
            return "[pin]";
        case 0x1F680:
            return "[^]";
        case 0x2B50:
        case 0x1F31F:
            return "[*]";
        case 0x1F514:
            return "[bell]";
        case 0x1F44B:
            return "[wave]";
        case 0x1F64F:
            return "[pls]";
        default:
            return nullptr;
    }
}

// A ROUTING packet is an ACK when its Routing payload carries no error (or
// error_reason == NONE).  Meshtastic's Routing.error_reason is varint field 3;
// an ACK serialises to an empty payload, a NAK sets the field.
// Routing.Error from mesh.proto, the ones a text message actually meets. Named rather
// than numbered because the number alone sends the reader to a .proto file, and the
// difference between "nobody had a route" and "your key is not the one I hold" is the
// difference between waiting and fixing something.
static const char* routing_error_name(uint32_t e) {
    switch (e) {
        case 1:
            return "no route";
        case 2:
            return "got NAK";
        case 3:
            return "timeout";
        case 5:
            return "max retransmit";
        case 6:
            return "no channel";
        case 7:
            return "too large";
        case 8:
            return "no response";
        case 9:
            return "duty cycle";
        case 32:
            return "bad request";
        case 33:
            return "not authorized";
        case 34:
            return "PKC failed";
        case 35:
            return "PKC unknown key";
        case 38:
            return "rate limit";
        default:
            return "error";
    }
}

// The reason itself, not just whether there was one. 0 is an acknowledgement.
static uint32_t routing_error(const meshtastic::DecodedData& d) {
    size_t i = 0;
    while (i < d.payload_len) {
        uint64_t tag;
        size_t c = meshtastic::decode_varint(d.payload + i, d.payload_len - i, tag);
        if (!c) break;
        i += c;
        const uint8_t fn = static_cast<uint8_t>(tag >> 3);
        const uint8_t wt = static_cast<uint8_t>(tag & 0x07);
        if (wt != 0) break;
        uint64_t v;
        c = meshtastic::decode_varint(d.payload + i, d.payload_len - i, v);
        if (!c) break;
        i += c;
        if (fn == 3) return static_cast<uint32_t>(v);
    }
    return 0;
}

static bool routing_is_ack(const meshtastic::DecodedData& d) {
    size_t i = 0;
    while (i < d.payload_len) {
        uint64_t tag;
        size_t c = meshtastic::decode_varint(d.payload + i, d.payload_len - i, tag);
        if (!c) break;
        i += c;
        const uint8_t fn = static_cast<uint8_t>(tag >> 3);
        const uint8_t wt = static_cast<uint8_t>(tag & 0x07);
        if (wt == 0) {
            uint64_t v;
            c = meshtastic::decode_varint(d.payload + i, d.payload_len - i, v);
            if (!c) break;
            i += c;
            if (fn == 3) return v == 0;  // error_reason: 0 = ACK
        } else {
            break;
        }
    }
    return true;  // empty / no error field -> ACK
}

}  // namespace

MeshConsole::MeshConsole(Rect parent_rect)
    : Widget{parent_rect} {
    cols_ = parent_rect.width() / CHAR_W;
    visible_ = parent_rect.height() / LINE_H;  // rows on screen; max_lines_ = history
    set_focusable(true);                       // so the encoder / Up-Down can scroll
    // Claim the history up front: growing it a line at a time chops the heap into
    // fragments, and the keyboard view later needs one contiguous block.
    lines_.reserve(max_lines_);
    lines_.emplace_back();
}

void MeshConsole::new_line() {
    lines_.emplace_back();
    lines_.back().tech = writing_tech_;
    lines_.back().msg = pending_msg_;
    lines_.back().colour = writing_col_;
    lines_.back().mine = writing_mine_;
    cur_cols_ = 0;
    while (lines_.size() > max_lines_) lines_.erase(lines_.begin());
}

void MeshConsole::visible_lines(std::vector<size_t>& out) const {
    out.clear();  // capacity is kept, so this allocates only once
    out.reserve(lines_.size());
    for (size_t k = lines_.size(); k > 0; --k) {
        const size_t idx = k - 1;
        const bool tech = lines_[idx].tech;
        if (filter_ == 1 && tech) continue;
        if (filter_ == 2 && !tech) continue;
        out.push_back(idx);
    }
}

size_t MeshConsole::message_at(Coord y) const {
    const auto r = screen_rect();
    const int row = (y - r.top()) / LINE_H;
    if (row < 0 || row >= static_cast<int>(visible_)) return SIZE_MAX;
    visible_lines(vis_buf_);  // newest first
    const auto& vis = vis_buf_;
    const size_t max_back = (vis.size() > visible_) ? vis.size() - visible_ : 0;
    const size_t back = (scroll_back_ < max_back) ? scroll_back_ : max_back;
    const size_t shown = (vis.size() - back < visible_) ? vis.size() - back : visible_;
    if (static_cast<size_t>(row) >= shown) return SIZE_MAX;
    // Row 0 is the top of the window, which is the oldest line shown.
    const size_t idx = vis[back + shown - 1 - static_cast<size_t>(row)];
    if (!lines_[idx].msg) return SIZE_MAX;
    return lines_[idx].msg - 1;
}

namespace {
// The glyph table, held here rather than inside the chat view: the keyboard needs it
// too, to letter its own keys in whatever alphabet the card supplies. One table, one
// owner, and the chat view still decides when it is loaded and freed.
// One table per face. The card's glyphs have to match the height of the font they sit
// beside, so a table drawn for the 8x16 face is useless to the 5x8 one and the compact
// chat showed Latin only. Two files, loaded independently: whoever wants Cyrillic in
// both faces puts both on the card, and whoever uses one face pays for one.
struct GlyphTable {
    uint8_t* data{nullptr};
    MeshConsole::GlyphRange ranges[MeshConsole::MAX_RANGES]{};
    uint8_t nranges{0};
    uint8_t stride{16};  // bytes per glyph: (w * h + 7) / 8
};
GlyphTable g_big;    // 8x16, beside the standard face
GlyphTable g_small;  // 5x8, beside the compact one

int glyph_index_in(const GlyphTable& g, uint32_t cp) {
    if (!g.data || cp > 0xFFFF) return -1;
    for (uint8_t i = 0; i < g.nranges; i++) {
        const MeshConsole::GlyphRange& r = g.ranges[i];
        if (cp >= r.first && cp < static_cast<uint32_t>(r.first) + r.count)
            return static_cast<int>(r.index) + static_cast<int>(cp - r.first);
    }
    return -1;
}

// The bitmap for a codepoint in the face being drawn, or nullptr.
const uint8_t* glyph_bitmap_for(uint32_t cp, bool small) {
    const GlyphTable& g = small ? g_small : g_big;
    const int gi = glyph_index_in(g, cp);
    return (gi >= 0) ? g.data + gi * g.stride : nullptr;
}

// The standard face, which is what the keyboard letters its keys with.
const uint8_t* glyph_bitmap(uint32_t cp) {
    return glyph_bitmap_for(cp, false);
}

int glyph_index(uint32_t cp) {
    return glyph_index_in(g_big, cp);
}

// Append one codepoint as UTF-8. Cyrillic is two bytes, so a key press stopped being
// a one-character append.
void utf8_append(std::string& out, uint32_t cp) {
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}
}  // namespace

// GeoMapView is about ten kilobytes: nine slots of tile cache, each holding an open
// file with its own 512-byte FatFs buffer, plus thirty marker records. This app leaves
// around seven, so the map does not open from here - and asking for it anyway panicked
// the device with "Out of Memory". Saying so is worth more than crashing, and the
// number tells the reader whether a restart would help.
// The open chat, so a refusal can be explained wherever it happens. A button that
// silently does nothing is indistinguishable from a broken one - which is exactly how
// it was reported.
MeshtasticChatView* g_chat = nullptr;

// A block the size of the map view, taken while the memory is still whole and held
// until the map wants it. Three kilobytes of settings pages opened and closed leave
// the heap in pieces none of which is big enough, so the map stopped opening after a
// few minutes of ordinary use - not because the memory was gone, but because it was
// in the wrong shape. Reserving costs the room permanently and buys a map that always
// opens, which is the better trade for a button that exists.
void* g_map_reserve = nullptr;

// Hand the reservation back to the allocator so the push that follows takes it. The
// free list is address-ordered and this is the only block of its size, so the very
// next request of that size gets it.
static void map_reserve_take() {
    if (g_map_reserve) {
        chHeapFree(g_map_reserve);
        g_map_reserve = nullptr;
    }
}

// A block the size of the map view, taken while the heap is still whole and held until
// the map wants it. The map needs 3736 bytes in one piece, and after a few settings
// pages have been opened and closed the largest free block is about 1400 - not because
// the memory is gone (eight kilobytes are free) but because it is in pieces.
//
// Tried once before and reverted: at that point the app left 4920 bytes of core, and
// holding 3736 of them starved the settings pages - "want 1412 free 744", and a walk
// through the app ended with twenty-four bytes left. Two entries off the node table and
// the glyph tables staying unloaded moved the startup figure to 8640, which leaves
// 4904 after the reservation against the ~2550 the settings pages actually take. The
// numbers are the reason it is back, and the same numbers are what to check if it
// misbehaves again.
static void map_reserve_restore() {
    if (!g_map_reserve) g_map_reserve = chHeapAlloc(0x0, sizeof(GeoMapView));
}

static bool map_would_fit(MeshtasticChatView* say_here) {
    if (!say_here) say_here = g_chat;
    if (g_map_reserve) {
        map_reserve_take();
        return true;
    }
    // Ask the allocator, do not estimate. chCoreStatus() reports the core that has not
    // been carved up yet, which is not the same as the memory available: once a few
    // screens have been opened and closed the core is nearly all spoken for while the
    // heap holds plenty of freed blocks. Guarding on it refused a map that would have
    // opened perfectly well - and the one call site that had no guard proved it by
    // opening anyway.
    //
    // So take the exact block the view needs and give it straight back. Freeing puts it
    // at the head of the free list, so the push that follows gets the same block.
    void* probe = chHeapAlloc(0x0, sizeof(GeoMapView));
    if (probe) {
        chHeapFree(probe);
        return true;
    }
    // Naming the way out matters: the memory comes back with the app, not with time.
    if (say_here)
        say_here->write_console("* map needs memory: reopen the app\n");
    return false;
}

// A signed decimal degree, parsed by hand and bounded. atof() takes anything: a word
// becomes 0 and puts the node on Null Island, and 90.5 is accepted as a latitude that
// cannot exist - the map draws from about -85 to 85, so the position simply vanished
// off the top of the world with nothing to say why. Returns false and changes nothing
// when the text is not a number in range.
static bool parse_degrees(const std::string& v, float limit, float& out) {
    size_t i = 0;
    bool neg = false;
    if (i < v.size() && (v[i] == '-' || v[i] == '+')) {
        neg = (v[i] == '-');
        i++;
    }
    uint32_t whole = 0, frac = 0, fdigits = 0;
    bool any = false;
    for (; i < v.size() && v[i] >= '0' && v[i] <= '9'; i++) {
        whole = whole * 10 + static_cast<uint32_t>(v[i] - '0');
        any = true;
        if (whole > 180) return false;
    }
    if (i < v.size() && (v[i] == '.' || v[i] == ',')) {
        i++;
        for (; i < v.size() && v[i] >= '0' && v[i] <= '9'; i++) {
            if (fdigits < 6) {
                frac = frac * 10 + static_cast<uint32_t>(v[i] - '0');
                fdigits++;
            }
            any = true;
        }
    }
    if (!any || i != v.size()) return false;  // a letter, two dots, trailing junk
    float scale = 1.0f;
    for (uint32_t k = 0; k < fdigits; k++) scale *= 10.0f;
    float d = static_cast<float>(whole) + static_cast<float>(frac) / scale;
    if (d > limit) return false;
    out = neg ? -d : d;
    return true;
}

// ---- Text entry ------------------------------------------------------------

const uint16_t MeshTextEntry::SETS[4][MeshTextEntry::KEYS] = {
    {
        0x0031,
        0x0032,
        0x0033,
        0x0034,
        0x0035,
        0x0036,
        0x0037,
        0x0038,
        0x0039,
        0x0030,
        0x0071,
        0x0077,
        0x0065,
        0x0072,
        0x0074,
        0x0079,
        0x0075,
        0x0069,
        0x006F,
        0x0070,
        0x0061,
        0x0073,
        0x0064,
        0x0066,
        0x0067,
        0x0068,
        0x006A,
        0x006B,
        0x006C,
        0x002D,
        0x007A,
        0x0078,
        0x0063,
        0x0076,
        0x0062,
        0x006E,
        0x006D,
        0x002C,
        0x002E,
        0x003F,
    },
    {
        0x0021,
        0x0040,
        0x0023,
        0x0024,
        0x0025,
        0x005E,
        0x0026,
        0x002A,
        0x0028,
        0x0029,
        0x0051,
        0x0057,
        0x0045,
        0x0052,
        0x0054,
        0x0059,
        0x0055,
        0x0049,
        0x004F,
        0x0050,
        0x0041,
        0x0053,
        0x0044,
        0x0046,
        0x0047,
        0x0048,
        0x004A,
        0x004B,
        0x004C,
        0x005F,
        0x005A,
        0x0058,
        0x0043,
        0x0056,
        0x0042,
        0x004E,
        0x004D,
        0x003B,
        0x003A,
        0x002F,
    },
    {
        0x0439,
        0x0446,
        0x0443,
        0x043A,
        0x0435,
        0x043D,
        0x0433,
        0x0448,
        0x0449,
        0x0437,
        0x0444,
        0x044B,
        0x0432,
        0x0430,
        0x043F,
        0x0440,
        0x043E,
        0x043B,
        0x0434,
        0x0436,
        0x044F,
        0x0447,
        0x0441,
        0x043C,
        0x0438,
        0x0442,
        0x044C,
        0x0431,
        0x044E,
        0x044D,
        0x0445,
        0x044A,
        0x0451,
        0x0020,
        0x002E,
        0x002C,
        0x002D,
        0x003F,
        0x0021,
        0x003A,
    },
    {
        0x0419,
        0x0426,
        0x0423,
        0x041A,
        0x0415,
        0x041D,
        0x0413,
        0x0428,
        0x0429,
        0x0417,
        0x0424,
        0x042B,
        0x0412,
        0x0410,
        0x041F,
        0x0420,
        0x041E,
        0x041B,
        0x0414,
        0x0416,
        0x042F,
        0x0427,
        0x0421,
        0x041C,
        0x0418,
        0x0422,
        0x042C,
        0x0411,
        0x042E,
        0x042D,
        0x0425,
        0x042A,
        0x0401,
        0x0020,
        0x002E,
        0x002C,
        0x002D,
        0x003F,
        0x0021,
        0x003A,
    },
};

// What pressing "Aa" gives next, so the label is a promise rather than a riddle.
const char* const MeshTextEntry::SET_NAMES[5] = {"ab", "AB", "ru", "RU", "..."};

// shift, script, space, delete, ok - the space bar gets what Esc used to hold.
const uint8_t MeshTextEntry::CTRL_W[MeshTextEntry::CTRLS] = {40, 40, 80, 40, 40};

int MeshTextEntry::ctrl_x(int idx) const {
    int x = 0;
    for (int i = 0; i < idx && i < CTRLS; i++) x += CTRL_W[i];
    return x;
}

bool MeshTextEntry::repeatable(int8_t s) const {
    return (s >= 0 && s < KEYS) || s == KEYS + 2 || s == KEYS + 3;
}

namespace {
// The open entry, if any. One at a time by construction: it is a modal screen.
MeshTextEntry* g_entry = nullptr;
}  // namespace

// Draw a UTF-8 string the way the chat does: ASCII and Latin-1 from the firmware
// font, everything else from the card's glyph table. painter.draw_string() walks
// bytes, so a Cyrillic letter came out as two Latin-1 squiggles.
static void draw_utf8(Point p, const std::string& s, Color fg, Color bg) {
    Coord x = p.x();
    size_t i = 0;
    uint32_t cp;
    Painter painter;
    while (utf8_next(s, i, cp)) {
        if (x + 8 > screen_width) break;
        if (cp >= 0x20 && cp < 0x80) {
            const char c[2] = {static_cast<char>(cp), 0};
            painter.draw_string({x, p.y()}, font::fixed_8x16, fg, bg, c);
        } else if (cp >= 0xA0 && cp <= 0xFF) {
            const char c[2] = {static_cast<char>(cp), 0};
            painter.draw_string({x, p.y()}, font::fixed_8x16, fg, bg, c);
        } else if (const uint8_t* pix = glyph_bitmap(cp)) {
            portapack::display.draw_glyph({x, p.y()}, Glyph{8, 16, pix}, fg, bg);
        }
        x += 8;
    }
}

void mesh_text_entry_tick() {
    if (g_entry) g_entry->on_frame();
}

MeshTextEntry::~MeshTextEntry() {
    if (g_entry == this) g_entry = nullptr;
}

MeshTextEntry::MeshTextEntry(NavigationView& nav,
                             std::string& str,
                             size_t max_len,
                             bool start_numeric,
                             std::function<void(std::string&)> on_done)
    : nav_{nav}, str_{str}, max_len_{max_len}, on_done_{std::move(on_done)} {
    set_focusable(true);
    // Digits share the top row with the letters, so a numeric field starts with the
    // cursor on "1" rather than on a different keyboard.
    if (start_numeric) sel_ = 0;
    caret_ = str_.size();
    build_extras();
    g_entry = this;
}

// Everything the card's table can draw that the four fixed layouts do not already
// carry, laid out in codepoint order. No language is named anywhere in here.
void MeshTextEntry::focus() {
    View::focus();
}

void MeshTextEntry::build_extras() {
    extras_n_ = 0;
    if (!g_big.data) return;
    for (uint8_t r = 0; r < g_big.nranges && extras_n_ < KEYS; r++) {
        const MeshConsole::GlyphRange& rg = g_big.ranges[r];
        for (uint16_t k = 0; k < rg.count && extras_n_ < KEYS; k++) {
            const uint16_t cp = static_cast<uint16_t>(rg.first + k);
            bool known = false;
            for (uint8_t si = 0; si < 4 && !known; si++)
                for (int c = 0; c < KEYS; c++)
                    if (SETS[si][c] == cp) {
                        known = true;
                        break;
                    }
            if (!known) extras_[extras_n_++] = cp;
        }
    }
}

uint8_t MeshTextEntry::scripts_available() const {
    if (!g_big.data) return 1;  // latin only: the keys are drawn in the standard face
    return extras_n_ ? 3 : 2;
}

bool MeshTextEntry::del_one() {
    if (caret_ > str_.size()) caret_ = str_.size();
    if (caret_ == 0) return false;
    size_t n = caret_ - 1;
    while (n > 0 && (static_cast<uint8_t>(str_[n]) & 0xC0) == 0x80) n--;
    str_.erase(n, caret_ - n);
    caret_ = n;
    follow_caret();
    set_dirty();
    return true;
}

void MeshTextEntry::on_frame() {
    // A finger resting on DEL: after half a second it starts eating the line.
    if (pressing_ && repeatable(sel_)) {
        if (++hold_ >= HOLD_START && ((hold_ - HOLD_START) % HOLD_EVERY) == 0) {
            const size_t before = str_.size();
            activate();
            if (str_.size() != before) repeated_ = true;
        }
    } else {
        hold_ = 0;
    }
    // Half a second on, half a second off. Only the caret cell is redrawn.
    if (++caret_tick_ < 30) return;
    caret_tick_ = 0;
    caret_on_ = !caret_on_;
    set_dirty();
}

size_t MeshTextEntry::char_count() const {
    size_t n = 0;
    for (char c : str_)
        if ((static_cast<uint8_t>(c) & 0xC0) != 0x80) n++;
    return n;
}

size_t MeshTextEntry::byte_at(size_t chars) const {
    size_t n = 0, i = 0;
    for (; i < str_.size(); i++) {
        if ((static_cast<uint8_t>(str_[i]) & 0xC0) == 0x80) continue;
        if (n == chars) return i;
        n++;
    }
    return str_.size();
}

size_t MeshTextEntry::chars_before(size_t bytes) const {
    size_t n = 0;
    for (size_t i = 0; i < bytes && i < str_.size(); i++)
        if ((static_cast<uint8_t>(str_[i]) & 0xC0) != 0x80) n++;
    return n;
}

void MeshTextEntry::follow_caret() {
    const size_t cols = screen_width / 8;
    const size_t window = cols * TEXT_ROWS - 8;  // room kept for the counter
    const size_t pos = chars_before(caret_);
    if (pos < view_start_) view_start_ = pos;
    if (pos >= view_start_ + window) view_start_ = pos - window + 1;
}

void MeshTextEntry::place_caret(Coord x, Coord y) {
    const size_t cols = screen_width / 8;
    const int row = (y - 4) / 16;
    const int col = (x - 2) / 8;
    size_t n = view_start_ + static_cast<size_t>((row < 0 ? 0 : row)) * cols +
               static_cast<size_t>(col < 0 ? 0 : col);
    const size_t total = char_count();
    if (n > total) n = total;
    caret_ = byte_at(n);
    follow_caret();
    redraw_all_ = true;
    set_dirty();
}

void MeshTextEntry::put(uint32_t cp) {
    // max_len_ counts bytes, the way every caller's buffer does. A Cyrillic letter is
    // two of them, so a "twelve character" name holds six - the honest answer for a
    // field whose size the protocol fixes in bytes.
    const size_t need = (cp < 0x80) ? 1u : (cp < 0x800 ? 2u : 3u);
    if (str_.size() + need > max_len_) return;
    std::string one;
    utf8_append(one, cp);
    if (caret_ > str_.size()) caret_ = str_.size();
    str_.insert(caret_, one);
    caret_ += one.size();
    follow_caret();
    // Shift for one character falls away once that character is typed; locked does not.
    if (shift_ == 1) {
        shift_ = 0;
        redraw_all_ = true;
    }
    set_dirty();
}

void MeshTextEntry::activate() {
    if (sel_ < KEYS) {
        const uint8_t page = (script_ < 2) ? static_cast<uint8_t>(script_ * 2 + (shift_ ? 1 : 0)) : 4;
        const uint16_t cp = (page < 4) ? SETS[page][sel_]
                                       : ((sel_ < extras_n_) ? extras_[sel_] : 0);
        if (cp) put(cp);
        return;
    }
    switch (sel_ - KEYS) {
        case 0:  // shift: off -> next character only -> locked -> off, as on a phone
            shift_ = static_cast<uint8_t>((shift_ + 1) % 3);
            redraw_all_ = true;
            set_dirty();
            break;
        case 1:  // script
            script_ = static_cast<uint8_t>((script_ + 1) % scripts_available());
            redraw_all_ = true;
            set_dirty();
            break;
        case 2:
            put(' ');
            break;
        case 3:
            del_one();
            break;
        default:  // accept
            if (on_done_) on_done_(str_);
            nav_.pop();
            break;
    }
}

int MeshTextEntry::hit(Coord x, Coord y) const {
    if (y < GRID_Y) return FIELD;  // the text area: places the caret, types nothing
    if (y >= CTRL_Y && y < CTRL_Y + CTRL_H) {
        int acc = 0;
        for (int k = 0; k < CTRLS; k++) {
            acc += CTRL_W[k];
            if (x < acc) return KEYS + k;
        }
        return KEYS + CTRLS - 1;
    }
    if (y < GRID_Y || y >= GRID_Y + ROWS * KEY_H) return -1;
    const int col = x / KEY_W;
    const int row = (y - GRID_Y) / KEY_H;
    if (col < 0 || col >= COLS || row < 0 || row >= ROWS) return -1;
    return row * COLS + col;
}

bool MeshTextEntry::on_touch(const TouchEvent event) {
    const auto r = screen_rect();
    const int i = hit(event.point.x() - r.left(), event.point.y() - r.top());
    // A finger dragged across the panel arrives as Start, a stream of Moves, and End,
    // with the contact breaking and remaking on the way. Only the first of those
    // decides anything: Move is ignored outright, and a Start too soon after the last
    // release is bounce rather than a new press.
    if (event.type == TouchEvent::Type::Move) return true;
    if (event.type == TouchEvent::Type::End) {
        // Fires the key that is lit, not the one under the release coordinate: on a
        // resistive panel the last sample before the finger leaves often jumps a key
        // or two, and requiring the two to agree threw away good presses.
        (void)i;
        // A press that already repeated has done its work; firing again on release
        // would eat one character more than the finger asked for.
        if (pressing_ && !repeated_) activate();
        pressing_ = false;
        repeated_ = false;
        hold_ = 0;
        last_release_ = static_cast<uint32_t>(chTimeNow());
        set_dirty();
        return true;
    }
    if (static_cast<uint32_t>(chTimeNow()) - last_release_ < TOUCH_GUARD) return true;
    if (i == FIELD) {
        const auto rr = screen_rect();
        sel_ = FIELD;  // and stays there, so Left/Right keep walking the caret
        place_caret(event.point.x() - rr.left(), event.point.y() - rr.top());
        last_release_ = static_cast<uint32_t>(chTimeNow());
        return true;
    }
    if (i >= 0) {
        sel_ = static_cast<int8_t>(i);
        pressing_ = true;
        set_dirty();
    }
    return true;
}

bool MeshTextEntry::on_encoder(const EncoderEvent delta) {
    if (sel_ == FIELD) {
        // In the field the encoder walks the caret, one character at a time.
        const size_t total = char_count();
        size_t pos = chars_before(caret_);
        if (delta > 0 && pos < total) pos++;
        if (delta < 0 && pos > 0) pos--;
        caret_ = byte_at(pos);
        follow_caret();
        redraw_all_ = true;
        set_dirty();
        return true;
    }
    int i = sel_ + delta;
    const int last = KEYS + CTRLS - 1;
    if (i < 0) i = last;
    if (i > last) i = 0;
    sel_ = static_cast<int8_t>(i);
    set_dirty();
    return true;
}

bool MeshTextEntry::on_key(const KeyEvent key) {
    // Left and Up together: the firmware's own "back", and it leaves without saving
    // wherever the selection happens to be.
    if (key == KeyEvent::Back) {
        nav_.pop();
        return true;
    }
    const int last = KEYS + CTRLS - 1;
    if (sel_ == FIELD) {
        const size_t total = char_count();
        size_t pos = chars_before(caret_);
        switch (key) {
            case KeyEvent::Left:
                if (pos > 0) pos--;
                break;
            case KeyEvent::Right:
                if (pos < total) pos++;
                break;
            case KeyEvent::Down:
            case KeyEvent::Select:
                sel_ = 0;  // back to the keys
                redraw_all_ = true;
                set_dirty();
                return true;
            case KeyEvent::Up:
                // Above the text there is nothing left, so this is the way out. Without
                // it the D-pad walked in a circle from the keys to the field and back,
                // and the only exit was to accept what had been typed - Esc having gone
                // to the back arrow, which a finger can reach and a D-pad cannot.
                nav_.pop();
                return true;
            default:
                return false;
        }
        caret_ = byte_at(pos);
        follow_caret();
        redraw_all_ = true;
        set_dirty();
        return true;
    }
    int i = sel_;
    switch (key) {
        case KeyEvent::Select:
            activate();
            return true;
        case KeyEvent::Left:
            i = (i > 0) ? i - 1 : last;
            break;
        case KeyEvent::Right:
            i = (i < last) ? i + 1 : 0;
            break;
        case KeyEvent::Up:
            if (i >= 0 && i < COLS) {
                sel_ = FIELD;  // above the top row is the text
                redraw_all_ = true;
                set_dirty();
                return true;
            }
            i = (i >= KEYS) ? (KEYS - COLS + ((i - KEYS) * COLS) / CTRLS) : (i - COLS);
            break;
        case KeyEvent::Down:
            i = (i >= KEYS) ? ((i - KEYS) * COLS) / CTRLS
                            : (i + COLS < KEYS ? i + COLS : KEYS + (i * CTRLS) / COLS);
            break;
        default:
            return false;
    }
    if (i < 0) i = 0;
    if (i > last) i = last;
    sel_ = static_cast<int8_t>(i);
    set_dirty();
    return true;
}

bool MeshTextEntry::on_keyboard(const KeyboardEvent event) {
    const char c = static_cast<char>(event);
    if (c == '\n' || c == '\r') {
        if (on_done_) on_done_(str_);
        nav_.pop();
        return true;
    }
    if (c == 8 || c == 127) {
        del_one();
        return true;
    }
    if (c >= 0x20 && c < 0x7F) {
        put(static_cast<uint32_t>(c));
        return true;
    }
    return false;
}

void MeshTextEntry::draw_key(Painter& painter, int idx) {
    const auto r = screen_rect();
    const int col = idx % COLS, row = idx / COLS;
    const Coord x = r.left() + col * KEY_W;
    const Coord y = r.top() + GRID_Y + row * KEY_H;
    const bool on = (sel_ == idx);
    // Pressed is brighter than merely selected: with a finger over the key, the only
    // way to know which one it is on is for it to change while held.
    const Color bg = on ? (pressing_ ? Theme::getInstance()->fg_green->foreground
                                     : Theme::getInstance()->fg_blue->foreground)
                        : Theme::getInstance()->bg_medium->background;
    painter.fill_rectangle({x + 1, y + 1, KEY_W - 2, KEY_H - 2}, bg);
    const uint8_t page = (script_ < 2) ? static_cast<uint8_t>(script_ * 2 + (shift_ ? 1 : 0)) : 4;
    const uint16_t cp = (page < 4) ? SETS[page][idx] : ((idx < extras_n_) ? extras_[idx] : 0);
    if (!cp) return;  // an empty cell on a short extras page
    const Coord tx = x + KEY_W / 2 - 4, ty = y + KEY_H / 2 - 8;
    if (cp < 0x80) {
        const char c[2] = {static_cast<char>(cp), 0};
        painter.draw_string({tx, ty}, font::fixed_8x16,
                            Theme::getInstance()->fg_light->foreground, bg, c);
    } else if (const uint8_t* pix = glyph_bitmap(cp)) {
        // Lettered from the card's own table, the same bitmaps the chat draws with.
        portapack::display.draw_glyph({tx, ty}, Glyph{8, 16, pix},
                                      Theme::getInstance()->fg_light->foreground, bg);
    }
}

void MeshTextEntry::draw_ctrl(Painter& painter, int idx) {
    static const char* const SHIFTS[3] = {"ab", "Ab", "AB"};
    static const char* const SCRIPTS[3] = {"lat", "rus", "..."};
    static const char* const NAMES[CTRLS] = {"", "", "space", "DEL", "OK"};
    const auto r = screen_rect();
    const int w = CTRL_W[idx];
    const Coord x = r.left() + ctrl_x(idx);
    const Coord y = r.top() + CTRL_Y;
    const bool on = (sel_ == KEYS + idx);
    const Color bg = on ? (pressing_ ? Theme::getInstance()->fg_green->foreground
                                     : Theme::getInstance()->bg_dark->background)
                        : Theme::getInstance()->bg_medium->background;
    painter.fill_rectangle({x + 1, y + 1, w - 2, CTRL_H - 2}, bg);
    // Shift shows the case it is in; script shows the one it will switch to.
    const char* label = (idx == 0)   ? SHIFTS[shift_]
                        : (idx == 1) ? SCRIPTS[(script_ + 1) % scripts_available()]
                                     : NAMES[idx];
    const int len = static_cast<int>(strlen(label));
    // Locked shift is stated rather than hinted: an underline under "AB".
    painter.draw_string({x + w / 2 - len * 4, y + CTRL_H / 2 - 8}, font::fixed_8x16,
                        Theme::getInstance()->fg_light->foreground, bg, label);
    if (idx == 0 && shift_ == 2)
        painter.draw_hline({x + w / 2 - len * 4, y + CTRL_H / 2 + 8}, len * 8,
                           Theme::getInstance()->fg_light->foreground);
}

void MeshTextEntry::draw_text_line(Painter& painter) {
    const auto r = screen_rect();
    painter.fill_rectangle({r.left(), r.top(), screen_width, TEXT_H},
                           Theme::getInstance()->bg_darkest->background);
    // A frame while the field holds the selection: otherwise there is no telling why
    // Left and Right stopped moving between keys.
    if (sel_ == FIELD)
        painter.draw_rectangle({r.left(), r.top(), screen_width, TEXT_H},
                               Theme::getInstance()->fg_blue->foreground);
    const size_t cols = screen_width / 8;
    const size_t total = char_count();
    // Two rows of text, with the last eight columns of the second kept for the count.
    const size_t window = cols * TEXT_ROWS - 8;
    const size_t from = (view_start_ > total) ? total : view_start_;
    const size_t upto = ((from + window) < total) ? (from + window) : total;
    const std::string shown = str_.substr(byte_at(from), byte_at(upto) - byte_at(from));

    // Split across the rows by glyph, not by byte.
    size_t drawn = 0, i = 0;
    for (int row = 0; row < TEXT_ROWS && i < shown.size(); row++) {
        size_t start = i, n = 0;
        while (i < shown.size() && n < cols) {
            i++;
            while (i < shown.size() && (static_cast<uint8_t>(shown[i]) & 0xC0) == 0x80) i++;
            n++;
        }
        draw_utf8({r.left() + 2, r.top() + 4 + row * 16}, shown.substr(start, i - start),
                  Theme::getInstance()->fg_light->foreground,
                  Theme::getInstance()->bg_darkest->background);
        drawn += n;
    }
    (void)drawn;

    // A caret rather than a block, and grey rather than green: it marks a place, it is
    // not a character. Only this cell blinks - redrawing the whole line twice a second
    // made the text itself flicker.
    const size_t pos = chars_before(caret_);
    if (pos >= from) draw_caret(painter);

    // How much room is left, because the protocol counts bytes and a message that will
    // not fit should say so before it is typed rather than after it is cut.
    const std::string count = to_string_dec_uint(static_cast<uint32_t>(str_.size())) + "/" +
                              to_string_dec_uint(static_cast<uint32_t>(max_len_));
    painter.draw_string({r.left() + screen_width - static_cast<Coord>(count.size()) * 8 - 2,
                         r.top() + 4 + 16},
                        font::fixed_8x16,
                        (str_.size() >= max_len_) ? Theme::getInstance()->fg_red->foreground
                                                  : Theme::getInstance()->fg_dark->foreground,
                        Theme::getInstance()->bg_darkest->background, count);
    drawn_len_ = str_.size();
}

// Just the caret cell, for the blink. The text around it is already on the glass.
void MeshTextEntry::draw_caret(Painter& painter) {
    const auto r = screen_rect();
    const size_t cols = screen_width / 8;
    const size_t pos = chars_before(caret_);
    if (pos < view_start_) return;
    const size_t off = pos - view_start_;
    if (off >= cols * TEXT_ROWS) return;
    const Coord cx = r.left() + 2 + static_cast<Coord>(off % cols) * 8;
    const Coord cy = r.top() + 4 + static_cast<Coord>(off / cols) * 16;
    // A bar in the gap between two characters, not a "|" drawn over one of them. Sitting
    // on a letter it hid the letter, and left no way to tell whether DEL would take the
    // one underneath or the one before it. The glyphs are six pixels wide in an
    // eight-pixel cell, so the two pixels in front of the cell are empty and the bar
    // covers nothing - which also means the blink can clear them without redrawing text.
    const Coord bx = (cx >= r.left() + 2 + 2) ? cx - 2 : r.left();
    painter.fill_rectangle({bx, cy, 2, 16},
                           caret_on_ ? Theme::getInstance()->fg_medium->foreground
                                     : Theme::getInstance()->bg_darkest->background);
}

void MeshTextEntry::paint(Painter& painter) {
    if (!redraw_all_) {
        // The usual case: a finger moved, a key fired, or the caret blinked. Two keys
        // and maybe the text line differ; the other forty-three are already right.
        if (drawn_len_ != str_.size()) {
            draw_text_line(painter);
            caret_drawn_ = caret_on_;
        } else if (caret_on_ != caret_drawn_) {
            caret_drawn_ = caret_on_;
            draw_caret(painter);  // one cell, so the line beside it stays still
        }
        const int8_t was = drawn_sel_;
        drawn_sel_ = sel_;
        if (was >= 0 && was != sel_) {
            if (was < KEYS)
                draw_key(painter, was);
            else
                draw_ctrl(painter, was - KEYS);
        }
        if (sel_ >= 0) {
            if (sel_ < KEYS)
                draw_key(painter, sel_);
            else
                draw_ctrl(painter, sel_ - KEYS);
        }
        return;
    }
    redraw_all_ = false;
    drawn_sel_ = sel_;
    caret_drawn_ = caret_on_;
    draw_text_line(painter);
    for (int i = 0; i < KEYS; i++) draw_key(painter, i);
    for (int i = 0; i < CTRLS; i++) draw_ctrl(painter, i);
}

void mesh_prompt(NavigationView& nav,
                 std::string& str,
                 size_t max_len,
                 std::function<void(std::string&)> on_done,
                 bool numeric) {
    nav.push<MeshTextEntry>(str, max_len, numeric, std::move(on_done));
}

void MeshConsole::set_ext_font(const uint8_t* glyphs, const GlyphRange*, uint8_t) {
    // The tables themselves live beside the console now, one per face; this only marks
    // whether there is anything to draw with, for the "no glyph" notice.
    ext_glyphs_ = glyphs;
    set_dirty();
}

int MeshConsole::ext_index(uint32_t cp) const {
    return glyph_index_in(small_font_ ? g_small : g_big, cp);
}

void MeshConsole::append_out(uint32_t out_cp) {
    // Our own messages keep the last column for the delivery dot. Without that, a line
    // that filled the width left the dot nowhere to go and it simply was not drawn -
    // so whether a message showed its delivery state depended on how long it was.
    const size_t limit = writing_mine_ ? (cols_ > 1 ? cols_ - 1 : 1) : cols_;
    if (cur_cols_ >= limit) {
        // Break at the last space rather than wherever the column ran out. Our own
        // messages carry their time at the end, and a mid-column break split it: the
        // line showed "[20:" and the next one "37]", which reads as a fault rather
        // than as wrapping. Only whole words move down, and only if the tail is short
        // enough to be worth moving.
        std::string& line = lines_.back().text;
        size_t sp = line.find_last_of(' ');
        std::string tail;
        if (sp != std::string::npos && sp + 1 < line.size() &&
            (line.size() - sp - 1) <= limit / 2) {
            tail = line.substr(sp + 1);
            line.resize(sp);
        }
        new_line();
        if (!tail.empty()) {
            lines_.back().text = tail;
            // Recount in glyphs: the tail may hold multi-byte characters.
            cur_cols_ = glyph_columns(tail);
        }
    }
    utf8_encode(out_cp, lines_.back().text);
    ++cur_cols_;
}

void MeshConsole::emit_cp(uint32_t cp) {
    if (cp == '\t') {
        append_out(' ');
        return;
    }
    if (cp == 0x07) {
        had_bell_ = true;
        return;
    }  // bell: an alert, not a glyph
    // The phone app writes its alert as the bell emoji plus the character. Treat the
    // emoji as an alert in its own right - a client that sent only the emoji still
    // meant to get attention - and then let it fall through to its "[bell]" token, so
    // unlike the bare character it is also something to read.
    if (cp == 0x1F514) had_bell_ = true;
    if (cp < 0x20) return;  // other control -> drop
    if (cp < 0x80) {
        append_out(cp);
        return;
    }  // ASCII
    if (cp >= 0xA0 && cp <= 0xFF) {
        append_out(cp);
        return;
    }  // Latin-1 (accents, deg, u)
    // A card font, if one is loaded, decides the rest of the alphabet.
    if (ext_index(cp) >= 0) {
        append_out(cp);
        return;
    }
    if (const char* tok = emoji_token(cp)) {  // known emoji -> token
        for (; *tok; ++tok) append_out(static_cast<uint8_t>(*tok));
        return;
    }
    // The built-in faces cover ASCII and Latin-1 and nothing else. Rather than pretend,
    // mark the character and let the chat say the text was not fully readable.
    had_unicode_ = true;
    append_out(0xB7);
}

void MeshConsole::set_font_size(uint8_t px) {
    // Two sizes, because the display has exactly two bitmap faces and no outline font
    // to scale: eight pixels is the compact 5x8, sixteen the standard 8x16. Anything in
    // between means dropping rows and columns out of a glyph that is only eight pixels
    // wide to begin with, and the letters fall apart.
    const bool small = (px < 12);
    px = small ? 8 : 16;
    if (px == font_size_) return;
    font_size_ = px;
    small_font_ = small;
    CHAR_W = small ? 5 : 8;
    LINE_H = small ? 8 : 16;
    const auto r = screen_rect();
    cols_ = (r.width() / CHAR_W) ? static_cast<size_t>(r.width() / CHAR_W) : 1;
    visible_ = (r.height() / LINE_H) ? static_cast<size_t>(r.height() / LINE_H) : 1;
    scroll_back_ = 0;
    set_dirty();
}

void MeshConsole::write_tech(const std::string& utf8) {
    writing_tech_ = true;
    write(utf8);
    writing_tech_ = false;
}

void MeshConsole::write_tech_col(const std::string& utf8, uint8_t colour) {
    writing_col_ = colour;
    writing_tech_ = true;
    write(utf8);
    writing_tech_ = false;
    writing_col_ = 0;
}

void MeshConsole::write(const std::string& utf8) {
    scroll_back_ = 0;  // a new message snaps the view back to the newest line
    // The first line of this write inherits the flags; new_line() carries them on.
    // "mine" among them, and it has to be re-stated here: the newline that ends one of
    // our own messages creates the next line while the flag is still set, so that empty
    // line was already marked as ours. Whatever was written into it next - almost
    // always a status line - came out right-aligned like a sent message. One line,
    // every time, just after something we sent.
    if (!lines_.empty()) {
        lines_.back().tech = writing_tech_;
        lines_.back().msg = pending_msg_;
        lines_.back().colour = writing_col_;
        lines_.back().mine = writing_mine_;
    }
    size_t i = 0;
    uint32_t cp;
    while (utf8_next(utf8, i, cp)) {
        if (cp == '\n')
            new_line();
        else if (cp != '\r')
            emit_cp(cp);
    }
    pending_msg_ = 0;
    set_dirty();
}

void MeshConsole::clear(bool /*clear_buffer*/) {
    lines_.clear();
    lines_.emplace_back();
    cur_cols_ = 0;
    scroll_back_ = 0;
    set_dirty();
}

void MeshConsole::write_tagged(const std::string& utf8, uint8_t marker, uint32_t tag) {
    if (cur_cols_ != 0) new_line();  // this message starts on its own line
    // Delivery states belong to messages we sent; the flag follows the text onto every
    // wrapped row so a long one stays aligned as a block, while only the first row
    // carries the dot.
    // 1..4 are delivery states, and all four belong to messages we sent. This read 1..3
    // while there were three of them; the fourth ("sent, never confirmed") then counted
    // as somebody else's on reload, so the message jumped from the right edge to the
    // left and its badge landed in the five pixels reserved for an incoming stripe,
    // where it does not fit. Live it looked right - the flag is set directly there -
    // and only replaying the history showed it.
    writing_mine_ = (marker >= 1 && marker <= 4);
    lines_.back().marker = marker;
    lines_.back().tag = tag;
    lines_.back().mine = writing_mine_;
    write(utf8);
    writing_mine_ = false;
}

void MeshConsole::set_marker(uint32_t tag, uint8_t marker) {
    if (tag == 0) return;
    for (auto& l : lines_)
        if (l.tag == tag) l.marker = marker;
    set_dirty();
}

void MeshConsole::draw_marker(Coord x, Coord y, uint8_t marker) {
    Color c;
    switch (marker) {
        case 1:
            c = Color::yellow();
            break;  // pending / sending
        case 2:
            c = Color::green();
            break;  // delivered (ACK)
        case 3:
            c = Color::red();
            break;  // failed (no ACK)
        case 4: {
            // Sent, and nothing ever came back. That is not a failure - a broadcast has
            // nobody in particular to answer it - so it is neither the red of a dead
            // direct message nor the yellow of one still in flight. A question mark says
            // what we actually know, and a shade of colour cannot.
            //
            // A whole character cell is reserved beside our own messages for this mark
            // and the stripe uses three pixels of it, so the badge has the room.
            // Five wide: exactly the small face's glyph, and exactly what is left
            // between the last text column and the scrollbar.
            const Color badge = Color::yellow();
            portapack::display.fill_rectangle({x, y + 3, 5, 10}, badge);
            Painter painter;
            painter.draw_string({x, y + 4}, {font::fixed_5x8, badge, Color::black()}, "?");
            return;
        }
        default:
            // 8+n: the sender's own colour, so a glance at the left edge is enough
            // to tell who is talking even with names switched off.
            if (marker < 8) return;
            c = node_palette(marker - 8);
            break;
    }
    // A stripe down the left edge of the line rather than a dot beside it: the same
    // mark the node list uses, and it costs two pixels of width instead of a character.
    portapack::display.fill_rectangle({x, y, 3, static_cast<int>(LINE_H) - 1}, c);
}

size_t MeshConsole::glyph_columns(const std::string& utf8) const {
    size_t i = 0, cols = 0;
    uint32_t cp;
    while (utf8_next(utf8, i, cp)) ++cols;
    return cols;
}

void MeshConsole::draw_line(Coord y, const std::string& utf8, Coord indent, uint8_t colour) {
    const auto r = screen_rect();
    const Color fg = colour ? node_palette(colour) : style().foreground;
    const Color bg = style().background;
    const uint8_t* fdata = small_font_ ? font::fixed_5x8.get_data()
                                       : font::fixed_8x16.get_data();
    // Bytes per glyph = (w * h + 7) / 8: five for the 5x8 face, sixteen for 8x16.
    const int gstride = small_font_ ? 5 : 16;
    Coord x = r.left() + indent;
    size_t i = 0;
    uint32_t cp;
    while (x + CHAR_W <= r.right() + 1 && utf8_next(utf8, i, cp)) {
        const uint8_t* pix = nullptr;
        if (cp >= 0x20 && cp < 0x80)
            pix = fdata + (cp - 0x20) * gstride;  // ASCII
        else if (cp >= 0xA0 && cp <= 0xFF && !small_font_)
            pix = fdata + (cp - 0x40) * 16;  // Latin-1 (8x16 face only)
        else {
            // Whichever table matches the face being drawn; there may be neither.
            pix = glyph_bitmap_for(cp, small_font_);
        }
        if (pix)
            portapack::display.draw_glyph({x, y}, Glyph{CHAR_W, LINE_H, pix}, fg, bg);
        x += CHAR_W;
    }
}

void MeshConsole::paint(Painter& painter) {
    const auto r = screen_rect();
    painter.fill_rectangle(r, style().background);
    // Show a visible_-row window ending scroll_back_ lines above the newest line.
    visible_lines(vis_buf_);  // newest first, filter applied
    const auto& vis = vis_buf_;
    const size_t n = vis.size();
    const size_t max_back = (n > visible_) ? n - visible_ : 0;
    const size_t back = (scroll_back_ < max_back) ? scroll_back_ : max_back;
    const size_t shown = (n - back < visible_) ? n - back : visible_;
    const size_t start = n - back - shown;  // index into vis of the topmost row
    Coord y = r.top();
    for (size_t i = 0; i < shown; ++i) {
        // vis is newest-first, so the window runs from vis[back] (its newest line)
        // backwards; row 0 is the oldest line in it. The old expression indexed from
        // the far end of the whole buffer, which pinned the view to the very oldest
        // lines: new messages never appeared at the bottom, and the scrollbar tracked
        // that inverted window. message_at() always used this formula - they disagreed,
        // so tapping a line opened the wrong message too.
        const size_t k = vis[back + shown - 1 - i];
        const uint8_t marker = lines_[k].marker;
        // Our own messages are set against the right edge: it separates what we said
        // from what came in at a glance, and keeps the delivery dot away from the
        // colour stripes that identify other nodes, where the two were easy to confuse.
        const bool mine = lines_[k].mine;
        Coord indent = 0;
        if (mine) {
            const size_t cols = glyph_columns(lines_[k].text);
            const size_t room = (cols_ > 1) ? cols_ - 1 : 1;  // last column holds the dot
            indent = static_cast<Coord>((cols < room ? room - cols : 0) * CHAR_W);
            const Coord dot_x = r.left() + indent + static_cast<Coord>(cols * CHAR_W) + 2;
            // Attempts so far, in front of the message they belong to, for as long as
            // it is still being retried.
            if (!retry_text_.empty() && lines_[k].tag == retry_tag_ && retry_tag_) {
                const Coord rx = r.left() + indent -
                                 static_cast<Coord>((retry_text_.size() + 1) * CHAR_W);
                if (rx >= r.left()) draw_line(y, retry_text_, rx - r.left(), 0);
            }
            // The stripe is three pixels wide, so three is what has to fit. Asking for
            // four lost it entirely in the compact face, where the text reaches one
            // pixel further right - our own messages simply had no delivery colour.
            if (marker && dot_x + 3 <= r.right())  // only the first row carries the dot
                draw_marker(dot_x, y, marker);
        } else if (marker) {
            draw_marker(r.left(), y, marker);
            indent = 5;  // clear the stripe, not a whole character
        }
        draw_line(y, lines_[k].text, indent, lines_[k].colour);
        y += LINE_H;
    }
    // Thin scrollbar on the right edge (only when there is history to scroll). The
    // thumb tracks the visible window and slides as Up/Down page through the buffer.
    if (n > visible_) {
        const Coord track_h = r.height();
        Coord thumb_h = static_cast<Coord>(track_h * visible_ / n);
        if (thumb_h < 6) thumb_h = 6;
        Coord thumb_y = r.top() + static_cast<Coord>(track_h * start / n);
        if (thumb_y + thumb_h > r.bottom()) thumb_y = r.bottom() - thumb_h;
        // One pixel, not two. The delivery badge beside our own messages is six wide and
        // the reserved column ends where the screen does, so a two-pixel bar covered the
        // tail of its question mark - it read as a padlock rather than a question.
        portapack::display.fill_rectangle({r.right() - 1, thumb_y, 1, thumb_h},
                                          style().foreground);
    }
}

bool MeshConsole::on_key(const KeyEvent) {
    // Nothing: the D-pad walks past the chat in a single press instead of paging
    // through it. Getting from the buttons below to the tabs above used to mean
    // scrolling the whole history a line at a time. The encoder scrolls it instead -
    // land on the chat with the D-pad, then turn the wheel.
    return false;
}

bool MeshConsole::on_encoder(const EncoderEvent delta) {
    if (delta == 0) return true;
    const size_t n = lines_.size();
    const size_t max_back = (n > visible_) ? n - visible_ : 0;
    // Turning the encoder the way a page scrolls down moves toward newer messages and
    // takes the scrollbar thumb down with it - the other way round felt inverted
    // against every scrollbar people already use.
    if (delta > 0) {  // scroll down -> newer
        if (scroll_back_ == 0) return false;
        scroll_back_--;
    } else {  // scroll up -> older
        if (scroll_back_ >= max_back) return false;
        scroll_back_++;
    }
    set_dirty();
    return true;
}

// ============================================================================
// MeshtasticLogger
// ============================================================================

void MeshtasticLogger::log_packet(const PacketHeader& header, int8_t rssi, int16_t snr_tenths, uint8_t length, uint8_t colour) {
    // The header, not the whole packet: a MeshPacket is 275 bytes of payload buffer and
    // the receive handler has the M0's 4 KB process stack to live on. The header has
    // everything this row needs anyway.
    if (!header_written_) {
        header_written_ = true;
        log_file.write_raw("time,from,to,id,hops,hoplimit,rssi,snr,len,chan,colour");
    }
    const uint8_t hs = header.hop_start(), hl = header.hop_limit();
    const int16_t s10 = snr_tenths < 0 ? static_cast<int16_t>(-snr_tenths) : snr_tenths;
    // write_raw ends the line itself.
    // ISO order with separators: to_string_timestamp packs YYYYMMDDHHMMSS with none,
    // which a spreadsheet reads as one big number and a person reads with a finger on
    // the screen. This costs a few bytes a row and is the difference between a log you
    // can sort and filter and one you have to decode.
    const auto now = rtc_time::now();
    const std::string when =
        to_string_dec_uint(now.year(), 4, '0') + "-" +
        to_string_dec_uint(now.month(), 2, '0') + "-" +
        to_string_dec_uint(now.day(), 2, '0') + " " +
        to_string_dec_uint(now.hour(), 2, '0') + ":" +
        to_string_dec_uint(now.minute(), 2, '0') + ":" +
        to_string_dec_uint(now.second(), 2, '0');
    log_file.write_raw(
        when + "," +
        to_string_hex(header.from, 8) + "," +
        to_string_hex(header.to, 8) + "," +
        to_string_hex(header.packet_id, 8) + "," +
        to_string_dec_uint((hs >= hl) ? (hs - hl) : 0) + "," +
        to_string_dec_uint(hl) + "," +
        to_string_dec_int(rssi) + "," +
        (snr_tenths < 0 ? "-" : "") + to_string_dec_uint(s10 / 10) + "." +
        to_string_dec_uint(s10 % 10) + "," +
        to_string_dec_uint(length) + "," +
        to_string_hex(header.channel_hash, 2) + "," +
        to_string_dec_uint(colour));
}

// ============================================================================
// MeshtasticChatView
// ============================================================================

MeshtasticChatView::MeshtasticChatView(NavigationView& nav,
                                       MeshRouter& router,
                                       NodeDB& node_db,
                                       Rect parent_rect)
    : View(parent_rect), nav_(nav), router_(router), node_db_(node_db) {
    g_chat = this;
    init_radio_mode_switch();

    add_children({&labels_rf_, &field_lna_, &field_vga_, &field_amp_,
                  &text_pkt_, &text_count_, &text_rssi_,
                  &console_,
                  &button_send_, &button_chan_, &button_dest_, &button_filter_,
                  &button_clear_});
    // The signal meter widget is deliberately left out: it redraws on every RSSI
    // update from the M4, and those framebuffer writes contend for the shared bus and
    // starve the LoRa receiver - dropped samples, broken sync.
    //
    // The Cyrillic font and the saved conversation are not read here either. Opening a
    // file drags in FatFs and a File object with its 512-byte sector buffer, and this
    // constructor already sits at the deepest point of an app launch on a 4 KB stack;
    // both happen on the first timer tick instead, see deferred_load_.

    field_lna_.set_value(receiver_model.lna());
    field_vga_.set_value(receiver_model.vga());
    field_amp_.set_value(receiver_model.rf_amp());

    field_lna_.on_change = [this](int32_t v) { receiver_model.set_lna(v); report_gain(); };
    field_vga_.on_change = [this](int32_t v) { receiver_model.set_vga(v); report_gain(); };
    field_amp_.on_change = [this](int32_t v) { receiver_model.set_rf_amp(v); report_gain(); };

    button_send_.on_select = [this](Button&) {
        // No keyboard when the radio is locked to receive: every key would be typed
        // into a message that cannot leave, and the refusal would only arrive at the
        // end. Say it before the typing, not after.
        if (radio_mode_ == 2) {
            write_console("* receive only: tap RX to unlock\n");
            return;
        }
        compose_str_.clear();
        mesh_prompt(nav_, compose_str_, 80, [this](std::string& text) {
                if (text.empty()) return;
                // Ask for an ACK only on a direct (unicast) message - a broadcast
                // has no single recipient, so it would always time out to red.
                const bool unicast = (dest_id_ != BROADCAST_ADDR);
                if (unicast) {
                    node_db_.mark_dm(dest_id_);  // keep the thread in the list
                    router_.request_key_from(dest_id_);
                }
                uint8_t* const buf = pkt_scratch;
                size_t len = router_.build_text_tx(buf, sizeof(pkt_scratch),
                                                   text.c_str(), text.size(),
                                                   dest_id_, unicast);
                if (len > 0) {
                    // packet_id lives at bytes 8-11 of the header we just built.
                    const uint32_t pid = buf[8] | (buf[9] << 8) | (buf[10] << 16) | (static_cast<uint32_t>(buf[11]) << 24);
                    ChatMessage m;
                    m.from_name = "Me";
                    m.from_id = router_.local_node_id();
                    m.text = text;
                    m.timestamp = to_string_datetime(rtc_time::now(), HM);
                    m.outgoing = true;
                    m.packet_id = pid;
                    // Locked to transmit, nothing is received - so no acknowledgement
                    // can ever arrive and "pending" would be a promise the radio
                    // cannot keep. It goes straight to "sent, nothing came back".
                    m.status = (radio_mode_ == 1) ? 4 : 1;
                    m.sent_tick = now_ticks_;
                    // A broadcast never times out to red, and neither does anything
                    // sent while the radio cannot hear an answer.
                    m.expects_ack = unicast && (radio_mode_ != 1);
                    m.to_id = dest_id_;
                    messages_.push_back(m);
                    console_.set_message_index(messages_.size() - 1);
                    if (messages_.size() > MAX_MESSAGES)
                        messages_.erase(messages_.begin());
                    console_.write_tagged(text + stamp_after() + "\n", m.status, pid);
                    // Saved exactly as shown: the delivery dot carries "this one is
                    // mine", so the old "me:" prefix was noise the live view never had.
                    messages_.back().hist_offset = save_line(text + stamp_after() + "\n",
                                                             m.status);
                    // Which conversation this line lives in, so its delivery byte can be
                    // patched later even if the reader has walked off to another chat.
                    messages_.back().hist_peer = dest_id_;
                    messages_.back().hist_ch = channel_idx_;
                    text_count_.set("m:" + to_string_dec_uint(++total_msgs_));
                    set_pkt_indicator(true);
                    // Hand the built packet up to the owning view to actually key
                    // the radio.  Without this the message was only echoed to the
                    // chat and never transmitted (RF-silent compose+send).
                    if (on_tx_request_) on_tx_request_(buf, len);
                } }, false);
    };

    button_chan_.on_select = [this](Button&) {
        if (on_channels_) on_channels_();
    };

    button_dest_.on_select = [this](Button&) {
        const size_t total = node_db_.count();
        if (total == 0) {
            set_dest(BROADCAST_ADDR);
            return;
        }
        if (dest_id_ == BROADCAST_ADDR) {
            dest_node_idx_ = 0;
        } else {
            dest_node_idx_++;
        }
        if (dest_node_idx_ >= total) {
            set_dest(BROADCAST_ADDR);
            return;
        }
        const NodeEntry* ne = node_db_.at(dest_node_idx_);
        set_dest((ne && ne->active) ? ne->node_id : BROADCAST_ADDR, false);
    };

    button_filter_.on_select = [this](Button&) {
        set_filter(static_cast<uint8_t>((console_.filter() + 1) % 3));
    };

    button_clear_.on_select = [this](Button&) {
        messages_.clear();
        console_.clear(true);
    };
}

void MeshtasticChatView::focus() {
    button_send_.focus();
}

bool MeshtasticChatView::on_touch(const TouchEvent event) {
    if (event.type != TouchEvent::Type::End) return false;
    const auto r = screen_rect();
    const Coord y = event.point.y();
    if (y < r.top() + CON_Y || y >= r.top() + CON_Y + CON_H) return false;
    const size_t idx = console_.message_at(y);  // message_at subtracts its own top
    if (idx >= messages_.size()) return false;
    nav_.push<MeshtasticMessageView>(
        messages_[idx],
        [this](std::string text, uint32_t dest) {
            set_dest(dest);
            if (text.empty()) {  // reply: open the keyboard on this peer
                button_send_.on_select(button_send_);
                return;
            }
            const bool unicast = (dest != BROADCAST_ADDR);
            uint8_t* const buf = pkt_scratch;
            size_t len = router_.build_text_tx(buf, sizeof(pkt_scratch), text.c_str(), text.size(),
                                               dest, unicast);
            if (!len) return;
            if (unicast) router_.request_key_from(dest);
            const uint32_t pid = buf[8] | (buf[9] << 8) | (buf[10] << 16) | (static_cast<uint32_t>(buf[11]) << 24);
            ChatMessage m;
            m.from_name = "Me";
            m.from_id = router_.local_node_id();
            m.to_id = dest;
            m.text = text;
            m.timestamp = to_string_datetime(rtc_time::now(), HM);
            m.outgoing = true;
            m.packet_id = pid;
            m.status = (radio_mode_ == 1) ? 4 : 1;
            m.sent_tick = now_ticks_;
            m.expects_ack = unicast && (radio_mode_ != 1);
            messages_.push_back(m);
            if (messages_.size() > MAX_MESSAGES) messages_.erase(messages_.begin());
            console_.set_message_index(messages_.size() - 1);
            console_.write_tagged(text + stamp_after() + "\n", m.status, pid);
            text_count_.set("m:" + to_string_dec_uint(++total_msgs_));
            set_pkt_indicator(true);
            if (on_tx_request_) on_tx_request_(buf, len);
        });
    return true;
}

void MeshtasticChatView::set_filter(uint8_t mode) {
    // The caption and the filter are set together, always. They used to part company at
    // startup: the filter follows the "Status lines" setting, the caption was whatever
    // the button was built with, so with status lines off the button read "All" while
    // half the chat was hidden - and nothing that went wrong announced itself.
    static const char* names[3] = {"All", "Msg", "Tec"};
    if (mode > 2) mode = 0;
    console_.set_filter(mode);
    button_filter_.set_text(names[mode]);
}

// How deep the scrollback goes. Shrinking it drops the oldest rows straight away
// rather than waiting for them to age out, so the memory is returned when the setting
// says so and not at some later moment the user cannot see.
void MeshConsole::set_max_lines(size_t n) {
    if (n < 4) n = 4;
    max_lines_ = n;
    while (lines_.size() > max_lines_) lines_.erase(lines_.begin());
    if (scroll_back_ > lines_.size()) scroll_back_ = 0;
    set_dirty();
}

void MeshtasticChatView::set_display(const ChatDisplay& d) {
    const bool want_glyphs = d.glyph_font && !display_.glyph_font;
    display_ = d;
    // Turning the table on takes effect at once. It used to wait for the next launch,
    // because the file is read on the first timer tick and the setting was read there
    // too - so ticking the box did nothing visible and looked broken.
    if (want_glyphs) load_ext_font();
    console_.set_font_size(d.font_size);
    console_.set_max_lines(d.hist_lines);
    // A signal reading only means something when there is one node on the other end.
    // On a channel the last packet could be from anyone, so the figure is meaningless
    // there and the room goes to the rest of the status row.
    text_rssi_.hidden(!d.show_db);
    set_filter(d.show_tech ? 0 : 1);
}

std::string MeshtasticChatView::stamp() const {
    if (display_.time_mode == 0) return {};
    return "[" + to_string_datetime(rtc_time::now(), display_.time_mode == 2 ? HMS : HM) + "] ";
}

std::string MeshtasticChatView::stamp_after() const {
    // Our own messages are set against the right edge, so their time reads better
    // after the text than in front of it - the words start where the eye already is,
    // and the times still line up down the right-hand side.
    if (display_.time_mode == 0) return {};
    return " [" + to_string_datetime(rtc_time::now(), display_.time_mode == 2 ? HMS : HM) + "]";
}

std::string MeshtasticChatView::peer_label(const meshtastic::NodeEntry* e,
                                           uint32_t id) const {
    if (display_.name_mode == 3) return {};  // the colour square identifies it
    if (display_.name_mode == 2 || !e) return to_string_hex(id, 8);
    if (display_.name_mode == 1)
        return e->short_name[0] ? std::string(e->short_name) : to_string_hex(id, 8);
    return e->long_name[0] ? std::string(e->long_name) : to_string_hex(id, 8);
}

std::u16string MeshtasticChatView::conversation_path(uint32_t peer, uint8_t channel) {
    // /LOGS/mesh_c<N>.txt for a channel, /LOGS/mesh_d<nodeid>.txt for a private thread.
    // Flat names in an existing directory: creating one costs code we do not have the
    // flash for, and the card always has /LOGS.
    std::u16string p = u"/LOGS/mesh_";
    if (peer != BROADCAST_ADDR) {
        p += u'd';
        const std::string hex = to_string_hex(peer, 8);
        for (char c : hex) p += static_cast<char16_t>(c);
    } else {
        p += u'c';
        p += static_cast<char16_t>('0' + (channel % 10));
    }
    p += u".txt";
    return p;
}

uint32_t MeshtasticChatView::save_line_to(const std::u16string& path,
                                          const std::string& line,
                                          uint8_t marker) {
    if (!display_.save_count) return 0;
    auto f = std::make_unique<File>();
    if (f->append(path).is_valid()) return 0;
    std::string body = line;
    while (!body.empty() && (body.back() == '\n' || body.back() == '\r')) body.pop_back();
    // Colour prefix: 0x01 as a sentinel, then the marker biased into printable range.
    // The first attempt used 'A' + marker, which collided with a line beginning "[" -
    // the timestamp bracket was swallowed as if it were a marker. Written with a plain
    // newline; the log writer's own line ending used to leave a stray carriage return
    // that came back as a blank row between every message.
    const uint32_t marker_at = static_cast<uint32_t>(f->size()) + 1;
    const std::string rec = std::string(1, '\x01') +
                            std::string(1, static_cast<char>(0x20 + (marker & 0x3F))) +
                            body + "\n";
    if (!f->write(rec.data(), rec.size())) return 0;
    return marker_at;  // so a delivery result can be written back over it later
}

uint32_t MeshtasticChatView::save_line(const std::string& line, uint8_t marker) {
    return save_line_to(history_path(), line, marker);
}

void MeshtasticChatView::patch_saved_marker(const ChatMessage& m, uint8_t marker) {
    // One byte, in place: the history should show what became of a message, not the
    // "sending" state it had when the line was written.
    //
    // Into the message's OWN conversation, not whichever one is on screen. A retry runs
    // for up to a minute and the reader is free to walk off to another chat meanwhile;
    // this used to patch the open file at the pending message's offset, which lands on
    // an unrelated byte of an unrelated conversation.
    //
    // And only where a record really begins. The offset is taken when the line is
    // written and the file can move under it afterwards - cleared, or folded back to
    // the last N messages, which drops bytes off the front and shifts everything behind
    // them. Written blind, the delivery byte then lands in the middle of an unrelated
    // line: saved conversations really do carry those scars, a stray byte inside a
    // message or a record with its leading sentinel gone. The sentinel sits one byte in
    // front of the marker, so read it back first and write nothing if it is not there.
    if (!m.hist_offset) return;
    auto f = std::make_unique<File>();
    if (f->open(conversation_path(m.hist_peer, m.hist_ch), false).is_valid()) return;
    if (!f->seek(m.hist_offset - 1)) return;
    char sentinel = 0;
    const auto rd = f->read(&sentinel, 1);
    if (!rd || *rd != 1 || sentinel != '\x01') return;
    if (!f->seek(m.hist_offset)) return;
    const char b = static_cast<char>(0x20 + (marker & 0x3F));
    f->write(&b, 1);
}

void MeshtasticChatView::clear_all_history() {
    // Every path this app can write: one file per channel, one per direct-message peer.
    // No directory scan needed - and none available on a card that may hold thousands
    // of unrelated files.
    for (uint8_t ch = 0; ch <= 10; ch++)
        delete_file(conversation_path(BROADCAST_ADDR, ch));
    for (size_t i = 0; i < node_db_.count(); i++) {
        const NodeEntry* e = node_db_.at(i);
        if (e && e->node_id) delete_file(conversation_path(e->node_id, 0));
    }
    // The files are gone, so every remembered position in them is meaningless. Left
    // set, a message still waiting for its delivery result would write its byte into a
    // fresh, much shorter file at an offset from the old one.
    for (auto& mm : messages_) mm.hist_offset = 0;
    console_.clear(true);
    console_.write_tech("* history cleared\n");
    set_dirty();
}

void MeshtasticChatView::switch_conversation() {
    console_.clear(true);
    load_history();
    set_dirty();
}

void MeshtasticChatView::set_channel_index(uint8_t idx) {
    if (channel_idx_ == idx) return;
    channel_idx_ = idx;
    if (dest_id_ == BROADCAST_ADDR) switch_conversation();
}

void MeshtasticChatView::report_gain() {
    if (loading_gain_ || !on_gain_) return;  // restoring, not changing
    on_gain_(static_cast<uint8_t>(field_lna_.value()),
             static_cast<uint8_t>(field_vga_.value()),
             static_cast<uint8_t>(field_amp_.value()));
}

void MeshtasticChatView::set_gains(uint8_t lna, uint8_t vga, uint8_t amp) {
    loading_gain_ = true;
    receiver_model.set_lna(lna);
    receiver_model.set_vga(vga);
    receiver_model.set_rf_amp(amp);
    field_lna_.set_value(lna);
    field_vga_.set_value(vga);
    field_amp_.set_value(amp);
    loading_gain_ = false;
}

void MeshtasticChatView::update_padlock() {
    const NodeEntry* e = (dest_id_ != BROADCAST_ADDR) ? node_db_.find(dest_id_) : nullptr;
    const bool locked = e ? e->has_pubkey : channel_private_;
    if (active_encrypted_ == locked) return;
    active_encrypted_ = locked;
    set_dirty();
}

namespace {
// The saved chat is replayed through this one buffer instead of a heap string sized
// from the "keep N messages" setting. At its maximum that string was 200 * 64 = 12.8 kB
// asked for in a single piece, plus a vector holding every line again. The allocator
// serves a block that big out of core memory and never hands it back, so opening two
// threads was enough to leave less contiguous memory than the on-screen keyboard needs
// (4848 B) and composing a message died with "Out of memory". A fixed tail costs the
// same on every call, and the console keeps only 28 lines anyway.
constexpr size_t HIST_TAIL = 1024;
char hist_buf[HIST_TAIL];
}  // namespace

// Which of our in-flight messages owns the line whose delivery byte sits at `off`.
//
// A replayed line carries no packet id - the history does not store one - so the retry
// counter, which hangs off the line with a matching tag, had nothing to attach itself to
// after a chat switch and simply vanished while the send was still going. The offset is
// the one thing both sides know: save_line_to returns it, and it can be recomputed while
// reading the file back.
uint32_t MeshtasticChatView::tag_at_offset(uint32_t off) const {
    if (!off) return 0;
    for (const auto& m : messages_) {
        if (!m.outgoing || m.status != 1 || !m.packet_id) continue;
        if (m.hist_offset != off) continue;
        if (m.hist_peer != dest_id_ || m.hist_ch != channel_idx_) continue;  // another chat
        return m.packet_id;
    }
    return 0;
}

// A glyph table from the card, so the chat can show an alphabet the firmware font
// does not carry. Absent, unreadable or malformed, nothing happens at all and those
// characters keep their placeholder: the file is an addition, never a requirement.
//
//   "MGF1" | w=8 | h=16 | ranges | flags | ranges * {u16 first, u8 count} | glyphs
//
// Codepoint ranges rather than one built-in script, so the same file serves Cyrillic,
// Greek, Hebrew or the accented Latin letters Latin-1 leaves out, and the choice
// belongs to whoever holds the card. tools/lora_bench/gen_font.py writes it.
//
// The table is allocated only when a file is there and only as large as it claims,
// through chHeapAlloc rather than new: core memory is scarce here, and an optional
// extra has no business panicking the firmware because it could not have its 4 kB.
MeshtasticChatView::~MeshtasticChatView() {
    if (g_chat == this) g_chat = nullptr;
    console_.set_ext_font(nullptr, nullptr, 0);
    if (g_big.data) {
        chHeapFree(g_big.data);
        g_big = GlyphTable{};
    }
    if (g_small.data) {
        chHeapFree(g_small.data);
        g_small = GlyphTable{};
    }
    ext_font_ = nullptr;
}

// One table, from one file, for one face. Absent, unreadable or malformed, nothing
// happens at all and those characters keep their placeholder: the file is an addition,
// never a requirement.
//
//   "MGF1" | w | h | ranges | flags | ranges * {u16 first, u8 count} | glyphs
//
// Codepoint ranges rather than one built-in script, so the same file serves Cyrillic,
// Greek, Hebrew or the accented Latin letters Latin-1 leaves out, and the choice
// belongs to whoever holds the card. tools/lora_bench/gen_font.py writes it.
static bool load_one_table(const char16_t* path, uint8_t want_w, uint8_t want_h, GlyphTable& out, uint32_t max_glyphs, uint32_t keyboard_bytes) {
    if (out.data) return true;
    auto f = std::make_unique<File>();
    if (f->open(path).is_valid()) return false;

    uint8_t head[8 + MeshConsole::MAX_RANGES * 3];
    const auto rd = f->read(head, 8);
    if (!rd || *rd != 8) return false;
    if (memcmp(head, "MGF1", 4) != 0) return false;
    if (head[4] != want_w || head[5] != want_h) return false;
    const uint8_t nranges = head[6];
    if (!nranges || nranges > MeshConsole::MAX_RANGES) return false;
    const size_t rbytes = static_cast<size_t>(nranges) * 3u;
    const auto rr = f->read(head + 8, rbytes);
    if (!rr || *rr != rbytes) return false;

    MeshConsole::GlyphRange ranges[MeshConsole::MAX_RANGES]{};
    uint32_t total = 0;
    for (uint8_t i = 0; i < nranges; i++) {
        const uint8_t* const p = head + 8 + i * 3;
        if (!p[2]) return false;  // an empty run means the file was not written properly
        ranges[i].first = static_cast<uint16_t>(p[0] | (p[1] << 8));
        ranges[i].count = p[2];
        ranges[i].index = static_cast<uint16_t>(total);
        total += p[2];
    }
    if (!total || total > max_glyphs) return false;

    const uint8_t stride = static_cast<uint8_t>((want_w * want_h + 7) / 8);
    const size_t bytes = total * stride;
    // Reading is not worth more than typing: leave the keyboard its room.
    if (chCoreStatus() < bytes + keyboard_bytes + 512u) return false;
    uint8_t* const table = static_cast<uint8_t*>(chHeapAlloc(0x0, bytes));
    if (!table) return false;
    const auto gr = f->read(table, bytes);
    if (!gr || *gr != bytes) {
        chHeapFree(table);
        return false;
    }
    out.data = table;
    out.stride = stride;
    out.nranges = nranges;
    for (uint8_t i = 0; i < nranges; i++) out.ranges[i] = ranges[i];
    return true;
}

void MeshtasticChatView::load_ext_font() {
    if (!display_.glyph_font) return;  // off unless the reader asked for it
    // Both faces, independently: mesh_font.fnt letters the standard 8x16 one and the
    // keyboard, mesh_font5.fnt the compact 5x8 chat. Either may be missing.
    const bool big = load_one_table(u"/APPS/mesh_font.fnt", 8, 16, g_big,
                                    MAX_EXT_GLYPHS, KEYBOARD_BYTES);
    load_one_table(u"/APPS/mesh_font5.fnt", 5, 8, g_small, MAX_EXT_GLYPHS, KEYBOARD_BYTES);
    if (!g_big.data && !g_small.data) {
        ext_font_short_ = true;
        return;
    }
    ext_font_ = g_big.data ? g_big.data : g_small.data;  // so has_ext_font() answers
    console_.set_ext_font(ext_font_, nullptr, 0);
    (void)big;
}

void MeshtasticChatView::load_history() {
    if (!display_.save_count) return;
    auto f = std::make_unique<File>();
    const std::u16string path = history_path();
    if (f->open(path).is_valid()) return;
    const size_t size = f->size();
    if (!size) return;
    const size_t want = std::min<size_t>(size, HIST_TAIL);
    f->seek(size - want);
    const auto rd = f->read(hist_buf, want);
    if (!rd) return;
    const size_t len = *rd;

    // A tail taken from the middle of the file starts inside a line; drop that piece.
    size_t start = 0;
    if (want < size) {
        while (start < len && hist_buf[start] != '\n') start++;
        if (start < len) start++;
    }
    uint32_t shown = 0;
    for (size_t i = start; i < len; i++)
        if (hist_buf[i] == '\n') shown++;
    if (shown)
        console_.write_tech("* history (" + to_string_dec_uint(shown) + ")\n");

    // The counter in the status row is a running total, so seed it from the replayed
    // history: without this it restarts at 1 on every launch while a full chat is
    // already on screen. Status lines start with '*' and are not messages.
    uint32_t replayed = 0;
    std::string l;  // one string, reused by every line
    size_t from = start;
    const size_t base = size - want;  // where this tail sits in the file
    for (size_t i = start; i < len; i++) {
        if (hist_buf[i] != '\n') continue;
        const size_t line_start = from;
        l.assign(hist_buf + from, i - from);
        from = i + 1;
        // Tolerate both line endings, including files written by the older code.
        while (!l.empty() && (l.back() == '\r' || l.back() == '\n')) l.pop_back();
        if (l.empty()) continue;
        // Lines from this version start 0x01 <marker+0x20>; anything else is text as-is,
        // which keeps files written by earlier builds readable.
        if (l.size() > 2 && l[0] == '\x01') {
            // The colour byte comes off the card, so treat it as untrusted: markers
            // only run 0..15 and anything else is drawn plain.
            const uint8_t m = static_cast<uint8_t>(l[1] - 0x20);
            if (l[2] != '*') replayed++;
            // The delivery byte is the second in the record, and its offset is what a
            // pending message remembers - so a line still being retried gets its tag
            // back and the counter finds it again.
            console_.write_tagged(l.substr(2) + "\n", (m < 16) ? m : 0,
                                  tag_at_offset(static_cast<uint32_t>(base + line_start + 1)));
        } else if (l.size() > 2 && l[0] >= 'A' && l[0] <= 'Z' && l[1] == '[') {
            // One build wrote the colour as a letter before the timestamp. Read it back
            // rather than leave a stray capital at the start of every old line.
            replayed++;
            console_.write_tagged(l.substr(1) + "\n",
                                  static_cast<uint8_t>(l[0] - 'A'), 0);
        } else {
            // A line from an older build: drop the "me: " it used to carry so the two
            // formats at least read alike until the old lines age out.
            const size_t me = l.find("] me: ");
            if (me != std::string::npos) l.erase(me + 2, 4);
            if (l[0] != '*') replayed++;
            console_.write(l + "\n");
        }
    }
    if (replayed) {
        total_msgs_ = replayed;
        text_count_.set("m:" + to_string_dec_uint(total_msgs_));
    }

    // The log only ever grows by appending, so fold it back to the window the setting
    // asks for once it has run well past it. Copied a bufferful at a time, because the
    // whole point of the rework above was to stop asking for one huge block.
    const size_t keep = display_.save_count * 64;
    if (size > keep * 4) {
        // Into a second file, not over the one being read: the copy is chunked now, so
        // truncating the original in place would eat the bytes still to be copied.
        const std::u16string tmp = path + u".tmp";
        // Start on a record boundary. Cutting at a round number of bytes left the file
        // beginning inside a line, and that fragment stays there for good: it is
        // dropped while the file is long enough to be read as a tail, and shown raw
        // once it is short enough to be read whole.
        size_t begin = size - keep;
        {
            f->seek(begin);
            const auto got = f->read(hist_buf, std::min<size_t>(HIST_TAIL, size - begin));
            if (!got) return;
            size_t k = 0;
            while (k < *got && hist_buf[k] != '\n') k++;
            if (k < *got) begin += k + 1;
        }
        {
            auto out = std::make_unique<File>();
            if (out->create(tmp).is_valid()) return;
            size_t pos = begin;
            while (pos < size) {
                const size_t n = std::min<size_t>(HIST_TAIL, size - pos);
                f->seek(pos);
                const auto got = f->read(hist_buf, n);
                if (!got || !*got) return;
                // Bail before the original is destroyed. A card with no room left
                // would otherwise leave a short copy standing in for the history.
                if (!out->write(hist_buf, *got)) return;
                pos += *got;
            }
        }
        f.reset();
        delete_file(path);
        rename_file(tmp, path);
        // Every saved position in this conversation just moved. A message still waiting
        // for its delivery result remembers where its byte was, and left alone it would
        // patch that byte at the old place - which is now somebody else's line.
        const uint32_t dropped = static_cast<uint32_t>(begin);
        for (auto& mm : messages_) {
            if (mm.hist_peer != dest_id_ || mm.hist_ch != channel_idx_) continue;
            mm.hist_offset = (mm.hist_offset > dropped) ? mm.hist_offset - dropped : 0;
        }
    }
}

void MeshtasticChatView::apply_dest_label() {
    if (dest_id_ == BROADCAST_ADDR) {
        button_dest_.set_text(">All");
        return;
    }
    const NodeEntry* ne = node_db_.find(dest_id_);
    // A DM to a node whose public key we have is end-to-end encrypted (PKC); mark it
    // so it is obvious which conversations are private.
    const std::string lock = (ne && ne->has_pubkey) ? "*" : "";
    const std::string tag = (ne && ne->long_name[0])
                                ? std::string(ne->long_name).substr(0, 15)
                                : to_string_hex(dest_id_, 8);
    button_dest_.set_text(">" + lock + tag);
}

void MeshtasticChatView::set_dest(uint32_t node_id, bool open_thread) {
    const bool changed = (dest_id_ != node_id);
    dest_id_ = node_id;
    if (dest_id_ == BROADCAST_ADDR) {
        dest_node_idx_ = 0;
    } else {
        // Only an explicit "message this node" opens a thread - merely cycling the
        // ">" button past a node must not fill the conversations list with everyone.
        if (open_thread) node_db_.mark_dm(dest_id_);
        // Keep the ">" cycle button in step with a destination set from elsewhere.
        for (size_t i = 0; i < node_db_.count(); i++) {
            const NodeEntry* e = node_db_.at(i);
            if (e && e->node_id == dest_id_) {
                dest_node_idx_ = static_cast<uint8_t>(i);
                break;
            }
        }
    }
    apply_dest_label();
    update_padlock();
    text_rssi_.hidden(!display_.show_db);
    if (changed) switch_conversation();
}

void MeshtasticChatView::paint(Painter& painter) {
    const auto r = screen_rect();
    painter.fill_rectangle({r.left(), r.top(), screen_width, STATUS_H},
                           Theme::getInstance()->bg_darkest->background);
    // Encryption padlock at the far right. Private (custom channel + passphrase) =
    // GREEN, closed shackle. Not private (public/unencrypted) = RED, and the shackle
    // is flipped open the other way (its LEFT leg lifts instead of the right).
    const Coord px = r.left() + screen_width - 12;
    const Coord py = r.top();
    const bool locked = active_encrypted_;
    const Color c = locked ? Color::green() : Color::red();
    const Color bg = Theme::getInstance()->bg_darkest->background;
    if (locked) {
        painter.fill_rectangle({px + 2, py + 2, 2, 6}, c);  // left leg down
        painter.fill_rectangle({px + 2, py + 1, 6, 2}, c);  // top of arch
        painter.fill_rectangle({px + 6, py + 2, 2, 6}, c);  // right leg down
    } else {
        painter.fill_rectangle({px + 2, py + 0, 2, 5}, c);  // left leg lifted (mirrored)
        painter.fill_rectangle({px + 2, py + 0, 6, 2}, c);  // top raised
        painter.fill_rectangle({px + 6, py + 2, 2, 6}, c);  // right leg down
    }
    // Body + keyhole.
    painter.fill_rectangle({px, py + 7, 10, 8}, c);       // lock body
    painter.fill_rectangle({px + 4, py + 9, 2, 2}, bg);   // keyhole hole
    painter.fill_rectangle({px + 4, py + 10, 1, 3}, bg);  // keyhole slot
}

// Text::paint already inverts its style while the widget holds focus, so the widget
// only has to carry a style worth inverting: white on black inverts to a solid white
// box with black letters. Setting the white background here instead was the bug - the
// inversion then turned it back into dark-on-light, which read as the text going a
// shade paler rather than as a selection.
void RadioModeText::restyle(bool) {
    static const Style plain{
        .font = Theme::getInstance()->fg_light->font,
        .background = Color::black(),
        .foreground = Color::white()};
    set_style(&plain);
    set_dirty();
}

void RadioModeText::on_focus() {
    restyle(true);
}

void RadioModeText::on_blur() {
    restyle(false);
}

bool RadioModeText::on_key(const KeyEvent key) {
    if (key == KeyEvent::Select && on_select) {
        on_select(*this);
        return true;
    }
    // Everything else passes through, the encoder included. A widget that swallowed the
    // encoder would cycle the radio mode instead of letting the selection move on, and
    // there would be no way off it - the same trap the node list and the chat console
    // each had to be dug out of.
    return false;
}

void MeshtasticChatView::init_radio_mode_switch() {
    // Focusable, so the D-pad can reach it. It was touch-only, which on a device with
    // a perfectly good navigation cross is half a control.
    text_pkt_.set_focusable(true);
    // A Text only receives touches once it has a handler, which is exactly what makes
    // the indicator a switch: tap it to cycle both ways -> transmit only -> receive only.
    text_pkt_.on_select = [this](Text&) {
        set_radio_mode(static_cast<uint8_t>((radio_mode_ + 1) % 3));
        write_console(radio_mode_ == 1   ? "* transmit only\n"
                      : radio_mode_ == 2 ? "* receive only, nothing is sent\n"
                                         : "* send and receive\n");
    };
}

void MeshtasticChatView::set_radio_mode(uint8_t m) {
    radio_mode_ = (m > 2) ? 0 : m;
    apply_radio_mode();
    if (on_radio_mode_) on_radio_mode_(radio_mode_);
}

void MeshtasticChatView::apply_radio_mode() {
    // Square brackets say which way the radio is held; they are part of the string, so
    // nothing repaints over them. The inverted block used to mean "locked" and now
    // means "focused" - one appearance for one idea, rather than two ideas sharing it.
    switch (radio_mode_) {
        case 1:
            text_pkt_.set("[TX]");
            break;
        case 2:
            text_pkt_.set("[RX]");
            break;
        default:
            text_pkt_.set(" RX ");
            break;
    }
}

void MeshtasticChatView::set_pkt_indicator(bool tx) {
    if (radio_mode_) return;  // a locked direction stays as it is drawn
    // Only touch the Text widget - a view-level set_dirty() here repaints the
    // Console child too, and Console::paint() re-writes its last line
    // (ui_widget.cpp: paint -> write(buffer)), so every packet/TX event was
    // visually duplicating the newest chat line.
    text_pkt_.set(tx ? " TX " : " RX ");
}

void MeshtasticChatView::on_packet(const MeshPacket& pkt) {
    const std::string from_hex = to_string_hex(pkt.header.from, 8);
    const NodeEntry* ne = node_db_.find(pkt.header.from);
    const std::string from_name = (ne && ne->long_name[0])
                                      ? std::string(ne->long_name)
                                      : from_hex;

    // Chat shows TEXT only. NODEINFO/POSITION/TELEMETRY/ROUTING already update the
    // Nodes + Data tabs via the router dispatch, so keep them out of the chat.
    if (pkt.data.portnum != PortNum::TEXT_MESSAGE) return;

    ChatMessage m;
    m.from_name = from_name;
    m.from_id = pkt.header.from;
    m.text = pkt.text_payload();
    m.timestamp = to_string_datetime(rtc_time::now(), HM);
    m.outgoing = false;

    // Trim trailing CONTROL bytes only (an occasional symbol error mis-reads the
    // protobuf length -> a trailing junk byte that breaks the duplicate check). Do NOT
    // strip high bytes here - UTF-8 Cyrillic/emoji are 0x80-0xFF and must survive; the
    // console decodes them (a truncated multi-byte tail renders as a "." placeholder).
    while (!m.text.empty() && static_cast<uint8_t>(m.text.back()) < 0x20)
        m.text.pop_back();

    // Collapse Meshtastic's reliability re-sends: skip if the same sender+text already
    // appeared among the last few messages.
    // Within a minute of the same text from the same node, and no further: Meshtastic's
    // own re-sends carry the same packet id and the router has already dropped those,
    // so anything reaching here is a repeat the user typed - and typing "ok" twice in a
    // conversation is not a fault. Unbounded, this quietly swallowed real messages,
    // which still went into the store and were still acknowledged: the sender saw a
    // receipt for a message that was never shown.
    m.sent_tick = now_ticks_;
    for (auto it = messages_.rbegin(); it != messages_.rend() && it != messages_.rbegin() + 6; ++it) {
        if (!it->outgoing && it->from_id == m.from_id && it->text == m.text &&
            (now_ticks_ - it->sent_tick) < 3600u)
            return;
    }

    m.to_id = pkt.header.to;
    m.rssi = pkt.rx_rssi;
    m.channel = pkt.channel_index;
    {
        const uint8_t hs = pkt.header.hop_start(), hl = pkt.header.hop_limit();
        m.hops = (hs >= hl) ? static_cast<uint8_t>(hs - hl) : 0;
    }
    if (messages_.size() >= MAX_MESSAGES) messages_.erase(messages_.begin());
    messages_.push_back(m);

    // A text addressed to us alone is a direct message: open/keep its thread and tag
    // the line so it is not mistaken for channel traffic.
    const bool direct = (pkt.header.to == router_.local_node_id());
    if (direct) node_db_.mark_dm(pkt.header.from);

    // Tag messages that arrived on a custom (non-primary) channel.
    const std::string ch_tag = direct
                                   ? std::string("[DM] ")
                                   : ((pkt.channel_index != 0)
                                          ? "<c" + to_string_dec_uint(pkt.channel_index) + "> "
                                          : "");
    const std::string who = peer_label(ne, pkt.header.from);
    const std::string line = stamp() + ch_tag + who + (who.empty() ? "" : ": ") + m.text;
    // A node the user gave a colour gets a square at the left edge, like the delivery
    // dots on our own messages.
    const uint8_t colour = (ne && ne->colour) ? static_cast<uint8_t>(8 + ne->colour) : 0;

    // Each conversation keeps its own history. A message belongs to the thread with its
    // sender when it was addressed to us alone, otherwise to the channel it arrived on.
    // Anything from another conversation is filed there and only announced here, so a
    // channel's history stays that channel's history.
    bool rang = false;  // a bell chimed for this message, so no second sound
    const bool here = direct ? (dest_id_ == pkt.header.from)
                             : (dest_id_ == BROADCAST_ADDR && pkt.channel_index == channel_idx_);
    if (here) {
        console_.set_message_index(messages_.size() - 1);  // tap this line for details
        // Start clean: these flags are raised by anything the console renders, and
        // replaying the saved history at startup raises them too. Left over, they made
        // the first real message of a session claim a bell and unprintable characters
        // that belonged to a line written minutes or days earlier.
        console_.take_unicode_flag();
        console_.take_bell_flag();
        if (colour)
            console_.write_tagged(line + "\n", colour, 0);
        else
            console_.write(line + "\n");
        save_line(line + "\n", colour);
        // The display has glyphs for ASCII and Latin-1 only. Say plainly that some of
        // the text could not be shown rather than leaving a row of dots to puzzle over.
        // Say which of the two it is, because they call for different things. With no
        // table on the card the reader can put one there; with one loaded, the text is
        // simply in an alphabet this file does not carry - and the ones no file can
        // carry are the shaped and the thousand-glyph scripts, not a missing download.
        // Four different situations, and each wants something different from the
        // reader: nothing to do, turn the setting on, find the file, or free some
        // memory. One message for all of them told nobody anything.
        if (console_.take_unicode_flag())
            console_.write_tech(console_.has_ext_font()
                                    ? "* no glyph for some characters\n"
                                : !display_.glyph_font
                                    ? "* glyphs off: Setup > Chat > SD glyphs\n"
                                : ext_font_short_
                                    ? "* glyph table left out: low memory\n"
                                    : "* no glyphs: add APPS/mesh_font.fnt\n");
        // A bell is an attention call, so it chimes on its own terms - the message beep
        // can be off and this still rings, which is the whole point of the character.
        rang = console_.take_bell_flag();
        if (rang) {
            console_.write_tech("* bell from " + who + "\n");
            // One distinct, longer tone rather than a pair: sleeping between two beeps
            // would block the packet handler, and that is not a thread to stall.
            // It rings for either setting - a bell that arrives while only the ordinary
            // chime is on should still sound like a bell, not like every other message.
            if (display_.bell_alert || (beep_flag_ && *beep_flag_))
                baseband::request_audio_beep(1500, 24000, 400);
        }
    } else {
        save_line_to(conversation_path(direct ? pkt.header.from : BROADCAST_ADDR,
                                       pkt.channel_index),
                     line + "\n", colour);
        console_.write_tech("* new message in " +
                            (direct ? std::string("DM ") + who
                                    : "ch" + to_string_dec_uint(pkt.channel_index)) +
                            "\n");
    }
    text_count_.set("m:" + to_string_dec_uint(++total_msgs_));
    // ...and the ordinary chime only when the bell has not already spoken for it.
    if (!rang && beep_flag_ && *beep_flag_)
        baseband::request_audio_beep(880, 24000, 80);  // short chime on a new message
    set_pkt_indicator(false);
}

void MeshtasticChatView::on_ack(uint32_t request_id, bool is_ack) {
    for (auto& m : messages_) {
        if (m.outgoing && m.status == 1 && m.packet_id == request_id) {
            m.status = is_ack ? 2 : 3;  // delivered (green) / failed (red)
            console_.set_marker(request_id, m.status);
            patch_saved_marker(m, m.status);
            return;
        }
    }
}

// Attempts ran out without an acknowledgement. A unicast that nobody answered has
// failed. A broadcast has only gone UNCONFIRMED: nobody in particular owed us an
// answer, and the message may well have arrived - it did, the day this was written,
// while the dot went red anyway. Timing broadcasts out to red was ruled out once
// before, for reading as "TX is broken"; the retry path had quietly brought it back.
void MeshtasticChatView::on_retries_done(uint32_t request_id) {
    for (auto& m : messages_) {
        if (m.outgoing && m.status == 1 && m.packet_id == request_id) {
            // A direct message nobody answered has failed; a broadcast has only gone
            // unconfirmed, and the two deserve different marks.
            m.status = m.expects_ack ? 3 : 4;
            console_.set_marker(request_id, m.status);
            patch_saved_marker(m, m.status);
            return;
        }
    }
}

void MeshtasticChatView::tick(uint32_t now_ticks) {
    now_ticks_ = now_ticks;
    for (auto& m : messages_) {
        // Only a direct (unicast) message times out to red - a broadcast has no
        // single recipient to ACK it, so it stays yellow (or turns green if we
        // hear it re-flooded). Timing broadcasts out to red just looked like "TX
        // is broken".
        // And never while it is still being retried: this clock and the retry counter
        // used to run independently, so a message went red at the second attempt of
        // three while the third was still to come.
        if (m.outgoing && m.status == 1 && m.expects_ack &&
            !(console_.retry_tag() && m.packet_id == console_.retry_tag()) &&
            now_ticks - m.sent_tick > ACK_TIMEOUT_TICKS) {
            m.status = 3;  // no ACK in time -> failed (red)
            console_.set_marker(m.packet_id, 3);
            // ...and write it into the history as well, or the red is forgotten the
            // moment the conversation is reloaded and the line comes back yellow.
            patch_saved_marker(m, 3);
        }
    }
}

// ============================================================================
// MeshtasticNodeDetailView
// ============================================================================

MeshtasticNodeDetailView::MeshtasticNodeDetailView(NavigationView& nav,
                                                   NodeDB& node_db,
                                                   uint32_t node_id,
                                                   std::function<void(uint32_t)> on_message,
                                                   uint32_t now_ticks,
                                                   std::function<void(uint32_t, uint8_t)> on_request_stats)
    : nav_(nav), node_db_(node_db), node_id_(node_id), now_ticks_(now_ticks), on_message_(std::move(on_message)), on_request_stats_(std::move(on_request_stats)) {
    add_children({&opt_page_, &options_colour_, &button_exchange_,
                  &button_msg_, &button_qr_, &button_map_, &button_trace_,
                  &button_stats_, &button_metrics_});
    button_exchange_.on_select = [this](Button&) {
        if (on_exchange_) on_exchange_();
    };
    {  // The entry is looked up again below for the button captions.
        const NodeEntry* ne = node_db_.find(node_id_);
        options_colour_.set_by_value(ne ? ne->colour : 0);
    }
    options_colour_.on_change = [this](size_t, OptionsField::value_t v) {
        NodeEntry* n = node_db_.find(node_id_);
        if (n) n->colour = static_cast<uint8_t>(v);
        set_dirty();  // the swatch beside the picker shows the colour itself
    };
    opt_page_.on_change = [this](size_t, OptionsField::value_t v) {
        page_ = static_cast<uint8_t>(v);
        options_colour_.hidden(page_ != 0);  // the colour belongs to the identity page
        set_dirty();
    };
    // Two questions, two buttons, each landing on the page that will show its answer.
    // Staying on the card is deliberate: the node replies over the air, which takes
    // seconds, and the page fills itself in when the reply arrives.
    auto ask = [this](uint8_t what, uint8_t show_page) {
        if (on_request_stats_) on_request_stats_(node_id_, what);
        opt_page_.set_by_value(show_page);
        page_ = show_page;
        set_dirty();
    };
    button_stats_.on_select = [ask](Button&) { ask(REQ_STATS, 7); };
    button_metrics_.on_select = [ask](Button&) { ask(REQ_METRICS, 1); };

    const NodeEntry* e = node_db_.find(node_id_);
    const bool has_pos = e && e->has_position &&
                         (e->position.latitude != 0.0 || e->position.longitude != 0.0);
    // Nothing to point at is not an error, so the button says what it has rather than
    // disappearing: a node that has never sent a position still gets a Trace beside it.
    button_map_.set_text(has_pos ? "Map" : "No pos");
    button_map_.on_select = [this](Button&) { show_on_map(); };
    button_trace_.on_select = [this](Button&) {
        if (on_request_stats_) on_request_stats_(node_id_, REQ_TRACE);
    };

    // A direct message is end-to-end encrypted when we know the node's public key
    // (PKC); otherwise it still goes out unicast, encrypted with the channel key.
    // A node that says nobody reads messages on it gets told so rather than offered a
    // conversation - the same courtesy the phone app extends. Writing still works if
    // you insist; the flag is advisory.
    button_msg_.set_text((e && e->unmessagable) ? "Message (unmonitored)"
                         : (e && e->has_pubkey) ? "Message (encrypted)"
                                                : "Message");
    button_qr_.on_select = [this](Button&) { show_qr(); };
    button_msg_.on_select = [this](Button&) {
        if (!on_message_) return;
        on_message_(node_id_);  // owner points the chat at this node and shows it
        nav_.pop();             // back to the tabs, now on the Chat tab
    };
}

void MeshtasticNodeDetailView::focus() {
    opt_page_.focus();
}

void MeshtasticNodeDetailView::show_qr() {
    const NodeEntry* e = node_db_.find(node_id_);
    if (!e) return;
    // The standard contact URL: a phone that scans it gets the node and its public
    // key in one go, which is what enables an encrypted direct message to it.
    qr_url_ = meshtastic::contact_share_url(e->node_id, e->long_name, e->short_name,
                                            e->public_key, e->has_pubkey);
    std::string l1 = e->long_name[0] ? std::string(e->long_name)
                                     : "!" + to_string_hex(node_id_, 8);
    std::string l2;
    {
        const char* hw = hw_model_name(e->hw_model);
        if (hw) l2 = hw;
        const char* rl = mesh_role_name(e->role);
        if (rl && e->role) l2 += (l2.empty() ? "" : ", ") + std::string(rl);
        if (!l2.empty()) l2 += "  ";
        l2 += e->has_pubkey ? "[key]" : "[no key]";
    }
    nav_.push<MeshtasticQRView>(qr_url_, l1, l2);
}

void MeshtasticNodeDetailView::show_on_map() {
    const NodeEntry* e = node_db_.find(node_id_);
    if (!e || !e->has_position) return;
    if (e->position.latitude == 0.0 && e->position.longitude == 0.0) return;

    const float lat = static_cast<float>(e->position.latitude);
    const float lon = static_cast<float>(e->position.longitude);
    const std::string tag = e->long_name[0]
                                ? std::string(e->long_name).substr(0, 10)
                                : to_string_hex(e->node_id, 8);

    if (!map_would_fit(nullptr)) return;
    auto* map = nav_.push<GeoMapView>(
        tag, e->position.altitude_m,
        GeoPos::alt_unit::METERS, GeoPos::spd_unit::HIDDEN,
        lat, lon, 999.0f, []() { map_reserve_restore(); });
    map->clear_markers();
    GeoMarker m;
    m.lat = lat;
    m.lon = lon;
    m.angle = 400;  // static marker (no bearing arrow)
    m.tag = tag;
    m.color = Color::green();
    map->store_marker(m);
}

// The page's own name, for the line a page shows when it has nothing else. Kept next
// to the paint routine rather than beside the page list: the list is what the picker
// reads and carries the "n/8" numbering with it, which does not belong in a row.
static const char* page_title(uint8_t page) {
    switch (page) {
        case 0:
            return "Identity";
        case 1:
            return "Radio";
        case 2:
            return "Environ";
        case 3:
            return "Weather";
        case 4:
            return "Air";
        case 5:
            return "Power";
        case 6:
            return "Health";
        case 8:
            return "Route";
        default:
            return "Stats";
    }
}

void MeshtasticNodeDetailView::paint(Painter& painter) {
    const NodeEntry* e = node_db_.find(node_id_);
    const auto& fg = *Theme::getInstance()->fg_light;
    const auto& dim = *Theme::getInstance()->fg_dark;
    const auto r = screen_rect();
    const int x0 = r.left();
    const int LH = 18;
    // Down to the first of the fixed buttons, not two rows short of it. "Exchange
    // info" belongs to the identity page and is hidden on the others, but hiding a
    // widget does not erase it - and the fill stopped inside its last two rows, so its
    // bottom edge stayed on screen as a grey bar under every other page.
    painter.fill_rectangle({x0, r.top() + 26, screen_width, 304 - 26 - 108},
                           Theme::getInstance()->bg_darkest->background);
    if (!e) return;
    Coord y = r.top() + 30;
    const Coord y_first = y;

    // Field captions in the same medium grey the settings screens use: the darker one
    // sank into the background and read as disabled text.
    const auto& cap = *Theme::getInstance()->fg_medium;
    auto row = [&](const char* label, const std::string& val) {
        painter.draw_string({x0, y}, cap, label);
        painter.draw_string({x0 + 11 * 8, y}, fg, val.substr(0, 19));
        y += LH;
    };
    auto f1 = [](float v) -> std::string {  // one decimal, no printf
        const bool neg = v < 0;
        float a = neg ? -v : v;
        const int w = static_cast<int>(a);
        const int f = static_cast<int>((a - w) * 10.0f + 0.5f);
        return (neg ? "-" : "") + to_string_dec_int(w) + "." +
               to_string_dec_uint(static_cast<uint32_t>(f));
    };
    auto age = [&](uint32_t tick) -> std::string {
        if (!tick) return "--";
        // The card takes its clock once, when it opens. Anything stamped after that -
        // a route traced from the button below, say - is in the future by that clock,
        // and reading "--" made a fresh entry look like a broken one.
        if (now_ticks_ < tick) return "now";
        const uint32_t s2 = (now_ticks_ - tick) / 60;
        if (s2 < 60) return to_string_dec_uint(s2) + "s ago";
        if (s2 < 3600) return to_string_dec_uint(s2 / 60) + "m ago";
        return to_string_dec_uint(s2 / 3600) + "h ago";
    };
    // "47s" rather than "47s ago", for the lines that carry two of them.
    auto age_short = [&](uint32_t tick) -> std::string {
        const std::string a = age(tick);
        return a.substr(0, a.find(' '));
    };
    auto dur = [](uint32_t sec) -> std::string {
        if (sec < 3600) return to_string_dec_uint(sec / 60) + "m";
        if (sec < 86400) return to_string_dec_uint(sec / 3600) + "h";
        return to_string_dec_uint(sec / 86400) + "d";
    };

    if (page_ == 0) {
        // Captioned like every other row, because it now sits in the rows rather than
        // beside a button. The picker names the colour; the swatch shows it, so it can
        // be matched against the stripe in the chat and in the node list without
        // having to guess what "cyan" looks like.
        painter.draw_string({x0, r.top() + 178}, *Theme::getInstance()->fg_medium, "Colour");
        painter.fill_rectangle({x0 + 20 * 8, r.top() + 178, 20, 12},
                               e->colour ? node_palette(e->colour)
                                         : Theme::getInstance()->bg_darkest->background);
    }

    switch (page_) {
        case 0:  // identity
            // Meshtastic writes a node number as eight hex digits behind an exclamation
            // mark. Label it, or the line reads like an error rather than an address.
            row("Node ID", "!" + to_string_hex(e->node_id, 8));
            row("Name", e->long_name[0] ? std::string(e->long_name) : "(unknown)");
            row("Short", e->short_name[0] ? std::string(e->short_name) : "?");
            {
                const char* hw = hw_model_name(e->hw_model);
                if (hw)
                    row("Device", hw);
                else if (e->hw_model)
                    row("Device", "#" + to_string_dec_uint(e->hw_model));
                // Shown even when it is the ordinary Client. Hiding the commonest
                // role meant the line was missing from almost every node, which reads
                // as "we do not know" rather than "it is a plain client".
                // Role, and the node's own advisory that nobody reads messages on it,
                // share a line. Adding Role pushed the second line of the public key
                // down onto the colour picker, and a separate line for the advisory
                // would put it straight back: the page has exactly eight rows to give.
                const char* rl = mesh_role_name(e->role);
                if (rl)
                    row("Role", std::string(rl) + (e->unmessagable ? ", unread" : ""));
            }
            row("Heard", age_short(e->last_seen_uptime) + ", first " +
                             age_short(e->first_seen_uptime));
            if (e->has_pubkey) {
                const std::string b64 = meshtastic::base64_encode(e->public_key, 32);
                row("Key", b64.substr(0, 19));
                row("", b64.substr(19, 19));
            } else {
                row("Key", "(none, no PKC)");
            }
            break;
        case 1:  // radio and device metrics
            row("RSSI", to_string_dec_int(e->last_rssi) + " dBm");
            row("SNR", f1(e->last_snr) + " dB");
            row("Hops", to_string_dec_uint(e->hop_count));
            row("Battery", e->battery_level < 0 ? "--"
                                                : (e->battery_level > 100 ? std::string("charging")
                                                                          : to_string_dec_uint(static_cast<uint32_t>(e->battery_level)) + "%"));
            if (e->voltage >= 0) row("Voltage", f1(e->voltage) + " V");
            if (e->uptime_seconds) row("Uptime", dur(e->uptime_seconds));
            if (e->channel_utilization >= 0) row("Ch util", f1(e->channel_utilization) + " %");
            if (e->air_util_tx >= 0) row("Air TX", f1(e->air_util_tx) + " %");
            // From its NeighborInfo: how many nodes it hears directly, and - the useful
            // half - what it hears US at. A link is often good one way and poor the
            // other, and nothing we can measure here would ever show that.
            if (e->nbr_count) row("Neighbors", to_string_dec_uint(e->nbr_count));
            if (e->nbr_hears_us) row("Hears us", f1(e->nbr_snr_to_us) + " dB");
            break;
        case 2:  // environment and position
            if (e->temperature > -900) row("Temp", f1(e->temperature) + " C");
            if (e->humidity >= 0) row("Humidity", f1(e->humidity) + " %");
            if (e->pressure >= 0) row("Pressure", f1(e->pressure) + " hPa");
            if (e->gas_resistance >= 0) row("Gas", f1(e->gas_resistance));
            if (e->lux >= 0) row("Light", f1(e->lux) + " lx");
            if (e->iaq) row("IAQ", to_string_dec_uint(e->iaq));
            // Coordinates used to be listed here too, which made no sense: a position is
            // not an environment reading. It has the Data tab and the "Show on Map"
            // button below, both of which show it properly.
            break;
        case 3: {  // weather and ground sensors
            // These come straight from the last telemetry packet: keeping every field
            // of telemetry.proto for all sixteen nodes would not fit in memory, so the
            // detail pages show the newest packet and say so when it is someone else's.
            const auto& tl = node_db_.last_telemetry();
            if (node_db_.last_telemetry_node() != e->node_id) {
                break;
            }
            bool any = false;
            auto opt = [&](const char* label, bool present, const std::string& v) {
                if (!present) return;
                row(label, v);
                any = true;
            };
            opt("Wind", tl.wind_speed >= 0, f1(tl.wind_speed) + " m/s");
            opt("Gust", tl.wind_gust >= 0, f1(tl.wind_gust) + " m/s");
            opt("Lull", tl.wind_lull >= 0, f1(tl.wind_lull) + " m/s");
            opt("Direction", tl.wind_direction <= 360,
                to_string_dec_uint(tl.wind_direction) + " deg");
            opt("Rain 1h", tl.rainfall_1h >= 0, f1(tl.rainfall_1h) + " mm");
            opt("Rain 24h", tl.rainfall_24h >= 0, f1(tl.rainfall_24h) + " mm");
            opt("Soil wet", tl.soil_moisture <= 100,
                to_string_dec_uint(tl.soil_moisture) + " %");
            opt("Soil temp", tl.soil_temperature > -900, f1(tl.soil_temperature) + " C");
            opt("Radiation", tl.radiation >= 0, f1(tl.radiation) + " uR/h");
            opt("Weight", tl.weight >= 0, f1(tl.weight) + " kg");
            opt("Distance", tl.distance >= 0, f1(tl.distance) + " mm");
            opt("UV", tl.uv_lux >= 0, f1(tl.uv_lux) + " lx");
            opt("IR", tl.ir_lux >= 0, f1(tl.ir_lux) + " lx");
            opt("White", tl.white_lux >= 0, f1(tl.white_lux) + " lx");
            opt("1-Wire", tl.one_wire_temperature > -900,
                f1(tl.one_wire_temperature) + " C");
            break;
        }
        case 4:  // air quality
            if (!e->has_air_data) {
                break;
            }
            row("PM2.5", to_string_dec_uint(e->pm25) + " ug/m3");
            row("PM10", to_string_dec_uint(e->pm10) + " ug/m3");
            if (e->co2) row("CO2", to_string_dec_uint(e->co2) + " ppm");
            if (node_db_.last_telemetry_node() == e->node_id) {
                const auto& tl = node_db_.last_telemetry();
                if (tl.pm1_std) row("PM1.0", to_string_dec_uint(tl.pm1_std) + " ug/m3");
                if (tl.pm40_std) row("PM4.0", to_string_dec_uint(tl.pm40_std) + " ug/m3");
                if (tl.particles_03) row("0.3um", to_string_dec_uint(tl.particles_03));
                if (tl.particles_25) row("2.5um", to_string_dec_uint(tl.particles_25));
                if (tl.particles_100) row("10um", to_string_dec_uint(tl.particles_100));
                if (tl.particles_tps >= 0) row("Typ size", f1(tl.particles_tps) + " um");
                if (tl.formaldehyde >= 0) row("HCHO", f1(tl.formaldehyde) + " ppb");
                if (tl.pm_voc_idx >= 0) row("VOC idx", f1(tl.pm_voc_idx));
                if (tl.pm_nox_idx >= 0) row("NOx idx", f1(tl.pm_nox_idx));
                if (tl.co2_temperature > -900) row("CO2 temp", f1(tl.co2_temperature) + " C");
            }
            break;
        case 5: {  // power channels
            const auto& tl = node_db_.last_telemetry();
            if (node_db_.last_telemetry_node() != e->node_id || !tl.has_power) {
                break;
            }
            for (int c = 0; c < 8; c++) {
                if (tl.ch_voltage[c] < 0 && tl.ch_current[c] < 0) continue;
                row(("Ch" + to_string_dec_uint(c + 1)).c_str(),
                    f1(tl.ch_voltage[c]) + " V  " + f1(tl.ch_current[c]) + " mA");
            }
            break;
        }
        case 6: {  // wearable sensors, and host metrics
            const auto& tl = node_db_.last_telemetry();
            if (node_db_.last_telemetry_node() != e->node_id) {
                break;
            }
            if (tl.has_health) {
                if (tl.heart_bpm) row("Pulse", to_string_dec_uint(tl.heart_bpm) + " bpm");
                if (tl.spo2) row("SpO2", to_string_dec_uint(tl.spo2) + " %");
                if (tl.body_temperature > -900) row("Body", f1(tl.body_temperature) + " C");
            } else if (tl.has_host) {
                row("Host up", dur(tl.host_uptime));
                row("Free mem", to_string_dec_uint(tl.freemem / 1024) + " KB");
                row("Disk 1", to_string_dec_uint(tl.diskfree1 / 1048576) + " MB");
                row("Load", to_string_dec_uint(tl.load1) + "/" +
                                to_string_dec_uint(tl.load5) + "/" +
                                to_string_dec_uint(tl.load15));
            } else if (tl.has_traffic) {
                row("Inspected", to_string_dec_uint(tl.packets_inspected));
                row("Pos dedup", to_string_dec_uint(tl.position_dedup_drops));
                row("Cache hit", to_string_dec_uint(tl.nodeinfo_cache_hits));
                row("Rate drop", to_string_dec_uint(tl.rate_limit_drops));
                row("Unknown", to_string_dec_uint(tl.unknown_packet_drops));
                row("Hops out", to_string_dec_uint(tl.hop_exhausted));
            }
            break;
        }
        case 8: {  // the traceroute journal
            // Every trace we have an answer for, newest first, not just this node's:
            // a route is read against its neighbours, and one line per node is what
            // fits. A star marks the node whose card this is.
            for (size_t i = 0; i < node_db_.route_count(); i++) {
                const auto& rt = node_db_.route(i);
                std::string path;
                for (uint8_t h = 0; h < rt.hop_count; h++)
                    path += (h ? " " : "") + to_string_hex(rt.hops[h], 4);
                const std::string when = age_short(rt.when);
                const std::string lbl = to_string_hex(rt.node_id & 0xFFFF, 4) +
                                        (rt.node_id == e->node_id ? "*" : " ") + " " + when;
                row(lbl.c_str(), path.empty() ? "direct" : path);
            }
            break;
        }
        default: {  // LocalStats
            // Only the newest answer is kept, whoever it came from.
            const auto& st = node_db_.last_stats();
            if (!e->has_stats || st.node_id != e->node_id) {
                break;
            }
            row("Uptime", dur(st.uptime));
            row("TX pkts", to_string_dec_uint(st.packets_tx));
            row("RX pkts", to_string_dec_uint(st.packets_rx));
            row("RX bad", to_string_dec_uint(st.packets_rx_bad));
            row("RX dupe", to_string_dec_uint(st.rx_dupe));
            row("Relayed", to_string_dec_uint(st.tx_relay));
            row("Nodes", to_string_dec_uint(st.nodes_online) + " of " +
                             to_string_dec_uint(st.nodes_total));
            if (st.heap_total)
                row("Heap free", to_string_dec_uint(st.heap_free / 1024) + " KB");
            {
                const auto& tl = node_db_.last_telemetry();
                if (node_db_.last_telemetry_node() == e->node_id) {
                    if (tl.tx_relay_canceled)
                        row("Relay cxl", to_string_dec_uint(tl.tx_relay_canceled));
                    if (tl.tx_dropped)
                        row("TX drop", to_string_dec_uint(tl.tx_dropped));
                    if (tl.has_noise)
                        row("Noise", to_string_dec_int(tl.noise_floor) + " dBm");
                }
            }
            break;
        }
    }
    // A page with nothing on it looks broken rather than empty, and any page can be
    // blank: a node that has never sent weather has no weather to show. Said once,
    // here, in the same words and the same colours as a real row - six pages used to
    // each invent their own phrase, and the seams showed.
    if (y == y_first) row(page_title(page_), "not reported");
}

MeshtasticMessageView::MeshtasticMessageView(NavigationView& nav, const ChatMessage& msg, std::function<void(std::string, uint32_t)> on_resend)
    : nav_(nav), msg_(msg), on_resend_(std::move(on_resend)) {
    add_children({&button_resend_});
    // Ours can go out again as it is; a peer's message turns the button into a reply,
    // which opens the keyboard already addressed to the sender.
    button_resend_.set_text(msg_.outgoing ? "Resend" : "Reply");
    button_resend_.on_select = [this](Button&) {
        if (on_resend_) {
            if (msg_.outgoing)
                on_resend_(msg_.text, msg_.to_id);
            else
                on_resend_(std::string(), msg_.from_id);
        }
        nav_.pop();
    };
}

void MeshtasticMessageView::focus() {
    button_resend_.focus();
}

void MeshtasticMessageView::paint(Painter& painter) {
    const auto& fg = *Theme::getInstance()->fg_light;
    const auto& dim = *Theme::getInstance()->fg_dark;
    const auto r = screen_rect();
    const int x0 = r.left();
    Coord y = r.top() + 8;
    painter.fill_rectangle({x0, r.top(), screen_width, r.height()},
                           Theme::getInstance()->bg_darkest->background);

    auto row = [&](const char* label, const std::string& val) {
        painter.draw_string({x0, y}, dim, label);
        painter.draw_string({x0 + 9 * 8, y}, fg, val.substr(0, 21));
        y += 18;
    };
    static const char* STATUS[4] = {"-", "pending", "delivered", "no ack"};

    row("From", msg_.outgoing ? std::string("me") : msg_.from_name);
    if (!msg_.outgoing) row("Node", "!" + to_string_hex(msg_.from_id, 8));
    row("To", msg_.to_id == meshtastic::BROADCAST_ADDR
                  ? std::string("everyone")
                  : "!" + to_string_hex(msg_.to_id, 8));
    row("Time", msg_.timestamp);
    row("Status", STATUS[msg_.status < 4 ? msg_.status : 0]);
    if (msg_.packet_id) row("Packet", to_string_hex(msg_.packet_id, 8));
    row("Length", to_string_dec_uint(static_cast<uint32_t>(msg_.text.size())) + " chars");
    if (!msg_.outgoing) {
        row("Signal", to_string_dec_int(msg_.rssi) + " dBm");
        row("Hops", to_string_dec_uint(msg_.hops));
        row("Channel", msg_.channel == 0 ? std::string("primary")
                                         : to_string_dec_uint(msg_.channel));
    }
    y += 6;
    painter.draw_string({x0, y}, dim, "Text");
    y += 18;
    // Wrap the body over the remaining rows (30 characters fit across the screen).
    const std::string& t = msg_.text;
    for (size_t i = 0; i < t.size() && y < r.top() + 240; i += 30, y += 18)
        painter.draw_string({x0, y}, fg, t.substr(i, 30));
}

MeshtasticNodesConfigView::MeshtasticNodesConfigView(NavigationView& nav,
                                                     uint32_t& offline_after_s,
                                                     uint32_t& forget_after_min)
    : nav_(nav), offline_after_s_(offline_after_s), forget_after_min_(forget_after_min) {
    add_children({&label_off_, &field_off_, &label_forget_, &field_forget_, &label_hint_});
    field_off_.set_value(static_cast<int32_t>(offline_after_s_ / 60));
    field_forget_.set_value(static_cast<int32_t>(forget_after_min_));
    field_off_.on_change = [this](int32_t v) { offline_after_s_ = static_cast<uint32_t>(v) * 60; };
    field_forget_.on_change = [this](int32_t v) { forget_after_min_ = static_cast<uint32_t>(v); };
}

void MeshtasticNodesConfigView::focus() {
    field_off_.focus();
}

// ============================================================================
// MeshtasticNodesView
// ============================================================================

MeshtasticNodesView::MeshtasticNodesView(NavigationView& nav,
                                         NodeDB& node_db,
                                         Rect parent_rect)
    : View(parent_rect), nav_(nav), node_db_(node_db) {
    add_children({&button_clear_, &button_sort_, &button_setup_});
    // The list itself also takes focus so the encoder / D-pad move the selection and
    // Select opens the node detail.
    set_focusable(true);

    button_clear_.on_select = [this](Button&) {
        node_db_.clear();
        selected_ = scroll_offset_ = 0;
        set_dirty();
    };
    button_sort_.on_select = [this](Button&) { cycle_sort(); };
    update_sort_caption();
    button_setup_.on_select = [this](Button&) {
        nav_.push<MeshtasticNodesConfigView>(offline_after_s_, forget_after_min_);
    };
}

size_t MeshtasticNodesView::online_count() const {
    size_t n = 0;
    const uint32_t limit = offline_after_s_ * 60;  // ~60 Hz ticks
    for (size_t i = 0; i < node_db_.count(); i++) {
        const NodeEntry* e = node_db_.at(i);
        if (e && e->active && e->last_seen_uptime &&
            now_ticks_ >= e->last_seen_uptime &&
            (now_ticks_ - e->last_seen_uptime) <= limit) n++;
    }
    return n;
}

void MeshtasticNodesView::rebuild_order() {
    order_.clear();
    const uint32_t forget_ticks = forget_after_min_ * 3600u;  // ~60 Hz ticks per second
    for (size_t i = 0; i < node_db_.count() && i < 255; i++) {
        NodeEntry* e = node_db_.at(i);
        if (!e || !e->active) continue;
        // Auto-forget: a node that has been silent past the limit drops off the list.
        if (forget_ticks && e->last_seen_uptime && now_ticks_ > e->last_seen_uptime &&
            (now_ticks_ - e->last_seen_uptime) > forget_ticks) {
            e->active = false;
            continue;
        }
        order_.push_back(static_cast<uint8_t>(i));
    }
    auto key_less = [this](uint8_t ia, uint8_t ib) {
        const NodeEntry* a = node_db_.at(ia);
        const NodeEntry* b = node_db_.at(ib);
        if (!a || !b) return false;
        switch (sort_) {
            case Sort::Id:
                return a->node_id < b->node_id;
            case Sort::Name: {
                const std::string na = a->long_name[0] ? a->long_name : "";
                const std::string nb = b->long_name[0] ? b->long_name : "";
                return na < nb;
            }
            case Sort::Signal:
                return a->last_rssi < b->last_rssi;
            default:
                return a->last_seen_uptime < b->last_seen_uptime;
        }
    };
    // Small n (50 nodes max): an insertion sort keeps the code (and the flash) small.
    for (size_t i = 1; i < order_.size(); i++) {
        const uint8_t v = order_[i];
        size_t j = i;
        while (j > 0 && (sort_desc_ ? key_less(order_[j - 1], v) : key_less(v, order_[j - 1]))) {
            order_[j] = order_[j - 1];
            j--;
        }
        order_[j] = v;
    }
}

void MeshtasticNodesView::update_sort_caption() {
    static const char* const names[4] = {"ID", "Name", "Sig", "Age"};
    button_sort_.set_text(std::string(names[static_cast<uint8_t>(sort_) & 3u]) +
                          (sort_desc_ ? " v" : " ^"));
}

void MeshtasticNodesView::toggle_sort(Sort key) {
    if (sort_ == key) {
        sort_desc_ = !sort_desc_;  // same column again -> flip the direction
    } else {
        sort_ = key;
        sort_desc_ = true;
    }
    selected_ = scroll_offset_ = 0;
    update_sort_caption();
    set_dirty();
}

// One button has to reach all eight states, so it walks them in order: the column
// descending, then ascending, then on to the next column. Tapping a header title still
// jumps straight to that column for anyone who found that first.
void MeshtasticNodesView::cycle_sort() {
    if (sort_desc_) {
        sort_desc_ = false;
    } else {
        sort_ = static_cast<Sort>((static_cast<uint8_t>(sort_) + 1u) & 3u);
        sort_desc_ = true;
    }
    selected_ = scroll_offset_ = 0;
    update_sort_caption();
    set_dirty();
}

void MeshtasticNodesView::focus() {
    Widget::focus();
}

void MeshtasticNodesView::move_selection(int delta) {
    const size_t total = order_.size();
    if (total == 0) return;
    int sel = static_cast<int>(selected_) + delta;
    if (sel < 0) sel = 0;
    if (sel >= static_cast<int>(total)) sel = static_cast<int>(total) - 1;
    if (static_cast<size_t>(sel) == selected_) return;  // at a boundary -> no repaint
    selected_ = static_cast<size_t>(sel);
    // Keep the selection on-screen.
    if (selected_ < scroll_offset_)
        scroll_offset_ = selected_;
    else if (selected_ >= scroll_offset_ + VISIBLE)
        scroll_offset_ = selected_ - VISIBLE + 1;
    set_dirty();
}

void MeshtasticNodesView::open_selected() {
    if (selected_ >= order_.size()) return;
    const NodeEntry* e = node_db_.at(order_[selected_]);
    if (!e || !e->active) return;
    auto* v = nav_.push<MeshtasticNodeDetailView>(node_db_, e->node_id, on_message_,
                                                  now_ticks_, on_stats_);
    v->set_on_exchange(on_exchange_);
}

bool MeshtasticNodesView::on_encoder(const EncoderEvent delta) {
    if (delta == 0) return true;
    if (delta > 0 && selected_ + 1 >= order_.size()) return false;  // out to the buttons
    // At the top of the list, scrolling further up releases focus so the tab bar
    // (and thus leaving the Nodes list) is reachable again - otherwise the list
    // traps the encoder and you can never navigate away.
    if (delta < 0 && selected_ == 0) return false;
    move_selection(delta > 0 ? 1 : -1);
    return true;
}

bool MeshtasticNodesView::on_key(const KeyEvent key) {
    // The D-pad crosses the list in one press instead of walking it a row at a time -
    // the same as the chat, and for the same reason: getting from the tabs to the
    // buttons underneath should not mean pressing Down once per node. Scrolling is the
    // encoder's job; Select still opens whatever the encoder left highlighted.
    if (key == KeyEvent::Select) {
        open_selected();
        return true;
    }
    return false;
}

bool MeshtasticNodesView::on_touch(const TouchEvent event) {
    if (event.type != TouchEvent::Type::End) return true;  // swallow press/move
    const auto r = screen_rect();
    const int rel = event.point.y() - r.top();
    if (rel < HDR_H) {
        // Header row: each title owns a slice of the width.
        const int x = event.point.x() - r.left();
        if (x < 5 * 8)
            toggle_sort(Sort::Id);
        else if (x < 15 * 8)
            toggle_sort(Sort::Name);
        else if (x < 20 * 8)
            toggle_sort(Sort::Signal);
        else
            toggle_sort(Sort::Age);
        return true;
    }
    const int list_rel = rel - HDR_H;
    if (list_rel >= 0 && list_rel < VISIBLE * ROW_H) {
        const size_t row = scroll_offset_ + static_cast<size_t>(list_rel / ROW_H);
        if (row < order_.size()) {
            selected_ = row;
            set_dirty();
            open_selected();
        }
    }
    return true;
}

void MeshtasticNodesView::paint(Painter& painter) {
    const auto& bg = Theme::getInstance()->bg_darkest->background;
    const auto& hdr = *Theme::getInstance()->fg_light;
    const auto r = screen_rect();
    const int x0 = r.left();
    const int y0 = r.top();

    painter.fill_rectangle({x0, y0, screen_width, SV_H}, bg);

    // Header: tapping a title sorts by it, and the arrow shows the direction.
    auto arrow = [this](Sort key) -> const char* {
        if (sort_ != key) return " ";
        return sort_desc_ ? "\x19" : "\x18";  // down / up
    };
    painter.draw_string({x0, y0}, hdr,
                        "ID" + std::string(arrow(Sort::Id)) + "  Name" + arrow(Sort::Name) +
                            "          dBm" + arrow(Sort::Signal) + " Age" + arrow(Sort::Age));
    painter.draw_hline({x0, y0 + HDR_H - 2}, screen_width, hdr.foreground);

    // "12s" / "5m" / "3h" since a node was last heard.
    auto fmt_age = [this](uint32_t seen) -> std::string {
        if (seen == 0 || now_ticks_ < seen) return "--";
        uint32_t s2 = (now_ticks_ - seen) / 60;  // ~60 Hz ticks -> seconds
        if (s2 < 60) return to_string_dec_uint(s2) + "s";
        if (s2 < 3600) return to_string_dec_uint(s2 / 60) + "m";
        return to_string_dec_uint(s2 / 3600) + "h";
    };

    rebuild_order();
    for (size_t i = 0; i < static_cast<size_t>(VISIBLE); i++) {
        const size_t pos = i + scroll_offset_;
        if (pos >= order_.size()) break;
        const NodeEntry* e = node_db_.at(order_[pos]);
        if (!e) continue;

        // The highlight marks where the encoder is, so it appears once the list has
        // focus. Drawn unconditionally it covered the first node from the start, hiding
        // the very colour that ties it to its messages.
        const bool sel = (pos == selected_) && has_focus();
        const int y = y0 + HDR_H + static_cast<int>(i) * ROW_H;
        painter.fill_rectangle({x0, y, screen_width, ROW_H}, sel ? Color::blue() : bg);
        const Style st = sel ? Style{hdr.font, Color::blue(), Color::white()} : hdr;

        std::string id = to_string_hex(e->node_id, 8).substr(4, 4);
        std::string name = e->long_name[0] ? std::string(e->long_name)
                                           : (e->short_name[0] ? std::string(e->short_name) : std::string());
        // Fourteen characters, not nine: the two number columns were sitting well
        // short of the right edge, and a name is the thing you actually read.
        name = name.substr(0, 14);
        name.resize(14, ' ');
        std::string sig = (e->last_rssi != 0) ? to_string_dec_int(e->last_rssi) : "--";
        sig.resize(4, ' ');
        painter.draw_string({x0, y}, st, id);
        painter.draw_string({x0 + 5 * 8, y}, st,
                            name + " " + sig + " " + fmt_age(e->last_seen_uptime));
        // The node's colour as a stripe down the right edge - the same mark its
        // messages carry in the chat. Beside the text rather than in it, so a dark
        // colour never costs legibility.
        if (e->colour)
            painter.fill_rectangle({x0 + screen_width - 6, y + 1, 5, ROW_H - 2},
                                   node_palette(e->colour));
    }

    // The ceiling is stated, not implied. The database is a fixed array sized against
    // the M0's core memory, and the thirteenth node does not queue - it evicts the one
    // heard longest ago. Better that it says 10/10 than that someone works out for
    // themselves why a neighbour keeps vanishing.
    painter.draw_string({x0, y0 + FOOT_Y}, hdr,
                        "Nodes:" + to_string_dec_uint(static_cast<uint32_t>(order_.size())) +
                            "/" + to_string_dec_uint(static_cast<uint32_t>(meshtastic::MAX_NODES)) +
                            "  Online:" + to_string_dec_uint(static_cast<uint32_t>(online_count())));
}

void MeshtasticNodesView::refresh() {
    // Throttle RX-driven repaints (~4/s) so a burst of packets doesn't stutter
    // encoder scrolling; the selection move repaints immediately on its own.
    if (now_ticks_ - last_refresh_tick_ < 15) return;
    last_refresh_tick_ = now_ticks_;
    set_dirty();
}

// ============================================================================
// MeshtasticMapView
// ============================================================================

MeshtasticMapView::MeshtasticMapView(NavigationView& nav,
                                     NodeDB& node_db,
                                     Rect parent_rect)
    : View(parent_rect), nav_(nav), node_db_(node_db) {
    add_children({&text_batt_, &text_util_, &text_up_,
                  &button_open_map_, &button_telemetry_, &button_resend_,
                  &button_resend_pos_});

    button_telemetry_.on_select = [this](Button&) {
        if (on_telemetry_) on_telemetry_();
    };
    button_resend_pos_.on_select = [this](Button&) {
        if (on_resend_pos_) on_resend_pos_();
    };
    button_resend_.on_select = [this](Button&) {
        if (on_resend_) on_resend_();
    };

    button_open_map_.on_select = [this](Button&) {
        // Center on our own fix if we have one, else on the first node with a
        // position (so the map isn't stranded at 0,0 when GPS has no fix yet).
        float center_lat = lat_, center_lon = lon_;
        if (!has_fix_) {
            const size_t total = node_db_.count();
            for (size_t i = 0; i < total; i++) {
                const NodeEntry* e = node_db_.at(i);
                if (e && e->active && e->has_position &&
                    (e->position.latitude != 0.0 || e->position.longitude != 0.0)) {
                    center_lat = static_cast<float>(e->position.latitude);
                    center_lon = static_cast<float>(e->position.longitude);
                    break;
                }
            }
        }

        if (!map_would_fit(nullptr)) return;
        auto* map = nav_.push<GeoMapView>(
            "me",
            static_cast<int32_t>(alt_),
            GeoPos::alt_unit::METERS,
            GeoPos::spd_unit::HIDDEN,
            center_lat, center_lon, 999.0f, []() { map_reserve_restore(); });

        // Plot every known node with a GPS position as a green marker (up to
        // the GeoMap 30-marker limit).
        map->clear_markers();
        // No marker of our own: the map already draws a crosshair at its centre and
        // labels it "me", and a second symbol on the same pixel only fought with it.
        const size_t total = node_db_.count();
        for (size_t i = 0; i < total; i++) {
            const NodeEntry* e = node_db_.at(i);
            if (!e || !e->active || !e->has_position) continue;
            if (e->position.latitude == 0.0 && e->position.longitude == 0.0) continue;
            GeoMarker m;
            m.lat = static_cast<float>(e->position.latitude);
            m.lon = static_cast<float>(e->position.longitude);
            m.angle = 400;  // >360 = static marker, no bearing arrow
            m.tag = e->long_name[0]
                        ? std::string(e->long_name).substr(0, 10)
                        : to_string_hex(e->node_id, 8);
            // Green for a node we hear directly, orange for one that reaches us only
            // through a relay - the line drawn to it takes the same colour, so the map
            // shows the shape of the mesh and not just where everyone is.
            m.color = (e->hop_count == 0) ? Color::green() : Color::orange();
            if (map->store_marker(m) == MapMarkerStored::MARKER_LIST_FULL) break;
        }
        // Lines from where we are to everyone we know of. Without a position of our
        // own there is no sensible place to draw them from.
        if (has_fix_ || manual_)
            map->set_marker_links(lat_, lon_);
        else
            map->clear_marker_links();
    };
}

void MeshtasticMapView::focus() {
    button_open_map_.focus();
}

void MeshtasticMapView::paint(Painter& painter) {
    {  // Wipe the entire sub-view first: the tabs share one area, and painting only
        // the text rows left the previous tab's widgets showing through around and
        // behind the button row.
        const auto rr = screen_rect();
        painter.fill_rectangle({rr.left(), rr.top(), screen_width, SV_H},
                               Theme::getInstance()->bg_darkest->background);
    }
    const auto& bg = Theme::getInstance()->bg_darkest->background;
    const auto& fg = *Theme::getInstance()->fg_light;
    const auto& dim = *Theme::getInstance()->fg_medium;
    const auto r = screen_rect();
    const int x0 = r.left();
    const int y0 = r.top();

    painter.fill_rectangle({x0, y0, screen_width, BOT_Y}, bg);

    // 4-decimal fixed-point format for a coordinate.
    auto fmt_coord = [](float v) {
        int i = static_cast<int>(v);
        int f = static_cast<int>(std::abs((v - static_cast<float>(i)) * 10000.0f));
        std::string s = to_string_dec_int(i) + ".";
        if (f < 10)
            s += "000";
        else if (f < 100)
            s += "00";
        else if (f < 1000)
            s += "0";
        return s + to_string_dec_uint(static_cast<uint32_t>(f));
    };

    if (has_fix_) {
        painter.draw_string({x0, y0}, fg, "Lat: " + fmt_coord(lat_));
        painter.draw_string({x0, y0 + 16}, fg, "Lon: " + fmt_coord(lon_));
        // Altitude on its own line, above where the fix is described: it belongs with
        // the coordinates, not squeezed in beside the source of the position.
        painter.draw_string({x0, y0 + 32}, fg, "Alt: " + to_string_dec_int(static_cast<int32_t>(alt_)) + " m" + (manual_ ? std::string() : "   Sats: " + to_string_dec_uint(sats_)));
        painter.draw_string({x0, y0 + 48}, dim,
                            manual_ ? "Fixed position (Setup)" : "GPS fix");
    } else {
        painter.draw_string({x0, y0}, dim, "GPS: no fix");
        painter.draw_string({x0, y0 + 16}, dim, "(sky view needed, or set");
        painter.draw_string({x0, y0 + 32}, dim, " a fixed pos in Setup)");
    }

    // Battery and airtime live in their own widgets (ROW_BATT / ROW_UTIL) so a changing
    // reading repaints one line instead of the whole tab. Environment telemetry, which
    // only exists with an i2c sensor attached, follows them.
    if (has_env_) {
        int t10 = static_cast<int>(temp_ * 10.0f);
        painter.draw_string({x0, y0 + 112}, fg,
                            "T:" + to_string_dec_int(t10 / 10) + "." + to_string_dec_uint(std::abs(t10 % 10)) +
                                "C H:" + to_string_dec_int(static_cast<int>(hum_)) +
                                "% P:" + to_string_dec_int(static_cast<int>(press_)));
    }

    const int list_y = y0 + (has_env_ ? 132 : 116);
    painter.draw_hline({x0, list_y - 4}, screen_width, dim.foreground);
    painter.draw_string({x0, list_y}, fg, "Nodes with GPS:");

    int y = list_y + 16;
    size_t shown = 0;
    size_t seen = 0;  // nodes with a position, counted past the scroll offset
    const size_t total = node_db_.count();
    for (size_t i = 0; i < total && y < y0 + BOT_Y - 16; i++) {
        const NodeEntry* e = node_db_.at(i);
        if (!e || !e->active || !e->has_position) continue;
        if (e->position.latitude == 0.0 && e->position.longitude == 0.0) continue;
        if (seen++ < gps_scroll_) continue;

        std::string name = e->long_name[0]
                               ? std::string(e->long_name).substr(0, 8)
                               : to_string_hex(e->node_id, 4);
        name.resize(9, ' ');

        int nlat_i = static_cast<int>(e->position.latitude);
        int nlon_i = static_cast<int>(e->position.longitude);
        int nlat_f = static_cast<int>(std::abs((e->position.latitude - nlat_i) * 1000.0));
        int nlon_f = static_cast<int>(std::abs((e->position.longitude - nlon_i) * 1000.0));

        std::string pos = to_string_dec_int(nlat_i) + ".";
        if (nlat_f < 10)
            pos += "00";
        else if (nlat_f < 100)
            pos += "0";
        pos += to_string_dec_uint(static_cast<uint32_t>(nlat_f)) + " ";
        pos += to_string_dec_int(nlon_i) + ".";
        if (nlon_f < 10)
            pos += "00";
        else if (nlon_f < 100)
            pos += "0";
        pos += to_string_dec_uint(static_cast<uint32_t>(nlon_f));

        painter.draw_string({x0, y}, fg, name + pos);
        y += 16;
        shown++;
    }

    if (shown == 0) {
        painter.draw_string({x0, list_y + 16},
                            dim, gps_scroll_ ? "" : "None received yet");
    }
    // Say so when the list runs past the bottom, or the encoder looks broken rather
    // than unused.
    {
        size_t with_pos = 0;
        for (size_t i = 0; i < total; i++) {
            const NodeEntry* e = node_db_.at(i);
            if (e && e->active && e->has_position &&
                (e->position.latitude != 0.0 || e->position.longitude != 0.0)) with_pos++;
        }
        if (with_pos > shown + gps_scroll_ || gps_scroll_)
            painter.draw_string({x0, list_y}, dim,
                                "Nodes with GPS: " +
                                    to_string_dec_uint(static_cast<uint32_t>(gps_scroll_ + 1)) +
                                    "-" +
                                    to_string_dec_uint(static_cast<uint32_t>(gps_scroll_ + shown)) +
                                    "/" + to_string_dec_uint(static_cast<uint32_t>(with_pos)));
    }
}

void MeshtasticMapView::update_position(float lat, float lon, float alt, uint8_t sats) {
    lat_ = lat;
    lon_ = lon;
    alt_ = alt;
    sats_ = sats;
    has_fix_ = true;
    manual_ = false;
    set_dirty();
}

void MeshtasticMapView::set_fixed_position(float lat, float lon, int32_t alt_m, uint8_t sats) {
    // The altitude and satellite count entered by hand belong on screen too - showing a
    // flat zero for a position the user configured looked like a broken reading.
    lat_ = lat;
    lon_ = lon;
    alt_ = static_cast<float>(alt_m);
    sats_ = sats;
    has_fix_ = true;
    manual_ = true;
    set_dirty();
}

void MeshtasticMapView::refresh_nodes() {
    set_dirty();
}

// Encoder turns reach here by bubbling up from whichever button holds the focus, so the
// list scrolls without taking the D-pad away from the buttons.
bool MeshtasticMapView::on_encoder(const EncoderEvent delta) {
    if (delta == 0) return false;
    size_t with_pos = 0;
    const size_t total = node_db_.count();
    for (size_t i = 0; i < total; i++) {
        const NodeEntry* e = node_db_.at(i);
        if (e && e->active && e->has_position &&
            (e->position.latitude != 0.0 || e->position.longitude != 0.0)) with_pos++;
    }
    if (with_pos == 0) return false;
    int v = static_cast<int>(gps_scroll_) + ((delta > 0) ? 1 : -1);
    if (v < 0) v = 0;
    if (v > static_cast<int>(with_pos) - 1) v = static_cast<int>(with_pos) - 1;
    if (v == static_cast<int>(gps_scroll_)) return false;
    gps_scroll_ = static_cast<uint8_t>(v);
    set_dirty();
    return true;
}

void MeshtasticMapView::set_battery(uint8_t pct, float volts, bool charging) {
    batt_pct_ = pct;
    batt_charging_ = charging;
    const int v100 = static_cast<int>(volts * 100.0f + 0.5f);
    const std::string s = "Batt: " + to_string_dec_uint(pct) + "%  " +
                          to_string_dec_int(v100 / 100) + "." +
                          (v100 % 100 < 10 ? "0" : "") + to_string_dec_uint(v100 % 100) + "V" +
                          (charging ? "  charging" : "");
    // Repaint only when the line really changes. The gauge jitters by a hundredth of a
    // volt from one read to the next, and redrawing an identical row every second is
    // exactly what the eye reads as flicker.
    if (s == batt_text_) return;
    batt_text_ = s;
    text_batt_.set(s);
}

void MeshtasticMapView::set_utilisation(float ch_util, float air_util) {
    auto pct = [](float v) {
        const int p10 = static_cast<int>(v * 10.0f + 0.5f);
        return to_string_dec_int(p10 / 10) + "." + to_string_dec_uint(p10 % 10) + "%";
    };
    const std::string s = "ChUtil: " + pct(ch_util) + "  AirTX: " + pct(air_util);
    if (s == util_text_) return;
    util_text_ = s;
    text_util_.set(s);
}

// The uptime we are broadcasting, in the units it is broadcast in - seconds - shown
// the way a person reads them. It only changes once a minute, and repainting an
// identical row is what the eye reads as flicker, so an unchanged line is skipped
// like the two above it.
void MeshtasticMapView::set_uptime(uint32_t seconds) {
    const uint32_t d = seconds / 86400, h = (seconds / 3600) % 24, m = (seconds / 60) % 60;
    std::string s = "Up: ";
    if (d) s += to_string_dec_uint(d) + "d ";
    if (d || h) s += to_string_dec_uint(h) + "h ";
    s += to_string_dec_uint(m) + "m";
    if (s == up_text_) return;
    up_text_ = s;
    text_up_.set(s);
}

// ============================================================================
// Telemetry pages
// ============================================================================

MeshTelemetryMenuView::MeshTelemetryMenuView(NavigationView& nav, MeshSettings& cfg, MeshSettingsHooks& hooks)
    : MeshSettingsPage(nav, cfg, hooks) {
    add_children({&button_device_, &button_position_, &button_sensors_, &button_timing_});
    button_device_.on_select = [this](Button&) {
        nav_.push<MeshTelemetryDevicePageView>(cfg_, hooks_);
    };
    button_position_.on_select = [this](Button&) {
        nav_.push<MeshTelemetryPositionPageView>(cfg_, hooks_);
    };
    button_sensors_.on_select = [this](Button&) {
        nav_.push<MeshTelemetrySensorsPageView>(cfg_, hooks_);
    };
    button_timing_.on_select = [this](Button&) {
        nav_.push<MeshTelemetryTimingPageView>(cfg_, hooks_);
    };
}

MeshTelemetryDevicePageView::MeshTelemetryDevicePageView(NavigationView& nav,
                                                         MeshSettings& cfg,
                                                         MeshSettingsHooks& hooks)
    : MeshTelemetryPage(nav, cfg, hooks) {
    add_children({&labels_, &field_batt_, &field_pct_, &field_volt_, &field_dv_,
                  &field_util_, &field_chutil_, &field_airutil_,
                  &field_up_, &field_uphours_});
    auto& t = cfg_.telemetry;

    field_batt_.set_by_value(t.batt_mode);
    field_batt_.on_change = [this](size_t, OptionsField::value_t v) {
        cfg_.telemetry.batt_mode = static_cast<uint8_t>(v);
        changed();
    };
    field_pct_.set_value(t.batt_pct);
    field_pct_.on_change = [this](int32_t v) {
        cfg_.telemetry.batt_pct = static_cast<uint8_t>(v);
        changed();
    };

    field_volt_.set_by_value(t.volt_mode);
    field_volt_.on_change = [this](size_t, OptionsField::value_t v) {
        cfg_.telemetry.volt_mode = static_cast<uint8_t>(v);
        changed();
    };
    field_dv_.set_value(t.volt_dV);
    field_dv_.on_change = [this](int32_t v) {
        cfg_.telemetry.volt_dV = static_cast<uint8_t>(v);
        changed();
    };

    field_util_.set_by_value(t.util_mode);
    field_util_.on_change = [this](size_t, OptionsField::value_t v) {
        cfg_.telemetry.util_mode = static_cast<uint8_t>(v);
        changed();
    };
    field_chutil_.set_value(t.chutil_pct);
    field_chutil_.on_change = [this](int32_t v) {
        cfg_.telemetry.chutil_pct = static_cast<uint8_t>(v);
        changed();
    };
    field_airutil_.set_value(t.airutil_pct);
    field_airutil_.on_change = [this](int32_t v) {
        cfg_.telemetry.airutil_pct = static_cast<uint8_t>(v);
        changed();
    };

    field_up_.set_by_value(t.up_mode);
    field_up_.on_change = [this](size_t, OptionsField::value_t v) {
        cfg_.telemetry.up_mode = static_cast<uint8_t>(v);
        changed();
    };
    field_uphours_.set_value(t.up_hours);
    field_uphours_.on_change = [this](int32_t v) {
        cfg_.telemetry.up_hours = static_cast<uint32_t>(v);
        changed();
    };
}

MeshTelemetryPositionPageView::MeshTelemetryPositionPageView(NavigationView& nav,
                                                             MeshSettings& cfg,
                                                             MeshSettingsHooks& hooks)
    : MeshTelemetryPage(nav, cfg, hooks) {
    add_children({&labels_, &check_gps_, &button_lat_, &button_lon_, &field_alt_,
                  &button_alt_, &field_sats_, &button_map_});

    check_gps_.set_value(cfg_.gps_enabled);
    check_gps_.on_select = [this](Checkbox&, bool v) { cfg_.gps_enabled = v; changed(); };

    if (cfg_.fixed_lat != 0.0f) button_lat_.set_text(to_string_decimal(cfg_.fixed_lat, 4));
    if (cfg_.fixed_lon != 0.0f) button_lon_.set_text(to_string_decimal(cfg_.fixed_lon, 4));

    button_lat_.on_select = [this](Button&) {
        entry_ = to_string_decimal(cfg_.fixed_lat, 4);
        mesh_prompt(nav_, entry_, 12, [this](std::string& v) {
                        float d = 0.0f;
                        if (!parse_degrees(v, 90.0f, d)) return;
                        cfg_.fixed_lat = d;
                        button_lat_.set_text(to_string_decimal(cfg_.fixed_lat, 4));
                        changed(); }, false);
    };
    button_lon_.on_select = [this](Button&) {
        entry_ = to_string_decimal(cfg_.fixed_lon, 4);
        mesh_prompt(nav_, entry_, 12, [this](std::string& v) {
                        float d = 0.0f;
                        if (!parse_degrees(v, 180.0f, d)) return;
                        cfg_.fixed_lon = d;
                        button_lon_.set_text(to_string_decimal(cfg_.fixed_lon, 4));
                        changed(); }, false);
    };

    field_alt_.set_value(cfg_.telemetry.fixed_alt_m);
    field_alt_.on_change = [this](int32_t v) { cfg_.telemetry.fixed_alt_m = v; changed(); };
    button_alt_.on_select = [this](Button&) {
        // Typed in full rather than nudged a hundred steps at a time.
        entry_ = to_string_dec_int(cfg_.telemetry.fixed_alt_m);
        mesh_prompt(nav_, entry_, 6, [this](std::string& v) {
                        int32_t n = 0;
                        bool neg = false;
                        size_t i = 0;
                        if (!v.empty() && v[0] == '-') {
                            neg = true;
                            i = 1;
                        }
                        for (; i < v.size(); i++)
                            if (v[i] >= '0' && v[i] <= '9') n = n * 10 + (v[i] - '0');
                        cfg_.telemetry.fixed_alt_m = neg ? -n : n;
                        field_alt_.set_value(cfg_.telemetry.fixed_alt_m);
                        changed(); }, true);
    };

    field_sats_.set_value(cfg_.telemetry.fixed_sats);
    field_sats_.on_change = [this](int32_t v) {
        cfg_.telemetry.fixed_sats = static_cast<uint8_t>(v);
        changed();
    };

    button_map_.on_select = [this](Button&) { if (hooks_.pick_on_map) hooks_.pick_on_map(); };
}

MeshTelemetrySensorsPageView::MeshTelemetrySensorsPageView(NavigationView& nav,
                                                           MeshSettings& cfg,
                                                           MeshSettingsHooks& hooks)
    : MeshTelemetryPage(nav, cfg, hooks) {
    row_check_[0] = &row_check_0_;
    row_value_[0] = &row_value_0_;
    row_unit_[0] = &row_unit_0_;
    row_check_[1] = &row_check_1_;
    row_value_[1] = &row_value_1_;
    row_unit_[1] = &row_unit_1_;
    row_check_[2] = &row_check_2_;
    row_value_[2] = &row_value_2_;
    row_unit_[2] = &row_unit_2_;
    row_check_[3] = &row_check_3_;
    row_value_[3] = &row_value_3_;
    row_unit_[3] = &row_unit_3_;
    row_check_[4] = &row_check_4_;
    row_value_[4] = &row_value_4_;
    row_unit_[4] = &row_unit_4_;
    row_check_[5] = &row_check_5_;
    row_value_[5] = &row_value_5_;
    row_unit_[5] = &row_unit_5_;

    add_children({&labels_, &field_family_, &button_all_on_, &button_random_,
                  &button_all_off_, &button_send_});
    button_send_.on_select = [this](Button&) {
        if (hooks_.send_telemetry) hooks_.send_telemetry();
    };
    for (size_t r = 0; r < XROWS; r++)
        add_children({row_check_[r], row_value_[r], row_unit_[r]});

    {
        OptionsField::options_t opts;
        opts.reserve(NUM_EXTRA_GROUPS);
        for (size_t g = 0; g < NUM_EXTRA_GROUPS; g++)
            opts.emplace_back(EXTRA_GROUPS[g], static_cast<OptionsField::value_t>(g));
        field_family_.set_options(std::move(opts));
    }

    for (size_t r = 0; r < XROWS; r++) {
        const size_t row = r;
        row_check_[r]->on_select = [this, row](Checkbox&, bool v) {
            const size_t k = row_idx_[row];
            if (k < NUM_EXTRA_METRICS) cfg_.telemetry.extra_on[k] = v ? 1 : 0;
            changed();
        };
        row_value_[r]->on_change = [this, row](int32_t v) {
            const size_t k = row_idx_[row];
            if (k < NUM_EXTRA_METRICS) cfg_.telemetry.extra_raw[k] = static_cast<uint16_t>(v);
            changed();
        };
    }
    // The selector drives the rows, and the rows never drive the selector back.
    // OptionsField::set_by_value() calls set_selected_index(), whose trigger_change
    // argument defaults to TRUE, so it fires on_change even when the index has not
    // moved: writing the group back from inside this handler called it again, and
    // again, 144 bytes of stack a turn until the M0's 4 KB were gone. The field
    // already holds the right value when it calls us - there is nothing to write back.
    field_family_.on_change = [this](size_t, OptionsField::value_t v) {
        show_family(static_cast<size_t>(v));
    };
    show_family(family_);

    button_all_on_.on_select = [this](Button&) {
        for (size_t k = 0; k < NUM_EXTRA_METRICS; k++) cfg_.telemetry.extra_on[k] = 1;
        show_family(family_);
        changed();
    };
    button_random_.on_select = [this](Button&) {
        // Plausible values, not arbitrary ones: each field carries its own ceiling, so
        // a wind gust comes out in metres per second and a pulse in beats per minute.
        static uint32_t seed = 0;
        seed ^= static_cast<uint32_t>(rtc_time::now().second() + 1) * 2654435761u;
        for (size_t k = 0; k < NUM_EXTRA_METRICS; k++) {
            seed = seed * 1664525u + 1013904223u;
            const uint16_t cap = EXTRA_METRICS[k].rnd_max ? EXTRA_METRICS[k].rnd_max : 100;
            cfg_.telemetry.extra_raw[k] = static_cast<uint16_t>((seed >> 16) % (cap + 1u));
            cfg_.telemetry.extra_on[k] = 1;
        }
        show_family(family_);
        changed();
    };
    button_all_off_.on_select = [this](Button&) {
        for (size_t k = 0; k < NUM_EXTRA_METRICS; k++) cfg_.telemetry.extra_on[k] = 0;
        show_family(family_);
        changed();
    };
}

void MeshTelemetrySensorsPageView::show_family(size_t group) {
    if (group >= NUM_EXTRA_GROUPS) group = 0;
    family_ = group;
    size_t row = 0;
    for (size_t k = 0; k < NUM_EXTRA_METRICS && row < XROWS; k++) {
        if (EXTRA_METRICS[k].group != group) continue;
        row_idx_[row] = k;
        row_check_[row]->set_text(EXTRA_METRICS[k].name);
        row_check_[row]->set_value(cfg_.telemetry.extra_on[k] != 0);
        row_value_[row]->set_value(cfg_.telemetry.extra_raw[k]);
        // Several fields are stored scaled so the entry box can stay whole numbers; the
        // unit says which, e.g. tenths of a metre per second.
        row_unit_[row]->set(EXTRA_METRICS[k].scale == 1.0f
                                ? std::string(EXTRA_METRICS[k].unit)
                                : (EXTRA_METRICS[k].scale == 0.1f ? "0.1 " : ".01 ") +
                                      std::string(EXTRA_METRICS[k].unit));
        row_check_[row]->hidden(false);
        row_value_[row]->hidden(false);
        row_unit_[row]->hidden(false);
        row++;
    }
    for (; row < XROWS; row++) {
        row_idx_[row] = NUM_EXTRA_METRICS;  // no field on this row
        row_check_[row]->hidden(true);
        row_value_[row]->hidden(true);
        row_unit_[row]->hidden(true);
    }
    set_dirty();
}

MeshTelemetryTimingPageView::MeshTelemetryTimingPageView(NavigationView& nav,
                                                         MeshSettings& cfg,
                                                         MeshSettingsHooks& hooks)
    : MeshTelemetryPage(nav, cfg, hooks) {
    add_children({&labels_, &field_pos_min_, &field_tel_min_});

    field_pos_min_.set_value(cfg_.telemetry.pos_min);
    field_pos_min_.on_change = [this](int32_t v) {
        cfg_.telemetry.pos_min = static_cast<uint8_t>(v);
        changed();
    };
    field_tel_min_.set_value(cfg_.telemetry.tel_min);
    field_tel_min_.on_change = [this](int32_t v) {
        cfg_.telemetry.tel_min = static_cast<uint8_t>(v);
        changed();
    };
}

// ============================================================================
// MeshtasticChannelsView
// ============================================================================

MeshtasticChannelsView::MeshtasticChannelsView(NavigationView& nav, uint8_t preset_idx, uint8_t region_idx, uint8_t freq_slot, uint8_t cr, std::string* names, std::string* keys, bool* enabled, uint8_t* active, std::string* primary_name, NodeDB& node_db, uint32_t open_dm, std::function<void()> on_change, std::function<void(uint32_t)> on_dm, std::function<std::string()> make_key)
    : nav_(nav), preset_idx_(preset_idx), region_idx_(region_idx), freq_slot_(freq_slot), cr_(cr), names_(names), keys_(keys), enabled_(enabled), active_(active), primary_name_(primary_name), node_db_(node_db), open_dm_(open_dm), on_change_(std::move(on_change)), on_dm_(std::move(on_dm)), make_key_(std::move(make_key)) {
    // The list itself takes focus: the encoder / D-pad move the selection, Select
    // acts on the row. No child widgets, so nothing competes for focus.
    add_children({&button_del_, &button_rnd_, &button_qr_, &button_bell_});
    button_bell_.on_select = [this](Button&) { send_bell(); };
    set_focusable(true);
    // Open on the conversation the chat is showing, so the marked row and the
    // highlighted row agree and the action buttons work on what you were just reading.
    selected_ = row_of_open();
    button_del_.on_select = [this](Button&) { delete_channel(); };
    button_rnd_.on_select = [this](Button&) { randomize_key(); };
    button_qr_.on_select = [this](Button&) { show_qr(); };
}

void MeshtasticChannelsView::focus() {
    Widget::focus();
}

size_t MeshtasticChannelsView::dm_count() const {
    size_t n = 0;
    for (size_t i = 0; i < node_db_.count(); i++) {
        const NodeEntry* e = node_db_.at(i);
        if (e && e->active && e->dm_thread) n++;
    }
    return n;
}

uint32_t MeshtasticChannelsView::dm_at(size_t n) const {
    size_t seen = 0;
    for (size_t i = 0; i < node_db_.count(); i++) {
        const NodeEntry* e = node_db_.at(i);
        if (!e || !e->active || !e->dm_thread) continue;
        if (seen++ == n) return e->node_id;
    }
    return 0;
}

size_t MeshtasticChannelsView::row_of_open() const {
    const size_t total = row_count();
    for (size_t r = 0; r < total; r++) {
        const Row info = row_at(r);
        if (open_dm_) {
            if (info.kind == Row::Dm && info.node == open_dm_) return r;
        } else if (info.kind == Row::Primary) {
            if (*active_ == 0) return r;
        } else if (info.kind == Row::Channel) {
            if (*active_ == static_cast<size_t>(info.slot) + 1) return r;
        }
    }
    return 0;
}

MeshtasticChannelsView::Row MeshtasticChannelsView::row_at(size_t row) const {
    Row out;
    if (row == 0) {
        out.kind = Row::Primary;
        return out;
    }
    size_t seen = 1;
    for (int c = 0; c < NUM_CUSTOM; c++) {  // configured channels first
        if (!(enabled_[c] && !names_[c].empty())) continue;
        if (seen == row) {
            out.kind = Row::Channel;
            out.slot = c;
            return out;
        }
        seen++;
    }
    const size_t dms = dm_count();
    for (size_t d = 0; d < dms; d++) {  // then the private threads
        if (seen == row) {
            out.kind = Row::Dm;
            out.node = dm_at(d);
            return out;
        }
        seen++;
    }
    for (int c = 0; c < NUM_CUSTOM; c++) {  // and finally the free slots
        if (enabled_[c] && !names_[c].empty()) continue;
        if (seen == row) {
            out.kind = Row::Empty;
            out.slot = c;
            return out;
        }
        seen++;
    }
    return out;
}

void MeshtasticChannelsView::paint(Painter& painter) {
    const auto& bg = Theme::getInstance()->bg_darkest->background;
    const auto& fg = *Theme::getInstance()->fg_light;
    const auto r = screen_rect();
    const int x0 = r.left();
    const int y0 = r.top();

    painter.fill_rectangle({x0, y0, screen_width, r.height()}, bg);

    const size_t total = row_count();
    for (size_t i = 0; i < static_cast<size_t>(VISIBLE); i++) {
        const size_t row = i + scroll_offset_;
        if (row >= total) break;
        const int y = y0 + static_cast<int>(i) * ROW_H;
        const bool sel = (row == selected_);
        painter.fill_rectangle({x0, y, screen_width, ROW_H}, sel ? Color::blue() : bg);
        const Style st = sel ? Style{fg.font, Color::blue(), Color::white()} : fg;

        const Row info = row_at(row);
        std::string t;
        switch (info.kind) {
            case Row::Primary:
                t = std::string((!open_dm_ && *active_ == 0) ? "> " : "  ") + "0 " +
                    primary_label();
                break;
            case Row::Channel:
                t = std::string((!open_dm_ &&
                                 *active_ == static_cast<size_t>(info.slot) + 1)
                                    ? "> "
                                    : "  ") +
                    to_string_dec_uint(info.slot + 1) + " " + names_[info.slot] +
                    (keys_[info.slot].empty() ? " (open)" : " [enc]");
                break;
            case Row::Dm: {
                const NodeEntry* e = node_db_.find(info.node);
                // A thread is end-to-end encrypted when we hold that node's public key.
                t = std::string((open_dm_ == info.node) ? "> DM " : "  DM ") +
                    ((e && e->has_pubkey) ? "[pkc] " : "");
                t += (e && e->long_name[0]) ? std::string(e->long_name).substr(0, 18)
                                            : to_string_hex(info.node, 8);
                break;
            }
            default:
                t = "  " + to_string_dec_uint(info.slot + 1) + " <empty>";
                break;
        }
        painter.draw_string({x0, y + 4}, st, t.substr(0, 30));
    }

    // Hints below the list (drawn, not Labels - the flash budget is tight).
    const int hy = y0 + LIST_H + 4;
    painter.draw_string({x0 + 4, hy}, fg, "Tap: name / key / switch");
}

void MeshtasticChannelsView::move_selection(int delta) {
    const size_t total = row_count();
    if (total == 0) return;
    int sel = static_cast<int>(selected_) + delta;
    if (sel < 0) sel = 0;
    if (sel >= static_cast<int>(total)) sel = static_cast<int>(total) - 1;
    if (static_cast<size_t>(sel) == selected_) return;
    selected_ = static_cast<size_t>(sel);
    if (selected_ < scroll_offset_)
        scroll_offset_ = selected_;
    else if (selected_ >= scroll_offset_ + VISIBLE)
        scroll_offset_ = selected_ - VISIBLE + 1;
    set_dirty();
}

bool MeshtasticChannelsView::on_encoder(const EncoderEvent delta) {
    if (delta == 0) return true;
    move_selection(delta > 0 ? 1 : -1);
    return true;
}

bool MeshtasticChannelsView::on_key(const KeyEvent key) {
    if (key == KeyEvent::Down && selected_ + 1 >= row_count()) return false;
    switch (key) {
        case KeyEvent::Up:
            move_selection(-1);
            return true;
        case KeyEvent::Down:
            move_selection(1);
            return true;
        case KeyEvent::Select:
            open_row(selected_);
            return true;
        default:
            return false;  // Back etc. propagate (nav pop)
    }
}

bool MeshtasticChannelsView::on_touch(const TouchEvent event) {
    if (event.type != TouchEvent::Type::End) return true;  // swallow press/move
    const int rel = event.point.y() - screen_rect().top();
    if (rel < 0 || rel >= LIST_H) return true;  // hint area: ignore
    const size_t row = scroll_offset_ + static_cast<size_t>(rel / ROW_H);
    if (row >= row_count()) return true;
    selected_ = row;
    set_dirty();
    open_row(row);
    return true;
}

void MeshtasticChannelsView::open_row(size_t row) {
    const Row info = row_at(row);
    switch (info.kind) {
        case Row::Primary:
            // Same gesture as a custom slot: tap to switch, tap again once it is the
            // active one to edit it. The name is half the channel hash, so it has to be
            // reachable - a peer whose primary is called something else is otherwise
            // simply unreachable, with nothing on screen to say why.
            if (!open_dm_ && *active_ == 0)
                edit_primary_name();
            else
                use_primary();
            break;
        case Row::Channel:
        case Row::Empty:
            select_or_edit(info.slot);
            break;
        case Row::Dm:
            if (info.node && on_dm_) {
                on_dm_(info.node);  // owner points the chat at this node and shows Chat
                nav_.pop();
            }
            break;
    }
}

// What the primary channel is actually called, and the hash byte that name produces -
// shown because an invisible one-byte mismatch is exactly what makes two nodes deaf to
// each other while every diagnostic still looks healthy.
std::string MeshtasticChannelsView::primary_label() const {
    const std::string name = (primary_name_ && !primary_name_->empty())
                                 ? *primary_name_
                                 : std::string(PRESET_CHANNEL_NAMES[preset_idx_]);
    uint8_t h = 0;
    for (char c : name) h ^= static_cast<uint8_t>(c);
    for (uint8_t b : meshtastic::DEFAULT_PSK) h ^= b;
    return name + " h" + to_string_hex(h, 2);
}

void MeshtasticChannelsView::edit_primary_name() {
    if (!primary_name_) return;
    // Prompt straight into the setting, like the custom slots do - the keyboard needs a
    // string that outlives it. Pre-filled with the preset name so clearing the field is
    // how you go back to following the preset.
    if (primary_name_->empty()) *primary_name_ = PRESET_CHANNEL_NAMES[preset_idx_];
    mesh_prompt(nav_, *primary_name_, 11, [this](std::string&) {
                    if (on_change_) on_change_();
                    set_dirty(); }, false);
}

void MeshtasticChannelsView::use_primary() {
    *active_ = 0;
    // ...and leave any private thread with it. Switching to the public channel while
    // the chat stayed in a DM meant the row never read as "open", so tapping it again
    // switched channel a second time instead of offering to rename it.
    if (on_dm_) on_dm_(meshtastic::BROADCAST_ADDR);
    if (on_change_) on_change_();
    nav_.pop();
}

void MeshtasticChannelsView::select_or_edit(int i) {
    if (!enabled_[i] || names_[i].empty()) {
        // Empty slot -> set the NAME (one keyboard). Enables + activates it.
        edit_custom(i);
    } else if (*active_ == static_cast<uint8_t>(i + 1)) {
        // Already active -> set/change the encryption KEY (one keyboard, empty =
        // unencrypted). A SEPARATE prompt, never nested after the name - the old
        // name->key nesting read as "the keyboard won't close".
        mesh_prompt(nav_, keys_[i], 32, [this](std::string&) {
                        if (on_change_) on_change_();
                        set_dirty(); }, false);
    } else {
        // Configured but not active -> switch to it and return to the chat.
        *active_ = static_cast<uint8_t>(i + 1);
        if (on_change_) on_change_();
        nav_.pop();
    }
}

void MeshtasticChannelsView::edit_custom(int i) {
    // ONE keyboard for the channel name. Setting a name enables + activates the
    // channel (unencrypted). Tap the active channel again to add an AES key.
    mesh_prompt(nav_, names_[i], 12, [this, i](std::string&) {
                    enabled_[i] = !names_[i].empty();
                    *active_ = enabled_[i] ? static_cast<uint8_t>(i + 1) : 0;
                    if (on_change_) on_change_();
                    set_dirty(); }, false);
}

int MeshtasticChannelsView::target_slot() const {
    // Highlight a custom channel and the action rows work on it; otherwise they fall
    // back to the active channel, which is how "switch to it, then Delete" behaves.
    const Row info = row_at(selected_);
    if (info.kind == Row::Channel || info.kind == Row::Empty) return info.slot;
    if (*active_ >= 1 && *active_ <= NUM_CUSTOM) return *active_ - 1;
    return -1;
}

void MeshtasticChannelsView::send_bell() {
    if (!on_bell_) return;
    const Row info = row_at(selected_);
    switch (info.kind) {
        case Row::Primary:
            on_bell_(0, 0);
            break;
        case Row::Channel:
            on_bell_(static_cast<uint8_t>(info.slot + 1), 0);
            break;
        case Row::Dm:
            on_bell_(0xFF, info.node);
            break;
        default:
            return;  // an empty slot has nowhere to ring
    }
    nav_.pop();
}

void MeshtasticChannelsView::delete_channel() {
    const int i = target_slot();
    if (i < 0) return;  // primary (public) can't be deleted
    names_[i].clear();
    keys_[i].clear();
    enabled_[i] = false;
    if (*active_ == static_cast<uint8_t>(i + 1))
        *active_ = 0;              // deleted the active one -> public primary
    if (on_change_) on_change_();  // re-push channel table + persist
    set_dirty();
}

void MeshtasticChannelsView::randomize_key() {
    const int i = target_slot();
    if (i < 0 || !make_key_) return;
    // A random 128-bit key entered as 32 hex characters is taken as the raw AES key,
    // so the channel is private and can be shared verbatim through its QR code.
    keys_[i] = make_key_();
    if (names_[i].empty()) names_[i] = "ch" + to_string_dec_uint(i + 1);
    enabled_[i] = true;
    *active_ = static_cast<uint8_t>(i + 1);
    if (on_change_) on_change_();
    set_dirty();
}

MeshtasticQRView::MeshtasticQRView(NavigationView& nav, const std::string& text, std::string line1, std::string line2)
    : text_(text), line1_(std::move(line1)), line2_(std::move(line2)) {
    add_children({&image_, &button_close_});
    // The widget keeps the pointer, so the text has to live in the view.
    image_.set_text(text_.c_str());
    button_close_.on_select = [&nav](Button&) { nav.pop(); };
}

void MeshtasticQRView::paint(Painter& painter) {
    const auto r = screen_rect();
    painter.fill_rectangle({r.left(), r.top(), screen_width, 60},
                           Theme::getInstance()->bg_darkest->background);
    painter.draw_string({r.left(), r.top() + 8}, *Theme::getInstance()->fg_light,
                        line1_.substr(0, 30));
    painter.draw_string({r.left(), r.top() + 28}, *Theme::getInstance()->fg_medium,
                        line2_.substr(0, 30));
}

void MeshtasticChannelsView::show_qr() {
    // Whatever row is highlighted is what gets shared - a private thread offers the
    // node's contact code, a channel its own join code. It used to ask target_slot(),
    // which fell back to the active channel and so only ever produced the public one.
    const Row sel = row_at(selected_);
    if (sel.kind == Row::Dm && sel.node) {
        const NodeEntry* e = node_db_.find(sel.node);
        if (!e) return;
        qr_url_ = meshtastic::contact_share_url(e->node_id, e->long_name, e->short_name,
                                                e->public_key, e->has_pubkey);
        nav_.push<MeshtasticQRView>(
            qr_url_,
            e->long_name[0] ? std::string(e->long_name) : "!" + to_string_hex(sel.node, 8),
            e->has_pubkey ? "contact  [key]" : "contact  [no key]");
        return;
    }
    const int i = (sel.kind == Row::Channel) ? sel.slot : -1;
    uint8_t key[16];
    std::string l1, l2;

    // Everything the far end needs to end up on our air, not just on our channel.
    const auto& preset = meshtastic::MODEM_PRESETS[preset_idx_];
    meshtastic::ChannelShare sh;
    sh.modem_preset = meshtastic::meshtastic_modem_preset(preset_idx_);
    sh.region_code = meshtastic::meshtastic_region_code(region_idx_);
    sh.sf = preset.sf;
    sh.bw_hz = preset.bw_hz;
    sh.cr = cr_;
    // The slot we actually transmit on, 1-based. A node that imports a channel makes it
    // its primary and then derives the frequency from THAT name - so without this it
    // retunes away from us and the shared channel is silent, which is exactly what
    // happened. Naming the slot outright removes the guesswork on both sides.
    sh.channel_num = meshtastic::channel_slot(region_idx_, preset_idx_, freq_slot_,
                                              primary_name_ ? primary_name_->c_str() : nullptr) +
                     1;

    const uint8_t default_psk = 1;  // "the well known public key", as the phone encodes it
    if (i >= 0 && enabled_[i] && !names_[i].empty()) {
        const bool has_key = !keys_[i].empty();
        if (has_key) MeshtasticView::derive_channel_key(keys_[i], key);
        // A custom channel of ours is a SECONDARY one: it shares the primary's air and
        // is told apart by the channel hash. So the primary rides along, first, or the
        // far end installs ours as its primary instead of beside it.
        sh.primary_name = meshtastic::PRESET_CHANNEL_NAMES[preset_idx_];
        sh.primary_psk = &default_psk;
        sh.primary_psk_len = 1;
        sh.name = names_[i].c_str();
        sh.psk = key;
        sh.psk_len = has_key ? sizeof(key) : 0;
        qr_url_ = meshtastic::channel_share_url(sh);
        l1 = "Channel " + names_[i];
        l2 = std::string(meshtastic::PRESET_CHANNEL_NAMES[preset_idx_]) +
             (has_key ? "  [encrypted]" : "  [open]");
    } else {
        sh.name = meshtastic::PRESET_CHANNEL_NAMES[preset_idx_];
        sh.psk = &default_psk;
        sh.psk_len = 1;
        qr_url_ = meshtastic::channel_share_url(sh);
        l1 = std::string("Channel ") + meshtastic::PRESET_CHANNEL_NAMES[preset_idx_];
        l2 = "public, well known key";
    }
    nav_.push<MeshtasticQRView>(qr_url_, l1, l2);
}

MeshtasticSetupMenuView::MeshtasticSetupMenuView(Rect parent_rect)
    : View(parent_rect) {
    add_children({&button_profile_, &button_radio_, &button_privacy_,
                  &button_system_, &button_chat_, &disclaimer_});
    button_profile_.on_select = [this](Button&) { if (on_open_) on_open_(0); };
    button_radio_.on_select = [this](Button&) { if (on_open_) on_open_(1); };
    button_privacy_.on_select = [this](Button&) { if (on_open_) on_open_(2); };
    button_system_.on_select = [this](Button&) { if (on_open_) on_open_(3); };
    button_chat_.on_select = [this](Button&) { if (on_open_) on_open_(4); };
}

void MeshtasticSetupMenuView::focus() {
    button_profile_.focus();
}

void MeshtasticSetupMenuView::paint(Painter& painter) {
    const auto r = screen_rect();
    painter.fill_rectangle({r.left(), r.top(), screen_width, r.height()},
                           Theme::getInstance()->bg_darkest->background);
    painter.draw_string({r.left(), r.top() + 8}, *Theme::getInstance()->fg_light,
                        "Settings");
}

// ============================================================================
// Setup pages
// ============================================================================

// Router, Router Client and Router Late are infrastructure roles, and Repeater was
// deprecated upstream in 2.7.11 for tearing holes in the rebroadcast chain. None of them
// is a "stronger node" setting: a Router asks every other node to route through it, and
// a Router never cancels a rebroadcast it has already heard someone else make. Set on a
// radio indoors or on a desk, that is duplicated traffic plus a routing preference
// pointing nowhere - and it is the rest of the mesh that pays, not this node. Three short
// lines, because 240 pixels hold thirty characters.
void MeshProfilePageView::show_role_warning() {
    const uint8_t r = cfg_.node_role;
    const bool router = (r == 2 || r == 3 || r == 11);
    const bool repeater = (r == 4);
    if (!router && !repeater) {
        text_warn0_.set("");
        text_warn1_.set("");
        text_warn2_.set("");
        return;
    }
    if (repeater) {
        text_warn0_.set("Repeater is deprecated: it");
        text_warn1_.set("tears holes in the relay");
        text_warn2_.set("chain. Use Client instead.");
        return;
    }
    text_warn0_.set("Infrastructure role: others");
    text_warn1_.set("route through you, and you");
    text_warn2_.set("always relay. Only if high up.");
}

MeshProfilePageView::MeshProfilePageView(NavigationView& nav, MeshSettings& cfg, MeshSettingsHooks& hooks)
    : MeshSettingsPage(nav, cfg, hooks) {
    add_children({&labels_, &button_name_, &button_short_, &button_id_, &button_rnd_,
                  &field_role_, &field_device_, &field_hw_num_, &button_hw_type_,
                  &button_share_, &text_warn0_, &text_warn1_, &text_warn2_});

    text_warn0_.set_style(Theme::getInstance()->fg_red);
    text_warn1_.set_style(Theme::getInstance()->fg_red);
    text_warn2_.set_style(Theme::getInstance()->fg_red);

    button_share_.on_select = [this](Button&) { if (hooks_.share_me) hooks_.share_me(); };

    button_name_.set_text(cfg_.node_long_name[0] ? cfg_.node_long_name : "Set Name");
    button_name_.on_select = [this](Button&) {
        entry_ = std::string(cfg_.node_long_name);
        mesh_prompt(nav_, entry_, 40, [this](std::string& val) {
            if (val.empty()) return;
            strncpy(cfg_.node_long_name, val.c_str(), 40);
            cfg_.node_long_name[40] = '\0';
            button_name_.set_text(cfg_.node_long_name);
            changed();  // persist the name + re-announce it
        },
                    false);
    };

    button_short_.set_text(cfg_.node_short_name[0] ? cfg_.node_short_name : "Set");
    button_short_.on_select = [this](Button&) {
        entry_ = std::string(cfg_.node_short_name);
        mesh_prompt(nav_, entry_, 4, [this](std::string& val) {
                        if (val.empty()) return;
                        strncpy(cfg_.node_short_name, val.c_str(), 4);
                        cfg_.node_short_name[4] = '\0';
                        button_short_.set_text(cfg_.node_short_name);
                        changed(); }, false);
    };

    button_id_.set_text(to_string_hex(cfg_.node_id, 8));
    button_id_.on_select = [this](Button&) {
        entry_ = to_string_hex(cfg_.node_id, 8);
        mesh_prompt(nav_, entry_, 8, [this](std::string& v) {
            uint32_t id = 0;
            size_t digits = 0;
            for (char c : v) {
                int d = -1;
                if (c >= '0' && c <= '9')
                    d = c - '0';
                else if (c >= 'a' && c <= 'f')
                    d = c - 'a' + 10;
                else if (c >= 'A' && c <= 'F')
                    d = c - 'A' + 10;
                if (d < 0) return;  // one stray character and the whole id is a guess
                id = (id << 4) | static_cast<uint32_t>(d);
                digits++;
            }
            // 0 is "unset" and all-ones is the broadcast address; a node answering to
            // either is a node the mesh cannot address.
            if (!digits || id == 0 || id == meshtastic::BROADCAST_ADDR) return;
            cfg_.node_id = id;
            button_id_.set_text(to_string_hex(cfg_.node_id, 8));
            changed(); }, false);
    };

    button_rnd_.on_select = [this](Button&) {
        // The old seed was the seconds hand alone: sixty possible identities, on a
        // network where two nodes sharing one break each other's routing. Mixed from
        // the whole clock and the id being replaced, and never 0 or broadcast.
        const auto now = rtc_time::now();
        uint32_t x = (static_cast<uint32_t>(now.hour()) << 16) ^
                     (static_cast<uint32_t>(now.minute()) << 8) ^
                     static_cast<uint32_t>(now.second()) ^ cfg_.node_id;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        x *= 0x9e3779b9u;
        if (x == 0 || x == meshtastic::BROADCAST_ADDR) x = 0x0BADC0DEu;
        cfg_.node_id = x;
        button_id_.set_text(to_string_hex(cfg_.node_id, 8));
        changed();
    };

    field_role_.set_by_value(cfg_.node_role);
    field_role_.on_change = [this](size_t, OptionsField::value_t v) {
        cfg_.node_role = static_cast<uint8_t>(v);
        show_role_warning();
        changed();
    };
    show_role_warning();

    // Every board mesh.proto knows, built from the same table the node cards use.
    // Ours leads the list: it is what we are, and what we go back to.
    {
        // The boards people actually hold, not all 142 the protocol knows. Every one of
        // them became a std::string in a vector on the M0's heap, and the page cost
        // 3984 bytes to open - measured. The on-screen keyboard wants 4848 in one piece
        // right after, so "Name:" could not be typed at all: a dropdown nobody scrolls
        // to the end of was spending the memory that entering your own name needs.
        // Anything not listed is still reachable through "or number:" below, and peers'
        // boards are still named in full on their cards - that reads HW_MODELS directly.
        static constexpr uint8_t PICKED[] = {0, 4, 5, 7, 9, 10, 12, 16, 18, 25, 31,
                                             43, 44, 50, 51, 71, 72, 89, 102, 103, 110};
        OptionsField::options_t opts;
        opts.reserve(sizeof(PICKED) + 2);
        opts.emplace_back("HackRF H4M", 255);
        bool have_current = (cfg_.hw_model == 255);
        for (const auto& h : HW_MODELS) {
            bool picked = false;
            for (uint8_t id : PICKED) picked = picked || (h.id == id);
            // ...and whatever this device is set to, so the field can name it even when
            // the number came from the field beside it.
            if (h.id == cfg_.hw_model) {
                picked = true;
                have_current = true;
            }
            if (picked && h.id != 255) opts.emplace_back(h.name, h.id);
        }
        (void)have_current;
        field_device_.set_options(std::move(opts));
    }
    field_device_.set_by_value(cfg_.hw_model);
    field_device_.on_change = [this](size_t, OptionsField::value_t v) {
        cfg_.hw_model = static_cast<uint8_t>(v);
        field_hw_num_.set_value(cfg_.hw_model);
        changed();  // persist + re-announce the spoofed model
    };

    field_hw_num_.set_value(cfg_.hw_model);
    field_hw_num_.on_change = [this](int32_t v) {
        cfg_.hw_model = static_cast<uint8_t>(v);
        field_device_.set_by_value(cfg_.hw_model);
        changed();
    };
    button_hw_type_.on_select = [this](Button&) {
        // Any HardwareModel number, not just the ones in the list above.
        entry_ = to_string_dec_uint(cfg_.hw_model);
        mesh_prompt(nav_, entry_, 3, [this](std::string& v) {
                        uint32_t n = 0;
                        for (char c : v)
                            if (c >= '0' && c <= '9') n = n * 10 + (c - '0');
                        cfg_.hw_model = static_cast<uint8_t>(n > 255 ? 255 : n);
                        field_hw_num_.set_value(cfg_.hw_model);
                        field_device_.set_by_value(cfg_.hw_model);
                        changed(); }, true);
    };
}

// Quarter-wave whip length for the frequency this page is currently configuring.
// Region, preset, slot and the override all move the frequency, so all of them move
// this line too - which is the point of showing it here rather than in a separate
// calculator the user would have to go and find with the number in their head.
void MeshRadioPageView::update_whip() {
    const uint64_t hz = (cfg_.freq_override_hz != 0)
                            ? static_cast<uint64_t>(cfg_.freq_override_hz)
                            : channel_frequency(cfg_.region_idx, cfg_.preset_idx, cfg_.freq_slot,
                                                cfg_.primary_name.c_str());
    if (!hz) {
        text_whip_.set("");
        return;
    }
    // c / f / 4, in TENTHS of a centimetre. c is 29979245800 cm/s, so a quarter wave
    // in tenths of a cm is 299792458000 / (4 f) = 74948114500 / f. Getting this
    // constant wrong by a hundred put 869 MHz at "0.0 cm" - integer division does not
    // round, it just tells you nothing.
    const uint32_t mm10 = static_cast<uint32_t>(74948114500ull / hz);
    text_whip_.set("Whip 1/4 wave: " + to_string_dec_uint(mm10 / 10) + "." +
                   to_string_dec_uint(mm10 % 10) + " cm");
}

MeshRadioPageView::MeshRadioPageView(NavigationView& nav, MeshSettings& cfg, MeshSettingsHooks& hooks)
    : MeshSettingsPage(nav, cfg, hooks) {
    add_children({&labels_, &field_region_, &field_preset_, &field_hops_, &field_cr_,
                  &button_freq_, &field_slot_, &field_nodeinfo_,
                  &check_mqtt_, &check_ignore_mqtt_, &field_txpwr_, &field_txdb_,
                  &text_whip_});
    update_whip();

    field_cr_.set_by_value(cfg_.coding_rate);
    field_cr_.on_change = [this](size_t, OptionsField::value_t v) {
        cfg_.coding_rate = static_cast<uint8_t>(v);
        changed_radio();  // the demodulator has to be told, so the radio restarts
    };

    {
        OptionsField::options_t opts;
        opts.reserve(meshtastic::NUM_REGIONS);
        for (uint8_t i = 0; i < meshtastic::NUM_REGIONS; i++)
            opts.emplace_back(meshtastic::REGIONS[i].name, i);
        field_region_.set_options(std::move(opts));
    }
    field_region_.set_by_value(cfg_.region_idx);
    field_region_.on_change = [this](size_t, OptionsField::value_t v) {
        cfg_.region_idx = static_cast<uint8_t>(v);
        update_whip();
        changed_radio();
    };

    {
        OptionsField::options_t opts;
        opts.reserve(meshtastic::NUM_MODEM_PRESETS);
        for (uint8_t i = 0; i < meshtastic::NUM_MODEM_PRESETS; i++) {
            if (i == meshtastic::PRESET_VERY_LONG_SLOW) continue;  // see mesh_regions.hpp
            opts.emplace_back(meshtastic::MODEM_PRESETS[i].name, i);
        }
        field_preset_.set_options(std::move(opts));
    }
    // Anyone already sitting on it lands on LONG_SLOW, which is what our table gave it
    // anyway - same spreading factor, same bandwidth, and it exists on the other side.
    if (cfg_.preset_idx == meshtastic::PRESET_VERY_LONG_SLOW)
        cfg_.preset_idx = meshtastic::PRESET_LONG_SLOW;
    field_preset_.set_by_value(cfg_.preset_idx);
    field_preset_.on_change = [this](size_t, OptionsField::value_t v) {
        cfg_.preset_idx = static_cast<uint8_t>(v);
        update_whip();
        changed_radio();
    };

    field_hops_.set_value(cfg_.hop_limit);
    field_hops_.on_change = [this](int32_t v) {
        cfg_.hop_limit = static_cast<uint8_t>(v);
        changed();
    };

    button_freq_.set_text(fmt_freq_mhz(cfg_.freq_override_hz));
    button_freq_.on_select = [this](Button&) {
        // Megahertz, e.g. "868.825"; empty or zero hands it back to the region.
        entry_.clear();
        mesh_prompt(nav_, entry_, 10, [this](std::string& val) {
                        // Parsed by hand. std::stod throws on the first letter, and an
                        // exception here halts the system: this firmware has no handler
                        // for one. Typing a letter into a frequency box is a slip, not
                        // grounds for taking the radio down.
                        //
                        // "868.825" and "868" both work, in megahertz. Anything else -
                        // a letter, two dots, a number the radio cannot reach - leaves
                        // the setting exactly as it was.
                        if (val.empty() || val == "0") {
                            cfg_.freq_override_hz = 0;
                        } else {
                            uint32_t mhz = 0, frac = 0, fdigits = 0;
                            size_t i = 0;
                            bool ok = false;
                            for (; i < val.size() && val[i] >= '0' && val[i] <= '9'; i++) {
                                mhz = mhz * 10 + static_cast<uint32_t>(val[i] - '0');
                                ok = true;
                                if (mhz > 6000) return;  // past what the radio can tune
                            }
                            if (i < val.size() && val[i] == '.') {
                                i++;
                                for (; i < val.size() && val[i] >= '0' && val[i] <= '9'; i++) {
                                    if (fdigits < 6) {
                                        frac = frac * 10 + static_cast<uint32_t>(val[i] - '0');
                                        fdigits++;
                                    }
                                    ok = true;
                                }
                            }
                            if (!ok || i != val.size()) return;  // trailing junk
                            uint32_t scale = 1000000;
                            for (uint32_t k = 0; k < fdigits; k++) scale /= 10;
                            const uint32_t hz = mhz * 1000000u + frac * scale;
                            if (hz < 1000000u) return;  // below the tuner's floor
                            cfg_.freq_override_hz = hz;
                        }
                        button_freq_.set_text(fmt_freq_mhz(cfg_.freq_override_hz));
                        update_whip();
                        changed_radio(); }, false);
    };

    field_slot_.set_value(cfg_.freq_slot);
    field_slot_.on_change = [this](int32_t v) {
        cfg_.freq_slot = static_cast<uint8_t>(v);
        update_whip();
        changed_radio();
    };

    field_nodeinfo_.set_value(cfg_.nodeinfo_min);
    field_nodeinfo_.on_change = [this](int32_t v) {
        cfg_.nodeinfo_min = static_cast<uint32_t>(v);
        changed();
    };

    // Transmit power. The numeric field only means anything in Custom, so it shows only
    // there rather than sitting greyed out inviting a click that does nothing.
    field_txpwr_.set_by_value(cfg_.tx_pwr_mode);
    field_txdb_.set_value(cfg_.tx_pwr_db);
    field_txdb_.hidden(cfg_.tx_pwr_mode != 2);
    field_txpwr_.on_change = [this](size_t, OptionsField::value_t v) {
        cfg_.tx_pwr_mode = static_cast<uint8_t>(v);
        field_txdb_.hidden(cfg_.tx_pwr_mode != 2);
        set_dirty();
        changed();
    };
    field_txdb_.on_change = [this](int32_t v) {
        cfg_.tx_pwr_db = static_cast<uint8_t>(v);
        changed();
    };

    check_mqtt_.set_value(cfg_.mqtt_ok);
    check_mqtt_.on_select = [this](Checkbox&, bool v) { cfg_.mqtt_ok = v; changed(); };

    check_ignore_mqtt_.set_value(cfg_.ignore_mqtt);
    check_ignore_mqtt_.on_select = [this](Checkbox&, bool v) { cfg_.ignore_mqtt = v; changed(); };
}

const uint8_t MeshPrivacyPageView::RAND_BIT[MeshPrivacyPageView::NBITS] = {
    MeshSettings::RAND_ID, MeshSettings::RAND_NAME, MeshSettings::RAND_ROLE,
    MeshSettings::RAND_DEV, MeshSettings::RAND_POS, MeshSettings::RAND_TEL};

MeshPrivacyPageView::MeshPrivacyPageView(NavigationView& nav, MeshSettings& cfg, MeshSettingsHooks& hooks)
    : MeshSettingsPage(nav, cfg, hooks) {
    add_children({&labels_, &check_receipts_, &check_pki_, &check_listen_, &check_stats_,
                  &button_rand_, &button_resend_,
                  &check_r_id_, &check_r_name_, &check_r_role_,
                  &check_r_dev_, &check_r_pos_, &check_r_tel_});
    Checkbox* boxes[NBITS] = {&check_r_id_, &check_r_name_, &check_r_role_,
                              &check_r_dev_, &check_r_pos_, &check_r_tel_};

    check_receipts_.set_value(cfg_.send_read_receipts);
    check_receipts_.on_select = [this](Checkbox&, bool v) { cfg_.send_read_receipts = v; changed(); };

    check_pki_.set_value(cfg_.pki_enabled);
    check_pki_.on_select = [this](Checkbox&, bool v) { cfg_.pki_enabled = v; changed(); };

    check_listen_.set_value(cfg_.listen_only);
    check_listen_.on_select = [this](Checkbox&, bool v) {
        cfg_.listen_only = v;
        // A node's public key travels in one place only: the NodeInfo it broadcasts.
        // Silencing beacons therefore stops peers ever learning the key, and encrypted
        // private messages start failing in both directions with no visible connection
        // to the setting that caused it. Say it here, where the choice is made.
        if (v && cfg_.pki_enabled)
            if (g_chat)
                g_chat->write_console(
                    "* beacons off: peers cannot learn your key,\n"
                    "  so encrypted DMs will stop working\n");
        changed();
    };

    check_stats_.set_value(cfg_.answer_stats);
    check_stats_.on_select = [this](Checkbox&, bool v) { cfg_.answer_stats = v; changed(); };

    button_rand_.set_text(cfg_.rand_min ? (to_string_dec_uint(cfg_.rand_min) + " min") : "off");
    button_rand_.on_select = [this](Button&) {
        // Free entry: a fixed step would make the moments we change identity
        // predictable, which rather defeats the point.
        entry_ = to_string_dec_uint(cfg_.rand_min);
        mesh_prompt(nav_, entry_, 4, [this](std::string& v) {
                        cfg_.rand_min = static_cast<uint32_t>(atoi(v.c_str()));
                        button_rand_.set_text(
                            cfg_.rand_min ? (to_string_dec_uint(cfg_.rand_min) + " min") : "off");
                        changed(); }, true);
    };

    for (size_t i = 0; i < NBITS; i++) {
        const uint8_t bit = RAND_BIT[i];
        boxes[i]->set_value(cfg_.rand_mask & bit);
        boxes[i]->on_select = [this, bit](Checkbox&, bool v) {
            cfg_.rand_mask = v ? (cfg_.rand_mask | bit)
                               : static_cast<uint8_t>(cfg_.rand_mask & ~bit);
            changed();
        };
    }

    button_resend_.on_select = [this](Button&) { if (hooks_.resend) hooks_.resend(); };
}

const uint8_t MeshSystemPageView::ECHO_BIT[MeshSystemPageView::NBITS] = {
    MeshSettings::ECHO_TEXT, MeshSettings::ECHO_SIG,
    MeshSettings::ECHO_MEM, MeshSettings::ECHO_UP, MeshSettings::ECHO_BUILD};

// The mode is not a setting of its own - it is read back from the two that already
// exist, so nothing new has to be stored or kept in step. An empty keyword answers
// everything; the bell emoji as the keyword answers bells and nothing else, because
// that is exactly what a bell carries.
uint8_t MeshSystemPageView::echo_mode() const {
    if (!cfg_.echo_enabled) return ECHO_OFF;
    if (cfg_.echo_key.empty()) return ECHO_ANY;
    if (cfg_.echo_key == BELL_EMOJI) return ECHO_ONLY_BELL;
    return ECHO_CUSTOM;
}

void MeshSystemPageView::apply_echo_mode(uint8_t mode) {
    cfg_.echo_enabled = (mode != ECHO_OFF);
    if (mode == ECHO_ANY)
        cfg_.echo_key.clear();
    else if (mode == ECHO_ONLY_BELL)
        cfg_.echo_key = BELL_EMOJI;
    else if (mode == ECHO_CUSTOM && (cfg_.echo_key.empty() || cfg_.echo_key == BELL_EMOJI))
        cfg_.echo_key = "test";  // something to edit rather than an empty button
    button_echo_key_.hidden(mode != ECHO_CUSTOM);
    button_echo_key_.set_text(cfg_.echo_key);
}

MeshSystemPageView::MeshSystemPageView(NavigationView& nav, MeshSettings& cfg, MeshSettingsHooks& hooks)
    : MeshSettingsPage(nav, cfg, hooks) {
    add_children({&labels_, &check_log_, &check_sf_, &field_sf_max_, &field_sf_ttl_,
                  &check_key_repeat_, &field_echo_, &button_echo_key_,
                  &check_e_text_, &check_e_sig_,
                  &check_e_mem_, &check_e_up_, &check_e_build_, &field_nbr_});
    Checkbox* boxes[NBITS] = {&check_e_text_, &check_e_sig_,
                              &check_e_mem_, &check_e_up_, &check_e_build_};

    check_log_.set_value(cfg_.logging);
    check_log_.on_select = [this](Checkbox&, bool v) { cfg_.logging = v; changed(); };

    check_sf_.set_value(cfg_.sf_enabled);
    check_sf_.on_select = [this](Checkbox&, bool v) { cfg_.sf_enabled = v; changed(); };

    field_sf_max_.set_value(static_cast<int32_t>(cfg_.sf_max));
    field_sf_max_.on_change = [this](int32_t v) { cfg_.sf_max = static_cast<uint32_t>(v); changed(); };

    field_sf_ttl_.set_value(static_cast<int32_t>(cfg_.sf_ttl_min));
    field_sf_ttl_.on_change = [this](int32_t v) { cfg_.sf_ttl_min = static_cast<uint32_t>(v); changed(); };

    check_key_repeat_.set_value(cfg_.key_repeat);
    check_key_repeat_.on_select = [this](Checkbox&, bool v) { cfg_.key_repeat = v; changed(); };

    apply_echo_mode(echo_mode());
    field_echo_.set_by_value(echo_mode());
    field_echo_.on_change = [this](size_t, OptionsField::value_t v) {
        apply_echo_mode(static_cast<uint8_t>(v));
        changed();
    };

    button_echo_key_.on_select = [this](Button&) {
        mesh_prompt(nav_, cfg_.echo_key, 12, [this](std::string& v) {
                        if (v.empty()) v = "test";  // "custom" with no text is "any text"
                        button_echo_key_.set_text(v);
                        changed(); }, false);
    };

    field_nbr_.set_value(static_cast<int32_t>(cfg_.nbr_min));
    field_nbr_.on_change = [this](int32_t v) {
        cfg_.nbr_min = static_cast<uint32_t>(v);
        changed();
    };

    for (size_t i = 0; i < NBITS; i++) {
        const uint8_t bit = ECHO_BIT[i];
        boxes[i]->set_value(cfg_.echo_mask & bit);
        boxes[i]->on_select = [this, bit](Checkbox&, bool v) {
            cfg_.echo_mask = v ? (cfg_.echo_mask | bit)
                               : static_cast<uint8_t>(cfg_.echo_mask & ~bit);
            changed();
        };
    }
}

MeshChatPageView::MeshChatPageView(NavigationView& nav, MeshSettings& cfg, MeshSettingsHooks& hooks)
    : MeshSettingsPage(nav, cfg, hooks) {
    add_children({&labels_, &field_time_, &field_names_, &check_db_,
                  &check_beep_, &check_sf_notify_, &check_bell_, &check_glyphs_, &field_save_,
                  &field_retries_, &field_retry_s_, &button_clear_, &field_font_,
                  &field_lines_});

    field_lines_.set_value(static_cast<int32_t>(cfg_.chat.hist_lines));
    field_lines_.on_change = [this](int32_t v) {
        cfg_.chat.hist_lines = static_cast<uint32_t>(v);
        changed();
    };

    field_time_.set_by_value(cfg_.chat.time_mode);
    field_time_.on_change = [this](size_t, OptionsField::value_t v) {
        cfg_.chat.time_mode = static_cast<uint8_t>(v);
        changed();
    };

    field_names_.set_by_value(cfg_.chat.name_mode);
    field_names_.on_change = [this](size_t, OptionsField::value_t v) {
        cfg_.chat.name_mode = static_cast<uint8_t>(v);
        changed();
    };

    check_db_.set_value(cfg_.chat.show_db);
    check_db_.on_select = [this](Checkbox&, bool v) { cfg_.chat.show_db = v; changed(); };

    check_beep_.set_value(cfg_.beep_on_rx);
    check_beep_.on_select = [this](Checkbox&, bool v) { cfg_.beep_on_rx = v; changed(); };

    check_sf_notify_.set_value(cfg_.chat.sf_notify);
    check_sf_notify_.on_select = [this](Checkbox&, bool v) { cfg_.chat.sf_notify = v; changed(); };
    // Takes effect on the next launch: the table is read once, at startup, and
    // dropping it out from under a chat that is already drawing with it would mean
    // repainting every line that used it.
    check_glyphs_.set_value(cfg_.chat.glyph_font);
    check_glyphs_.on_select = [this](Checkbox&, bool v) { cfg_.chat.glyph_font = v; changed(); };

    check_bell_.set_value(cfg_.chat.bell_alert);
    check_bell_.on_select = [this](Checkbox&, bool v) { cfg_.chat.bell_alert = v; changed(); };

    field_save_.set_value(static_cast<int32_t>(cfg_.chat.save_count));
    field_save_.on_change = [this](int32_t v) {
        cfg_.chat.save_count = static_cast<uint32_t>(v);
        changed();
    };

    field_retries_.set_value(static_cast<int32_t>(cfg_.chat.tx_retries));
    field_retries_.on_change = [this](int32_t v) {
        cfg_.chat.tx_retries = static_cast<uint32_t>(v);
        changed();
    };

    field_retry_s_.set_value(static_cast<int32_t>(cfg_.chat.tx_retry_s));
    field_retry_s_.on_change = [this](int32_t v) {
        cfg_.chat.tx_retry_s = static_cast<uint32_t>(v);
        changed();
    };

    button_clear_.on_select = [this](Button&) {
        if (hooks_.clear_history) hooks_.clear_history();
    };

    field_font_.set_by_value(cfg_.chat.font_size);
    field_font_.on_change = [this](size_t, OptionsField::value_t v) {
        cfg_.chat.font_size = static_cast<uint8_t>(v);
        set_dirty();  // redraw the "Aa" sample in the chosen face
        changed();
    };
}

void MeshChatPageView::paint(Painter& painter) {
    View::paint(painter);
    const auto r = screen_rect();
    const Coord x = r.left() + 13 * 8;
    const Coord y = r.top() + S6;
    const auto& fg = *Theme::getInstance()->fg_light;
    painter.fill_rectangle({x, y - 2, 12 * 8, 20},
                           Theme::getInstance()->bg_darkest->background);
    const bool small = (cfg_.chat.font_size < 12);
    painter.draw_string({x, y + (small ? 4 : 0)},
                        {small ? font::fixed_5x8 : font::fixed_8x16,
                         fg.background, fg.foreground},
                        "Aa");
    painter.draw_string({x + 4 * 8, y}, fg, small ? "5x8" : "8x16");
}
// ============================================================================
// MeshtasticView (main controller)
// ============================================================================

// Every setting this app keeps on the card: its name on disk, where it lives inside
// MeshSettings, and what type it is. A table rather than eighty-five lines of code -
// written out one call at a time it compiled to 2.8 KB, which on a firmware with a
// kilobyte of head room is not a price worth paying for the same list.
// MeshSettings is a plain struct with no bases and no virtuals, so offsetof applies.
namespace {
enum class K : uint8_t { U8,
                         U32,
                         I32,
                         BOOL,
                         FLOAT,
                         STR };
struct SettingDef {
    std::string_view name;
    uint16_t offset;
    K kind;
};
const SettingDef SETTING_DEFS[] = {
    {"region"sv, offsetof(MeshSettings, region_idx), K::U8},
    {"preset"sv, offsetof(MeshSettings, preset_idx), K::U8},
    {"n_long"sv, offsetof(MeshSettings, node_long_str), K::STR},
    {"n_short"sv, offsetof(MeshSettings, node_short_str), K::STR},
    {"node_id"sv, offsetof(MeshSettings, node_id), K::U32},
    {"role"sv, offsetof(MeshSettings, node_role), K::U8},
    {"gps_en"sv, offsetof(MeshSettings, gps_enabled), K::BOOL},
    {"hops"sv, offsetof(MeshSettings, hop_limit), K::U8},
    {"mqtt_ok"sv, offsetof(MeshSettings, mqtt_ok), K::BOOL},
    {"hw_model"sv, offsetof(MeshSettings, hw_model), K::U8},
    {"log"sv, offsetof(MeshSettings, logging), K::BOOL},
    {"rd_ack"sv, offsetof(MeshSettings, send_read_receipts), K::BOOL},
    {"listen"sv, offsetof(MeshSettings, listen_only), K::BOOL},
    {"astats"sv, offsetof(MeshSettings, answer_stats), K::BOOL},
    {"pchname"sv, offsetof(MeshSettings, primary_name), K::STR},
    {"rxlna"sv, offsetof(MeshSettings, rx_lna), K::U8},
    {"rxvga"sv, offsetof(MeshSettings, rx_vga), K::U8},
    {"rxamp"sv, offsetof(MeshSettings, rx_amp), K::U8},
    {"t_upm"sv, offsetof(MeshSettings, telemetry.up_mode), K::U8},
    {"t_uph"sv, offsetof(MeshSettings, telemetry.up_hours), K::U32},
    {"fix_lat"sv, offsetof(MeshSettings, fixed_lat), K::FLOAT},
    {"fix_lon"sv, offsetof(MeshSettings, fixed_lon), K::FLOAT},
    {"freq_hz"sv, offsetof(MeshSettings, freq_override_hz), K::U32},
    {"rand_min"sv, offsetof(MeshSettings, rand_min), K::U32},
    {"rand_mask"sv, offsetof(MeshSettings, rand_mask), K::U8},
    {"beep_rx"sv, offsetof(MeshSettings, beep_on_rx), K::BOOL},
    {"pki_en"sv, offsetof(MeshSettings, pki_enabled), K::BOOL},
    {"pki_priv"sv, offsetof(MeshSettings, pki_priv_hex), K::STR},
    {"fslot"sv, offsetof(MeshSettings, freq_slot), K::U8},
    {"cr"sv, offsetof(MeshSettings, coding_rate), K::U8},
    {"txpm"sv, offsetof(MeshSettings, tx_pwr_mode), K::U8},
    {"txpd"sv, offsetof(MeshSettings, tx_pwr_db), K::U8},
    {"hline"sv, offsetof(MeshSettings, chat.hist_lines), K::U32},
    {"niv_min"sv, offsetof(MeshSettings, nodeinfo_min), K::U32},
    {"nbr_min"sv, offsetof(MeshSettings, nbr_min), K::U32},
    {"ign_mqtt"sv, offsetof(MeshSettings, ignore_mqtt), K::BOOL},
    {"sf_en"sv, offsetof(MeshSettings, sf_enabled), K::BOOL},
    {"sf_max"sv, offsetof(MeshSettings, sf_max), K::U32},
    {"sf_ttl"sv, offsetof(MeshSettings, sf_ttl_min), K::U32},
    {"keyrep"sv, offsetof(MeshSettings, key_repeat), K::BOOL},
    {"radiomode"sv, offsetof(MeshSettings, radio_mode), K::U8},
    {"echo"sv, offsetof(MeshSettings, echo_enabled), K::BOOL},
    {"echo_mask"sv, offsetof(MeshSettings, echo_mask), K::U8},
    {"echo_key"sv, offsetof(MeshSettings, echo_key), K::STR},
    {"c_time"sv, offsetof(MeshSettings, chat.time_mode), K::U8},
    {"c_name"sv, offsetof(MeshSettings, chat.name_mode), K::U8},
    {"c_db"sv, offsetof(MeshSettings, chat.show_db), K::BOOL},
    {"c_tech"sv, offsetof(MeshSettings, chat.show_tech), K::BOOL},
    {"c_save"sv, offsetof(MeshSettings, chat.save_count), K::U32},
    {"c_glyph"sv, offsetof(MeshSettings, chat.glyph_font), K::BOOL},
    {"c_sfnote"sv, offsetof(MeshSettings, chat.sf_notify), K::BOOL},
    {"c_bell"sv, offsetof(MeshSettings, chat.bell_alert), K::BOOL},
    {"c_fsize"sv, offsetof(MeshSettings, chat.font_size), K::U8},
    {"c_retry"sv, offsetof(MeshSettings, chat.tx_retries), K::U32},
    {"c_retrys"sv, offsetof(MeshSettings, chat.tx_retry_s), K::U32},
    {"t_bmode"sv, offsetof(MeshSettings, telemetry.batt_mode), K::U8},
    {"t_bpct"sv, offsetof(MeshSettings, telemetry.batt_pct), K::U8},
    {"t_vmode"sv, offsetof(MeshSettings, telemetry.volt_mode), K::U8},
    {"t_vdv"sv, offsetof(MeshSettings, telemetry.volt_dV), K::U8},
    {"t_umode"sv, offsetof(MeshSettings, telemetry.util_mode), K::U8},
    {"t_alt"sv, offsetof(MeshSettings, telemetry.fixed_alt_m), K::I32},
    {"t_sats"sv, offsetof(MeshSettings, telemetry.fixed_sats), K::U8},
    {"t_extra"sv, offsetof(MeshSettings, extra_pack), K::STR},
    {"t_uch"sv, offsetof(MeshSettings, telemetry.chutil_pct), K::U8},
    {"t_uair"sv, offsetof(MeshSettings, telemetry.airutil_pct), K::U8},
    {"t_posmin"sv, offsetof(MeshSettings, telemetry.pos_min), K::U8},
    {"t_telmin"sv, offsetof(MeshSettings, telemetry.tel_min), K::U8},
    {"c1_name"sv, offsetof(MeshSettings, ch_name[0]), K::STR},
    {"c1_key"sv, offsetof(MeshSettings, ch_key[0]), K::STR},
    {"c1_en"sv, offsetof(MeshSettings, ch_enabled[0]), K::BOOL},
    {"c2_name"sv, offsetof(MeshSettings, ch_name[1]), K::STR},
    {"c2_key"sv, offsetof(MeshSettings, ch_key[1]), K::STR},
    {"c2_en"sv, offsetof(MeshSettings, ch_enabled[1]), K::BOOL},
    {"c3_name"sv, offsetof(MeshSettings, ch_name[2]), K::STR},
    {"c3_key"sv, offsetof(MeshSettings, ch_key[2]), K::STR},
    {"c3_en"sv, offsetof(MeshSettings, ch_enabled[2]), K::BOOL},
    {"c4_name"sv, offsetof(MeshSettings, ch_name[3]), K::STR},
    {"c4_key"sv, offsetof(MeshSettings, ch_key[3]), K::STR},
    {"c4_en"sv, offsetof(MeshSettings, ch_enabled[3]), K::BOOL},
    {"c5_name"sv, offsetof(MeshSettings, ch_name[4]), K::STR},
    {"c5_key"sv, offsetof(MeshSettings, ch_key[4]), K::STR},
    {"c5_en"sv, offsetof(MeshSettings, ch_enabled[4]), K::BOOL},
    {"c6_name"sv, offsetof(MeshSettings, ch_name[5]), K::STR},
    {"c6_key"sv, offsetof(MeshSettings, ch_key[5]), K::STR},
    {"c6_en"sv, offsetof(MeshSettings, ch_enabled[5]), K::BOOL},
    {"c7_name"sv, offsetof(MeshSettings, ch_name[6]), K::STR},
    {"c7_key"sv, offsetof(MeshSettings, ch_key[6]), K::STR},
    {"c7_en"sv, offsetof(MeshSettings, ch_enabled[6]), K::BOOL},
    {"c8_name"sv, offsetof(MeshSettings, ch_name[7]), K::STR},
    {"c8_key"sv, offsetof(MeshSettings, ch_key[7]), K::STR},
    {"c8_en"sv, offsetof(MeshSettings, ch_enabled[7]), K::BOOL},
    {"c9_name"sv, offsetof(MeshSettings, ch_name[8]), K::STR},
    {"c9_key"sv, offsetof(MeshSettings, ch_key[8]), K::STR},
    {"c9_en"sv, offsetof(MeshSettings, ch_enabled[8]), K::BOOL},
    {"c10_name"sv, offsetof(MeshSettings, ch_name[9]), K::STR},
    {"c10_key"sv, offsetof(MeshSettings, ch_key[9]), K::STR},
    {"c10_en"sv, offsetof(MeshSettings, ch_enabled[9]), K::BOOL},
    {"act_ch"sv, offsetof(MeshSettings, active_channel), K::U8},
};
}  // namespace

__attribute__((noinline)) SettingBindings MeshtasticView::settings_bindings() {
    SettingBindings b;
    b.reserve(sizeof(SETTING_DEFS) / sizeof(SETTING_DEFS[0]));
    auto* base = reinterpret_cast<uint8_t*>(&cfg_);
    for (const auto& d : SETTING_DEFS) {
        void* p = base + d.offset;
        switch (d.kind) {
            case K::U8:
                b.emplace_back(d.name, static_cast<uint8_t*>(p));
                break;
            case K::U32:
                b.emplace_back(d.name, static_cast<uint32_t*>(p));
                break;
            case K::I32:
                b.emplace_back(d.name, static_cast<int32_t*>(p));
                break;
            case K::BOOL:
                b.emplace_back(d.name, static_cast<bool*>(p));
                break;
            case K::FLOAT:
                b.emplace_back(d.name, static_cast<float*>(p));
                break;
            case K::STR:
                b.emplace_back(d.name, static_cast<std::string*>(p));
                break;
        }
    }
    return b;
}

MeshtasticView::MeshtasticView(NavigationView& nav)
    : nav_(nav),
      chat_view_(nav, router_, node_db_, {0, 3 * 8, screen_width, screen_height - 40}),
      nodes_view_(nav, node_db_, {0, 3 * 8, screen_width, screen_height - 40}),
      map_view_(nav, node_db_, {0, 3 * 8, screen_width, screen_height - 40}),
      setup_menu_({0, 3 * 8, screen_width, screen_height - 40}) {
    add_children({&tab_view_,
                  &chat_view_, &nodes_view_, &map_view_, &setup_menu_});

    nodes_view_.hidden(true);
    map_view_.hidden(true);
    setup_menu_.hidden(true);

    // First run: give this device an identity of its own. The default used to be
    // 0xDEADBEEF for everybody, so any two fresh installs were the same node as far as
    // the mesh could tell - two nodes sharing an address break each other's routing,
    // and the phantom peers that leaves behind take hours to age out of everyone's
    // node list. Named after it too, the way Meshtastic names a node from its address.
    if (cfg_.node_id == 0xDEADBEEFu || cfg_.node_id == 0 ||
        cfg_.node_id == meshtastic::BROADCAST_ADDR) {
        const auto now = rtc_time::now();
        uint32_t x = (static_cast<uint32_t>(now.year()) << 26) ^
                     (static_cast<uint32_t>(now.day()) << 21) ^
                     (static_cast<uint32_t>(now.hour()) << 16) ^
                     (static_cast<uint32_t>(now.minute()) << 8) ^
                     static_cast<uint32_t>(now.second()) ^ chCoreStatus();
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        x *= 0x9e3779b9u;
        if (x == 0 || x == meshtastic::BROADCAST_ADDR) x = 0x0BADC0DEu;
        cfg_.node_id = x;
        const std::string tail = to_string_hex(x & 0xFFFF, 4);
        cfg_.node_long_str = "Mesh-" + tail;
        cfg_.node_short_str = tail;
    }

    // Restore saved node names (SettingsManager loaded them into the std::string
    // mirrors from SD) into the char[] working copies the UI + NodeInfo builder use.
    strncpy(cfg_.node_long_name, cfg_.node_long_str.c_str(), sizeof(cfg_.node_long_name) - 1);
    cfg_.node_long_name[sizeof(cfg_.node_long_name) - 1] = '\0';
    strncpy(cfg_.node_short_name, cfg_.node_short_str.c_str(), sizeof(cfg_.node_short_name) - 1);
    cfg_.node_short_name[sizeof(cfg_.node_short_name) - 1] = '\0';
    unpack_extras();  // the custom telemetry fields ride in one settings string
    // Earlier builds stored a size index here rather than a height in pixels; anything
    // below the smallest face is one of those and means "the normal size".
    // Older builds stored a size index here, and one build a free pixel height; both
    // collapse onto the two sizes that exist.
    cfg_.chat.font_size = (cfg_.chat.font_size < 12) ? 8 : 16;

    // One press, one step: with repeat on, holding a key a moment too long runs a menu
    // past where you wanted it. Restored on the way out so other apps keep their own
    // behaviour.
    saved_repeat_ = get_switches_repeat_config();
    if (!cfg_.key_repeat) set_switches_repeat_config(SwitchesState{});
    // The settings buttons were captioned at construction (before this sync), so
    // re-caption them now with the persisted identity instead of the defaults.

    router_.set_local_node(cfg_.node_id, DEFAULT_PSK, sizeof(DEFAULT_PSK));
    // Reading the key file, deriving the channel table and starting the receiver all
    // happen on the first timer tick instead - see start_deferred(). Every one of them
    // reaches the card or the baseband, a kilobyte and a half of stack below whatever
    // called us, and this constructor may itself be running inside the launcher's touch
    // dispatch. The timer calls them from the top of the event loop, where the whole
    // 4 KB is free.
    // Seed packet ids from wall time: peers cache (from, id) pairs for dedup,
    // so restarting the counter at 1 every boot makes them silently drop our
    // first messages as replays of the previous run.
    {
        const auto now = rtc_time::now();
        const uint32_t secs = ((now.day() * 24u + now.hour()) * 60u + now.minute()) * 60u + now.second();
        router_.seed_packet_id((secs << 6) ^ cfg_.node_id);
    }

    router_.set_on_packet([this](const MeshPacket& pkt) {
        // Incoming ROUTING ACK/NAK for one of our sent messages -> update its dot.
        if (pkt.data.portnum == PortNum::ROUTING && pkt.data.request_id != 0) {
            const uint32_t err = routing_error(pkt.data);
            chat_view_.on_ack(pkt.data.request_id, err == 0);
            // A refusal ends the attempt as surely as an acknowledgement does: the far
            // side answered, and it said no. Retrying was leaving a counter ticking
            // beside a message already marked failed, which reads as a contradiction
            // and is one. The reason is named, because "red" on its own tells nobody
            // whether to wait, move closer, or fix a key.
            stop_retry(pkt.data.request_id);
            if (err != 0)
                chat_view_.write_console("* refused by " +
                                         to_string_hex(pkt.header.from & 0xFFFF, 4) + ": " +
                                         routing_error_name(err) + "\n");
        }

        // A LocalStats answer can take many seconds over LoRa: say so when it lands.
        if (pkt.data.portnum == PortNum::TELEMETRY &&
            node_db_.last_stats().node_id == pkt.header.from)
            chat_view_.write_console("* stats from " +
                                     to_string_hex(pkt.header.from & 0xFFFF, 4) + "\n");

        // Set our clock from the mesh, once, and only while it is still sitting at the
        // 1980 the RTC powers up with - never over a time the user set themselves.
        // A peer's own wall clock rides in POSITION field 4 and Telemetry field 1;
        // both are already decoded, and either will do.
        if (!rtc_is_set()) {
            uint32_t peer_time = 0;
            if (pkt.data.portnum == PortNum::POSITION) {
                // Read it back off the node rather than decoding again here: the
                // database is filled before this callback runs.
                if (const NodeEntry* e = node_db_.find(pkt.header.from))
                    peer_time = e->position.timestamp;
            } else if (pkt.data.portnum == PortNum::TELEMETRY) {
                peer_time = node_db_.last_telemetry().timestamp;
            }
            // Sanity: after 2024 and this side of 2100, so one corrupt packet cannot
            // throw the clock into the next century.
            if (peer_time > 1704067200u && peer_time < 4102444800u) {
                rtc::RTC t;
                unix_to_rtc(peer_time, t);
                rtc_time::set(t);
                chat_view_.write_console("* clock set from " +
                                         to_string_hex(pkt.header.from & 0xFFFF, 4) + "\n");
            }
        }

        // "Trace route" in the phone app: a RouteDiscovery addressed to us, asking which
        // path reached us. A reply is expected from every node, and without one the app
        // just times out - which is what it was doing. Noted here, answered in on_timer.
        if (pkt.data.portnum == PortNum::TRACEROUTE && pkt.header.to == cfg_.node_id &&
            pkt.data.want_response && !pkt.data.request_id) {
            trace_dest_ = pkt.header.from;
            trace_req_id_ = pkt.header.packet_id;
            trace_snr_ = pkt.rx_snr;
            trace_ch_ = pkt.channel_index;
            const uint8_t hs = pkt.header.hop_start(), hl = pkt.header.hop_limit();
            trace_hops_ = (hs > hl) ? static_cast<uint8_t>(hs - hl) : 0;
            // Copied whole or not at all: half a RouteDiscovery is a corrupt one, and
            // an answer carrying only our own SNR still tells the asker we are here.
            trace_len_ = (pkt.data.payload_len <= sizeof(trace_buf_))
                             ? static_cast<uint8_t>(pkt.data.payload_len)
                             : 0;
            memcpy(trace_buf_, pkt.data.payload, trace_len_);
        }

        // The other direction: a RouteDiscovery carrying a request_id is the answer to
        // our own "Trace". Field 1 lists the nodes the packet crossed on the way out.
        // A stock node packs that list (tag 0x0A, nanopb's default for a repeated
        // scalar); ours writes an entry at a time (0x0D). Both are valid protobuf and
        // both turn up on the air, so both are read here. Anything else ends the walk:
        // a field of unknown length cannot be stepped over safely.
        if (pkt.data.portnum == PortNum::TRACEROUTE && pkt.header.to == cfg_.node_id &&
            pkt.data.request_id) {
            std::string path;
            size_t hops = 0;
            uint16_t via[3]{};
            const uint8_t* p = pkt.data.payload;
            const uint8_t* const end = p + pkt.data.payload_len;
            while (p < end) {
                size_t n = 0;
                if (*p == 0x0D) {  // field 1, one fixed32
                    p++;
                    n = 4;
                } else if (*p == 0x0A && end - p >= 2) {  // field 1, packed fixed32
                    n = p[1];
                    p += 2;
                } else {
                    break;
                }
                if (static_cast<size_t>(end - p) < n) break;
                for (; n >= 4; n -= 4, p += 4) {
                    const uint32_t id = p[0] | (p[1] << 8) | (p[2] << 16) |
                                        (static_cast<uint32_t>(p[3]) << 24);
                    hops++;
                    // Three fit the console's thirty columns; the count above the path
                    // still tells you if there were more.
                    if (hops <= 3) {
                        via[hops - 1] = static_cast<uint16_t>(id & 0xFFFF);
                        path += (hops > 1 ? " " : "") + to_string_hex(id & 0xFFFF, 4);
                    }
                }
            }
            // Kept as well as printed: the chat scrolls away, and the point of a route
            // is what it looked like last time.
            node_db_.add_route(pkt.header.from, uptime_ticks_, via,
                               static_cast<uint8_t>(hops > 3 ? 3 : hops));
            chat_view_.write_console(
                "* route " + to_string_hex(pkt.header.from & 0xFFFF, 4) + ": " +
                (hops ? to_string_dec_uint(hops) + " via " + path : std::string("direct")) +
                "\n");
        }

        // The phone app's "exchange user info" and "request position" are a NodeInfo and
        // a Position carrying want_response. Both were being received and stored and
        // neither was answered, so the phone sat waiting until it gave up. The NodeInfo
        // reply matters twice over: it is the only place our public key travels, and
        // without that key there can be no encrypted direct message either way.
        // Answered even with beacons off - a reply to a direct question is not a
        // beacon - but not in listen-only, which means exactly that.
        if (pkt.data.want_response && pkt.header.to == cfg_.node_id && !cfg_.listen_only &&
            (pkt.data.portnum == PortNum::NODEINFO || pkt.data.portnum == PortNum::POSITION)) {
            intro_dest_ = pkt.header.from;
            intro_pos_ = (pkt.data.portnum == PortNum::POSITION);
        }

        // A node (or a phone through one) asking us for telemetry gets an answer, but not
        // from here: reading the fuel gauge over i2c, encrypting and queueing a packet is
        // far too much work for the packet callback. Note who asked; the timer replies.
        if (pkt.data.portnum == PortNum::TELEMETRY && pkt.data.want_response &&
            pkt.header.to == cfg_.node_id && cfg_.answer_stats && !cfg_.listen_only) {
            metrics_dest_ = pkt.header.from;
            metrics_req_id_ = pkt.header.packet_id;  // echoed back so the asker matches it
            // Telemetry.variant is a oneof, and a request carries an EMPTY copy of the
            // one it wants: field 6 asks for LocalStats, anything else (or an empty
            // Telemetry) asks for the device metrics a phone shows. We answered device
            // metrics to both, which is why every LocalStats field read zero.
            metrics_stats_ = false;
            for (size_t k = 0; k + 1 < pkt.data.payload_len;) {
                const uint8_t tag = pkt.data.payload[k];
                if (tag == 0x0D) {
                    k += 5;
                    continue;
                }  // time (fixed32)
                if ((tag & 0x07) != 2) break;  // not a sub-message
                if ((tag >> 3) == 6) {
                    metrics_stats_ = true;
                    break;
                }
                k += 2 + pkt.data.payload[k + 1];
            }
        }

        // Telemetry is otherwise invisible - it only moves numbers on other tabs - so
        // announce which set of metrics arrived and the headline reading from it.
        if (pkt.data.portnum == PortNum::TELEMETRY &&
            node_db_.last_telemetry_node() == pkt.header.from) {
            const auto& tl = node_db_.last_telemetry();
            std::string what;
            if (tl.has_device) {
                what = "dev";
                if (tl.battery_level)
                    what += " " + to_string_dec_uint(tl.battery_level) + "%";
            } else if (tl.has_stats)
                what = "stats";
            else if (tl.has_env)
                what = "env";
            else if (tl.has_air)
                what = "air";
            else if (tl.has_power)
                what = "power";
            else if (tl.has_health)
                what = "health";
            else if (tl.has_host)
                what = "host";
            else if (tl.has_traffic)
                what = "traffic";
            else
                what = "empty";
            chat_view_.write_console("* telem " +
                                     to_string_hex(pkt.header.from & 0xFFFF, 4) +
                                     " " + what + "\n");
        }

        chat_view_.on_packet(pkt);
        nodes_view_.refresh();
        map_view_.refresh_nodes();
        store_and_forward(pkt);  // keep recent texts / answer an "SF" history request

        // Echo test: bounce every heard text back to its sender with the signal we
        // heard it at, so someone walking away can watch the link degrade and see
        // exactly where it stops answering. Never echo an echo, or two nodes with the
        // setting on would ping-pong for ever. The reply itself is assembled by the
        // timer - see echo_dest_.
        if (cfg_.echo_enabled && pkt.data.portnum == PortNum::TEXT_MESSAGE &&
            pkt.header.from != cfg_.node_id && !echo_dest_) {
            std::string in = pkt.text_payload();
            // Read the bell off the untrimmed text: ours ends with the character, and
            // the trim below would take it away before anyone had looked.
            const bool bell = has_bell(in);
            while (!in.empty() && static_cast<uint8_t>(in.back()) < 0x20) in.pop_back();
            const bool is_echo = (in.compare(0, 5, "echo ") == 0);
            // An empty keyword answers everything; otherwise only texts containing it,
            // so several people can range-test on one channel without cross-talk.
            const bool matches = cfg_.echo_key.empty() ||
                                 in.find(cfg_.echo_key) != std::string::npos;
            // A bell is a call for attention, so it can trigger a reply on its own -
            // useful when the far end has no text to send, only a ring. (See above for
            // where `bell` is read.)
            if (!in.empty() && !is_echo && matches) {
                // The reply reads as a report on how the far end's transmission arrived
                // here: "echo hello -73dBm 6.2SNR 0hop up12m". Everything after the text
                // describes reception, which is what a range test is looking for.
                std::string r = "echo";
                if (cfg_.echo_mask & MeshSettings::ECHO_TEXT) {
                    std::string body = in;
                    // Strip the bell so the reply does not ring the far end back - the
                    // emoji as well as the character, or the echo would keep ringing it.
                    strip_bell(body);
                    if (!body.empty())
                        r += " " + body.substr(0, 20);
                    else if (bell)
                        r += " bell";
                }
                if (cfg_.echo_mask & MeshSettings::ECHO_SIG) {
                    const uint8_t hs = pkt.header.hop_start(), hl = pkt.header.hop_limit();
                    const int snr10 = static_cast<int>(pkt.rx_snr * 10.0f);
                    r += " " + to_string_dec_int(pkt.rx_rssi) + "dBm " +
                         to_string_dec_int(snr10 / 10) + "." +
                         to_string_dec_uint(static_cast<uint32_t>(std::abs(snr10 % 10))) +
                         "SNR " + to_string_dec_uint((hs >= hl) ? (hs - hl) : 0) + "hop";
                    // The gain this receiver was running at. Without it the dBm and SNR
                    // above cannot be compared between two runs of a range test, which
                    // is the one thing anyone does with them.
                    r += " L" + to_string_dec_uint(receiver_model.lna()) +
                         "V" + to_string_dec_uint(receiver_model.vga()) +
                         "A" + to_string_dec_uint(receiver_model.rf_amp() ? 1 : 0);
                }
                if (cfg_.echo_mask & MeshSettings::ECHO_MEM)
                    r += " heap" + to_string_dec_uint(chCoreStatus() / 1024) + "k";
                if (cfg_.echo_mask & MeshSettings::ECHO_UP)
                    r += " up" + to_string_dec_uint(uptime_ticks_ / 3600u) + "m";
                if (cfg_.echo_mask & MeshSettings::ECHO_BUILD)
                    r += " " + std::string(VERSION_STRING);
                echo_text_ = r;
                echo_dest_ = pkt.header.from;
                echo_delay_ = ECHO_AFTER_RX;
            }
        }

        // Reply with an ACK when a packet addressed to us wants one - unless the
        // user turned off read receipts (stealth: read, but don't reveal it).
        if (cfg_.send_read_receipts && pkt.header.to == cfg_.node_id && pkt.header.want_ack()) {
            ack_dest_ = pkt.header.from;  // built by the timer, see above
            ack_request_id_ = pkt.header.packet_id;
            ack_channel_ = pkt.channel_index;
        }
    });

    // A composed chat message is built in the chat view but transmitted here.
    chat_view_.set_gains(cfg_.rx_lna, cfg_.rx_vga, cfg_.rx_amp);
    chat_view_.set_on_gain([this](uint8_t lna, uint8_t vga, uint8_t amp) {
        cfg_.rx_lna = lna;
        cfg_.rx_vga = vga;
        cfg_.rx_amp = amp;
        persist();
    });

    chat_view_.set_on_tx_request([this](const uint8_t* data, size_t len) {
        queue_tx(data, len, true);  // typed by the user: never drop it for a relay
        arm_retry(data, len);       // repeat it until the receipt comes back
    });

    // Open the telemetry-override config from the Map tab.
    map_view_.set_on_resend_pos([this]() { broadcast_position(); });
    map_view_.set_on_telemetry([this]() {
        nav_.push<MeshTelemetryMenuView>(cfg_, hooks_);
    });

    // Point the chat at a node and show it: used by the node detail's "Message"
    // button and by a DM thread picked from the conversations list.
    auto open_dm = [this](uint32_t id) {
        chat_view_.set_dest(id);
        tab_view_.set_selected(0);  // Chat tab
    };
    nodes_view_.set_on_message(open_dm);
    chat_view_.set_on_radio_mode([this](uint8_t m) {
        const uint8_t was = cfg_.radio_mode;
        cfg_.radio_mode = m;
        // Transmit-only means the receiver is off, not ignored. Ignoring what arrives
        // still costs the current the front end draws, and still lights the RX lamp -
        // so the lock now reaches the hardware rather than only the software.
        if (m == 1 && was != 1) {
            receiver_model.disable();
            baseband::shutdown();
        } else if (was == 1 && m != 1 && !is_transmitting_) {
            start_rx();
        }
        persist();
    });
    chat_view_.set_radio_mode(cfg_.radio_mode);
    // A lock saved from the last session applies before the first packet, not after it.
    if (cfg_.radio_mode == 1) {
        receiver_model.disable();
        baseband::shutdown();
    }
    // "Request local stats": a telemetry request the node answers with its counters.
    // "Exchange user info": our NodeInfo, asking to be answered. Broadcast, not aimed at
    // the node whose card it was pressed on - a peer that already holds our public key
    // discards any unicast from us that is only channel-encrypted, so a direct request
    // would be thrown away unread. Broadcast is what a stock node sends for this too,
    // and everyone in earshot answers with their own NodeInfo - which is the one place
    // a public key travels, and so the thing that makes an encrypted DM possible.
    nodes_view_.set_on_exchange([this]() {
        uint8_t* const buf = pkt_scratch;
        const size_t len = router_.build_nodeinfo_tx(buf, sizeof(pkt_scratch),
                                                     cfg_.node_long_name,
                                                     cfg_.node_short_name, true);
        // Name the channel it went out on. A NodeInfo is a broadcast, and a broadcast
        // only ever reaches the channel it was sent on: asking a node that lives on the
        // primary channel while sitting on a custom one is silence with no explanation,
        // and the card gives no hint which channel the request used.
        if (len > 0 && queue_tx(buf, len, true))
            chat_view_.write_console("* sent our info on " + active_channel_name() +
                                     ", asked for theirs\n");
        else
            chat_view_.write_console("* info exchange not sent\n");
    });
    nodes_view_.set_on_request_stats([this](uint32_t id, uint8_t what) {
        uint8_t* const buf = pkt_scratch;
        const bool trace = (what == REQ_TRACE);
        size_t len = trace ? router_.build_traceroute_request_tx(buf, sizeof(pkt_scratch), id)
                           : router_.build_stats_request_tx(buf, sizeof(pkt_scratch), id,
                                                            what == REQ_STATS);
        if (len > 0 && queue_tx(buf, len, true))
            chat_view_.write_console(std::string("* ") +
                                     (trace ? "route" : what == REQ_STATS ? "stats"
                                                                          : "metrics") +
                                     " requested from " +
                                     to_string_hex(id & 0xFFFF, 4) + "\n");
    });

    // The chat "Ch" button opens the conversations list: channels (name + key =
    // encryption) and the direct-message threads.
    auto open_channels = [this, open_dm]() {
        auto* v = nav_.push<MeshtasticChannelsView>(cfg_.preset_idx, cfg_.region_idx, cfg_.freq_slot, active_coding_rate(), cfg_.ch_name, cfg_.ch_key, cfg_.ch_enabled, &cfg_.active_channel, &cfg_.primary_name, node_db_, chat_view_.open_dm(), [this]() { apply_channels(); persist(); }, open_dm, [this]() { return random_channel_key(); });
        v->set_on_bell([this](uint8_t channel, uint32_t node) { send_bell(channel, node); });
    };
    chat_view_.set_on_channels(open_channels);
    chat_view_.set_beep_flag(&cfg_.beep_on_rx);
    chat_view_.set_display(cfg_.chat);
    // History is loaded from the timer, not here: see the note in the chat view ctor.
    // Setup -> Position "Pick location on map": open the GeoMap in PROMPT mode and take
    // the confirmed point as our fixed position (which then overrides live GPS).

    // What a settings page is allowed to ask for. Wired once, here, and handed to
    // every page by reference. A page asks for this when it closes, not on every
    // encoder click, and the request only arms a timer here.
    map_reserve_restore();  // claim the map's room while the heap is still whole
    hooks_.apply = [this]() { apply_delay_ = APPLY_DELAY_TICKS; };
    hooks_.apply_radio = [this]() {
        apply_delay_ = APPLY_DELAY_TICKS;
        apply_radio_ = true;  // region / preset / frequency: restart the receiver
    };
    hooks_.resend = [this]() { nodeinfo_timer_ = nodeinfo_period() - 60; };
    hooks_.clear_history = [this]() { chat_view_.clear_all_history(); };
    hooks_.pick_on_map = [this]() { pick_position_on_map(); };
    // Our own contact code, in the same format a node card offers for a peer: scanning
    // it adds us to a phone together with our public key, which is what lets that phone
    // send us an encrypted direct message without anyone typing anything.
    hooks_.send_telemetry = [this]() { broadcast_telemetry(); };
    hooks_.share_me = [this]() {
        share_url_ = meshtastic::contact_share_url(cfg_.node_id, cfg_.node_long_name,
                                                   cfg_.node_short_name, pki_pub_,
                                                   cfg_.pki_enabled);
        nav_.push<MeshtasticQRView>(share_url_,
                                    cfg_.node_long_name[0] ? std::string(cfg_.node_long_name)
                                                           : std::string("this node"),
                                    cfg_.pki_enabled ? "my contact  [key]"
                                                     : "my contact  [no key]");
    };

    // Each page is its own window, pushed on demand and freed by navigation.
    setup_menu_.set_on_open([this](uint8_t page) {
        switch (page) {
            case 0:
                nav_.push<MeshProfilePageView>(cfg_, hooks_);
                break;
            case 1:
                nav_.push<MeshRadioPageView>(cfg_, hooks_);
                break;
            case 2:
                nav_.push<MeshPrivacyPageView>(cfg_, hooks_);
                break;
            case 3:
                nav_.push<MeshSystemPageView>(cfg_, hooks_);
                break;
            default:
                nav_.push<MeshChatPageView>(cfg_, hooks_);
                break;
        }
    });
    map_view_.set_on_resend([this]() { nodeinfo_timer_ = nodeinfo_period() - 60; });

    // The log file, the keys, the channels and the receiver: see start_deferred().
    // ...and a moment later the radio is restarted. The framework applies the view's own RadioState
    // when the app becomes visible, which lands AFTER this constructor and retunes the
    // receiver to that state's default frequency - 868.825, while LongFast lives on
    // 869.075. The result was a receiver that heard nothing until some setting was
    // touched. Re-running start_rx() from the timer puts it back on the real channel.
    apply_delay_ = 60;
    apply_radio_ = true;
    refresh_own_position();
    // Announce ourselves shortly after start (peers learn our name), but NOT
    // from the constructor: a self-TX ~0.6 s in - right as the radio/baseband
    // are still settling from run_image - can wedge the M0 on an app restart.
    // Seed the timer so the first NODEINFO fires ~5 s in via on_timer instead.
    nodeinfo_timer_ = nodeinfo_period() - 300;
    // First telemetry ~8 s in - after the nodeinfo, staggered so the two
    // self-TXs don't land back-to-back. Seed relative to the configured period.
    const uint32_t telp0 = cfg_.telemetry.tel_min * 3600u;
    telemetry_timer_ = (telp0 > 480) ? telp0 - 480 : 0;
}

MeshtasticView::~MeshtasticView() {
    // Clear the models' enabled_ flags (not just radio::disable()). The models
    // are global singletons that outlive this view; leaving enabled_ set made
    // the NEXT launch's start_rx() set_*() calls message a shut-down M4 before
    // run_image() - an infinite send_message() spin = the second-appstart
    // freeze. disable() also calls radio::disable().
    set_switches_repeat_config(saved_repeat_);  // hand the keys back as we found them
    audio::output::stop();                      // release the codec if the RX chime enabled it
    receiver_model.disable();
    transmitter_model.disable();
    baseband::shutdown();
    // Let the M4 finish its (heavy) processor teardown before anyone can
    // run_image() again - m4_init() extracts the next image into M4 code RAM
    // assuming the core is already halted (core_control.cpp), and stomping a
    // still-running teardown poisons the baseband -> "Baseband Send Fail" on
    // the next app launch.
    chThdSleepMilliseconds(50);
}

void MeshtasticView::focus() {
    tab_view_.focus();
}

void MeshtasticView::set_parent_rect(const Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);
}

uint32_t MeshtasticView::identity_signature() const {
    // Cheap fingerprint of everything a peer learns from our NodeInfo. Only a real
    // change is worth an out-of-turn announcement.
    uint32_t h = cfg_.node_id ^ (static_cast<uint32_t>(cfg_.hw_model) << 8) ^
                 (static_cast<uint32_t>(cfg_.node_role) << 16);
    for (const char* p = cfg_.node_long_name; *p; ++p) h = h * 31u + static_cast<uint8_t>(*p);
    for (const char* p = cfg_.node_short_name; *p; ++p) h = h * 31u + static_cast<uint8_t>(*p);
    return h;
}

void MeshtasticView::apply_settings() {
    // Router keeps our id: outgoing packets must carry the current one.
    router_.set_local_node(cfg_.node_id, DEFAULT_PSK, sizeof(DEFAULT_PSK));
    apply_channels();  // channel hashes follow the preset name
    init_pki();        // (re)apply PKC: generate a key on first enable
    chat_view_.set_display(cfg_.chat);
    apply_key_repeat();
    refresh_own_position();

    // Re-announce ourselves within ~3 s, but only when the identity really changed -
    // scrolling through the role or device list used to put a NodeInfo on the air at
    // every step.
    const uint32_t sig = identity_signature();
    if (sig != identity_sig_) {
        identity_sig_ = sig;
        nodeinfo_timer_ = nodeinfo_period() - 180;
    }

    // FULL RX restart (shutdown + re-run baseband image + reconfigure freq/SF/BW), and
    // only for the settings that need it. A light set_lora_config+retune left the
    // baseband in a bad state when the BANDWIDTH changed - reaching any BW250 preset
    // from LongFast crosses the BW125 presets, and that transition broke the BW250
    // streaming RX. start_rx() makes a preset switch behave like a fresh app launch.
    if (apply_radio_) {
        apply_radio_ = false;
        start_rx();
    }
    persist();  // survive a later crash or power-off
}

void MeshtasticView::pick_position_on_map() {
    float clat = cfg_.fixed_lat, clon = cfg_.fixed_lon;
    if (clat == 0.0f && clon == 0.0f && has_gps_fix_) {
        clat = static_cast<float>(gps_position_.latitude);
        clon = static_cast<float>(gps_position_.longitude);
    }
    if (!map_would_fit(nullptr)) return;
    nav_.push<GeoMapView>(
        static_cast<int32_t>(0), GeoPos::alt_unit::METERS, GeoPos::spd_unit::HIDDEN,
        clat, clon,
        [this](int32_t, float lat, float lon, int32_t) {
            cfg_.fixed_lat = lat;
            cfg_.fixed_lon = lon;
            cfg_.gps_enabled = false;  // a picked point overrides live GPS
            map_view_.set_fixed_position(lat, lon, cfg_.telemetry.fixed_alt_m, cfg_.telemetry.fixed_sats);
            refresh_own_position();  // broadcast + redraw
            persist();
            map_reserve_restore();  // the picker is closing; take the room back
        });
}

// An acknowledged message must stop being repeated, whichever kind of acknowledgement
// arrived. A broadcast's is the implicit one - hearing a neighbour flood our own packet
// back - and that path used to turn the dot green and leave the retry timer running, so
// a message already known to have arrived was sent two more times for nothing.
void MeshtasticView::stop_retry(uint32_t pid) {
    if (!pid || pid != retry_pid_) return;
    retry_len_ = 0;
    retry_pid_ = 0;
    chat_view_.set_retry({});
}

void MeshtasticView::arm_retry(const uint8_t* data, size_t len) {
    // A broadcast is repeated too. It has nobody in particular to acknowledge it, but
    // it has the implicit one: hearing our own packet flooded back by a neighbour. If
    // that never comes, nobody heard us, and trying again is exactly the right answer.
    if (len < meshtastic::PKT_HEADER_SIZE || !cfg_.chat.tx_retries) return;
    const uint32_t to = data[0] | (data[1] << 8) | (data[2] << 16) |
                        (static_cast<uint32_t>(data[3]) << 24);
    const bool want_ack = (data[12] >> 3) & 1;
    if (to != meshtastic::BROADCAST_ADDR && !want_ack) return;

    // Locked to transmit: nothing is received, so no acknowledgement can arrive and
    // there is nothing for retries to wait for. Counting attempts would only be
    // theatre - the message is already marked "sent, nothing came back".
    if (cfg_.radio_mode == 1) return;

    memcpy(retry_buf_, data, std::min(len, sizeof(retry_buf_)));
    retry_len_ = len;
    retry_pid_ = data[8] | (data[9] << 8) | (data[10] << 16) |
                 (static_cast<uint32_t>(data[11]) << 24);
    retry_left_ = static_cast<uint8_t>(cfg_.chat.tx_retries);
    retry_at_ = uptime_ticks_ + cfg_.chat.tx_retry_s * 60u;
    // The tag first: without it the counter has no line to attach itself to and the
    // opening "0/3" was never drawn at all.
    chat_view_.set_retry_tag(retry_pid_);
    chat_view_.set_retry("0/" + to_string_dec_uint(cfg_.chat.tx_retries));
}

void MeshtasticView::resend_lora_config() {
    const auto& preset = MODEM_PRESETS[cfg_.preset_idx];
    baseband::set_lora_config(preset.sf, preset.bw_hz, active_coding_rate(), cfg_.node_id);
}

// The gain the transmitter is asked for, and why.
//
// Nothing here is calibrated: HackRF's TX gain is a control position, not dBm, and what
// actually leaves the antenna depends on the band, the hardware and what is screwed onto
// the socket. So this does not claim to enforce anything. "Region" simply attenuates by
// as many dB as the chosen region's limit sits below the most permissive entry in the
// table - a rule that is monotone, conservative and stated plainly rather than a number
// invented to look official. "Max" asks for everything.
//
// The firmware's own global cap (config_tx_gain_max_db) and its TX-disable switch apply
// on top of whatever this returns: the operator's system-wide choice always wins.
uint8_t MeshtasticView::active_tx_gain() const {
    constexpr uint8_t GAIN_MAX = 47;
    if (cfg_.tx_pwr_mode == 1) return GAIN_MAX;  // max
    if (cfg_.tx_pwr_mode == 2) return (cfg_.tx_pwr_db > GAIN_MAX) ? GAIN_MAX : cfg_.tx_pwr_db;

    int8_t most = -128;  // most permissive region
    for (uint8_t i = 0; i < meshtastic::NUM_REGIONS; i++)
        if (meshtastic::REGIONS[i].max_power_dbm > most) most = meshtastic::REGIONS[i].max_power_dbm;
    const uint8_t idx = (cfg_.region_idx < meshtastic::NUM_REGIONS) ? cfg_.region_idx : 0;
    const int down = most - meshtastic::REGIONS[idx].max_power_dbm;
    return (down >= GAIN_MAX) ? 0 : static_cast<uint8_t>(GAIN_MAX - down);
}

uint8_t MeshtasticView::active_coding_rate() const {
    // The override if there is one, the preset's own rate otherwise. This is what we
    // TRANSMIT at. The receiver no longer depends on it: it takes each packet's rate
    // from that packet's own header, so a wrong entry here can no longer make us
    // misread what other people send.
    const uint8_t cr = cfg_.coding_rate;
    return (cr >= 5 && cr <= 8) ? cr : meshtastic::MODEM_PRESETS[cfg_.preset_idx].cr;
}

void MeshtasticView::start_deferred() {
    // Load-or-generate our Curve25519 identity keypair and hand it to the router
    // (must follow set_local_node: the id feeds the first-boot key entropy).
    init_pki();
    // Push the channel table (primary + custom channels) to the router. The primary
    // hash MUST match the active preset's default channel or on_raw_rx drops every
    // packet (it was once hardcoded to ShortTurbo 0x0E -> all LongFast 0x08 packets,
    // though decoded byte-exact, were silently rejected). apply_channels() derives it.
    apply_channels();
    if (cfg_.logging) {
        logger_ = std::make_unique<MeshtasticLogger>();
        // /LOGS, like the chat history: the card always has it, and creating a
        // directory costs flash this app does not have. The old path was /MESHTASTIC,
        // which the card does not have - the log had nowhere to go.
        logger_->append(u"/LOGS/mesh_rx.csv");
    }
    start_rx();
}

void MeshtasticView::start_rx() {
    is_transmitting_ = false;
    // A fresh demodulator is never mid-packet; the status message from the old one
    // (if any) can't arrive any more, so don't let a stale "busy" hold TX.
    rx_busy_ = false;
    // Clear the enabled flag before the set_*() calls below: while it is set, each
    // ReceiverModel::set_*() immediately messages the baseband, and here the M4
    // image is down (shutdown, pre-run_image) so that send would spin forever.
    // Covers both the app-restart path (stale enabled_ from the prior instance)
    // and the TX->RX return (transmitter left enabled).
    receiver_model.disable();
    transmitter_model.disable();
    baseband::shutdown();
    // Same M4-teardown guard as trigger_tx: shutdown() returns once the M4
    // *consumes* the ShutdownMessage, not when its ~Processor/~BasebandThread
    // teardown finishes. run_image() below hard-resets the M4, and doing that
    // mid-teardown wedges the baseband queue -> "Baseband Send Fail" panic.
    // Bites on the second app launch (previous session's image state) and on
    // the TX->RX return path.
    chThdSleepMilliseconds(30);
    update_radio_config();

    // Actually configure the receiver (update_radio_config only fills the
    // unused radio_state_; the app never set the receiver freq/rate before, so
    // RX ran at whatever global persistent tuning was active). Sampling rate
    // 2 MHz to match proc_lora's baseband_fs (-> /2 = 1 MHz, integer OS for the
    // fold demod).  WidebandFMAudio mode gives tuning_offset = -fs/4, which the
    // CIC3 FS/4 down-shift cancels (signal lands at DC).  2 MHz (vs 4) gives the
    // M4 a 1024 us/buffer deadline so the fine-align FFT burst fits without drops.
    const uint32_t freq = (cfg_.freq_override_hz != 0)
                              ? cfg_.freq_override_hz
                              : static_cast<uint32_t>(channel_frequency(cfg_.region_idx, cfg_.preset_idx, cfg_.freq_slot,
                                                                        cfg_.primary_name.c_str()));
    receiver_model.set_sampling_rate(2'000'000);
    receiver_model.set_baseband_bandwidth(1'750'000);
    receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);
    receiver_model.set_target_frequency(freq);
    // Leave the gains alone here: start_rx() runs on every return from transmit and
    // on every settings change, so hardcoding 32/32 silently undid whatever the user
    // had dialled in on the chat row moments earlier. enable() below re-applies the
    // model's stored LNA/VGA/amp, which is what the L/V/A fields edit.

    baseband::run_image(portapack::spi_flash::image_tag_lora_rx);
    receiver_model.enable();
    // Configure again almost immediately rather than half a second later. The repeat
    // exists because the first message can land while the M4 is still coming up, and
    // until it takes, the demodulator listens with the wrong settings. Half a second
    // of that is exactly when a stock node's reply to a request arrives - which is why
    // acknowledgements and telemetry answers went missing after every transmission.
    reconfig_delay_ = 6;
    const auto& preset = MODEM_PRESETS[cfg_.preset_idx];
    // Pass our node id: the demodulator validates a header by its destination field and
    // would otherwise accept broadcasts only, so unicast frames addressed to us (direct
    // messages, ACKs) never reached the app.
    baseband::set_lora_config(preset.sf, preset.bw_hz, active_coding_rate(), cfg_.node_id);
    // Enable the codec output path only when the RX chime is on, so the beep is
    // audible; keep it stopped otherwise to spare RX timing.
    // The codec stays up when anything might need to make a sound: the message chime,
    // or a bell alert, which rings even when ordinary messages are silent.
    if (cfg_.beep_on_rx || cfg_.chat.bell_alert) {
        audio::set_rate(audio::Rate::Hz_24000);
        audio::output::start();
    } else {
        audio::output::stop();
    }
}

void MeshtasticView::trigger_tx() {
    is_transmitting_ = true;
    tx_start_tick_ = uptime_ticks_;
    chat_view_.set_pkt_indicator(true);

    // Use the explicit Meshtastic channel/override frequency - NOT
    // receiver_model.target_frequency() (that returns the global persistent
    // tuning freq, which the app never sets, so TX could land off-channel).
    update_radio_config();
    const uint32_t freq = (cfg_.freq_override_hz != 0)
                              ? cfg_.freq_override_hz
                              : static_cast<uint32_t>(channel_frequency(cfg_.region_idx, cfg_.preset_idx, cfg_.freq_slot,
                                                                        cfg_.primary_name.c_str()));
    // (debug [TXa] log suppressed for a clean chat)
    receiver_model.disable();

    // Shut down RX baseband before starting TX baseband.
    baseband::shutdown();
    // Guard the RX->TX baseband switch: shutdown() only spins until the M4 *consumes*
    // the ShutdownMessage (event loop exits); the M4 then runs ~LoRaProcessor ->
    // ~BasebandThread which is what actually calls dma::disable()/sgpio disable. On
    // non-PRALINE (H4M) shutdown() has NO post-delay (the guarding sleep is #ifdef
    // PRALINE), so run_image() below could hard-reset the M4 mid-teardown of the
    // heavy energy-triggered RX proc -> intermittent "Baseband Send Fail". Give the
    // M4 (separate core) a few ms to finish tearing down cleanly.
    chThdSleepMilliseconds(30);

    // Has to match proc_lora_tx's TX_FS (2.5 MHz), or the chirp bandwidth and timing
    // come out wrong.
    transmitter_model.set_sampling_rate(2'500'000);
    transmitter_model.set_baseband_bandwidth(1'750'000);
    transmitter_model.set_target_frequency(freq);
    transmitter_model.enable();
    // Apply gain AFTER enable so it reaches the (now-active) radio TX path.
    transmitter_model.set_rf_amp(true);
    transmitter_model.set_tx_gain(active_tx_gain());

    baseband::run_image(portapack::spi_flash::image_tag_lora_tx);
    const auto& preset = MODEM_PRESETS[cfg_.preset_idx];
    baseband::set_lora_config(preset.sf, preset.bw_hz, active_coding_rate());
    baseband::send_lora_packet(tx_buf_, tx_len_);
}

void MeshtasticView::on_tx_done(uint8_t nsym) {
    (void)nsym;
    transmitter_model.disable();
    tx_pending_ = false;
    chat_view_.set_pkt_indicator(false);
    // Return to listen mode (start_rx calls baseband::shutdown internally). Measured at
    // ~48 ms, which is why the demodulator is reconfigured 100 ms later rather than
    // half a second: a stock node answers a request in the same breath.
    //
    // Unless the radio is locked to transmit: then the receiver is meant to be off, and
    // waking it after every packet was the reason the RX light stayed on in a mode
    // whose whole point is not listening.
    if (cfg_.radio_mode == 1) {
        // start_rx() is what normally clears this, and skipping it left the flag up for
        // good: the transmit watchdog then fired every second, called this function,
        // which returned here without clearing it, and announced another "TX timeout,
        // radio restarted" - a loop fed by its own alarm.
        is_transmitting_ = false;
        receiver_model.disable();
        baseband::shutdown();
        return;
    }
    start_rx();
}

void MeshtasticView::update_radio_config() {
    // Frequency: explicit override takes priority over region table
    const uint64_t freq = (cfg_.freq_override_hz != 0)
                              ? static_cast<uint64_t>(cfg_.freq_override_hz)
                              : channel_frequency(cfg_.region_idx, cfg_.preset_idx, cfg_.freq_slot,
                                                  cfg_.primary_name.c_str());

    const auto& preset = MODEM_PRESETS[cfg_.preset_idx];
    const uint32_t bw2 = static_cast<uint32_t>(preset.bw_hz) * 2u;

    // Sample rate MUST be 2 500 000 to match proc_lora's hardcoded baseband_fs.
    radio_state_ = RxRadioState{
        static_cast<uint32_t>(freq),
        bw2,
        2500000,
        ReceiverModel::Mode::WidebandFMAudio};
}

// The two console lines the receive handler writes, each assembled in a frame of its
// own. A concatenation like this leaves half a dozen std::string temporaries behind,
// and on_lora_packet is one of the deepest things the M0 runs on its 4 KB process
// stack - the temporaries alone were a quarter of it.
__attribute__((noinline)) void MeshtasticView::announce_node(const NodeEntry& e,
                                                             uint32_t from,
                                                             bool named) {
    // Drawn in the node's own colour, the same one that marks its messages and its row
    // in the node list.
    if (named)
        chat_view_.write_colour("* " + std::string(e.long_name) +
                                    " (" + to_string_hex(from & 0xFFFF, 4) + ") joined\n",
                                e.colour);
    else
        chat_view_.write_colour("* node " + to_string_hex(from, 8) + " joined\n", e.colour);
}

__attribute__((noinline)) void MeshtasticView::announce_relay(uint32_t from,
                                                              const uint8_t* relay) {
    // The id goes in the line: a relayed frame is the one place a bit error can reach
    // the air, so it can be compared with what the far node logs.
    const uint32_t rpid = relay[8] | (relay[9] << 8) | (relay[10] << 16) |
                          (static_cast<uint32_t>(relay[11]) << 24);
    // How far the packet has already travelled, not what is left of its budget. The
    // remaining budget is what used to be printed, and on a two-node mesh "hop 6" read
    // as six hops through neighbours that do not exist. hop_start is untouched by
    // make_relay; the limit in the buffer has been decremented once, hence the +1.
    const uint8_t hs = (relay[12] >> 5) & 0x07;
    const uint8_t hl = static_cast<uint8_t>((relay[12] & 0x07) + 1);
    chat_view_.write_console("* relayed " + to_string_hex(from & 0xFFFF, 4) +
                             " " + to_string_hex(rpid, 8) +
                             " hop " + to_string_dec_uint((hs > hl) ? (hs - hl) : 0) + "\n");
}

void MeshtasticView::on_lora_packet(const LoRaPacketMessage* msg) {
    if (!msg) return;
    if (msg->length < PKT_HEADER_SIZE) return;
    // Transmit-only: what arrives is not acted on - no chat lines, no node list, no
    // relaying - so the radio behaves as a beacon and nothing more.
    if (cfg_.radio_mode == 1) return;

    // Stir the RF-noise entropy pool. RSSI (from the CSS peak power) carries thermal
    // noise in its low bits and the arrival tick is unpredictable - genuine entropy for
    // PKC key generation (generate_pki_key), unlike a cold-boot clock.
    entropy_pool_ = (entropy_pool_ * 1664525u + 1013904223u) ^
                    (static_cast<uint32_t>(static_cast<uint8_t>(msg->rssi)) << 3) ^
                    (uptime_ticks_ << 11) ^ (static_cast<uint32_t>(msg->length) << 19);
    entropy_pool_ ^= entropy_pool_ >> 15;

    // Implicit ACK: we hear our OWN packet flooded back by a neighbour -> the
    // broadcast reached at least one node. Match it to a pending sent message so
    // its delivery dot turns green (the router itself drops from==us, so read the
    // 16-byte plaintext header here before it does).
    const uint32_t from = msg->data[4] | (msg->data[5] << 8) |
                          (msg->data[6] << 16) | (static_cast<uint32_t>(msg->data[7]) << 24);
    if (from == cfg_.node_id) {
        const uint32_t pid = msg->data[8] | (msg->data[9] << 8) |
                             (msg->data[10] << 16) | (static_cast<uint32_t>(msg->data[11]) << 24);
        chat_view_.on_ack(pid, true);
        stop_retry(pid);
    }

    chat_view_.set_pkt_indicator(false);
    chat_view_.set_last_signal(msg->rssi, msg->snr_tenths);

    // Neighbour discovery announcement: sample the node DB before the router parses
    // the frame, so we can tell "never heard this id" and "id known, but still nameless"
    // apart from an ordinary repeat packet. Sampling here (rather than in the packet
    // callback) also covers nodes we only observe - a frame that is learned but not
    // delivered, e.g. one flagged via_mqtt while "Ignore MQTT pkts" is on.
    const NodeEntry* pre_e = node_db_.find(from);
    const bool node_was_new = (pre_e == nullptr);
    const bool node_had_name = pre_e && pre_e->long_name[0];

    const bool should_rebroadcast = router_.on_raw_rx(
        msg->data, msg->length,
        msg->rssi,  // approximate dBm from the M4 dechirp peak power
        msg->snr_tenths / 10.0f,
        uptime_ticks_);

    // Two announcements per neighbour at most: one when its id is first heard, one when
    // its NodeInfo finally resolves that id to a name (which usually arrives later).
    const NodeEntry* post_e = (from != cfg_.node_id) ? node_db_.find(from) : nullptr;
    if (post_e) {
        const bool named = post_e->long_name[0] != 0;
        if ((named && (node_was_new || !node_had_name)) || node_was_new)
            announce_node(*post_e, from, named);
    }

    // Rebroadcast (flood).  This ALSO serves as Meshtastic's implicit ACK for broadcasts:
    // the sender hears its own packet flooded back and stops re-sending.  Disabling it
    // made the Heltec resend the same text with new packet_ids -> duplicate messages.
    // Relay per protocol: hop_limit-1 + our relay_node byte (raw copy would look
    // like the origin itself re-sending, not a relay).
    // CLIENT_MUTE (1) exists precisely to listen without repeating anything, and a
    // hidden client (8) keeps just as quiet. Every other role relays.
    const bool role_relays = (cfg_.node_role != 1 && cfg_.node_role != 8);
    if (should_rebroadcast && role_relays && tx_pending_)
        router_.note_tx_dropped();  // a relay the radio was too busy to carry
    if (should_rebroadcast && role_relays && !tx_pending_) {
        uint8_t* const relay_buf = pkt_scratch;
        const size_t n = std::min<size_t>(msg->length, sizeof(pkt_scratch));
        memcpy(relay_buf, msg->data, n);
        router_.make_relay(relay_buf);
        if (queue_tx(relay_buf, n)) {
            router_.note_relayed();
            // The id goes in the line: a relayed frame is the one place a bit error can
            // reach the air, so it can be compared with what the far node logs.
            announce_relay(from, relay_buf);
        }
    }

    if (cfg_.logging && logger_) {
        const auto hdr = PacketHeader::from_bytes(msg->data);
        // The colour is the one the chat paints this node's lines with, so a row here
        // can be matched to a stripe on screen without decoding node ids by eye.
        const auto* e = node_db_.find(hdr.from);
        logger_->log_packet(hdr, msg->rssi, msg->snr_tenths, msg->length,
                            e ? e->colour : 0);
    }
}

void MeshtasticView::refresh_own_position() {
    // A live GPS fix always wins and drives the Map via on_gps_update.
    if (has_gps_fix_) return;
    // No live fix yet: with GPS off, show the manually-entered Setup position;
    // otherwise show "waiting for fix".
    if (!cfg_.gps_enabled && (cfg_.fixed_lat != 0.0 || cfg_.fixed_lon != 0.0))
        map_view_.set_fixed_position(static_cast<float>(cfg_.fixed_lat),
                                     static_cast<float>(cfg_.fixed_lon),
                                     cfg_.telemetry.fixed_alt_m, cfg_.telemetry.fixed_sats);
    else
        map_view_.clear_fix();
}

void MeshtasticView::on_gps_update(const GPSPosDataMessage* msg) {
    if (!msg) return;
    const float lat = msg->lat;
    const float lon = msg->lon;
    if (lat == 0.0f && lon == 0.0f) return;
    if (lat < -90.0f || lat > 90.0f) return;
    if (lon < -180.0f || lon > 180.0f) return;

    const bool first_fix = !has_gps_fix_;

    gps_position_.latitude = static_cast<double>(lat);
    gps_position_.longitude = static_cast<double>(lon);
    gps_position_.altitude_m = msg->altitude;
    gps_position_.satellites_in_view = msg->satinuse;
    gps_position_.valid = true;
    gps_position_.timestamp = now_unix();
    has_gps_fix_ = true;

    map_view_.update_position(lat, lon, static_cast<float>(msg->altitude), msg->satinuse);

    // Announce our position shortly after the first fix (like real Meshtastic),
    // instead of waiting up to a full POSITION_PERIOD. Seed the timer so the
    // on_timer path fires it ~5 s in - going through queue_tx keeps the TX off
    // this GPS-callback context.
    if (first_fix && cfg_.gps_enabled) {
        const uint32_t posp0 = cfg_.telemetry.pos_min * 3600u;
        if (posp0 > 300) position_timer_ = posp0 - 300;  // announce ~5 s after first fix
    }
}

void MeshtasticView::on_timer() {
    // The keyboard has no tick of its own: one handler per message, and this is it.
    mesh_text_entry_tick();

    uptime_ticks_++;
    nodes_view_.set_now(uptime_ticks_);     // live "last heard" ages
    chat_view_.tick(uptime_ticks_);         // age pending messages -> red on ACK timeout
    router_.set_hop_limit(cfg_.hop_limit);  // keep the Setup hop-limit applied to our TX
    router_.set_ok_to_mqtt(cfg_.mqtt_ok);
    router_.set_ignore_mqtt(cfg_.ignore_mqtt);
    router_.set_hw_model(cfg_.hw_model);
    router_.set_role(cfg_.node_role);

    // Keep the screen on while this app is in the foreground. The global
    // backlight timeout (handle_rtc_tick) counts bl_tick_counter up every
    // second and only physical input (touch/keys/encoder) resets it - a passive
    // RX monitor never gets touched, so the screen would blank mid-reception.
    // Resetting each frame-sync tick (~60 Hz) holds it below the timeout, and
    // if it already went to sleep (e.g. in the menu before launch) wake it so
    // it doesn't stay dark while we're actively receiving.
    // Hold the backlight timeout off - a passive receiver never gets touched and the
    // screen would blank mid-reception - but do NOT wake a screen that is already off.
    // Waking it here is what made the moon button useless in this app: it switched the
    // display off and the very next tick switched it back on.
    portapack::bl_tick_counter = 0;

    // Periodic mesh presence. Ticks come from DisplayFrameSync (~60 Hz):
    // nodeinfo every ~10 min, position every ~5 min - sparse enough that the
    // self-TX M0 work + RX->TX baseband swap doesn't measurably eat RX time
    // (each self-TX starves the real-time RX decimation for ~0.2 s). The ctor
    // seeds nodeinfo_timer_ so the first announce lands ~5 s after start.
    // A hidden client (role 8) never advertises itself: it answers when addressed and
    // is otherwise invisible. A repeater (4) has nothing of its own to report either.
    // "Listen only" silences every unprompted transmission; the intervals themselves
    // are left alone, so switching it off puts things back as they were. A NodeInfo
    // interval of zero means never on its own, the way the other two already did.
    const bool role_announces = (cfg_.node_role != 8) && !cfg_.listen_only;
    const bool role_reports = (cfg_.node_role != 8 && cfg_.node_role != 4) &&
                              !cfg_.listen_only;
    if (role_announces && cfg_.nodeinfo_min && ++nodeinfo_timer_ >= nodeinfo_period()) {
        nodeinfo_timer_ = 0;
        broadcast_nodeinfo();
    }

    // Neighbour list: sparse on purpose, six hours by default.
    if (role_reports && cfg_.nbr_min && ++nbr_timer_ >= cfg_.nbr_min * 3600u) {
        nbr_timer_ = 0;
        broadcast_neighborinfo();
    }

    // Periodic identity randomisation (min -> ~60 Hz ticks); 0 = off. Rolls the node
    // id / name / hw-model so we masquerade as a fresh node, then re-announces.
    if (cfg_.rand_min > 0 && ++rand_timer_ >= static_cast<uint32_t>(cfg_.rand_min) * 3600u) {
        rand_timer_ = 0;
        randomize_identity();
    }

    // POSITION broadcast: user-set period (minutes -> ~60 Hz ticks); 0 = never.
    const uint32_t pos_period = cfg_.telemetry.pos_min * 3600u;
    if (role_reports && pos_period && ++position_timer_ >= pos_period) {
        position_timer_ = 0;
        if (cfg_.gps_enabled && gps_position_.valid) {
            broadcast_position();
        } else if (!cfg_.gps_enabled && (cfg_.fixed_lat != 0.0f || cfg_.fixed_lon != 0.0f)) {
            PositionData fixed;
            fixed.latitude = cfg_.fixed_lat;
            fixed.longitude = cfg_.fixed_lon;
            fixed.timestamp = now_unix();
            fixed.valid = true;
            uint8_t* const buf = pkt_scratch;
            size_t len = router_.build_position_tx(buf, sizeof(pkt_scratch), fixed);
            if (len > 0 && queue_tx(buf, len)) announce_position();
        }
    }

    // Battery for the Data tab, once a second: it is one widget's worth of repaint,
    // not the whole tab, so a live reading costs nothing visually.
    if (uptime_ticks_ % 60 == 30)
        read_battery();

    // Telemetry (device + environment metrics) every ~5 min. The ctor seeds
    // telemetry_timer_ so the first one goes out ~8 s after start.
    // TELEMETRY (device/env) broadcast: user-set period; 0 = never.
    const uint32_t tel_period = cfg_.telemetry.tel_min * 3600u;
    if (role_reports && tel_period && ++telemetry_timer_ >= tel_period) {
        telemetry_timer_ = 0;
        broadcast_telemetry();
    }

    // Store and Forward: drain one queued replay line whenever the single TX slot is
    // free, so a burst of history doesn't get dropped by queue_tx's single-slot guard.
    if (!sf_replay_.empty() && !tx_pending_ && !is_transmitting_) {
        const std::string& line = sf_replay_.front();
        uint8_t* const buf = pkt_scratch;
        size_t len = router_.build_text_tx(buf, sizeof(pkt_scratch), line.c_str(), line.size(),
                                           sf_replay_dest_, false);
        if (len > 0 && queue_tx(buf, len))
            if (cfg_.chat.sf_notify) chat_view_.write_console("* SF: sent to " +
                                                              to_string_hex(sf_replay_dest_ & 0xFFFF, 4) + ", " +
                                                              to_string_dec_uint(static_cast<uint32_t>(sf_replay_.size() - 1)) +
                                                              " left\n");
        sf_replay_.erase(sf_replay_.begin());
    }

    // First tick after construction: everything that touches the card or the baseband,
    // away from the deep stack of the constructor.
    if (!deferred_load_) {
        deferred_load_ = true;
        start_deferred();
        chat_view_.load_deferred();
        load_known_nodes();
    }

    update_utilisation();

    // Store and Forward retention: a text nobody came back for is dropped once it is
    // older than the hold time, so the buffer is not permanently occupied by messages
    // for a node that never returns.
    if (cfg_.sf_ttl_min && !sf_store_.empty() && (uptime_ticks_ % 600u) == 0) {
        const uint32_t ttl = cfg_.sf_ttl_min * 60u * 60u;  // minutes -> ~60 Hz ticks
        size_t dropped = 0;
        while (!sf_store_.empty() && (uptime_ticks_ - sf_store_.front().tick) > ttl) {
            sf_store_.erase(sf_store_.begin());
            ++dropped;
        }
        if (dropped && cfg_.chat.sf_notify)
            chat_view_.write_console("* SF: dropped " +
                                     to_string_dec_uint(static_cast<uint32_t>(dropped)) +
                                     " expired\n");
    }

    // Write the conversation list only when it actually changed - a name resolving, a
    // key arriving or a new thread - rather than on a timer for its own sake.
    if ((uptime_ticks_ % 300u) == 0) {
        uint32_t sig = 0;
        for (size_t i = 0; i < node_db_.count(); i++) {
            const NodeEntry* e = node_db_.at(i);
            if (!e || !e->active) continue;
            sig = sig * 31u + e->node_id + (e->has_pubkey ? 1u : 0u) +
                  (e->dm_thread ? 2u : 0u) + (e->colour * 4u) +
                  static_cast<uint32_t>(strlen(e->long_name));
        }
        if (sig != threads_sig_) {
            threads_sig_ = sig;
            save_known_nodes();
        }
    }

    // Repeat a direct message that has not been acknowledged yet. The bytes are resent
    // verbatim, packet id included, so the far side treats it as the same message.
    if (retry_len_ && uptime_ticks_ >= retry_at_) {
        if (retry_left_ && !tx_pending_) {
            --retry_left_;
            queue_tx(retry_buf_, retry_len_, true);
            retry_at_ = uptime_ticks_ + cfg_.chat.tx_retry_s * 60u;
            // Attempt counter in the status row rather than a pile of chat lines.
            chat_view_.set_retry_tag(retry_pid_);
            chat_view_.set_retry(
                to_string_dec_uint(cfg_.chat.tx_retries - retry_left_) + "/" +
                to_string_dec_uint(cfg_.chat.tx_retries));
        } else if (!retry_left_) {
            chat_view_.on_retries_done(retry_pid_);  // out of attempts
            chat_view_.set_retry({});                // and clear the counter
            retry_len_ = 0;
            retry_pid_ = 0;
        }
    }

    // Second helping of the demodulator configuration after a baseband restart.
    if (reconfig_delay_ && --reconfig_delay_ == 0 && !is_transmitting_) resend_lora_config();

    // Transmit watchdog: recover if the baseband never reported the frame finished.
    if (is_transmitting_ && (uptime_ticks_ - tx_start_tick_) > TX_TIMEOUT_TICKS) {
        chat_view_.write_console("* TX timeout, radio restarted\n");
        on_tx_done();  // drops back to receive and frees the slot
    }

    // Deferred replies (read receipt, echo): assembled here, where the stack is
    // shallow, instead of inside the packet callback.
    if (echo_delay_) --echo_delay_;
    // A frame turned away by the channel filter leaves no other trace: it is dropped
    // before decryption, so no counter of ours moves and nothing reaches the chat. Say
    // so once per burst, with both bytes - the mismatch is otherwise invisible.
    {
        const uint32_t n = router_.bad_hash_count();
        if (n != bad_hash_seen_) {
            const bool first = (bad_hash_seen_ == 0) || (n - bad_hash_seen_) > 8;
            bad_hash_seen_ = n;
            if (first || (n % 8) == 0)
                // Short enough to survive the 30-character console: the sender's low
                // half is all that is needed to tell 0476 from noise.
                chat_view_.write_console(
                    "* hash " + to_string_hex(router_.last_bad_hash(), 2) +
                    "!=" + to_string_hex(router_.primary_hash(), 2) +
                    " src " + to_string_hex(router_.last_bad_from() & 0xFFFF, 4) +
                    " n" + to_string_dec_uint(router_.bad_hash_count()) + "\n");
        }
    }

    if (trace_dest_ && !tx_pending_) {
        const uint32_t asker = trace_dest_;
        const uint32_t req_id = trace_req_id_;
        trace_dest_ = 0;
        trace_req_id_ = 0;
        const size_t len = router_.build_traceroute_tx(pkt_scratch, sizeof(pkt_scratch),
                                                       asker, req_id,
                                                       trace_buf_, trace_len_,
                                                       trace_snr_, trace_hops_, trace_ch_);
        if (len > 0 && queue_tx(pkt_scratch, len))
            chat_view_.write_console("* route traced for " +
                                     to_string_hex(asker & 0xFFFF, 4) + "\n");
    }

    if (intro_dest_ && !tx_pending_) {
        const uint32_t asker = intro_dest_;
        intro_dest_ = 0;
        chat_view_.write_console(std::string(intro_pos_ ? "* position asked by "
                                                        : "* our info asked by ") +
                                 to_string_hex(asker & 0xFFFF, 4) + "\n");
        // Broadcast, as a stock node answers: the asker hears it, and so does anyone
        // else who has been waiting to learn the same thing.
        if (intro_pos_)
            broadcast_position();
        else
            broadcast_nodeinfo();
    }

    if (metrics_dest_ && !tx_pending_) {
        const uint32_t asker = metrics_dest_;
        const uint32_t req_id = metrics_req_id_;
        metrics_dest_ = 0;
        metrics_req_id_ = 0;
        const bool want_stats = metrics_stats_;
        metrics_stats_ = false;
        static TelemetryData reply;  // ~1 KB: never on the stack, see broadcast_telemetry
        if (want_stats) {
            // LocalStats: the router's own counters, not a sensor reading anywhere.
            // Fields are set one by one rather than by assigning a fresh TelemetryData:
            // that temporary is about a kilobyte, and this stack has four in total.
            reply.channel_utilization = -1.0f;  // nothing here measures either, so both
            reply.air_util_tx = -1.0f;          // stay out of the message entirely
            reply.heap_total = 0;
            const auto& c = router_.counters();
            reply.stat_uptime = uptime_ticks_ / 60;
            if (cfg_.telemetry.up_mode == 1)
                reply.stat_uptime = cfg_.telemetry.up_hours * 3600u;
            reply.send_uptime = (cfg_.telemetry.up_mode != 2);
            reply.packets_tx = c.tx;
            reply.packets_rx = c.rx;
            reply.packets_rx_bad = c.rx_bad;
            reply.rx_dupe = c.rx_dupe;
            reply.tx_relay = c.tx_relay;
            reply.tx_dropped = c.tx_dropped;
            reply.nodes_total = static_cast<uint32_t>(node_db_.count());
            reply.nodes_online = static_cast<uint32_t>(nodes_view_.online_count());
        } else {
            read_battery();
            reply = telemetry_;
            reply.uptime_seconds = uptime_ticks_ / 60;
            apply_telemetry_overrides(reply);  // ...which may replace or withhold it
            reply.has_device = true;
        }
        reply.timestamp = now_unix();
        uint8_t* const buf = pkt_scratch;
        const size_t len = router_.build_telemetry_tx(buf, sizeof(pkt_scratch), reply,
                                                      want_stats ? TelemetryData::LOCAL_STATS
                                                                 : TelemetryData::DEVICE,
                                                      nullptr, 0, asker, req_id);
        if (len > 0 && queue_tx(buf, len))
            chat_view_.write_console(std::string(want_stats ? "* stats sent to "
                                                            : "* metrics sent to ") +
                                     to_string_hex(asker & 0xFFFF, 4) + "\n");
    }
    if (!ack_dest_ && !tx_pending_ && (keyreq_dest_ = router_.take_key_request())) {
        uint8_t* const buf = pkt_scratch;
        // Broadcast, not addressed to the peer: the very guard we are working around
        // would discard a direct request just as it discards the message. A broadcast
        // NodeInfo carrying want_response is what a stock node sends to ask the mesh to
        // introduce itself, and it is answered by everyone in earshot.
        const size_t len = router_.build_nodeinfo_tx(buf, sizeof(pkt_scratch), cfg_.node_long_name,
                                                     cfg_.node_short_name, true);
        keyreq_dest_ = 0;
        if (len > 0 && queue_tx(buf, len))
            chat_view_.write_console("* key requested\n");
    }
    if (ack_dest_ && !tx_pending_) {
        uint8_t* const buf = pkt_scratch;
        const size_t len = router_.build_ack_tx(buf, sizeof(pkt_scratch), ack_dest_,
                                                ack_request_id_, ack_channel_);
        ack_dest_ = 0;
        if (len > 0) queue_tx(buf, len);
        // Let the receipt land before the echo follows it.
        if (echo_dest_ && echo_delay_ < ECHO_AFTER_ACK) echo_delay_ = ECHO_AFTER_ACK;
    } else if (echo_dest_ && !echo_delay_ && !tx_pending_) {
        uint8_t* const buf = pkt_scratch;
        // Sent exactly like a message typed in the chat, receipt request included: the
        // same packet without it was the one shape a stock node refused to decode, and
        // the acknowledgement doubles as proof the echo arrived.
        const size_t len = router_.build_text_tx(buf, sizeof(pkt_scratch), echo_text_.c_str(),
                                                 echo_text_.size(), echo_dest_, true);
        const uint32_t dest = echo_dest_;
        router_.request_key_from(dest);
        echo_dest_ = 0;
        // Say it went, and where. The echo is the whole point of a range test and it
        // was the one reply that left without a word: with the setting on and nothing
        // in the chat there is no telling a link that has gone quiet from a reply that
        // was never built. The channel matters as much as the fact - an echo rides the
        // channel in use, and a far end listening on another one will never hear it.
        const std::string sent = echo_text_;
        echo_text_.clear();
        if (len > 0 && queue_tx(buf, len))
            chat_view_.write_console("* echo to " + to_string_hex(dest & 0xFFFF, 4) +
                                     " on " + active_channel_name() + ": " + sent + "\n");
        else
            chat_view_.write_console("* echo not sent\n");
    }

    // Settings settle: apply once, a beat after the last change.
    if (apply_delay_ && --apply_delay_ == 0) apply_settings();

    // Radio slot free and packets waiting behind it -> load the next one.
    if (!tx_pending_ && !is_transmitting_ && !tx_queue_.empty()) {
        const std::vector<uint8_t> next = std::move(tx_queue_.front());
        tx_queue_.erase(tx_queue_.begin());
        queue_tx(next.data(), next.size());
    }

    // One-shot TX: fire exactly once when countdown reaches zero - but listen before
    // talking. trigger_tx() unloads the RX baseband, so transmitting while a packet is
    // streaming destroys that reception; at LongFast a full NodeInfo takes ~0.8 s on
    // air, ten times the TX countdown, which is why long packets never survived while
    // short ones did. The hold is capped so a lost "reception ended" status can never
    // wedge the transmitter for good.
    const bool rx_hold = rx_busy_ && (uptime_ticks_ - rx_busy_tick_) < RX_HOLD_TICKS;
    if (tx_pending_ && !is_transmitting_ && tx_delay_ > 0 && !rx_hold) {
        if (--tx_delay_ == 0)
            trigger_tx();
    }
}

void MeshtasticView::on_lora_rx_status(const LoRaRxStatusMessage* msg) {
    if (!msg) return;
    rx_busy_ = msg->receiving;
    rx_busy_tick_ = uptime_ticks_;
}

bool MeshtasticView::queue_tx(const uint8_t* data, size_t len, bool from_user) {
    if (!len) return false;
    // Receive-only: the antenna stays quiet, whatever asked to transmit - receipts,
    // relays, announcements and the user's own messages alike.
    if (cfg_.radio_mode == 2) return false;
    if (tx_pending_) {
        // The radio holds one packet at a time. Queue behind it instead of throwing the
        // packet away: a relay in flight used to silently swallow a message the user had
        // just typed, which then sat "pending" and timed out red although it was never
        // transmitted. A typed message outranks a relay, so when the queue is full it
        // evicts the oldest queued packet rather than being dropped itself.
        if (tx_queue_.size() >= TX_QUEUE_MAX) {
            if (!from_user) {
                router_.note_tx_dropped();
                return false;
            }
            tx_queue_.erase(tx_queue_.begin());
            router_.note_tx_dropped();  // the one we made room by discarding
        }
        tx_queue_.emplace_back(data, data + std::min(len, sizeof(tx_buf_)));
        router_.note_tx();
        return true;
    }
    memcpy(tx_buf_, data, std::min(len, sizeof(tx_buf_)));
    tx_len_ = len;
    tx_pending_ = true;
    tx_delay_ = (uptime_ticks_ < 30) ? (30 - uptime_ticks_ + 5) : 5;
    chat_view_.set_pkt_indicator(true);
    router_.note_tx();
    return true;
}

void MeshtasticView::store_and_forward(const meshtastic::MeshPacket& pkt) {
    if (!cfg_.sf_enabled || pkt.data.portnum != PortNum::TEXT_MESSAGE) return;
    if (pkt.header.from == cfg_.node_id) return;

    std::string text = pkt.text_payload();
    while (!text.empty() && static_cast<uint8_t>(text.back()) < 0x20) text.pop_back();
    if (text.empty()) return;

    // "SF" (case-insensitive) = a history request: queue the stored texts back to the asker.
    if (text.size() == 2 && (text[0] == 'S' || text[0] == 's') &&
        (text[1] == 'F' || text[1] == 'f')) {
        sf_replay_.clear();
        sf_replay_dest_ = pkt.header.from;
        // Who may be given what, exactly as Meshtastic decides it: a broadcast on the
        // channel the question came in on, or a message addressed to the asker itself.
        // Never someone else's private message - replaying those in clear to whoever
        // typed "SF" is what this used to do.
        for (const auto& m : sf_store_) {
            const bool bcast = (m.to == meshtastic::BROADCAST_ADDR &&
                                m.channel == pkt.channel_index);
            const bool theirs = (m.to == pkt.header.from);
            if ((bcast || theirs) && m.from != pkt.header.from)
                sf_replay_.push_back("[SF] " + m.text);
        }
        if (sf_replay_.empty()) sf_replay_.push_back("[SF] no stored messages");
        if (cfg_.chat.sf_notify) chat_view_.write_console("* SF: history requested by " +
                                                          to_string_hex(pkt.header.from & 0xFFFF, 4) + ", " +
                                                          to_string_dec_uint(static_cast<uint32_t>(sf_replay_.size())) +
                                                          " msgs\n");
        return;  // don't store the request itself
    }
    // A message addressed to US is not stored. We are its destination, not a waypoint on
    // the way to one: it has arrived, it is on screen, and nobody can ever be handed it
    // back - replay gives a message to the node it was addressed to, and we do not ask
    // ourselves for history. Every one of them was quietly spending a slot in a ring of
    // twelve and evicting the messages this store actually exists for. It also explains
    // the "stored" notices that appeared in private conversations for messages that had
    // plainly arrived.
    //
    // The test sits here rather than at the top because a history request arrives as a
    // direct message to us, and must still be answered.
    if (pkt.header.to == cfg_.node_id) return;

    // Remember it (ring of the last cfg_.sf_max texts), addressee included - a message
    // passing through us for someone who is not listening yet is precisely what this
    // store is for, and it is handed over only to that someone.
    const size_t cap = cfg_.sf_max ? cfg_.sf_max : 1;
    // Already full means the oldest is being dropped to make room. Say so with a "+":
    // "8/8" on its own reads as "just filled up" however long it has been overwriting,
    // and the difference decides whether the buffer is still worth asking for.
    const bool wrapped = sf_store_.size() >= cap;
    while (sf_store_.size() >= cap) sf_store_.erase(sf_store_.begin());
    sf_store_.push_back({pkt.header.from, pkt.header.to, text, uptime_ticks_,
                         pkt.channel_index});
    if (cfg_.chat.sf_notify) chat_view_.write_console("* SF: stored " +
                                                      to_string_dec_uint(static_cast<uint32_t>(sf_store_.size())) +
                                                      (wrapped ? "+/" : "/") + to_string_dec_uint(static_cast<uint32_t>(cap)) +
                                                      " from " + to_string_hex(pkt.header.from & 0xFFFF, 4) + "\n");
}

std::string MeshtasticView::active_channel_name() const {
    // The primary channel is named after the preset unless the user has renamed it -
    // the same name the channels screen shows, and the same one the channel hash is
    // derived from, so a notice quoting it can be compared with a stock node's setup.
    // Both indices come out of saved settings, and both are clamped elsewhere before
    // anything can call this. Clamped here as well because "elsewhere" is not a
    // guarantee a reader can check at a glance, and the cost is two comparisons.
    if (cfg_.active_channel == 0 || cfg_.active_channel > MeshSettings::NUM_CUSTOM) {
        const size_t np = sizeof(meshtastic::PRESET_CHANNEL_NAMES) /
                          sizeof(meshtastic::PRESET_CHANNEL_NAMES[0]);
        const uint8_t pi = (cfg_.preset_idx < np) ? cfg_.preset_idx : 0;
        return cfg_.primary_name.empty()
                   ? std::string(meshtastic::PRESET_CHANNEL_NAMES[pi])
                   : cfg_.primary_name;
    }
    return cfg_.ch_name[cfg_.active_channel - 1];
}

std::string MeshtasticView::random_channel_key() {
    // Seeded from the RF noise pool that every received packet stirs, so the key is
    // not a function of the boot clock alone. 32 hex characters is what
    // derive_channel_key() takes as a raw AES-128 key.
    static const char* HEX = "0123456789abcdef";
    std::string out;
    out.reserve(32);
    for (int i = 0; i < 4; i++) {
        const uint32_t w = rand_next() ^ (entropy_pool_ * (i + 1u)) ^ (uptime_ticks_ << i);
        for (int k = 28; k >= 0; k -= 4) out += HEX[(w >> k) & 0xF];
    }
    return out;
}

void MeshtasticView::broadcast_nodeinfo() {
    uint8_t* const buf = pkt_scratch;
    size_t len = router_.build_nodeinfo_tx(buf, sizeof(pkt_scratch),
                                           cfg_.node_long_name, cfg_.node_short_name);
    if (len == 0) return;
    // Announce it in the chat only if the packet was really accepted for transmission -
    // "+key" confirms the NodeInfo carried our public key (so peers can PKC us).
    if (queue_tx(buf, len))
        chat_view_.write_console(std::string("* NodeInfo sent") +
                                 (router_.pki_enabled() ? " +key: " : ": ") +
                                 cfg_.node_long_name + "\n");
}

void MeshtasticView::send_bell(uint8_t channel, uint32_t node) {
    // A message whose whole content is the bell character: Meshtastic clients treat it
    // as an alert and ring, which is the point - it needs no words.
    const bool to_node = (channel == 0xFF);
    if (!to_node && channel != cfg_.active_channel) {
        cfg_.active_channel = channel;  // ring on the channel that was highlighted
        apply_channels();
    }
    // Word for word what the phone app sends, so a bell from us looks to everyone else
    // exactly like a bell from a phone. The bare character alone was invisible: other
    // clients rang but showed an empty message, and ours said "Bell", which is not what
    // anyone else writes.
    static const std::string bell = std::string(BELL_EMOJI) + BELL_WORDS + BELL_CHAR;
    uint8_t* const buf = pkt_scratch;
    const uint32_t dest = to_node ? node : BROADCAST_ADDR;
    const size_t len = router_.build_text_tx(buf, sizeof(pkt_scratch), bell.c_str(),
                                             bell.size(), dest, to_node);
    if (len > 0 && queue_tx(buf, len, true))
        chat_view_.write_console(std::string("* bell sent to ") +
                                 (to_node ? to_string_hex(node & 0xFFFF, 4)
                                          : "channel " + to_string_dec_uint(channel)) +
                                 "\n");
}

void MeshtasticView::broadcast_position() {
    // Without a live fix everything about the position comes from the settings - and
    // that has to include the coordinates. Only the altitude and the satellite count
    // were being copied here, while latitude and longitude were left at whatever
    // on_gps_update had last written: nothing at all on a device with no GPS. So every
    // packet went out as 0,0 - the null island - with a believable altitude and a
    // believable satellite count attached. Our own Map looked right the whole time,
    // because refresh_own_position() pushes the fixed point into the map view and
    // nowhere else, so the one screen that could have caught this agreed with us.
    if (!has_gps_fix_) {
        gps_position_.latitude = cfg_.fixed_lat;
        gps_position_.longitude = cfg_.fixed_lon;
        gps_position_.altitude_m = cfg_.telemetry.fixed_alt_m;
        gps_position_.satellites_in_view = cfg_.telemetry.fixed_sats;
        // Nothing to report is not the same as being at 0,0. A stock node with no
        // position sends no POSITION at all rather than putting itself off the coast
        // of Africa on everyone's map.
        if (cfg_.fixed_lat == 0.0f && cfg_.fixed_lon == 0.0f) {
            chat_view_.write_console("* no position set\n");
            return;
        }
    }
    // Stamped only once the clock is real; the encoder drops the field when it is 0,
    // which reads as "undated" rather than as 1980.
    gps_position_.timestamp = now_unix();
    uint8_t* const buf = pkt_scratch;
    size_t len = router_.build_position_tx(buf, sizeof(pkt_scratch), gps_position_);
    if (len > 0 && queue_tx(buf, len)) announce_position();
}

void MeshtasticView::broadcast_neighborinfo() {
    // Meshtastic keeps this off the public channel: runOnce() only puts it on the air
    // when the primary channel is not the default one, or the frequency slot is not the
    // default either. It is a heavy packet and the shared channel is busy enough.
    if (cfg_.active_channel == 0 && cfg_.freq_slot == 0) return;
    uint8_t* const buf = pkt_scratch;
    const size_t len = router_.build_neighborinfo_tx(buf, sizeof(pkt_scratch),
                                                     cfg_.nbr_min * 60u);
    if (len > 0 && queue_tx(buf, len))
        chat_view_.write_console("* neighbors sent\n");
}

void MeshtasticView::announce_position() {
    // Identity, position and metrics travel as three separate packets - NodeInfo says
    // only who we are - so each says so for itself. The coordinates themselves are on
    // the Data tab already; repeating them here would only be noise.
    const bool live = has_gps_fix_ && cfg_.gps_enabled;
    chat_view_.write_console(std::string("* position sent (") +
                             (live ? "live GPS" : "fixed") + ")\n");
}

void MeshtasticView::read_battery() {
    // BatteryStateMessage is only emitted by the Battery Info app, so read the
    // fuel gauge directly (i2c0 is free during normal operation - the charger
    // loop only runs in deep-sleep/charge mode).
    if (!battery::BatteryManagement::isDetected()) return;
    uint8_t valid_mask = 0, percent = 0;
    uint16_t voltage_mv = 0;
    int32_t current = 0;
    bool may_have_changed = false;  // next's gauge reports this; we read every tick anyway
    battery::BatteryManagement::getBatteryInfo(valid_mask, percent, voltage_mv, current,
                                               may_have_changed);
    if (valid_mask == 0) return;
    // The gauge does not answer every field on every read, and treating a missing one
    // as zero made the row flick between a reading and nothing. Keep what was last
    // valid and update only what this read actually returned.
    if (valid_mask & battery::BatteryManagement::BATT_VALID_PERCENT) batt_pct_ = percent;
    if (valid_mask & battery::BatteryManagement::BATT_VALID_VOLTAGE) batt_mv_ = voltage_mv;
    if (valid_mask & battery::BatteryManagement::BATT_VALID_CURRENT) {
        // Over 25 mA into the pack means the charger is running - the same threshold
        // the Battery app uses, and the only charge indication the gauge gives.
        batt_charging_ = (current >= 25);
        batt_have_current_ = true;
    }
    if (!batt_mv_) return;
    telemetry_.battery_level = batt_pct_;
    telemetry_.voltage = batt_mv_ / 1000.0f;
    telemetry_.has_device = true;
    map_view_.set_battery(batt_pct_, batt_mv_ / 1000.0f,
                          batt_have_current_ && batt_charging_);
}

void MeshtasticView::on_environment_update(const EnvironmentDataMessage* msg) {
    if (!msg) return;
    telemetry_.temperature = msg->temperature;
    telemetry_.relative_humidity = msg->humidity;
    telemetry_.barometric_pressure = msg->pressure;
    telemetry_.has_env = true;
    map_view_.set_environment(msg->temperature, msg->humidity, msg->pressure);
}

void MeshtasticView::apply_telemetry_overrides(TelemetryData& t) const {
    // Uptime first: the callers used to write the real figure AFTER this ran, which
    // would have overwritten a custom one.
    switch (cfg_.telemetry.up_mode) {
        case 1:
            t.uptime_seconds = cfg_.telemetry.up_hours * 3600u;
            t.stat_uptime = t.uptime_seconds;
            t.has_device = true;
            break;
        case 2:
            t.send_uptime = false;
            break;
        default:
            break;  // real - filled in by the caller
    }
    switch (cfg_.telemetry.batt_mode) {
        case 1:
            t.battery_level = cfg_.telemetry.batt_pct;
            t.has_device = true;
            break;  // custom %
        case 2:
            t.battery_level = 101;
            t.has_device = true;
            break;  // charging
        case 3:
            t.send_battery = false;
            break;  // off
        default:
            break;  // real
    }
    switch (cfg_.telemetry.volt_mode) {
        case 1:
            t.voltage = cfg_.telemetry.volt_dV / 10.0f;
            t.has_device = true;
            break;  // custom
        case 2:
            t.send_voltage = false;
            break;  // off
        default:
            break;  // real
    }
    // Airtime: "Real" sends the measured figures the app keeps for the last hour.
    switch (cfg_.telemetry.util_mode) {
        case 1:
            t.channel_utilization = static_cast<float>(cfg_.telemetry.chutil_pct);
            t.air_util_tx = static_cast<float>(cfg_.telemetry.airutil_pct);
            t.send_util = true;
            t.has_device = true;
            break;
        case 2:
            t.send_util = false;
            break;
        default:
            t.send_util = true;
            t.has_device = true;
            break;
    }
    // Environment and air quality are entirely table-driven now: temperature, humidity
    // and pressure are entries in the sensor table like everything else, so nothing is
    // added here - broadcast_telemetry turns on whichever message their fields belong to.
    t.send_env_base = false;
    t.has_env = false;
    t.has_air = false;
}

void MeshtasticView::broadcast_telemetry() {
    read_battery();
    // Build a TX copy so the local Map keeps showing the REAL readings while we
    // broadcast whatever the user configured (real / custom / charging).
    static TelemetryData tx;  // see the note above: too large for a stack frame
    tx = telemetry_;
    apply_telemetry_overrides(tx);
    // Fields the device has no sensor for, as the user filled them in. Collected before
    // anything is decided: a metric someone entered by hand is itself a reason to send
    // the message it belongs to, even when the rest of that group is switched off.
    static TelemetryData::ExtraMetric extras[NUM_EXTRA_METRICS];
    size_t n_extra = 0;
    bool have_power = false, have_health = false;
    for (size_t k = 0; k < NUM_EXTRA_METRICS; k++) {
        if (!cfg_.telemetry.extra_on[k]) continue;
        const auto& d = EXTRA_METRICS[k];
        extras[n_extra++] = {d.variant, d.field, d.is_float,
                             static_cast<float>(cfg_.telemetry.extra_raw[k]) * d.scale};
        switch (d.variant) {
            case TelemetryData::ENVIRONMENT:
                tx.has_env = true;
                break;
            case TelemetryData::AIR_QUALITY:
                tx.has_air = true;
                break;
            case TelemetryData::POWER:
                have_power = true;
                break;
            case TelemetryData::HEALTH:
                have_health = true;
                break;
            default:
                break;
        }
    }

    // Nothing to report until we've seen a real reading or the user set a custom.
    if (!tx.has_device && !tx.has_env && !tx.has_air && !have_power && !have_health)
        return;
    tx.timestamp = now_unix();
    // Real uptime only when the override did not put its own there.
    if (cfg_.telemetry.up_mode != 1)
        tx.uptime_seconds = uptime_ticks_ / 60;  // ~60 Hz ticks -> seconds

    // Telemetry.variant is a oneof -> one metrics type per packet. Rotate through the
    // enabled ones so each gets its turn on the air instead of starving.
    const bool have[5] = {tx.has_device, tx.has_env, tx.has_air, have_power, have_health};
    uint8_t v = telemetry_turn_;
    for (uint8_t i = 0; i < 5; i++, v = (v + 1) % 5)
        if (have[v]) break;
    if (!have[v]) return;
    telemetry_turn_ = (v + 1) % 5;
    uint8_t* const buf = pkt_scratch;
    size_t len = router_.build_telemetry_tx(buf, sizeof(pkt_scratch), tx,
                                            static_cast<TelemetryData::Variant>(v),
                                            extras, n_extra);
    if (len > 0 && queue_tx(buf, len)) {
        // Which set of metrics went out - they take turns, one per packet.
        static const char* const kinds[5] = {"device", "environment", "air quality",
                                             "power", "health"};
        chat_view_.write_console(std::string("* telemetry sent (") + kinds[v] + ")\n");
    }
}

void MeshtasticView::derive_channel_key(const std::string& text, uint8_t out[16]) {
    bool all_hex = (text.size() == 32);
    for (size_t i = 0; all_hex && i < 32; i++)
        if (hex_digit(text[i]) < 0) all_hex = false;
    if (all_hex) {  // exact 32 hex -> raw AES-128 key
        for (int i = 0; i < 16; i++)
            out[i] = static_cast<uint8_t>((hex_digit(text[2 * i]) << 4) | hex_digit(text[2 * i + 1]));
        return;
    }
    for (int i = 0; i < 16; i++) out[i] = 0;  // else derive from passphrase
    if (text.empty()) {
        out[0] = 1;
        return;
    }
    for (size_t i = 0; i < 16; i++)
        out[i] = static_cast<uint8_t>(text[i % text.size()] ^ (0x9E * (i + 1)));
}

void MeshtasticView::apply_channels() {
    // Slot 0 = primary (preset default channel name + well-known PSK).
    // The name is whatever the user set, falling back to the preset's. Deriving it from
    // the preset alone only ever worked against a node whose own primary name happened
    // to match - and a configured Meshtastic node stores an explicit one that does not
    // change with its preset.
    if (cfg_.primary_name.empty()) {
        channel_hash_ = meshtastic::preset_channel_hash(cfg_.preset_idx, DEFAULT_PSK,
                                                        sizeof(DEFAULT_PSK));
    } else {
        channel_hash_ = meshtastic::channel_hash(cfg_.primary_name.c_str(), DEFAULT_PSK,
                                                 sizeof(DEFAULT_PSK));
    }
    router_.set_channel(0, channel_hash_, DEFAULT_PSK, sizeof(DEFAULT_PSK), true);
    // Slots 1..MeshSettings::NUM_CUSTOM = custom channels (name + passphrase/key).
    for (int i = 0; i < MeshSettings::NUM_CUSTOM; i++) {
        const bool on = cfg_.ch_enabled[i] && !cfg_.ch_name[i].empty();
        if (on) {
            uint8_t key[16];
            derive_channel_key(cfg_.ch_key[i], key);
            router_.set_channel(i + 1,
                                meshtastic::channel_hash(cfg_.ch_name[i].c_str(), key, 16),
                                key, 16, true);
        } else {
            router_.set_channel(i + 1, 0, nullptr, 0, false);
        }
    }
    if (cfg_.active_channel > MeshSettings::NUM_CUSTOM) cfg_.active_channel = 0;
    if (cfg_.active_channel != 0 &&
        !(cfg_.ch_enabled[cfg_.active_channel - 1] && !cfg_.ch_name[cfg_.active_channel - 1].empty()))
        cfg_.active_channel = 0;  // active slot got cleared -> primary
    router_.set_active_channel(cfg_.active_channel);
    chat_view_.set_channel_label(
        "Ch:" + (cfg_.active_channel == 0 ? std::string("P")
                                          : to_string_dec_uint(cfg_.active_channel)));
    // Padlock = PRIVACY, not "encrypted at all": the primary/public channel rides the
    // well-known key (anyone can read it), so it shows open. Only a custom channel with
    // its own passphrase is truly private -> closed green padlock.
    chat_view_.set_encrypted(cfg_.active_channel != 0 &&
                             !cfg_.ch_key[cfg_.active_channel - 1].empty());
    chat_view_.set_channel_index(cfg_.active_channel);
}

void MeshtasticView::update_utilisation() {
    // Sample the two busy flags every tick, then roll the window on every bucket.
    if (rx_busy_ && util_rx_[util_slot_] < 0xFFFF) util_rx_[util_slot_]++;
    if (tx_pending_ && util_tx_[util_slot_] < 0xFFFF) util_tx_[util_slot_]++;
    if (util_span_ticks_ < UTIL_BUCKETS * UTIL_BUCKET_TK) util_span_ticks_++;

    if (++util_slot_ticks_ >= UTIL_BUCKET_TK) {
        util_slot_ticks_ = 0;
        util_slot_ = (util_slot_ + 1) % UTIL_BUCKETS;
        util_rx_[util_slot_] = 0;
        util_tx_[util_slot_] = 0;
    }
    // Totals once a second: the window rolls every five minutes, but the figures are on
    // screen and should not sit at zero for the first five.
    if (util_slot_ticks_ % 60u) return;

    uint32_t rx = 0, tx = 0;
    for (uint8_t i = 0; i < UTIL_BUCKETS; i++) {
        rx += util_rx_[i];
        tx += util_tx_[i];
    }
    const uint32_t span = util_span_ticks_ ? util_span_ticks_ : 1;
    telemetry_.channel_utilization = 100.0f * static_cast<float>(rx) / static_cast<float>(span);
    telemetry_.air_util_tx = 100.0f * static_cast<float>(tx) / static_cast<float>(span);
    telemetry_.has_device = true;
    map_view_.set_utilisation(telemetry_.channel_utilization, telemetry_.air_util_tx);
    map_view_.set_uptime(uptime_ticks_ / 60);  // ticks are ~60 Hz; the wire carries seconds
}

void MeshtasticView::save_known_nodes() {
    // Every node we can name or hold a key for, not only the ones we have a private
    // thread with. A peer's public key travels in its NodeInfo and nowhere else, and a
    // stock node sends that once every few hours - so losing the key on restart meant
    // being unable to open that peer's encrypted direct messages until it next chose to
    // introduce itself. Keeping the key is a dozen short lines on the card.
    // The last field marks a private thread, so the conversations list is restored too.
    auto f = std::make_unique<File>();
    if (f->create(u"/LOGS/mesh_dms.txt").is_valid()) return;
    for (size_t i = 0; i < node_db_.count(); i++) {
        const NodeEntry* e = node_db_.at(i);
        if (!e || !e->active) continue;
        if (!e->has_pubkey && !e->long_name[0] && !e->dm_thread) continue;  // nothing to keep
        std::string line = to_string_hex(e->node_id, 8) + "," + e->long_name + "," +
                           e->short_name + ",";
        if (e->has_pubkey)
            for (int k = 0; k < 32; k++) line += to_string_hex(e->public_key[k], 2);
        // Thread flag and the colour the user gave this node. The colour was lost on
        // every restart, so old chat lines kept the colour saved with them while new
        // ones from the same node came out plain - "the colours sometimes disappear".
        line += e->dm_thread ? ",1," : ",0,";
        line += to_string_dec_uint(e->colour) + "\n";
        f->write(line.data(), line.size());
    }
}

void MeshtasticView::load_known_nodes() {
    auto f = std::make_unique<File>();
    if (f->open(u"/LOGS/mesh_dms.txt").is_valid()) return;
    const size_t size = f->size();
    if (!size || size > 4096) return;
    std::string buf(size, '\0');
    const auto rd = f->read(&buf[0], size);
    if (!rd) return;
    buf.resize(*rd);

    size_t pos = 0;
    while (pos < buf.size()) {
        const size_t eol = buf.find('\n', pos);
        const std::string line = buf.substr(pos, (eol == std::string::npos) ? std::string::npos
                                                                            : eol - pos);
        pos = (eol == std::string::npos) ? buf.size() : eol + 1;
        // id,long,short,pubkey-hex - the last field is empty for a node we have no key for.
        const size_t c1 = line.find(','), c2 = line.find(',', c1 + 1),
                     c3 = line.find(',', c2 + 1);
        if (c1 == std::string::npos || c2 == std::string::npos || c3 == std::string::npos)
            continue;
        uint32_t id = 0;
        for (size_t k = 0; k < c1; k++) {
            const int v = hex_digit(line[k]);
            if (v < 0) {
                id = 0;
                break;
            }
            id = (id << 4) | static_cast<uint32_t>(v);
        }
        if (!id) continue;
        const std::string ln = line.substr(c1 + 1, c2 - c1 - 1);
        const std::string sn = line.substr(c2 + 1, c3 - c2 - 1);
        node_db_.update_node(id, ln.empty() ? nullptr : ln.c_str(),
                             sn.empty() ? nullptr : sn.c_str());
        // id,long,short,pubkey-hex[,dm]. Files written before the dm flag existed have
        // four fields and every line in them was a thread, which is what the default says.
        const size_t c4 = line.find(',', c3 + 1);
        const std::string key = line.substr(c3 + 1, (c4 == std::string::npos)
                                                        ? std::string::npos
                                                        : c4 - c3 - 1);
        const bool is_thread = (c4 == std::string::npos) || (line[c4 + 1] != '0');
        // ...and the colour after it, if this file is new enough to carry one.
        const size_t c5 = (c4 == std::string::npos) ? std::string::npos
                                                    : line.find(',', c4 + 1);
        uint8_t colour = 0;
        if (c5 != std::string::npos && c5 + 1 < line.size())
            colour = static_cast<uint8_t>(atoi(line.c_str() + c5 + 1) & 7);
        if (key.size() >= 64) {
            uint8_t pub[32];
            bool ok = true;
            for (int k = 0; k < 32 && ok; k++) {
                const int hi = hex_digit(key[2 * k]), lo = hex_digit(key[2 * k + 1]);
                if (hi < 0 || lo < 0)
                    ok = false;
                else
                    pub[k] = static_cast<uint8_t>((hi << 4) | lo);
            }
            if (ok) node_db_.set_pubkey(id, pub);
        }
        if (is_thread) node_db_.mark_dm(id);
        if (colour) {
            NodeEntry* n = node_db_.find(id);
            if (n) n->colour = colour;
        }
    }
}

void MeshtasticView::pack_extras() {
    static const char* HEX = "0123456789abcdef";
    cfg_.extra_pack.clear();
    cfg_.extra_pack.reserve(NUM_EXTRA_METRICS * 5);
    for (size_t k = 0; k < NUM_EXTRA_METRICS; k++) {
        cfg_.extra_pack += cfg_.telemetry.extra_on[k] ? '1' : '0';
        const uint16_t v = cfg_.telemetry.extra_raw[k];
        for (int sh = 12; sh >= 0; sh -= 4) cfg_.extra_pack += HEX[(v >> sh) & 0xF];
    }
}

void MeshtasticView::unpack_extras() {
    for (size_t k = 0; k < NUM_EXTRA_METRICS; k++) {
        const size_t at = k * 5;
        if (at + 4 >= cfg_.extra_pack.size()) break;
        cfg_.telemetry.extra_on[k] = (cfg_.extra_pack[at] == '1') ? 1 : 0;
        uint32_t v = 0;
        bool ok = true;
        for (int d = 1; d <= 4 && ok; d++) {
            const int hv = hex_digit(cfg_.extra_pack[at + d]);
            if (hv < 0)
                ok = false;
            else
                v = (v << 4) | static_cast<uint32_t>(hv);
        }
        if (ok) cfg_.telemetry.extra_raw[k] = static_cast<uint16_t>(v);
    }
}

void MeshtasticView::apply_key_repeat() {
    set_switches_repeat_config(cfg_.key_repeat ? saved_repeat_ : SwitchesState{});
}

void MeshtasticView::persist() {
    // Mirror the char[] node names into their bound std::string copies, then write
    // the whole settings block to /SETTINGS/meshtastic.ini right now.
    cfg_.node_long_str = cfg_.node_long_name;
    cfg_.node_short_str = cfg_.node_short_name;
    pack_extras();
    settings_.save();
}

void MeshtasticView::init_pki() {
    // Decode a persisted 64-hex-char private key, if present and valid.
    bool have = cfg_.pki_priv_hex.size() == 64;
    if (have) {
        for (size_t i = 0; i < 32; ++i) {
            const int hi = hex_digit(cfg_.pki_priv_hex[2 * i]);
            const int lo = hex_digit(cfg_.pki_priv_hex[2 * i + 1]);
            if (hi < 0 || lo < 0) {
                have = false;
                break;
            }
            pki_priv_[i] = static_cast<uint8_t>((hi << 4) | lo);
        }
    }
    // Generate a key only when PKC is actually enabled and none exists yet. Deferring
    // generation to the moment the user turns PKC on (rather than at cold-boot ctor)
    // means the entropy pool has already gathered real RF noise from received packets,
    // and the enable time itself is unpredictable - both far stronger than a boot clock.
    if (!have && cfg_.pki_enabled) {
        generate_pki_key();
        have = true;
    }
    if (have) meshtastic::pki::x25519_base(pki_pub_, pki_priv_);
    // Advertise + use PKC only when enabled AND we actually hold a key.
    router_.set_pki(pki_priv_, pki_pub_, have && cfg_.pki_enabled);
}

void MeshtasticView::generate_pki_key() {
    // Mix every entropy source we have through SHA-256, then clamp per RFC 7748:
    //  - full RTC date+time (not just seconds),
    //  - uptime at the (unpredictable) instant PKC was enabled,
    //  - the RF-noise entropy pool accumulated from received-packet RSSI/timing,
    //  - the node id and PRNG state.
    const auto t = rtc_time::now();
    const uint32_t rtc_packed =
        (static_cast<uint32_t>(t.year()) << 26) ^ (static_cast<uint32_t>(t.month()) << 22) ^
        (static_cast<uint32_t>(t.day()) << 17) ^ (static_cast<uint32_t>(t.hour()) << 12) ^
        (static_cast<uint32_t>(t.minute()) << 6) ^ static_cast<uint32_t>(t.second());
    uint32_t words[8] = {
        rtc_packed, uptime_ticks_, cfg_.node_id, rand_state_,
        entropy_pool_, entropy_pool_ ^ 0x9E3779B9u, rand_next(), rand_next()};
    meshtastic::pki::sha256(reinterpret_cast<const uint8_t*>(words),
                            sizeof(words), pki_priv_);
    meshtastic::pki::clamp_private(pki_priv_);
    static const char H[] = "0123456789abcdef";
    cfg_.pki_priv_hex.clear();
    for (size_t i = 0; i < 32; ++i) {
        cfg_.pki_priv_hex += H[pki_priv_[i] >> 4];
        cfg_.pki_priv_hex += H[pki_priv_[i] & 0xF];
    }
    persist();  // keep the identity stable across reboots
}

uint32_t MeshtasticView::rand_next() {
    uint32_t x = rand_state_;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rand_state_ = x ? x : 0x1u;
    return rand_state_;
}

void MeshtasticView::randomize_identity() {
    // Fold in the RTC so successive boots pick different identities.
    rand_state_ ^= (static_cast<uint32_t>(rtc_time::now().second()) + 1u) * 0x9e3779b9u;

    if (cfg_.rand_mask & MeshSettings::RAND_ID) {
        cfg_.node_id = rand_next();
        if (cfg_.node_id == 0) cfg_.node_id = 0xDEADBEEFu;
        router_.set_local_node(cfg_.node_id, DEFAULT_PSK, sizeof(DEFAULT_PSK));
    }
    if (cfg_.rand_mask & MeshSettings::RAND_NAME) {
        // Names follow the id when it changes, so the pair stays plausible.
        const std::string hx = to_string_hex(cfg_.node_id, 8);
        const std::string sn = hx.substr(4, 4);
        const std::string ln = "Node-" + hx.substr(0, 4);
        strncpy(cfg_.node_short_name, sn.c_str(), sizeof(cfg_.node_short_name) - 1);
        cfg_.node_short_name[sizeof(cfg_.node_short_name) - 1] = '\0';
        strncpy(cfg_.node_long_name, ln.c_str(), sizeof(cfg_.node_long_name) - 1);
        cfg_.node_long_name[sizeof(cfg_.node_long_name) - 1] = '\0';
    }
    if (cfg_.rand_mask & MeshSettings::RAND_DEV) {
        // A random common board, so peers see a different device each time.
        static const uint8_t models[] = {43, 10, 4, 12, 9, 50, 71, 25};
        cfg_.hw_model = models[rand_next() % (sizeof(models) / sizeof(models[0]))];
        router_.set_hw_model(cfg_.hw_model);
    }
    if (cfg_.rand_mask & MeshSettings::RAND_ROLE) {
        // Only the roles that behave like an ordinary participant: picking REPEATER
        // or a TAK role at random would change how we treat other people's traffic.
        static const uint8_t roles[] = {0, 1, 5, 6, 8};  // client, mute, tracker, sensor, hidden
        cfg_.node_role = roles[rand_next() % (sizeof(roles) / sizeof(roles[0]))];
        router_.set_role(cfg_.node_role);
    }

    if (cfg_.rand_mask & MeshSettings::RAND_POS) {
        // Move the broadcast position, keeping it plausible rather than teleporting
        // across the planet: a random point within about half a degree (~50 km) of
        // where we claim to be. Live GPS has to be off, or the real fix would win.
        auto jitter = [this]() {
            return (static_cast<float>(rand_next() % 100000u) / 100000.0f - 0.5f);
        };
        float base_lat = cfg_.fixed_lat, base_lon = cfg_.fixed_lon;
        if (base_lat == 0.0f && base_lon == 0.0f) {  // never set: pick something real
            base_lat = 55.0f;
            base_lon = 37.0f;
        }
        cfg_.fixed_lat = base_lat + jitter();
        cfg_.fixed_lon = base_lon + jitter();
        cfg_.gps_enabled = false;
        refresh_own_position();
    }
    if (cfg_.rand_mask & MeshSettings::RAND_TEL) {
        // Plausible readings, not noise: a receiver that sees 300% humidity knows the
        // node is lying, which rather defeats the purpose.
        cfg_.telemetry.batt_mode = 1;
        cfg_.telemetry.batt_pct = static_cast<uint8_t>(20 + rand_next() % 80);
        cfg_.telemetry.volt_mode = 1;
        cfg_.telemetry.volt_dV = static_cast<uint8_t>(33 + rand_next() % 9);  // 3.3-4.1 V
        // Everything a sensor would report lives in the metrics table, each entry with
        // its own believable ceiling, so one loop covers temperature through pulse.
        for (size_t k = 0; k < NUM_EXTRA_METRICS; k++) {
            const uint16_t cap = EXTRA_METRICS[k].rnd_max ? EXTRA_METRICS[k].rnd_max : 100;
            cfg_.telemetry.extra_raw[k] = static_cast<uint16_t>(rand_next() % (cap + 1u));
            cfg_.telemetry.extra_on[k] = 1;
        }
    }

    nodeinfo_timer_ = nodeinfo_period() - 60;  // announce the new identity within ~1 s
    persist();
}

}  // namespace ui
