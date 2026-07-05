#include <cstdint>
#include <cstring>

#include "ch.h"

#include "ui.hpp"
#include "ui_text.hpp"
#include "portapack.hpp"
#include "radio.hpp"
#include "baseband_api.hpp"

#include "drone_scanner_ui.hpp"
#include "pattern_manager_view.hpp"
#include "pattern_matcher.hpp"
#include "peak_detector.hpp"
#include "constants.hpp"

namespace drone_analyzer {

// ============================================================================
// Constructor / Destructor — handler lifecycle pattern
// ============================================================================

PatternManagerView::PatternManagerView(NavigationView& nav) noexcept
    : View()
    , nav_(nav)
    , labels_{
          {{UI_POS_X(0), UI_POS_Y(0)}, "PTR Pattern", Color::white()},
          {{UI_POS_X(0), 20}, "Tap spectrum to select", Color::white()},
      }
    , field_patterns_{{0, LIST_Y}, 22, {}, false}
    , button_capture_{{UI_POS_X(0), 270, UI_POS_WIDTH(5), 20}, "Capt"}
    , button_save_{{UI_POS_X(6), 270, UI_POS_WIDTH(5), 20}, "Save"}
    , button_delete_{{UI_POS_X(12), 270, UI_POS_WIDTH(5), 20}, "Del"}
    , button_toggle_{{UI_POS_X(18), 270, UI_POS_WIDTH(5), 20}, "On/Off"}
    , button_back_{{UI_POS_X(24), 270, UI_POS_WIDTH(3), 20}, "<="}
    , field_freq_mhz_{{UI_POS_X(10), UI_POS_Y(1)}, 5, {100, 7200}, 1, ' '}
    , button_enable_all_{{UI_POS_X(6), BUTTONS2_Y, UI_POS_WIDTH(5), 20}, "EnAll"}
    , button_disable_all_{{UI_POS_X(12), BUTTONS2_Y, UI_POS_WIDTH(6), 20}, "DisAll"}
    , label_status_{{UI_POS_X(0), 30, UI_POS_WIDTH(28), 20}, "Idle"} {

    add_children({
        &labels_,
        &field_patterns_,
        &button_capture_,
        &button_save_,
        &button_delete_,
        &button_toggle_,
        &button_back_,
        &field_freq_mhz_,
        &button_enable_all_,
        &button_disable_all_,
        &label_status_,
    });

    button_back_.on_select = [this](ui::Button&) {
        nav_.pop();
    };

    button_capture_.on_select = [this](ui::Button&) {
        if (capture_completed_) {
            capture_completed_ = false;
            selected_bin_ = -1;
            capture_active_ = false;
            view_state_ = ViewState::IDLE;
            label_status_.set("Ready");
            set_dirty();
            return;
        }
        if (selected_bin_ < 0) {
            label_status_.set("Select bin first!");
            set_dirty();
            return;
        }
        start_capture();
    };

    button_save_.on_select = [this](ui::Button&) {
        capture_and_save();
    };

    button_delete_.on_select = [this](ui::Button&) {
        delete_selected();
    };

    button_toggle_.on_select = [this](ui::Button&) {
        toggle_enabled();
    };

    field_patterns_.on_change = [this](size_t index, int32_t) {
        selected_index_ = static_cast<uint8_t>(index);
    };

    // Frequency input — MHz field with FrequencyKeypadView for precise entry
    field_freq_mhz_.on_change = [this](int32_t v) {
        capture_freq_ = static_cast<FreqHz>(v) * 1000000ULL;
    };

    field_freq_mhz_.on_select = [this](ui::NumberField&) {
        baseband::spectrum_streaming_stop();
        auto new_view = nav_.push<FrequencyKeypadView>(
            static_cast<rf::Frequency>(field_freq_mhz_.value()) * 1000000ULL);
        new_view->on_changed = [this](rf::Frequency f) {
            field_freq_mhz_.set_value(static_cast<int32_t>(f / 1000000ULL));
        };
    };

    // Enable All / Disable All buttons
    button_enable_all_.on_select = [this](ui::Button&) {
        if (pm_ == nullptr || scanner_ == nullptr) return;
        const size_t count = pm_->get_pattern_count();
        for (size_t i = 0; i < count; ++i) {
            const SignalPattern* p = pm_->get_pattern(i);
            if (p != nullptr && !p->is_enabled()) {
                (void)pm_->toggle_pattern(i);
            }
        }
        scanner_->refresh_patterns();
        refresh_list();
    };

    button_disable_all_.on_select = [this](ui::Button&) {
        if (pm_ == nullptr || scanner_ == nullptr) return;
        const size_t count = pm_->get_pattern_count();
        for (size_t i = 0; i < count; ++i) {
            const SignalPattern* p = pm_->get_pattern(i);
            if (p != nullptr && p->is_enabled()) {
                (void)pm_->toggle_pattern(i);
            }
        }
        scanner_->refresh_patterns();
        refresh_list();
    };
}

PatternManagerView::~PatternManagerView() noexcept {
    // on_hide() already handles: unregister_handlers(), spectrum_streaming_stop(), baseband::shutdown()
    // Destructor is called after on_hide() during nav_.pop(). Just reset state.
    view_state_ = ViewState::IDLE;
    capture_active_ = false;
}

// ============================================================================
// Handler lifecycle — register on show, unregister on hide
// ============================================================================

void PatternManagerView::register_handlers() noexcept {
    if (handlers_active_) return;

    auto* const h = reinterpret_cast<HandlerStorage*>(handler_storage_);

    new (&h->spectrum_config) MessageHandlerRegistration{
        Message::ID::ChannelSpectrumConfig,
        [this](Message* const p) {
            const auto message = *reinterpret_cast<const ChannelSpectrumConfigMessage*>(p);
            if (message.fifo != nullptr) {
                this->on_channel_spectrum_config(message.fifo);
            }
        },
    };

    new (&h->frame_sync) MessageHandlerRegistration{
        Message::ID::DisplayFrameSync,
        [this](Message* const) {
            this->on_frame_sync();
        },
    };

    handlers_active_ = true;
}

void PatternManagerView::unregister_handlers() noexcept {
    if (!handlers_active_) return;

    auto* const h = reinterpret_cast<HandlerStorage*>(handler_storage_);
    h->spectrum_config.~MessageHandlerRegistration();
    h->frame_sync.~MessageHandlerRegistration();

    handlers_active_ = false;
}

// ============================================================================
// Lifecycle — on_show/on_hide handler registration
// ============================================================================

void PatternManagerView::on_show() {
    scanner_ = get_scanner_ptr();
    if (scanner_ == nullptr) {
        label_status_.set("Scanner not ready");
        set_dirty();
        return;
    }

    pm_ = &scanner_->get_pattern_manager();
    scanner_config_ = scanner_->get_config();

    if (pm_->reload_patterns() != ErrorCode::SUCCESS) {
        label_status_.set("Load failed");
    }

    capture_completed_ = false;
    capture_active_ = false;
    selected_bin_ = -1;

    init_sweep_range();
    refresh_list();

    // Register handlers BEFORE enabling baseband (prevents missed FIFO config)
    register_handlers();

    // Start baseband for live spectrum display
    portapack::receiver_model.set_sampling_rate(SWEEP_SLICE_BW);
    portapack::receiver_model.set_baseband_bandwidth(SWEEP_SLICE_BW);
    baseband::set_spectrum(SWEEP_SLICE_BW, SWEEP_FFT_TRIGGER);

    view_state_ = ViewState::LIVE;
    current_sweep_freq_ = sweep_start_;
    radio::set_tuning_frequency(rf::Frequency(current_sweep_freq_));
    chThdSleepMilliseconds(5);
    baseband::spectrum_streaming_start();

    set_dirty();
}

void PatternManagerView::on_hide() {
    // Stop baseband streaming if we started it
    if (view_state_ == ViewState::LIVE || view_state_ == ViewState::CAPTURING) {
        baseband::spectrum_streaming_stop();
    }
    view_state_ = ViewState::IDLE;
    capture_active_ = false;

    // Unregister handlers — prevents stale lambda calls
    unregister_handlers();

    // CRITICAL: shutdown baseband to reset baseband_image_running flag.
    // Without this, DroneScannerUI::on_show() calling run_image() panics
    // with "BBRunning" because the flag is still true.
    baseband::shutdown();
}

void PatternManagerView::focus() {
    button_capture_.focus();
}

// ============================================================================
// Touch handling — tap spectrum area to select bin
// ============================================================================

bool PatternManagerView::on_touch(const ui::TouchEvent event) noexcept {
    if (event.type != ui::TouchEvent::Type::Start) return false;

    const int16_t x = event.point.x();
    const int16_t y = event.point.y();

    if (x >= SPECTRUM_X && x < SPECTRUM_X + SPECTRUM_WIDTH &&
        y >= SPECTRUM_Y && y < SPECTRUM_Y + SPECTRUM_HEIGHT) {
        const int16_t bin = static_cast<int16_t>(
            (x - SPECTRUM_X) * FFT_BIN_COUNT / SPECTRUM_WIDTH);
        if (bin >= 0 && bin < static_cast<int16_t>(FFT_BIN_COUNT)) {
            on_bin_selected(bin);
            return true;
        }
    }
    return false;
}

// ============================================================================
// Spectrum display — render with red highlight on selected bin
// ============================================================================

void PatternManagerView::paint(ui::Painter& painter) noexcept {
    if (view_state_ == ViewState::CAPTURING || view_state_ == ViewState::LIVE || capture_completed_) {
        draw_spectrum(painter, capture_spectrum_, selected_bin_);
    }
}

void PatternManagerView::draw_spectrum(
    ui::Painter& painter,
    const uint8_t* spectrum,
    int16_t sel_bin) noexcept {
    // Stack: ~16 bytes (locals only)
    painter.fill_rectangle({SPECTRUM_X, SPECTRUM_Y, SPECTRUM_WIDTH, SPECTRUM_HEIGHT}, Color::black());

    if (spectrum == nullptr) return;

    constexpr uint8_t max_display = 180;

    for (size_t i = 0; i < SPECTRUM_WIDTH; ++i) {
        const uint8_t val = (i < FFT_BIN_COUNT) ? spectrum[i] : 0;
        const uint16_t bar_height = static_cast<uint16_t>((val * SPECTRUM_HEIGHT) / max_display);

        Color bar_color = Color::green();
        if (val > 150) bar_color = Color::yellow();
        else if (val > 100) bar_color = Color::cyan();

        // Highlight selected bin range in red
        if (sel_bin >= 0 && i >= static_cast<size_t>(sel_bin - 2) && i <= static_cast<size_t>(sel_bin + 2)) {
            bar_color = Color::red();
        }

        painter.fill_rectangle(
            {static_cast<uint16_t>(SPECTRUM_X + i),
             static_cast<uint16_t>(SPECTRUM_Y + SPECTRUM_HEIGHT - bar_height),
             1, bar_height},
            bar_color);
    }

    // Draw selection frame
    if (sel_bin >= 0 && sel_bin < static_cast<int16_t>(FFT_BIN_COUNT)) {
        painter.draw_rectangle(
            {static_cast<uint16_t>(sel_bin - 2 + SPECTRUM_X),
             SPECTRUM_Y, 5, SPECTRUM_HEIGHT},
            Color::red());
    }
}

// ============================================================================
// Message handlers — baseband spectrum data
// ============================================================================

void PatternManagerView::on_channel_spectrum_config(ChannelSpectrumFIFO* fifo) noexcept {
    spectrum_fifo_ = fifo;
}

void PatternManagerView::on_frame_sync() noexcept {
    if (spectrum_fifo_ == nullptr) return;
    if (view_state_ == ViewState::IDLE && !capture_active_) return;

    ChannelSpectrum& spectrum = spectrum_buffer_;
    if (!spectrum_fifo_->out(spectrum)) return;

    // Copy to display buffer
    for (size_t i = 0; i < FFT_BIN_COUNT && i < spectrum.db.size(); ++i) {
        capture_spectrum_[i] = spectrum.db[i];
    }

    // LIVE mode: step frequency for next frame
    if (view_state_ == ViewState::LIVE && sweep_start_ > 0) {
        current_sweep_freq_ = (current_sweep_freq_ < sweep_end_)
            ? current_sweep_freq_ + sweep_step_
            : sweep_start_;
        radio::set_tuning_frequency(rf::Frequency(current_sweep_freq_));
    }

    // Single-frame capture mode
    if (capture_active_) {
        capture_active_ = false;
        capture_completed_ = true;
        view_state_ = ViewState::IDLE;

        char buf[32];
        snprintf(buf, sizeof(buf), "Done! Bin:%d", static_cast<int>(selected_bin_));
        label_status_.set(buf);
    }

    set_dirty();
}

// ============================================================================
// Sweep range initialization from scanner config
// ============================================================================

void PatternManagerView::init_sweep_range() noexcept {
    // Use sweep window 1 from scanner config (always enabled)
    sweep_start_ = scanner_config_.sweep_start_freq;
    sweep_end_ = scanner_config_.sweep_end_freq;
    sweep_step_ = scanner_config_.sweep_step_freq;

    if (sweep_start_ == 0 || sweep_end_ <= sweep_start_) {
        // Fallback: use 2.4 GHz WiFi band
        sweep_start_ = 2'400'000'000ULL;
        sweep_end_ = 2'500'000'000ULL;
        sweep_step_ = static_cast<FreqHz>(SWEEP_BINS_PER_STEP) * SWEEP_BIN_SIZE;
    }

    if (sweep_step_ == 0) {
        sweep_step_ = static_cast<FreqHz>(SWEEP_BINS_PER_STEP) * SWEEP_BIN_SIZE;
    }

    // Set frequency field from center of sweep range
    const FreqHz center_mhz = (sweep_start_ + (sweep_end_ - sweep_start_) / 2) / 1000000ULL;
    field_freq_mhz_.set_value(static_cast<int32_t>(center_mhz));
}

FreqHz PatternManagerView::bin_to_frequency(int16_t bin) const noexcept {
    if (sweep_start_ == 0 || sweep_end_ == 0) return 0;
    const FreqHz center = sweep_start_ + (sweep_end_ - sweep_start_) / 2;
    const FreqHz bin_step = (sweep_end_ - sweep_start_) / SWEEP_PIXELS_PER_SLICE;
    const FreqHz offset = static_cast<FreqHz>(bin - 128) * bin_step;
    return center + offset;
}

// ============================================================================
// Bin selection — from touch or auto-detect peak
// ============================================================================

void PatternManagerView::on_bin_selected(int16_t bin) noexcept {
    selected_bin_ = bin;
    capture_freq_ = bin_to_frequency(bin);

    char buf[32];
    const uint32_t mhz = static_cast<uint32_t>(capture_freq_ / 1000000);
    const uint32_t khz = static_cast<uint32_t>((capture_freq_ % 1000000) / 1000);
    snprintf(buf, sizeof(buf), "Bin:%d %lu.%03luMHz", static_cast<int>(bin),
             static_cast<unsigned long>(mhz), static_cast<unsigned long>(khz));
    label_status_.set(buf);
    set_dirty();
}

// ============================================================================
// Capture — single-frame FFT capture
// ============================================================================

void PatternManagerView::start_capture() noexcept {
    if (selected_bin_ < 0) {
        label_status_.set("Select bin first!");
        set_dirty();
        return;
    }

    capture_freq_ = bin_to_frequency(selected_bin_);
    label_status_.set("Capturing...");
    view_state_ = ViewState::CAPTURING;
    capture_active_ = true;
    capture_completed_ = false;

    // Tune to selected frequency
    radio::set_tuning_frequency(rf::Frequency(capture_freq_));
    chThdSleepMilliseconds(5);

    set_dirty();
}

// ============================================================================
// Save — normalize FFT, extract features, save to SD
// Stack: ~288 bytes (sort_buf 256B + locals 32B)
// ============================================================================

void PatternManagerView::capture_and_save() noexcept {
    if (pm_ == nullptr || scanner_ == nullptr) {
        label_status_.set("No scanner");
        set_dirty();
        return;
    }

    if (!capture_completed_ || selected_bin_ < 0) {
        label_status_.set("Run Capt first");
        set_dirty();
        return;
    }

    if (pm_->get_pattern_count() >= MAX_PATTERNS) {
        label_status_.set("Max patterns");
        set_dirty();
        return;
    }

    SignalPattern pattern{};

    // Normalize 256-bin FFT to 16-bin waveform
    PatternMatcher::normalize(capture_spectrum_, pattern.waveform);

    // Extract peak features for auto-threshold
    uint8_t sort_buf[FFT_BIN_COUNT];  // 256 bytes on stack
    const PeakDetector::PeakInfo peak = PeakDetector::find(
        capture_spectrum_, sort_buf,
        PeakDetector::Range::Full, PeakDetector::EdgePolicy::Wide);

    pattern.features.peak_position = static_cast<uint8_t>(peak.index / PATTERN_BIN_SCALE_FACTOR);
    pattern.features.peak_value = peak.value;
    pattern.features.noise_floor = peak.noise_floor;
    pattern.features.margin = peak.margin;

    // Auto-tune threshold from SNR margin
    constexpr uint16_t MIN_AUTO_THRESHOLD = 400;
    constexpr uint16_t MAX_AUTO_THRESHOLD = 800;
    const uint16_t auto_th = static_cast<uint16_t>(
        MIN_AUTO_THRESHOLD + static_cast<uint16_t>(peak.margin) * 10U);
    pattern.match_threshold = (auto_th > MAX_AUTO_THRESHOLD) ? MAX_AUTO_THRESHOLD : auto_th;

    // Auto-generate name
    const size_t count = pm_->get_pattern_count();
    snprintf(pattern.name, sizeof(pattern.name), "PTR_%zu", count + 1);

    pattern.center_freq = capture_freq_;
    pattern.range_width = (sweep_end_ > sweep_start_) ? (sweep_end_ - sweep_start_) : SWEEP_SLICE_BW;
    pattern.flags = SignalPattern::Flags::ENABLED;

    const ErrorCode err = pm_->save_pattern(pattern);
    if (err == ErrorCode::SUCCESS) {
        scanner_->refresh_patterns();
        refresh_list();
        capture_completed_ = false;
        selected_bin_ = -1;
        label_status_.set("Saved!");
    } else if (err == ErrorCode::BUFFER_FULL) {
        label_status_.set("Max patterns");
    } else {
        label_status_.set("Save failed");
    }
    set_dirty();
}

// ============================================================================
// Delete — remove selected pattern
// ============================================================================

void PatternManagerView::delete_selected() noexcept {
    if (pm_ == nullptr || scanner_ == nullptr) return;
    if (selected_index_ >= pm_->get_pattern_count()) return;

    const ErrorCode err = pm_->delete_pattern(selected_index_);
    if (err == ErrorCode::SUCCESS) {
        scanner_->refresh_patterns();
        refresh_list();
        label_status_.set("Deleted");
    } else {
        label_status_.set("Delete failed");
    }
    set_dirty();
}

// ============================================================================
// Toggle — enable/disable selected pattern
// ============================================================================

void PatternManagerView::toggle_enabled() noexcept {
    if (pm_ == nullptr || scanner_ == nullptr) return;
    if (selected_index_ >= pm_->get_pattern_count()) return;

    const ErrorCode err = pm_->toggle_pattern(selected_index_);
    if (err == ErrorCode::SUCCESS) {
        scanner_->refresh_patterns();
        refresh_list();
    } else {
        label_status_.set("Toggle failed");
        set_dirty();
    }
}

// ============================================================================
// Pattern list — stack-safe refresh (64 bytes max per iteration)
// ============================================================================

void PatternManagerView::refresh_list() noexcept {
    if (pm_ == nullptr) {
        label_status_.set("No pattern mgr");
        set_dirty();
        return;
    }

    constexpr size_t MAX_OPTS = MAX_PATTERNS + 1;
    ui::OptionsField::option_t opts[MAX_OPTS];  // ~44 bytes on stack
    char text_buf[64]{};                         // 64 bytes on stack
    size_t count = 0;

    const size_t pattern_count = pm_->get_pattern_count();
    for (size_t i = 0; i < pattern_count && i < MAX_PATTERNS && count < MAX_OPTS - 1; ++i) {
        const SignalPattern* p = pm_->get_pattern(i);
        if (p == nullptr) continue;

        const char* state = p->is_enabled() ? "+" : "-";
        snprintf(text_buf, sizeof(text_buf), "[%s] %.20s", state, p->name);
        opts[count] = {text_buf, static_cast<int32_t>(i)};
        ++count;
    }

    if (count == 0) {
        snprintf(text_buf, sizeof(text_buf), "No patterns");
        opts[0] = {text_buf, 0};
        count = 1;
    }

    field_patterns_.set_options({opts, opts + count});

    // Update status with count
    char status[32];
    snprintf(status, sizeof(status), "%zu pattern(s)", pattern_count);
    label_status_.set(status);
    set_dirty();
}

}  // namespace drone_analyzer
