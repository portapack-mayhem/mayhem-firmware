/*
 * Copyright (C) 2026
 *
 * This file is part of PortaPack.
 */

#pragma once

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_widget.hpp"
#include "replay_thread.hpp"
#include "signal.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace ui {

// Simple scrolling text area. Unlike ui::Console it does not use hardware
// scrolling and keeps its own line buffer, so it survives hide/show cycles
// (e.g. while the on-screen keyboard or the Audio RX app is open).
class TerminalOutput : public Widget {
   public:
    TerminalOutput(Rect parent_rect);

    void paint(Painter& painter) override;
    void add_line(const std::string& text);
    void clear_lines();
    // Positive: show older lines. Negative: show newer lines.
    void scroll_older(int lines);

   private:
    static constexpr size_t max_stored_lines = 64;

    size_t max_lines_;
    size_t chars_per_line_;
    size_t scroll_{0};  // how many lines the view is scrolled back from the newest
    std::vector<std::string> lines_{};
};

class TerminalView : public View {
   public:
    TerminalView(NavigationView& nav);
    ~TerminalView();

    void focus() override;
    bool on_key(const KeyEvent key) override;
    bool on_encoder(const EncoderEvent delta) override;
    std::string title() const override { return "Terminal"; };

   private:
    enum class RadioMode : uint8_t {
        Idle,
        Receive,
        Transmit,
    };

    void prompt_command();
    void run_input();
    void execute_line(const std::string& line);
    void history_up();
    void history_down();
    void show_input();
    bool is_directory_path(const std::string& path);
    void write_line(const std::string& text);
    void run_command(const std::string& line);
    void launch_app(const std::string& name);
    bool run_file_command(const std::string_view command, const std::vector<std::string_view>& arguments);
    void show_text_file(const std::string& path, size_t max_lines);
    void run_script(const std::string& path);
    void print_help();
    void list_apps(const std::string& filter);
    void print_prompt(const std::string& line);
    void print_status();
    void stop_radio(bool report = true);
    void start_receive(const std::string& frequency, const std::string& mode);
    void start_transmit(const std::string& path, const std::string& frequency,
                        const std::string& gain, const std::string& amp);
    void on_replay_done(uint32_t return_code);

    NavigationView& nav_;
    RadioMode radio_mode_{RadioMode::Idle};
    std::unique_ptr<ReplayThread> replay_thread_{};
    bool replay_ready_{};
    std::string current_path_{"/"};
    std::string command_buffer_{};
    std::string pending_line_{};
    bool has_pending_{false};
    std::vector<std::string> command_history_{};
    size_t history_index_{0};
    bool in_script_{false};

    Text text_radio_status{{UI_POS_X(0), UI_POS_Y(0), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, ""};
    Text text_input{{UI_POS_X(0), UI_POS_Y(1), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "/> "};
    TerminalOutput output{{0, UI_POS_Y(2), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(5)}};

    // Bottom row, 3 text lines tall (48 px). Widths add up to the 30 chars of the screen.
    Button button_command{{UI_POS_X(0), UI_POS_Y_BOTTOM(3), UI_POS_WIDTH(6), UI_POS_HEIGHT(3)}, "Cmd"};
    Button button_run{{UI_POS_X(6), UI_POS_Y_BOTTOM(3), UI_POS_WIDTH(5), UI_POS_HEIGHT(3)}, "Run"};
    Button button_stop{{UI_POS_X(11), UI_POS_Y_BOTTOM(3), UI_POS_WIDTH(6), UI_POS_HEIGHT(3)}, "Stop"};
    Button button_clear{{UI_POS_X(17), UI_POS_Y_BOTTOM(3), UI_POS_WIDTH(5), UI_POS_HEIGHT(3)}, "Clr"};
    Button button_up{{UI_POS_X(22), UI_POS_Y_BOTTOM(3), UI_POS_WIDTH(4), UI_POS_HEIGHT(3)}, "Up"};
    Button button_down{{UI_POS_X(26), UI_POS_Y_BOTTOM(3), UI_POS_WIDTH(4), UI_POS_HEIGHT(3)}, "Dn"};

    MessageHandlerRegistration message_handler_fifo_signal{
        Message::ID::RequestSignal,
        [this](const Message* p) {
            const auto message = static_cast<const RequestSignalMessage*>(p);
            if (message->signal == RequestSignalMessage::Signal::FillRequest)
                replay_ready_ = true;
        }};

    MessageHandlerRegistration message_handler_replay_done{
        Message::ID::ReplayThreadDone,
        [this](const Message* p) {
            const auto message = static_cast<const ReplayThreadDoneMessage*>(p);
            on_replay_done(message->return_code);
        }};
};

}  // namespace ui
