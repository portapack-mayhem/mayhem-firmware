#include "ui_rf3d.hpp"
#include "ui.hpp"
#include "ui_freqman.hpp"
#include "tone_key.hpp"
#include "analog_audio_app.hpp"
#include "portapack.hpp"
#include "audio.hpp"

using namespace portapack;

namespace ui::external_app::rf3d {

RF3DView::RF3DView(NavigationView& nav)
    : nav_{nav}, bar_heights(NUM_BARS, 0), prev_bar_heights(NUM_BARS, 0) {
    baseband::run_image(spi_flash::image_tag_wfm_audio);
    add_children({&rssi, &channel, &audio, &field_frequency, &field_lna, &field_vga, 
                  &options_modulation, &field_volume, &text_ctcss, &record_view, &dummy});

    field_lna.on_show_options = [this]() { this->on_show_options_rf_gain(); };
    field_vga.on_show_options = [this]() { this->on_show_options_rf_gain(); };

    receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);
    receiver_model.set_sampling_rate(3072000);
    receiver_model.set_baseband_bandwidth(40000);
    receiver_model.set_target_frequency(93100000);
    receiver_model.set_rf_amp(true);
    receiver_model.enable();

    field_lna.set_value(40);
    field_vga.set_value(62);

    options_modulation.set_by_value(toUType(ReceiverModel::Mode::WidebandFMAudio));
    options_modulation.on_change = [this](size_t, OptionsField::value_t v) {
        this->on_modulation_changed(static_cast<ReceiverModel::Mode>(v));
    };
    options_modulation.on_show_options = [this]() { this->on_show_options_modulation(); };

    field_frequency.set_value(93100000);

    record_view.set_filename_date_frequency(true);
    record_view.on_error = [&nav](std::string message) {
        nav.display_modal("Error", message);
    };

    record_view.set_sampling_rate(48000);
    audio::output::start();
}

RF3DView::~RF3DView() {
    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();
}

void RF3DView::focus() {
    field_frequency.focus();
}

void RF3DView::start() {
    if (!running) {
        running = true;
    }
}

void RF3DView::stop() {
    if (running) {
        running = false;
    }
}

void RF3DView::update_spectrum(const AudioSpectrum& spectrum) {
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

void RF3DView::render_equalizer(Painter& painter) {
    const int num_bars = SCREEN_WIDTH / (BAR_WIDTH + BAR_SPACING);
    const int num_segments = RENDER_HEIGHT / SEGMENT_HEIGHT;

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

            Color segment_color = (seg >= active_segments - 2 && seg < active_segments) ? Color(255, 255, 255) : Color(255, 0, 255);
            painter.fill_rectangle({x, y, BAR_WIDTH, SEGMENT_HEIGHT - 1}, segment_color);
        }

        prev_bar_heights[bar] = active_segments;
    }
}

void RF3DView::paint(Painter& painter) {
    if (!initialized) {
        initialized = true;
        start();
        painter.fill_rectangle({0, header_height, SCREEN_WIDTH, RENDER_HEIGHT}, Color(0, 0, 0));
    }
    render_equalizer(painter);
}

void RF3DView::on_modulation_changed(ReceiverModel::Mode modulation) {
    stop();
    update_modulation(modulation);
    on_show_options_modulation();
    start();
}

void RF3DView::on_show_options_rf_gain() {
    auto widget = std::make_unique<RadioGainOptionsView>(options_view_rect, Theme::getInstance()->option_active);
    set_options_widget(std::move(widget));
    field_lna.set_style(Theme::getInstance()->option_active);
}

void RF3DView::on_show_options_modulation() {
    std::unique_ptr<Widget> widget;
    const auto modulation = receiver_model.modulation();
    switch (modulation) {
        case ReceiverModel::Mode::AMAudio:
            widget = std::make_unique<ui::AMOptionsView>(nullptr, options_view_rect, Theme::getInstance()->option_active);
            text_ctcss.hidden(true);
            break;
        case ReceiverModel::Mode::NarrowbandFMAudio:
            widget = std::make_unique<ui::NBFMOptionsView>(options_view_rect, Theme::getInstance()->option_active);
            text_ctcss.hidden(false);
            break;
        case ReceiverModel::Mode::WidebandFMAudio:
            widget = std::make_unique<ui::WFMOptionsView>(options_view_rect, Theme::getInstance()->option_active);
            text_ctcss.hidden(true);
            break;
        case ReceiverModel::Mode::SpectrumAnalysis:
            widget = std::make_unique<ui::SPECOptionsView>(nullptr, options_view_rect, Theme::getInstance()->option_active);
            text_ctcss.hidden(true);
            break;
        default:
            break;
    }
    set_options_widget(std::move(widget));
    options_modulation.set_style(Theme::getInstance()->option_active);
}

void RF3DView::on_frequency_step_changed(rf::Frequency f) {
    receiver_model.set_frequency_step(f);
    field_frequency.set_step(f);
}

void RF3DView::on_reference_ppm_correction_changed(int32_t v) {
    persistent_memory::set_correction_ppb(v * 1000);
}

void RF3DView::remove_options_widget() {
    if (options_widget) {
        remove_child(options_widget.get());
        options_widget.reset();
    }
    field_lna.set_style(nullptr);
    options_modulation.set_style(nullptr);
    field_frequency.set_style(nullptr);
}

void RF3DView::set_options_widget(std::unique_ptr<Widget> new_widget) {
    remove_options_widget();
    if (new_widget) {
        options_widget = std::move(new_widget);
    } else {
        options_widget = std::make_unique<Rectangle>(options_view_rect, Theme::getInstance()->option_active->background);
    }
    add_child(options_widget.get());
}

void RF3DView::update_modulation(ReceiverModel::Mode modulation) {
    audio::output::mute();
    record_view.stop();
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
        baseband::set_spectrum(receiver_model.sampling_rate(), 63);
    }

    receiver_model.set_modulation(modulation);
    receiver_model.set_sampling_rate(3072000);
    receiver_model.set_rf_amp(true);
    receiver_model.enable();

    size_t record_sampling_rate = 0;
    switch (modulation) {
        case ReceiverModel::Mode::AMAudio:
            record_sampling_rate = 12000;
            break;
        case ReceiverModel::Mode::NarrowbandFMAudio:
            record_sampling_rate = 24000;
            break;
        case ReceiverModel::Mode::WidebandFMAudio:
            record_sampling_rate = 48000;
            break;
        default:
            break;
    }
    record_view.set_sampling_rate(record_sampling_rate);

    if (modulation != ReceiverModel::Mode::SpectrumAnalysis) {
        audio::output::unmute();
    }
}

void RF3DView::handle_coded_squelch(uint32_t value) {
    text_ctcss.set(tonekey::tone_key_string_by_value(value, text_ctcss.parent_rect().width() / 8));
}

} // namespace ui::external_app::rf3d