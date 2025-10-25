// ui_settings_combined.cpp - Unified implementation for Enhanced Drone Analyzer Settings App
// Combines implementations from Settings UI classes and manager classes

#include "ui_settings_combined.hpp"
#include <algorithm>
#include <sstream>
#include <fstream>

namespace ui::external_app::enhanced_drone_analyzer {

// ===========================================
// PART 1: SETTINGS MANAGER IMPLEMENTATION (from ui_drone_config.hpp)
// ===========================================

Language DroneAnalyzerSettingsManager::current_language_ = Language::ENGLISH;

const std::map<std::string, const char*> DroneAnalyzerSettingsManager::translations_english = {
    {"save_settings", "Save Settings"},
    {"load_settings", "Load Settings"},
    {"audio_settings", "Audio Settings"},
    {"hardware_settings", "Hardware Settings"},
    {"scan_interval", "Scan Interval"},
    {"rssi_threshold", "RSSI Threshold"},
    {"spectrum_mode", "Spectrum Mode"}
};

DroneAnalyzerSettingsManager& get_settings_manager() {
    static DroneAnalyzerSettingsManager instance;
    return instance;
}

bool DroneAnalyzerSettingsManager::load(DroneAnalyzerSettings& settings) {
    if (settings.settings_file_path.empty()) {
        settings.settings_file_path = "/sdcard/ENHANCED_DRONE_ANALYZER_SETTINGS.txt";
    }

    std::ifstream file(settings.settings_file_path);
    if (!file.is_open()) {
        reset_to_defaults(settings);
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t equals_pos = line.find('=');
        if (equals_pos != std::string::npos) {
            std::string key = line.substr(0, equals_pos);
            std::string value = line.substr(equals_pos + 1);

            // Parse key-value pairs and update settings
            if (key == "spectrum_mode") {
                if (value == "NARROW") settings.spectrum_mode = SpectrumMode::NARROW;
                else if (value == "MEDIUM") settings.spectrum_mode = SpectrumMode::MEDIUM;
                else if (value == "WIDE") settings.spectrum_mode = SpectrumMode::WIDE;
                else if (value == "ULTRA_WIDE") settings.spectrum_mode = SpectrumMode::ULTRA_WIDE;
            }
            else if (key == "scan_interval_ms") settings.scan_interval_ms = std::stoul(value);
            else if (key == "rssi_threshold_db") settings.rssi_threshold_db = std::stoi(value);
            else if (key == "enable_audio_alerts") settings.enable_audio_alerts = (value == "true");
            else if (key == "audio_alert_frequency_hz") settings.audio_alert_frequency_hz = std::stoul(value);
            else if (key == "audio_alert_duration_ms") settings.audio_alert_duration_ms = std::stoul(value);
            else if (key == "enable_real_hardware") settings.enable_real_hardware = (value == "true");
            else if (key == "demo_mode") settings.demo_mode = (value == "true");
            else if (key == "hardware_bandwidth_hz") settings.hardware_bandwidth_hz = std::stoul(value);
        }
    }

    return validate(settings);
}

bool DroneAnalyzerSettingsManager::save(const DroneAnalyzerSettings& settings) {
    if (settings.settings_file_path.empty()) {
        // Default path on SD card
        const_cast<DroneAnalyzerSettings&>(settings).settings_file_path = "/sdcard/ENHANCED_DRONE_ANALYZER_SETTINGS.txt";
    }

    std::ofstream file(settings.settings_file_path);
    if (!file.is_open()) {
        return false;
    }

    // Write settings in key=value format
    file << "spectrum_mode=";
    switch (settings.spectrum_mode) {
        case SpectrumMode::NARROW: file << "NARROW"; break;
        case SpectrumMode::MEDIUM: file << "MEDIUM"; break;
        case SpectrumMode::WIDE: file << "WIDE"; break;
        case SpectrumMode::ULTRA_WIDE: file << "ULTRA_WIDE"; break;
    }
    file << "\n";

    file << "scan_interval_ms=" << settings.scan_interval_ms << "\n";
    file << "rssi_threshold_db=" << settings.rssi_threshold_db << "\n";
    file << "enable_audio_alerts=" << (settings.enable_audio_alerts ? "true" : "false") << "\n";
    file << "audio_alert_frequency_hz=" << settings.audio_alert_frequency_hz << "\n";
    file << "audio_alert_duration_ms=" << settings.audio_alert_duration_ms << "\n";
    file << "enable_real_hardware=" << (settings.enable_real_hardware ? "true" : "false") << "\n";
    file << "demo_mode=" << (settings.demo_mode ? "true" : "false") << "\n";
    file << "hardware_bandwidth_hz=" << settings.hardware_bandwidth_hz << "\n";
    file << "freqman_path=" << settings.freqman_path << "\n";

    return file.good();
}

void DroneAnalyzerSettingsManager::reset_to_defaults(DroneAnalyzerSettings& settings) {
    settings.spectrum_mode = SpectrumMode::MEDIUM;
    settings.scan_interval_ms = 1000;
    settings.rssi_threshold_db = DEFAULT_RSSI_THRESHOLD_DB;
    settings.enable_audio_alerts = true;
    settings.audio_alert_frequency_hz = 800;
    settings.audio_alert_duration_ms = 500;
    settings.enable_wideband_scanning = false;
    settings.wideband_min_freq_hz = WIDEBAND_DEFAULT_MIN;
    settings.wideband_max_freq_hz = WIDEBAND_DEFAULT_MAX;
    settings.min_frequency_hz = 2400000000ULL;
    settings.max_frequency_hz = 2500000000ULL;
    settings.show_detailed_info = true;
    settings.auto_save_logs = true;
    settings.log_file_path = "/eda_logs";
    settings.freqman_path = "DRONES";
    settings.settings_file_path = "/sdcard/ENHANCED_DRONE_ANALYZER_SETTINGS.txt";
    settings.hardware_bandwidth_hz = 24000000;
    settings.enable_real_hardware = true;
    settings.demo_mode = false;
}

bool DroneAnalyzerSettingsManager::validate(const DroneAnalyzerSettings& settings) {
    if (settings.scan_interval_ms < 100 || settings.scan_interval_ms > 10000) return false;
    if (settings.rssi_threshold_db < -120 || settings.rssi_threshold_db > -30) return false;
    if (settings.audio_alert_frequency_hz < 200 || settings.audio_alert_frequency_hz > 3000) return false;
    if (settings.audio_alert_duration_ms < 50 || settings.audio_alert_duration_ms > 2000) return false;
    if (settings.hardware_bandwidth_hz < 1000000 || settings.hardware_bandwidth_hz > 100000000) return false;
    return true;
}

std::string DroneAnalyzerSettingsManager::serialize(const DroneAnalyzerSettings& settings) {
    std::ostringstream oss;

    oss << "spectrum_mode=";
    switch (settings.spectrum_mode) {
        case SpectrumMode::NARROW: oss << "NARROW"; break;
        case SpectrumMode::MEDIUM: oss << "MEDIUM"; break;
        case SpectrumMode::WIDE: oss << "WIDE"; break;
        case SpectrumMode::ULTRA_WIDE: oss << "ULTRA_WIDE"; break;
    }
    oss << "|scan_interval_ms=" << settings.scan_interval_ms;
    oss << "|rssi_threshold_db=" << settings.rssi_threshold_db;
    oss << "|enable_audio_alerts=" << (settings.enable_audio_alerts ? "true" : "false");
    // Add more fields as needed...

    return oss.str();
}

bool DroneAnalyzerSettingsManager::deserialize(DroneAnalyzerSettings& settings, const std::string& data) {
    std::istringstream iss(data);
    std::string token;

    while (std::getline(iss, token, '|')) {
        size_t equals_pos = token.find('=');
        if (equals_pos != std::string::npos) {
            std::string key = token.substr(0, equals_pos);
            std::string value = token.substr(equals_pos + 1);

            // Parse and apply values
            if (key == "spectrum_mode") {
                if (value == "NARROW") settings.spectrum_mode = SpectrumMode::NARROW;
                else if (value == "MEDIUM") settings.spectrum_mode = SpectrumMode::MEDIUM;
                else if (value == "WIDE") settings.spectrum_mode = SpectrumMode::WIDE;
                else if (value == "ULTRA_WIDE") settings.spectrum_mode = SpectrumMode::ULTRA_WIDE;
            }
            else if (key == "scan_interval_ms") settings.scan_interval_ms = std::stoul(value);
            else if (key == "rssi_threshold_db") settings.rssi_threshold_db = std::stoi(value);
            else if (key == "enable_audio_alerts") settings.enable_audio_alerts = (value == "true");
        }
    }

    return validate(settings);
}

const char* DroneAnalyzerSettingsManager::translate(const std::string& key) {
    auto it = translations_english.find(key);
    if (it != translations_english.end()) {
        return it->second;
    }
    return key.c_str();  // Fallback to the key itself
}

const char* DroneAnalyzerSettingsManager::get_translation(const std::string& key) {
    return translate(key);
}

// ============ ScannerConfig Implementation ============

ScannerConfig::ScannerConfig(ConfigData config)
    : config_data_(config) {
}

bool ScannerConfig::load_from_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        size_t equals_pos = line.find('=');
        if (equals_pos != std::string::npos) {
            std::string key = line.substr(0, equals_pos);
            std::string value = line.substr(equals_pos + 1);

            if (key == "spectrum_mode") {
                if (value == "NARROW") config_data_.spectrum_mode = SpectrumMode::NARROW;
                else if (value == "MEDIUM") config_data_.spectrum_mode = SpectrumMode::MEDIUM;
                else if (value == "WIDE") config_data_.spectrum_mode = SpectrumMode::WIDE;
                else if (value == "ULTRA_WIDE") config_data_.spectrum_mode = SpectrumMode::ULTRA_WIDE;
            }
            else if (key == "rssi_threshold_db") config_data_.rssi_threshold_db = std::stoi(value);
            else if (key == "scan_interval_ms") config_data_.scan_interval_ms = std::stoul(value);
            else if (key == "enable_audio_alerts") config_data_.enable_audio_alerts = (value == "true");
            else if (key == "freqman_path") config_data_.freqman_path = value;
        }
    }

    return true;
}

bool ScannerConfig::save_to_file(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "spectrum_mode=";
    switch (config_data_.spectrum_mode) {
        case SpectrumMode::NARROW: file << "NARROW"; break;
        case SpectrumMode::MEDIUM: file << "MEDIUM"; break;
        case SpectrumMode::WIDE: file << "WIDE"; break;
        case SpectrumMode::ULTRA_WIDE: file << "ULTRA_WIDE"; break;
    }
    file << "\n";

    file << "rssi_threshold_db=" << config_data_.rssi_threshold_db << "\n";
    file << "scan_interval_ms=" << config_data_.scan_interval_ms << "\n";
    file << "enable_audio_alerts=" << (config_data_.enable_audio_alerts ? "true" : "false") << "\n";
    file << "freqman_path=" << config_data_.freqman_path << "\n";

    return file.good();
}

void ScannerConfig::set_frequency_range(uint32_t min_hz, uint32_t max_hz) {
    (void)min_hz; (void)max_hz;
    // Implementation for frequency range setting
}

void ScannerConfig::set_rssi_threshold(int32_t threshold) {
    config_data_.rssi_threshold_db = threshold;
}

void ScannerConfig::set_scan_interval(uint32_t interval_ms) {
    config_data_.scan_interval_ms = interval_ms;
}

void ScannerConfig::set_audio_alerts(bool enabled) {
    config_data_.enable_audio_alerts = enabled;
}

void ScannerConfig::set_freqman_path(const std::string& path) {
    config_data_.freqman_path = path;
}

void ScannerConfig::set_scanning_mode(const std::string& mode) {
    if (mode == "Database") config_data_.spectrum_mode = SpectrumMode::MEDIUM;
    else if (mode == "Wideband") config_data_.spectrum_mode = SpectrumMode::WIDE;
    // Add more modes as needed
}

bool ScannerConfig::is_valid() const {
    return config_data_.rssi_threshold_db >= -120 && config_data_.rssi_threshold_db <= -30 &&
           config_data_.scan_interval_ms >= 100 && config_data_.scan_interval_ms <= 10000;
}

// ============ DroneFrequencyPresets Implementation ============

static const std::vector<DronePreset> default_presets = {
    {"2.4GHz Band Scan", "Drone_2_4GHz", 2400000000ULL, ThreatLevel::MEDIUM, DroneType::MAVIC},
    {"2.5GHz Band Scan", "Drone_2_5GHz", 2500000000ULL, ThreatLevel::HIGH, DroneType::PHANTOM},
    {"DJI Mavic Series", "DJI_Mavic", 2437000000ULL, ThreatLevel::HIGH, DroneType::DJI_MINI},
    {"Parrot Anafi", "Parrot_Anafi", 2450000000ULL, ThreatLevel::MEDIUM, DroneType::PARROT_ANAFI},
    {"Military UAV Band", "Military_UAV", 5000000000ULL, ThreatLevel::CRITICAL, DroneType::MILITARY_DRONE}
};

const std::vector<DronePreset>& DroneFrequencyPresets::get_all_presets() {
    return default_presets;
}

std::vector<DroneType> DroneFrequencyPresets::get_available_types() {
    return {DroneType::MAVIC, DroneType::PHANTOM, DroneType::DJI_MINI,
            DroneType::PARROT_ANAFI, DroneType::PARROT_BEBOP,
            DroneType::PX4_DRONE, DroneType::MILITARY_DRONE};
}

std::string DroneFrequencyPresets::get_type_display_name(DroneType type) {
    switch (type) {
        case DroneType::MAVIC: return "DJI Mavic";
        case DroneType::PHANTOM: return "DJI Phantom";
        case DroneType::DJI_MINI: return "DJI Mini";
        case DroneType::PARROT_ANAFI: return "Parrot Anafi";
        case DroneType::PARROT_BEBOP: return "Parrot Bebop";
        case DroneType::PX4_DRONE: return "PX4 Drone";
        case DroneType::MILITARY_DRONE: return "Military UAV";
        default: return "Unknown";
    }
}

std::vector<DronePreset> DroneFrequencyPresets::get_presets_of_type(const std::vector<DronePreset>& all_presets, DroneType type) {
    std::vector<DronePreset> filtered;
    std::copy_if(all_presets.begin(), all_presets.end(), std::back_inserter(filtered),
                 [type](const DronePreset& preset) { return preset.drone_type == type; });
    return filtered;
}

bool DroneFrequencyPresets::apply_preset(ScannerConfig& config, const DronePreset& preset) {
    (void)config; (void)preset;
    // Implementation to apply preset to ScannerConfig
    return true;
}

// ============ DronePresetSelector Implementation ============

void DronePresetSelector::show_preset_menu(NavigationView& nav, PresetMenuView callback) {
    auto preset_names = DroneFrequencyPresets::get_preset_names();
    auto all_presets = DroneFrequencyPresets::get_all_presets();

    class PresetMenuView : public MenuView {
    public:
        PresetMenuView(std::vector<std::string> names, std::function<void(const DronePreset&)> on_selected,
                      const std::vector<DronePreset>& presets)
            : MenuView(), names_(names), on_selected_fn_(on_selected), presets_(presets) {
            for (const auto& name : names) {
                add_item({name, Color::white()});
            }
        }

        bool on_key(const KeyEvent key) override {
            if (key == KeyEvent::Select) {
                size_t idx = highlighted_index();
                if (idx < presets_.size()) {
                    if (on_selected_fn_) {
                        on_selected_fn_(presets_[idx]);
                    }
                }
                return true;
            }
            return MenuView::on_key(key);
        }

    private:
        std::vector<std::string> names_;
        std::function<void(const DronePreset&)> on_selected_fn_;
        const std::vector<DronePreset>& presets_;
    };

    nav.push<PresetMenuView>(preset_names, callback, all_presets);
}

void DronePresetSelector::show_type_filtered_presets(NavigationView& nav, DroneType type, FilteredPresetMenuView callback) {
    auto filtered_presets = DroneFrequencyPresets::get_presets_of_type(DroneFrequencyPresets::get_all_presets(), type);
    std::vector<std::string> names;
    for (const auto& preset : filtered_presets) {
        names.push_back(preset.display_name);
    }

    class FilteredPresetMenuView : public MenuView {
    public:
        FilteredPresetMenuView(std::vector<std::string> names, std::function<void(const DronePreset&)> on_selected,
                              const std::vector<DronePreset>& presets)
            : MenuView(), names_(names), on_selected_fn_(on_selected), presets_(presets) {
            for (const auto& name : names) {
                add_item({name, Color::white()});
            }
        }

        bool on_key(const KeyEvent key) override {
            if (key == KeyEvent::Select) {
                size_t idx = highlighted_index();
                if (idx < presets_.size()) {
                    if (on_selected_fn_) {
                        on_selected_fn_(presets_[idx]);
                    }
                }
                return true;
            }
            return MenuView::on_key(key);
        }

    private:
        std::vector<std::string> names_;
        std::function<void(const DronePreset&)> on_selected_fn_;
        const std::vector<DronePreset>& presets_;
    };

    nav.push<FilteredPresetMenuView>(names, callback, filtered_presets);
}

PresetMenuView DronePresetSelector::create_config_updater(ScannerConfig& config_to_update) {
    // Return lambda that updates config when preset selected
    return [&config_to_update](const DronePreset& preset) {
        // Apply preset to config
        DroneFrequencyPresets::apply_preset(config_to_update, preset);
    };
}

// ============ SimpleDroneValidation Implementation ============

bool SimpleDroneValidation::validate_frequency_range(Frequency freq_hz) {
    return freq_hz >= MIN_HARDWARE_FREQ && freq_hz <= MAX_HARDWARE_FREQ;
}

bool SimpleDroneValidation::validate_rssi_signal(int32_t rssi_db, ThreatLevel threat) {
    (void)threat;
    return rssi_db >= -120 && rssi_db <= 0;
}

ThreatLevel SimpleDroneValidation::classify_signal_strength(int32_t rssi_db) {
    if (rssi_db >= -70) return ThreatLevel::HIGH;
    if (rssi_db >= -80) return ThreatLevel::MEDIUM;
    return ThreatLevel::LOW;
}

DroneType SimpleDroneValidation::identify_drone_type(Frequency freq_hz, int32_t rssi_db) {
    (void)rssi_db;

    // Basic frequency-based identification
    if (freq_hz >= 2400000000ULL && freq_hz <= 2500000000ULL) {
        if (freq_hz >= 2430000000ULL && freq_hz <= 2480000000ULL) {
            return DroneType::MAVIC;
        } else if (freq_hz >= 5200000000ULL && freq_hz <= 5800000000ULL) {
            return DroneType::MILITARY_DRONE;
        }
        return DroneType::PHANTOM;
    }

    return DroneType::UNKNOWN;
}

bool SimpleDroneValidation::validate_drone_detection(Frequency freq_hz, int32_t rssi_db,
                                                   DroneType type, ThreatLevel threat) {
    return validate_frequency_range(freq_hz) && validate_rssi_signal(rssi_db, threat) && type != DroneType::UNKNOWN;
}

// ============ DroneFrequencyEntry Implementation ============

DroneFrequencyEntry::DroneFrequencyEntry(Frequency freq, DroneType type, ThreatLevel threat,
                                       int32_t rssi_thresh, uint32_t bw_hz, const char* desc)
    : frequency_hz(freq), drone_type(type), threat_level(threat),
      rssi_threshold_db(rssi_thresh), bandwidth_hz(bw_hz), description(desc) {
}

bool DroneFrequencyEntry::is_valid() const {
    return SimpleDroneValidation::validate_frequency_range(frequency_hz) &&
           rssi_threshold_db >= -120 && rssi_threshold_db <= 0 &&
           bandwidth_hz > 0;
}

// ===========================================
// PART 2: UI IMPLEMENTATIONS
// ===========================================

#include "ui_widget.hpp"
#include "app_settings.hpp"

// HardwareSettingsView Implementation
HardwareSettingsView::HardwareSettingsView(NavigationView& nav)
    : nav_(nav)
{
    // Using proper PortaPack UI widgets
    add_children({
        &checkbox_real_hardware_,
        &text_real_hardware_
    });

    load_current_settings();
}

void HardwareSettingsView::focus() {
    button_save_.focus();
}

std::string HardwareSettingsView::title() const {
    return "Hardware Settings";
}

void HardwareSettingsView::load_current_settings() {
    // Load settings from TXT file or defaults
    DroneAnalyzerSettings settings;
    if (!DroneAnalyzerSettingsManager::load(settings)) {
        DroneAnalyzerSettingsManager::reset_to_defaults(settings);
    }

    // Set UI elements
    checkbox_real_hardware_.set_value(settings.enable_real_hardware);

    size_t mode_idx = 0;
    switch (settings.spectrum_mode) {
        case SpectrumMode::ULTRA_NARROW: mode_idx = 0; break;
        case SpectrumMode::NARROW: mode_idx = 1; break;
        case SpectrumMode::MEDIUM: mode_idx = 2; break;
        case SpectrumMode::WIDE: mode_idx = 3; break;
        case SpectrumMode::ULTRA_WIDE: mode_idx = 4; break;
    }
    field_spectrum_mode_.set_value(mode_idx);

    // Note: Hardware control not available in settings app, settings are passed to scanner
}

void HardwareSettingsView::save_current_settings() {
    DroneAnalyzerSettings settings;
    DroneAnalyzerSettingsManager::load(settings); // Load current

    settings.enable_real_hardware = checkbox_real_hardware_.value();
    settings.demo_mode = !checkbox_real_hardware_.value();

    size_t mode_idx = field_spectrum_mode_.selected_index();
    switch (mode_idx) {
        case 0: settings.spectrum_mode = SpectrumMode::ULTRA_NARROW; break;
        case 1: settings.spectrum_mode = SpectrumMode::NARROW; break;
        case 2: settings.spectrum_mode = SpectrumMode::MEDIUM; break;
        case 3: settings.spectrum_mode = SpectrumMode::WIDE; break;
        case 4: settings.spectrum_mode = SpectrumMode::ULTRA_WIDE; break;
    }

    DroneAnalyzerSettingsManager::save(settings);
}

void HardwareSettingsView::update_ui_from_settings() {
    load_current_settings();
}

void HardwareSettingsView::update_settings_from_ui() {
    save_current_settings();
}

void HardwareSettingsView::on_save_settings() {
    save_current_settings();
    nav_.display_modal("Success", "Hardware settings saved\nto TXT file");
}

AudioSettingsView::AudioSettingsView(NavigationView& nav)
    : View(), nav_(nav)
{
}

void AudioSettingsView::focus() {
    button_save_.focus();
}

std::string AudioSettingsView::title() const {
    return "Audio Settings";
}

void AudioSettingsView::load_current_settings() {
    // Load from settings TXT
    DroneAnalyzerSettings settings;
    if (!DroneAnalyzerSettingsManager::load(settings)) {
        DroneAnalyzerSettingsManager::reset_to_defaults(settings);
    }

    checkbox_audio_enabled_.set_value(settings.enable_audio_alerts);
    number_alert_frequency_.set_value(settings.audio_alert_frequency_hz);
    number_alert_duration_.set_value(settings.audio_alert_duration_ms);

    // Volume not implemented yet
    // Repeat alerts not implemented yet
}

void AudioSettingsView::save_current_settings() {
    DroneAnalyzerSettings settings;
    DroneAnalyzerSettingsManager::load(settings); // Load current

    settings.enable_audio_alerts = checkbox_audio_enabled_.value();
    settings.audio_alert_frequency_hz = number_alert_frequency_.value();
    settings.audio_alert_duration_ms = number_alert_duration_.value();

    DroneAnalyzerSettingsManager::save(settings); // Save back to TXT
}

void AudioSettingsView::update_ui_from_settings() {
    load_current_settings();
}

void AudioSettingsView::update_settings_from_ui() {
    save_current_settings();
}

void AudioSettingsView::on_save_settings() {
    save_current_settings();
    nav_.display_modal("Success", "Audio settings saved\nto TXT file");
}

} // namespace ui::external_app::enhanced_drone_analyzer
