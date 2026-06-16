#ifndef __UI_SECPLUSTX__
#define __UI_SECPLUSTX__

#include <cstdint>
#include <string>

#include "ui.hpp"
#include "file.hpp"
#include "ui_transmitter.hpp"
#include "transmitter_model.hpp"
#include "file_path.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "encoders.hpp"
#include "string_format.hpp"

namespace ui::external_app::ui_secplustx {

enum class SecplusVersion : uint8_t {
    V1,
    V2,
};

struct SecplusData {
    SecplusVersion version;
    std::string name;
    bool has_data;
    uint64_t fixed_code;
    uint32_t rolling_code;
    uint32_t data;
};

class SecplusTXView : public View {
   public:
    void focus() override { button_open.focus(); }
    SecplusTXView(NavigationView& nav);
    ~SecplusTXView();

    std::string title() const override {
        return "Security+";
    }

   private:
    std::string file_path = (secplus_dir / "DEFAULT.SECPLUS").string();
    SecplusData data{SecplusVersion::V2, "Remote", false, 0, 0, 0};
    std::string buffer{};

    NavigationView& nav_;
    TxRadioState radio_state_{
        315000000,
        1750000,
        OOK_SAMPLERATE};

    app_settings::SettingsManager settings_{"tx_secplus", app_settings::Mode::TX, {{"file_path"sv, &file_path}}};

    Button button_open{{UI_POS_X(1), UI_POS_Y(1), screen_width / 2 - UI_POS_X(1), UI_POS_HEIGHT(2)}, "Open"};
    Button button_save{{screen_width / 2, UI_POS_Y(1), screen_width / 2 - UI_POS_X(1), UI_POS_HEIGHT(2)}, "Save"};

    // remote data
    TextField field_name{{UI_POS_X(1), UI_POS_Y(3), UI_POS_WIDTH_REMAINING(2), UI_POS_HEIGHT(1)}, "Remote"};
    Labels labels{
        {{UI_POS_X(1), UI_POS_Y(4)}, "Fixed:", Theme::getInstance()->fg_medium->foreground},
        {{UI_POS_X(1), UI_POS_Y(5)}, "Rolling:", Theme::getInstance()->fg_medium->foreground}};
    SymField field_fixed{{UI_POS_X(10), UI_POS_Y(4)}, 10, SymField::Type::Hex, true};
    SymField field_rolling{{UI_POS_X(10), UI_POS_Y(5)}, 7, SymField::Type::Hex, true};
    Checkbox has_data{{UI_POS_X(1), UI_POS_Y(6)}, 5, "Data:", true};
    SymField field_data{{UI_POS_X(10), UI_POS_Y(6)}, 8, SymField::Type::Hex, true};

    // options
    Checkbox learn_mode{{UI_POS_X(1), UI_POS_Y(7)}, 5, "Learn", true};
    Checkbox autosave{{UI_POS_X(1), UI_POS_Y(8)}, 8, "Autosave", true};

    ProgressBar progressbar{{UI_POS_X(2), UI_POS_Y_BOTTOM(5.75), UI_POS_WIDTH_REMAINING(4), UI_POS_HEIGHT(1)}};

    TransmitterView tx_view{UI_POS_Y_BOTTOM(4), 1000000, 0};

    Optional<SecplusData> read_secplus_file(const std::filesystem::path& file_path);
    bool write_secplus_file(const std::filesystem::path& file_path, const SecplusData& data);
    void save_data();
    void reload_data();
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
