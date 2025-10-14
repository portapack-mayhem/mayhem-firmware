// ui_drone_analyzer_lifecycle.hpp
// Lifecycle management for Enhanced Drone Analyzer

#ifndef __UI_DRONE_ANALYZER_LIFECYCLE_HPP__
#define __UI_DRONE_ANALYZER_LIFECYCLE_HPP__

#include "ui.hpp"
#include "radio.hpp"
#include "portapack.hpp"
#include "app_settings.hpp"
#include "baseband_api.hpp"
#include "portapack_shared_memory.hpp"
#include <string>

namespace ui::external_app::enhanced_drone_analyzer {

class DroneAnalyzerLifecycle {
private:
    NavigationView& nav_;

public:
    DroneAnalyzerLifecycle(NavigationView& nav);
    ~DroneAnalyzerLifecycle();

    // Baseband and hardware initialization
    void initialize_hardware();

    // Database and scanner setup
    void initialize_components();
    void cleanup_components();

    // Settings and modal dialogs
    std::function<void()> on_open_settings;
    void show_settings_modal();
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_DRONE_ANALYZER_LIFECYCLE_HPP__
