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
                  &button_color,
                  &rssi,
                  &menu_view});

    // Restore saved frequency
    field_frequency.set_value(frequency_value);
    receiver_model.set_target_frequency(frequency_value);

    // Frequency change callback
    field_frequency.updated = [this](rf::Frequency f) {
        update_freq(f);
    };

    // Color button cycles through available colors
    button_color.on_select = [this](Button&) {
        cycle_color();
    };

    // Configure receiver
    receiver_model.set_sampling_rate(3072000);
    receiver_model.set_baseband_bandwidth(1750000);
    receiver_model.enable();
    receiver_model.set_squelch_level(0);

    // Initialize FLEX baseband
    baseband::set_flex_config();

    log_message("FLEX RX Ready");
}

FlexAppView::~FlexAppView() {
    receiver_model.disable();
    baseband::shutdown();
}

void FlexAppView::focus() {
    field_frequency.focus();
}

// Cycle to next text color and refresh display
void FlexAppView::cycle_color() {
    current_color_index = (current_color_index + 1) % text_colors.size();
    rebuild_menu();
}

// Rebuild entire menu with current color
void FlexAppView::rebuild_menu() {
    menu_view.clear();
    Color current_color = text_colors[current_color_index];
    for (const auto& msg : log_messages) {
        menu_view.add_item({msg,
                            current_color,
                            nullptr,
                            [](KeyEvent) {}});
    }
    if (menu_view.item_count() > 0) {
        menu_view.set_highlighted(menu_view.item_count() - 1);
    }
}

// Add message to log with automatic line wrapping
void FlexAppView::log_message(const std::string& message) {
    // Calculate characters per line based on screen width (8 pixels per char)
    const size_t chars_per_line = screen_width / 8;
    Color current_color = text_colors[current_color_index];

    std::string remaining = message;
    bool first_line = true;
    bool needs_rebuild = false;
    size_t lines_added = 0;

    // Split message into screen-width chunks
    while (!remaining.empty()) {
        std::string line;
        if (remaining.length() <= chars_per_line) {
            line = remaining;
            remaining.clear();
        } else {
            line = remaining.substr(0, chars_per_line);
            remaining = remaining.substr(chars_per_line);
        }

        // Indent continuation lines
        if (!first_line) {
            line = " " + line;
        }
        first_line = false;

        // Remove oldest line if at limit
        if (log_messages.size() >= MAX_LOG_LINES) {
            log_messages.erase(log_messages.begin());
            needs_rebuild = true;
        }

        log_messages.push_back(line);
        lines_added++;
    }

    // Either rebuild all or just add new lines
    if (needs_rebuild) {
        rebuild_menu();
    } else {
        size_t start_idx = log_messages.size() - lines_added;
        for (size_t i = start_idx; i < log_messages.size(); i++) {
            menu_view.add_item({log_messages[i],
                                current_color,
                                nullptr,
                                [](KeyEvent) {}});
        }
        if (menu_view.item_count() > 0) {
            menu_view.set_highlighted(menu_view.item_count() - 1);
        }
    }
}

// Update frequency and save for persistence
void FlexAppView::update_freq(rf::Frequency f) {
    frequency_value = f;
    receiver_model.set_target_frequency(f);
}

// Handle decoded FLEX packet from baseband
void FlexAppView::on_packet(const FlexPacketMessage* message) {
    std::string text = "FLEX ";
    text += to_string_dec_uint(message->packet.bitrate);
    text += " ";
    text += to_string_dec_uint(message->packet.capcode);
    text += ": ";
    text += message->packet.message;

    log_message(text);
}

// Handle stats message (currently unused)
void FlexAppView::on_stats(const FlexStatsMessage* /* message */) {
}

// Handle debug message from baseband
void FlexAppView::on_debug(const FlexDebugMessage* message) {
    std::string text = "DBG: ";
    text += message->text;
    text += " " + to_string_hex(message->val1, 8);
    text += " " + to_string_hex(message->val2, 8);
    log_message(text);
}

}  // namespace ui::external_app::flex_rx