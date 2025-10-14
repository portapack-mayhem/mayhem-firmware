// ui_drone_ui_controls.cpp
// Implementation of UI controls and event handlers

#include "ui_drone_ui_controls.hpp"
#include "ui_minimal_drone_analyzer.hpp"
#include "message.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

DroneUIControls::DroneUIControls(Button& start, Button& stop, Button& save, Button& load,
                                 Button& mode, Button& audio, Button& settings, Button& advanced,
                                 Button& warning)
    : button_start_(start), button_stop_(stop), button_save_freq_(save), button_load_file_(load),
      button_mode_(mode), button_audio_(audio), button_settings_(settings), button_advanced_(advanced),
      button_frequency_warning_(warning), view_(nullptr) {
}

void DroneUIControls::setup_button_handlers() {
    button_start_.on_select = [this](Button&) { on_start_scan_handler(button_start_); };
    button_stop_.on_select = [this](Button&) { on_stop_scan_handler(button_stop_); };
    button_save_freq_.on_select = [this](Button&) { on_save_frequency_handler(button_save_freq_); };
    button_load_file_.on_select = [this](Button&) { on_load_frequency_file_handler(button_load_file_); };
    button_mode_.on_select = [this](Button&) { on_toggle_mode_handler(button_mode_); };
    button_audio_.on_select = [this](Button&) { on_audio_toggle_handler(button_audio_); };
    button_settings_.on_select = [this](Button&) { on_open_settings_handler(button_settings_); };
    button_advanced_.on_select = [this](Button&) { on_advanced_settings_handler(button_advanced_); };
    button_frequency_warning_.on_select = [this](Button&) { on_frequency_warning_handler(button_frequency_warning_); };
}

void DroneUIControls::on_start_scan_handler(Button& button) {
    if (view_) view_->on_start_scan();
}

void DroneUIControls::on_stop_scan_handler(Button& button) {
    if (view_) view_->on_stop_scan();
}

void DroneUIControls::on_save_frequency_handler(Button& button) {
    if (view_) view_->on_save_frequency();
}

void DroneUIControls::on_load_frequency_file_handler(Button& button) {
    if (view_) view_->on_load_frequency_file();
}

void DroneUIControls::on_toggle_mode_handler(Button& button) {
    if (view_) view_->on_toggle_mode();
}

void DroneUIControls::on_audio_toggle_handler(Button& button) {
    if (view_) view_->on_audio_toggle();
}

void DroneUIControls::on_open_settings_handler(Button& button) {
    if (view_) view_->on_open_settings();
}

void DroneUIControls::on_advanced_settings_handler(Button& button) {
    if (view_) view_->on_advanced_settings();
}

void DroneUIControls::on_frequency_warning_handler(Button& button) {
    if (view_) view_->on_frequency_warning();
}

} // namespace ui::external_app::enhanced_drone_analyzer
