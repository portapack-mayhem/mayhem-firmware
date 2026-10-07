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

#include "ui_spec_an.hpp"

#include "baseband_api.hpp"
#include "event_m0.hpp"
#include "portapack.hpp"
#include "radio.hpp"
#include "receiver_model.hpp"
#include "ui_receiver.hpp"
#include "ui_font_fixed_5x8.hpp"
#include "ui_font_fixed_8x16.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>

using namespace portapack;
using namespace ::spec_an;

extern ui::SystemView* system_view_ptr;

namespace ui::external_app::spec_an {

namespace {

/* Landscape layout, 320 x 240. */
constexpr int kLine1Y = 0;
constexpr int kLine2Y = 9;
constexpr int kGridTop = 18;
constexpr int kAnnoY = 191;
constexpr int kSoftH = 20;
constexpr int kBackW = 20;
constexpr int kKeyW = 42;
constexpr int kSmallChars = 64;
constexpr int kBigChars = 40;

constexpr uint64_t kMaxFreq = 7'250'000'000ULL;
constexpr uint64_t kMinSpan = 10'000;
constexpr uint64_t kFullStart = 1'000'000;
constexpr uint64_t kFullStop = 6'000'000'000ULL;

/* Ascending. Narrow spans take the lowest rate that covers them in one tune:
 * finer bins for the same FFT size and a narrower anti-alias filter. */
constexpr uint32_t kSampleRates[] = {2'500'000, 5'000'000, 10'000'000, 20'000'000};
constexpr int32_t kDbDiv[] = {100, 200, 500, 1000, 2000};
constexpr size_t kDbDivCount = sizeof(kDbDiv) / sizeof(kDbDiv[0]);
constexpr uint8_t kMaxVbwIdx = 5;
constexpr uint8_t kMaxRbwIdx = 4;

constexpr Color c_bg{0, 0, 0};
constexpr Color c_border{110, 110, 110};
constexpr Color c_grid{55, 55, 55};
constexpr Color c_fill{44, 38, 0};
constexpr Color c_fill_grid{80, 72, 24};
constexpr Color c_dline{0, 150, 255};
constexpr Color c_text{230, 230, 230};
constexpr Color c_label{0, 200, 220};
constexpr Color c_dim{150, 150, 150};
constexpr Color c_key_bg{30, 30, 40};
constexpr Color c_key_active{0, 50, 110};
constexpr Color c_key_cursor{255, 210, 0};
constexpr Color c_key_lit{80, 255, 80};
constexpr Color c_warn{255, 140, 0};

constexpr Color c_trace[3] = {{255, 230, 0}, {0, 210, 255}, {255, 90, 210}};
constexpr Color c_marker[4] = {{255, 70, 70}, {70, 255, 70}, {90, 140, 255}, {255, 160, 0}};

using Item = SpecAnView::Item;
using TraceMode = SpecAnView::TraceMode;

constexpr Item menu_root[] = {Item::MenuFreq, Item::MenuSpan, Item::MenuAmpt, Item::MenuBw, Item::MenuTrace, Item::MenuMkr, Item::MenuDisp};
constexpr Item menu_freq[] = {Item::Center, Item::Start, Item::Stop, Item::Keypad};
constexpr Item menu_span[] = {Item::Span, Item::FullSpan, Item::ZoomIn, Item::ZoomOut, Item::LastSpan};
constexpr Item menu_ampt[] = {Item::RefLevel, Item::DbDiv, Item::Lna, Item::Vga, Item::Amp, Item::RefOffset, Item::AutoRef};
constexpr Item menu_bw[] = {Item::Rbw, Item::Vbw, Item::Window, Item::Detector, Item::Layout, Item::Settle};
constexpr Item menu_trace[] = {Item::TraceSel, Item::TraceMode, Item::AvgCount, Item::Fill, Item::TraceClear};
constexpr Item menu_mkr[] = {Item::MkrSel, Item::MkrMode, Item::PeakSearch, Item::NextPeak, Item::MkrToCf, Item::MkrToRef, Item::MkrAllOff};
constexpr Item menu_disp[] = {Item::Waterfall, Item::DispLine, Item::SweepMode, Item::SweepRun, Item::Flip};

template <size_t N>
constexpr uint8_t count_of(const Item (&)[N]) {
    return N;
}

const char* const window_names[] = {"Rectangular", "Hann", "Blackman-Harris", "Flat top"};
const char* const detector_names[] = {"Peak", "Average", "Sample", "Neg peak"};
const char* const layout_names[] = {"Upper sideband", "Both sidebands"};
const char* const trace_mode_names[] = {"Clear/Write", "Max hold", "Min hold", "Average", "View", "Blank"};
const char* const trace_mode_short[] = {"ClrW", "MaxH", "MinH", "Avg", "View", "Blnk"};

/* Waterfall palette: black, navy, blue, cyan, yellow, red, white. */
void build_palette(std::array<Color, 256>& palette) {
    struct Stop {
        int i;
        int r, g, b;
    };
    static constexpr Stop stops[] = {
        {0, 0, 0, 0},
        {48, 0, 0, 100},
        {96, 0, 70, 210},
        {144, 0, 200, 200},
        {192, 240, 220, 0},
        {232, 255, 60, 0},
        {255, 255, 255, 255},
    };
    for (size_t s = 0; s + 1 < sizeof(stops) / sizeof(stops[0]); s++) {
        const Stop& a = stops[s];
        const Stop& b = stops[s + 1];
        const int w = b.i - a.i;
        for (int i = a.i; i <= b.i; i++) {
            const int t = i - a.i;
            palette[i] = Color(
                a.r + (b.r - a.r) * t / w,
                a.g + (b.g - a.g) * t / w,
                a.b + (b.b - a.b) * t / w);
        }
    }
}

/* Small fixed buffer formatter: no heap, no printf. */
class TextBuf {
   public:
    TextBuf& s(const char* str) {
        while (*str && n_ < sizeof(buf_) - 1) buf_[n_++] = *str++;
        buf_[n_] = 0;
        return *this;
    }

    TextBuf& c(char ch) {
        if (n_ < sizeof(buf_) - 1) buf_[n_++] = ch;
        buf_[n_] = 0;
        return *this;
    }

    TextBuf& u(uint64_t v, int min_digits = 1) {
        char tmp[21];
        int i = 0;
        do {
            tmp[i++] = '0' + (v % 10);
            v /= 10;
        } while (v && i < 20);
        while (i < min_digits && i < 20) tmp[i++] = '0';
        while (i) c(tmp[--i]);
        return *this;
    }

    TextBuf& i(int64_t v, bool plus = false) {
        if (v < 0) {
            c('-');
            v = -v;
        } else if (plus) {
            c('+');
        }
        return u(static_cast<uint64_t>(v));
    }

    /* Centi-dB with one or two decimals. */
    TextBuf& cdb(int32_t v, int decimals = 1, bool plus = false) {
        if (v < 0) {
            c('-');
            v = -v;
        } else if (plus) {
            c('+');
        }
        u(v / 100);
        c('.');
        if (decimals >= 2)
            u(v % 100, 2);
        else
            u((v % 100) / 10);
        return *this;
    }

    /* Fixed point with `decimals` digits out of `scale_digits`, trailing
     * zeros trimmed down to `min_decimals`. */
    TextBuf& fixed(uint64_t v, int scale_digits, int min_decimals) {
        uint64_t scale = 1;
        for (int k = 0; k < scale_digits; k++) scale *= 10;
        u(v / scale);
        if (scale_digits == 0) return *this;
        char frac[20];
        uint64_t f = v % scale;
        for (int k = scale_digits - 1; k >= 0; k--) {
            frac[k] = '0' + (f % 10);
            f /= 10;
        }
        int keep = scale_digits;
        while (keep > min_decimals && frac[keep - 1] == '0') keep--;
        if (keep) {
            c('.');
            for (int k = 0; k < keep; k++) c(frac[k]);
        }
        return *this;
    }

    /* MHz with every digit down to the Hz, grouped "433.920 000". */
    TextBuf& mhz_full(uint64_t hz) {
        u(hz / 1'000'000);
        c('.');
        const uint32_t f = hz % 1'000'000;
        u(f / 1000, 3);
        c(' ');
        u(f % 1000, 3);
        return s(" MHz");
    }

    TextBuf& freq(uint64_t hz) {
        if (hz >= 1'000'000'000ULL) return fixed(hz, 9, 3).s(" GHz");
        if (hz >= 1'000'000) return fixed(hz, 6, 3).s(" MHz");
        if (hz >= 1'000) return fixed(hz, 3, 1).s(" kHz");
        return u(hz).s(" Hz");
    }

    TextBuf& freq_compact(uint64_t hz) {
        if (hz >= 1'000'000'000ULL) return fixed(hz, 9, 3).s("G");
        if (hz >= 1'000'000) return fixed(hz, 6, 2).s("M");
        if (hz >= 1'000) return fixed(hz, 3, 1).s("k");
        return u(hz);
    }

    TextBuf& pad(size_t width) {
        while (n_ < width && n_ < sizeof(buf_) - 1) c(' ');
        return *this;
    }

    size_t size() const { return n_; }
    const char* str() const { return buf_; }

   private:
    char buf_[80]{};
    size_t n_{0};
};

/* Largest 1/2/5 x 10^k not above v. */
uint64_t nice_floor(uint64_t v) {
    if (v < 1) return 1;
    uint64_t p = 1;
    while (p * 10 <= v) p *= 10;
    if (p * 5 <= v) return p * 5;
    if (p * 2 <= v) return p * 2;
    return p;
}

/* Next 1/2/5 step above (dir > 0) or below v. */
uint64_t step_125(uint64_t v, int dir) {
    if (dir > 0) {
        const uint64_t f = nice_floor(v);
        uint64_t p = 1;
        while (p * 10 <= f) p *= 10;
        const uint64_t m = f / p;
        return (m == 1) ? 2 * p : (m == 2) ? 5 * p
                                           : 10 * p;
    }
    const uint64_t f = nice_floor(v);
    if (f < v) return f;
    uint64_t p = 1;
    while (p * 10 <= f) p *= 10;
    const uint64_t m = f / p;
    return (m == 5) ? 2 * p : (m == 2) ? p
                                       : p / 2;
}

int floor_log2(uint32_t v) {
    int r = 0;
    while (v >>= 1) r++;
    return r;
}

inline void compiler_barrier() {
    __asm__ volatile("" ::: "memory");
}

}  // namespace

/* Lifecycle ***************************************************************/

SpecAnView::SpecAnView(NavigationView& nav)
    : nav_{nav} {
    set_focusable(true);

    enter_landscape();
    kSoftY = screen_height - kSoftH;
    kActiveY = screen_height - kSoftH - 16 - 3;  // font - paddings
    kSpecRowsSplit = (screen_height > 300) ? 171 : 91;
    /* Persisted values may be from another build: keep them in range. */
    span_ = std::clamp<uint64_t>(span_, kMinSpan, kMaxFreq);
    center_ = std::clamp<uint64_t>(center_, span_ / 2, kMaxFreq - span_ / 2);
    last_span_ = std::clamp<uint64_t>(last_span_, kMinSpan, kMaxFreq);
    ref_cdb_ = std::clamp<int32_t>(ref_cdb_, -15000, 3000);
    dbdiv_idx_ = std::min<uint8_t>(dbdiv_idx_, kDbDivCount - 1);
    lna_ = std::min<uint8_t>(lna_ & ~7, 40);
    vga_ = std::min<uint8_t>(vga_ & ~1, 62);
    ref_offset_cdb_ = std::clamp<int32_t>(ref_offset_cdb_, -10000, 10000);
    rbw_idx_ = std::min(rbw_idx_, kMaxRbwIdx);
    vbw_idx_ = std::min(vbw_idx_, kMaxVbwIdx);
    window_ = std::min<uint8_t>(window_, static_cast<uint8_t>(Window::Count) - 1);
    detector_ = std::min<uint8_t>(detector_, static_cast<uint8_t>(Detector::Count) - 1);
    if (!settings_.loaded()) {
#ifdef PRALINE
        layout_ = static_cast<uint8_t>(BinLayout::Both);
#else
        layout_ = static_cast<uint8_t>(BinLayout::Upper);
#endif
    }
    layout_ = std::min<uint8_t>(layout_, static_cast<uint8_t>(BinLayout::Count) - 1);
    settle_us_ = std::min<uint32_t>(settle_us_, 5000);
    avg_log2_ = std::clamp<uint8_t>(avg_log2_, 1, 7);
    dline_cdb_ = std::clamp<int32_t>(dline_cdb_, -20000, 3000);
    for (auto& m : trace_mode_raw_) m = std::min<uint8_t>(m, static_cast<uint8_t>(TraceMode::Count) - 1);
    trace_sel_ = std::min<uint8_t>(trace_sel_, kTraces - 1);
    mkr_sel_ = std::min<uint8_t>(mkr_sel_, kMarkers - 1);
    for (size_t i = 0; i < kMarkers; i++) {
        Marker& m = markers_[i];
        m.mode = std::min<uint8_t>(m.mode, i == 0 ? 1 : 2); /* M1 has no delta mode */
        m.trace = std::min<uint8_t>(m.trace, kTraces - 1);
        m.x = std::clamp<int32_t>(m.x, 0, kPoints - 1);
    }

    build_palette(palette_);
    live_.fill(kNoData);
    wf_acc_.fill(kNoData);
    for (auto& t : trace_) t.fill(kNoData);
    drawn_top_.fill(255);
    drawn_bot_.fill(0);

    baseband::run_image(portapack::spi_flash::image_tag_spec_an);
    radio::set_fast_retune(true);

    receiver_model.set_sampling_rate(kSampleRates[3]);
    receiver_model.set_baseband_bandwidth(15'000'000);
    receiver_model.set_squelch_level(0);
    receiver_model.enable();
    cur_fs_ = kSampleRates[3];
    cur_filter_ = 15'000'000;
    apply_gains();

    set_menu(menu_root, count_of(menu_root), 1);
    system_view_ptr->set_app_fullscreen(true);
    update_geometry();
    apply_plan(true);
}

SpecAnView::~SpecAnView() {
    radio::set_fast_retune(false);
    receiver_model.set_sampling_rate(3'072'000);  // other apps expect this on entry
    receiver_model.disable();
    baseband::shutdown();

    leave_landscape();
    system_view_ptr->set_app_fullscreen(false);
    system_view_ptr->set_dirty();
}

void SpecAnView::focus() {
    View::focus();
}

void SpecAnView::enter_landscape() {
    if (landscape_) return;
    display.set_landscape(true, flip_);
    landscape_ = true;
    update_geometry();
}

void SpecAnView::leave_landscape() {
    if (!landscape_) return;
    landscape_ = false;
    if (display.is_landscape()) display.set_landscape(false);
    display.fill_rectangle(display.screen_rect(), c_bg);
}

void SpecAnView::paint(Painter&) {
    /* Called on first show and when coming back from the keypad. */
    enter_landscape();
    redraw_all();
}

/* Sweep plan **************************************************************/

void SpecAnView::compute_plan() {
    const auto layout = static_cast<BinLayout>(layout_);
    const uint64_t span = span_;
    /* Display points sit at start + x * span / (P - 1); each one owns the
     * frequencies half a point either side of it. */
    const uint64_t coverage = span + span / (kPoints - 1);

    /* One tune covers layout_count(N) / N of fs, whatever N is. */
    uint32_t fs = kSampleRates[3];
    for (const uint32_t rate : kSampleRates) {
        const uint64_t capacity = static_cast<uint64_t>(rate) * layout_count(layout, 256) / 256;
        if (capacity >= coverage + coverage / 32) {
            fs = rate;
            break;
        }
    }

    uint8_t log2n = kMaxFftLog2;
    if (rbw_idx_ == 0) {
        /* Auto RBW: the coarsest FFT whose bins are no wider than a point. */
        for (uint8_t l = kMinFftLog2; l <= kMaxFftLog2; l++) {
            if (static_cast<uint64_t>(fs) * (kPoints - 1) <= (span << l)) {
                log2n = l;
                break;
            }
        }
    } else {
        log2n = std::clamp<uint8_t>(7 + rbw_idx_, kMinFftLog2, kMaxFftLog2);
    }

    const uint32_t n = 1u << log2n;
    const uint32_t k_max = layout_count(layout, n);
    /* +3: interpolation needs a bin either side of the outermost points. */
    const uint64_t bins_needed = (coverage * n + fs - 1) / fs + 3;
    const uint32_t slices = static_cast<uint32_t>((bins_needed + k_max - 1) / k_max);
    const uint32_t k = static_cast<uint32_t>((bins_needed + slices - 1) / slices);

    plan_.fs = fs;
    plan_.log2n = log2n;
    plan_.avg = 1u << vbw_idx_;
    plan_.bin_first = static_cast<int16_t>(layout_first(layout, n));
    plan_.bins_per_slice = static_cast<uint16_t>(k);
    plan_.slices = static_cast<uint16_t>(std::min<uint32_t>(slices, 65535));
    plan_.f_bin0 = static_cast<int64_t>(start_freq()) - static_cast<int64_t>(span / (2 * (kPoints - 1)));
    plan_.filter_bw = (fs / 4) * 3;
    plan_.rbw_hz = static_cast<uint32_t>(static_cast<uint64_t>(fs) * window_enbw_centibins(static_cast<Window>(window_)) / (100ULL * n));
    /* One DMA buffer straddles the retune, then whole buffers for settling. */
    plan_.settle_bufs = static_cast<uint8_t>(1 + (static_cast<uint64_t>(settle_us_) * fs + 2048ULL * 1'000'000 - 1) / (2048ULL * 1'000'000));
}

void SpecAnView::apply_plan(bool reset_traces) {
    compute_plan();

    if (plan_.fs != cur_fs_) {
        receiver_model.set_sampling_rate(plan_.fs);
        cur_fs_ = plan_.fs;
    }
    if (plan_.filter_bw != cur_filter_) {
        receiver_model.set_baseband_bandwidth(plan_.filter_bw);
        cur_filter_ = plan_.filter_bw;
    }

    gen_++;
    const SpecAnConfigMessage message{
        &shared_,
        plan_.fs,
        gen_,
        plan_.log2n,
        plan_.avg,
        plan_.bin_first,
        plan_.bins_per_slice,
        plan_.slices,
        static_cast<uint64_t>(plan_.fs) * (kPoints - 1),
        span_ << plan_.log2n,
        static_cast<Window>(window_),
        static_cast<Detector>(detector_)};
    baseband::set_spec_an_config(message);

    if (reset_traces) clear_traces();
    restart_sweep();
}

int64_t SpecAnView::slice_lo(uint16_t slice) const {
    const int64_t bins = static_cast<int64_t>(slice) * plan_.bins_per_slice - plan_.bin_first;
    const int64_t lo = plan_.f_bin0 + ((bins * plan_.fs) >> plan_.log2n);
    return std::clamp<int64_t>(lo, 0, kMaxFreq);
}

void SpecAnView::restart_sweep() {
    sweep_++;
    slice_ = 0;
    sweeping_ = true;
    issue(0, true);
}

void SpecAnView::issue(uint16_t slice, bool retune) {
    if (retune) radio::set_tuning_frequency(slice_lo(slice));
    shared_.req_slice = slice;
    shared_.req_sweep = sweep_;
    shared_.req_settle = retune ? plan_.settle_bufs : 0;
    compiler_barrier();
    shared_.req_seq = ++seq_;
    issued_at_ = chTimeNow();
}

void SpecAnView::on_captured(const SpecAnCapturedMessage& message) {
    if (message.gen != gen_ || message.seq != seq_ || !sweeping_) return;

    /* The M4 has its samples: retune now, it FFTs while we do. */
    if (slice_ + 1u < plan_.slices) {
        slice_++;
        issue(slice_, true);
    } else if (continuous_) {
        sweep_++;
        slice_ = 0;
        issue(0, plan_.slices > 1);
    } else {
        sweeping_ = false;
    }
}

void SpecAnView::on_slice(const SpecAnSliceMessage& message) {
    if (message.gen != gen_) return;

    /* A timed out request that the M4 had in fact captured gets answered
     * twice; the M4 replays the same points, so drop the repeat. */
    const uint32_t id = (static_cast<uint32_t>(message.sweep) << 16) | message.slice;
    if (id == last_reply_) return;
    last_reply_ = id;

    if (message.px_end > message.px_begin && message.px_end <= kPoints) {
        ingest(shared_.pix[message.sweep & 1], message.px_begin, message.px_end);
    }
    if (message.last) on_sweep_done();
}

int32_t SpecAnView::level_offset() const {
    /* dBFS -> input referred: undo the receive gain. Absolute calibration is
     * left to the user's reference offset. */
    return ref_offset_cdb_ - 100 * (static_cast<int32_t>(lna_) + vga_ + (amp_ ? 14 : 0));
}

void SpecAnView::ingest(const int16_t* src, uint16_t px_begin, uint16_t px_end) {
    const int32_t offset = level_offset();
    for (size_t x = px_begin; x < px_end; x++) {
        const int16_t v = static_cast<int16_t>(std::clamp<int32_t>(src[x] + offset, -32000, 32000));
        live_[x] = v;
        if (v > wf_acc_[x]) wf_acc_[x] = v;
    }

    for (size_t t = 0; t < kTraces; t++) {
        auto& tr = trace_[t];
        switch (static_cast<TraceMode>(trace_mode_raw_[t])) {
            case TraceMode::ClearWrite:
                std::copy(live_.begin() + px_begin, live_.begin() + px_end, tr.begin() + px_begin);
                break;

            case TraceMode::MaxHold:
                for (size_t x = px_begin; x < px_end; x++)
                    if (tr[x] == kNoData || live_[x] > tr[x]) tr[x] = live_[x];
                break;

            case TraceMode::MinHold:
                for (size_t x = px_begin; x < px_end; x++)
                    if (tr[x] == kNoData || live_[x] < tr[x]) tr[x] = live_[x];
                break;

            case TraceMode::Average: {
                /* Exponential in log power ("video" averaging); weight 1/2^k
                 * grows in from 1/1 so the trace settles quickly. */
                const int k = std::min<int>(avg_log2_, floor_log2(avg_sweeps_[t] + 1u));
                for (size_t x = px_begin; x < px_end; x++) {
                    if (tr[x] == kNoData || k == 0) {
                        tr[x] = live_[x];
                    } else {
                        const int32_t d = live_[x] - tr[x];
                        tr[x] = static_cast<int16_t>(tr[x] + ((d + (1 << (k - 1))) >> k));
                    }
                }
                break;
            }

            default:
                break;
        }
    }

    mark_dirty(static_cast<int>(px_begin) - 1, px_end);
    for (const auto& m : markers_) {
        if (m.mode && m.x + 3 >= px_begin && m.x - 3 < px_end) mark_marker_dirty(m);
    }
}

void SpecAnView::on_sweep_done() {
    for (size_t t = 0; t < kTraces; t++) {
        if (avg_sweeps_[t] < 0xffff) avg_sweeps_[t]++;
    }

    const systime_t now = chTimeNow();
    rate_count_++;
    if (now - rate_t0_ >= 500) {
        sweep_us_ = static_cast<uint32_t>((now - rate_t0_) * 1000 / rate_count_);
        rate_t0_ = now;
        rate_count_ = 0;
    }

    /* At most ~30 waterfall lines a second; faster sweeps are peak-held
     * into one line so nothing short-lived is lost. */
    if (waterfall_ && now - wf_last_ >= 33) {
        wf_last_ = now;
        push_waterfall_line();
    }
}

void SpecAnView::clear_traces() {
    live_.fill(kNoData);
    wf_acc_.fill(kNoData);
    for (auto& t : trace_) t.fill(kNoData);
    avg_sweeps_.fill(0);
    rate_t0_ = chTimeNow();
    rate_count_ = 0;
    mark_all_dirty();
}

void SpecAnView::apply_gains() {
    receiver_model.set_lna(lna_);
    receiver_model.set_vga(vga_);
    receiver_model.set_rf_amp(amp_);
}

/* Display *****************************************************************/

void SpecAnView::update_geometry() {
    spec_rows_ = waterfall_ ? kSpecRowsSplit : kSpecRowsFull;
    div_rows_ = (spec_rows_ - 1) / 10;
    wf_top_ = kGridTop + spec_rows_ + 1;
    wf_rows_ = waterfall_ ? std::min<int>(kActiveY - kGridTop - wf_top_, kWaterfallRows) : 0;

    const int32_t range = 10 * kDbDiv[dbdiv_idx_];
    ymul_ = ((spec_rows_ - 1) << 16) / range;
    wf_mul_ = (255 << 16) / range;

    for (int r = 0; r < spec_rows_; r++) hgrid_row_[r] = (r % div_rows_) == 0;
}

int SpecAnView::y_of(int32_t cdb) const {
    const int32_t y = ((ref_cdb_ - cdb) * ymul_) >> 16;
    return std::clamp<int32_t>(y, 0, spec_rows_ - 1);
}

void SpecAnView::mark_dirty(int x0, int x1) {
    x0 = std::max(x0, 0);
    x1 = std::min<int>(x1, kPoints - 1);
    for (int x = x0; x <= x1; x++) dirty_[x >> 5] |= 1u << (x & 31);
}

void SpecAnView::mark_all_dirty() {
    dirty_.fill(0xffffffffu);
}

void SpecAnView::mark_marker_dirty(const Marker& m) {
    mark_dirty(m.x - 3, m.x + 3);
}

void SpecAnView::redraw_all() {
    if (!landscape_) return;
    display.fill_rectangle({0, 0, 320, 240}, c_bg);
    /* Every column repaints its whole graticule once. */
    drawn_top_.fill(0);
    drawn_bot_.fill(static_cast<uint8_t>(spec_rows_ - 1));
    mark_all_dirty();
    for (auto& line : text_cache_) line[0] = 0x7f;
    keys_dirty_ = true;
    wf_dirty_ = true;
    full_redraw_ = false;

    render_columns();
    render_waterfall();
    render_text();
    render_softkeys();
}

void SpecAnView::on_frame_sync() {
    if (!landscape_) return;

    /* A lost request or reply would stall the sweep: re-ask for the slice. */
    if (sweeping_ && chTimeNow() - issued_at_ > 250) issue(slice_, true);

    if (full_redraw_) {
        redraw_all();
        return;
    }
    render_columns();
    if (wf_dirty_) render_waterfall();
    render_text();
    if (keys_dirty_) render_softkeys();
}

void SpecAnView::render_columns() {
    /* Work from a snapshot: the pumped handlers below keep marking columns
     * dirty, and with a single-tune sweep every slice dirties all of them.
     * Looping on the live bitmap would never get past the first word. What
     * they mark now is drawn next frame. */
    const auto todo = dirty_;
    dirty_.fill(0);

    size_t done = 0;
    for (size_t w = 0; w < todo.size(); w++) {
        uint32_t bits = todo[w];
        while (bits) {
            const int bit = __builtin_ctz(bits);
            bits &= bits - 1;
            render_column(static_cast<int>(w * 32 + bit));
            /* Let the sweep advance while we draw: retune requests wait in
             * the queue otherwise. Handlers only mark columns dirty. */
            if ((++done & 31) == 0) EventDispatcher::pump_application_queue();
        }
    }
}

void SpecAnView::render_column(int x) {
    struct Seg {
        int y0{0};
        int y1{0};
        Color color{};
    };
    Seg segs[kTraces + kMarkers];
    size_t nsegs = 0;

    const int h = spec_rows_;
    int top = 255;
    int bot = -1;
    int fill_y = -1;

    for (int t = kTraces - 1; t >= 0; t--) {
        if (static_cast<TraceMode>(trace_mode_raw_[t]) == TraceMode::Blank) continue;
        const auto& tr = trace_[t];
        if (tr[x] == kNoData) continue;
        const int yc = y_of(tr[x]);
        const int ya = (x > 0 && tr[x - 1] != kNoData) ? y_of(tr[x - 1]) : yc;
        const int yb = (x < static_cast<int>(kPoints) - 1 && tr[x + 1] != kNoData) ? y_of(tr[x + 1]) : yc;
        /* Join halfway to each neighbour: a connected line, one column wide. */
        const int ma = (ya + yc) >> 1;
        const int mb = (yb + yc) >> 1;
        const int y0 = std::min({yc, ma, mb});
        const int y1 = std::max({yc, ma, mb});
        segs[nsegs++] = {y0, y1, c_trace[t]};
        top = std::min(top, y0);
        bot = std::max(bot, y1);
        if (t == 0 && fill_) fill_y = yc;
    }
    if (fill_y >= 0) {
        top = std::min(top, fill_y);
        bot = h - 1;
    }

    int dl_y = -1;
    if (dline_on_ && (x & 3) < 2) {
        dl_y = y_of(dline_cdb_);
        top = std::min(top, dl_y);
        bot = std::max(bot, dl_y);
    }

    for (size_t i = 0; i < kMarkers; i++) {
        const Marker& m = markers_[i];
        if (!m.mode) continue;
        const int d = std::abs(x - m.x);
        if (d > 3) continue;
        const int16_t level = trace_[m.trace][m.x];
        if (level == kNoData) continue;
        const int ym = y_of(level);
        int y0, y1;
        if (ym >= 6) { /* pointing down onto the trace */
            y0 = ym - 5;
            y1 = ym - 2 - d;
        } else { /* no room above: point up from below */
            y0 = ym + 2 + d;
            y1 = ym + 5;
        }
        y0 = std::clamp(y0, 0, h - 1);
        y1 = std::clamp(y1, 0, h - 1);
        segs[nsegs++] = {y0, y1, c_marker[i]};
        top = std::min(top, y0);
        bot = std::max(bot, y1);
    }

    /* Repaint the union of what was there and what is there now. */
    const int r0 = std::min<int>(top, drawn_top_[x]);
    const int r1 = std::min<int>(std::max<int>(bot, drawn_bot_[x]), h - 1);
    drawn_top_[x] = static_cast<uint8_t>(std::min(top, 255));
    drawn_bot_[x] = static_cast<uint8_t>(std::max(bot, 0));
    if (r0 > r1) return;

    const bool edge = (x == 0) || (x == static_cast<int>(kPoints) - 1);
    const bool vgrid = (x & 31) == 0;
    const bool even = (x & 1) == 0;
    for (int r = r0; r <= r1; r++) {
        Color c = c_bg;
        if (edge || r == 0 || r == h - 1)
            c = c_border;
        else if ((hgrid_row_[r] && even) || (vgrid && !(r & 1)))
            c = c_grid;
        column_buf_[r] = c;
    }

    if (fill_y >= 0) {
        for (int r = std::max(r0, fill_y); r <= std::min(r1, h - 2); r++) {
            column_buf_[r] = (column_buf_[r].v == c_bg.v) ? c_fill : c_fill_grid;
        }
    }
    if (dl_y >= r0 && dl_y <= r1) column_buf_[dl_y] = c_dline;
    for (size_t i = 0; i < nsegs; i++) {
        const int a = std::max(r0, segs[i].y0);
        const int b = std::min(r1, segs[i].y1);
        for (int r = a; r <= b; r++) column_buf_[r] = segs[i].color;
    }

    display.start_pixels({x, kGridTop + r0, 1, r1 - r0 + 1});
    display.stream_pixels(&column_buf_[r0], r1 - r0 + 1);
}

void SpecAnView::push_waterfall_line() {
    uint8_t* const wf = shared_.waterfall;
    if (!wf) return;

    uint8_t* const row = wf + wf_head_ * kPoints;
    const int32_t bottom = ref_cdb_ - 10 * kDbDiv[dbdiv_idx_];
    for (size_t x = 0; x < kPoints; x++) {
        const int16_t v = wf_acc_[x];
        int32_t q = 0;
        if (v != kNoData && v > bottom) {
            q = ((v - bottom) * wf_mul_) >> 16;
            if (q > 255) q = 255;
        }
        row[x] = static_cast<uint8_t>(q);
        wf_acc_[x] = kNoData;
    }

    wf_head_ = (wf_head_ + 1 == kWaterfallRows) ? 0 : wf_head_ + 1;
    if (wf_count_ < kWaterfallRows) wf_count_++;
    wf_dirty_ = true;
}

void SpecAnView::render_waterfall() {
    wf_dirty_ = false;
    if (!waterfall_ || wf_rows_ <= 0) return;

    const uint8_t* const wf = shared_.waterfall;
    display.start_pixels({0, wf_top_, static_cast<int>(kPoints), wf_rows_});
    /* Newest line on top. Message pumping in between is safe: handlers do
     * not touch the LCD, so the open window survives. */
    int line = static_cast<int>(wf_head_) - 1;
    for (int r = 0; r < wf_rows_; r++) {
        if (line < 0) line += static_cast<int>(kWaterfallRows);
        if (wf && r < wf_count_)
            display.stream_pixels_lut(wf + line * kPoints, palette_.data(), kPoints);
        else
            display.stream_fill(c_bg, kPoints);
        line--;
        if ((r & 15) == 15) EventDispatcher::pump_application_queue();
    }
}

void SpecAnView::draw_text_line(size_t slot, Point p, const Font& font, Color fg, Color bg, const char* text, size_t width) {
    auto& cache = text_cache_[slot];
    if (std::strncmp(cache.data(), text, cache.size()) == 0) return;
    std::strncpy(cache.data(), text, cache.size() - 1);
    cache[cache.size() - 1] = 0;

    char padded[80];
    const size_t n = std::min(std::strlen(text), width);
    std::memcpy(padded, text, n);
    std::memset(padded + n, ' ', width - n);
    padded[width] = 0;
    Painter painter;
    painter.draw_string(p, font, fg, bg, padded);
}

void SpecAnView::render_text() {
    /* Readouts jitter with the noise; refresh them at a readable rate. */
    static systime_t last = 0;
    const systime_t now = chTimeNow();
    const bool slow_tick = now - last >= 150;
    if (slow_tick) last = now;

    {
        TextBuf t;
        t.s("Ref ").cdb(ref_cdb_).s("dB ").i(kDbDiv[dbdiv_idx_] / 100).s("dB/ LNA").u(lna_).s(" VGA").u(vga_).s(amp_ ? " AMP+" : " AMP-");
        if (ref_offset_cdb_) t.s(" Ofs").cdb(ref_offset_cdb_, 1, true);
        TextBuf right;
        right.s(sweeping_ ? (continuous_ ? "CONT " : "SGL ") : "HOLD ");
        if (sweep_us_ >= 10'000)
            right.u(sweep_us_ / 1000).s("ms");
        else
            right.u(sweep_us_ / 1000).c('.').u((sweep_us_ / 100) % 10).s("ms");
        t.pad(kSmallChars - right.size()).s(right.str());
        draw_text_line(0, {0, kLine1Y}, font::fixed_5x8, c_text, c_bg, t.str(), kSmallChars);
    }

    if (slow_tick) {
        TextBuf t;
        const Marker& ref = markers_[0];
        size_t shown = 0;
        for (size_t i = 0; i < kMarkers && shown < 2; i++) {
            /* The selected marker first, then the next one that is on. */
            const size_t idx = (mkr_sel_ + i) % kMarkers;
            const Marker& m = markers_[idx];
            if (!m.mode) continue;
            const int32_t level = marker_level(m);
            if (shown) t.s("  ");
            if (m.mode == 2 && ref.mode && idx != 0) {
                const int64_t df = static_cast<int64_t>(point_freq(m.x)) - static_cast<int64_t>(point_freq(ref.x));
                t.c('D').u(idx + 1).c(' ').c(df < 0 ? '-' : '+').freq_compact(df < 0 ? -df : df).s("Hz ");
                if (level != kNoData && marker_level(ref) != kNoData) t.cdb(level - marker_level(ref), 2, true).s("dB");
            } else {
                t.c('M').u(idx + 1).c(' ').freq_compact(point_freq(m.x)).s("Hz ");
                if (level != kNoData) t.cdb(level, 2).s("dB");
            }
            shown++;
        }
        draw_text_line(1, {0, kLine2Y}, font::fixed_5x8, c_marker[mkr_sel_], c_bg, t.str(), kSmallChars);
    }

    {
        TextBuf t;
        t.s("CF ").freq_compact(center_).s("Hz  Span ").freq_compact(span_).s("Hz  RBW ").freq_compact(plan_.rbw_hz).s("Hz  VBW ").freq_compact(plan_.rbw_hz / plan_.avg).s("Hz");
        TextBuf right;
        if (settle_low() && plan_.slices > 1) right.s("SETTLE LOW  ");
        right.u(plan_.slices).s(plan_.slices == 1 ? "tune" : "tunes");
        t.pad(kSmallChars - right.size()).s(right.str());
        draw_text_line(2, {0, kAnnoY}, font::fixed_5x8, c_dim, c_bg, t.str(), kSmallChars);
    }

    /* Active function: label left (cyan), value right (white). */
    TextBuf label;
    TextBuf value;
    Color value_color = c_text;
    switch (active_) {
        case Item::Center:
            label.s("Center");
            value.mhz_full(center_);
            break;
        case Item::Start:
            label.s("Start");
            value.mhz_full(start_freq());
            break;
        case Item::Stop:
            label.s("Stop");
            value.mhz_full(stop_freq());
            break;
        case Item::Span:
            label.s("Span");
            value.freq(span_);
            break;
        case Item::RefLevel:
            label.s("Ref level");
            value.cdb(ref_cdb_).s(" dB");
            break;
        case Item::DbDiv:
            label.s("Scale");
            value.i(kDbDiv[dbdiv_idx_] / 100).s(" dB/div");
            break;
        case Item::Lna:
            label.s("LNA gain");
            value.u(lna_).s(" dB");
            break;
        case Item::Vga:
            label.s("VGA gain");
            value.u(vga_).s(" dB");
            break;
        case Item::RefOffset:
            label.s("Ref offset");
            value.cdb(ref_offset_cdb_, 1, true).s(" dB");
            break;
        case Item::Rbw:
            label.s("Res BW");
            value.s(rbw_idx_ ? "" : "Auto ").freq(plan_.rbw_hz).s(" N").u(1u << plan_.log2n);
            break;
        case Item::Vbw:
            label.s("Video BW");
            value.freq(plan_.rbw_hz / plan_.avg).s(" avg").u(plan_.avg);
            break;
        case Item::Window:
            label.s("Window");
            value.s(window_names[window_]);
            break;
        case Item::Detector:
            label.s("Detector");
            value.s(detector_names[detector_]);
            break;
        case Item::Layout:
            label.s("Bins");
            value.s(layout_names[layout_]);
            break;
        case Item::Settle:
            label.s("Settle");
            value.u(settle_us_).s("us ").u(plan_.settle_bufs).s("buf");
            if (settle_low()) {
                value.s(" <").u(kSettleMinUs);
                value_color = c_warn;
            }
            break;
        case Item::TraceSel:
        case Item::TraceMode:
            label.s("Trace ").u(trace_sel_ + 1u);
            value.s(trace_mode_names[trace_mode_raw_[trace_sel_]]);
            break;
        case Item::AvgCount:
            label.s("Avg count");
            value.u(1u << avg_log2_);
            break;
        case Item::MkrSel: {
            const Marker& m = markers_[mkr_sel_];
            label.s("Marker ").u(mkr_sel_ + 1u);
            if (!m.mode) {
                value.s("Off");
            } else if (slow_tick || text_cache_[3][0] == 0x7f) {
                value.freq(point_freq(m.x));
                const int32_t level = marker_level(m);
                if (level != kNoData) value.c(' ').cdb(level).s("dB");
            } else {
                return; /* keep the last readout until the next slow tick */
            }
            break;
        }
        case Item::DispLine:
            label.s("Display line");
            if (dline_on_)
                value.cdb(dline_cdb_).s(" dB");
            else
                value.s("Off");
            break;
        default:
            label.s("Spectrum Analyzer");
            break;
    }

    TextBuf line;
    line.s(label.str()).c('\x01').s(value.str());
    auto& cache = text_cache_[3];
    if (std::strncmp(cache.data(), line.str(), cache.size()) == 0) return;
    std::strncpy(cache.data(), line.str(), cache.size() - 1);

    const size_t vlen = std::min<size_t>(value.size(), kBigChars);
    const size_t lwidth = kBigChars - vlen;
    TextBuf lpad;
    lpad.s(label.str()).pad(lwidth);
    Painter painter;
    painter.draw_string({0, kActiveY}, font::fixed_8x16, c_label, c_bg, std::string_view{lpad.str(), lwidth});
    painter.draw_string({static_cast<int>(lwidth * 8), kActiveY}, font::fixed_8x16, value_color, c_bg, std::string_view{value.str(), vlen});
}

void SpecAnView::render_softkeys() {
    keys_dirty_ = false;
    Painter painter;

    const bool back_cursor = cursor_ == 0;
    display.fill_rectangle({0, kSoftY, kBackW - 1, kSoftH}, back_cursor ? c_key_cursor : c_key_bg);
    painter.draw_string({6, kSoftY + 2}, font::fixed_8x16, back_cursor ? c_bg : c_text, back_cursor ? c_key_cursor : c_key_bg, "<");

    int x = kBackW;
    for (uint8_t i = 0; i < menu_len_; i++, x += kKeyW) {
        const Item item = menu_[i];
        const bool cur = cursor_ == i + 1;
        const Color bg = cur ? c_key_cursor : (item == active_ ? c_key_active : c_key_bg);
        const Color fg = cur ? c_bg : (item_lit(item) ? c_key_lit : c_text);
        display.fill_rectangle({x, kSoftY, kKeyW - 1, kSoftH}, bg);
        const char* label = item_label(item);
        const int w = std::min<int>(std::strlen(label), 5) * 8;
        painter.draw_string({x + (kKeyW - 1 - w) / 2, kSoftY + 2}, font::fixed_8x16, fg, bg, std::string_view{label, std::min<size_t>(std::strlen(label), 5)});
    }
    if (x < 320) display.fill_rectangle({x, kSoftY, 320 - x, kSoftH}, c_bg);
}

/* Controls ****************************************************************/

void SpecAnView::set_menu(const Item* menu, uint8_t len, uint8_t cursor) {
    menu_ = menu;
    menu_len_ = len;
    cursor_ = std::min(cursor, len);
    keys_dirty_ = true;
}

void SpecAnView::go_back() {
    if (menu_ != menu_root) {
        static const Item* const submenus[] = {menu_freq, menu_span, menu_ampt, menu_bw, menu_trace, menu_mkr, menu_disp};
        uint8_t idx = 0;
        for (uint8_t i = 0; i < count_of(menu_root); i++)
            if (submenus[i] == menu_) idx = i;
        set_menu(menu_root, count_of(menu_root), idx + 1);
        return;
    }
    nav_.pop();
}

KeyEvent SpecAnView::remap_key(KeyEvent key) const {
    if (!display.is_landscape()) return key;
    /* The D-pad turns with the panel. Default: device turned anticlockwise. */
    if (!flip_) {
        switch (key) {
            case KeyEvent::Up:
                return KeyEvent::Left;
            case KeyEvent::Right:
                return KeyEvent::Up;
            case KeyEvent::Down:
                return KeyEvent::Right;
            case KeyEvent::Left:
                return KeyEvent::Down;
            default:
                return key;
        }
    }
    switch (key) {
        case KeyEvent::Up:
            return KeyEvent::Right;
        case KeyEvent::Right:
            return KeyEvent::Down;
        case KeyEvent::Down:
            return KeyEvent::Left;
        case KeyEvent::Left:
            return KeyEvent::Up;
        default:
            return key;
    }
}

Point SpecAnView::remap_touch(Point p) const {
    if (!display.is_landscape()) return p;
    /* Inverse of the MADCTL mapping in ILI9341::set_landscape(). */
    if (!flip_) return {p.y(), 239 - p.x()};
    return {319 - p.y(), p.x()};
}

bool SpecAnView::on_key(const KeyEvent key) {
    switch (remap_key(key)) {
        case KeyEvent::Left:
            if (cursor_ > 0) cursor_--;
            keys_dirty_ = true;
            return true;
        case KeyEvent::Right:
            if (cursor_ < menu_len_) cursor_++;
            keys_dirty_ = true;
            return true;
        case KeyEvent::Up:
            adjust(active_, 1, true);
            return true;
        case KeyEvent::Down:
            adjust(active_, -1, true);
            return true;
        case KeyEvent::Select:
            if (cursor_ == 0)
                go_back();
            else
                activate(menu_[cursor_ - 1]);
            return true;
        case KeyEvent::Back:
            go_back();
            return true;
        default:
            return false;
    }
}

bool SpecAnView::on_encoder(const EncoderEvent delta) {
    adjust(active_, delta, false);
    return true;
}

bool SpecAnView::on_touch(const TouchEvent event) {
    /* The dispatcher delivers Start twice (hit test, then capture), so
     * Start only selects and the action fires once, on End. */
    if (event.type == TouchEvent::Type::End) {
        const TouchTarget target = touch_target_;
        touch_target_ = TouchTarget::None;
        if (target == TouchTarget::Softkey && cursor_ <= menu_len_) {
            if (cursor_ == 0)
                go_back();
            else
                activate(menu_[cursor_ - 1]);
        } else if (target == TouchTarget::Keypad) {
            open_keypad(active_);
        }
        return true;
    }
    if (event.type == TouchEvent::Type::Start) touch_target_ = TouchTarget::None;
    const Point p = remap_touch(event.point);

    if (p.y() >= kSoftY) {
        if (event.type != TouchEvent::Type::Start) return true;
        const int idx = (p.x() < kBackW) ? 0 : 1 + (p.x() - kBackW) / kKeyW;
        if (idx > menu_len_) return true;
        cursor_ = idx;
        keys_dirty_ = true;
        touch_target_ = TouchTarget::Softkey;
        return true;
    }

    if (p.y() >= kGridTop && p.y() < kGridTop + spec_rows_) {
        /* Tap or drag on the plot moves the selected marker. */
        Marker& m = markers_[mkr_sel_];
        mark_marker_dirty(m);
        if (!m.mode) m.mode = 1;
        m.x = std::clamp<int>(p.x(), 0, kPoints - 1);
        mark_marker_dirty(m);
        active_ = Item::MkrSel;
        keys_dirty_ = true;
        return true;
    }

    if (event.type == TouchEvent::Type::Start && p.y() >= kActiveY && p.y() < kSoftY) {
        if (active_ == Item::Center || active_ == Item::Start || active_ == Item::Stop || active_ == Item::Span)
            touch_target_ = TouchTarget::Keypad;
    }
    return true;
}

void SpecAnView::open_keypad(Item item) {
    uint64_t value = center_;
    if (item == Item::Start) value = start_freq();
    if (item == Item::Stop) value = stop_freq();
    if (item == Item::Span) value = span_;

    /* The keypad is a portrait view. paint() turns us back on return. */
    leave_landscape();
    auto keypad = nav_.push<FrequencyKeypadView>(static_cast<rf::Frequency>(value));
    keypad->on_changed = [this, item](rf::Frequency f) {
        const uint64_t v = static_cast<uint64_t>(std::clamp<rf::Frequency>(f, 0, kMaxFreq));
        switch (item) {
            case Item::Start:
                set_start_stop(v, stop_freq());
                break;
            case Item::Stop:
                set_start_stop(start_freq(), v);
                break;
            case Item::Span:
                set_center_span(center_, v);
                break;
            default:
                set_center_span(v, span_);
                break;
        }
    };
}

void SpecAnView::activate(Item item) {
    keys_dirty_ = true;
    switch (item) {
        case Item::MenuFreq:
            set_menu(menu_freq, count_of(menu_freq), 1);
            active_ = Item::Center;
            return;
        case Item::MenuSpan:
            set_menu(menu_span, count_of(menu_span), 1);
            active_ = Item::Span;
            return;
        case Item::MenuAmpt:
            set_menu(menu_ampt, count_of(menu_ampt), 1);
            active_ = Item::RefLevel;
            return;
        case Item::MenuBw:
            set_menu(menu_bw, count_of(menu_bw), 1);
            active_ = Item::Rbw;
            return;
        case Item::MenuTrace:
            set_menu(menu_trace, count_of(menu_trace), 2);
            active_ = Item::TraceMode;
            return;
        case Item::MenuMkr:
            set_menu(menu_mkr, count_of(menu_mkr), 3);
            active_ = Item::MkrSel;
            return;
        case Item::MenuDisp:
            set_menu(menu_disp, count_of(menu_disp), 1);
            return;

        /* Value items: first press makes them active, a second press does the
         * natural thing for touch users (keypad, cycle, toggle). */
        case Item::Center:
        case Item::Start:
        case Item::Stop:
        case Item::Span:
            if (active_ == item)
                open_keypad(item);
            else
                active_ = item;
            return;
        case Item::DbDiv:
        case Item::Window:
        case Item::Detector:
        case Item::Layout:
        case Item::TraceSel:
        case Item::TraceMode:
        case Item::Rbw:
        case Item::Vbw:
            if (active_ == item)
                adjust(item, 1, false);
            else
                active_ = item;
            return;
        case Item::RefLevel:
        case Item::Lna:
        case Item::Vga:
        case Item::RefOffset:
        case Item::Settle:
        case Item::AvgCount:
            active_ = item;
            return;
        case Item::MkrSel:
            if (active_ == item) {
                mkr_sel_ = (mkr_sel_ + 1) % kMarkers;
            }
            active_ = item;
            if (!markers_[mkr_sel_].mode) peak_search();
            return;
        case Item::DispLine:
            /* Off: turn on. On and already active: turn off. */
            if (!dline_on_)
                dline_on_ = true;
            else if (active_ == item)
                dline_on_ = false;
            mark_all_dirty();
            active_ = item;
            return;

        case Item::Keypad:
            open_keypad((active_ == Item::Start || active_ == Item::Stop || active_ == Item::Span) ? active_ : Item::Center);
            return;
        case Item::FullSpan:
            set_start_stop(kFullStart, kFullStop);
            active_ = Item::Span;
            return;
        case Item::ZoomIn:
            set_center_span(center_, span_ / 2);
            active_ = Item::Span;
            return;
        case Item::ZoomOut:
            set_center_span(center_, span_ * 2);
            active_ = Item::Span;
            return;
        case Item::LastSpan:
            set_center_span(center_, last_span_);
            active_ = Item::Span;
            return;
        case Item::Amp:
            amp_ = !amp_;
            apply_gains();
            clear_traces();
            return;
        case Item::AutoRef: {
            int32_t peak = INT32_MIN;
            for (const int16_t v : trace_[0])
                if (v != kNoData) peak = std::max<int32_t>(peak, v);
            if (peak != INT32_MIN) {
                /* Peak a little under the top line: ref on the next 5 dB
                 * boundary at least 5 dB above it. */
                const int32_t r = peak + 500;
                const int32_t up = (r >= 0) ? ((r + 499) / 500) * 500 : -((-r) / 500) * 500;
                ref_cdb_ = std::clamp<int32_t>(up, -15000, 3000);
                update_geometry();
                mark_all_dirty();
                wf_dirty_ = true;
            }
            active_ = Item::RefLevel;
            return;
        }
        case Item::Fill:
            fill_ = !fill_;
            mark_all_dirty();
            return;
        case Item::TraceClear:
            trace_[trace_sel_].fill(kNoData);
            avg_sweeps_[trace_sel_] = 0;
            mark_all_dirty();
            return;
        case Item::MkrMode: {
            Marker& m = markers_[mkr_sel_];
            mark_marker_dirty(m);
            m.mode = (m.mode + 1) % (mkr_sel_ == 0 ? 2 : 3);
            if (m.mode) m.trace = trace_sel_;
            mark_marker_dirty(m);
            active_ = Item::MkrSel;
            return;
        }
        case Item::PeakSearch:
            peak_search();
            active_ = Item::MkrSel;
            return;
        case Item::NextPeak:
            next_peak();
            active_ = Item::MkrSel;
            return;
        case Item::MkrToCf: {
            const Marker& m = markers_[mkr_sel_];
            if (m.mode) set_center_span(point_freq(m.x), span_);
            active_ = Item::Center;
            return;
        }
        case Item::MkrToRef: {
            const int32_t level = marker_level(markers_[mkr_sel_]);
            if (markers_[mkr_sel_].mode && level != kNoData) {
                ref_cdb_ = std::clamp<int32_t>(level, -15000, 3000);
                update_geometry();
                mark_all_dirty();
            }
            active_ = Item::RefLevel;
            return;
        }
        case Item::MkrAllOff:
            for (auto& m : markers_) {
                mark_marker_dirty(m);
                m.mode = 0;
            }
            return;
        case Item::Waterfall:
            waterfall_ = !waterfall_;
            wf_acc_.fill(kNoData);
            update_geometry();
            full_redraw_ = true;
            return;
        case Item::SweepMode:
            continuous_ = !continuous_;
            if (continuous_ && !sweeping_) restart_sweep();
            return;
        case Item::SweepRun:
            restart_sweep();
            return;
        case Item::Flip:
            flip_ = !flip_;
            if (landscape_) display.set_landscape(true, flip_);
            full_redraw_ = true;
            return;
        default:
            return;
    }
}

void SpecAnView::adjust(Item item, int32_t steps, bool coarse) {
    if (!steps) return;
    keys_dirty_ = true;
    switch (item) {
        case Item::Center: {
            const int64_t d = steps * static_cast<int64_t>(coarse ? span_ / 10 : fine_step());
            set_center_span(static_cast<uint64_t>(std::max<int64_t>(static_cast<int64_t>(center_) + d, 0)), span_);
            break;
        }
        case Item::Start: {
            const int64_t d = steps * static_cast<int64_t>(coarse ? span_ / 10 : fine_step());
            set_start_stop(static_cast<uint64_t>(std::max<int64_t>(static_cast<int64_t>(start_freq()) + d, 0)), stop_freq());
            break;
        }
        case Item::Stop: {
            const int64_t d = steps * static_cast<int64_t>(coarse ? span_ / 10 : fine_step());
            set_start_stop(start_freq(), static_cast<uint64_t>(std::max<int64_t>(static_cast<int64_t>(stop_freq()) + d, 0)));
            break;
        }
        case Item::Span: {
            uint64_t s = span_;
            for (int32_t i = 0; i < std::abs(steps); i++) s = step_125(s, steps);
            set_center_span(center_, s);
            break;
        }
        case Item::RefLevel:
            ref_cdb_ = std::clamp<int32_t>(ref_cdb_ + steps * (coarse ? kDbDiv[dbdiv_idx_] : 100), -15000, 3000);
            update_geometry();
            mark_all_dirty();
            break;
        case Item::DbDiv:
            dbdiv_idx_ = (dbdiv_idx_ + kDbDivCount + (steps > 0 ? 1 : -1)) % kDbDivCount;
            update_geometry();
            mark_all_dirty();
            break;
        case Item::Lna:
            lna_ = std::clamp<int32_t>(lna_ + 8 * steps, 0, 40);
            apply_gains();
            clear_traces();
            break;
        case Item::Vga:
            vga_ = std::clamp<int32_t>(vga_ + 2 * steps, 0, 62);
            apply_gains();
            clear_traces();
            break;
        case Item::RefOffset:
            ref_offset_cdb_ = std::clamp<int32_t>(ref_offset_cdb_ + steps * (coarse ? 1000 : 10), -10000, 10000);
            clear_traces();
            break;
        case Item::Rbw:
            rbw_idx_ = std::clamp<int32_t>(rbw_idx_ + steps, 0, kMaxRbwIdx);
            apply_plan(true);
            break;
        case Item::Vbw:
            vbw_idx_ = std::clamp<int32_t>(vbw_idx_ + steps, 0, kMaxVbwIdx);
            apply_plan(true);
            break;
        case Item::Window: {
            const int n = static_cast<int>(Window::Count);
            window_ = (window_ + n + (steps > 0 ? 1 : -1)) % n;
            apply_plan(true);
            break;
        }
        case Item::Detector: {
            const int n = static_cast<int>(Detector::Count);
            detector_ = (detector_ + n + (steps > 0 ? 1 : -1)) % n;
            apply_plan(true);
            break;
        }
        case Item::Layout:
            layout_ = (layout_ + 1) % static_cast<uint8_t>(BinLayout::Count);
            apply_plan(true);
            break;
        case Item::Settle:
            settle_us_ = std::clamp<int32_t>(static_cast<int32_t>(settle_us_) + steps * (coarse ? 500 : 50), 0, 5000);
            apply_plan(false);
            break;
        case Item::TraceSel:
            trace_sel_ = (trace_sel_ + kTraces + (steps > 0 ? 1 : -1)) % kTraces;
            break;
        case Item::TraceMode: {
            const int n = static_cast<int>(TraceMode::Count);
            uint8_t& mode = trace_mode_raw_[trace_sel_];
            mode = (mode + n + (steps > 0 ? 1 : -1)) % n;
            if (static_cast<TraceMode>(mode) != TraceMode::View) {
                trace_[trace_sel_].fill(kNoData);
                avg_sweeps_[trace_sel_] = 0;
            }
            mark_all_dirty();
            break;
        }
        case Item::AvgCount:
            avg_log2_ = std::clamp<int32_t>(avg_log2_ + steps, 1, 7);
            break;
        case Item::MkrSel: {
            Marker& m = markers_[mkr_sel_];
            mark_marker_dirty(m);
            if (!m.mode) m.mode = 1;
            m.x = std::clamp<int32_t>(m.x + steps * (coarse ? 32 : 1), 0, kPoints - 1);
            mark_marker_dirty(m);
            break;
        }
        case Item::DispLine:
            dline_on_ = true;
            dline_cdb_ = std::clamp<int32_t>(dline_cdb_ + steps * (coarse ? kDbDiv[dbdiv_idx_] : 100), -20000, 3000);
            mark_all_dirty();
            break;
        default:
            break;
    }
}

const char* SpecAnView::item_label(Item item) const {
    static const char* const trace_labels[] = {"Tr 1", "Tr 2", "Tr 3"};
    static const char* const marker_labels[] = {"Mkr1", "Mkr2", "Mkr3", "Mkr4"};
    static const char* const marker_modes[] = {"Off", "Norm", "Delta"};
    switch (item) {
        case Item::MenuFreq:
            return "FREQ";
        case Item::MenuSpan:
            return "SPAN";
        case Item::MenuAmpt:
            return "AMPT";
        case Item::MenuBw:
            return "BW";
        case Item::MenuTrace:
            return "TRACE";
        case Item::MenuMkr:
            return "MKR";
        case Item::MenuDisp:
            return "DISP";
        case Item::Center:
            return "Centr";
        case Item::Start:
            return "Start";
        case Item::Stop:
            return "Stop";
        case Item::Keypad:
            return "Keys";
        case Item::Span:
            return "Span";
        case Item::FullSpan:
            return "Full";
        case Item::ZoomIn:
            return "Zoom+";
        case Item::ZoomOut:
            return "Zoom-";
        case Item::LastSpan:
            return "Last";
        case Item::RefLevel:
            return "Ref";
        case Item::DbDiv:
            return "dB/dv";
        case Item::Lna:
            return "LNA";
        case Item::Vga:
            return "VGA";
        case Item::Amp:
            return "Amp";
        case Item::RefOffset:
            return "Offst";
        case Item::AutoRef:
            return "AutoR";
        case Item::Rbw:
            return "RBW";
        case Item::Vbw:
            return "VBW";
        case Item::Window:
            return "Windw";
        case Item::Detector:
            return "Detct";
        case Item::Layout:
            return "Bins";
        case Item::Settle:
            return "Settl";
        case Item::TraceSel:
            return trace_labels[trace_sel_];
        case Item::TraceMode:
            return trace_mode_short[trace_mode_raw_[trace_sel_]];
        case Item::AvgCount:
            return "AvgN";
        case Item::Fill:
            return "Fill";
        case Item::TraceClear:
            return "Clear";
        case Item::MkrSel:
            return marker_labels[mkr_sel_];
        case Item::MkrMode:
            return marker_modes[markers_[mkr_sel_].mode];
        case Item::PeakSearch:
            return "Peak";
        case Item::NextPeak:
            return "NxtPk";
        case Item::MkrToCf:
            return ">CF";
        case Item::MkrToRef:
            return ">Ref";
        case Item::MkrAllOff:
            return "AllOf";
        case Item::Waterfall:
            return "Wfall";
        case Item::DispLine:
            return "DLine";
        case Item::SweepMode:
            return continuous_ ? "Cont" : "Singl";
        case Item::SweepRun:
            return "Run";
        case Item::Flip:
            return "Flip";
        default:
            return "";
    }
}

bool SpecAnView::item_lit(Item item) const {
    switch (item) {
        case Item::Amp:
            return amp_;
        case Item::Fill:
            return fill_;
        case Item::Waterfall:
            return waterfall_;
        case Item::DispLine:
            return dline_on_;
        case Item::SweepMode:
            return continuous_;
        case Item::Flip:
            return flip_;
        case Item::MkrMode:
            return markers_[mkr_sel_].mode != 0;
        default:
            return false;
    }
}

/* Frequencies *************************************************************/

uint64_t SpecAnView::start_freq() const {
    return center_ - span_ / 2;
}

uint64_t SpecAnView::stop_freq() const {
    return start_freq() + span_;
}

uint64_t SpecAnView::point_freq(int x) const {
    return start_freq() + static_cast<uint64_t>(x) * span_ / (kPoints - 1);
}

uint64_t SpecAnView::fine_step() const {
    return nice_floor(std::max<uint64_t>(span_ / 100, 1));
}

void SpecAnView::set_center_span(uint64_t center, uint64_t span) {
    span = std::clamp<uint64_t>(span, kMinSpan, kMaxFreq);
    center = std::clamp<uint64_t>(center, span / 2, kMaxFreq - span / 2);
    if (span != span_) last_span_ = span_;
    center_ = center;
    span_ = span;
    apply_plan(true);
}

void SpecAnView::set_start_stop(uint64_t start, uint64_t stop) {
    if (stop < start + kMinSpan) {
        if (start + kMinSpan <= kMaxFreq)
            stop = start + kMinSpan;
        else
            start = stop - kMinSpan;
    }
    set_center_span(start + (stop - start) / 2, stop - start);
}

/* Markers *****************************************************************/

int32_t SpecAnView::marker_level(const Marker& m) const {
    return trace_[m.trace][m.x];
}

void SpecAnView::peak_search() {
    Marker& m = markers_[mkr_sel_];
    const auto& tr = trace_[m.mode ? m.trace : trace_sel_];
    int best = -1;
    for (size_t x = 0; x < kPoints; x++) {
        if (tr[x] != kNoData && (best < 0 || tr[x] > tr[best])) best = x;
    }
    mark_marker_dirty(m);
    if (!m.mode) {
        m.mode = 1;
        m.trace = trace_sel_;
    }
    if (best >= 0) m.x = best;
    mark_marker_dirty(m);
}

void SpecAnView::next_peak() {
    Marker& m = markers_[mkr_sel_];
    if (!m.mode) {
        peak_search();
        return;
    }
    const auto& tr = trace_[m.trace];
    const int16_t current = tr[m.x];
    if (current == kNoData) return;

    /* Highest local maximum strictly below the current one, 3 dB clear of
     * the lowest point between it and the marker so noise doesn't count. */
    int best = -1;
    for (int x = 1; x < static_cast<int>(kPoints) - 1; x++) {
        const int16_t v = tr[x];
        if (v == kNoData || v >= current || std::abs(x - m.x) < 2) continue;
        if (v < tr[x - 1] || v < tr[x + 1]) continue;
        if (best >= 0 && v <= tr[best]) continue;
        int16_t valley = v;
        const int step = (x < m.x) ? 1 : -1;
        for (int i = x; i != m.x; i += step) valley = std::min(valley, tr[i]);
        if (v - valley < 300) continue;
        best = x;
    }
    if (best < 0) return;
    mark_marker_dirty(m);
    m.x = best;
    mark_marker_dirty(m);
}

} /* namespace ui::external_app::spec_an */
