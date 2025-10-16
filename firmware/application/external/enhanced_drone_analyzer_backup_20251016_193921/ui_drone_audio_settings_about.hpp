// ui_drone_audio_settings_about.hpp
// About/Author modal for Enhanced Drone Analyzer

#ifndef __UI_DRONE_AUDIO_SETTINGS_ABOUT_HPP__
#define __UI_DRONE_AUDIO_SETTINGS_ABOUT_HPP__

#include "ui.hpp"
#include "ui/navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

class AuthorContactView : public View {
public:
    AuthorContactView(NavigationView& nav);

    void focus() override;
    std::string title() const override { return "About Author"; }

private:
    NavigationView& nav_;

    // UI Elements
    Text text_title_{{5, 10}, "ENHANCED DRONE ANALYZER"};
    Text text_version_{{5, 25}, "Version 0.3 (PRODUCTION READY)"};

    Text text_feedback_title_{{5, 45}, "FEEDBACK & SUPPORT:"};
    Text text_telegram_{{5, 55}, "Telegram ID: 5364595248"};
    Text text_username_{{5, 65}, "@max_ai_master"};

    Text text_donation_title_{{5, 85}, "DONATION:"};
    Text text_yoomoney_{{5, 95}, "YooMoney: 41001810704697"};

    Text text_disclaimer_{{5, 115}, "2025 - Enhanced Drone Technology"};

    Button button_close_{{150, 135}, "CLOSE"};

    // Button handler
    void on_close();
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_DRONE_AUDIO_SETTINGS_ABOUT_HPP__
