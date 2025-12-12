#ifndef __UI_FLEX_RX_H__
#define __UI_FLEX_RX_H__

#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "ui_rssi.hpp"
#include "ui_spectrum.hpp"
#include "ui_record_view.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "freqman_db.hpp"

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
    RxRadioState radio_state_{
        929612500 /* frequency - common pager freq in some regions, or 931M */,
        max283x::filter::bandwidth_minimum /* bandwidth */,
        3072000 /* sampling rate */
    };

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
        {21 * 8, 0, 6 * 8, 4}};

    // Console - starts at row 1, extends to bottom of screen
    Console console{
        {0, 1 * 16, screen_width, screen_height - 1 * 16}};

    // Logic
    void on_packet(const FlexPacketMessage* message);
    void on_stats(const FlexStatsMessage* message);
    void on_debug(const FlexDebugMessage* message);

    // Message Handlers
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

    // Config
    void update_freq(rf::Frequency f);
};

}  // namespace ui::external_app::flex_rx

#endif /*__UI_FLEX_RX_H__*/