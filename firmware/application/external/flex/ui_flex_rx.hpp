/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#ifndef __UI_FLEX_RX_H__
#define __UI_FLEX_RX_H__

#include "ui.hpp"
#include "ui_language.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "log_file.hpp"
#include "utility.hpp"

namespace ui::external_app::flex_rx {

class FlexLogger {
   public:
    Optional<File::Error> append(const std::filesystem::path& filename) {
        return log_file.append(filename);
    }
    void log_decoded(const Timestamp& timestamp, const std::string& data);

   private:
    LogFile log_file{};
};

class FlexRxView : public View {
   public:
    FlexRxView(NavigationView& nav);
    ~FlexRxView();
    void focus() override;
    std::string title() const override { return "Flex RX"; };

   private:
    void on_packet(const FlexPacketMessage& message);
    void on_stats(const FlexStatsMessage& stats);
    void on_freqchg(int64_t freq);

    NavigationView& nav_;
    RxRadioState radio_state_{};
    app_settings::SettingsManager settings_{"rx_flex", app_settings::Mode::RX};
    uint8_t console_color{0};
    bool logging{false};
    std::string current_message{};
    uint32_t last_address{0};
    
    uint16_t last_baud_rate{0};
    bool last_has_sync{false};
    uint8_t last_frames{0};
    uint32_t last_bits{0};

    RFAmpField field_rf_amp{{13 * 8, 0 * 16}};
    LNAGainField field_lna{{15 * 8, 0 * 16}};
    VGAGainField field_vga{{18 * 8, 0 * 16}};
    RSSI rssi{{21 * 8, 0, 6 * 8, 4}};
    Channel channel{{21 * 8, 5, 6 * 8, 4}};
    AudioVolumeField field_volume{{28 * 8, 0 * 16}};
    RxFrequencyField field_frequency{{0 * 8, 0 * 16}, nav_};
    Checkbox check_log{{0 * 8, 1 * 16}, 3, LanguageHelper::currentMessages[LANG_LOG], false};
    Text text_debug{{0 * 8, 12 + 2 * 16, screen_width, 16}, ""};
    Console console{{0, 4 * 16, 240, 176}};
    Rectangle sync_status{{214, 1 * 16, 12, 12}, Color::red()};

    std::unique_ptr<FlexLogger> logger{};

    MessageHandlerRegistration message_handler_packet{
        Message::ID::FlexPacket,
        [this](Message* const p) {
            const auto message = static_cast<const FlexPacketMessage*>(p);
            this->on_packet(*message);
        }};
    MessageHandlerRegistration message_handler_stats{
        Message::ID::FlexStats,
        [this](Message* const p) {
            const auto message = static_cast<const FlexStatsMessage*>(p);
            this->on_stats(*message);
        }};
    MessageHandlerRegistration message_handler_freqchg{
        Message::ID::FreqChangeCommand,
        [this](Message* const p) {
            const auto message = static_cast<const FreqChangeCommandMessage*>(p);
            this->on_freqchg(message->freq);
        }};
};

}  // namespace ui::external_app::flex_rx

#endif /*__UI_FLEX_RX_H__*/