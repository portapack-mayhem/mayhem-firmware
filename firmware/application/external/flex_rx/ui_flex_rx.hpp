#ifndef __UI_FLEX_RX_H__
#define __UI_FLEX_RX_H__

#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "ui_rssi.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "log_file.hpp"

#include <string>
#include <vector>

namespace ui::external_app::flex_rx {

enum FlexFilter : uint8_t {
    FLEX_FILTER_NONE,
    FLEX_FILTER_DROP,  // hide pages for the filter capcode
    FLEX_FILTER_KEEP   // show only pages for the filter capcode
};

// Options set in the FLEX Config screen, kept in the app settings (as POCSAG RX does).
struct FlexSettings {
    bool enable_log = false;     // decoded pages to LOGS/FLEX.TXT
    bool log_raw = false;        // also every packet with all its fields (BIW included)
    bool small_font = false;     // message list in the 5x8 font
    bool hide_tone_only = true;  // tone-only (address-only) pages carry no text
    uint8_t filter_mode = FLEX_FILTER_NONE;
    uint64_t filter_capcode = 0;
};

class FlexSettingsView : public View {
   public:
    FlexSettingsView(NavigationView& nav, FlexSettings& settings);

    std::string title() const override { return "FLEX Config"; };
    void focus() override { button_save.focus(); }

   private:
    FlexSettings& settings_;

    Labels labels{
        {{UI_POS_X(2), UI_POS_Y(10)}, "Filter Mode:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(2), UI_POS_Y(12)}, "Capcode:", Theme::getInstance()->fg_light->foreground},
    };

    Checkbox check_log{
        {UI_POS_X(2), UI_POS_Y(1)},
        10,
        "Enable Log"};

    Checkbox check_log_raw{
        {UI_POS_X(2), UI_POS_Y(3)},
        12,
        "Log Raw Data"};

    Checkbox check_small_font{
        {UI_POS_X(2), UI_POS_Y(5)},
        14,
        "Use Small Font"};

    Checkbox check_hide_tone{
        {UI_POS_X(2), UI_POS_Y(7)},
        14,
        "Hide Tone Only"};

    OptionsField opt_filter_mode{
        {UI_POS_X_RIGHT(12), UI_POS_Y(10)},
        4,
        {{"None", FLEX_FILTER_NONE},
         {"Drop", FLEX_FILTER_DROP},
         {"Keep", FLEX_FILTER_KEEP}}};

    SymField field_filter_capcode{
        {UI_POS_X_RIGHT(12), UI_POS_Y(12)},
        10,
        SymField::Type::Dec,
        true /*explicit_edit*/};

    Button button_save{
        {UI_POS_X_CENTER(10), UI_POS_Y_BOTTOM(4), UI_POS_WIDTH(10), UI_POS_HEIGHT(2)},
        "Save"};
};

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
    FlexSettings flex_settings_{};

    RxRadioState radio_state_{};

    // Persistent settings. Declared before the gain fields on purpose: RFAmpField,
    // LNAGainField and VGAGainField read the receiver model when they are constructed,
    // so the saved amp/LNA/VGA have to be loaded first or the fields show (and keep)
    // the defaults.
    app_settings::SettingsManager settings_{
        "rx_flex",
        app_settings::Mode::RX,
        {{"frequency", &frequency_value},
         {"small_font", &flex_settings_.small_font},
         {"log", &flex_settings_.enable_log},
         {"log_raw", &flex_settings_.log_raw},
         {"hide_tone_only", &flex_settings_.hide_tone_only},
         {"filter_mode", &flex_settings_.filter_mode},
         {"filter_capcode", &flex_settings_.filter_capcode}}};

    LogFile log_file_{};

    // Status bar state (updated from BIW packets)
    char status_time_[12]{};  // "HH:MM:SS"
    char status_tz_[16]{};    // "UTC+HH:MM es=N" (worst case 14 chars + null)
    uint16_t status_lid_{0};
    uint16_t status_cz_{0};
    uint16_t status_cc_{0};
    uint8_t status_extsec_{0};  // SysInfo extended-seconds (I7-I9, 0-7, 0.9375s each)

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
        {UI_POS_X(21), UI_POS_Y(0), UI_POS_WIDTH_REMAINING(21), 4}};

    // Status rows 1-3. Row 1: cycle/frame, rate, polarity and the Config button (as in
    // POCSAG RX); row 2: network time and zone, with what Config has switched on;
    // row 3: system ids (BIW).
    Text text_status1{
        {0, UI_POS_Y(1), UI_POS_WIDTH_REMAINING(9), UI_POS_HEIGHT(1)},
        ""};
    Button button_config{
        {UI_POS_X_RIGHT(8), UI_POS_Y(1), UI_POS_WIDTH(8), UI_POS_HEIGHT(1)},
        "Config"};
    Text text_time{
        {0, UI_POS_Y(2), UI_POS_WIDTH_REMAINING(8), UI_POS_HEIGHT(1)},
        ""};
    Text text_config{
        {UI_POS_X_RIGHT(8), UI_POS_Y(2), UI_POS_WIDTH(8), UI_POS_HEIGHT(1)},
        ""};
    Text text_status2{
        {0, UI_POS_Y(3), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)},
        ""};

    // Message display area, below the status rows down to the bottom of the view.
    Console console{
        {0, UI_POS_Y(4), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(5)}};

    void refresh_ui();
    bool passes_filter(const flex::FlexPacket& pkt) const;

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