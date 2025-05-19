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

void gfxEQView::on_show() {
    needs_background_redraw = true;
}

void gfxEQView::on_hide() {
    needs_background_redraw = true;
}

void gfxEQView::update_audio_spectrum(const AudioSpectrum& spectrum) {
    const float bin_frequency_size = 48000.0f / 128;

    for (int bar = 0; bar < NUM_BARS; bar++) {
        float start_freq = FREQUENCY_BANDS[bar];
        float end_freq = FREQUENCY_BANDS[bar + 1];

        int start_bin = std::max(1, (int)(start_freq / bin_frequency_size));
        int end_bin = std::min(127, (int)(end_freq / bin_frequency_size));

        if (start_bin >= end_bin) {
            end_bin = start_bin + 1;
        }

        float total_energy = 0;
        int bin_count = 0;

        for (int bin = start_bin; bin <= end_bin; bin++) {
            total_energy += spectrum.db[bin];
            bin_count++;
        }

        float avg_db = bin_count > 0 ? (total_energy / bin_count) : 0;

        // Manually boost highs for better visual balance
        float treble_boost = 1.0f;
        if (bar == 10)
            treble_boost = 1.7f;
        else if (bar >= 9)
            treble_boost = 1.3f;
        else if (bar >= 7)
            treble_boost = 1.3f;

        // Mid emphasis for a V-shape effect
        float mid_boost = 1.0f;
        if (bar == 4 || bar == 5 || bar == 6) mid_boost = 1.2f;

        float amplified_db = avg_db * treble_boost * mid_boost;

        if (amplified_db > 255) amplified_db = 255;

        float band_scale = 1.0f;
        int target_height = (amplified_db * RENDER_HEIGHT * band_scale) / 255;

        if (target_height > RENDER_HEIGHT) {
            target_height = RENDER_HEIGHT;
        }

        // Adjusted to look nice to my eyes
        float rise_speed = 0.8f;
        float fall_speed = 1.0f;

        if (target_height > bar_heights[bar]) {
            bar_heights[bar] = bar_heights[bar] * (1.0f - rise_speed) + target_height * rise_speed;
        } else {
            bar_heights[bar] = bar_heights[bar] * (1.0f - fall_speed) + target_height * fall_speed;
        }
    }
}

void gfxEQView::render_equalizer(Painter& painter) {
    const int num_segments = RENDER_HEIGHT / SEGMENT_HEIGHT;
    const ColorTheme& theme = themes[current_theme];

    for (int bar = 0; bar < NUM_BARS; bar++) {
        int x = HORIZONTAL_OFFSET + bar * (BAR_WIDTH + BAR_SPACING);
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