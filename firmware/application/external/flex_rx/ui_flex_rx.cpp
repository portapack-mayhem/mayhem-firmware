#include "ui_flex_rx.hpp"

#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"
#include "string_format.hpp"
#include "memory_map.hpp"

using namespace portapack;

namespace ui::external_app::flex_rx {

FlexAppView::FlexAppView(NavigationView& nav)
    : nav_{nav} {
    // Load baseband image for FLEX decoding
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&field_frequency,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &rssi,
                  &console});

    // Restore saved frequency
    field_frequency.set_value(frequency_value);
    receiver_model.set_target_frequency(frequency_value);

    // Frequency change callback
    field_frequency.updated = [this](rf::Frequency f) {
        update_freq(f);
    };

    // Configure receiver
    receiver_model.set_sampling_rate(3072000);
    receiver_model.set_baseband_bandwidth(1750000);
    receiver_model.enable();
    receiver_model.set_squelch_level(0);

    // Initialize FLEX baseband
    baseband::set_flex_config();

    console.writeln("Ready");
}

FlexAppView::~FlexAppView() {
    receiver_model.disable();
    baseband::shutdown();
}

void FlexAppView::focus() {
    field_frequency.focus();
}

// Redraw all messages to console
void FlexAppView::redraw_console() {
    console.clear(true);
    bool first = true;
    for (const auto& msg : messages) {
        if (!first) {
            console.writeln("");  // Blank line between messages
        }
        first = false;
        console.writeln(msg);
    }
}

// Add message to log with automatic line wrapping
void FlexAppView::log_message(const std::string& message) {
    const size_t chars_per_line = screen_width / 8;
    // Console height accounts for status bar and controls row
    const size_t console_lines = (screen_height - 2 * 16) / 16;

    messages.push_back(message);

    // Calculate total lines used (messages + blank lines between them)
    size_t total_lines = 0;
    for (size_t i = 0; i < messages.size(); i++) {
        if (i > 0) total_lines++;  // Count blank line separator
        size_t msg_lines = (messages[i].length() + chars_per_line - 1) / chars_per_line;
        if (msg_lines == 0) msg_lines = 1;
        total_lines += msg_lines;
    }

    // If console would overflow, remove oldest messages and redraw
    if (total_lines > console_lines) {
        while (total_lines > console_lines && !messages.empty()) {
            const auto& oldest = messages.front();
            size_t oldest_lines = (oldest.length() + chars_per_line - 1) / chars_per_line;
            if (oldest_lines == 0) oldest_lines = 1;
            total_lines -= oldest_lines;
            if (messages.size() > 1) total_lines--;  // Remove separator line too
            messages.erase(messages.begin());
        }
        redraw_console();
    } else {
        // Just append new message
        if (messages.size() > 1) {
            console.writeln("");  // Blank line before new message
        }
        console.writeln(message);
    }
}

// Update frequency and save for persistence
void FlexAppView::update_freq(rf::Frequency f) {
    frequency_value = f;
    receiver_model.set_target_frequency(f);
}

// Handle decoded FLEX packet from baseband
void FlexAppView::on_packet(const FlexPacketMessage* message) {
    log_message(message->packet.message);
}

// Handle stats message (currently unused)
void FlexAppView::on_stats(const FlexStatsMessage*) {
}

// Debug handler - uncomment to see baseband debug messages
void FlexAppView::on_debug(const FlexDebugMessage* message) {
    (void)message;  // Suppress unused parameter warning
    // std::string text = "DBG: ";
    // text += message->text;
    // text += " " + to_string_hex(message->val1, 8);
    // text += " " + to_string_hex(message->val2, 8);
    // log_message(text);
}

}  // namespace ui::external_app::flex_rx