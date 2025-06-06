/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#ifndef __UI_GFXEQ_HPP__
#define __UI_GFXEQ_HPP__

#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "message.hpp"
#include "baseband_api.hpp"
#include "portapack.hpp"
#include "ui_spectrum.hpp"
#include "ui_freq_field.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"

namespace ui::external_app::gfxeq {

class gfxEQView : public View {
   public:
    gfxEQView(NavigationView& nav);
    ~gfxEQView();

    gfxEQView(const gfxEQView&) = delete;
    gfxEQView& operator=(const gfxEQView&) = delete;

    void focus() override;
    std::string title() const override { return "gfxEQ"; }

    void on_freqchg(int64_t freq);

   private:
    struct ColorTheme {
        Color base_color;
        Color peak_color;
    };

    NavigationView& nav_;

    uint32_t current_theme{0};
    const std::array<ColorTheme, 20> themes{
        ColorTheme{Color(255, 0, 255), Color(255, 255, 255)},
        ColorTheme{Color(0, 255, 0), Color(255, 0, 0)},
        ColorTheme{Color(0, 0, 255), Color(255, 255, 0)},
        ColorTheme{Color(255, 128, 0), Color(255, 0, 128)},
        ColorTheme{Color(128, 0, 255), Color(0, 255, 255)},
        ColorTheme{Color(255, 255, 0), Color(0, 255, 128)},
        ColorTheme{Color(255, 0, 0), Color(0, 128, 255)},
        ColorTheme{Color(0, 255, 128), Color(255, 128, 255)},
        ColorTheme{Color(128, 128, 128), Color(255, 255, 255)},
        ColorTheme{Color(255, 64, 0), Color(0, 255, 64)},
        ColorTheme{Color(0, 128, 128), Color(255, 192, 0)},
        ColorTheme{Color(0, 255, 0), Color(0, 128, 0)},
        ColorTheme{Color(32, 64, 32), Color(0, 255, 0)},
        ColorTheme{Color(64, 0, 128), Color(255, 0, 255)},
        ColorTheme{Color(0, 64, 0), Color(0, 255, 128)},
        ColorTheme{Color(255, 255, 255), Color(0, 0, 255)},
        ColorTheme{Color(128, 0, 0), Color(255, 128, 0)},
        ColorTheme{Color(0, 128, 255), Color(255, 255, 128)},
        ColorTheme{Color(64, 64, 64), Color(255, 0, 0)},
        ColorTheme{Color(255, 192, 0), Color(0, 64, 128)}};

    ButtonWithEncoder button_frequency{{0 * 8, 0 * 16 + 4, 11 * 8, 1 * 8}, ""};
    RFAmpField field_rf_amp{{13 * 8, 0 * 16}};
    LNAGainField field_lna{{15 * 8, 0 * 16}};
    VGAGainField field_vga{{18 * 8, 0 * 16}};
    Button button_mood{{21 * 8, 0, 6 * 8, 16}, "MOOD"};
    AudioVolumeField field_volume{{screen_width - 2 * 8, 0 * 16}};
    GraphEq gr{{2, UI_POS_DEFAULT_HEIGHT, UI_POS_MAXWIDTH - 4, UI_POS_HEIGHT_REMAINING(2)}, false};

    rf::Frequency frequency_value{93100000};

    RxRadioState rx_radio_state_{};

    app_settings::SettingsManager settings_{
        "rx_gfx_eq",
        app_settings::Mode::RX,
        {{"theme", &current_theme},
         {"frequency", &frequency_value}}};

    void cycle_theme();

    MessageHandlerRegistration message_handler_audio_spectrum{
        Message::ID::AudioSpectrum,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const AudioSpectrumMessage*>(p);
            this->gr.update_audio_spectrum(*message.data);
        }};

    MessageHandlerRegistration message_handler_freqchg{
        Message::ID::FreqChangeCommand,
        [this](Message* const p) {
            const auto message = static_cast<const FreqChangeCommandMessage*>(p);
            this->on_freqchg(message->freq);
        }};
};

}  // namespace ui::external_app::gfxeq

#endif