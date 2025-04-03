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

gfxEQView::gfxEQView(NavigationView& nav)
    : nav_{nav}, bar_heights(NUM_BARS, 0), prev_bar_heights(NUM_BARS, 0) {
    add_children({&button_frequency, &field_rf_amp, &field_lna, &field_vga,
                  &button_mood, &field_volume});

    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();

    baseband::run_image(portapack::spi_flash::image_tag_wfm_audio);

    receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);
    receiver_model.set_wfm_configuration(1);  // 200k => 0 , 180k => 1 , 40k => 2. Set to 1 or 2 for better reception
    receiver_model.set_sampling_rate(3072000);
    receiver_model.set_baseband_bandwidth(1750000);

    audio::set_rate(audio::Rate::Hz_48000);
    audio::output::start();
    receiver_model.set_headphone_volume(receiver_model.headphone_volume());  // WM8731 hack
                                                                             //
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
}

gfxEQView::~gfxEQView() {
    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();
}

void gfxEQView::focus() {
    button_frequency.focus();
}

void gfxEQView::on_show() {
    needs_background_redraw = true;
}

void gfxEQView::on_hide() {
    needs_background_redraw = true;
}

void gfxEQView::update_audio_spectrum(const AudioSpectrum& spectrum) {
    const int bins_per_bar = 128 / NUM_BARS;
    for (int bar = 0; bar < NUM_BARS; bar++) {
        int start_bin = bar * bins_per_bar;
        uint8_t max_db = 0;
        for (int bin = start_bin; bin < start_bin + bins_per_bar; bin++) {
            if (spectrum.db[bin] > max_db) {
                max_db = spectrum.db[bin];
            }
        }
        int height = (max_db * RENDER_HEIGHT) / 255;
        bar_heights[bar] = height;
    }
}

void gfxEQView::render_equalizer(Painter& painter) {
    const int num_bars = SCREEN_WIDTH / (BAR_WIDTH + BAR_SPACING);
    const int num_segments = RENDER_HEIGHT / SEGMENT_HEIGHT;
    const ColorTheme& theme = themes[current_theme];

    for (int bar = 0; bar < num_bars; bar++) {
        int x = bar * (BAR_WIDTH + BAR_SPACING);
        int active_segments = (bar_heights[bar] * num_segments) / RENDER_HEIGHT;

        if (prev_bar_heights[bar] > active_segments) {
            int clear_height = (prev_bar_heights[bar] - active_segments) * SEGMENT_HEIGHT;
            int clear_y = SCREEN_HEIGHT - prev_bar_heights[bar] * SEGMENT_HEIGHT;
            painter.fill_rectangle({x, clear_y, BAR_WIDTH, clear_height}, Color(0, 0, 0));
        }

        for (int seg = 0; seg < active_segments; seg++) {
            int y = SCREEN_HEIGHT - (seg + 1) * SEGMENT_HEIGHT;
            if (y < header_height) break;

            Color segment_color = (seg >= active_segments - 2 && seg < active_segments) ? theme.peak_color : theme.base_color;
            painter.fill_rectangle({x, y, BAR_WIDTH, SEGMENT_HEIGHT - 1}, segment_color);
        }

        prev_bar_heights[bar] = active_segments;
    }
}

void gfxEQView::paint(Painter& painter) {
    if (needs_background_redraw) {
        painter.fill_rectangle({0, header_height, SCREEN_WIDTH, RENDER_HEIGHT}, Color(0, 0, 0));
        needs_background_redraw = false;
    }
    render_equalizer(painter);
}

void gfxEQView::cycle_theme() {
    current_theme = (current_theme + 1) % themes.size();
}

}  // namespace ui::external_app::gfxeq
