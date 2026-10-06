/*
 * Copyright (C) 2026 SecLBL
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

#include "ui_music.hpp"

#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"
#include "radio.hpp"
#include "string_format.hpp"
#include "ui_fileman.hpp"
#include "utility.hpp"

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <utility>

using namespace portapack;

namespace ui::external_app::music {

static void dots(Painter& painter, Point at, std::string text, size_t width, Color color, uint8_t zoom);

static std::string mmss(uint32_t seconds) {
    return to_string_dec_uint(seconds / 60 % 100, 2, '0') + ":" + to_string_dec_uint(seconds % 60, 2, '0');
}

MusicView::MusicView(NavigationView& nav)
    : nav_{nav} {
    // The view takes the focus itself, so Up from the transport can leave to the title bar.
    set_focusable(true);
    srand(LPC_RTC->CTIME0);
    // Loaded settings come from a file anyone can edit.
    for (auto& gain : eq_)
        if (gain > 2 * eq_max_db) gain = eq_max_db;
    repeat_ %= 3;
    // The baseband sample clock paces the audio; without TX enabled nobody else sets it.
    radio::set_baseband_rate(3072000);
    audio::set_rate(audio::Rate::Hz_48000);
    // The output stays on for the whole session: the baseband plays silence when there is
    // no stream. Stopping it per track would freeze stale audio in the codec buffers and
    // switch the amplifier, both audible on every seek, skip and resume.
    audio::output::start();
    set_volume(receiver_model.normalized_headphone_volume());
}

MusicView::~MusicView() {
    stop();
    audio::output::stop();
    baseband::shutdown();
}

/* Playlist ***************************************************************/

void MusicView::open_picker() {
    ensure_directory(u"MUSIC");
    auto open_view = nav_.push<FileLoadView>(".WAV");
    open_view->push_dir(u"MUSIC");
    open_view->on_changed = [this](std::filesystem::path path) {
        open_playlist(path);
    };
}

void MusicView::open_playlist(const std::filesystem::path& file) {
    dir_ = file.parent_path();
    count_ = 0;
    slot_ = 0;
    bool found = false;
    for (const auto& entry : std::filesystem::directory_iterator(dir_, u"*.WAV")) {
        if (count_ == max_tracks) break;
        if (entry.path() == file.filename()) {
            slot_ = count_;
            found = true;
        }
        order_[count_] = count_;
        count_++;
    }
    if (!count_) return;
    if (!found) {
        // The picked file lies beyond what a playlist can hold.
        stop();
        count_ = 0;
        error_ = "FOLDER > 255 TRACKS";
        redraw();
        return;
    }
    set_shuffle(shuffle_);
    load(slot_);
}

void MusicView::set_shuffle(bool on) {
    shuffle_ = on;
    if (!count_) return;

    const uint8_t current = order_[slot_];
    for (uint16_t i = 0; i < count_; i++)
        order_[i] = i;
    slot_ = current;

    if (on) {
        for (uint16_t i = count_ - 1; i > 0; i--)
            std::swap(order_[i], order_[rand() % (i + 1)]);
        // The running track becomes the start of the shuffled order.
        for (uint16_t i = 0; i < count_; i++)
            if (order_[i] == current) std::swap(order_[i], order_[0]);
        slot_ = 0;
    }
}

// NB: tracks are not kept in RAM; the folder is rescanned on every track change
// and plays in FAT directory order. Cache/sort names if folders get big or order matters.
void MusicView::load(uint16_t slot) {
    slot_ = slot;
    retries_ = 0;
    uint16_t n = order_[slot_];
    for (const auto& entry : std::filesystem::directory_iterator(dir_, u"*.WAV")) {
        if (n-- == 0) {
            file_ = dir_ / entry.path();
            break;
        }
    }
    play_from(0);
}

/* Deck *******************************************************************/

constexpr size_t header_bytes = 512;  // how much of a WAV file is searched for its chunks

// Feel of the "record". The encoder is coarse, so these want tuning on the device.
constexpr int32_t chase_frames = 4800;      // time constant the playhead follows the hand with (100 ms)
constexpr int32_t settle_frames = 96;       // this close to the hand counts as arrived
constexpr int32_t jump_frames = 36000;      // further behind than this (0.75 s): seek, don't play the way
constexpr int32_t max_speed = 2 * 256 - 8;  // Q8, just under 2x so a block's source fits the window
constexpr int32_t min_speed = 26;           // Q8, 0.1x: don't crawl forever on the last few frames

DeckReader::DeckReader(File&& file, const WavInfo& info, Deck& deck, size_t max_read)
    : file_{std::move(file)},
      info_{info},
      deck_{deck},
      max_read_{max_read},
      head_{deck.head},
      done_{deck.turned},
      seek_seen_{deck.seek_seq} {
    if (info_.format == WavInfo::IMA_ADPCM)
        block_ = std::make_unique<uint8_t[]>(info_.block_align + 3);  // + room to line it up, see read_at()
}

// Workaround: the SD card driver corrupts whole sectors read into a buffer that is not
// word aligned relative to the file position. Such reads are split so they go through
// FatFs's own aligned sector buffer instead.
File::Result<File::Size> DeckReader::read_at(uint32_t position, void* out, uint32_t bytes) {
    constexpr uint32_t sector = 512;
    auto p = static_cast<uint8_t*>(out);

    if (position != file_position_) file_.seek(position);
    file_position_ = position + bytes;

    if (((reinterpret_cast<uintptr_t>(p) - position) & 3) == 0)
        return file_.read(p, bytes);

    while (bytes) {
        uint32_t n = std::min<uint32_t>(bytes, sector - position % sector);
        if (n == sector) n = sector / 2;
        const auto result = file_.read(p, n);
        if (result.is_error()) return result;
        p += n;
        position += n;
        bytes -= n;
    }
    return File::Size{0};
}

File::Result<File::Size> DeckReader::fetch(uint32_t frame, uint32_t count, int16_t* out) {
    if (info_.format == WavInfo::PCM)
        return read_at(info_.data_start + frame * info_.block_align, out, count * info_.block_align);

    // ADPCM only decodes forwards from the start of a block, so going back inside a
    // block (reverse scratching) decodes that block again up to the wanted frame.
    while (count) {
        const uint32_t number = frame / info_.block_frames;
        const uint32_t offset = frame % info_.block_frames;
        if (number != block_number_ || offset < ima_.position()) {
            // Placed in the buffer so that it lines up with its position in the file.
            const uint32_t position = info_.data_start + number * info_.block_align;
            uint8_t* const block = block_.get() + (position & 3);
            const auto result = read_at(position, block, info_.block_align);
            if (result.is_error()) return result;
            block_number_ = number;
            ima_.start(block, info_.channels);
        }
        while (ima_.position() < offset)
            ima_.next(nullptr);

        const uint32_t n = std::min<uint32_t>(count, info_.block_frames - offset);
        for (uint32_t i = 0; i < n; i++, out += info_.channels)
            ima_.next(out);
        frame += n;
        count -= n;
    }
    return File::Size{0};
}

File::Result<File::Size> DeckReader::read(void* const buffer, const File::Size bytes) {
    const uint32_t channels_ = info_.channels;
    const uint32_t total_ = info_.frames;
    const size_t frame_bytes = channels_ * sizeof(int16_t);
    const int32_t frames = bytes / frame_bytes;
    int32_t pending = deck_.turned - done_;

    // An absolute seek from the UI (dragging the dial) overrides any turning.
    if (deck_.seek_seq != seek_seen_) {
        seek_seen_ = deck_.seek_seq;
        const uint32_t seek_to = deck_.seek_to;
        head_ = std::min(seek_to, total_);
        done_ = deck_.turned;
        pending = 0;
        deck_.head = head_;
    }

    if (std::abs(pending) > jump_frames) {
        head_ = clip<int32_t>((int32_t)head_ + pending, 0, total_);
        done_ += pending;
        pending = 0;
        deck_.head = head_;
    }

    if (std::abs(pending) >= settle_frames) {
        // Scratch: speed is proportional to the distance still to go, either direction,
        // so the sound follows the hand and slows to a stop like a record being held.
        int32_t speed = clip<int32_t>(pending * 256 / chase_frames, -max_speed, max_speed);
        if (std::abs(speed) < min_speed) speed = (pending > 0) ? min_speed : -min_speed;
        int32_t advance = speed * frames / 256;
        if (std::abs(advance) > std::abs(pending)) advance = pending;

        // NB: one seek + read per block. Without a FatFs cluster map a backward seek
        // walks the FAT from the start of the file; if reverse stutters deep into big
        // files, set up fast seek (cltbl) for the file.
        const int32_t last = total_ ? total_ - 1 : 0;
        const int32_t lo = clip<int32_t>(std::min<int32_t>(head_, head_ + advance), 0, last);
        const int32_t hi = clip<int32_t>(std::max<int32_t>(head_, head_ + advance), 0, last);
        if (!window_) window_ = std::make_unique<int16_t[]>(2 * max_read_ / sizeof(int16_t));
        const auto result = fetch(lo, hi - lo + 1, window_.get());
        if (result.is_error()) return result;

        auto out = static_cast<int16_t*>(buffer);
        const int32_t step = advance * 65536 / frames;  // Q16 source frames per output frame
        int32_t offset = 0;
        for (int32_t i = 0; i < frames; i++, offset += step) {
            const int32_t at = clip<int32_t>((int32_t)head_ + (offset >> 16), lo, hi) - lo;
            for (uint32_t c = 0; c < channels_; c++)
                *out++ = window_[at * channels_ + c];
        }

        head_ = clip<int32_t>((int32_t)head_ + advance, 0, total_);
        done_ += advance;
        deck_.head = head_;
        return File::Size{bytes};
    }

    done_ += pending;
    window_.reset();

    if (deck_.paused) {
        memset(buffer, 0, bytes);
        return File::Size{bytes};
    }

    // Normal playback. Returning 0 ends the track, but only once the blocks still queued
    // in the stream have had time to play; until then it is fed silence.
    const uint32_t count = std::min<uint32_t>(frames, total_ - head_);
    if (count == 0) {
        if (drained_ >= stream_buffers * max_read_) return File::Size{0};
        drained_ += bytes;
        memset(buffer, 0, bytes);
        return File::Size{bytes};
    }
    drained_ = 0;
    const auto result = fetch(head_, count, static_cast<int16_t*>(buffer));
    if (result.is_error()) return result;
    memset(static_cast<uint8_t*>(buffer) + count * frame_bytes, 0, bytes - count * frame_bytes);
    head_ += count;
    deck_.head = head_;
    return File::Size{bytes};
}

/* Playback ***************************************************************/

void MusicView::stop() {
    generation_++;  // whatever the old thread still reports no longer counts
    // A thread still waiting for the baseband to get ready only ends once it is let
    // through; without this, restarting twice in a row would wait on it forever.
    ready_signal = true;
    replay_thread.reset();
    ready_signal = false;
    playing_ = false;
}

void MusicView::play_from(uint32_t sample) {
    stop();

    File file;
    WavInfo info;
    error_ = nullptr;
    if (file.open(file_).is_valid()) {
        error_ = "CAN'T OPEN FILE";
    } else {
        auto header = std::make_unique<uint8_t[]>(header_bytes);
        const auto result = file.read(header.get(), header_bytes);
        error_ = parse_wav(header.get(), result.is_ok() ? result.value() : 0, file.size(), info);
    }

    if (error_) {
        rate_ = total_ = pos_ = 0;
        redraw();
        return;
    }

    rate_ = info.rate;
    const uint32_t channels = info.channels;
    total_ = info.frames;
    deck_.head = pos_ = (sample < total_) ? sample : 0;
    deck_.paused = false;
    channels_ = channels;
    apply_eq();

    // ~21 ms blocks: small enough for the deck to follow the encoder closely.
    const size_t read_size = 2048 * channels;
    replay_thread = std::make_unique<ReplayThread>(
        std::make_unique<DeckReader>(std::move(file), info, deck_, read_size),
        read_size, stream_buffers,
        &ready_signal,
        [generation = generation_](uint32_t return_code) {
            ReplayThreadDoneMessage message{return_code | (generation << 8)};
            EventDispatcher::send_message(message);
        });

    playing_ = true;
    redraw();
}

void MusicView::step(int dir, bool automatic) {
    if (!count_) return;

    if (automatic && repeat_ == 2) return play_from(0);
    // Like any player: "previous" first goes back to the start of the track.
    if (dir < 0 && pos_ > 3 * rate_) return play_from(0);

    // When the playlist advances by itself, a file it can't play is skipped over.
    uint16_t next = (slot_ + count_ + dir) % count_;
    for (uint16_t tries = 0; tries < count_; tries++) {
        if (automatic && repeat_ == 0 && next == 0) {
            stop();
            pos_ = 0;
            redraw();
            return;
        }
        load(next);
        if (!automatic || !error_) return;
        next = (next + 1) % count_;
    }
}

// Turning the encoder moves the record. Slow turns and flicks scratch; a sustained fast
// spin in one direction grows into a seek.
void MusicView::turn(int32_t delta) {
    if (!replay_thread) return;

    const auto now = chTimeNow();
    const bool forward = delta > 0;
    if ((now - last_turn_) < MS2ST(spin_gap_ms) && forward == spin_forward_) {
        if (spin_ < 255) spin_++;
    } else {
        spin_ = 0;
    }
    last_turn_ = now;
    spin_forward_ = forward;

    const int32_t boost = (spin_ > spin_start) ? std::min<int32_t>((spin_ - spin_start) / 6, 6) : 0;
    deck_.turned = deck_.turned + delta * (boost ? seek_frames << boost : frames_per_detent);
}

void MusicView::set_volume(int32_t v) {
    // Not receiver_model.set_normalized_headphone_volume(): that only reaches the codec
    // while the receiver is enabled, which it never is here.
    volume_ = clip<int32_t>(v, 0, 99);
    auto new_volume = volume_t::decibel(volume_ - 99) + audio::headphone::volume_range().max;
    persistent_memory::set_headphone_volume(new_volume);
    audio::headphone::set_volume(new_volume);
}

void MusicView::apply_eq() {
    std::array<int8_t, eq_bands> gain_db{};
    for (size_t i = 0; i < eq_bands; i++)
        gain_db[i] = eq_[i] - eq_max_db;
    baseband::set_audio_play_config(channels_, gain_db);
}

/* Input ******************************************************************/

void MusicView::activate() {
    switch (cursor_) {
        case Transport:
            if (!count_)
                open_picker();
            else if (replay_thread) {
                // The stream keeps running while paused (the deck plays silence), so pause,
                // resume and scratching while paused need no restart.
                playing_ = !playing_;
                deck_.paused = !playing_;
            } else
                play_from(pos_);
            break;
        case List:
            open_picker();
            break;
        case Shuffle:
            set_shuffle(!shuffle_);
            break;
        case Eq:
            nav_.push<EqView>(eq_, [this]() { apply_eq(); });
            break;
        case Repeat:
            repeat_ = (repeat_ + 1) % 3;
            break;
        default:
            break;
    }
    redraw();
}

bool MusicView::on_key(KeyEvent key) {
    switch (key) {
        case KeyEvent::Select:
            activate();
            break;
        case KeyEvent::Down:
            if (cursor_ == Transport) cursor_ = List;
            break;
        case KeyEvent::Up:
            if (cursor_ == Transport) return false;  // let navigation reach the title bar
            cursor_ = Transport;
            break;
        case KeyEvent::Left:
        case KeyEvent::Right: {
            const int dir = (key == KeyEvent::Right) ? 1 : -1;
            if (cursor_ == Transport)
                step(dir, false);
            else
                cursor_ = clip<int>(cursor_ + dir, List, Volume);
            break;
        }
        default:
            return false;
    }
    redraw();
    return true;
}

bool MusicView::on_encoder(EncoderEvent delta) {
    if (cursor_ == Volume) {
        set_volume(volume_ + delta);
        redraw();
    } else {
        turn(delta);
    }
    return true;
}

// Everything is placed from the screen size and the ui.hpp grid, relative to the view.
MusicView::Layout MusicView::layout() const {
    const int height = size().height();
    const int top = UI_POS_Y(3);  // below the header

    Layout l;
    l.title_y = height - UI_POS_HEIGHT(5);
    l.row_y = height - UI_POS_HEIGHT(2) - UI_POS_DEFAULT_HEIGHT_SMALL;
    l.cell = screen_width / row_items;
    l.columns = (screen_width - UI_POS_WIDTH(2)) / (glyph_width * 2);
    l.cx = screen_width / 2;
    l.cy = (top + l.title_y) / 2;
    l.radius = std::min<int>(UI_POS_WIDTH_PERCENT(30), (l.title_y - top) / 2 - UI_POS_DEFAULT_HEIGHT);
    return l;
}

// Touch: bottom row items, << and >> beside the dial, the middle of the dial for
// play/pause, and the ring itself to drag the playhead.
bool MusicView::on_touch(TouchEvent event) {
    Point p;
    const auto kind = touch_.feed(event, p);
    if (kind == Touch::None) return true;

    const auto l = layout();
    const int x = p.x() - screen_pos().x();
    const int y = p.y() - screen_pos().y();

    if (kind == Touch::Down) {
        ring_drag_ = false;
        if (y >= l.row_y - UI_POS_DEFAULT_HEIGHT) {
            cursor_ = clip<int>(List + x / l.cell, List, Volume);
            activate();
            return true;
        }
        if (std::abs(y - l.cy) > l.radius + 4) return true;

        cursor_ = Transport;
        if (x < UI_POS_X(5) || x >= UI_POS_X_RIGHT(5)) {
            step(x < UI_POS_X(5) ? -1 : 1, false);
            redraw();
            return true;
        }
        const int dx = x - l.cx, dy = y - l.cy;
        const int inside = l.radius - 22;
        if (dx * dx + dy * dy < inside * inside) {
            activate();
            return true;
        }
        ring_drag_ = true;
        redraw();
    }

    if (ring_drag_ && replay_thread) {
        // The playhead goes to the ring dot nearest to the finger.
        uint32_t nearest = 0;
        int32_t nearest_d2 = INT32_MAX;
        for (uint32_t i = 0; i < ring_dots; i++) {
            const auto d = ring_dot(i, l.radius);
            const int32_t ex = x - l.cx - d.x(), ey = y - l.cy - d.y();
            if (ex * ex + ey * ey < nearest_d2) {
                nearest_d2 = ex * ex + ey * ey;
                nearest = i;
            }
        }
        deck_.seek_to = (uint64_t)nearest * total_ / ring_dots;
        deck_.seek_seq = deck_.seek_seq + 1;
    }
    return true;
}

// Every display frame: the shown position glides to where the record really is, so
// seeking sweeps the dial instead of jumping across it.
void MusicView::animate() {
    if (!replay_thread) return;

    const int32_t gap = (int32_t)deck_.head - (int32_t)pos_;
    if (gap == 0) return;
    pos_ += (gap / 6) ? gap / 6 : gap;

    if (head_dot() != painted_dot_ || (rate_ && pos_ / rate_ != painted_seconds_))
        redraw(Tick);
}

/* Drawing ****************************************************************/

void MusicView::redraw(uint8_t what) {
    if (what > paint_) paint_ = what;
    set_dirty();
}

// "Dot matrix" text: zoomed 5x8 glyphs with a black grid over them, one dot per font pixel.
static void dots(Painter& painter, Point at, std::string text, size_t width, Color color, uint8_t zoom) {
    text.resize(width, ' ');
    const Style style{font::fixed_5x8, Color::black(), color};
    const Rect r{at, {(int)(width * glyph_width * zoom), glyph_height * zoom}};

    for (auto c : text)
        at += Point{painter.draw_char(at, style, c, zoom), 0};

    if (zoom < 2) return;
    for (int x = zoom - 1; x < r.width(); x += zoom)
        painter.draw_vline({r.left() + x, r.top()}, r.height(), Color::black());
    for (int y = zoom - 1; y < r.height(); y += zoom)
        painter.draw_hline({r.left(), r.top() + y}, r.width(), Color::black());
}

void MusicView::paint(Painter& painter) {
    const auto l = layout();
    const int x0 = screen_pos().x();
    const int y0 = screen_pos().y();
    const int right = x0 + UI_POS_X_RIGHT(1);
    const bool all = clear_ || paint_ != Tick;

    if (clear_)
        painter.fill_rectangle(screen_rect(), Color::black());
    clear_ = false;
    paint_ = 0;

    if (all) {
        dots(painter, {x0 + UI_POS_X(1), y0 + UI_POS_Y(0) + 4}, "PLAYLIST", 8, Color::light_grey(), 1);
        dots(painter, {right - 7 * glyph_width, y0 + UI_POS_Y(0) + 4},
             to_string_dec_uint(count_ ? slot_ + 1 : 0, 3, '0') + "/" + to_string_dec_uint(count_, 3, '0'),
             7, Color::light_grey(), 1);
        dots(painter, {x0 + UI_POS_X(1), y0 + UI_POS_Y(1)}, count_ ? dir_.filename().string() : "NO PLAYLIST", l.columns, Color::white(), 2);

        dots(painter, {x0 + UI_POS_X(1), y0 + l.title_y},
             error_ ? error_ : count_ ? file_.stem().string()
                                      : "PRESS SELECT",
             l.columns, error_ ? Color::red() : Color::white(), 2);

        dots(painter, {x0 + UI_POS_X(1), y0 + l.cy - glyph_height}, "<<", 2, Color::light_grey(), 2);
        dots(painter, {right - 2 * glyph_width * 2, y0 + l.cy - glyph_height}, ">>", 2, Color::light_grey(), 2);

        // Bottom row: LIST / SHUF / RPT / EQ / VOL, red bar marks the cursor.
        const std::string labels[row_items] = {
            "LIST",
            "SHUF",
            repeat_ == 2 ? "RPT1" : repeat_ == 1 ? "RPTA"
                                                 : "RPT",
            "EQ",
            "V " + to_string_dec_uint(volume_, 2)};
        const bool lit[row_items] = {true, shuffle_, repeat_ != 0, true, true};
        const int label_width = 4 * glyph_width * 2;
        for (int i = 0; i < row_items; i++) {
            const int x = x0 + UI_POS_X_TABLE(row_items, i) + (l.cell - label_width) / 2;
            dots(painter, {x, y0 + l.row_y}, labels[i], 4, lit[i] ? Color::white() : Color::grey(), 2);
            painter.fill_rectangle({x, y0 + l.row_y + 20, label_width, 3}, cursor_ == List + i ? Color::red() : Color::black());
        }
    }

    // Dial: a ring of dots that fills up with the track, red dot is the playhead.
    // Ticks come at frame rate while seeking, so only repaint what changed.
    const uint32_t head = head_dot();
    const auto shade = [](uint32_t i, uint32_t at) { return i < at ? 0 : i == at ? 1
                                                                                 : 2; };
    static const Color shades[3] = {Color::white(), Color::red(), Color::dark_grey()};
    for (uint32_t i = 0; i < ring_dots; i++) {
        if (!all && shade(i, head) == shade(i, painted_dot_)) continue;
        const auto d = ring_dot(i, l.radius);
        painter.fill_rectangle({x0 + l.cx - 2 + d.x(), y0 + l.cy - 2 + d.y(), 4, 4}, shades[shade(i, head)]);
    }
    painted_dot_ = head;

    // Clock in the middle of the dial, state above it, track length below.
    const uint32_t seconds = rate_ ? pos_ / rate_ : 0;
    if (all || seconds != painted_seconds_)
        dots(painter, {x0 + l.cx - 5 * glyph_width * 4 / 2, y0 + l.cy - glyph_height * 4 / 2}, mmss(seconds), 5, Color::white(), 4);
    painted_seconds_ = seconds;

    if (all) {
        const int x = x0 + l.cx - 7 * glyph_width / 2;
        dots(painter, {x, y0 + l.cy - UI_POS_HEIGHT(2)}, playing_ ? "PLAYING" : "PAUSED", 7, playing_ ? Color::red() : Color::light_grey(), 1);
        dots(painter, {x, y0 + l.cy + UI_POS_HEIGHT(1) + UI_POS_DEFAULT_HEIGHT_SMALL}, "/ " + mmss(rate_ ? total_ / rate_ : 0), 7, Color::light_grey(), 1);
    }
}

/* Equalizer **************************************************************/

EqView::EqView(NavigationView& nav, uint8_t* gains, std::function<void()> on_change)
    : nav_{nav},
      gains_{gains},
      on_change_{std::move(on_change)} {
    set_focusable(true);
}

void EqView::set(int band, int value) {
    band_ = clip<int>(band, 0, eq_bands - 1);
    const uint8_t gain = clip<int>(value, 0, 2 * eq_max_db);
    if (gain != gains_[band_]) {
        gains_[band_] = gain;
        on_change_();
    }
    set_dirty();
}

bool EqView::on_key(KeyEvent key) {
    switch (key) {
        case KeyEvent::Select:
            nav_.pop();
            return true;
        case KeyEvent::Left:
            set(band_ - 1, gains_[clip<int>(band_ - 1, 0, eq_bands - 1)]);
            break;
        case KeyEvent::Right:
            set(band_ + 1, gains_[clip<int>(band_ + 1, 0, eq_bands - 1)]);
            break;
        case KeyEvent::Up:
            set(band_, gains_[band_] + 1);
            break;
        case KeyEvent::Down:
            set(band_, gains_[band_] - 1);
            break;
        default:
            return false;
    }
    return true;
}

bool EqView::on_encoder(EncoderEvent delta) {
    set(band_, gains_[band_] + delta);
    return true;
}

// Sliders can be dragged. A drag stays on the slider it started on.
bool EqView::on_touch(TouchEvent event) {
    Point p;
    const auto kind = touch_.feed(event, p);
    if (kind == Touch::None) return true;

    if (kind == Touch::Down)
        band_ = clip<int>((p.x() - screen_pos().x()) / (screen_width / eq_bands), 0, eq_bands - 1);
    const int y = p.y() - screen_pos().y() - slider_top;
    set(band_, 2 * eq_max_db - (y + slider_pitch / 2) / slider_pitch);
    return true;
}

void EqView::paint(Painter& painter) {
    static const char* const names[eq_bands] = {"100", "300", "1K", "3K5", "10K"};
    const int x0 = screen_pos().x();
    const int y0 = screen_pos().y();
    const int height = size().height();
    const int label_y = y0 + height - UI_POS_HEIGHT(3) - 10;

    if (clear_) {
        painter.fill_rectangle(screen_rect(), Color::black());
        dots(painter, {x0 + UI_POS_X(1), y0 + height - UI_POS_HEIGHT(1) - 4}, "TURN: GAIN   </>: BAND   SELECT: BACK", 37, Color::light_grey(), 1);
    }
    clear_ = false;

    for (int b = 0; b < (int)eq_bands; b++) {
        // Everything in a column is centred on its middle.
        const int x = x0 + UI_POS_X_TABLE(eq_bands, b) + screen_width / eq_bands / 2;
        const int db = gains_[b] - eq_max_db;
        const bool selected = (b == band_);

        dots(painter, {x - 4 * glyph_width / 2, y0 + UI_POS_DEFAULT_HEIGHT_SMALL},
             (db > 0 ? "+" : db < 0 ? "-"
                                    : " ") +
                 to_string_dec_uint(std::abs(db)),
             4, selected ? Color::white() : Color::light_grey(), 1);

        // One dot per dB, +12 at the top. White from the 0 dB line to the red knob.
        for (int step = 0; step <= 2 * eq_max_db; step++) {
            const int y = y0 + slider_top + (2 * eq_max_db - step) * slider_pitch;
            const bool knob = (step == gains_[b]);
            const bool filled = (step > eq_max_db) ? (step < gains_[b]) : (step < eq_max_db && step > gains_[b]);
            painter.fill_rectangle({x - 16, y, 32, 5}, Color::black());
            if (knob)
                painter.fill_rectangle({x - 12, y, 24, 5}, Color::red());
            else
                painter.fill_rectangle({x - 2, y, 4, 4},
                                       filled ? Color::white() : step == eq_max_db ? Color::light_grey()
                                                                                   : Color::dark_grey());
        }

        const int label_width = 3 * glyph_width * 2;
        dots(painter, {x - label_width / 2, label_y}, names[b], 3, selected ? Color::white() : Color::grey(), 2);
        painter.fill_rectangle({x - label_width / 2, label_y + 20, label_width, 3}, selected ? Color::red() : Color::black());
    }
}

}  // namespace ui::external_app::music
