// Consolidated configuration and types for Enhanced Drone Analyzer

#ifndef __UI_DRONE_CONFIG_HPP__
#define __UI_DRONE_CONFIG_HPP__

#include <cstdint>
#include <vector>
#include <string>
#include <set>
#include <functional>                // For std::function in lambdas
#include "radio_state.hpp"          // Required for rf::Frequency type

namespace ui::external_app::enhanced_drone_analyzer {

// Define constants early to avoid scope issues
static constexpr int16_t TREND_THRESHOLD_DB = 3;
static constexpr uint32_t WIDEBAND_MAX_SLICES = 32;
static constexpr rf::Frequency WIDEBAND_DEFAULT_MIN = 1000000;
static constexpr rf::Frequency WIDEBAND_DEFAULT_MAX = 6000000000;

// Other constants
static constexpr uint32_t SCAN_THREAD_STACK_SIZE = 2048;
static constexpr rf::Frequency WIDEBAND_SLICE_WIDTH = 2500000;
static constexpr uint32_t WIDEBAND_BIN_NB = 256;
static constexpr uint32_t WIDEBAND_BIN_NB_NO_DC = WIDEBAND_BIN_NB - 16;
static constexpr uint32_t WIDEBAND_BIN_WIDTH = WIDEBAND_SLICE_WIDTH / WIDEBAND_BIN_NB;
static constexpr uint8_t WIDEBAND_DC_LOWER_BOUND = 122;
static constexpr uint8_t WIDEBAND_DC_UPPER_BOUND = 134;
static constexpr uint8_t WIDEBAND_EDGE_MARGIN = 2;
static constexpr size_t MAX_TRACKED_DRONES = 8;
static constexpr size_t MAX_DATABASE_ENTRIES = 64;
static constexpr size_t SPECTRUM_BUFFER_SIZE = 256;
static constexpr size_t DETECTION_TABLE_SIZE = 512;
static constexpr int32_t SPECTRUM_BANDWIDTHS[] = {
    4000000,
    8000000,
    12000000,
    20000000,
    24000000
};
static constexpr int32_t SPECTRUM_SAMPLING_RATES[] = {
    4000000,
    8000000,
    12000000,
    20000000,
    24000000
};
static constexpr uint8_t MIN_DETECTION_COUNT = 3;
static constexpr int32_t HYSTERESIS_MARGIN_DB = 5;
static constexpr float RSSI_SMOOTHING_ALPHA = 0.3f;
static constexpr int32_t DEFAULT_RSSI_THRESHOLD_DB = -90;
static constexpr int32_t WIDEBAND_RSSI_THRESHOLD_DB = -85;
static constexpr int32_t WIDEBAND_DYNAMIC_THRESHOLD_OFFSET_DB = 5;
static constexpr uint32_t MIN_SCAN_INTERVAL_MS = 500;
static constexpr uint32_t STALE_DRONE_TIMEOUT_MS = 30000;
static constexpr uint32_t DETECTION_RESET_INTERVAL = 50;
static constexpr uint32_t HYBRID_WIDEBAND_RATIO = 4;
static constexpr uint32_t DRONE_BANDWIDTH_PRESETS[] = {
    3000000,
    6000000,
    10000000,
    20000000,
    0
};
static constexpr uint16_t SOS_FREQUENCY_HZ = 1500;
static constexpr uint32_t DETECTION_BEEP_DURATION_MS = 200;
static constexpr size_t MINI_SPECTRUM_WIDTH = 240;
static constexpr size_t MINI_SPECTRUM_HEIGHT = 24;
static constexpr uint32_t MINI_SPECTRUM_Y_START = 180;
static constexpr size_t MAX_DISPLAYED_DRONES = 3;
static constexpr rf::Frequency MINI_SPECTRUM_MIN_FREQ = 2400000000ULL;
static constexpr rf::Frequency MINI_SPECTRUM_MAX_FREQ = 2500000000ULL;
static constexpr rf::Frequency MIN_HARDWARE_FREQ = 50000000ULL;
static constexpr rf::Frequency MAX_HARDWARE_FREQ = 6000000000ULL;
static constexpr rf::Frequency MIN_DRONE_FREQUENCY = 240000000ULL;
static constexpr rf::Frequency MAX_DRONE_FREQUENCY = 6000000000ULL;
static constexpr rf::Frequency ISM_CENTER_FREQ = 433000000ULL;
static constexpr float RSSI_THREAT_WEIGHT = 0.4f;
static constexpr float DATABASE_THREAT_WEIGHT = 0.6f;
static constexpr size_t DRONE_TRACKING_SIZE = 15;
static constexpr size_t TOTAL_TRACKING_MEMORY = MAX_TRACKED_DRONES * DRONE_TRACKING_SIZE;
static constexpr size_t DETECTION_MEMORY_SIZE = DETECTION_TABLE_SIZE * sizeof(uint8_t) * 2;

// Aliases for cleaner code
using Frequency = rf::Frequency;

enum class DroneType {
    UNKNOWN = 0,
    DJI_MAVIC,
    DJI_MINI,
    DJI_AIR,
    DJI_PHANTOM,
    DJI_AVATA,
    FPV_RACING,
    FPV_FREESTYLE,
    ORLAN_10,
    LANCET,
    SHAHED_136,
    BAYRAKTAR_TB2,
    MILITARY_UAV,
    RUSSIAN_EW_STATION,
    FIBER_OPTIC_DRONE,
    SVOY,
    CHUZHOY,
    HUYLO,
    HUYLINA,
    PVD,
    VSU,
    BRAT,
    KROT,
    PLENNYY,
    NE_PLENNYY,
    CODE_300,
    CODE_200,
    CODE_500,
    MAX_TYPES
};

enum class ThreatLevel {
    NONE = 0,
    LOW = 1,
    MEDIUM = 2,
    HIGH = 3,
    CRITICAL = 4
};

// PHASE 1 FIX 2: Add missing SpectrumMode enum definition
enum class SpectrumMode {
    ULTRA_NARROW = 0,    // 4MHz - for precision analysis
    NARROW = 1,          // 8MHz - for detailed scanning
    MEDIUM = 2,          // 12MHz - balanced performance (default)
    WIDE = 3,            // 20MHz - for wide area coverage
    ULTRA_WIDE = 4       // 24MHz - maximum bandwidth
};

enum class MovementTrend : uint8_t {
    STATIC = 0,
    APPROACHING = 1,
    RECEDING = 2,
    UNKNOWN = 3
};

    struct DroneFrequencyEntry {
        rf::Frequency frequency_hz;
        uint8_t drone_type_idx;
        uint8_t threat_level_idx;
        int8_t rssi_threshold_db;
        uint8_t bandwidth_idx;
        uint16_t name_offset;

    DroneFrequencyEntry(rf::Frequency freq, DroneType type, ThreatLevel threat,
                       int32_t rssi_thresh, uint32_t /*bw_hz*/, const char* /*desc*/)
            : frequency_hz(freq),
              drone_type_idx(static_cast<uint8_t>(type)),
              threat_level_idx(static_cast<uint8_t>(threat)),
              rssi_threshold_db(static_cast<int8_t>(rssi_thresh)),
              bandwidth_idx(0),
              name_offset(0) {}

        bool is_valid() const {
            return frequency_hz >= 50000000ULL && frequency_hz <= 6000000000ULL &&
                   rssi_threshold_db >= -120 && rssi_threshold_db <= -20;
        }
    };

struct TrackedDrone {
    rf::Frequency frequency;
    int16_t last_rssi;
    int16_t prev_rssi;
    systime_t last_seen;
    uint8_t drone_type;
    uint8_t trend_history;
    uint8_t update_count;

    TrackedDrone() : frequency(0ULL), last_rssi(-120), prev_rssi(-120),
                     last_seen(0), drone_type(0), trend_history(0), update_count(0) {}

    void add_rssi(int16_t rssi, systime_t time) {
        prev_rssi = last_rssi;
        last_rssi = rssi;
        last_seen = time;

        if (update_count < 255) update_count++;

        if (update_count >= 2) {
            calculate_simple_trend();
        }
    }

    MovementTrend get_trend() const {
        return static_cast<MovementTrend>(trend_history & 0x03);
    }

private:
    void calculate_simple_trend() {
        int16_t diff = last_rssi - prev_rssi;

        MovementTrend new_trend = MovementTrend::STATIC;

        if (diff > TREND_THRESHOLD_DB) {
            new_trend = MovementTrend::APPROACHING;
        } else if (diff < -TREND_THRESHOLD_DB) {
            new_trend = MovementTrend::RECEDING;
        }

        trend_history = (trend_history << 2) | static_cast<uint8_t>(new_trend);
    }
};

struct SimpleDroneValidation {
    static bool validate_rssi_signal(int32_t rssi_db, ThreatLevel threat) {
        if (rssi_db < -120 || rssi_db > -10) {
            return false;
        }

        int8_t detection_threshold;
        switch (threat) {
            case ThreatLevel::CRITICAL:
                detection_threshold = -75;
                break;
            case ThreatLevel::HIGH:
                detection_threshold = -80;
                break;
            case ThreatLevel::MEDIUM:
                detection_threshold = -85;
                break;
            case ThreatLevel::LOW:
                detection_threshold = -90;
                break;
            case ThreatLevel::NONE:
            default:
                detection_threshold = -95;
                break;
        }

        return rssi_db >= detection_threshold;
    }

    static ThreatLevel classify_signal_strength(int32_t rssi_db) {
        if (rssi_db >= -75) return ThreatLevel::CRITICAL;
        if (rssi_db >= -80) return ThreatLevel::HIGH;
        if (rssi_db >= -85) return ThreatLevel::MEDIUM;
        if (rssi_db >= -90) return ThreatLevel::LOW;
        return ThreatLevel::NONE;
    }

    static bool validate_frequency_range(rf::Frequency freq_hz) {
        return freq_hz >= 50000000 && freq_hz <= 6000000000UL;
    }
};

struct WidebandSlice {
    rf::Frequency center_frequency;
    uint8_t max_power;
    int16_t max_index;

    WidebandSlice() : center_frequency(0), max_power(0), max_index(-1) {}
    explicit WidebandSlice(rf::Frequency freq) : center_frequency(freq), max_power(0), max_index(-1) {}
};

struct WidebandScanData {
    WidebandSlice slices[WIDEBAND_MAX_SLICES];
    uint32_t slices_nb;
    uint32_t slice_counter;
    rf::Frequency min_freq;
    rf::Frequency max_freq;

    void reset() {
        slices_nb = 0;
        slice_counter = 0;
        min_freq = WIDEBAND_DEFAULT_MIN;
        max_freq = WIDEBAND_DEFAULT_MAX;
        for (auto& slice : slices) {
            slice = WidebandSlice();
        }
    }
};

struct DronePreset {
    std::string display_name;
    Frequency frequency_hz;
    DroneType drone_type;
    ThreatLevel threat_level;
    int32_t rssi_threshold_db;
    std::string name_template;
};

class DroneFrequencyPresets {
public:
    static const std::vector<DronePreset>& get_all_presets();

    static std::vector<DronePreset> get_presets_by_type(DroneType type);

    static const DronePreset* find_preset_by_name(const std::string& display_name);

    static std::vector<std::string> get_preset_names();

    static std::vector<DroneType> get_available_types();

    static std::string get_type_display_name(DroneType type);

private:
    static const std::vector<DronePreset> s_presets;
};

class DronePresetSelector {
public:
    static void show_preset_menu(NavigationView& nav,
                                std::function<void(const DronePreset&)> on_selected);
    static void show_type_filtered_presets(NavigationView& nav, DroneType type,
                                          std::function<void(const DronePreset&)> on_selected);
};

const std::vector<DronePreset> DroneFrequencyPresets::s_presets = {

    {"DJI Mini 4 Pro",       2400000000ULL, DroneType::DJI_MINI,   ThreatLevel::NONE,     -75, "DJI Mini 4 Pro"},
    {"DJI Mini 4 Pro (5.8GHz)", 5800000000, DroneType::DJI_MINI,   ThreatLevel::NONE,     -75, "DJI Mini 4 Pro (Video)"},
    {"DJI Mavic 3 Pro",      2400000000, DroneType::DJI_MAVIC,  ThreatLevel::NONE,     -70, "DJI Mavic 3 Pro"},
    {"DJI Mavic 3 Pro (5.8GHz)", 5800000000, DroneType::DJI_MAVIC,  ThreatLevel::NONE,     -70, "DJI Mavic 3 Pro (Video)"},

    {"Autel EVO Max 4T",     902000000,  DroneType::DJI_MAVIC,  ThreatLevel::NONE,     -72, "Autel EVO Max 4T"},
    {"Autel EVO Max 4T (2.4GHz)", 2400000000, DroneType::DJI_MAVIC,  ThreatLevel::NONE,     -72, "Autel EVO Max 4T (2.4GHz)"},
    {"Autel EVO Max 4T (5.8GHz)", 5800000000, DroneType::DJI_MAVIC,  ThreatLevel::NONE,     -72, "Autel EVO Max 4T (Video)"},

    {"Parrot ANAFI USA",     2400000000, DroneType::UNKNOWN,   ThreatLevel::NONE,     -78, "Parrot ANAFI USA"},
    {"Parrot ANAFI USA (Video)", 5800000000, DroneType::UNKNOWN,   ThreatLevel::NONE,     -78, "Parrot ANAFI USA (5.8GHz)"},

    {"Analog FPV (2.4GHz)",  2400000000, DroneType::FPV_RACING, ThreatLevel::NONE,     -80, "Analog FPV Racing"},
    {"Digital FPV (5.8GHz)", 5800000000, DroneType::FPV_RACING, ThreatLevel::NONE,     -75, "Digital FPV Racing"},
    {"Long Range FPV (433MHz)", 433920000,  DroneType::FPV_RACING, ThreatLevel::NONE,     -85, "Long Range FPV"},
    {"Long Range FPV (868MHz)", 868000000,  DroneType::FPV_RACING, ThreatLevel::NONE,     -82, "Long Range FPV (868MHz)"},

    {"Skydio X10 (5.8GHz)",  5800000000, DroneType::UNKNOWN,   ThreatLevel::NONE,     -75, "Skydio X10"},

    {"Orlan-10 (200MHz)",    200000000,  DroneType::ORLAN_10,  ThreatLevel::HIGH,    -85, "Orlan-10 Tactical"},
    {"Orlan-10 (433MHz)",    433920000,  DroneType::ORLAN_10,  ThreatLevel::HIGH,    -85, "Orlan-10 Tactical"},
    {"Orlan-10 (868MHz)",    868000000,  DroneType::ORLAN_10,  ThreatLevel::HIGH,    -85, "Orlan-10 Tactical"},
    {"Orlan-10 (915MHz)",    915000000,  DroneType::ORLAN_10,  ThreatLevel::HIGH,    -85, "Orlan-10 Tactical"},
    {"Orlan-10 (1.2GHz)",    1200000000, DroneType::ORLAN_10,  ThreatLevel::HIGH,    -80, "Orlan-10 Tactical"},
    {"Orlan-10 (2.4GHz)",    2400000000, DroneType::ORLAN_10,  ThreatLevel::HIGH,    -80, "Orlan-10 Tactical"},
    {"Orlan-10 (Video)",     900000000,  DroneType::ORLAN_10,  ThreatLevel::HIGH,    -82, "Orlan-10 Telemetry"},

    {"Lancet-3 (900MHz)",    900000000,  DroneType::LANCET,    ThreatLevel::CRITICAL,-82, "Lancet-3 Loitering"},
    {"Lancet-3 (5.8GHz)",    5800000000, DroneType::LANCET,    ThreatLevel::CRITICAL,-75, "Lancet-3 Loitering"},
    {"Shahed-131/136 (800MHz)", 800000000, DroneType::LANCET,    ThreatLevel::CRITICAL,-85, "Shahed Loitering"},
    {"Shahed-131/136 (2.6GHz)", 2600000000, DroneType::LANCET,    ThreatLevel::CRITICAL,-78, "Shahed Loitering"},

    {"Eleron-3SV (868MHz)",  868000000,  DroneType::UNKNOWN,   ThreatLevel::HIGH,    -85, "Eleron-3SV Tactical"},
    {"Eleron-3SV (915MHz)",  915000000,  DroneType::UNKNOWN,   ThreatLevel::HIGH,    -85, "Eleron-3SV Tactical"},
    {"Eleron-3SV (1.2GHz)",  1200000000, DroneType::UNKNOWN,   ThreatLevel::HIGH,    -82, "Eleron-3SV Tactical"},

    {"Forpost-R (465MHz)",   465000000,  DroneType::UNKNOWN,   ThreatLevel::HIGH,    -85, "Forpost-R Tactical"},
    {"Forpost-R (5.8GHz)",   5800000000, DroneType::UNKNOWN,   ThreatLevel::HIGH,    -75, "Forpost-R Tactical"},

    {"Bayraktar TB2 (868MHz)", 868000000,  DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-85, "Bayraktar TB2 MALE"},
    {"Bayraktar TB2 (915MHz)", 915000000,  DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-85, "Bayraktar TB2 MALE"},
    {"Bayraktar TB2 (2.4GHz)", 2400000000, DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-80, "Bayraktar TB2 MALE"},
    {"Bayraktar TB2 (5.8GHz)", 5800000000, DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-75, "Bayraktar TB2 MALE"},

    {"CH-4B Rainbow (900MHz)", 900000000,  DroneType::UNKNOWN,   ThreatLevel::HIGH,    -85, "CH-4B Rainbow MALE"},
    {"CH-4B Rainbow (4.4GHz)", 4400000000, DroneType::UNKNOWN,   ThreatLevel::HIGH,    -78, "CH-4B Rainbow MALE"},
    {"CH-5 Rainbow (841MHz)",  841000000,  DroneType::UNKNOWN,   ThreatLevel::HIGH,    -85, "CH-5 Rainbow MALE"},
    {"CH-5 Rainbow (1.4GHz)",  1400000000, DroneType::UNKNOWN,   ThreatLevel::HIGH,    -80, "CH-5 Rainbow MALE"},

    {"Hermes 450 (UHF)",     600000000,  DroneType::UNKNOWN,   ThreatLevel::HIGH,    -85, "Hermes 450 Tactical"},
    {"Hermes 450 (C-band)",  5000000000, DroneType::UNKNOWN,   ThreatLevel::HIGH,    -75, "Hermes 450 Tactical"},

    {"MQ-9 Reaper (UHF)",    300000000,  DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-90, "MQ-9 Reaper HALE"},
    {"MQ-9 Reaper (C-band)", 5500000000, DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-75, "MQ-9 Reaper HALE"},
    {"RQ-4 Global Hawk (UHF)", 300000000, DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-90, "RQ-4 Global Hawk HALE"},
    {"RQ-4 Global Hawk (Ku)", 13000000000,DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-70, "RQ-4 Global Hawk HALE"},

    {"Switchblade 600",      1300000000, DroneType::LANCET,    ThreatLevel::CRITICAL,-82, "Switchblade 600 Munition"},

    {"Marker UGV (2.4GHz)",  2400000000, DroneType::UNKNOWN,   ThreatLevel::MEDIUM,  -80, "Marker UGV"},
    {"Marker UGV (LTE 700)", 700000000,  DroneType::UNKNOWN,   ThreatLevel::MEDIUM,  -85, "Marker UGV"},
    {"Uran-6 UGV",           400000000,  DroneType::UNKNOWN,   ThreatLevel::MEDIUM,  -85, "Uran-6 UGV"},

    {"Military SATCOM (UHF)", 300000000,  DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-88, "Military SATCOM UHF"},
    {"Military SATCOM (C-band)", 5500000000, DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-75, "Military SATCOM C-band"},
    {"Military SATCOM (Ku)", 13000000000, DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-70, "Military SATCOM Ku-band"},
};

const std::vector<DronePreset>& DroneFrequencyPresets::get_all_presets() {
    return s_presets;
}

std::vector<DronePreset> DroneFrequencyPresets::get_presets_by_type(DroneType type) {
    std::vector<DronePreset> filtered;
    for (const auto& preset : s_presets) {
        if (preset.drone_type == type) {
            filtered.push_back(preset);
        }
    }
    return filtered;
}

const DronePreset* DroneFrequencyPresets::find_preset_by_name(const std::string& display_name) {
    for (const auto& preset : s_presets) {
        if (preset.display_name == display_name) {
            return &preset;
        }
    }
    return nullptr;
}

std::vector<std::string> DroneFrequencyPresets::get_preset_names() {
    std::vector<std::string> names;
    for (const auto& preset : s_presets) {
        names.push_back(preset.display_name);
    }
    return names;
}

void DronePresetSelector::show_preset_menu(NavigationView& nav,
                                          std::function<void(const DronePreset&)> on_selected) {
    auto preset_names = DroneFrequencyPresets::get_preset_names();

    class PresetMenuView : public MenuView {
    public:
        PresetMenuView(std::vector<std::string> names, std::function<void(const DronePreset&)> callback)
            : MenuView(),
              on_selected_fn(callback), preset_names(names) {
            for (const auto& name : names) {
                add_item(MenuItem{name, ui::Color::white(), nullptr, [](KeyEvent){}});
            }
        }

        virtual bool on_key(const KeyEvent key) override {
            if (key == KeyEvent::Up) {
                set_dirty();
                return true;
            }
            if (key == KeyEvent::Down) {
                set_dirty();
                return true;
            }
            if (key == KeyEvent::Select) {
                size_t idx = highlighted_index();
                if (idx < preset_names.size()) {
                    const DronePreset* preset = DroneFrequencyPresets::find_preset_by_name(preset_names[idx]);
                    if (preset && on_selected_fn) {
                        on_selected_fn(*preset);
                    }
                }
                return true;
            }
            return MenuView::on_key(key);
        }

    private:
        std::function<void(const DronePreset&)> on_selected_fn;
        std::vector<std::string> preset_names;
    };

    nav.push<PresetMenuView>(preset_names, on_selected);
}

std::vector<DroneType> DroneFrequencyPresets::get_available_types() {
    std::set<DroneType> types_set;
    for (const auto& preset : s_presets) {
        types_set.insert(preset.drone_type);
    }
    return std::vector<DroneType>(types_set.begin(), types_set.end());
}

std::string DroneFrequencyPresets::get_type_display_name(DroneType type) {
    switch (type) {
        case DroneType::DJI_MAVIC: return "DJI Mavic";
        case DroneType::DJI_MINI: return "DJI Mini";
        case DroneType::PARROT_ANAFI: return "Parrot";
        case DroneType::FPV_RACING: return "FPV Racing";
        case DroneType::ORLAN_10: return "Orlan-10";
        case DroneType::LANCET: return "Lancet";
        case DroneType::UNKNOWN: default: return "Other/Unknown";
    }
}

void DronePresetSelector::show_type_filtered_presets(NavigationView& nav, DroneType type,
                                          std::function<void(const DronePreset&)> on_selected) {
    auto filtered_presets = DroneFrequencyPresets::get_presets_by_type(type);

    std::vector<std::string> names;
    for (const auto& preset : filtered_presets) {
        names.push_back(preset.display_name);
    }

    class FilteredPresetMenuView : public MenuView {
    public:
        FilteredPresetMenuView(std::vector<std::string> names, std::vector<DronePreset> presets, std::function<void(const DronePreset&)> callback)
            : MenuView(),
              on_selected_fn(callback), preset_names(names), filtered_presets(presets) {
            for (const auto& name : names) {
                add_item(MenuItem{name, ui::Color::white(), nullptr, [](KeyEvent){}});
            }
        }

        virtual bool on_key(const KeyEvent key) override {
            if (key == KeyEvent::Select) {
                size_t idx = highlighted_index();
                if (idx < filtered_presets.size()) {
                    on_selected_fn(filtered_presets[idx]);
                }
                return true;
            }
            return MenuView::on_key(key);
        }

    private:
        std::function<void(const DronePreset&)> on_selected_fn;
        std::vector<std::string> preset_names;
        std::vector<DronePreset> filtered_presets;
    };

    nav.push<FilteredPresetMenuView>(names, filtered_presets, on_selected);
}

/**
 * TXT-Based Configuration System - Implements communication between Settings and Scanner apps
 */
class ScannerConfig {
public:
    struct ConfigData {
        std::string scanning_mode = "DATABASE";
        uint32_t min_frequency_hz = 50000000;
        uint32_t max_frequency_hz = 6000000000;
        int32_t rssi_threshold_db = -90;
        uint32_t scan_interval_ms = 750;
        bool enable_audio_alerts = true;
        std::string freqman_path = "DRONES";
    };

    ScannerConfig() = default;
    ~ScannerConfig() = default;

    const ConfigData& get_data() const { return config_data_; }

    // TXT file operations
    bool load_from_file(const std::string& filepath = "SCANNER_CONFIG.txt");
    bool save_to_file(const std::string& filepath = "SCANNER_CONFIG.txt") const;

    // Configuration setters for settings app
    void set_scanning_mode(const std::string& mode) { config_data_.scanning_mode = mode; }
    void set_frequency_range(uint32_t min_hz, uint32_t max_hz) {
        config_data_.min_frequency_hz = min_hz;
        config_data_.max_frequency_hz = max_hz;
    }
    void set_rssi_threshold(int32_t threshold) { config_data_.rssi_threshold_db = threshold; }
    void set_scan_interval(uint32_t interval_ms) { config_data_.scan_interval_ms = interval_ms; }
    void set_audio_alerts(bool enabled) { config_data_.enable_audio_alerts = enabled; }
    void set_freqman_path(const std::string& path) { config_data_.freqman_path = path; }

private:
    ConfigData config_data_;
};

bool ScannerConfig::load_from_file(const std::string& filepath) {
    // Implementation uses File and FileLineReader
    // Creates/gets sdcard/SCANNER_CONFIG.txt
    File f;
    auto path = "settings/" + filepath;  // Put in settings directory

    auto error = f.open(path);
    if (error) {
        return false; // File not found, use defaults
    }

    auto reader = FileLineReader(f);
    for (const auto& line : reader) {
        auto cols = split_string(line, '=');
        if (cols.size() == 2) {
            const std::string& key = trim(cols[0]);
            const std::string& value = trim(cols[1]);
            if (key == "scanning_mode") config_data_.scanning_mode = value;
            else if (key == "min_frequency_hz") config_data_.min_frequency_hz = std::stoul(value);
            else if (key == "max_frequency_hz") config_data_.max_frequency_hz = std::stoul(value);
            else if (key == "rssi_threshold_db") config_data_.rssi_threshold_db = std::stoi(value);
            else if (key == "scan_interval_ms") config_data_.scan_interval_ms = std::stoul(value);
            else if (key == "enable_audio_alerts") config_data_.enable_audio_alerts = (value == "true");
            else if (key == "freqman_path") config_data_.freqman_path = value;
        }
    }
    return true;
}

bool ScannerConfig::save_to_file(const std::string& filepath) const {
    // Write to sdcard/SCANNER_CONFIG.txt
    File f;
    auto path = "settings/" + filepath;

    ensure_directory(settings_dir);
    auto error = f.create(path);
    if (error) return false;

    // Write key=value pairs
    auto write_line = [&](const std::string& key, const std::string& value) {
        f.write(key.data(), key.length());
        f.write("=", 1);
        f.write(value.data(), value.length());
        f.write("\n", 1);
    };

    write_line("scanning_mode", config_data_.scanning_mode);
    write_line("min_frequency_hz", std::to_string(config_data_.min_frequency_hz));
    write_line("max_frequency_hz", std::to_string(config_data_.max_frequency_hz));
    write_line("rssi_threshold_db", std::to_string(config_data_.rssi_threshold_db));
    write_line("scan_interval_ms", std::to_string(config_data_.scan_interval_ms));
    write_line("enable_audio_alerts", config_data_.enable_audio_alerts ? "true" : "false");
    write_line("freqman_path", config_data_.freqman_path);

    return true;
}

} 

#endif
