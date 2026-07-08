#ifndef __UI_RDS_RX_H__
#define __UI_RDS_RX_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "message.hpp"
#include <string>

using namespace ui;

namespace ui::external_app::rds_rx {

class RdsRxView : public View {
   public:
    RdsRxView(NavigationView& nav);
    ~RdsRxView();
    void focus() override;

    std::string title() const override { return "RDS RX"; };

   private:
    NavigationView& nav_;
    RxRadioState radio_state_{};
    app_settings::SettingsManager settings_{"rx_rds", app_settings::Mode::RX};

    RFAmpField field_rf_amp{{13 * 8, UI_POS_Y(0)}};
    LNAGainField field_lna{{15 * 8, UI_POS_Y(0)}};
    VGAGainField field_vga{{18 * 8, UI_POS_Y(0)}};

    RxFrequencyField field_frequency{{UI_POS_X(0), UI_POS_Y(0)}, nav_};

    RSSI rssi{{UI_POS_X(21), 0, UI_POS_WIDTH_REMAINING(24), 4}};
    Channel channel{{UI_POS_X(21), 5, UI_POS_WIDTH_REMAINING(24), 4}};

    Text text_pi{{0, 2 * 16, 8 * 12, 16}, "PI: ----"};
    Text text_tp{{120, 2 * 16, 8 * 10, 16}, "TP: -"};

    Text text_pty{{0, 3 * 16, 240, 16}, "PTY: --"};

    Text text_ps_label{{0, 4 * 16, 8 * 4, 16}, "PS: "};
    Text text_ps_name{{8 * 4, 4 * 16, 8 * 10, 16}, "        "};

    Text text_rt_label{{0, 5 * 16, 240, 16}, "Radio Text:"};
    // A 64 karakteres RT tördelve (Soronként maximum 30 karakter fér el)
    Text text_rt_1{{0, 6 * 16, 240, 16}, ""};
    Text text_rt_2{{0, 7 * 16, 240, 16}, ""};
    Text text_rt_3{{0, 8 * 16, 240, 16}, ""};

    Console console{{0, 9 * 16, screen_width, screen_height - (10 * 16)}};

    char ps_name[9] = "        ";
    char radio_text[65] = {0};

    void on_data_rds(const RDSGroupMessage& message);

    MessageHandlerRegistration message_handler_rds{
        Message::ID::RdsData,
        [this](Message* const p) {
            this->on_data_rds(*static_cast<const RDSGroupMessage*>(p));
        }};
};

}  // namespace ui::external_app::rds_rx

#endif