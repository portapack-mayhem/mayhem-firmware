// ui_drone_settings.cpp - Implementation of drone analyzer settings management

#include "ui_drone_settings.hpp"
#include <string>
#include <sstream>

namespace ui::external_app::enhanced_drone_analyzer {

// Reset all settings to factory defaults
void DroneAnalyzerSettings::reset_to_defaults() {
    spectrum_mode = SpectrumMode::MEDIUM;

    // Spectrum settings
    spectrum.min_scan_interval_ms = 1000u;
    spectrum.stale_timeout_ms = STALE_DRONE_TIMEOUT_MS;
    spectrum.default_rssi_threshold = DEFAULT_RSSI_THRESHOLD_DB;
    spectrum.rssi_smoothing_alpha = 0.6f;

    // Detection settings
    detection.min_detection_count = 5u;
    detection.hysteresis_margin_db = HYSTERESIS_MARGIN_DB;
    detection.trend_threshold_db = 5;
    detection.detection_reset_interval = 100u;

    // Audio settings
    audio.alert_frequency_hz = SOS_FREQUENCY_HZ;
    audio.beep_duration_ms = DETECTION_BEEP_DURATION_MS;
    audio.alert_squelch_db = -80;

    // Display settings
    display.show_rssi_graph = true;
    display.show_trends = true;
    display.max_display_drones = MAX_DISPLAYED_DRONES;
    display.color_scheme = 0;
}

// Serialize settings for SettingsManager storage
// Сейчас упрощенная сериализация, позже можно сделать JSON
std::string DroneAnalyzerSettings::serialize() const {
    std::stringstream ss;

    // Версия формата для будущей совместимости
    ss << "v1:";

    // Spectrum settings
    ss << spectrum_mode << ":";
    ss << spectrum.min_scan_interval_ms << ":";
    ss << spectrum.stale_timeout_ms << ":";
    ss << spectrum.default_rssi_threshold << ":";
    ss << spectrum.rssi_smoothing_alpha << ":";

    // Detection settings
    ss << static_cast<int>(detection.min_detection_count) << ":";
    ss << detection.hysteresis_margin_db << ":";
    ss << detection.trend_threshold_db << ":";
    ss << detection.detection_reset_interval << ":";

    // Audio settings
    ss << audio.alert_frequency_hz << ":";
    ss << audio.beep_duration_ms << ":";
    ss << audio.alert_squelch_db << ":";

    // Display settings
    ss << (display.show_rssi_graph ? "1" : "0") << ":";
    ss << (display.show_trends ? "1" : "0") << ":";
    ss << display.max_display_drones << ":";
    ss << (display.color_scheme ? "1" : "0");

    return ss.str();
}

// Deserialize settings from SettingsManager storage
bool DroneAnalyzerSettings::deserialize(const std::string& data) {
    std::stringstream ss(data);
    std::string token;

    // Check version
    if (!std::getline(ss, token, ':')) return false;
    if (token != "v1") return false; // Unsupported version

    auto parse_int = [&](int& value) {
        if (!std::getline(ss, token, ':')) return false;
        value = std::stoi(token);
        return true;
    };

    auto parse_uint = [&](uint32_t& value) {
        if (!std::getline(ss, token, ':')) return false;
        value = static_cast<uint32_t>(std::stoul(token));
        return true;
    };

    auto parse_float = [&](float& value) {
        if (!std::getline(ss, token, ':')) return false;
        value = std::stof(token);
        return true;
    };

    auto parse_bool = [&](bool& value) {
        if (!std::getline(ss, token, ':')) return false;
        value = (token == "1");
        return true;
    };

    try {
        int spectrum_mode_int;
        if (!parse_int(spectrum_mode_int)) return false;
        spectrum_mode = static_cast<SpectrumMode>(spectrum_mode_int);

        // Spectrum
        if (!parse_uint(spectrum.min_scan_interval_ms)) return false;
        if (!parse_uint(spectrum.stale_timeout_ms)) return false;
        if (!parse_int(spectrum.default_rssi_threshold)) return false;
        if (!parse_float(spectrum.rssi_smoothing_alpha)) return false;

        // Detection
        int min_det_count;
        if (!parse_int(min_det_count)) return false;
        detection.min_detection_count = static_cast<uint8_t>(min_det_count);
        if (!parse_int(detection.hysteresis_margin_db)) return false;
        if (!parse_int(detection.trend_threshold_db)) return false;
        if (!parse_uint(detection.detection_reset_interval)) return false;

        // Audio
        if (!parse_uint(audio.alert_frequency_hz)) return false;
        if (!parse_uint(audio.beep_duration_ms)) return false;
        if (!parse_int(audio.alert_squelch_db)) return false;

        // Display
        if (!parse_bool(display.show_rssi_graph)) return false;
        if (!parse_bool(display.show_trends)) return false;
        size_t max_drones;
        if (!parse_uint(max_drones)) return false;
        display.max_display_drones = max_drones;
        if (!parse_bool(display.color_scheme)) return false;

        return true;
    } catch (const std::exception&) {
        return false; // Parse error
    }
}

// Get default settings instance
const DroneAnalyzerSettings& get_default_settings() {
    static DroneAnalyzerSettings defaults;
    static bool initialized = false;

    if (!initialized) {
        defaults.reset_to_defaults();
        initialized = true;
    }

    return defaults;
}

} // namespace ui::external_app::enhanced_drone_analyzer
