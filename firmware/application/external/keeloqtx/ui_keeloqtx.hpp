#ifndef __KEELOQTX__
#define __KEELOQTX__

#include "ui.hpp"
#include "ui_transmitter.hpp"
#include "transmitter_model.hpp"
#include "file_path.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "encoders.hpp"
#include "string_format.hpp"
#include "../../keeloq_file.hpp"
#include "../../keeloq_keystore.hpp"

#define KEELOQ_HEADER "101010101010101010101010000000000"

namespace ui::external_app::ui_keeloqtx {

class KeeloqTXView : public View {
   public:
    KeeloqTXView(NavigationView& nav);
    ~KeeloqTXView();

    void focus() override;

    std::string title() const override {
        return "KeeLoqTX";
    }

   private:
    const std::string keeloq_fragments[2] = {"110", "100"};

    NavigationView& nav_;

    TxRadioState radio_state_{
        433920000,
        1750000,
        OOK_SAMPLERATE};

    app_settings::SettingsManager settings_{
        "tx_keeloq", app_settings::Mode::TX};

    std::filesystem::path file_path{};

    KeeloqKeystore keystore{};
    KeeloqKey current_key{};
    KeeloqData data{};

    uint32_t fix = 0;
    uint32_t hop = 0;
    uint64_t payload = 0;

    void update_hop();

    Labels labels{
        {{UI_POS_X(1), UI_POS_Y(0)}, "Manufacturer:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(1), UI_POS_Y(1)}, "Serial:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(1), UI_POS_Y(2)}, "Fix:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(1), UI_POS_Y(3)}, "Hop:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(1), UI_POS_Y(4)}, "Counter:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(1), UI_POS_Y(5)}, "Button:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(1), UI_POS_Y(6)}, "Repeat:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(2), UI_POS_Y(8)}, "Payload:", Theme::getInstance()->fg_light->foreground},
    };

    Text text_mf_name{{UI_POS_X(15), UI_POS_Y(0), 128, 16}, ""};
    Text text_serial{{UI_POS_X(9), UI_POS_Y(1), 64, 16}, "00000000"};
    Text text_fix{{UI_POS_X(6), UI_POS_Y(2), 64, 16}, "00000000"};
    Text text_hop{{UI_POS_X(6), UI_POS_Y(3), 64, 16}, "00000000"};

    NumberField field_counter{
        {UI_POS_X(10), UI_POS_Y(4)},
        5,
        {0, 65535},
        1,
        ' '};
    NumberField field_button{
        {UI_POS_X(9), UI_POS_Y(5)},
        2,
        {0, 15},
        1,
        ' '};
    NumberField field_repeat{
        {UI_POS_X(10), UI_POS_Y(6)},
        3,
        {0, 100},
        1,
        ' '};

    Text text_payload{{UI_POS_X(2), UI_POS_Y(9), 128, 16}, "0000000000000000"};

    void update_payload();

    Button button_open{{UI_POS_X(0), UI_POS_Y(11), screen_width, 32}, "Open file"};

    Text text_status{{UI_POS_X(2), UI_POS_Y_BOTTOM(7), 128, 16}, "Ready"};

    ProgressBar progressbar{
        {UI_POS_X(2), UI_POS_Y_BOTTOM(7) + 20, UI_POS_WIDTH_REMAINING(4), 16}};

    TransmitterView tx_view{
        (int16_t)UI_POS_Y_BOTTOM(4),
        50000,
        9};

    std::string encoded_data{};

    void encode_data();

    uint32_t repeat = 4;
    uint32_t pause_duration = 0;

    void start_tx();
    void stop_tx();

    void update_progress();
    void on_tx_progress(uint32_t progress, bool done);

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
            this->on_tx_progress(message.progress, message.done);
        }};
};

}  // namespace ui::external_app::ui_keeloqtx

#endif
