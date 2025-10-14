// ui_drone_analyzer_lifecycle.cpp
// Implementation of lifecycle management for Enhanced Drone Analyzer

#include "ui_drone_analyzer_lifecycle.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

DroneAnalyzerLifecycle::DroneAnalyzerLifecycle(NavigationView& nav)
    : nav_(nav) {
    // Initialize settings callback
    on_open_settings = [this]() {
        show_settings_modal();
    };
}

DroneAnalyzerLifecycle::~DroneAnalyzerLifecycle() {
    cleanup_components();
}

void DroneAnalyzerLifecycle::initialize_hardware() {
    // Baseband setup for spectrum analysis
    baseband::run_image(portapack::spi_flash::image_tag_wideband_spectrum);

    // Setup radio parameters
    receiver_model.set_modulation(ReceiverModel::Mode::SpectrumAnalysis);
    receiver_model.set_sampling_rate(12000000);      // 12MHz rate (spectrum mode)
    receiver_model.set_baseband_bandwidth(6000000);  // 6MHz bandwidth
    receiver_model.set_squelch_level(0);             // No squelch for spectrum

    // Setup radio direction and spectrum streaming
    radio::set_direction(rf::Direction::Receive);
    baseband::set_spectrum(6000000, 0);  // 6MHz bandwidth, trigger=0 for continuous

    // Enable receiver
    receiver_model.enable();
}

void DroneAnalyzerLifecycle::initialize_components() {
    // Components will be initialized here when we add them
    // For now this is a placeholder for the lifecycle management
}

void DroneAnalyzerLifecycle::cleanup_components() {
    // Cleanup spectrum streaming
    SpectrumStreamingConfigMessage stop_config{SpectrumStreamingConfigMessage::Mode::Stopped};
    shared_memory.application_queue.push(stop_config);

    // Disable hardware
    receiver_model.disable();
    baseband::shutdown();
}

void DroneAnalyzerLifecycle::show_settings_modal() {
    nav_.display_modal("Settings",
                      "Enhanced Drone Analyzer Settings\n"
                      "================================\n\n"
                      "This is a modular version.\n"
                      "Settings will be expanded as\n"
                      "we complete the refactoring.\n\n"
                      "Current features:\n"
                      "- Split into multiple files\n"
                      "- Hardware lifecycle management\n"
                      "- Component initialization\n\n"
                      "Roadmap: Database integration,\n"
                      "spectrum analysis, and\n"
                      "threat tracking coming soon.");
}

} // namespace ui::external_app::enhanced_drone_analyzer
