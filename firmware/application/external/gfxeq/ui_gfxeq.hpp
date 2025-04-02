#ifndef __UI_GFXEQ_HPP__
#define __UI_GFXEQ_HPP__

#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "message.hpp"
#include "baseband_api.hpp"
#include "portapack.hpp"
#include "ui_record_view.hpp"
#include "ui_spectrum.hpp"
#include "ui_freq_field.hpp"

namespace ui::external_app::gfxeq {

class gfxEQView : public View {
   public:
    gfxEQView(NavigationView& nav);
    ~gfxEQView();

    gfxEQView(const gfxEQView&) = delete;
    gfxEQView& operator=(const gfxEQView&) = delete;

    void focus() override;
    std::string title() const override { return "gfxEQ"; }

    void paint(Painter& painter) override;

   private:
    static constexpr ui::Dim header_height = 3 * 16;
    static constexpr int SCREEN_WIDTH = 240;
    static constexpr int SCREEN_HEIGHT = 320;
    static constexpr int RENDER_HEIGHT = 280;
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
    std::vector<int> bar_heights;
    std::vector<int> prev_bar_heights;
    bool running{false};
    size_t current_theme{0};
    const std::array<ColorTheme, 20> themes{
        ColorTheme{Color(255, 0, 255), Color(255, 255, 255)},    // Neon Fury
        ColorTheme{Color(0, 255, 0), Color(255, 0, 0)},          // Toxic Blaze
        ColorTheme{Color(0, 0, 255), Color(255, 255, 0)},        // Midnight Venom
        ColorTheme{Color(255, 128, 0), Color(255, 0, 128)},      // Inferno Pulse
        ColorTheme{Color(128, 0, 255), Color(0, 255, 255)},      // Cyber Vortex
        ColorTheme{Color(255, 255, 0), Color(0, 255, 128)},      // Solar Flare
        ColorTheme{Color(255, 0, 0), Color(0, 128, 255)},        // Blood Eclipse
        ColorTheme{Color(0, 255, 128), Color(255, 128, 255)},    // Acid Horizon
        ColorTheme{Color(128, 128, 128), Color(255, 255, 255)},  // Steel Phantom
        ColorTheme{Color(255, 64, 0), Color(0, 255, 64)},        // Ember Storm
        ColorTheme{Color(0, 128, 128), Color(255, 192, 0)},      // Teal Abyss
        ColorTheme{Color(0, 255, 0), Color(0, 128, 0)},          // Matrix Rain
        ColorTheme{Color(32, 64, 32), Color(0, 255, 0)},         // Hacker Terminal
        ColorTheme{Color(64, 0, 128), Color(255, 0, 255)},       // BBS Neon
        ColorTheme{Color(0, 64, 0), Color(0, 255, 128)},         // CRT Glow
        ColorTheme{Color(255, 255, 255), Color(0, 0, 255)},      // Digital Grid
        ColorTheme{Color(128, 0, 0), Color(255, 128, 0)},        // Redline Hack
        ColorTheme{Color(0, 128, 255), Color(255, 255, 128)},    // Cybernet Blue
        ColorTheme{Color(64, 64, 64), Color(255, 0, 0)},         // Shadow Net
        ColorTheme{Color(255, 192, 0), Color(0, 64, 128)}        // Amber Code
    };

    RxFrequencyField field_frequency{{5 * 8, 0 * 16}, nav_};
    LNAGainField field_lna{Point{15 * 8, 0 * 16}};
    VGAGainField field_vga{Point{18 * 8, 0 * 16}};
    OptionsField options_modulation{
        {0 * 8, 0 * 16},
        4,
        {{"AM  ", toUType(ReceiverModel::Mode::AMAudio)},
         {"NFM ", toUType(ReceiverModel::Mode::NarrowbandFMAudio)},
         {"WFM ", toUType(ReceiverModel::Mode::WidebandFMAudio)},
         {"SPEC", toUType(ReceiverModel::Mode::SpectrumAnalysis)}}};
    AudioVolumeField field_volume{Point{28 * 8, 0 * 16}};
    Text text_ctcss{{16 * 8, 1 * 16, 14 * 8, 1 * 16}, ""};
    RecordView record_view{
        {0 * 8, 2 * 16, 30 * 8, 1 * 16},
        u"AUD",
        u"AUDIO",
        RecordView::FileType::WAV,
        4096,
        4};
    const Rect options_view_rect{0 * 8, 1 * 16, 30 * 8, 1 * 16};
    std::unique_ptr<Widget> options_widget{};
    Button button_mood{{21 * 8, 0, 6 * 8, 16}, "MOOD"};
    Button dummy{{240, 0, 0, 0}, ""};

    void start();
    void stop();
    void update_audio_spectrum(const AudioSpectrum& spectrum);
    void render_equalizer(Painter& painter);
    void on_modulation_changed(ReceiverModel::Mode modulation);
    void on_show_options_rf_gain();
    void on_show_options_modulation();
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