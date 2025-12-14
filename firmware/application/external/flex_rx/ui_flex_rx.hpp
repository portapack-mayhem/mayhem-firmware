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

    RxRadioState radio_state_{};

    // Message storage for console redraw
    static constexpr size_t MAX_MESSAGES = 20;
    std::vector<std::string> messages{};

    // Helper methods
    void log_message(const std::string& message);
    void redraw_console();
    void update_freq(rf::Frequency f);

    // UI Elements - Row 0
    RxFrequencyField field_frequency{
        {0 * 8, 0 * 16},
        nav_};

    RFAmpField field_rf_amp{
        {13 * 8, 0 * 16}};
    LNAGainField field_lna{
        {15 * 8, 0 * 16}};
    VGAGainField field_vga{
        {18 * 8, 0 * 16}};

    RSSI rssi{
        {21 * 8, 0, 10 * 8, 4}};

    // Message display area
    Console console{
        {0, 1 * 16, screen_width, screen_height - 1 * 16}};

    // Persistent settings manager
    app_settings::SettingsManager settings_{
        "rx_flex",
        app_settings::Mode::RX,
        {{"frequency", &frequency_value}}};

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