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
    baseband::run_image(spi_flash::image_tag_wfm_audio);

    add_children({&field_frequency, &field_rf_amp, &field_lna, &field_vga,
                  &button_mood, &field_volume, &text_ctcss});

    field_frequency.set_step(25000);
    field_frequency.set_value(frequency_value);
    field_rf_amp.set_value(rf_amp_value);
    field_lna.set_value(lna_gain_value);
    field_vga.set_value(vga_gain_value);
    field_volume.set_value(volume_value);

    receiver_model.set_rf_amp(rf_amp_value);
    receiver_model.set_lna(lna_gain_value);
    receiver_model.set_vga(vga_gain_value);
    receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);
    receiver_model.set_sampling_rate(3072000);
    receiver_model.set_target_frequency(frequency_value);
    receiver_model.enable();

    audio::output::start();

    field_rf_amp.on_change = [this](bool v) {
        rf_amp_value = v;
        receiver_model.set_rf_amp(v);
    };
    field_lna.on_change = [this](int32_t v) {
        lna_gain_value = v;
        receiver_model.set_lna(v);
    };
    field_vga.on_change = [this](int32_t v) {
        vga_gain_value = v;
        receiver_model.set_vga(v);
    };
    button_mood.on_select = [this](Button&) { this->cycle_theme(); };
}

gfxEQView::~gfxEQView() {
    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();
}

void gfxEQView::focus() {
    field_frequency.focus();
}

void gfxEQView::on_show() {
    needs_background_redraw = true;
    set_dirty();
}

void gfxEQView::on_hide() {
    needs_background_redraw = true;
}

void gfxEQView::start() {
    if (!running) {
        running = true;
    }
}

void gfxEQView::stop() {
    if (running) {
        running = false;
    }
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
    if (!initialized) {
        initialized = true;
        start();
        painter.fill_rectangle({0, header_height, SCREEN_WIDTH, RENDER_HEIGHT}, Color(0, 0, 0));
    } else if (needs_background_redraw) {
        painter.fill_rectangle({0, header_height, SCREEN_WIDTH, RENDER_HEIGHT}, Color(0, 0, 0));
        needs_background_redraw = false;
    }
    render_equalizer(painter);
}

void gfxEQView::on_frequency_step_changed(rf::Frequency f) {
    receiver_model.set_frequency_step(f);
    field_frequency.set_step(f);
}

void gfxEQView::on_reference_ppm_correction_changed(int32_t v) {
    persistent_memory::set_correction_ppb(v * 1000);
}

void gfxEQView::remove_options_widget() {
    if (options_widget) {
        remove_child(options_widget.get());
        options_widget.reset();
    }
    field_rf_amp.set_style(nullptr);
    field_lna.set_style(nullptr);
    field_vga.set_style(nullptr);
    field_frequency.set_style(nullptr);
}

void gfxEQView::set_options_widget(std::unique_ptr<Widget> new_widget) {
    remove_options_widget();
    if (new_widget) {
        options_widget = std::move(new_widget);
    } else {
        options_widget = std::make_unique<Rectangle>(options_view_rect, Theme::getInstance()->option_active->background);
    }
    add_child(options_widget.get());
}

void gfxEQView::update_modulation(ReceiverModel::Mode modulation) {
    audio::output::mute();
    baseband::shutdown();

    spi_flash::image_tag_t image_tag;
    switch (modulation) {
        case ReceiverModel::Mode::AMAudio:
            image_tag = spi_flash::image_tag_am_audio;
            break;
        case ReceiverModel::Mode::NarrowbandFMAudio:
            image_tag = spi_flash::image_tag_nfm_audio;
            break;
        case ReceiverModel::Mode::WidebandFMAudio:
            image_tag = spi_flash::image_tag_wfm_audio;
            break;
        case ReceiverModel::Mode::SpectrumAnalysis:
            image_tag = spi_flash::image_tag_wideband_spectrum;
            break;
        default:
            image_tag = spi_flash::image_tag_wfm_audio;
            break;
    }

    baseband::run_image(image_tag);
    if (modulation == ReceiverModel::Mode::SpectrumAnalysis) {
        baseband::set_spectrum(receiver_model.sampling_rate(), 40);
    }

    receiver_model.set_modulation(modulation);
    receiver_model.set_sampling_rate(3072000);
    receiver_model.enable();

    if (modulation != ReceiverModel::Mode::SpectrumAnalysis) {
        audio::output::unmute();
    }
}

void gfxEQView::handle_coded_squelch(uint32_t value) {
    text_ctcss.set(tonekey::tone_key_string_by_value(value, text_ctcss.parent_rect().width() / 8));
}

void gfxEQView::cycle_theme() {
    current_theme = (current_theme + 1) % themes.size();
    set_dirty();
}

}  // namespace ui::external_app::gfxeq