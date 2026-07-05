#include <cstdint>
#include <cstring>
#include <array>
#include <new>

#include "ch.h"

#include "ui.hpp"
#include "ui_fileman.hpp"
#include "scanner.hpp"
#include "scanner_thread.hpp"
#include "drone_settings.hpp"
#include "settings_manager.hpp"
#include "drone_sweep_view.hpp"
#include "pattern_manager_view.hpp"
#include "pattern_matcher.hpp"
#include "pattern_manager.hpp"
#include "peak_detector.hpp"
#include "database.hpp"
#include "hardware_controller.hpp"
#include "audio_alerts.hpp"
#include "audio.hpp"
#include "constants.hpp"
#include "string_format.hpp"
#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"
#include "portapack.hpp"
#include "file_path.hpp"
#include "radio.hpp"

#include "drone_scanner_ui.hpp"

namespace drone_analyzer {

static HardwareController s_hardware;
static DatabaseManager s_database;
static DroneScanner s_scanner(s_database, s_hardware);
static ScannerThread s_scanner_thread(s_scanner);

// ============================================================================
// Handler registration — placement-new to allow manual lifetime control.
// ============================================================================
void DroneScannerUI::register_handlers() noexcept {
    if (handlers_active_) return;

    auto* const h = reinterpret_cast<HandlerStorage*>(handler_storage_);

    new (&h->spectrum_config) MessageHandlerRegistration{
        Message::ID::ChannelSpectrumConfig,
        [this](Message* const p) {
            const auto message = *reinterpret_cast<const ChannelSpectrumConfigMessage*>(p);
            this->spectrum_fifo_ = message.fifo;
        }
    };
    new (&h->frame_sync) MessageHandlerRegistration{
        Message::ID::DisplayFrameSync,
        [this](Message* const) {
            if (!this->scanning_) return;
            if (this->spectrum_fifo_ != nullptr) {
                // Use class member buffer instead of local stack to prevent M0 overflow
                ChannelSpectrum& spectrum = this->spectrum_buffer_;
                if (this->spectrum_fifo_->out(spectrum)) {
                    if (this->composite_active_) {
                        this->on_sweep_spectrum(spectrum);
                    } else {
                        this->on_channel_spectrum(spectrum);
                        this->db_scan_count_++;
                        if (this->db_scan_count_ >= this->DB_SCANS_PER_SWEEP) {
                            this->db_scan_count_ = 0;
                            this->sweep_auto_mode_ = true;
                            this->enter_sweep_mode();
                        }
                    }
                }
            }
            this->refresh_ui();
        }
    };
    new (&h->retune) MessageHandlerRegistration{
        Message::ID::Retune,
        [this](Message* const p) {
            const auto message = *reinterpret_cast<const RetuneMessage*>(p);
            this->on_retune(message.freq, message.range);
        }
    };
    new (&h->channel_stats) MessageHandlerRegistration{
        Message::ID::ChannelStatistics,
        [this](Message* const p) {
            const auto message = *reinterpret_cast<const ChannelStatisticsMessage*>(p);
            this->latest_max_db_ = message.statistics.max_db;
        }
    };

    handlers_active_ = true;
}

void DroneScannerUI::unregister_handlers() noexcept {
    if (!handlers_active_) return;

    auto* const h = reinterpret_cast<HandlerStorage*>(handler_storage_);
    h->spectrum_config.~MessageHandlerRegistration();
    h->frame_sync.~MessageHandlerRegistration();
    h->retune.~MessageHandlerRegistration();
    h->channel_stats.~MessageHandlerRegistration();

    handlers_active_ = false;
}

// ============================================================================
// Atomic 64-bit frequency access — prevents torn reads on Cortex-M4
// ============================================================================
void DroneScannerUI::set_current_frequency_safe(FreqHz freq) noexcept {
    chSysLock();
    current_frequency_ = freq;
    chSysUnlock();
}

FreqHz DroneScannerUI::get_current_frequency_safe() const noexcept {
    chSysLock();
    const FreqHz freq = current_frequency_;
    chSysUnlock();
    return freq;
}

DroneScannerUI::DroneScannerUI(NavigationView& nav) noexcept
    : View()
    , nav_(nav)
    , big_display_{{BIG_FREQUENCY_X, BIG_FREQUENCY_Y, BIG_FREQUENCY_WIDTH, 52}, 0}
    , sweep_transition_guard_()
    , button_debounce_guard_() {
    add_children({
        &labels_,
        &field_lna_,
        &field_vga_,
        &field_rf_amp_,
        &field_volume_,
        &labels_cyc_,
        &field_rssi_dec_cyc_,
        &big_display_,
        &drone_display_,
        &filter_labels_,
        &field_filter_,
        &button_median_,
        &button_start_stop_,
        &button_mode_,
        &button_load_,
        &button_settings_,
        &button_swp_,
        &button_ptr_
    });

    // Filter callback (Looking Glass style: OFF/MID/HIGH)
    field_filter_.on_change = [this](size_t, int32_t v) {
        min_color_power_ = static_cast<uint8_t>(v);
        drone_display_.set_spectrum_filter(min_color_power_);
    };
    field_filter_.set_by_value(DEFAULT_SPECTRUM_FILTER);

    // Sync RSSI decrease cycles from scanner config to UI field
    // NOTE: spectrum shape params NOT synced here to preserve display_margin_=0 default
    // (display and detection filtering are now separate)
    if (scanner_ptr_ != nullptr) {
        const auto cfg = scanner_ptr_->get_config();  // Stack: ~400B (one-time init, acceptable)
        field_rssi_dec_cyc_.set_value(static_cast<int32_t>(cfg.rssi_decrease_cycles));
    }

    // Median filter toggle (spike rejection on RSSI samples)
    button_median_.on_select = [this](ui::Button&) {
        median_enabled_ = !median_enabled_;
        if (scanner_ptr_ != nullptr) {
            scanner_ptr_->set_median_filter_enabled(median_enabled_);
        }
        button_median_.set_text(median_enabled_ ? "Md+" : "OFF");
        // Persist to SD card so state survives app restart
        SettingsStruct persist_settings;
        if (SettingsFileManager::load(persist_settings) == ErrorCode::SUCCESS) {
            persist_settings.median_enabled = median_enabled_;
            (void)SettingsFileManager::save(scanner_ptr_, persist_settings);
        }
    };

    // Register button callbacks BEFORE any early returns
    // Buttons must always respond, even if init fails
    button_start_stop_.on_select = [this](ui::Button&) {
        if (initialization_failed_ || scanner_ptr_ == nullptr) {
            show_error(ErrorCode::HARDWARE_NOT_INITIALIZED, ERROR_DURATION_MS);
            return;
        }
        // Debounce rapid taps that could cause race conditions
        if (!button_debounce_guard_.try_set()) return;

        if (scanning_) {
            // Stop
            if (composite_active_) {
                // Stop sweep streaming
                baseband::spectrum_streaming_stop();
                scanning_ = false;
                button_start_stop_.set_text("Start");
            } else {
                // Stop normal scanning
                scanner_thread_->set_scanning(false);
                (void)scanner_ptr_->stop_scanning();
                baseband::spectrum_streaming_stop();
                scanning_ = false;
                button_start_stop_.set_text("Start");
            }
        } else {
            // Start
            if (composite_active_) {
                // Start sweep streaming
                baseband::spectrum_streaming_start();
                scanning_ = true;
                button_start_stop_.set_text("Stop");
            } else {
                // Start normal scanning
                baseband::spectrum_streaming_start();
                (void)scanner_ptr_->start_scanning();
                scanner_thread_->set_scanning(true);
                scanning_ = true;
                button_start_stop_.set_text("Stop");
            }
        }
        // Clear debounce guard after handling completes
        button_debounce_guard_.clear();
    };

    button_mode_.on_select = [this](ui::Button&) {
        // Debounce rapid taps that could cause race conditions
        if (!button_debounce_guard_.try_set()) return;

        if (!composite_active_) {
            // Enter manual sweep mode (enter_sweep_mode handles re-entrancy guard)
            this->sweep_auto_mode_ = false;
            this->enter_sweep_mode();
            if (composite_active_) {
                button_mode_.set_text("Sweep");
            }
        } else {
            // Exit sweep mode back to normal DB scanning
            this->exit_sweep_mode();
            button_mode_.set_text("Mode");
        }
        // Clear debounce guard after state transition completes
        button_debounce_guard_.clear();
    };

    button_load_.on_select = [this](ui::Button&) {
        auto open_view = nav_.push<FileLoadView>(".TXT");
        open_view->push_dir(freqman_dir);
        open_view->on_changed = [this](const std::filesystem::path& new_file_path) {
            const bool was_scanning = scanning_;

            // Stop scanning if active
            if (scanning_) {
                scanner_thread_->set_scanning(false);
                (void)scanner_ptr_->stop_scanning();
                baseband::spectrum_streaming_stop();
                scanning_ = false;
                button_start_stop_.set_text("Start");
            }

            // Extract filename stem from path — iterate native() chars directly
            // Avoids path::string() which allocates std::string on heap
            // native() returns const std::u16string& (reference, no copy)
            char filename[32];
            const auto& native_path = new_file_path.native();
            size_t last_slash = 0;
            size_t last_dot = 0;
            for (size_t j = 0; j < native_path.size(); ++j) {
                const char c = static_cast<char>(native_path[j]);
                if (c == '/' || c == '\\') last_slash = j + 1;
                if (c == '.') last_dot = j;
            }
            size_t i = 0;
            const size_t stem_start = last_slash;
            const size_t stem_end = (last_dot > last_slash) ? last_dot : last_slash + 31;
            for (; i < 31 && (stem_start + i) < stem_end && (stem_start + i) < native_path.size(); ++i) {
                filename[i] = static_cast<char>(native_path[stem_start + i]);
            }
            filename[i] = '\0';

            // Set new database file and reload
            database_ptr_->set_database_file(filename);
            scanner_ptr_->clear_tracked_drones();

            const ErrorCode err = database_ptr_->load_frequency_database();
            db_loaded_ = (err == ErrorCode::SUCCESS);
            db_entry_count_ = database_ptr_->get_database_size();

            if (err == ErrorCode::SUCCESS && db_entry_count_ > 0) {
                char status_buf[MAX_TEXT_LENGTH];
                snprintf(status_buf, sizeof(status_buf), "%s (%lu)",
                         filename, (unsigned long)db_entry_count_);
                drone_display_.set_status_text(status_buf);

                char alert_buf[MAX_TEXT_LENGTH];
                snprintf(alert_buf, sizeof(alert_buf), "DB: %s loaded", filename);
                show_alert(alert_buf, 2000);

                // Reset scanner frequency to first entry in new database
                scanner_ptr_->reset_frequency();

                // Resume scanning if it was active before
                if (was_scanning) {
                    baseband::spectrum_streaming_start();
                    (void)scanner_ptr_->start_scanning();
                    scanner_thread_->set_scanning(true);
                    scanning_ = true;
                    button_start_stop_.set_text("Stop");
                }

                // Force UI refresh
                drone_display_.set_dirty();
                set_dirty();
            } else {
                char err_buf[MAX_TEXT_LENGTH];
                snprintf(err_buf, sizeof(err_buf), "Failed: %s (err %d)",
                         filename, static_cast<int>(err));
                show_alert(err_buf, 3000);

                // Show error in status
                drone_display_.set_status_text("DB load error");
                drone_display_.set_dirty();
                set_dirty();
            }
        };
    };

    button_settings_.on_select = [this](ui::Button&) {
        if (initialization_failed_ || scanner_ptr_ == nullptr) {
            show_error(ErrorCode::HARDWARE_NOT_INITIALIZED, ERROR_DURATION_MS);
            return;
        }
        // Remember if scanning was active so on_show() can restore it
        scanning_needs_restore_ = scanning_;
        // Stop scanning before pushing sub-view (matches PTR handler pattern).
        if (scanning_) {
            if (composite_active_) {
                exit_sweep_mode(true);
            } else {
                scanner_thread_->set_scanning(false);
                if (scanner_ptr_ != nullptr) {
                    (void)scanner_ptr_->stop_scanning();
                }
                baseband::spectrum_streaming_stop();
                scanning_ = false;
                button_start_stop_.set_text("Start");
            }
        }
        // Refresh config from scanner (SWP view may have changed sweep settings)
        const auto config = scanner_ptr_->get_config();  // Stack: ~104B (button press, infrequent)
        nav_.push<DroneSettingsView>(config, scanner_ptr_, &drone_display_);
    };

    // SWP button: open sweep settings view
    button_swp_.on_select = [this](ui::Button&) {
        if (initialization_failed_ || scanner_ptr_ == nullptr) {
            show_error(ErrorCode::HARDWARE_NOT_INITIALIZED, ERROR_DURATION_MS);
            return;
        }
        // Remember if scanning was active so on_show() can restore it
        scanning_needs_restore_ = scanning_;
        // Stop scanning before pushing sub-view (matches PTR handler pattern).
        // on_hide() also stops scanning, but doing it here ensures clean state
        // and prevents race conditions with the scanner thread.
        if (scanning_) {
            if (composite_active_) {
                exit_sweep_mode(true);
            } else {
                scanner_thread_->set_scanning(false);
                if (scanner_ptr_ != nullptr) {
                    (void)scanner_ptr_->stop_scanning();
                }
                baseband::spectrum_streaming_stop();
                scanning_ = false;
                button_start_stop_.set_text("Start");
            }
        }
        const auto config = scanner_ptr_->get_config();  // Stack: ~104B (button press, infrequent)
        nav_.push<DroneSweepView>(config, scanner_ptr_);
    };

    // PTR button: open pattern manager view
    button_ptr_.on_select = [this](ui::Button&) {
        if (initialization_failed_ || scanner_ptr_ == nullptr) {
            show_error(ErrorCode::HARDWARE_NOT_INITIALIZED, ERROR_DURATION_MS);
            return;
        }
        // CRITICAL: Unregister message handlers BEFORE pushing PatternManagerView.
        // Both views register handlers for ChannelSpectrumConfig / DisplayFrameSync.
        // Without explicit unregister, MessageHandlerMap hits chDbgPanic("MsgDblReg").
        unregister_handlers();

        // PTR uses baseband_needs_restore_ for full baseband restart, not scanning_needs_restore_
        scanning_needs_restore_ = false;

        // Stop scanning if active
        if (scanning_) {
            if (composite_active_) {
                // suppress_auto_restart: PatternManagerView handles its own streaming state.
                exit_sweep_mode(true);
            } else {
                scanner_thread_->set_scanning(false);
                if (scanner_ptr_ != nullptr) {
                    (void)scanner_ptr_->stop_scanning();
                }
                baseband::spectrum_streaming_stop();
                scanning_ = false;
                button_start_stop_.set_text("Start");
            }
        }

        // Mark baseband for restore when returning from PatternManagerView
        baseband_needs_restore_ = true;
        nav_.push<PatternManagerView>();
    };

    // Hardware initialization (callbacks are already set, safe to early-return)
    construct_objects();

    if (scanner_ptr_ == nullptr) {
        show_error(ErrorCode::INITIALIZATION_FAILED, ERROR_DURATION_MS);
        initialization_failed_ = true;
        return;
    }

    // Database load — SD card is already mounted at boot (portapack.cpp:569)
    const auto db_result = database_ptr_->load_frequency_database();
    db_loaded_ = (db_result == ErrorCode::SUCCESS);
    db_entry_count_ = database_ptr_->get_database_size();
    if (db_result != ErrorCode::SUCCESS) {
        show_error(db_result, ERROR_DURATION_MS);
    }

    const ErrorCode init_err = scanner_ptr_->initialize();
    if (init_err != ErrorCode::SUCCESS) {
        show_error(init_err, ERROR_DURATION_MS);
    }

    baseband::run_image(portapack::spi_flash::image_tag_wideband_spectrum);
    portapack::receiver_model.set_sampling_rate(DEFAULT_SAMPLE_RATE_HZ);
    portapack::receiver_model.set_baseband_bandwidth(DEFAULT_SAMPLE_RATE_HZ);
    baseband::set_spectrum(DEFAULT_SAMPLE_RATE_HZ, SWEEP_FFT_TRIGGER);
    portapack::receiver_model.enable();

    // FPV-OPTIMIZED RF frontend: ensure max sensitivity for long-range analog
    // 5.8 GHz video link detection. Non-destructive: only raises values that
    // are lower than FPV defaults (respects user-set higher values from other apps).
    // Field set_value() calls fire on_change → receiver_model.set_*() → hardware update.
    if (portapack::receiver_model.lna() < DEFAULT_LNA_GAIN) {
        field_lna_.set_value(static_cast<int32_t>(DEFAULT_LNA_GAIN));
    }
    if (portapack::receiver_model.vga() < DEFAULT_VGA_GAIN) {
        field_vga_.set_value(static_cast<int32_t>(DEFAULT_VGA_GAIN));
    }
    if (!portapack::receiver_model.rf_amp()) {
        field_rf_amp_.set_value(1);
    }

    // Set default headphone volume to 70/99 if not already set higher
    if (portapack::receiver_model.normalized_headphone_volume() < 70) {
        portapack::receiver_model.set_normalized_headphone_volume(70);
    }

    // Initialize audio codec for beep playback (WM8731/AK4951)
    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();

    scanner_thread_->start();

    scanning_ = false;

    scanner_ptr_->set_alert_callback([](ThreatLevel level) {
        AudioAlertManager::play_alert(level);
    });

    ScanConfig config;
    config.mode = scanning_mode_;
    config.rssi_threshold_dbm = RSSI_DETECTION_THRESHOLD_DBM;
    config.scan_interval_ms = SCAN_CYCLE_INTERVAL_MS;

    // Load all settings from SD card via centralized manager
    SettingsStruct startup_settings;
    const ErrorCode load_err = SettingsFileManager::load(startup_settings);
    if (load_err == ErrorCode::SUCCESS) {
        SettingsFileManager::apply_to_config(startup_settings, config);
        // Apply volume from settings (not during parsing to avoid side-effects)
        portapack::receiver_model.set_normalized_headphone_volume(startup_settings.volume);
    }

    const ErrorCode config_err = scanner_ptr_->set_config(config);
    if (config_err != ErrorCode::SUCCESS) {
        show_error(config_err, ERROR_DURATION_MS);
    }

    // Sync UI median_enabled from loaded config
    median_enabled_ = config.median_enabled;
    button_median_.set_text(median_enabled_ ? "Md+" : "OFF");
}

DroneScannerUI::~DroneScannerUI() noexcept {
    // Unregister handlers before destroying anything else.
    unregister_handlers();

    // CRITICAL: Exit sweep mode first to stop baseband streaming
    if (composite_active_) {
        exit_sweep_mode();
    }

    destruct_objects();

    audio::output::stop();
    portapack::receiver_model.disable();
    baseband::shutdown();

    // Ensure atomic flags are cleared on destruction
    button_debounce_guard_.clear();
    sweep_transition_guard_.clear();
}

void DroneScannerUI::focus() {
    button_start_stop_.focus();
}

void DroneScannerUI::construct_objects() noexcept {
    hardware_ptr_ = &s_hardware;
    database_ptr_ = &s_database;
    scanner_ptr_ = &s_scanner;
    scanner_thread_ = &s_scanner_thread;
}

void DroneScannerUI::destruct_objects() noexcept {
    if (scanner_thread_ != nullptr) {
        scanner_thread_->stop();
        scanner_thread_ = nullptr;
    }
    scanner_ptr_ = nullptr;
    database_ptr_ = nullptr;
    hardware_ptr_ = nullptr;
}

void DroneScannerUI::on_show() {
    if (baseband_needs_restore_ && !composite_active_) {
        baseband_needs_restore_ = false;
        // CRITICAL: shutdown() first to reset baseband_image_running flag.
        // Sub-views (PatternManagerView) stop streaming but never call shutdown(),
        // leaving baseband_image_running = true. Without shutdown(), run_image()
        // panics with "BBRunning".
        baseband::shutdown();
        baseband::run_image(portapack::spi_flash::image_tag_wideband_spectrum);
        portapack::receiver_model.set_sampling_rate(DEFAULT_SAMPLE_RATE_HZ);
        portapack::receiver_model.set_baseband_bandwidth(DEFAULT_SAMPLE_RATE_HZ);
        baseband::set_spectrum(DEFAULT_SAMPLE_RATE_HZ, SWEEP_FFT_TRIGGER);
        portapack::receiver_model.enable();

        // Restore audio output that may have been muted by a sub-view
        audio::output::unmute();
    }

    // Re-register handlers when becoming visible (after returning from sub-view)
    register_handlers();

    // Refresh patterns from SD — ensures patterns saved via inline save are available for SWEEP
    if (scanner_ptr_ != nullptr) {
        scanner_ptr_->refresh_patterns();
    }

    if (scanner_thread_ != nullptr && !scanner_thread_->is_active()) {
        scanner_thread_->start();
    }

    // Restore scanning after returning from a sub-view.
    // on_hide() stops scanning and sets scanning_ = false.
    // scanning_needs_restore_ remembers if scanning was active before the push.
    // For non-sweep mode: restart streaming and scanning if it was active.
    // For sweep mode: handled by the composite_active_ block below.
    if (!composite_active_ && !initialization_failed_ && scanner_ptr_ != nullptr && scanning_needs_restore_ && !scanning_) {
        scanning_needs_restore_ = false;
        baseband::spectrum_streaming_start();
        (void)scanner_ptr_->start_scanning();
        scanner_thread_->set_scanning(true);
        scanning_ = true;
        button_start_stop_.set_text("Stop");
    }

    // If in sweep mode, reload sweep range from config (Settings may have changed it)
    if (composite_active_ && scanner_ptr_ != nullptr) {
        const auto cfg = scanner_ptr_->get_config();  // Stack: ~400B (view transition, infrequent)

        // Reinit all windows from config
        last_tuned_freq_ = 0;
        skip_next_fft_ = true;
        sweep_[0].init(cfg.sweep_start_freq, cfg.sweep_end_freq, cfg.sweep_step_freq);
        sweep_[0].enabled = true;  // Window 0 always enabled
        sweep_[1].init(cfg.sweep2_start_freq, cfg.sweep2_end_freq, cfg.sweep2_step_freq);
        sweep_[1].enabled = cfg.sweep2_enabled;
        sweep_[2].init(cfg.sweep3_start_freq, cfg.sweep3_end_freq, cfg.sweep3_step_freq);
        sweep_[2].enabled = cfg.sweep3_enabled;
        sweep_[3].init(cfg.sweep4_start_freq, cfg.sweep4_end_freq, cfg.sweep4_step_freq);
        sweep_[3].enabled = cfg.sweep4_enabled;

        // Copy exception frequencies to each sweep window
        const FreqHz on_show_exc_radius = static_cast<FreqHz>(cfg.exception_radius_mhz) * 1000000ULL;
        for (uint8_t w = 0; w < MAX_SWEEP_WINDOWS; ++w) {
            for (uint8_t i = 0; i < EXCEPTIONS_PER_WINDOW; ++i) {
                sweep_[w].exceptions[i] = cfg.sweep_exceptions[w][i];
            }
            sweep_[w].exception_radius_hz = on_show_exc_radius;
        }

        // Find first enabled window
        active_sweep_idx_ = 0;
        for (uint8_t i = 0; i < MAX_SWEEP_WINDOWS; ++i) {
            if (sweep_[i].enabled) {
                active_sweep_idx_ = i;
                break;
            }
        }

        current_pair_ = pair_first(active_sweep_idx_);
        update_sweep_pair_display();

        radio::set_tuning_frequency(rf::Frequency(sweep_[active_sweep_idx_].f_center));
        set_current_frequency_safe(sweep_[active_sweep_idx_].f_center);

        if (!scanning_) {
            baseband::spectrum_streaming_start();
            scanning_ = true;
        }
    }
}

void DroneScannerUI::on_hide() {
    if (scanning_) {
        scanner_thread_->set_scanning(false);
        if (scanner_ptr_ != nullptr) {
            (void)scanner_ptr_->stop_scanning();
        }
        baseband::spectrum_streaming_stop();
        scanning_ = false;
        button_start_stop_.set_text("Start");
    }
    if (scanner_thread_ != nullptr) {
        scanner_thread_->stop();
    }
}

void DroneScannerUI::paint(Painter& painter) {
    (void)painter;
    // Alert/error timer expiry is now handled in refresh_ui() to avoid
    // requiring DroneScannerUI::paint() to be called every frame.
    // This eliminates the need for this->set_dirty() in refresh_ui(),
    // preventing full widget-tree repaints that cause screen flicker.
}

void DroneScannerUI::show_alert(const char* message, uint32_t duration_ms) noexcept {
    if (message == nullptr) return;
    (void)safe_str_copy(alert_message_, MAX_TEXT_LENGTH, message);
    alert_active_ = true;
    alert_start_time_ = chTimeNow();
    alert_duration_ms_ = duration_ms;
}

void DroneScannerUI::show_error(ErrorCode error, uint32_t duration_ms) noexcept {
    error_active_ = true;
    last_error_ = error;
    error_start_time_ = chTimeNow();
    error_duration_ms_ = duration_ms;
}

bool DroneScannerUI::on_touch(const ui::TouchEvent event) {
    return View::on_touch(event);
}

void DroneScannerUI::bigdisplay_update(BigDisplayColor color) noexcept {
    switch (color) {
        case BigDisplayColor::GREY:
            big_display_.set_style(Theme::getInstance()->fg_medium);
            break;
        case BigDisplayColor::YELLOW:
            big_display_.set_style(Theme::getInstance()->fg_yellow);
            break;
        case BigDisplayColor::GREEN:
            big_display_.set_style(Theme::getInstance()->fg_green);
            break;
        case BigDisplayColor::RED:
            big_display_.set_style(Theme::getInstance()->fg_red);
            break;
    }
    
    // BigFrequency::set() only accepts rf::Frequency (numeric), not strings
    // When frequency is 0 (uninitialized), display 0 Hz which formats as "0.000"
    big_display_.set(get_current_frequency_safe());
}

void DroneScannerUI::refresh_ui() noexcept {
    // Check alert/error timer expiry (moved from paint() to avoid full-tree repaint)
    {
        const SystemTime now = chTimeNow();
        if (alert_active_ && (now - alert_start_time_) >= alert_duration_ms_) {
            alert_active_ = false;
            drone_display_.set_dirty();  // Force repaint to clear alert overlay
        }
        if (error_active_ && (now - error_start_time_) >= error_duration_ms_) {
            error_active_ = false;
            drone_display_.set_dirty();
        }
    }

    if (scanner_ptr_ == nullptr || initialization_failed_) {
        current_scanner_state_ = ScannerState::IDLE;
        current_rssi_ = RSSI_NOISE_FLOOR_DBM;
        set_current_frequency_safe(0);
        bigdisplay_update(BigDisplayColor::GREY);
        displayed_drone_type_[0] = '\0';
        drone_type_display_timer_ = 0;
        drone_display_.set_status_text(STATUS_ERROR);
        return;
    }

    current_scanner_state_ = scanner_ptr_->get_state();

    // Rate-limit stale drone removal to 1Hz (was 60Hz — unnecessary churn)
    {
        static SystemTime last_stale_check = 0;
        constexpr uint32_t STALE_CHECK_INTERVAL_MS = 1000;
        const SystemTime now = chTimeNow();
        if ((now - last_stale_check) >= STALE_CHECK_INTERVAL_MS) {
            scanner_ptr_->remove_stale_drones(now);
            last_stale_check = now;
        }
    }

    // Build display data from tracked drones
    // Use single get_tracked_drones call for consistency (one lock, one snapshot)
    refresh_display_data_.clear();

    {
        const size_t count = scanner_ptr_->get_tracked_drones(refresh_drones_, MAX_DISPLAYED_DRONES);
        refresh_display_data_.drone_count = count;
        for (size_t i = 0; i < count; ++i) {
            refresh_display_data_.drones[i] = DisplayDroneEntry(refresh_drones_[i]);
        }
        // Clear stale entries beyond current count
        for (size_t i = count; i < MAX_DISPLAYED_DRONES; ++i) {
            refresh_display_data_.drones[i] = DisplayDroneEntry();
        }

        // Sort by threat level descending (CRITICAL first, NONE last)
        // Simple insertion sort — O(n²) but n ≤ MAX_DISPLAYED_DRONES (small)
        for (size_t i = 1; i < count; ++i) {
            const DisplayDroneEntry key = refresh_display_data_.drones[i];
            size_t j = i;
            while (j > 0 && refresh_display_data_.drones[j - 1].threat < key.threat) {
                refresh_display_data_.drones[j] = refresh_display_data_.drones[j - 1];
                --j;
            }
            refresh_display_data_.drones[j] = key;
        }
    }

    // Update status text based on state and database status
    // Always show database entry count so user can verify DB changed after Load

    if (!db_loaded_) {
        drone_display_.set_status_text("No DB");
    } else if (db_entry_count_ == 0) {
        drone_display_.set_status_text("DB empty");
    } else {
        switch (current_scanner_state_) {
            case ScannerState::SCANNING:
                snprintf(refresh_status_buf_, sizeof(refresh_status_buf_), "Scan (%lu)", (unsigned long)db_entry_count_);
                drone_display_.set_status_text(refresh_status_buf_);
                break;
            case ScannerState::LOCKING:
                snprintf(refresh_status_buf_, sizeof(refresh_status_buf_), "Lock (%lu)", (unsigned long)db_entry_count_);
                drone_display_.set_status_text(refresh_status_buf_);
                break;
            case ScannerState::TRACKING:
                snprintf(refresh_status_buf_, sizeof(refresh_status_buf_), "Track %lu (%lu)",
                         (unsigned long)refresh_display_data_.drone_count, (unsigned long)db_entry_count_);
                drone_display_.set_status_text(refresh_status_buf_);
                break;
            case ScannerState::PAUSED:
                snprintf(refresh_status_buf_, sizeof(refresh_status_buf_), "Pause (%lu)", (unsigned long)db_entry_count_);
                drone_display_.set_status_text(refresh_status_buf_);
                break;
            default:
                snprintf(refresh_status_buf_, sizeof(refresh_status_buf_), "Ready (%lu)", (unsigned long)db_entry_count_);
                drone_display_.set_status_text(refresh_status_buf_);
                break;
        }
    }

    drone_display_.update_display_data(refresh_display_data_);

    // Stop looping SOS alert if no HIGH/CRITICAL threats remain
    // (MEDIUM uses one-shot alerts — don't kill those)
    if (AudioAlertManager::is_sos_looping()) {
        bool has_high_threat = false;
        for (size_t i = 0; i < refresh_display_data_.drone_count; ++i) {
            if (refresh_display_data_.drones[i].threat >= ThreatLevel::HIGH) {
                has_high_threat = true;
                break;
            }
        }
        if (!has_high_threat) {
            AudioAlertManager::stop_alert();
        }
    }

    // Feed histogram data
    // Normal mode: from scanner histogram processor
    // Sweep mode: from composite buffer (live power distribution)
    if (!composite_active_) {
        const size_t hist_count = scanner_ptr_->get_histogram_snapshot(
            refresh_hist_data_, HISTOGRAM_BUFFER_SIZE
        );
        if (hist_count > 0) {
            drone_display_.set_histogram_data(refresh_hist_data_, hist_count);
        }
    } else {
        // Feed displayed pair's composite power levels as histogram bins
        const uint8_t w0 = pair_first(active_sweep_idx_);
        const uint8_t w1 = w0 + 1;
        size_t hist_idx = 0;

        if (sweep_[w0].enabled) {
            const size_t n0 = (COMPOSITE_SIZE < HISTOGRAM_BUFFER_SIZE)
                ? COMPOSITE_SIZE : HISTOGRAM_BUFFER_SIZE;
            for (size_t i = 0; i < n0 && hist_idx < HISTOGRAM_BUFFER_SIZE; ++i) {
                refresh_hist_data_[hist_idx++] = static_cast<uint16_t>(sweep_[w0].composite[i]) * COMPOSITE_TO_HIST_SCALE;
            }
        }
        if (w1 < MAX_SWEEP_WINDOWS && sweep_[w1].enabled) {
            for (size_t i = 0; i < COMPOSITE_SIZE && hist_idx < HISTOGRAM_BUFFER_SIZE; ++i) {
                refresh_hist_data_[hist_idx++] = static_cast<uint16_t>(sweep_[w1].composite[i]) * COMPOSITE_TO_HIST_SCALE;
            }
        }
        if (hist_idx > 0) {
            drone_display_.set_histogram_data(refresh_hist_data_, hist_idx);
        }
    }

    // Update big frequency display
    {
        RssiValue drone_rssi = RSSI_NOISE_FLOOR_DBM;
        if (refresh_display_data_.drone_count > 0) {
            drone_rssi = refresh_display_data_.drones[0].rssi;
        }
        current_rssi_ = drone_rssi;

        BigDisplayColor color = BigDisplayColor::GREY;
        switch (current_scanner_state_) {
            case ScannerState::LOCKING:
                color = BigDisplayColor::YELLOW;
                break;
            case ScannerState::TRACKING: {
                const int32_t critical_threshold = (scanner_ptr_ != nullptr)
                    ? scanner_ptr_->get_threat_critical_dbm()
                    : RSSI_CRITICAL_THREAT_THRESHOLD_DBM;
                color = (current_rssi_ >= critical_threshold)
                      ? BigDisplayColor::RED
                      : BigDisplayColor::GREEN;
                break;
            }
            default:
                color = BigDisplayColor::GREY;
                break;
        }
        bigdisplay_update(color);
    }

    if (current_scanner_state_ == ScannerState::LOCKING) {
        char drone_type[5]{'\0', '\0', '\0', '\0'};
        ErrorCode err = scanner_ptr_->get_current_drone_type(drone_type, 5);
        if (err == ErrorCode::SUCCESS && drone_type[0] != '\0') {
            (void)safe_str_copy(displayed_drone_type_, MAX_DRONE_TYPE_DISPLAY, drone_type);
            drone_type_display_timer_ = chTimeNow();
        }
    } else {
        displayed_drone_type_[0] = '\0';
        drone_type_display_timer_ = 0;
    }

    // Drive SOS/multi-beep pattern at ~60Hz
    AudioAlertManager::update();

    // REMOVED: this->set_dirty() — was forcing ALL 17 children to repaint
    // every frame, causing screen flicker. Each widget now only repaints
    // when its own set_dirty() is called (drone_display_, big_display_).
}

void DroneScannerUI::on_retune(FreqHz freq, uint32_t range) noexcept {
    (void)range;
    set_current_frequency_safe(freq);
}

void DroneScannerUI::on_channel_spectrum(const ChannelSpectrum& spectrum) noexcept {
    if (scanner_ptr_ != nullptr && scanning_) {
        // Use frequency from on_retune() — this matches the hardware tuning
        // when this spectrum was captured. Do NOT read scanner's current_frequency_
        // because the scanner thread may have already moved to the next frequency.
        const FreqHz freq = get_current_frequency_safe();
        if (freq != 0) {
            (void)scanner_ptr_->process_spectrum_message(spectrum, freq);
        }

        // Feed spectrum to DroneDisplay for visualization only when scanning
        drone_display_.set_spectrum_data(spectrum.db.data(), spectrum.db.size());
        drone_display_.set_dirty();
    }
}

void DroneScannerUI::enter_sweep_mode() noexcept {
    // Prevent re-entrant entry (double-tap on Mode button)
    if (composite_active_) return;
    if (!sweep_transition_guard_.try_set()) return;

    composite_active_ = true;
    last_tuned_freq_ = 0;
    skip_next_fft_ = true;  // first FFT after entry may be stale
    drone_display_.set_composite_mode(true);
    // Auto-enable noise floor filtering for composite display.
    // This suppresses the visible noise baseline without affecting the data.
    // Using SWEEP_DISPLAY_NOISE_MARGIN so only signals above noise + margin draw color.
    // The composite noise floor itself is computed automatically in set_composite_data().
    drone_display_.set_spectrum_filter(SWEEP_DISPLAY_NOISE_MARGIN);
    // Start with a clean EMA state and hidden scan heads.
    drone_display_.reset_composite_persistence();
    drone_display_.set_scan_head(-1, -1);

    const auto cfg = (scanner_ptr_ != nullptr)
        ? scanner_ptr_->get_config()   // Stack: ~400B (mode switch, infrequent)
        : ScanConfig();

    // Initialize all 4 sweep windows from config
    sweep_[0].init(cfg.sweep_start_freq, cfg.sweep_end_freq, cfg.sweep_step_freq);
    sweep_[0].enabled = true;  // Window 0 always enabled
    sweep_[1].init(cfg.sweep2_start_freq, cfg.sweep2_end_freq, cfg.sweep2_step_freq);
    sweep_[1].enabled = cfg.sweep2_enabled;
    sweep_[2].init(cfg.sweep3_start_freq, cfg.sweep3_end_freq, cfg.sweep3_step_freq);
    sweep_[2].enabled = cfg.sweep3_enabled;
    sweep_[3].init(cfg.sweep4_start_freq, cfg.sweep4_end_freq, cfg.sweep4_step_freq);
    sweep_[3].enabled = cfg.sweep4_enabled;

    // Copy exception frequencies to each sweep window
    const FreqHz exc_radius_hz = static_cast<FreqHz>(cfg.exception_radius_mhz) * 1000000ULL;
    for (uint8_t w = 0; w < MAX_SWEEP_WINDOWS; ++w) {
        for (uint8_t i = 0; i < EXCEPTIONS_PER_WINDOW; ++i) {
            sweep_[w].exceptions[i] = cfg.sweep_exceptions[w][i];
        }
        sweep_[w].exception_radius_hz = exc_radius_hz;
    }

    // Find first enabled window for round-robin
    active_sweep_idx_ = MAX_SWEEP_WINDOWS;  // invalid sentinel
    for (uint8_t i = 0; i < MAX_SWEEP_WINDOWS; ++i) {
        if (sweep_[i].enabled) {
            active_sweep_idx_ = i;
            break;
        }
    }
    if (active_sweep_idx_ >= MAX_SWEEP_WINDOWS) {
        // No enabled windows — abort sweep, return to normal mode
        composite_active_ = false;
        drone_display_.set_composite_mode(false);
        sweep_transition_guard_.clear();
        return;
    }

    // Initialize pair tracking (pairs: 0=[w0,w1], 2=[w2,w3])
    current_pair_ = pair_first(active_sweep_idx_);

    // Save DB index BEFORE stopping scanner, then derive frequency from DB entry.
    // This ensures last_db_frequency_ == entries[last_db_index_].frequency,
    // so get_next_frequency() finds the exact resume point after sweep.
    if (database_ptr_ != nullptr) {
        last_db_index_ = database_ptr_->get_current_index();
        // Use current frequency from scanner (thread-safe via atomic read)
        // This is the frequency the scanner was tuned to before sweep entry
        const FreqHz cur_freq = get_current_frequency_safe();
        if (cur_freq != 0) {
            last_db_frequency_ = cur_freq;
        } else {
            // Fallback: try to get from scanner's locked frequency
            last_db_frequency_ = scanner_ptr_->get_locked_frequency();
        }
    }

    // Stop scanner thread FIRST — UI drives tuning during sweep
    if (scanner_thread_ != nullptr) {
        scanner_thread_->set_scanning(false);
        scanner_thread_->reset_dwell();
    }
    if (scanner_ptr_ != nullptr) {
        (void)scanner_ptr_->stop_scanning();
        // Clear lock state to prevent stale LOCKING/TRACKING after resume
        scanner_ptr_->clear_lock_state();
    }
    scanning_ = false;

    // Configure baseband for sweep bandwidth
    portapack::receiver_model.set_sampling_rate(SWEEP_SLICE_BW);
    portapack::receiver_model.set_baseband_bandwidth(SWEEP_SLICE_BW);
    baseband::set_spectrum(SWEEP_SLICE_BW, SWEEP_FFT_TRIGGER);
    spectrum_fifo_ = nullptr;

    // Set up display for current pair (2+2 pagination)
    update_sweep_pair_display();

    radio::set_tuning_frequency(rf::Frequency(sweep_[active_sweep_idx_].f_center));
    // CRITICAL FIX: Wait for PLLs to settle before starting spectrum capture.
    // This is the initial sweep entry - ensures first FFT is valid.
    // Using 5ms to match Looking Glass app delay for consistency.
    chThdSleepMilliseconds(5);
    set_current_frequency_safe(sweep_[active_sweep_idx_].f_center);

    baseband::spectrum_streaming_start();
    scanning_ = true;
    button_start_stop_.set_text("Stop");
    drone_display_.set_dirty();  // ensure sweep display paints on entry

    // Allow exit_sweep_mode() to work after enter completes
    sweep_transition_guard_.clear();
}

void DroneScannerUI::exit_sweep_mode(bool suppress_auto_restart) noexcept {
    // Prevent re-entrant exit — must match enter_sweep_mode() guard pattern
    if (!composite_active_) return;
    if (!sweep_transition_guard_.try_set()) return;

    const bool was_auto = sweep_auto_mode_;
    composite_active_ = false;
    sweep_auto_mode_ = false;
    active_sweep_idx_ = 0;
    current_pair_ = 0;
    sweep_[0].enabled = false;
    sweep_[1].enabled = false;
    sweep_[2].enabled = false;
    sweep_[3].enabled = false;
    drone_display_.set_composite_mode(false);
    drone_display_.set_dual_sweep_mode(false);
    // Restore normal display filter when returning from sweep mode.
    // Without this, the sweep noise floor threshold would persist in normal mode,
    // causing weak signals to be invisible in non-sweep view.
    drone_display_.set_spectrum_filter(DEFAULT_SPECTRUM_FILTER);
    // Clean EMA state and hide scan heads so the next sweep entry starts fresh
    // and the non-sweep view does not show leftover markers.
    drone_display_.reset_composite_persistence();
    drone_display_.set_scan_head(-1, -1);

    // Stop streaming BEFORE nulling fifo — prevents stale data in frame_sync
    if (scanning_) {
        baseband::spectrum_streaming_stop();
        scanning_ = false;
    }
    spectrum_fifo_ = nullptr;
    db_scan_count_ = 0;
    button_start_stop_.set_text("Start");

    // Restore baseband to normal bandwidth immediately
    portapack::receiver_model.set_sampling_rate(DEFAULT_SAMPLE_RATE_HZ);
    portapack::receiver_model.set_baseband_bandwidth(DEFAULT_SAMPLE_RATE_HZ);
    baseband::set_spectrum(DEFAULT_SAMPLE_RATE_HZ, SWEEP_FFT_TRIGGER);

    // Only auto-restart scanning if NOT being called from view transition.
    // When suppress_auto_restart=true (video or pattern manager push),
    // the streaming restart would be immediately stopped by the caller,
    // creating a send_message() race that can panic "Baseband Send Fail".
    if (!suppress_auto_restart && was_auto && scanner_ptr_ != nullptr) {
        // Continue scanning from last DB position (skip already-scanned)
        // Restore both frequency AND database index for exact resume
        if (last_db_frequency_ != 0) {
            scanner_ptr_->set_scan_frequency(last_db_frequency_);
        }
        if (database_ptr_ != nullptr) {
            database_ptr_->set_current_index(last_db_index_);
        }

        if (scanner_thread_ != nullptr) {
            scanner_thread_->set_scanning(true);
        }
        (void)scanner_ptr_->start_scanning();
        baseband::spectrum_streaming_start();
        scanning_ = true;
        button_start_stop_.set_text("Stop");
    }

    sweep_transition_guard_.clear();
}

void DroneScannerUI::on_sweep_spectrum(const ChannelSpectrum& spectrum) noexcept {
    // Bounds check — prevent out-of-bounds access if state corrupted
    if (active_sweep_idx_ >= MAX_SWEEP_WINDOWS) {
        baseband::spectrum_streaming_stop();
        return;
    }

    baseband::spectrum_streaming_stop();
    auto& win = sweep_[active_sweep_idx_];
    win.process_bins(spectrum);

    // Skip first FFT after sweep entry — may be stale from previous mode
    // Restart retune at same frequency to get a clean FFT
    if (skip_next_fft_) {
        skip_next_fft_ = false;
        retune_sweep_window(win, nullptr);
        return;
    }

    if (scanner_ptr_ != nullptr) {
        // Use the exact frequency the radio was tuned to when this FFT was captured.
        // f_center may have already been incremented from the previous frame's step.
        const FreqHz fft_freq = (last_tuned_freq_ != 0) ? last_tuned_freq_ : win.f_center;

        // Create per-frame Looking Glass reordered view (continuous 240-pixel line).
        // Used by shape analysis (no DC gap corruption) and passed to scanner.
        // Pattern matching uses raw FFT directly (consistent with saved patterns).
        SweepProcessor::reorder_frame(spectrum, lg_frame_buf_);

        // Pass sweep range boundaries to prevent false positives outside the range
        scanner_ptr_->process_spectrum_sweep(spectrum, lg_frame_buf_, fft_freq, win.f_min, win.f_max);

        // Update pattern match highlight (red frame) if pattern matching is enabled
        // FIX: Convert 256-bin FFT index to 240-pixel screen index (HIGH-1)
        // Original bug: Direct cast from bin (0-255) to pixel caused 134-pixel offset error
        if (scanner_ptr_->is_pattern_matched()) {
            const uint8_t bin = scanner_ptr_->get_matched_pattern_bin();
            int16_t pixel = -1;
            if (bin >= FFT_DC_SPIKE_START && bin < FFT_DC_SPIKE_END) {
                pixel = -1;  // DC spike, skip
            } else if (bin >= SWEEP_FFT_MAP_START) {
                // Lower sideband: 134-255 -> 0-121
                pixel = static_cast<int16_t>(bin - SWEEP_FFT_MAP_START);
            } else if (bin < SWEEP_FFT_MAP_CROSSOVER) {
                // Upper sideband: 2-119 -> 118-237 (using UPPER_OFFSET = 118)
                static constexpr uint8_t UPPER_OFFSET = SWEEP_FFT_MAP_CROSSOVER - 2;
                pixel = static_cast<int16_t>(bin + UPPER_OFFSET);
            }
            drone_display_.set_matched_pattern_bin(pixel);
        } else {
            drone_display_.set_matched_pattern_bin(-1);
        }
    }

    // Live display update: show current pair data every frame
    update_sweep_pair_display();
    drone_display_.set_dirty();  // repaint sweep display every frame

    if (win.pixel_index < COMPOSITE_SIZE) {
        // Normal step: advance frequency within current window
        // REVERT (CRITICAL): Use < to prevent infinite stall at f_max.
        // The overshoot is safely rejected by process_spectrum_sweep() frequency range check.
        if (win.f_center < win.f_max) {
            win.f_center += win.step_hz;
            retune_sweep_window(win, nullptr);
            return;
        }
        // Reached end of range — force completion and clear accumulators
        // to prevent stale values from affecting the next sweep pass.
        win.pixel_index = COMPOSITE_SIZE;
        win.bins_hz_acc = 0;
        win.pixel_max = 0;
    }

    // Current window sweep pass complete
    // Check if its pair is fully complete (both enabled windows in the pair done)
    const uint8_t w0 = current_pair_;
    const uint8_t w1 = w0 + 1;
    const bool w0_done = !sweep_[w0].enabled || sweep_[w0].pixel_index >= COMPOSITE_SIZE;
    const bool w1_done = (w1 >= MAX_SWEEP_WINDOWS) || !sweep_[w1].enabled || sweep_[w1].pixel_index >= COMPOSITE_SIZE;
    const bool pair_complete = w0_done && w1_done;

    if (pair_complete) {
        // Recompute noise floor once per sweep pass (not per frame).
        // This replaces the per-frame quickselect that previously ran in
        // set_composite_data() / set_sweep2_data(), saving ~1200 comparisons/frame.
        drone_display_.update_noise_floor();

        // Display final pair data before reset
        update_sweep_pair_display();
        drone_display_.set_dirty();

        // Reset windows in this pair for next scan pass.
        // NOTE: Do NOT call reset_composite_persistence() here — the EMA
        // persistence buffer must survive across passes so the noise floor
        // estimate (15th percentile) improves with each pass. The EMA decay
        // formula (max(raw, old*224/256)) already handles the transition:
        // signal peaks are retained, noise floor decays toward zero.
        if (sweep_[w0].enabled) {
            sweep_[w0].reset();
        }
        if (w1 < MAX_SWEEP_WINDOWS && sweep_[w1].enabled) sweep_[w1].reset();

        // Advance to next pair (pairs: 0=[w0,w1], 2=[w2,w3]).
        // Skip pairs with no enabled windows — otherwise current_pair_ points
        // at an empty pair and update_sweep_pair_display() bails out, freezing
        // the display even though the sweep logic keeps running.
        uint8_t next_pair = (current_pair_ + 2 < MAX_SWEEP_WINDOWS) ? current_pair_ + 2 : 0;
        for (uint8_t skip = 0; skip < MAX_SWEEP_WINDOWS / 2; ++skip) {
            const uint8_t np_w0 = next_pair;
            const uint8_t np_w1 = np_w0 + 1;
            const bool has_enabled = sweep_[np_w0].enabled ||
                (np_w1 < MAX_SWEEP_WINDOWS && sweep_[np_w1].enabled);
            if (has_enabled) break;
            next_pair = (next_pair + 2 < MAX_SWEEP_WINDOWS) ? next_pair + 2 : 0;
        }

        // Full cycle wrap: all pairs have been visited.
        // Exit auto-mode before round-robin changes active_sweep_idx_,
        // but apply SWEEP-AWARE decay first (see wrap detection block below for rationale).
        if (next_pair == 0 && sweep_auto_mode_) {
            // FIX: Apply RSSI decay before exit in auto-mode (HIGH-4 bug fix)
            // Original bug: exit_sweep_mode() was called without decay first,
            // causing stale drone entries to persist indefinitely in auto-mode.
            if (scanner_ptr_ != nullptr) {
                scanner_ptr_->apply_rssi_decay(true);  // is_sweep_mode = true
            }
            exit_sweep_mode();
            return;
        }

        current_pair_ = next_pair;
    }

    // Round-robin to next enabled window (GLOBAL cycling through all 4 windows)
    // This is critical: cycling must go through ALL windows, not just the current pair.
    // Otherwise, with 1-2 enabled windows, the round-robin wraps within the pair
    // and triggers premature pair_complete + auto-exit.
    uint8_t next = active_sweep_idx_;
    do {
        next = (next + 1) % MAX_SWEEP_WINDOWS;
    } while (!sweep_[next].enabled && next != active_sweep_idx_);

    // Wrap detection: if we cycled back to the first window of the pair
    // that just completed, a full round-robin pass is done.
    // This is the correct trigger for RSSI decay — it fires when ALL enabled
    // windows have been scanned once, regardless of how many windows are active.
    // A drone detected in the first window may not be re-seen for several seconds
    // while remaining windows are scanned — premature decay would remove it.
    // SWEEP-AWARE DECAY: Use cycle-based decay (is_sweep_mode=true) to tolerate
    // long gaps between visits in sweep mode.
    if (pair_complete && next == w0) {
        if (scanner_ptr_ != nullptr) {
            scanner_ptr_->apply_rssi_decay(true);  // is_sweep_mode = true
        }

        if (sweep_auto_mode_) {
            exit_sweep_mode();
            return;
        }
        // Manual mode: continue sweeping (pair already advanced above)
    }

    active_sweep_idx_ = next;
    last_tuned_freq_ = sweep_[next].f_center;  // freq for first FFT of new window
    retune_sweep_window(sweep_[next], nullptr);
}

void DroneScannerUI::update_sweep_pair_display() noexcept {
    const uint8_t w0 = current_pair_;
    const uint8_t w1 = w0 + 1;

    if (w0 >= MAX_SWEEP_WINDOWS) return;
    if (!sweep_[w0].enabled && (w1 >= MAX_SWEEP_WINDOWS || !sweep_[w1].enabled)) return;

    if (w1 < MAX_SWEEP_WINDOWS && sweep_[w0].enabled && sweep_[w1].enabled) {
        // Dual mode: upper = w0, lower = w1
        drone_display_.set_sweep_range(sweep_[w0].f_min, sweep_[w0].f_max);
        drone_display_.set_composite_data(sweep_[w0].composite, COMPOSITE_SIZE);
        drone_display_.set_dual_sweep_mode(true);
        drone_display_.set_sweep2_range(sweep_[w1].f_min, sweep_[w1].f_max);
        drone_display_.set_sweep2_data(sweep_[w1].composite, COMPOSITE_SIZE);
    } else if (sweep_[w0].enabled) {
        // Single mode: only w0
        drone_display_.set_sweep_range(sweep_[w0].f_min, sweep_[w0].f_max);
        drone_display_.set_composite_data(sweep_[w0].composite, COMPOSITE_SIZE);
        drone_display_.set_dual_sweep_mode(false);
    } else if (w1 < MAX_SWEEP_WINDOWS && sweep_[w1].enabled) {
        // Only w1 enabled (non-contiguous): show w1 as single
        drone_display_.set_sweep_range(sweep_[w1].f_min, sweep_[w1].f_max);
        drone_display_.set_composite_data(sweep_[w1].composite, COMPOSITE_SIZE);
        drone_display_.set_dual_sweep_mode(false);
    }

    // Push real-time scan-head positions for both bands. The display draws a
    // 1-px marker at the current pixel_index of each enabled window so the
    // user sees where the sweep is right now, even before the spectrum data
    // accumulates enough to fill the band.
    const int16_t head_w0 = (sweep_[w0].enabled)
        ? static_cast<int16_t>(sweep_[w0].pixel_index)
        : static_cast<int16_t>(-1);
    const int16_t head_w1 = (w1 < MAX_SWEEP_WINDOWS && sweep_[w1].enabled)
        ? static_cast<int16_t>(sweep_[w1].pixel_index)
        : static_cast<int16_t>(-1);
    drone_display_.set_scan_head(head_w0, head_w1);
}

uint8_t DroneScannerUI::pair_first(uint8_t idx) const noexcept {
    if (idx >= MAX_SWEEP_WINDOWS) return 0;
    return (idx / 2) * 2;
}

// ============================================================================
// SweepWindow implementations
// ============================================================================

void DroneScannerUI::SweepWindow::init(FreqHz start, FreqHz end, FreqHz step) noexcept {
    f_min = start;
    f_max = end;
    if (f_min >= f_max) {
        f_max = f_min + SWEEP_SLICE_BW;
    }
    const FreqHz range = f_max - f_min;

    // FIX: Exact division to avoid Hz drift per pixel (CRIT-3)
    pixel_step_hz = range / SWEEP_PIXELS_PER_SLICE;

    // CRITICAL FIX: step_hz must be >= usable FFT bandwidth per frame.
    // If step is smaller, FFT frames overlap. The sequential pixel_index
    // accumulation in SweepProcessor then fills composite[] long before
    // f_center reaches f_max, causing the sweep range to be truncated.
    // Usable bandwidth = SWEEP_BINS_PER_STEP * SWEEP_BIN_SIZE = 228 * 78125 = 17,812,500 Hz.
    static constexpr FreqHz MIN_STEP_HZ = SWEEP_BINS_PER_STEP * SWEEP_BIN_SIZE;

    step_hz = (step > 0) ? step : MIN_STEP_HZ;
    if (step_hz < MIN_STEP_HZ) {
        step_hz = MIN_STEP_HZ;
    }

    // Calculate frame count and align step to exact range coverage
    uint16_t frames = (range + step_hz / 2) / step_hz;
    if (frames == 0) {
        frames = 1;  // Safety: prevent division by zero
    }
    step_hz = range / frames;

    // Recalculate if rounding down pushed step below minimum
    if (step_hz < MIN_STEP_HZ && range >= MIN_STEP_HZ) {
        frames = range / MIN_STEP_HZ;
        if (frames == 0) frames = 1;
        step_hz = range / frames;
        if (step_hz < MIN_STEP_HZ) {
            step_hz = MIN_STEP_HZ;
        }
    }

    f_center_ini = f_min + (SWEEP_SLICE_BW / 2);
    reset();
}

void DroneScannerUI::SweepWindow::reset() noexcept {
    memset(composite, 0, COMPOSITE_SIZE);
    f_center = f_center_ini;
    pixel_index = 0;
    pixel_max = 0;
    bins_hz_acc = 0;
    settle_frames_remaining_ = 0;
}

bool DroneScannerUI::SweepWindow::is_exception(FreqHz hz) const noexcept {
    for (uint8_t i = 0; i < EXCEPTIONS_PER_WINDOW; ++i) {
        if (exceptions[i] == 0) continue;
        const FreqHz lo = (exceptions[i] > exception_radius_hz) ? (exceptions[i] - exception_radius_hz) : 0;
        const FreqHz hi = exceptions[i] + exception_radius_hz;
        if (hz >= lo && hz <= hi) return true;
    }
    return false;
}

void DroneScannerUI::SweepWindow::process_bins(const ChannelSpectrum& spectrum) noexcept {
    // Skip initial frames after frequency retune — baseband filters need time to settle.
    if (settle_frames_remaining_ > 0) {
        --settle_frames_remaining_;
        return;
    }
    SweepProcessor::process_frame(
        spectrum,
        composite,
        pixel_index,
        pixel_max,
        bins_hz_acc,
        pixel_step_hz,
        f_center,
        exception_radius_hz,
        exceptions,
        EXCEPTIONS_PER_WINDOW
    );
}

void DroneScannerUI::retune_sweep_window(SweepWindow& win, const char* prefix) noexcept {
    // NOTE: radio::set_tuning_frequency is thread-safe via internal SPI driver mutex
    radio::set_tuning_frequency(rf::Frequency(win.f_center));
    set_current_frequency_safe(win.f_center);
    last_tuned_freq_ = win.f_center;

    // 5ms PLL settle delay matches Looking Glass app — validated for RFFC5072 + MAX2837 lock.
    // Removing this causes corrupted FFT data from unsettled PLLs.
    static constexpr uint32_t PLL_SETTLE_MS = 5;
    chThdSleepMilliseconds(PLL_SETTLE_MS);

    baseband::spectrum_streaming_start();
    win.settle_frames_remaining_ = SWEEP_SETTLE_FRAMES;
    (void)prefix;
}

DroneScanner& get_scanner_instance() noexcept {
    return s_scanner;
}

DroneScanner* get_scanner_ptr() noexcept {
    return &s_scanner;
}

} // namespace drone_analyzer
