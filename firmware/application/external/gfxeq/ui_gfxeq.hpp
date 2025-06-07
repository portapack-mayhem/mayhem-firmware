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

<<<<<<< HEAD
#include "external/ui_grapheq.hpp"

=======
>>>>>>> 288f6bd5 (GFX EQ App (#2607))
class gfxEQView : public View {
   public:
    gfxEQView(NavigationView& nav);
    ~gfxEQView();

    gfxEQView(const gfxEQView&) = delete;
    gfxEQView& operator=(const gfxEQView&) = delete;

    void focus() override;
    std::string title() const override { return "gfxEQ"; }
<<<<<<< HEAD
<<<<<<< HEAD

    void on_freqchg(int64_t freq);

   private:
=======
    void on_show() override;
    void on_hide() override;
=======
>>>>>>> 00853f52 (Gfx widget and Radio (#2685))

    void on_freqchg(int64_t freq);

   private:
<<<<<<< HEAD
    static constexpr ui::Dim header_height = 2 * 16;
    static constexpr int RENDER_HEIGHT = 288;
    static constexpr int NUM_BARS = 11;
    static constexpr int BAR_SPACING = 2;
    int BAR_WIDTH = (screen_width - (BAR_SPACING * (NUM_BARS - 1))) / NUM_BARS;
    static constexpr int HORIZONTAL_OFFSET = 2;
    static constexpr int SEGMENT_HEIGHT = 10;

    static constexpr std::array<int, NUM_BARS + 1> FREQUENCY_BANDS = {
        375,    // Bass warmth and low rumble (e.g., deep basslines, kick drum body)
        750,    // Upper bass punch (e.g., bass guitar punch, kick drum attack)
        1500,   // Lower midrange fullness (e.g., warmth in vocals, guitar body)
        2250,   // Midrange clarity (e.g., vocal presence, snare crack)
        3375,   // Upper midrange bite (e.g., instrument definition, vocal articulation)
        4875,   // Presence and edge (e.g., guitar bite, vocal sibilance start)
        6750,   // Lower brilliance (e.g., cymbal shimmer, vocal clarity)
        9375,   // Brilliance and air (e.g., hi-hat crispness, breathy vocals)
        13125,  // High treble sparkle (e.g., subtle overtones, synth shimmer)
        16875,  // Upper treble airiness (e.g., faint harmonics, room ambiance)
        20625,  // Top-end sheen (e.g., ultra-high harmonics, noise floor)
        24375   // Extreme treble limit (e.g., inaudible overtones, signal cutoff, static)
    };

>>>>>>> 288f6bd5 (GFX EQ App (#2607))
=======
>>>>>>> 00853f52 (Gfx widget and Radio (#2685))
    struct ColorTheme {
        Color base_color;
        Color peak_color;
    };

    NavigationView& nav_;
<<<<<<< HEAD
<<<<<<< HEAD

=======
    bool needs_background_redraw{false};
    std::vector<int> bar_heights;
    std::vector<int> prev_bar_heights;
>>>>>>> 288f6bd5 (GFX EQ App (#2607))
=======

>>>>>>> 00853f52 (Gfx widget and Radio (#2685))
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
<<<<<<< HEAD
<<<<<<< HEAD
    AudioVolumeField field_volume{{screen_width - 2 * 8, 0 * 16}};
    GraphEq gr{{2, UI_POS_DEFAULT_HEIGHT, UI_POS_MAXWIDTH - 4, UI_POS_HEIGHT_REMAINING(2)}, false};
<<<<<<< HEAD
=======
    AudioVolumeField field_volume{{28 * 8, 0 * 16}};
>>>>>>> 288f6bd5 (GFX EQ App (#2607))
=======
    AudioVolumeField field_volume{{screen_width - 2 * 8, 0 * 16}};
>>>>>>> 3c8335c9 (Audio to right (#2664))
=======
>>>>>>> 00853f52 (Gfx widget and Radio (#2685))

    rf::Frequency frequency_value{93100000};

    RxRadioState rx_radio_state_{};

    app_settings::SettingsManager settings_{
        "rx_gfx_eq",
        app_settings::Mode::RX,
        {{"theme", &current_theme},
         {"frequency", &frequency_value}}};

<<<<<<< HEAD
<<<<<<< HEAD
=======
    void update_audio_spectrum(const AudioSpectrum& spectrum);
    void render_equalizer(Painter& painter);
>>>>>>> 288f6bd5 (GFX EQ App (#2607))
=======
>>>>>>> 00853f52 (Gfx widget and Radio (#2685))
    void cycle_theme();

    MessageHandlerRegistration message_handler_audio_spectrum{
        Message::ID::AudioSpectrum,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const AudioSpectrumMessage*>(p);
<<<<<<< HEAD
<<<<<<< HEAD
            this->gr.update_audio_spectrum(*message.data);
=======
            this->update_audio_spectrum(*message.data);
            this->set_dirty();
>>>>>>> 288f6bd5 (GFX EQ App (#2607))
=======
            this->gr.update_audio_spectrum(*message.data);
>>>>>>> 00853f52 (Gfx widget and Radio (#2685))
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