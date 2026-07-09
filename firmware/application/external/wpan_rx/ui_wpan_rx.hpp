#ifndef __UI_WPAN_RX_H__
#define __UI_WPAN_RX_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "message.hpp"

#include <string>

namespace ui::external_app::wpan_rx {

class WpanRxView : public View {
   public:
    WpanRxView(NavigationView& nav);
    ~WpanRxView();
    void focus() override;

    std::string title() const override { return "802.15.4 RX"; };

   private:
    NavigationView& nav_;
    RxRadioState radio_state_{};
    app_settings::SettingsManager settings_{"rx_wpan", app_settings::Mode::RX};

    // Standard rádió vevő vezérlők
    RFAmpField field_rf_amp{{13 * 8, UI_POS_Y(0)}};
    LNAGainField field_lna{{15 * 8, UI_POS_Y(0)}};
    VGAGainField field_vga{{18 * 8, UI_POS_Y(0)}};

    RxFrequencyField field_frequency{{UI_POS_X(0), UI_POS_Y(0)}, nav_};

    RSSI rssi{{UI_POS_X(21), 0, UI_POS_WIDTH_REMAINING(24), 4}};
    Channel channel{{UI_POS_X(21), 5, UI_POS_WIDTH_REMAINING(24), 4}};

    // Fekete terminál a csomagok megjelenítéséhez
    Console console{{0, 2 * 16, screen_width, screen_height - (2 * 16)}};

    void on_data_wpan(const WPANPacketMessage& message);

    MessageHandlerRegistration message_handler_wpan{
        Message::ID::WPANMessageID,
        [this](Message* const p) {
            this->on_data_wpan(*static_cast<const WPANPacketMessage*>(p));
        }};
};

}  // namespace ui::external_app::wpan_rx

#endif  // __UI_WPAN_RX_H__