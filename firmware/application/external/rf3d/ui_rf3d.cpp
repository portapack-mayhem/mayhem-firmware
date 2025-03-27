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
    : nav_{nav}, spectrum_data(SCREEN_WIDTH, std::vector<uint8_t>(MAX_RENDER_DEPTH, 0)) {
    baseband::run_image(portapack::spi_flash::image_tag_nfm_audio);
    add_children({&rssi, &channel, &audio, &field_frequency, &field_lna, &field_vga, 
                  &options_modulation, &field_volume, &text_ctcss, &record_view, &dummy});

    field_frequency.on_show_options = [this]() { this->on_show_options_frequency(); };
    field_lna.on_show_options = [this]() { this->on_show_options_rf_gain(); };
    field_vga.on_show_options = [this]() { this->on_show_options_rf_gain(); };

    auto modulation = receiver_model.modulation();
    options_modulation.set_by_value(toUType(modulation));
    options_modulation.on_change = [this](size_t, OptionsField::value_t v) {
        this->on_modulation_changed(static_cast<ReceiverModel::Mode>(v));
    };
    options_modulation.on_show_options = [this]() { this->on_show_options_modulation(); };

    record_view.set_filename_date_frequency(true);
    record_view.on_error = [&nav](std::string message) {
        nav.display_modal("Error", message);
    };

    audio::output::start();
    on_modulation_changed(modulation);
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
        baseband::spectrum_streaming_start();
        running = true;
    }
}

void RF3DView::stop() {
    if (running) {
        baseband::spectrum_streaming_stop();
        running = false;
    }
}

void RF3DView::update_spectrum(const ChannelSpectrum& spectrum) {
    for (int x = SCREEN_WIDTH - 1; x > 0; x--) {
        for (int z = 0; z < MAX_RENDER_DEPTH; z++) {
            spectrum_data[x][z] = spectrum_data[x - 1][z];
        }
    }
    for (int z = 0; z < MAX_RENDER_DEPTH; z++) {
        int index = z * SCREEN_WIDTH / MAX_RENDER_DEPTH;
        spectrum_data[0][z] = spectrum.db[index];
    }
    sampling_rate = spectrum.sampling_rate;
}

void RF3DView::render_3d_waterfall(Painter& painter) {
    painter.fill_rectangle({0, header_height, SCREEN_WIDTH, RENDER_HEIGHT}, Color::black());

    for (int x = 0; x < SCREEN_WIDTH; x++) {
        for (int z = 0; z < MAX_RENDER_DEPTH; z++) {
            int screen_x = x;
            int height = spectrum_data[x][z] * RENDER_HEIGHT / 255;
            int screen_y = SCREEN_HEIGHT - height;

            uint8_t r = spectrum_data[x][z];
            uint8_t g = 255 - spectrum_data[x][z];
            uint8_t b = 0;

            if (screen_y < SCREEN_HEIGHT) {
                painter.fill_rectangle({screen_x, screen_y, 1, height}, Color(r, g, b));
            }
        }
    }
}

void RF3DView::paint(Painter& painter) {
    if (!initialized) {
        initialized = true;
        start();
    }
    render_3d_waterfall(painter);
}

void RF3DView::frame_sync() {
    if (running && channel_fifo) {
        ChannelSpectrum channel_spectrum;
        while (channel_fifo->out(channel_spectrum)) {
            update_spectrum(channel_spectrum);
        }
        set_dirty();
    }
}

void RF3DView::on_modulation_changed(ReceiverModel::Mode modulation) {
    baseband::spectrum_streaming_stop();
    update_modulation(modulation);
    on_show_options_modulation();
    baseband::spectrum_streaming_start();
}

void RF3DView::on_show_options_frequency() {
    auto widget = std::make_unique<FrequencyOptionsView>(options_view_rect, Theme::getInstance()->option_active);
    widget->set_step(receiver_model.frequency_step());
    widget->on_change_step = [this](rf::Frequency f) { this->on_frequency_step_changed(f); };
    widget->set_reference_ppm_correction(persistent_memory::correction_ppb() / 1000);
    widget->on_change_reference_ppm_correction = [this](int32_t v) { this->on_reference_ppm_correction_changed(v); };
    set_options_widget(std::move(widget));
    field_frequency.set_style(Theme::getInstance()->option_active);
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

    portapack::spi_flash::image_tag_t image_tag;
    switch (modulation) {
        case ReceiverModel::Mode::AMAudio:
            image_tag = portapack::spi_flash::image_tag_am_audio;
            break;
        case ReceiverModel::Mode::NarrowbandFMAudio:
            image_tag = portapack::spi_flash::image_tag_nfm_audio;
            break;
        case ReceiverModel::Mode::WidebandFMAudio:
            image_tag = portapack::spi_flash::image_tag_wfm_audio;
            break;
        case ReceiverModel::Mode::SpectrumAnalysis:
            image_tag = portapack::spi_flash::image_tag_wideband_spectrum;
            break;
        default:
            image_tag = portapack::spi_flash::image_tag_nfm_audio;
            break;
    }

    baseband::run_image(image_tag);
    if (modulation == ReceiverModel::Mode::SpectrumAnalysis) {
        baseband::set_spectrum(sampling_rate, 63);
    }

    receiver_model.set_modulation(modulation);
    receiver_model.set_sampling_rate(modulation == ReceiverModel::Mode::SpectrumAnalysis ? sampling_rate : 3072000);
    receiver_model.set_baseband_bandwidth(modulation == ReceiverModel::Mode::SpectrumAnalysis ? sampling_rate / 2 : 1750000);
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