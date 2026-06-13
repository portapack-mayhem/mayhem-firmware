#ifndef __UI_SECPLUSTX__
#define __UI_SECPLUSTX__

#include <cstdint>

#include "ui.hpp"
#include "ui_transmitter.hpp"
#include "transmitter_model.hpp"
#include "file_path.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "encoders.hpp"
#include "string_format.hpp"

namespace ui::external_app::ui_secplustx {

class SecplusTXView : public View {
   public:
    void focus() override { field_fixed.focus(); }
    SecplusTXView(NavigationView& nav);
    ~SecplusTXView();

    std::string title() const override {
        return "Security+TX";
    }

   private:
    uint64_t rolling_code = 0, fixed_code = 0;
    NavigationView& nav_;

    TxRadioState radio_state_{
        315000000,
        1750000,
        OOK_SAMPLERATE};

    app_settings::SettingsManager settings_{
        "tx_secplus",
        app_settings::Mode::TX,
        {
            {"rolling_code"sv, &rolling_code},
            {"fixed_code"sv, &fixed_code},
        }};

    Checkbox learn_mode{{UI_POS_X(1), UI_POS_Y(0)}, 5, "Learn", true};
    Labels labels{
        {{UI_POS_X(1), UI_POS_Y(1)}, "Fixed:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(1), UI_POS_Y(2)}, "Rolling:", Theme::getInstance()->fg_light->foreground},
    };
    Checkbox enable_data{{UI_POS_X(1), UI_POS_Y(3)}, 5, "Data:", true};

    SymField field_fixed{{UI_POS_X(10), UI_POS_Y(1)}, 10, SymField::Type::Hex, true};
    SymField field_rolling{{UI_POS_X(10), UI_POS_Y(2)}, 7, SymField::Type::Hex, true};
    SymField field_data{{UI_POS_X(10), UI_POS_Y(3)}, 8, SymField::Type::Hex, true};

    ProgressBar progressbar{
        {UI_POS_X(2), UI_POS_Y_BOTTOM(7) + 20, UI_POS_WIDTH_REMAINING(4), 16}};

    TransmitterView tx_view{
        (int16_t)UI_POS_Y_BOTTOM(4),
        50000,
        0};

    void start_tx();
    void stop_tx();

    void on_tx_progress(uint32_t progress, bool done);

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
            this->on_tx_progress(message.progress, message.done);
        }};
};

}  // namespace ui::external_app::ui_secplustx

#endif
