// ui_drone_constant_settings_view.cpp
// Implementation of interactive constant settings dialog for Enhanced Drone Analyzer

#include "ui_drone_constant_settings_view.hpp"
#include "app_settings.hpp"
#include "string_format.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

// Initialize static variables with default values
rf::Frequency ConstantSettingsManager::current_min_drone_freq = MIN_DRONE_FREQUENCY;
rf::Frequency ConstantSettingsManager::current_max_drone_freq = MAX_DRONE_FREQUENCY;
uint8_t ConstantSettingsManager::current_min_detection_count = MIN_DETECTION_COUNT;
int32_t ConstantSettingsManager::current_hysteresis_margin_db = HYSTERESIS_MARGIN_DB;
int16_t ConstantSettingsManager::current_trend_threshold_db = TREND_THRESHOLD_DB;
int32_t ConstantSettingsManager::current_default_rssi_threshold_db = DEFAULT_RSSI_THRESHOLD_DB;
uint32_t ConstantSettingsManager::current_scan_interval_ms = MIN_SCAN_INTERVAL_MS;
uint32_t ConstantSettingsManager::current_stale_timeout_ms = STALE_DRONE_TIMEOUT_MS;

bool ConstantSettingsManager::load_settings() {
    // Load settings from persistent storage
    app_settings::SettingsManager settings_manager{"eda_constants", app_settings::Mode::SETTINGS, {
        {"min_drone_freq"sv, &current_min_drone_freq},
        {"max_drone_freq"sv, &current_max_drone_freq},
        {"min_detection_count"sv, (void*)&current_min_detection_count},
        {"hysteresis_margin_db"sv, &current_hysteresis_margin_db},
        {"trend_threshold_db"sv, (void*)&current_trend_threshold_db},
        {"default_rssi_threshold_db"sv, &current_default_rssi_threshold_db},
        {"scan_interval_ms"sv, &current_scan_interval_ms},
        {"stale_timeout_ms"sv, &current_stale_timeout_ms}
    }};

    return settings_manager.load();
}

bool ConstantSettingsManager::save_settings() {
    // Save settings to persistent storage
    app_settings::SettingsManager settings_manager{"eda_constants", app_settings::Mode::SETTINGS, {
        {"min_drone_freq"sv, &current_min_drone_freq},
        {"max_drone_freq"sv, &current_max_drone_freq},
        {"min_detection_count"sv, (void*)&current_min_detection_count},
        {"hysteresis_margin_db"sv, &current_hysteresis_margin_db},
        {"trend_threshold_db"sv, (void*)&current_trend_threshold_db},
        {"default_rssi_threshold_db"sv, &current_default_rssi_threshold_db},
        {"scan_interval_ms"sv, &current_scan_interval_ms},
        {"stale_timeout_ms"sv, &current_stale_timeout_ms}
    }};

    return settings_manager.save();
}

void ConstantSettingsManager::reset_to_defaults() {
    // Reset all constants to defaults from ui_drone_config.hpp
    current_min_drone_freq = MIN_DRONE_FREQUENCY;
    current_max_drone_freq = MAX_DRONE_FREQUENCY;
    current_min_detection_count = MIN_DETECTION_COUNT;
    current_hysteresis_margin_db = HYSTERESIS_MARGIN_DB;
    current_trend_threshold_db = TREND_THRESHOLD_DB;
    current_default_rssi_threshold_db = DEFAULT_RSSI_THRESHOLD_DB;
    current_scan_interval_ms = MIN_SCAN_INTERVAL_MS;
    current_stale_timeout_ms = STALE_DRONE_TIMEOUT_MS;
}

ConstantSettingsView::ConstantSettingsView(NavigationView& nav)
    : nav_(nav) {

    // Add all UI components to the view
    add_children({
        &text_freq_range_title_,
        &text_min_drone_freq_,
        &field_min_drone_freq_,
        &text_max_drone_freq_,
        &field_max_drone_freq_,
        &text_detection_title_,
        &text_min_detection_,
        &field_min_detection_count_,
        &text_hysteresis_,
        &field_hysteresis_margin_,
        &text_trend_threshold_,
        &field_trend_threshold_,
        &text_default_rssi_,
        &field_default_rssi_threshold_,
        &text_scanning_title_,
        &text_scan_interval_,
        &field_scan_interval_,
        &text_stale_timeout_,
        &field_stale_timeout_,
        &button_defaults_,
        &button_save_,
        &button_ok_,
        &text_status_
    });

    // Initialize field values from current settings
    initialize_field_values();

    // Set up button callbacks
    button_defaults_.on_select = [this]() { on_load_defaults(); };
    button_save_.on_select = [this]() { on_save_settings(); };
    button_ok_.on_select = [this]() { on_ok_pressed(); };

    // Set up field validation callbacks
    field_min_drone_freq_.on_change = [this](int32_t value) { validate_min_freq(value); };
    field_max_drone_freq_.on_change = [this](int32_t value) { validate_max_freq(value); };
    field_min_detection_count_.on_change = [this](int32_t) { validate_detection_params(); };
    field_hysteresis_margin_.on_change = [this](int32_t) { validate_detection_params(); };
    field_trend_threshold_.on_change = [this](int32_t) { validate_detection_params(); };
    field_default_rssi_threshold_.on_change = [this](int32_t) { validate_detection_params(); };

    update_status_text("Ready - edit values and press OK to apply");
}

void ConstantSettingsView::focus() {
    // Focus on first field for editing
    field_min_drone_freq_.focus();
}

void ConstantSettingsView::initialize_field_values() {
    // Load current values into UI fields (convert Hz to MHz for display)
    field_min_drone_freq_.set_value(ConstantSettingsManager::current_min_drone_freq / 1000000);
    field_max_drone_freq_.set_value(ConstantSettingsManager::current_max_drone_freq / 1000000);
    field_min_detection_count_.set_value(ConstantSettingsManager::current_min_detection_count);
    field_hysteresis_margin_.set_value(ConstantSettingsManager::current_hysteresis_margin_db);
    field_trend_threshold_.set_value(ConstantSettingsManager::current_trend_threshold_db);
    field_default_rssi_threshold_.set_value(ConstantSettingsManager::current_default_rssi_threshold_db);
    field_scan_interval_.set_value(ConstantSettingsManager::current_scan_interval_ms);
    field_stale_timeout_.set_value(ConstantSettingsManager::current_stale_timeout_ms);
}

void ConstantSettingsView::update_status_text(const char* text) {
    text_status_.set(text);
}

bool ConstantSettingsView::validate_all_fields() {
    bool valid = true;

    // Validate frequency ranges
    int32_t min_freq = field_min_drone_freq_.get_value();
    int32_t max_freq = field_max_drone_freq_.get_value();

    if (min_freq < 50 || min_freq > 6000) {  // 50MHz to 6000MHz (6GHz)
        update_status_text("Error: Min frequency out of range (50MHz-6000MHz)");
        valid = false;
    } else if (max_freq < min_freq || max_freq > 6000) {
        update_status_text("Error: Max frequency must be >= Min and <= 6000MHz");
        valid = false;
    }

    // Validate detection parameters
    int32_t min_det = field_min_detection_count_.get_value();
    int32_t hysteresis = field_hysteresis_margin_.get_value();
    int32_t trend_thresh = field_trend_threshold_.get_value();
    int32_t rssi_thresh = field_default_rssi_threshold_.get_value();

    if (min_det < 1 || min_det > 10) {
        update_status_text("Error: Min detection count must be 1-10");
        valid = false;
    } else if (hysteresis < -20 || hysteresis > 20) {
        update_status_text("Error: Hysteresis margin must be -20dB to +20dB");
        valid = false;
    } else if (trend_thresh < 1 || trend_thresh > 10) {
        update_status_text("Error: Trend threshold must be 1-10 dB");
        valid = false;
    } else if (rssi_thresh < -120 || rssi_thresh > -10) {
        update_status_text("Error: Default RSSI threshold must be -120dB to -10dB");
        valid = false;
    }

    if (valid) {
        update_status_text("All values valid - press OK to apply");
    }

    return valid;
}

void ConstantSettingsView::apply_current_values() {
    // Apply current field values to the manager
    ConstantSettingsManager::current_min_drone_freq = field_min_drone_freq_.get_value() * 1000000ULL;
    ConstantSettingsManager::current_max_drone_freq = field_max_drone_freq_.get_value() * 1000000ULL;
    ConstantSettingsManager::current_min_detection_count = field_min_detection_count_.get_value();
    ConstantSettingsManager::current_hysteresis_margin_db = field_hysteresis_margin_.get_value();
    ConstantSettingsManager::current_trend_threshold_db = field_trend_threshold_.get_value();
    ConstantSettingsManager::current_default_rssi_threshold_db = field_default_rssi_threshold_.get_value();
    ConstantSettingsManager::current_scan_interval_ms = field_scan_interval_.get_value();
    ConstantSettingsManager::current_stale_timeout_ms = field_stale_timeout_.get_value();
}

void ConstantSettingsView::validate_min_freq(int32_t value) {
    // Check if min frequency is within hardware limits
    if (value < 50 || value > 6000) {
        update_status_text("Warning: Frequency outside typical range");
    } else {
        validate_all_fields();
    }
}

void ConstantSettingsView::validate_max_freq(int32_t value) {
    // Check if max frequency is >= min frequency
    int32_t min_freq = field_min_drone_freq_.get_value();
    if (value < min_freq) {
        update_status_text("Error: Max frequency must be >= Min frequency");
    } else {
        validate_all_fields();
    }
}

void ConstantSettingsView::validate_detection_params() {
    validate_all_fields();
}

void ConstantSettingsView::on_load_defaults() {
    // Reset manager to defaults and update UI
    ConstantSettingsManager::reset_to_defaults();
    initialize_field_values();
    update_status_text("Default values loaded - press OK to apply");
}

void ConstantSettingsView::on_save_settings() {
    // Validate all fields first
    if (!validate_all_fields()) {
        return;
    }

    // Apply current values and save to persistent storage
    apply_current_values();

    if (ConstantSettingsManager::save_settings()) {
        update_status_text("Settings saved successfully");
    } else {
        update_status_text("Error: Failed to save settings");
    }
}

void ConstantSettingsView::on_ok_pressed() {
    // Validate all fields first
    if (!validate_all_fields()) {
        return;
    }

    // Apply current values (but don't save automatically)
    apply_current_values();
    update_status_text("Settings applied - remember to save if needed");

    // Close the dialog
    nav_.pop();
}

void ConstantSettingsView::on_cancel() {
    // Close without applying changes
    nav_.pop();
}

} // namespace ui::external_app::enhanced_drone_analyzer
