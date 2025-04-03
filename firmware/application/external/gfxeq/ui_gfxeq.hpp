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
    void on_show() override;
    void on_hide() override;

    void paint(Painter& painter) override;

   private:
    static constexpr ui::Dim header_height = 2 * 16;
    static constexpr int SCREEN_WIDTH = 240;
    static constexpr int SCREEN_HEIGHT = 320;
    static constexpr int RENDER_HEIGHT = 288;
    static constexpr int NUM_BARS = 16;
    static constexpr int BAR_WIDTH = SCREEN_WIDTH / NUM_BARS;
    static constexpr int BAR_SPACING = 2;
    static constexpr int SEGMENT_HEIGHT = 10;

    struct ColorTheme {
        Color base_color;
        Color peak_color;
    };

    NavigationView& nav_;
    bool initialized{false};
    bool needs_background_redraw{false};
    std::vector<int> bar_heights;
    std::vector<int> prev_bar_heights;
    bool running{false};
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

    RxFrequencyField field_frequency{{0 * 8, 0 * 16}, nav_};
    RFAmpField field_rf_amp{{13 * 8, 0 * 16}};
    LNAGainField field_lna{{15 * 8, 0 * 16}};
    VGAGainField field_vga{{18 * 8, 0 * 16}};
    Button button_mood{{21 * 8, 0, 6 * 8, 16}, "MOOD"};
    AudioVolumeField field_volume{{28 * 8, 0 * 16}};
    Text text_ctcss{{16 * 8, 1 * 16, 14 * 8, 1 * 16}, ""};
    const Rect options_view_rect{0 * 8, 1 * 16, 30 * 8, 1 * 16};
    std::unique_ptr<Widget> options_widget{};

    RxRadioState rx_radio_state_{};

    bool rf_amp_value{false};
    int32_t lna_gain_value{0};
    int32_t vga_gain_value{0};
    rf::Frequency frequency_value{93100000};
    int32_t volume_value{50};

    app_settings::SettingsManager settings_{
        "rx_gfx_eq",
        app_settings::Mode::RX,
        {{"theme", &current_theme},
         {"rf_amp", &rf_amp_value},
         {"lna_gain", &lna_gain_value},
         {"vga_gain", &vga_gain_value},
         {"frequency", &frequency_value},
         {"volume", &volume_value}}};

    void start();
    void stop();
    void update_audio_spectrum(const AudioSpectrum& spectrum);
    void render_equalizer(Painter& painter);
    void on_frequency_step_changed(rf::Frequency f);
    void on_reference_ppm_correction_changed(int32_t v);
    void remove_options_widget();
    void set_options_widget(std::unique_ptr<Widget> new_widget);
    void update_modulation(ReceiverModel::Mode modulation);
    void handle_coded_squelch(uint32_t value);
    void cycle_theme();

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {}};
    MessageHandlerRegistration message_handler_audio_spectrum{
        Message::ID::AudioSpectrum,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const AudioSpectrumMessage*>(p);
            this->update_audio_spectrum(*message.data);
            this->set_dirty();
        }};
    MessageHandlerRegistration message_handler_coded_squelch{
        Message::ID::CodedSquelch,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const CodedSquelchMessage*>(p);
            this->handle_coded_squelch(message.value);
        }};
};

}  // namespace ui::external_app::gfxeq

#endif