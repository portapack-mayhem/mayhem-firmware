// ui_drone_settings_view.cpp - Implementation of settings dialog

#include "ui_drone_settings_view.hpp"
#include "string_format.hpp"
#include "ui_popup.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

DroneSettingsView::DroneSettingsView(NavigationView& nav, DroneAnalyzerSettings& settings)
    : nav_(nav), settings_(settings), backup_settings_(settings)
{
    // Add all UI elements to children
    add_children({
        &tab_view,
        &button_save,
        &button_cancel,
        &button_reset,

        // Spectrum tab
        &text_spectrum_title,
        &text_spectrum_mode_label, &field_spectrum_mode,
        &text_scan_interval_label, &field_scan_interval,
        &text_rssi_threshold_label, &field_rssi_threshold,
        &text_rssi_alpha_label, &field_rssi_alpha,

        // Detection tab
        &text_detection_title,
        &text_min_detections_label, &field_min_detections,
        &text_hysteresis_label, &field_hysteresis,
        &text_trend_threshold_label, &field_trend_threshold,
        &text_reset_interval_label, &field_reset_interval,

        // Audio tab
        &text_audio_title,
        &text_alert_freq_label, &field_alert_freq,
        &text_beep_duration_label, &field_beep_duration,
        &text_alert_squelch_label, &field_alert_squelch,

        // Display tab
        &text_display_title,
        &check_show_graph,
        &check_show_trends,
        &text_max_drones_label, &field_max_drones,
        &text_color_scheme_label, &field_color_scheme,
    });

    setup_ui_elements();
    load_settings_to_ui();
}

void DroneSettingsView::setup_ui_elements() {
    // Event handlers for buttons
    button_save.on_select = [this](Button&) {
        if (validate_settings()) {
            save_ui_to_settings();
            nav_.pop();
        }
    };

    button_cancel.on_select = [this](Button&) {
        // Restore from backup (cancel changes)
        settings_ = backup_settings_;
        nav_.pop();
    };

    button_reset.on_select = [this](Button&) {
        settings_.reset_to_defaults();
        load_settings_to_ui();
    };

    // Spectrum mode change handler
    field_spectrum_mode.on_change = [this](size_t, OptionsField::value_t value) {
        settings_.spectrum_mode = static_cast<SpectrumMode>(value);
    };
}

void DroneSettingsView::load_settings_to_ui() {
    // Load spectrum settings
    field_spectrum_mode.set_by_value(static_cast<int>(settings_.spectrum_mode));
    field_scan_interval.set_value(settings_.spectrum.min_scan_interval_ms);
    field_rssi_threshold.set_value(settings_.spectrum.default_rssi_threshold);
    field_rssi_alpha.set_value(static_cast<int>(settings_.spectrum.rssi_smoothing_alpha * 10)); // Convert 0.1-0.9 to 1-9

    // Load detection settings
    field_min_detections.set_value(settings_.detection.min_detection_count);
    field_hysteresis.set_value(settings_.detection.hysteresis_margin_db);
    field_trend_threshold.set_value(settings_.detection.trend_threshold_db);
    field_reset_interval.set_value(settings_.detection.detection_reset_interval);

    // Load audio settings
    field_alert_freq.set_value(settings_.audio.alert_frequency_hz);
    field_beep_duration.set_value(settings_.audio.beep_duration_ms);
    field_alert_squelch.set_value(settings_.audio.alert_squelch_db);

    // Load display settings
    check_show_graph.set_value(settings_.display.show_rssi_graph);
    check_show_trends.set_value(settings_.display.show_trends);
    field_max_drones.set_value(settings_.display.max_display_drones);
    field_color_scheme.set_by_value(settings_.display.color_scheme);
}

void DroneSettingsView::save_ui_to_settings() {
    // Save spectrum settings
    settings_.spectrum.min_scan_interval_ms = field_scan_interval.to_integer();
    settings_.spectrum.default_rssi_threshold = field_rssi_threshold.to_integer();
    settings_.spectrum.rssi_smoothing_alpha = field_rssi_alpha.to_integer() / 10.0f; // Convert back

    // Save detection settings
    settings_.detection.min_detection_count = field_min_detections.to_integer();
    settings_.detection.hysteresis_margin_db = field_hysteresis.to_integer();
    settings_.detection.trend_threshold_db = field_trend_threshold.to_integer();
    settings_.detection.detection_reset_interval = field_reset_interval.to_integer();

    // Save audio settings
    settings_.audio.alert_frequency_hz = field_alert_freq.to_integer();
    settings_.audio.beep_duration_ms = field_beep_duration.to_integer();
    settings_.audio.alert_squelch_db = field_alert_squelch.to_integer();

    // Save display settings
    settings_.display.show_rssi_graph = check_show_graph.value();
    settings_.display.show_trends = check_show_trends.value();
    settings_.display.max_display_drones = field_max_drones.to_integer();
    settings_.display.color_scheme = field_color_scheme.selected_index_value();
}

void DroneSettingsView::validate_settings() {
    // Validate RSSI alpha (1-9 maps to 0.1-0.9)
    float alpha = field_rssi_alpha.to_integer() / 10.0f;
    if (alpha < 0.1f || alpha > 0.9f) {
        show_validation_error("RSSI Smooth must be 0.1-0.9");
        field_rssi_alpha.focus();
        return false;
    }

    // Validate minimum detections
    int min_det = field_min_detections.to_integer();
    if (min_det < 1 || min_det > 10) {
        show_validation_error("Min detections must be 1-10");
        field_min_detections.focus();
        return false;
    }

    // Validate scan interval
    uint32_t scan_int = field_scan_interval.to_integer();
    if (scan_int < 200 || scan_int > 5000) {
        show_validation_error("Scan interval must be 200-5000 ms");
        field_scan_interval.focus();
        return false;
    }

    // Validate alert frequency
    uint32_t alert_freq = field_alert_freq.to_integer();
    if (alert_freq < 400 || alert_freq > 3000) {
        show_validation_error("Alert frequency must be 400-3000 Hz");
        field_alert_freq.focus();
        return false;
    }

    return true;
}

void DroneSettingsView::show_validation_error(const std::string& message) {
    nav_.display_modal("Validation Error", message);
}

} // namespace ui::external_app::enhanced_drone_analyzer
