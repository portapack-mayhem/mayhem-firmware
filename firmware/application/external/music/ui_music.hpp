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

#ifndef __UI_MUSIC_H__
#define __UI_MUSIC_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_widget.hpp"
#include "io_wave.hpp"
#include "replay_thread.hpp"
#include "baseband_api.hpp"
#include "audio.hpp"
#include "app_settings.hpp"
#include "file.hpp"
#include "io.hpp"

#include <climits>
#include <cstdlib>
#include <memory>

namespace ui::external_app::music {

/* The "record": shared between the UI and the reader thread. Each field has one writer. */
struct Deck {
    volatile uint32_t head{0};      // frame being played (reader)
    volatile int32_t turned{0};     // total frames the encoder asked the record to move (UI)
    volatile bool paused{false};    // (UI)
    volatile uint32_t seek_to{0};   // frame to jump to... (UI)
    volatile uint32_t seek_seq{0};  // ...requested by bumping this (UI)
};

/* Feeds the replay stream from a WAV file, as 16 bit samples at the output rate. Normally
 * it just plays on (resampling files of another rate); when the encoder is
 * turned it plays the file at the speed and in the direction of the turning instead. */
class DeckReader : public stream::Reader {
   public:
    DeckReader(std::unique_ptr<WAVFileReader> wav, Deck& deck, size_t max_read);

    File::Result<File::Size> read(void* const buffer, const File::Size bytes) override;

   private:
    std::unique_ptr<WAVFileReader> wav_;
    Deck& deck_;
    const uint32_t channels_;
    const uint32_t total_;        // frames
    const size_t max_read_;       // largest read() the stream will ask for, bytes
    const int32_t normal_step_;   // file frames per output frame at normal speed, Q16
    const int32_t chase_frames_;  // the scratch constants, in frames of this file
    const int32_t settle_frames_;
    const int32_t jump_frames_;
    uint32_t head_;                        // frame the next output starts at...
    uint32_t fraction_{0};                 // ...and how far past it, Q16
    int32_t done_;                         // part of deck_.turned already played
    uint32_t seek_seen_;                   // last deck_.seek_seq acted on
    uint32_t drained_{0};                  // silence fed after the end of the track, bytes
    uint32_t next_frame_{UINT32_MAX};      // where the file is positioned, to skip needless seeks
    std::unique_ptr<int16_t[]> window_{};  // source audio for one resampled block

    File::Result<File::Size> fetch(uint32_t frame, uint32_t count, int16_t* out);
};

/* Workaround: the touch panel reports wrong positions while a finger lifts. The End point
 * is ignored, and a move only counts once the next one confirms it. */
class Touch {
   public:
    enum Kind { None,
                Down,
                Drag };

    Kind feed(const TouchEvent& event, Point& point) {
        switch (event.type) {
            case TouchEvent::Type::Start:
                // The dispatcher delivers Start twice: once to find the widget, once to it.
                if (down_) return None;
                down_ = true;
                active_ = (chTimeNow() - ended_) > MS2ST(bounce_ms);
                has_pending_ = false;
                last_ = event.point;
                point = event.point;
                return active_ ? Down : None;

            case TouchEvent::Type::Move: {
                if (!active_) return None;
                const int jump = std::abs(event.point.x() - last_.x()) + std::abs(event.point.y() - last_.y());
                last_ = event.point;
                if (jump > max_jump) {
                    has_pending_ = false;
                    return None;
                }
                const bool confirmed = has_pending_;
                point = pending_;
                pending_ = event.point;
                has_pending_ = true;
                return confirmed ? Drag : None;
            }

            default:
                active_ = false;
                down_ = false;
                ended_ = chTimeNow();
                return None;
        }
    }

   private:
    static constexpr int max_jump = 40;  // px between two reports; more is not a finger
    static constexpr uint32_t bounce_ms = 200;

    Point last_{};
    Point pending_{};
    systime_t ended_{0};
    bool active_{false};
    bool has_pending_{false};
    bool down_{false};
};

constexpr size_t eq_bands = AudioPlayConfigMessage::eq_bands;
constexpr uint8_t eq_max_db = 12;
constexpr size_t stream_buffers = 3;  // blocks queued between the reader and the baseband

// font::fixed_5x8, which all the dot matrix text is drawn from.
constexpr int glyph_width = 5;
constexpr int glyph_height = 8;

/* Five sliders for the audio_play equalizer. Edits the gains in place (0..24, 12 = 0 dB). */
class EqView : public View {
   public:
    EqView(NavigationView& nav, uint8_t* gains, std::function<void()> on_change);

    EqView(const EqView&) = delete;
    EqView& operator=(const EqView&) = delete;

    std::string title() const override { return "Equalizer"; }
    void focus() override { Widget::focus(); }
    void paint(Painter& painter) override;
    void on_show() override { clear_ = true; }

    bool on_key(KeyEvent key) override;
    bool on_encoder(EncoderEvent delta) override;
    bool on_touch(TouchEvent event) override;

   private:
    static constexpr int slider_top = UI_POS_Y(2) - 4;
    static constexpr int slider_pitch = 8;

    NavigationView& nav_;
    uint8_t* gains_;
    std::function<void()> on_change_;
    int band_{0};
    Touch touch_{};
    bool clear_{true};

    void set(int band, int value);
};

/* WAV player. A playlist is simply a folder of WAV files below /MUSIC. */
class MusicView : public View {
   public:
    MusicView(NavigationView& nav);
    ~MusicView();

    MusicView(const MusicView&) = delete;
    MusicView& operator=(const MusicView&) = delete;

    std::string title() const override { return "Music"; }
    void focus() override { Widget::focus(); }
    void paint(Painter& painter) override;
    void on_show() override { clear_ = true; }

    bool on_key(KeyEvent key) override;
    bool on_encoder(EncoderEvent delta) override;
    bool on_touch(TouchEvent event) override;

   private:
    enum Cursor : uint8_t { Transport,
                            List,
                            Shuffle,
                            Repeat,
                            Eq,
                            Volume };
    enum Paint : uint8_t { Tick = 1,  // dial and time only
                           Redraw };  // everything

    // NB: the uint8_t order table caps a playlist at 255 tracks; widen it if a folder ever holds more.
    static constexpr size_t max_tracks = 255;
    // Encoder feel, to be tuned on the device.
    static constexpr uint32_t detent_ms = 80;    // scratching: one detent moves the record this far
    static constexpr uint32_t seek_ms = 60;      // seeking: base step, doubled as the spin goes on
    static constexpr uint32_t spin_gap_ms = 40;  // detents closer than this count as spinning
    static constexpr int32_t spin_start = 12;    // spinning detents before a turn becomes a seek

    NavigationView& nav_;

    std::filesystem::path dir_{};   // the playlist
    std::filesystem::path file_{};  // the current track
    uint8_t order_[max_tracks]{};   // play order -> index of file in dir_
    uint16_t count_{0};
    uint16_t slot_{0};  // position in order_

    uint32_t rate_{0};
    uint32_t total_{0};  // frames
    uint32_t pos_{0};    // current frame
    Deck deck_{};
    systime_t last_turn_{0};
    uint8_t spin_{0};  // consecutive fast detents in one direction
    bool spin_forward_{true};
    bool playing_{false};
    bool shuffle_{false};
    uint8_t repeat_{0};                                                            // 0 off, 1 all, 2 one
    uint8_t eq_[eq_bands]{eq_max_db, eq_max_db, eq_max_db, eq_max_db, eq_max_db};  // 0..24, 12 = 0 dB
    uint8_t channels_{1};

    SettingsStore settings_{
        "music",
        {{"shuffle", &shuffle_},
         {"repeat", &repeat_},
         {"eq_100", &eq_[0]},
         {"eq_300", &eq_[1]},
         {"eq_1k", &eq_[2]},
         {"eq_3k5", &eq_[3]},
         {"eq_10k", &eq_[4]}}};

    uint8_t volume_{0};  // 0-99
    uint8_t cursor_{Transport};
    uint8_t paint_{Redraw};
    uint32_t painted_dot_{0};  // what the dial and the clock currently show
    uint32_t painted_seconds_{0};
    Touch touch_{};
    bool ring_drag_{false};  // the current touch started on the ring
    uint8_t retries_{0};     // read errors on the current track
    uint8_t generation_{0};  // of the replay thread, to tell its messages from an older one's
    bool clear_{true};       // view was just (re)shown: wipe before painting
    const char* error_{nullptr};

    std::unique_ptr<ReplayThread> replay_thread{};
    bool ready_signal{false};

    void open_picker();
    void open_playlist(const std::filesystem::path& file);
    void set_shuffle(bool on);
    void load(uint16_t slot);
    void play_from(uint32_t sample);
    void stop();
    void step(int dir, bool automatic);
    void turn(int32_t delta);
    void animate();

    static constexpr uint32_t ring_dots = 48;
    static constexpr int row_items = 5;  // LIST, SHUF, RPT, EQ, VOL

    struct Layout {
        int cx, cy, radius;  // dial
        int title_y;
        int row_y;       // bottom row
        int cell;        // width of a bottom row item
        size_t columns;  // zoom 2 dot characters in a line of text
    };
    Layout layout() const;
    static Point ring_dot(uint32_t i, uint32_t radius) { return fast_polar_to_point(i * 360 / ring_dots, radius); }
    uint32_t head_dot() const { return total_ ? (uint64_t)pos_ * ring_dots / total_ : 0; }
    void set_volume(int32_t v);
    void apply_eq();
    void activate();
    void redraw(uint8_t what = Redraw);

    MessageHandlerRegistration message_handler_fifo_signal{
        Message::ID::RequestSignal,
        [this](const Message* const p) {
            const auto message = static_cast<const RequestSignalMessage*>(p);
            if (message->signal == RequestSignalMessage::Signal::FillRequest)
                ready_signal = true;
        }};

    MessageHandlerRegistration message_handler_replay_thread_done{
        Message::ID::ReplayThreadDone,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const ReplayThreadDoneMessage*>(p);
            if ((message.return_code >> 8) != generation_) return;  // from a thread already replaced
            const uint32_t return_code = message.return_code & 0xFF;
            if (return_code == ReplayThread::END_OF_FILE)
                step(1, true);
            else if (return_code == ReplayThread::READ_ERROR) {
                // Seen when leaving the file picker, cause unknown: pick the track up
                // again where it was instead of giving up on the first failed read.
                const bool was_playing = playing_;
                if (retries_++ < 3) {
                    play_from(deck_.head);
                    if (!was_playing && replay_thread) {
                        playing_ = false;
                        deck_.paused = true;
                    }
                } else {
                    stop();
                    error_ = "READ ERROR";
                }
                redraw();
            }
        }};

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            animate();
        }};
};

}  // namespace ui::external_app::music

#endif /*__UI_MUSIC_H__*/
