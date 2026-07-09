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

    RFAmpField field_rf_amp{{UI_POS_X(13), UI_POS_Y(0)}};
    LNAGainField field_lna{{UI_POS_X(15), UI_POS_Y(0)}};
    VGAGainField field_vga{{UI_POS_X(18), UI_POS_Y(0)}};

    RxFrequencyField field_frequency{{UI_POS_X(0), UI_POS_Y(0)}, nav_};

    RSSI rssi{{UI_POS_X(21), UI_POS_Y(0), UI_POS_WIDTH_REMAINING(24), 4}};
    Channel channel{{UI_POS_X(21), 5, UI_POS_WIDTH_REMAINING(24), 4}};

    Text text_pi{{UI_POS_X(0), UI_POS_Y(2), UI_POS_WIDTH(12), UI_POS_HEIGHT(1)}, "PI: ----"};
    Text text_tp{{UI_POS_X(15), UI_POS_Y(2), UI_POS_WIDTH(10), UI_POS_HEIGHT(1)}, "TP: -"};

    Text text_pty{{UI_POS_X(0), UI_POS_Y(3), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "PTY: --"};

    Text text_ps_label{{UI_POS_X(0), UI_POS_Y(4), UI_POS_WIDTH(4), UI_POS_HEIGHT(1)}, "PS: "};
    Text text_ps_name{{UI_POS_X(4), UI_POS_Y(4), UI_POS_WIDTH(10), UI_POS_HEIGHT(1)}, "        "};

    Text text_rt_label{{UI_POS_X(0), UI_POS_Y(5), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "Radio Text:"};

    Text text_rt_1{{UI_POS_X(0), UI_POS_Y(6), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, ""};
    Text text_rt_2{{UI_POS_X(0), UI_POS_Y(7), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, ""};
    Text text_rt_3{{UI_POS_X(0), UI_POS_Y(8), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, ""};

    Console console{{UI_POS_X(0), UI_POS_Y(9), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(10)}};

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