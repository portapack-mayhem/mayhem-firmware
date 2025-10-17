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
// Additional constants initialized with defaults
float ConstantSettingsManager::current_rssi_smoothing_alpha = RSSI_SMOOTHING_ALPHA;
uint32_t ConstantSettingsManager::current_detection_reset_interval = DETECTION_RESET_INTERVAL;
size_t ConstantSettingsManager::current_max_displayed_drones = MAX_DISPLAYED_DRONES;
rf::Frequency ConstantSettingsManager::current_mini_spectrum_min_freq = MINI_SPECTRUM_MIN_FREQ;
rf::Frequency ConstantSettingsManager::current_mini_spectrum_max_freq = MINI_SPECTRUM_MAX_FREQ;
uint16_t ConstantSettingsManager::current_sos_frequency_hz = SOS_FREQUENCY_HZ;
uint32_t ConstantSettingsManager::current_beep_duration_ms = DETECTION_BEEP_DURATION_MS;
float ConstantSettingsManager::current_rssi_threat_weight = RSSI_THREAT_WEIGHT;
float ConstantSettingsManager::current_database_threat_weight = DATABASE_THREAT_WEIGHT;

// Additional hardware and buffer constants
size_t ConstantSettingsManager::current_detection_table_size = DETECTION_TABLE_SIZE;
size_t ConstantSettingsManager::current_max_tracked_drones = MAX_TRACKED_DRONES;
size_t ConstantSettingsManager::current_max_database_entries = MAX_DATABASE_ENTRIES;
size_t ConstantSettingsManager::current_spectrum_buffer_size = SPECTRUM_BUFFER_SIZE;
rf::Frequency ConstantSettingsManager::current_ism_center_freq = ISM_CENTER_FREQ;

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
        {"stale_timeout_ms"sv, &current_stale_timeout_ms},
        // Additional configurable constants
        {"rssi_smoothing_alpha"sv, &current_rssi_smoothing_alpha},
        {"detection_reset_interval"sv, &current_detection_reset_interval},
        {"max_displayed_drones"sv, &current_max_displayed_drones},
        {"mini_spectrum_min_freq"sv, &current_mini_spectrum_min_freq},
        {"mini_spectrum_max_freq"sv, &current_mini_spectrum_max_freq},
        {"sos_frequency_hz"sv, &current_sos_frequency_hz},
        {"beep_duration_ms"sv, &current_beep_duration_ms},
        {"rssi_threat_weight"sv, &current_rssi_threat_weight},
        {"database_threat_weight"sv, &current_database_threat_weight},
        // Additional hardware and buffer constants
        {"detection_table_size"sv, (void*)&current_detection_table_size},
        {"max_tracked_drones"sv, (void*)&current_max_tracked_drones},
        {"max_database_entries"sv, (void*)&current_max_database_entries},
        {"spectrum_buffer_size"sv, (void*)&current_spectrum_buffer_size},
        {"ism_center_freq"sv, &current_ism_center_freq}
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
        {"stale_timeout_ms"sv, &current_stale_timeout_ms},
        // Additional configurable constants
        {"rssi_smoothing_alpha"sv, &current_rssi_smoothing_alpha},
        {"detection_reset_interval"sv, &current_detection_reset_interval},
        {"max_displayed_drones"sv, &current_max_displayed_drones},
        {"mini_spectrum_min_freq"sv, &current_mini_spectrum_min_freq},
        {"mini_spectrum_max_freq"sv, &current_mini_spectrum_max_freq},
        {"sos_frequency_hz"sv, &current_sos_frequency_hz},
        {"beep_duration_ms"sv, &current_beep_duration_ms},
        {"rssi_threat_weight"sv, &current_rssi_threat_weight},
        {"database_threat_weight"sv, &current_database_threat_weight},
        // Additional hardware and buffer constants
        {"detection_table_size"sv, (void*)&current_detection_table_size},
        {"max_tracked_drones"sv, (void*)&current_max_tracked_drones},
        {"max_database_entries"sv, (void*)&current_max_database_entries},
        {"spectrum_buffer_size"sv, (void*)&current_spectrum_buffer_size},
        {"ism_center_freq"sv, &current_ism_center_freq}
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
    // Reset additional constants
    current_rssi_smoothing_alpha = RSSI_SMOOTHING_ALPHA;
    current_detection_reset_interval = DETECTION_RESET_INTERVAL;
    current_max_displayed_drones = MAX_DISPLAYED_DRONES;
    current_mini_spectrum_min_freq = MINI_SPECTRUM_MIN_FREQ;
    current_mini_spectrum_max_freq = MINI_SPECTRUM_MAX_FREQ;
    current_sos_frequency_hz = SOS_FREQUENCY_HZ;
    current_beep_duration_ms = DETECTION_BEEP_DURATION_MS;
    current_rssi_threat_weight = RSSI_THREAT_WEIGHT;
    current_database_threat_weight = DATABASE_THREAT_WEIGHT;
    // Reset additional hardware and buffer constants
    current_detection_table_size = DETECTION_TABLE_SIZE;
    current_max_tracked_drones = MAX_TRACKED_DRONES;
    current_max_database_entries = MAX_DATABASE_ENTRIES;
    current_spectrum_buffer_size = SPECTRUM_BUFFER_SIZE;
    current_ism_center_freq = ISM_CENTER_FREQ;
}

ConstantSettingsView::ConstantSettingsView(NavigationView& nav)
    : nav_(nav) {

    // Add all UI components to the view (including new additional fields)
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
        &text_spectrum_title_,
        &text_rssi_smoothing_,
        &field_rssi_smoothing_,
        &text_detection_reset_,
        &field_detection_reset_,
        &text_max_drones_,
        &field_max_drones_,
        &text_spectrum_min_,
        &field_spectrum_min_,
        &text_spectrum_max_,
        &field_spectrum_max_,
        &text_audio_title_,
        &text_sos_freq_,
        &field_sos_freq_,
        &text_beep_duration_,
        &field_beep_duration_,
        &text_threat_title_,
        &text_rssi_weight_,
        &field_rssi_weight_,
        &text_db_weight_,
        &field_db_weight_,
        &text_hardware_title_,
        &text_detection_table_,
        &field_detection_table_,
        &text_max_tracked_,
        &field_max_tracked_,
        &text_max_db_,
        &field_max_db_,
        &text_spectrum_buffer_,
        &field_spectrum_buffer_,
        &text_ism_center_,
        &field_ism_center_,
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

    // Set up field validation callbacks (extended for new fields)
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

    // Initialize additional fields with current values
    field_rssi_smoothing_.set_value(static_cast<int32_t>(ConstantSettingsManager::current_rssi_smoothing_alpha * 100)); // convert to int for display
    field_detection_reset_.set_value(ConstantSettingsManager::current_detection_reset_interval);
    field_max_drones_.set_value(ConstantSettingsManager::current_max_displayed_drones);
    field_spectrum_min_.set_value(ConstantSettingsManager::current_mini_spectrum_min_freq / 1000000000); // GHz in MHz field
    field_spectrum_max_.set_value(ConstantSettingsManager::current_mini_spectrum_max_freq / 1000000000);
    field_sos_freq_.set_value(ConstantSettingsManager::current_sos_frequency_hz);
    field_beep_duration_.set_value(ConstantSettingsManager::current_beep_duration_ms);
    field_rssi_weight_.set_value(static_cast<int32_t>(ConstantSettingsManager::current_rssi_threat_weight * 100));
    field_db_weight_.set_value(static_cast<int32_t>(ConstantSettingsManager::current_database_threat_weight * 100));

    // Initialize new hardware and buffer constants
    field_detection_table_.set_value(ConstantSettingsManager::current_detection_table_size);
    field_max_tracked_.set_value(ConstantSettingsManager::current_max_tracked_drones);
    field_max_db_.set_value(ConstantSettingsManager::current_max_database_entries);
    field_spectrum_buffer_.set_value(ConstantSettingsManager::current_spectrum_buffer_size);
    field_ism_center_.set_value(ConstantSettingsManager::current_ism_center_freq / 1000000);  // Hz to MHz
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

    // Apply additional field values
    ConstantSettingsManager::current_rssi_smoothing_alpha = field_rssi_smoothing_.get_value() / 100.0f; // convert back from int
    ConstantSettingsManager::current_detection_reset_interval = field_detection_reset_.get_value();
    ConstantSettingsManager::current_max_displayed_drones = field_max_drones_.get_value();
    ConstantSettingsManager::current_mini_spectrum_min_freq = field_spectrum_min_.get_value() * 1000000000ULL; // convert GHz back to Hz
    ConstantSettingsManager::current_mini_spectrum_max_freq = field_spectrum_max_.get_value() * 1000000000ULL; // convert GHz back to Hz
    ConstantSettingsManager::current_sos_frequency_hz = field_sos_freq_.get_value();
    ConstantSettingsManager::current_beep_duration_ms = field_beep_duration_.get_value();
    ConstantSettingsManager::current_rssi_threat_weight = field_rssi_weight_.get_value() / 100.0f;
    ConstantSettingsManager::current_database_threat_weight = field_db_weight_.get_value() / 100.0f;

    // Apply new hardware and buffer constants
    ConstantSettingsManager::current_detection_table_size = field_detection_table_.get_value();
    ConstantSettingsManager::current_max_tracked_drones = field_max_tracked_.get_value();
    ConstantSettingsManager::current_max_database_entries = field_max_db_.get_value();
    ConstantSettingsManager::current_spectrum_buffer_size = field_spectrum_buffer_.get_value();
    ConstantSettingsManager::current_ism_center_freq = field_ism_center_.get_value() * 1000000ULL;
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
