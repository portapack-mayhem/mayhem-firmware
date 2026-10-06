/*
 * Copyright (C) 2026 zxkmm
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

/* Swept spectrum analyzer, landscape.
 *
 * Radio side: see spec_an_shared.hpp for the M0/M4 protocol.
 *
 * Display side: the view draws itself straight to the LCD, no widgets.
 *  - Spectrum: a per-column compositor. Each column remembers the row span
 *    it last drew over the graticule; a redraw only covers the union of the
 *    old and new spans, so a stable trace costs almost nothing.
 *  - Waterfall: history lives in M4 RAM as palette indices and is blitted
 *    through a 256 entry LUT in a single LCD window.
 *  - Text: cached per line, redrawn only when it changes.
 *
 * Controls follow bench analyzers: a softkey row (Left/Right, Select),
 * an active function adjusted with the encoder (fine) or Up/Down (coarse,
 * one division), Back to the parent menu or out of the app. */

#ifndef __UI_SPEC_AN_H__
#define __UI_SPEC_AN_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_text.hpp"
#include "event_m0.hpp"

#include "app_settings.hpp"
#include "radio_state.hpp"
#include "message.hpp"
#include "spec_an_shared.hpp"

#include <array>
#include <cstdint>

namespace ui::external_app::spec_an {

class SpecAnView : public View {
   public:
    SpecAnView(NavigationView& nav);
    ~SpecAnView();

    SpecAnView(const SpecAnView&) = delete;
    SpecAnView(SpecAnView&&) = delete;
    SpecAnView& operator=(const SpecAnView&) = delete;
    SpecAnView& operator=(SpecAnView&&) = delete;

    void focus() override;
    void paint(Painter& painter) override;
    bool on_key(const KeyEvent key) override;
    bool on_encoder(const EncoderEvent delta) override;
    bool on_touch(const TouchEvent event) override;

    std::string title() const override { return "Spectrum"; };

    enum class Item : uint8_t {
        None,
        /* menus */
        MenuFreq,
        MenuSpan,
        MenuAmpt,
        MenuBw,
        MenuTrace,
        MenuMkr,
        MenuDisp,
        /* FREQ */
        Center,
        Start,
        Stop,
        Keypad,
        /* SPAN */
        Span,
        FullSpan,
        ZoomIn,
        ZoomOut,
        LastSpan,
        /* AMPT */
        RefLevel,
        DbDiv,
        Lna,
        Vga,
        Amp,
        RefOffset,
        AutoRef,
        /* BW */
        Rbw,
        Vbw,
        Window,
        Detector,
        Layout,
        Settle,
        /* TRACE */
        TraceSel,
        TraceMode,
        AvgCount,
        Fill,
        TraceClear,
        /* MKR */
        MkrSel,
        MkrMode,
        PeakSearch,
        NextPeak,
        MkrToCf,
        MkrToRef,
        MkrAllOff,
        /* DISP */
        Waterfall,
        DispLine,
        SweepMode,
        SweepRun,
        Flip,
    };

    enum class TraceMode : uint8_t {
        ClearWrite = 0,
        MaxHold,
        MinHold,
        Average,
        View,
        Blank,
        Count
    };

   private:
    static constexpr size_t kPoints = ::spec_an::kPoints;
    static constexpr size_t kTraces = 3;
    static constexpr size_t kMarkers = 4;
    static constexpr int kSpecRowsFull = 171;
    int kSpecRowsSplit = 91;
    int kSoftY = 220;
    int kActiveY = 201;

    struct Plan {
        uint32_t fs{20000000};
        uint8_t log2n{8};
        uint16_t avg{1};
        int16_t bin_first{8};
        uint16_t bins_per_slice{88};
        uint16_t slices{1};
        int64_t f_bin0{0}; /* frequency of global bin 0 */
        uint32_t filter_bw{15000000};
        uint32_t rbw_hz{0};
        uint8_t settle_bufs{2};
    };

    struct Marker {
        uint8_t mode{0}; /* 0 off, 1 normal, 2 delta to M1 */
        uint8_t trace{0};
        int32_t x{160}; /* int32 so it can be bound to a setting */
    };

    NavigationView& nav_;
    RxRadioState radio_state_{};

    /* Persisted settings: declared before settings_, which loads into them. */
    uint64_t center_{433'920'000};
    uint64_t span_{20'000'000};
    uint64_t last_span_{20'000'000};
    int32_t ref_cdb_{-4000};
    uint8_t dbdiv_idx_{3};
    uint8_t lna_{24};
    uint8_t vga_{20};
    bool amp_{false};
    int32_t ref_offset_cdb_{0};
    uint8_t rbw_idx_{0}; /* 0 auto, else N = 128 << idx */
    uint8_t vbw_idx_{0}; /* averages = 1 << idx */
    uint8_t window_{static_cast<uint8_t>(::spec_an::Window::BlackmanHarris)};
    uint8_t detector_{static_cast<uint8_t>(::spec_an::Detector::Peak)};
    uint8_t layout_{0};
    /* Synthesizer settling after each retune. HackRF One moves both PLLs on
     * every slice (its second LO tracks the RF, 2650 MHz - f/7) and smears
     * a CW across the slice below ~350 us. PRALINE keeps the MAX2831 on a
     * fixed IF across most bands and is clean even at 0. */
#ifdef PRALINE
    static constexpr uint32_t kSettleMinUs = 0;
    static constexpr uint32_t kSettleDefaultUs = 100;
#else
    static constexpr uint32_t kSettleMinUs = 350;
    static constexpr uint32_t kSettleDefaultUs = 400;
#endif
    uint32_t settle_us_{kSettleDefaultUs};
    bool settle_low() const { return settle_us_ < kSettleMinUs; }
    uint8_t avg_log2_{3};
    bool fill_{true};
    bool waterfall_{true};
    bool dline_on_{false};
    int32_t dline_cdb_{-6000};
    bool continuous_{true};
    bool flip_{false};
    uint8_t trace_mode_raw_[kTraces]{0, 5, 5};
    std::array<Marker, kMarkers> markers_{};
    uint8_t mkr_sel_{0};
    uint8_t trace_sel_{0};

    app_settings::SettingsManager settings_{
        "rx_specan",
        app_settings::Mode::NO_RF,
        {
            {"center"sv, &center_},
            {"span"sv, &span_},
            {"last_span"sv, &last_span_},
            {"ref_cdb"sv, &ref_cdb_},
            {"dbdiv"sv, &dbdiv_idx_},
            {"lna"sv, &lna_},
            {"vga"sv, &vga_},
            {"amp"sv, &amp_},
            {"ref_offset"sv, &ref_offset_cdb_},
            {"rbw"sv, &rbw_idx_},
            {"vbw"sv, &vbw_idx_},
            {"window"sv, &window_},
            {"detector"sv, &detector_},
            {"layout"sv, &layout_},
            {"settle_us2"sv, &settle_us_},
            {"avg_log2"sv, &avg_log2_},
            {"fill"sv, &fill_},
            {"waterfall"sv, &waterfall_},
            {"dline_on"sv, &dline_on_},
            {"dline_cdb"sv, &dline_cdb_},
            {"continuous"sv, &continuous_},
            {"flip"sv, &flip_},
            {"t1"sv, &trace_mode_raw_[0]},
            {"t2"sv, &trace_mode_raw_[1]},
            {"t3"sv, &trace_mode_raw_[2]},
            {"trace_sel"sv, &trace_sel_},
            {"mkr_sel"sv, &mkr_sel_},
            {"m1_mode"sv, &markers_[0].mode},
            {"m1_trace"sv, &markers_[0].trace},
            {"m1_x"sv, &markers_[0].x},
            {"m2_mode"sv, &markers_[1].mode},
            {"m2_trace"sv, &markers_[1].trace},
            {"m2_x"sv, &markers_[1].x},
            {"m3_mode"sv, &markers_[2].mode},
            {"m3_trace"sv, &markers_[2].trace},
            {"m3_x"sv, &markers_[2].x},
            {"m4_mode"sv, &markers_[3].mode},
            {"m4_trace"sv, &markers_[3].trace},
            {"m4_x"sv, &markers_[3].x},
        }};

    /* Sweep engine. */
    ::spec_an::Shared shared_{};
    Plan plan_{};
    uint32_t cur_fs_{0};
    uint32_t cur_filter_{0};
    uint16_t gen_{0};
    uint32_t seq_{0};
    uint8_t sweep_{0};
    uint16_t slice_{0};
    bool sweeping_{false};
    systime_t issued_at_{0};
    uint32_t last_reply_{UINT32_MAX}; /* (sweep << 16) | slice of the last SpecAnSlice taken */

    /* Sweep timing, averaged over a short window. */
    systime_t rate_t0_{0};
    uint32_t rate_count_{0};
    uint32_t sweep_us_{0};

    /* Traces, in centi-dB (gain compensated). */
    std::array<int16_t, kPoints> live_{};
    std::array<std::array<int16_t, kPoints>, kTraces> trace_{};
    std::array<uint16_t, kTraces> avg_sweeps_{};
    std::array<int16_t, kPoints> wf_acc_{};

    /* Display state. */
    bool landscape_{false};
    bool full_redraw_{true};
    int spec_rows_{kSpecRowsSplit};
    int wf_top_{0};
    int wf_rows_{0};
    int32_t ymul_{0}; /* rows per cdB, Q16 */
    int32_t wf_mul_{0};
    uint8_t div_rows_{9};
    std::array<uint8_t, kPoints> drawn_top_{};
    std::array<uint8_t, kPoints> drawn_bot_{};
    std::array<uint32_t, kPoints / 32> dirty_{};
    uint16_t wf_head_{0};
    uint16_t wf_count_{0};
    bool wf_dirty_{false};
    systime_t wf_last_{0};

    /* Softkeys / active function. */
    const Item* menu_{nullptr};
    uint8_t menu_len_{0};
    uint8_t cursor_{1};
    Item active_{Item::Center};
    bool keys_dirty_{true};

    /* What the current touch went down on, acted on at End. */
    enum class TouchTarget : uint8_t { None,
                                       Softkey,
                                       Keypad };
    TouchTarget touch_target_{TouchTarget::None};

    std::array<std::array<char, 80>, 4> text_cache_{};

    /* Render scratch: members, not stack (4 KiB) nor static (permanent). */
    std::array<Color, kSpecRowsFull> column_buf_{};
    std::array<bool, kSpecRowsFull> hgrid_row_{};
    std::array<Color, 256> palette_{};

    /* Sweep engine. */
    void compute_plan();
    void apply_plan(bool reset_traces);
    void restart_sweep();
    void issue(uint16_t slice, bool retune);
    int64_t slice_lo(uint16_t slice) const;
    void on_captured(const SpecAnCapturedMessage& message);
    void on_slice(const SpecAnSliceMessage& message);
    void on_sweep_done();
    void ingest(const int16_t* src, uint16_t px_begin, uint16_t px_end);
    void clear_traces();
    void apply_gains();
    int32_t level_offset() const;

    /* Display. */
    void enter_landscape();
    void leave_landscape();
    void update_geometry();
    void on_frame_sync();
    void redraw_all();
    void mark_dirty(int x0, int x1);
    void mark_all_dirty();
    void render_columns();
    void render_column(int x);
    void render_waterfall();
    void push_waterfall_line();
    void render_text();
    void render_softkeys();
    void draw_text_line(size_t slot, Point p, const Font& font, Color fg, Color bg, const char* text, size_t width);
    int y_of(int32_t cdb) const;

    /* Controls. */
    void set_menu(const Item* menu, uint8_t len, uint8_t cursor);
    void go_back();
    void activate(Item item);
    void adjust(Item item, int32_t steps, bool coarse);
    void open_keypad(Item item);
    const char* item_label(Item item) const;
    bool item_lit(Item item) const;
    KeyEvent remap_key(KeyEvent key) const;
    Point remap_touch(Point p) const;

    /* Frequency helpers. */
    uint64_t start_freq() const;
    uint64_t stop_freq() const;
    uint64_t point_freq(int x) const;
    void set_center_span(uint64_t center, uint64_t span);
    void set_start_stop(uint64_t start, uint64_t stop);
    uint64_t fine_step() const;

    /* Markers. */
    int32_t marker_level(const Marker& m) const;
    void peak_search();
    void next_peak();
    void mark_marker_dirty(const Marker& m);

    MessageHandlerRegistration message_handler_captured{
        Message::ID::SpecAnCaptured,
        [this](const Message* const p) {
            this->on_captured(*reinterpret_cast<const SpecAnCapturedMessage*>(p));
        }};

    MessageHandlerRegistration message_handler_slice{
        Message::ID::SpecAnSlice,
        [this](const Message* const p) {
            this->on_slice(*reinterpret_cast<const SpecAnSliceMessage*>(p));
        }};

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->on_frame_sync();
        }};
};

} /* namespace ui::external_app::spec_an */

#endif /*__UI_SPEC_AN_H__*/
