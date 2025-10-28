/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#include "ui_gfxeq.hpp"
#include "ui.hpp"
#include "ui_freqman.hpp"
#include "tone_key.hpp"
#include "analog_audio_app.hpp"
#include "portapack.hpp"
#include "audio.hpp"
#include "baseband_api.hpp"
#include "dsp_fir_taps.hpp"

using namespace portapack;

namespace ui::external_app::gfxeq {

#include "external/ui_grapheq.cpi"

gfxEQView::gfxEQView(NavigationView& nav)
    : nav_{nav} {
    // Get screen dimensions
    const auto screen_width = portapack::display.width();
    const auto screen_height = portapack::display.height();

    // Standard layout approach similar to other apps
    const int16_t controls_y = 4;

    // Position controls with standard spacing
    button_frequency.set_parent_rect({0, controls_y, 11 * 8, 16});
    field_rf_amp.set_parent_rect({12 * 8, controls_y, 2 * 8, 16});
    field_lna.set_parent_rect({14 * 8, controls_y, 2 * 8, 16});
    field_vga.set_parent_rect({17 * 8, controls_y, 3 * 8, 16});
    button_mood.set_parent_rect({21 * 8, controls_y, 4 * 8, 16});
    field_volume.set_parent_rect({screen_width - 2 * 8, controls_y, 2 * 8, 16});

    // Graph positioned 2 pixels to the right
    gr.set_parent_rect({2, controls_y + 20, screen_width, screen_height - controls_y - 24});

    add_children({&button_frequency, &field_rf_amp, &field_lna, &field_vga,
                  &button_mood, &field_volume, &gr});

    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();

    baseband::run_image(portapack::spi_flash::image_tag_wfm_audio);

    receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);
    receiver_model.set_wfm_configuration(1);  // 200k => 0 , 180k => 1 , 80k => 2. Set to 1 or 2 for better reception
    receiver_model.set_sampling_rate(3072000);
    receiver_model.set_baseband_bandwidth(1750000);

    audio::set_rate(audio::Rate::Hz_48000);
    audio::output::start();
    receiver_model.set_headphone_volume(receiver_model.headphone_volume());  // WM8731 hack
    receiver_model.enable();

    receiver_model.set_target_frequency(frequency_value);  // Retune to actual freq
    button_frequency.set_text("<" + to_string_short_freq(frequency_value) + ">");

    button_frequency.on_select = [this, &nav](ButtonWithEncoder& button) {
        auto new_view = nav_.push<FrequencyKeypadView>(frequency_value);
        new_view->on_changed = [this, &button](rf::Frequency f) {
            frequency_value = f;
            receiver_model.set_target_frequency(f);  // Retune to actual freq
            button_frequency.set_text("<" + to_string_short_freq(frequency_value) + ">");
        };
    };

    button_frequency.on_change = [this]() {
        int64_t def_step = 25000;
        frequency_value = frequency_value + (button_frequency.get_encoder_delta() * def_step);
        if (frequency_value < 1) {
            frequency_value = 1;
        }
        if (frequency_value > (MAX_UFREQ - def_step)) {
            frequency_value = MAX_UFREQ;
        }
        button_frequency.set_encoder_delta(0);
        receiver_model.set_target_frequency(frequency_value);  // Retune to actual freq
        button_frequency.set_text("<" + to_string_short_freq(frequency_value) + ">");
    };

    button_mood.on_select = [this](Button&) { this->cycle_theme(); };
    gr.set_theme(themes[current_theme].base_color, themes[current_theme].peak_color);
}

// needed to answer usb serial frequency set
void gfxEQView::on_freqchg(int64_t freq) {
    receiver_model.set_target_frequency(freq);  // Retune to actual freq
    button_frequency.set_text("<" + to_string_short_freq(freq) + ">");
}

gfxEQView::~gfxEQView() {
    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();
}

void gfxEQView::focus() {
    button_frequency.focus();
}

void gfxEQView::cycle_theme() {
    current_theme = (current_theme + 1) % themes.size();
    gr.set_theme(themes[current_theme].base_color, themes[current_theme].peak_color);
}

}  // namespace ui::external_app::gfxeq