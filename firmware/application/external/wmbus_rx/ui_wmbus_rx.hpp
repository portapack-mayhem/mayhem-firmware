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

    std::string title() const override { return "W-MBus"; };

   private:
    NavigationView& nav_;
    RxRadioState radio_state_{};
    app_settings::SettingsManager settings_{"rx_wmbus", app_settings::Mode::RX};

    OptionsField options_mode{
        {UI_POS_X(0), UI_POS_Y(0)},
        6,
        {{"T-Mode", 0}, {"C-Mode", 1}, {"S-Mode", 2}}};

    RFAmpField field_rf_amp{{UI_POS_X(13), UI_POS_Y(0)}};
    LNAGainField field_lna{{UI_POS_X(15), UI_POS_Y(0)}};
    VGAGainField field_vga{{UI_POS_X(18), UI_POS_Y(0)}};

    RxFrequencyField field_frequency{{UI_POS_X(0), UI_POS_Y(1)}, nav_};

    RSSI rssi{{UI_POS_X(21), 0, UI_POS_WIDTH_REMAINING(22), 4}};
    Channel channel{{UI_POS_X(21), 5, UI_POS_WIDTH_REMAINING(22), 4}};

    Text text_debug_err{{UI_POS_X(0), UI_POS_Y(2), UI_POS_MAXWIDTH, 16}, "Last Err: None"};

    Console console{{UI_POS_X(0), UI_POS_Y(3), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(4)}};

    void on_data_wmbus(const WMBusPacketMessage& message);
    std::string decode_manufacturer(uint8_t byte1, uint8_t byte2);
    std::string decode_device_type(uint8_t type_byte);
    void on_mode_changed(int32_t mode);

    MessageHandlerRegistration message_handler_wmbus{
        Message::ID::WMBusPacketMessageID,
        [this](Message* const p) {
            this->on_data_wmbus(*static_cast<const WMBusPacketMessage*>(p));
        }};
};

}  // namespace ui::external_app::wmbus_rx

#endif  // __UI_WMBUS_RX_H__