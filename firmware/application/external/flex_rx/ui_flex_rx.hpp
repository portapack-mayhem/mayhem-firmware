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

    RSSI rssi{
        {UI_POS_X(21), 0, UI_POS_WIDTH(9), 4}};

    // Message display area (below controls, account for status bar)
    Console console{
        {0, 1 * 16, screen_width, screen_height - 2 * 16}};

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