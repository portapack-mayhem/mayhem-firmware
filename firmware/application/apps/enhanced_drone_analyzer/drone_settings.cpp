#include <cstdint>
#include <cstring>
#include <cstdio>

#include "drone_settings.hpp"
#include "settings_manager.hpp"
#include "constants.hpp"
#include "scanner.hpp"
#include "audio_alerts.hpp"
#include "drone_display.hpp"
#include "ui_receiver.hpp"
#include "file.hpp"
#include "file_path.hpp"
#include "receiver_model.hpp"

namespace drone_analyzer {

// ============================================================================
// DroneSettingsView Constructor / Destructor
// ============================================================================

DroneSettingsView::DroneSettingsView(NavigationView& nav, const ScanConfig& config, DroneScanner* scanner_ptr, DroneDisplay* display) noexcept
    : ui::View()
    , labels_({
        {{UI_POS_X(1), UI_POS_Y(1)}, "Int(ms):", Color::white()},
        {{UI_POS_X(1), UI_POS_Y(3)}, "Sens:", Color::white()},
        {{UI_POS_X(13), UI_POS_Y(2)}, "Vol:", Color::white()},
        {{UI_POS_X(13), UI_POS_Y(3)}, "Cyc:", Color::white()},
        {{UI_POS_X(17), UI_POS_Y(5)}, "Mar:", Color::white()},
        {{UI_POS_X(17), UI_POS_Y(6)}, "Min:", Color::white()},
        {{UI_POS_X(0), UI_POS_Y(5)}, "MaxW:", Color::white()},
        {{UI_POS_X(0), UI_POS_Y(6)}, "Shrp:", Color::white()},
        {{UI_POS_X(10), UI_POS_Y(5)}, "Rat:", Color::white()},
        {{UI_POS_X(10), UI_POS_Y(6)}, "Vly:", Color::white()},
        {{UI_POS_X(10), UI_POS_Y(4)}, "Flat:", Color::white()},
        {{UI_POS_X(17), UI_POS_Y(4)}, "Sym:", Color::white()},
        {{UI_POS_X(0), UI_POS_Y(0)}, "CFAR:", Color::white()},
        {{UI_POS_X(11), UI_POS_Y(0)}, "Ref:", Color::white()},
        {{UI_POS_X(17), UI_POS_Y(0)}, "Grd:", Color::white()},
        {{UI_POS_X(22), UI_POS_Y(0)}, "Thr:", Color::white()},
        {{UI_POS_X(0), UI_POS_Y(16)}, "Lo:", Color::white()},
        {{UI_POS_X(8), UI_POS_Y(16)}, "Md:", Color::white()},
        {{UI_POS_X(16), UI_POS_Y(16)}, "Hi:", Color::white()},
        {{UI_POS_X(24), UI_POS_Y(16)}, "Cr:", Color::white()},
    })
    , field_scan_interval_({UI_POS_X(1), UI_POS_Y(2)}, 4, {10, 1000}, 10, ' ')
    , field_rssi_threshold_({UI_POS_X(1), UI_POS_Y(4)}, 3, {0, 100}, 1, ' ')
    , field_volume_({UI_POS_X(17), UI_POS_Y(2)}, 2, {0, 99}, 1, ' ')
    , field_rssi_dec_cyc_({UI_POS_X(17), UI_POS_Y(3)}, 2, {1, 50}, 1, ' ')
    , check_audio_alerts_({UI_POS_X(1), UI_POS_Y(9)}, 6, "Audio", false)
    , check_spectrum_visible_({UI_POS_X(20), UI_POS_Y(9)}, 5, "SpVis", false)
    , check_histogram_visible_({UI_POS_X(20), UI_POS_Y(13)}, 5, "Hist", false)
    , check_dwell_enabled_({UI_POS_X(1), UI_POS_Y(11)}, 6, "Dwell", false)
    , check_confirm_count_({UI_POS_X(1), UI_POS_Y(13)}, 8, "Confirm", false)
    , field_confirm_count_({UI_POS_X(13), UI_POS_Y(13)}, 2, {1, 10}, 1, ' ')
        , check_spectrum_detection_({UI_POS_X(20), UI_POS_Y(11)}, 5, "SpDet", false)
    , field_neighbor_margin_({UI_POS_X(17), UI_POS_Y(15)}, 2, {0, 15}, 1, ' ')
    , check_neighbor_margin_({UI_POS_X(20), UI_POS_Y(15)}, 4, "NB", false)
    , check_noise_blacklist_({UI_POS_X(1), UI_POS_Y(15)}, 8, "Blklist", false)
    , check_rssi_variance_({UI_POS_X(20), UI_POS_Y(7)}, 5, "RVar", false)
    , preview_({0, 152, 240, 48})
    , check_mahalanobis_({UI_POS_X(20), UI_POS_Y(2)}, 3, "MG", false)
    , field_mahalanobis_threshold_({UI_POS_X(22), UI_POS_Y(1)}, 3,
                               {MAHALANOBIS_THRESHOLD_MIN_X10, MAHALANOBIS_THRESHOLD_MAX_X10},
                               DEFAULT_MAHALOBIS_THRESHOLD_X10, ' ')
    , check_pattern_matching_({UI_POS_X(10), UI_POS_Y(8)}, 5, "Ptr", false)
    , field_spectrum_margin_({UI_POS_X(20), UI_POS_Y(5)}, 3, {5, 200}, 5, ' ')
    , field_spectrum_min_width_({UI_POS_X(20), UI_POS_Y(6)}, 3, {1, 100}, 1, ' ')
    , field_spectrum_max_width_({UI_POS_X(6), UI_POS_Y(5)}, 3, {2, 255}, 1, ' ')
    , field_spectrum_peak_sharpness_({UI_POS_X(6), UI_POS_Y(6)}, 3, {50, 250}, 5, ' ')
    , field_spectrum_peak_ratio_({UI_POS_X(13), UI_POS_Y(5)}, 3, {0, 255}, 5, ' ')
    , field_spectrum_valley_depth_({UI_POS_X(13), UI_POS_Y(6)}, 3, {0, 200}, 5, ' ')
    , field_spectrum_flatness_({UI_POS_X(14), UI_POS_Y(4)}, 3, {0, 100}, 5, ' ')
    , field_spectrum_symmetry_({UI_POS_X(20), UI_POS_Y(4)}, 3, {0, 100}, 5, ' ')
    , button_defaults_({UI_POS_X(0), UI_POS_Y_BOTTOM(2), UI_POS_WIDTH(13), 20}, "DEFAULT")
    , button_about_({UI_POS_X(13), UI_POS_Y_BOTTOM(2), UI_POS_WIDTH(2), 20}, "!")
    , button_save_({UI_POS_X(15), UI_POS_Y_BOTTOM(2), UI_POS_WIDTH(14), 20}, "SAVE")
    , button_info_margin_({UI_POS_X(0), UI_POS_Y(7), UI_POS_WIDTH(4), 16}, "Mrg?")
    , button_info_width_({UI_POS_X(5), UI_POS_Y(7), UI_POS_WIDTH(4), 16}, "Wid?")
    , button_info_sharp_({UI_POS_X(10), UI_POS_Y(7), UI_POS_WIDTH(4), 16}, "Shp?")
    , button_info_ratio_({UI_POS_X(15), UI_POS_Y(7), UI_POS_WIDTH(4), 16}, "Rat?")
    , field_threat_low_({UI_POS_X(3), UI_POS_Y(16)}, 3, {RSSI_MIN_DBM, RSSI_MAX_DBM}, DEFAULT_THREAT_LOW_DBM, ' ')
    , field_threat_medium_({UI_POS_X(11), UI_POS_Y(16)}, 3, {RSSI_MIN_DBM, RSSI_MAX_DBM}, DEFAULT_THREAT_MEDIUM_DBM, ' ')
    , field_threat_high_({UI_POS_X(19), UI_POS_Y(16)}, 3, {RSSI_MIN_DBM, RSSI_MAX_DBM}, RSSI_HIGH_THREAT_THRESHOLD_DBM, ' ')
    , field_threat_critical_({UI_POS_X(27), UI_POS_Y(16)}, 3, {RSSI_MIN_DBM, RSSI_MAX_DBM}, RSSI_CRITICAL_THREAT_THRESHOLD_DBM, ' ')
    , field_cfar_mode_({UI_POS_X(4), UI_POS_Y(0)}, 7, {
        {"OFF", static_cast<int32_t>(CFARMode::OFF)},
        {"CA", static_cast<int32_t>(CFARMode::CA)},
        {"GO", static_cast<int32_t>(CFARMode::GO)},
        {"SO", static_cast<int32_t>(CFARMode::SO)},
        {"HYBRID", static_cast<int32_t>(CFARMode::HYBRID)},
        {"OS", static_cast<int32_t>(CFARMode::OS)},
        {"VI", static_cast<int32_t>(CFARMode::VI)},
    })
    , field_cfar_ref_cells_({UI_POS_X(14), UI_POS_Y(0)}, 2, {4, 64}, 4, ' ')
    , field_cfar_guard_cells_({UI_POS_X(20), UI_POS_Y(0)}, 1, {0, 8}, 1, ' ')
    , field_cfar_threshold_({UI_POS_X(25), UI_POS_Y(0)}, 3, {10, 100}, 5, ' ')
    , button_info_cfar_({UI_POS_X(29), UI_POS_Y(0), UI_POS_WIDTH(3), 16}, "CF?")
    , nav_(nav)
    , scanner_ptr_(scanner_ptr)
    , display_ptr_(display)
    , original_config_(config)
    , settings_()
    , settings_dirty_(false) {

    // Extract current scanner config into settings struct
    SettingsFileManager::extract_from_config(config, settings_);

    add_children({
        &preview_,
        &labels_,
        &field_scan_interval_,
        &field_rssi_threshold_,
        &field_volume_,
        &field_rssi_dec_cyc_,
        &check_audio_alerts_,
        &check_spectrum_visible_,
        &check_histogram_visible_,
        &check_dwell_enabled_,
        &check_confirm_count_,
        &field_confirm_count_,
        &check_spectrum_detection_,
        &field_neighbor_margin_,
        &check_neighbor_margin_,
        &check_noise_blacklist_,
        &check_rssi_variance_,
        &check_mahalanobis_,
        &field_mahalanobis_threshold_,
        &check_pattern_matching_,
        &field_spectrum_margin_,
        &field_spectrum_min_width_,
        &field_spectrum_max_width_,
        &field_spectrum_peak_sharpness_,
        &field_spectrum_peak_ratio_,
        &field_spectrum_valley_depth_,
        &field_spectrum_flatness_,
        &field_spectrum_symmetry_,
        &button_defaults_,
        &button_about_,
        &button_save_,
        &button_info_margin_,
        &button_info_width_,
        &button_info_sharp_,
        &button_info_ratio_,
        &field_cfar_mode_,
        &field_cfar_ref_cells_,
        &field_cfar_guard_cells_,
        &field_cfar_threshold_,
        &button_info_cfar_,
        &field_threat_low_,
        &field_threat_medium_,
        &field_threat_high_,
        &field_threat_critical_,
    });

    // Load persisted settings from SD card (overrides config-based defaults)
    // If load fails, settings_ retains constructor defaults
    (void)SettingsFileManager::load(settings_);

    // Median filter is controlled by main UI button (Md+), not settings view.
    // Restore from scanner config to prevent SD card stale value from overriding
    // the user's button toggle.
    if (scanner_ptr_ != nullptr) {
        const auto live_cfg = scanner_ptr_->get_config();
        settings_.median_enabled = live_cfg.median_enabled;
    }

    // Populate UI fields from loaded settings
    apply_settings_to_ui();

    // --- Callbacks ---

    field_scan_interval_.on_change = [this](int32_t v) {
        settings_.scan_interval_ms = static_cast<uint32_t>(v);
        settings_dirty_ = true;
    };

    field_rssi_threshold_.on_change = [this](int32_t v) {
        settings_.alert_rssi_threshold_dbm = -20 - v;
        settings_.scan_sensitivity = static_cast<uint8_t>(v);
        settings_dirty_ = true;
    };

    field_volume_.on_change = [this](int32_t v) {
        settings_.volume = static_cast<uint8_t>(v);
        portapack::receiver_model.set_normalized_headphone_volume(static_cast<uint8_t>(v));
        settings_dirty_ = true;
    };

    field_rssi_dec_cyc_.on_change = [this](int32_t v) {
        settings_.rssi_decrease_cycles = static_cast<uint8_t>(v);
        settings_dirty_ = true;
    };

    check_audio_alerts_.on_select = [this](ui::Checkbox&, bool v) {
        settings_.audio_alerts_enabled = v;
        AudioAlertManager::set_enabled(v);
        settings_dirty_ = true;
    };

    check_spectrum_visible_.on_select = [this](ui::Checkbox&, bool v) {
        settings_.spectrum_visible = v;
        if (display_ptr_ != nullptr) {
            display_ptr_->set_spectrum_visible(v);
        }
        settings_dirty_ = true;
    };

    check_histogram_visible_.on_select = [this](ui::Checkbox&, bool v) {
        settings_.histogram_visible = v;
        if (display_ptr_ != nullptr) {
            display_ptr_->set_histogram_visible(v);
        }
        settings_dirty_ = true;
    };

    check_dwell_enabled_.on_select = [this](ui::Checkbox&, bool v) {
        settings_.dwell_enabled = v;
        settings_dirty_ = true;
    };

    check_confirm_count_.on_select = [this](ui::Checkbox&, bool v) {
        settings_.confirm_count_enabled = v;
        field_confirm_count_.visible(v);
        settings_dirty_ = true;
    };

    field_confirm_count_.on_change = [this](int32_t v) {
        settings_.confirm_count = static_cast<uint8_t>(v);
        settings_dirty_ = true;
    };

    check_noise_blacklist_.on_select = [this](ui::Checkbox&, bool v) {
        settings_.noise_blacklist_enabled = v;
        settings_dirty_ = true;
    };

    check_spectrum_detection_.on_select = [this](ui::Checkbox&, bool v) {
        settings_.spectrum_detection_enabled = v;
        set_shape_filter_visibility(v);
        settings_dirty_ = true;
    };

    check_neighbor_margin_.on_select = [this](ui::Checkbox&, bool v) {
        settings_.neighbor_margin_db = v ? DEFAULT_NEIGHBOR_MARGIN_DB : 0;
        settings_dirty_ = true;
    };

    field_neighbor_margin_.on_change = [this](int32_t v) {
        settings_.neighbor_margin_db = v;
        settings_dirty_ = true;
    };

    check_rssi_variance_.on_select = [this](ui::Checkbox&, bool v) {
        settings_.rssi_variance_enabled = v;
        settings_dirty_ = true;
    };

    field_spectrum_margin_.on_change = [this](int32_t v) {
        settings_.spectrum_margin = static_cast<uint8_t>(v);
        update_preview();
        settings_dirty_ = true;
    };

    field_spectrum_min_width_.on_change = [this](int32_t v) {
        settings_.spectrum_min_width = static_cast<uint8_t>(v);
        if (settings_.spectrum_min_width > settings_.spectrum_max_width) {
            settings_.spectrum_max_width = settings_.spectrum_min_width;
            field_spectrum_max_width_.set_value(settings_.spectrum_max_width);
        }
        update_preview();
        settings_dirty_ = true;
    };

    field_spectrum_max_width_.on_change = [this](int32_t v) {
        settings_.spectrum_max_width = static_cast<uint8_t>(v);
        if (settings_.spectrum_max_width < settings_.spectrum_min_width) {
            settings_.spectrum_min_width = settings_.spectrum_max_width;
            field_spectrum_min_width_.set_value(settings_.spectrum_min_width);
        }
        update_preview();
        settings_dirty_ = true;
    };

    field_spectrum_peak_sharpness_.on_change = [this](int32_t v) {
        settings_.spectrum_peak_sharpness = static_cast<uint8_t>(v);
        update_preview();
        settings_dirty_ = true;
    };

    field_spectrum_peak_ratio_.on_change = [this](int32_t v) {
        settings_.spectrum_peak_ratio = static_cast<uint8_t>(v);
        update_preview();
        settings_dirty_ = true;
    };

    field_spectrum_valley_depth_.on_change = [this](int32_t v) {
        settings_.spectrum_valley_depth = static_cast<uint8_t>(v);
        update_preview();
        settings_dirty_ = true;
    };

    field_spectrum_flatness_.on_change = [this](int32_t v) {
        settings_.spectrum_flatness = static_cast<uint8_t>(v);
        update_preview();
        settings_dirty_ = true;
    };

    field_spectrum_symmetry_.on_change = [this](int32_t v) {
        settings_.spectrum_symmetry = static_cast<uint8_t>(v);
        update_preview();
        settings_dirty_ = true;
    };

    // SAVE button: apply to scanner + save to SD card
    button_save_.on_select = [this](ui::Button&) {
        // Apply settings to scanner config
        if (scanner_ptr_ != nullptr) {
            ScanConfig updated_config = original_config_;
            SettingsFileManager::apply_to_config(settings_, updated_config);
            // Preserve sweep settings from scanner (SWP view manages them)
            const auto current_cfg = scanner_ptr_->get_config();
            updated_config.sweep_start_freq = current_cfg.sweep_start_freq;
            updated_config.sweep_end_freq = current_cfg.sweep_end_freq;
            updated_config.sweep_step_freq = current_cfg.sweep_step_freq;
            updated_config.sweep2_start_freq = current_cfg.sweep2_start_freq;
            updated_config.sweep2_end_freq = current_cfg.sweep2_end_freq;
            updated_config.sweep2_step_freq = current_cfg.sweep2_step_freq;
            updated_config.sweep2_enabled = current_cfg.sweep2_enabled;
            updated_config.sweep3_start_freq = current_cfg.sweep3_start_freq;
            updated_config.sweep3_end_freq = current_cfg.sweep3_end_freq;
            updated_config.sweep3_step_freq = current_cfg.sweep3_step_freq;
            updated_config.sweep3_enabled = current_cfg.sweep3_enabled;
            updated_config.sweep4_start_freq = current_cfg.sweep4_start_freq;
            updated_config.sweep4_end_freq = current_cfg.sweep4_end_freq;
            updated_config.sweep4_step_freq = current_cfg.sweep4_step_freq;
            updated_config.sweep4_enabled = current_cfg.sweep4_enabled;

            // Preserve sweep exceptions from scanner (SWP view manages them)
            for (uint8_t w = 0; w < 4; ++w) {
                for (uint8_t i = 0; i < EXCEPTIONS_PER_WINDOW; ++i) {
                    updated_config.sweep_exceptions[w][i] = current_cfg.sweep_exceptions[w][i];
                }
            }

            const ErrorCode err = scanner_ptr_->set_config(updated_config);
            if (err != ErrorCode::SUCCESS) {
                nav_.display_modal("Error", "Invalid settings.\nCheck min<=max\nand valid ranges.");
                return;
            }
        }

        save_settings_to_sd();
        nav_.pop();
    };

    button_defaults_.on_select = [this](ui::Button&) {
        settings_ = SettingsStruct();
        portapack::receiver_model.set_normalized_headphone_volume(settings_.volume);
        settings_dirty_ = true;
        apply_settings_to_ui();
    };

    button_about_.on_select = [this](ui::Button&) {
        nav_.display_modal("About",
            "Author: Kuznetsov Maxim\n"
            "Orenburg\n"
            "Card: 2202 20202 5787 1695\n"
            "YooMoney: 41001810704697\n"
            "TON: UQCdtMxQB5zbQBOICkY90l\n"
            "TQQqcs8V-V28Bf2AGvl8xOc5HR\n"
            "Telegram: @max_ai_master\n"
            "TM PowerHamster2188");
    };

    // Info buttons for spectrum filter settings
    button_info_margin_.on_select = [this](ui::Button&) {
        nav_.display_modal("Margin",
            "Porog shuma signala.\n"
            "Skolko dB piki dolzhny\n"
            "byt vyshe fona.\n"
            "Bolshe = menshe lozhnyh.\n"
            "20 = FPV po umolchaniyu.");
    };

    button_info_width_.on_select = [this](ui::Button&) {
        nav_.display_modal("Width",
            "Min: otbrasyvaet uzskie\n"
            "  ( игly )\n"
            " 3 = 234kHz (po umolch.)\n"
            "Max: otbrasyvaet shirokie\n"
            "  ( ploskie pomehi )\n"
            "200 = FPV (~15MHz),\n"
            "255 = prinyat vse.");
    };

    button_info_sharp_.on_select = [this](ui::Button&) {
        nav_.display_modal("Sharpness",
            "Ostota pika signala.\n"
            "Video link drona = V-forma.\n"
            "Bolshe = strogij filtr.\n"
            "80-150 dlya FPV video.\n"
            "50 = ljuboj signal.");
    };

    button_info_ratio_.on_select = [this](ui::Button&) {
        nav_.display_modal("Peak Ratio",
            "Otnoshenie vysoty k shirine.\n"
            "Visokij + uzkoj = dron.\n"
            "Nizkij = pomeha.\n"
            "0 = FPV (otklychen),\n"
            "80 = dlya uzkipolnyh.");
    };

    // CFAR callbacks
    field_cfar_mode_.on_change = [this](size_t, int32_t v) {
        settings_.cfar_mode = static_cast<CFARMode>(v);
        settings_dirty_ = true;
    };

    field_cfar_ref_cells_.on_change = [this](int32_t v) {
        settings_.cfar_ref_cells = static_cast<uint8_t>(v);
        settings_dirty_ = true;
    };

    field_cfar_guard_cells_.on_change = [this](int32_t v) {
        settings_.cfar_guard_cells = static_cast<uint8_t>(v);
        settings_dirty_ = true;
    };

    field_cfar_threshold_.on_change = [this](int32_t v) {
        settings_.cfar_threshold_x10 = static_cast<uint8_t>(v);
        settings_dirty_ = true;
    };

    // Mahalanobis gate callbacks
    check_mahalanobis_.on_select = [this](ui::Checkbox&, bool v) {
        settings_.mahalanobis_enabled = v;
        settings_dirty_ = true;
    };

    // Pattern matching toggle
    check_pattern_matching_.on_select = [this](ui::Checkbox&, bool v) {
        settings_.pattern_matching_enabled = v;
        settings_dirty_ = true;
    };

    field_mahalanobis_threshold_.on_change = [this](int32_t v) {
        settings_.mahalanobis_threshold_x10 = static_cast<uint8_t>(v);
        settings_dirty_ = true;
    };

    // Threat threshold callbacks — enforce ordering: low <= medium <= high <= critical
    // A guard flag prevents re-entrant callback execution when auto-adjusting neighbors.
    field_threat_low_.on_change = [this](int32_t v) {
        settings_.threat_low_dbm = v;
        // Ensure low <= medium: push medium up if needed
        if (v > settings_.threat_medium_dbm) {
            settings_.threat_medium_dbm = v;
            field_threat_medium_.set_value(v);
        }
        settings_dirty_ = true;
    };

    field_threat_medium_.on_change = [this](int32_t v) {
        settings_.threat_medium_dbm = v;
        // Ensure low <= medium: push low down if needed
        if (v < settings_.threat_low_dbm) {
            settings_.threat_low_dbm = v;
            field_threat_low_.set_value(v);
        }
        // Ensure medium <= high: push high up if needed
        if (v > settings_.threat_high_dbm) {
            settings_.threat_high_dbm = v;
            field_threat_high_.set_value(v);
        }
        settings_dirty_ = true;
    };

    field_threat_high_.on_change = [this](int32_t v) {
        settings_.threat_high_dbm = v;
        // Ensure medium <= high: push medium down if needed
        if (v < settings_.threat_medium_dbm) {
            settings_.threat_medium_dbm = v;
            field_threat_medium_.set_value(v);
        }
        // Ensure high <= critical: push critical up if needed
        if (v > settings_.threat_critical_dbm) {
            settings_.threat_critical_dbm = v;
            field_threat_critical_.set_value(v);
        }
        settings_dirty_ = true;
    };

    field_threat_critical_.on_change = [this](int32_t v) {
        settings_.threat_critical_dbm = v;
        // Ensure high <= critical: push high down if needed
        if (v < settings_.threat_high_dbm) {
            settings_.threat_high_dbm = v;
            field_threat_high_.set_value(v);
        }
        settings_dirty_ = true;
    };

    button_info_cfar_.on_select = [this](ui::Button&) {
        nav_.display_modal("CFAR",
            "Adaptivnyj porog CFAR.\n"
            "CA = srednee shuma.\n"
            "GO = max iz okon.\n"
            "SO = min iz okon.\n"
            "HYBRID = smes.\n"
            "OFF = fiksirovannyj.");
    };
}

DroneSettingsView::~DroneSettingsView() noexcept {
}

void DroneSettingsView::paint(ui::Painter& painter) {
    (void)painter;
}

void DroneSettingsView::focus() {
    field_scan_interval_.focus();
}

// ============================================================================
// UI Population
// ============================================================================

void DroneSettingsView::apply_settings_to_ui() noexcept {
    field_scan_interval_.set_value(settings_.scan_interval_ms);
    {
        const int32_t sens = -(settings_.alert_rssi_threshold_dbm + 20);
        field_rssi_threshold_.set_value(sens < 0 ? 0 : (sens > 100 ? 100 : sens));
    }
    field_volume_.set_value(static_cast<int32_t>(settings_.volume));
    field_rssi_dec_cyc_.set_value(static_cast<int32_t>(settings_.rssi_decrease_cycles));
    check_audio_alerts_.set_value(settings_.audio_alerts_enabled);
    check_spectrum_visible_.set_value(settings_.spectrum_visible);
    check_histogram_visible_.set_value(settings_.histogram_visible);
    check_dwell_enabled_.set_value(settings_.dwell_enabled);
    check_confirm_count_.set_value(settings_.confirm_count_enabled);
    field_confirm_count_.set_value(static_cast<int32_t>(settings_.confirm_count));
    field_confirm_count_.visible(settings_.confirm_count_enabled);
    check_noise_blacklist_.set_value(settings_.noise_blacklist_enabled);
    check_spectrum_detection_.set_value(settings_.spectrum_detection_enabled);
    field_spectrum_margin_.set_value(static_cast<int32_t>(settings_.spectrum_margin));
    field_spectrum_min_width_.set_value(static_cast<int32_t>(settings_.spectrum_min_width));
    field_spectrum_max_width_.set_value(static_cast<int32_t>(settings_.spectrum_max_width));
    field_spectrum_peak_sharpness_.set_value(static_cast<int32_t>(settings_.spectrum_peak_sharpness));
    field_spectrum_peak_ratio_.set_value(static_cast<int32_t>(settings_.spectrum_peak_ratio));
    field_spectrum_valley_depth_.set_value(static_cast<int32_t>(settings_.spectrum_valley_depth));
    field_spectrum_flatness_.set_value(static_cast<int32_t>(settings_.spectrum_flatness));
    field_spectrum_symmetry_.set_value(static_cast<int32_t>(settings_.spectrum_symmetry));
    check_neighbor_margin_.set_value(settings_.neighbor_margin_db > 0);
    field_neighbor_margin_.set_value(static_cast<int32_t>(settings_.neighbor_margin_db));
    check_rssi_variance_.set_value(settings_.rssi_variance_enabled);
    check_mahalanobis_.set_value(settings_.mahalanobis_enabled);
    field_mahalanobis_threshold_.set_value(static_cast<int32_t>(settings_.mahalanobis_threshold_x10));
    check_pattern_matching_.set_value(settings_.pattern_matching_enabled);
    field_cfar_mode_.set_by_value(static_cast<int32_t>(settings_.cfar_mode));
    field_cfar_ref_cells_.set_value(static_cast<int32_t>(settings_.cfar_ref_cells));
    field_cfar_guard_cells_.set_value(static_cast<int32_t>(settings_.cfar_guard_cells));
    field_cfar_threshold_.set_value(static_cast<int32_t>(settings_.cfar_threshold_x10));

    // Threat thresholds
    field_threat_low_.set_value(settings_.threat_low_dbm);
    field_threat_medium_.set_value(settings_.threat_medium_dbm);
    field_threat_high_.set_value(settings_.threat_high_dbm);
    field_threat_critical_.set_value(settings_.threat_critical_dbm);

    // Set initial visibility based on spectrum detection state
    set_shape_filter_visibility(settings_.spectrum_detection_enabled);

    update_preview();
}

// ============================================================================
// Preview Update Helper
// ============================================================================

void DroneSettingsView::update_preview() noexcept {
    preview_.set_params(
        settings_.spectrum_margin,
        settings_.spectrum_min_width,
        settings_.spectrum_max_width,
        settings_.spectrum_peak_sharpness,
        settings_.spectrum_peak_ratio,
        settings_.spectrum_valley_depth,
        settings_.spectrum_flatness,
        settings_.spectrum_symmetry);
}

void DroneSettingsView::set_shape_filter_visibility(bool visible) noexcept {
    field_spectrum_margin_.visible(visible);
    field_spectrum_min_width_.visible(visible);
    field_spectrum_max_width_.visible(visible);
    field_spectrum_peak_sharpness_.visible(visible);
    field_spectrum_peak_ratio_.visible(visible);
    field_spectrum_valley_depth_.visible(visible);
    field_spectrum_flatness_.visible(visible);
    field_spectrum_symmetry_.visible(visible);
    button_info_margin_.visible(visible);
    button_info_width_.visible(visible);
    button_info_sharp_.visible(visible);
    button_info_ratio_.visible(visible);
    // CFAR fields are also gated by spectrum detection
    field_cfar_mode_.visible(visible);
    field_cfar_ref_cells_.visible(visible);
    field_cfar_guard_cells_.visible(visible);
    field_cfar_threshold_.visible(visible);
    button_info_cfar_.visible(visible);
}

// ============================================================================
// SD Card Save (via centralized SettingsFileManager)
// ============================================================================

void DroneSettingsView::save_settings_to_sd() noexcept {
    (void)SettingsFileManager::save(scanner_ptr_, settings_);
}

} // namespace drone_analyzer