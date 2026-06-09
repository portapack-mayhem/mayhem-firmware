#include "ui_secplustx.hpp"
#include "ui_fileman.hpp"
#include "baseband_api.hpp"
#include <string_view>

#include "secplustx.hpp"

using namespace portapack;
using namespace ui;

namespace fs = std::filesystem;

namespace ui::external_app::ui_secplustx {

SecplusTXView::SecplusTXView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_ook);

    add_children({&labels,
                  &field_fixed,
                  &field_rolling,
                  &field_data,
                  &learn_mode,
                  &progressbar,
                  &tx_view});

    field_rolling.set_value(rolling_code);
    field_fixed.set_value(fixed_code);
    field_rolling.on_change = [this](SymField& field) { rolling_code = field.to_integer(); };
    field_fixed.on_change = [this](SymField& field) { fixed_code = field.to_integer(); };

    tx_view.on_edit_frequency = [this, &nav]() {
        auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.target_frequency());
        new_view->on_changed = [this](rf::Frequency f) {
            transmitter_model.set_target_frequency(f);
        };
    };

    tx_view.on_start = [this]() {
        start_tx();
    };

    tx_view.on_stop = [this]() {
        baseband::kill_ook();
        stop_tx();
    };
}

void SecplusTXView::start_tx() {
    uint8_t packet1[5]{};
    uint8_t packet2[5]{};

    secplustx::encode_v2(rolling_code, fixed_code, 0, 0, packet1, packet2);

    constexpr uint32_t preamble = 0b0000000000000000'1111;
    constexpr uint8_t packet1_indicator = 0b00;
    constexpr uint8_t packet2_indicator = 0b01;
    constexpr uint32_t blank_size = 23;

    size_t bitstream_length = 0;
    auto manchester_encode = [&bitstream_length](auto x, uint32_t size, bool backward = true) {
        for (uint32_t i = 0; i < size; ++i) {
            bool bit = (x >> (backward ? (size - 1) - i : i)) & 1;
            encoders::bitstream_append(bitstream_length, 2, bit ? 0b01 : 0b10);
        }
    };

    manchester_encode(preamble, 20);
    manchester_encode(packet1_indicator, 2);
    for (auto byte : packet1) manchester_encode(byte, 8);
    encoders::bitstream_append(bitstream_length, blank_size, 0);

    manchester_encode(preamble, 20);
    manchester_encode(packet2_indicator, 2);
    for (auto byte : packet2) manchester_encode(byte, 8);
    encoders::bitstream_append(bitstream_length, blank_size, 0);

    if (!learn_mode.value()) {
        rolling_code++;
        field_rolling.set_value(rolling_code);
        field_rolling.set_dirty();
    }

    progressbar.set_max(3);
    tx_view.set_transmitting(true);

    transmitter_model.enable();

    baseband::set_ook_data(
        bitstream_length,
        OOK_SAMPLERATE / 4000.0f,
        3,
        0);
}

void SecplusTXView::stop_tx() {
    transmitter_model.disable();
    progressbar.set_value(0);
    tx_view.set_transmitting(false);
}

void SecplusTXView::on_tx_progress(uint32_t progress, bool done) {
    if (!done) {
        progressbar.set_value(progress + 1);
    } else {
        chThdSleepMilliseconds(10);
        stop_tx();
    }
}

SecplusTXView::~SecplusTXView() {
    transmitter_model.disable();
    baseband::shutdown();
}

}  // namespace ui::external_app::ui_secplustx
