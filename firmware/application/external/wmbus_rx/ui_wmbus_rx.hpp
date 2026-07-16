#ifndef __UI_WMBUS_RX_H__
#define __UI_WMBUS_RX_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "message.hpp"
#include <string>

namespace ui::external_app::wmbus_rx {

class WMBusRxView : public View {
   public:
    WMBusRxView(NavigationView& nav);
    ~WMBusRxView();
    void focus() override;

    std::string title() const override { return "W-MBus (rtl_433) Scanner"; };

   private:
    NavigationView& nav_;
    RxRadioState radio_state_{};
    app_settings::SettingsManager settings_{"rx_wmbus", app_settings::Mode::RX};

    OptionsField options_mode{
        {0, UI_POS_Y(0)},
        12,
        {{"T-Mode 100k", 0}, {"C-Mode 100k", 1}, {"S-Mode 32k ", 2}}};

    RFAmpField field_rf_amp{{13 * 8, UI_POS_Y(0)}};
    LNAGainField field_lna{{15 * 8, UI_POS_Y(0)}};
    VGAGainField field_vga{{18 * 8, UI_POS_Y(0)}};

    RxFrequencyField field_frequency{{UI_POS_X(0), UI_POS_Y(1)}, nav_};

    RSSI rssi{{UI_POS_X(21), 0, UI_POS_WIDTH_REMAINING(24), 4}};
    Channel channel{{UI_POS_X(21), 5, UI_POS_WIDTH_REMAINING(24), 4}};

    Text text_debug_st{{0, 3 * 16, 240, 16}, "ST:UNSYNCED S:0 E:0"};
    Text text_debug_fm{{0, 4 * 16, 240, 16}, "FM Peak: 0 | Mod: T"};

    Console console{{0, 5 * 16, screen_width, screen_height - (5 * 16)}};

    void on_data_wmbus(const WMBusPacketMessage& message);
    std::string decode_manufacturer(uint8_t byte1, uint8_t byte2);
    void on_mode_changed(int32_t mode);
    bool is_crc_byte(int idx);  // Segédfüggvény a CRC kivágásához

    MessageHandlerRegistration message_handler_wmbus{
        Message::ID::WMBusPacketMessageID,
        [this](Message* const p) {
            this->on_data_wmbus(*static_cast<const WMBusPacketMessage*>(p));
        }};
};

}  // namespace ui::external_app::wmbus_rx

#endif