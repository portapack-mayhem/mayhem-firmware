// ui_minimal_drone_analyzer.hpp
// Header that provides backward compatibility and includes the new modular implementation
// This file now just forwards to the new UI module

#include "ui_drone_ui.hpp"

// For backward compatibility, make the EnhancedDroneSpectrumAnalyzerView available
// from the old header name. This allows gradual migration.
using ui::external_app::enhanced_drone_analyzer::EnhancedDroneSpectrumAnalyzerView;
