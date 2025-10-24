// ui_minimal_drone_analyzer.hpp
// Loading screen and minimal analyzer view for Enhanced Drone Analyzer

#ifndef __UI_MINIMAL_DRONE_ANALYZER_HPP__
#define __UI_MINIMAL_DRONE_ANALYZER_HPP__

#include "ui.hpp"
#include "ui.hpp"
#include "ui_drone_ui.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

// Loading screen UI for Enhanced Drone Analyzer
class LoadingScreenView : public View {
public:
    explicit LoadingScreenView(NavigationView& nav);
    ~LoadingScreenView();

    void paint(Painter& painter) override;

    bool on_key([[maybe_unused]] const KeyEvent key) override {
        return false;  // Don't handle keys during loading
    }

    bool on_touch([[maybe_unused]] const TouchEvent event) override {
        return false;  // Don't handle touch during loading
    }

private:
    NavigationView& nav_;
    Text text_eda_;
    uint32_t timer_start_;

    MessageHandlerRegistration message_handler_frame_sync_{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            // Check if 1 second has passed (1000ms)
            if (chTimeNow() - timer_start_ > 1000) {
                // Auto-dismiss loading screen
                nav_.pop();
                // Show main EnhancedDroneSpectrumAnalyzerView
                nav_.push<EnhancedDroneSpectrumAnalyzerView>();
            }
        }};
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_MINIMAL_DRONE_ANALYZER_HPP__
