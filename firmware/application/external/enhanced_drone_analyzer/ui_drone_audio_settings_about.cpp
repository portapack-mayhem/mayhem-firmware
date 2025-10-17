// ui_drone_audio_settings_about.cpp
// About/Author modal implementation for Enhanced Drone Analyzer

#include "ui_drone_audio_settings_about.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

AuthorContactView::AuthorContactView(NavigationView& nav)
    : nav_(nav) {

    // Add UI components
    add_children({
        &text_title_,
        &text_version_,
        &text_feedback_title_,
        &text_telegram_,
        &text_username_,
        &text_donation_title_,
        &text_yoomoney_,
        &text_disclaimer_,
        &button_close_
    });

    // Set up event handler
    button_close_.on_select = [this]() {
        on_close();
    };
}

void AuthorContactView::focus() {
    button_close_.focus();
}

void AuthorContactView::on_close() {
    nav_.pop();  // Return to previous view
}

} // namespace ui::external_app::enhanced_drone_analyzer
