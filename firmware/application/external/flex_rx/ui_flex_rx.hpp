#ifndef __UI_FLEX_RX_H__
#define __UI_FLEX_RX_H__

#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "ui_rssi.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"

#include <string>
#include <vector>
#include <array>

namespace ui::external_app::flex_rx {

class FlexAppView : public View {
   public:
    FlexAppView(NavigationView& nav);
    ~FlexAppView();

    void focus() override;
    std::string title() const override { return "FLEX RX"; };

   private:
    NavigationView& nav_;

    // Saved settings
    rf::Frequency frequency_value{931740000};  // Default FLEX frequency
    uint32_t current_color_index{0};           // Current text color selection

    // Available text colors for message display
    static constexpr std::array<Color, 7> text_colors = {{Color::green(),
                                                          Color::white(),
                                                          Color::cyan(),
                                                          Color::magenta(),
                                                          Color::yellow(),
                                                          Color::blue(),
                                                          Color::red()}};

    RxRadioState radio_state_{};

    // Message log settings
    static constexpr size_t MAX_LOG_LINES = 32;  // Limit to prevent memory issues
    std::vector<std::string> log_messages{};     // Stored log lines

    // Helper methods
    void log_message(const std::string& message);  // Add message with word wrap
    void rebuild_menu();                           // Rebuild menu after color change or overflow
    void update_freq(rf::Frequency f);             // Update tuned frequency
    void cycle_color();                            // Cycle through text colors

    // UI Elements - Row 0, dynamically positioned
    RxFrequencyField field_frequency{
        {UI_POS_X(0), UI_POS_Y(0)},
        nav_};

    RFAmpField field_rf_amp{
        {UI_POS_X(13), UI_POS_Y(0)}};
    LNAGainField field_lna{
        {UI_POS_X(15), UI_POS_Y(0)}};
    VGAGainField field_vga{
        {UI_POS_X(18), UI_POS_Y(0)}};

    // Color cycle button
    Button button_color{
        {UI_POS_X(21), UI_POS_Y(0), UI_POS_WIDTH(5), UI_POS_HEIGHT(1)},
        "COLOR"};

    RSSI rssi{
        {UI_POS_X(26), 0, UI_POS_WIDTH(4), 4}};

    // Message display area - scrollable menu view
    MenuView menu_view{
        {0, 1 * 16, screen_width, screen_height - 1 * 16},
        true};

    // Persistent settings manager
    app_settings::SettingsManager settings_{
        "rx_flex",
        app_settings::Mode::RX,
        {{"frequency", &frequency_value},
         {"color_index", &current_color_index}}};

    // Message handlers
    void on_packet(const FlexPacketMessage* message);
    void on_stats(const FlexStatsMessage* message);
    void on_debug(const FlexDebugMessage* message);

    // Message handler registrations
    MessageHandlerRegistration message_handler_packet{
        Message::ID::FlexPacket,
        [this](const Message* const p) {
            const auto message = *static_cast<const FlexPacketMessage*>(p);
            this->on_packet(&message);
        }};

    MessageHandlerRegistration message_handler_stats{
        Message::ID::FlexStats,
        [this](const Message* const p) {
            const auto message = *static_cast<const FlexStatsMessage*>(p);
            this->on_stats(&message);
        }};

    MessageHandlerRegistration message_handler_debug{
        Message::ID::FlexDebug,
        [this](const Message* const p) {
            const auto message = *static_cast<const FlexDebugMessage*>(p);
            this->on_debug(&message);
        }};
};

}  // namespace ui::external_app::flex_rx

#endif /*__UI_FLEX_RX_H__*/