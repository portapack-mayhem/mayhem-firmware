// test_compilation.cpp - Basic compilation test for modular apps
// This file tests that the unified headers can be included without major errors

// Test scanner app includes
#include "scanner/ui_scanner_combined.hpp"
using namespace ui::external_app::enhanced_drone_analyzer;

// Test settings app includes
#include "Settings/ui_settings_combined.hpp"

int main() {
    // Test basic types compilation
    ThreatLevel level = ThreatLevel::HIGH;
    DroneType type = DroneType::MAVIC;
    SpectrumMode mode = SpectrumMode::MEDIUM;

    // Test basic object creation
    DroneAnalyzerSettings settings;
    ScannerConfig config{{"DRONES", 0, -90, false, "MAVIC"}};

    // Test manager operations (without actual file I/O)
    DroneAnalyzerSettingsManager::reset_to_defaults(settings);
    bool valid = DroneAnalyzerSettingsManager::validate(settings);

    // Test simple algorithm usage
    DronePreset preset = DroneFrequencyPresets::get_all_presets()[0];
    bool preset_valid = preset.is_valid();

    // Test validation
    SimpleDroneValidation val;
    bool freq_ok = SimpleDroneValidation::validate_frequency_range(2400000000ULL);
    ThreatLevel threat = SimpleDroneValidation::classify_signal_strength(-70);

    // Basic output for verification
    printf("Settings initialized: %d\n", valid);
    printf("Preset valid: %d\n", preset_valid);
    printf("Frequency valid: %d\n", freq_ok);
    printf("Signal strength classified as: %u\n", static_cast<unsigned>(threat));

    return 0;  // Success if we compile and run
}
