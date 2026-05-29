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
    rf::Frequency frequency_value{931740000};

    RxRadioState radio_state_{};

    // Status bar state (updated from BIW packets)
    char status_time_[12]{};  // "HH:MM:SS"
    char status_tz_[12]{};    // "UTC+N"
    uint16_t status_lid_{0};
    uint16_t status_cz_{0};
    uint16_t status_cc_{0};

    // Helper methods
    void log_message(const std::string& message);
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

    // Status rows (rows 1-2)
    Text text_status1{
        {0, 1 * 16, screen_width, 16},
        ""};
    Text text_status2{
        {0, 2 * 16, screen_width, 16},
        ""};

    // Message display area (below status rows)
    Console console{
        {0, 3 * 16, screen_width, screen_height - 4 * 16}};

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