#include "ui_secplustx.hpp"

#include <string_view>
#include <string>
#include <utility>
#include <vector>

#include "secplustx.hpp"
#include "ui_fileman.hpp"
#include "file_reader.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "optional.hpp"

using namespace portapack;
using namespace ui;

namespace fs = std::filesystem;

namespace ui::external_app::ui_secplustx {

static constexpr uint8_t secplus_repeat_count = 3;
static constexpr float secplus_sample_rate = OOK_SAMPLERATE / 4000.0f;

Optional<SecplusData> SecplusTXView::read_secplus_file(const fs::path& file_path) {
    File file{};
    if (auto error = file.open(file_path)) return {};

    std::string raw = *FileLineReader{file}.begin();
    std::vector chunks = split_string(raw, ';');
    if (chunks.empty()) return {};

    SecplusData data{};
    data.version = static_cast<SecplusVersion>(std::strtoul(chunks[0].data(), NULL, 10));
    switch (data.version) {
        case SecplusVersion::V1:
            return {};  // unimplemented
        case SecplusVersion::V2: {
            if (chunks.size() != 6) return {};
            data.name = std::string{chunks[1]};
            if (data.name.empty()) data.name = "Remote";
            data.has_data = std::strtoul(chunks[2].data(), NULL, 10);
            data.fixed_code = std::strtoull(chunks[3].data(), NULL, 16);
            data.rolling_code = std::strtoul(chunks[4].data(), NULL, 16);
            data.data = std::strtoul(chunks[5].data(), NULL, 16);
            return data;
        }
        default:
            return {};
    }
}

bool SecplusTXView::write_secplus_file(const fs::path& file_path, const SecplusData& data) {
    ensure_directory(secplus_dir);
    File file{};
    if (auto error = file.create(file_path)) {
        return false;
    }

    switch (data.version) {
        case SecplusVersion::V1:
            return false;  // unimplemented
        case SecplusVersion::V2: {
            std::string formatted = to_string_dec_uint(static_cast<uint8_t>(data.version));
            formatted += ';';
            formatted += data.name;
            formatted += ';';
            formatted += to_string_dec_uint(data.has_data);
            formatted += ';';
            formatted += to_string_hex(data.fixed_code);
            formatted += ';';
            formatted += to_string_hex(data.rolling_code);
            formatted += ';';
            formatted += to_string_hex(data.data);
            file.write_line(formatted);
            file.close();
            return true;
        }
        default:
            return false;
    }
}

SecplusTXView::SecplusTXView(NavigationView& nav)
    : nav_{nav} {
    if (!fs::file_exists(file_path)) write_secplus_file(file_path, data);
    baseband::run_image(portapack::spi_flash::image_tag_ook);

    add_children({&button_open,
                  &button_save,
                  &field_name,
                  &labels,
                  &field_fixed,
                  &field_rolling,
                  &has_data,
                  &field_data,
                  &learn_mode,
                  &autosave,
                  &progressbar,
                  &tx_view});

    button_open.on_select = [this](const Button&) {
        ensure_directory(secplus_dir);
        auto open_view = nav_.push<FileLoadView>(".SECPLUS");
        open_view->push_dir(secplus_dir);
        open_view->on_changed = [this](fs::path path) { file_path = path.string(); };
        nav_.set_on_pop([this]() { reload_data(); });
    };
    button_save.on_select = [this](const Button&) { write_secplus_file(file_path, data); };

    field_name.on_select = [this, &nav](TextField&) {
        buffer = data.name;
        text_prompt(nav_, buffer, field_name.parent_rect().width() / UI_POS_DEFAULT_WIDTH, ENTER_KEYBOARD_MODE_ALPHA, [this](std::string& new_name) {
            data.name = new_name;
            field_name.set_text(new_name);
            save_data();
        });
    };
    field_fixed.on_change = [this](SymField& field) { data.fixed_code = field.to_integer(); };
    field_rolling.on_change = [this](SymField& field) { data.rolling_code = field.to_integer(); };
    field_data.on_change = [this](SymField& field) { data.data = field.to_integer(); };
    has_data.on_select = [this](Checkbox& checkbox, bool value) {
        data.has_data = value;
        // fade if inactive, otherwise use default style
        const Style* new_style = value ? &style() : Theme::getInstance()->fg_medium;
        checkbox.set_style(new_style);
        field_data.set_style(new_style);
        field_data.set_focusable(value);
    };
    tx_view.on_edit_frequency = [this]() {
        auto new_view = nav_.push<FrequencyKeypadView>(transmitter_model.target_frequency());
        new_view->on_changed = [](rf::Frequency f) {
            transmitter_model.set_target_frequency(f);
        };
    };

    tx_view.on_start = [this]() { start_tx(); };
    tx_view.on_stop = [this]() {
        baseband::kill_ook();
        stop_tx();
    };

    autosave.set_value(true);
    reload_data();
}

void SecplusTXView::save_data() {
    if (autosave.value()) write_secplus_file(file_path, data);
}

void SecplusTXView::reload_data() {
    if (auto result = read_secplus_file(file_path)) data = std::move(*result);
    // always update data, use default values if read failed
    field_name.set_text(data.name);
    has_data.set_value(data.has_data);
    field_rolling.set_value(data.rolling_code);
    field_fixed.set_value(data.fixed_code);
    field_data.set_value(data.data);
}

void SecplusTXView::start_tx() {
    uint8_t packet1[8]{};
    uint8_t packet2[8]{};
    size_t bitstream_length = 0;

    constexpr uint8_t packet1_indicator = 0b00;
    constexpr uint8_t packet2_indicator = 0b01;
    auto encode_packet = [&bitstream_length](uint8_t indicator, auto& packet, bool has_data) {
        constexpr uint32_t preamble = 0b0000000000000000'1111;
        constexpr uint32_t blank_size = 24;
        auto manchester_encode = [&bitstream_length](auto& x, uint32_t size) {
            for (uint32_t i = 0; i < size; ++i) {
                bool bit = (x >> (size - 1 - i)) & 1;
                encoders::bitstream_append(bitstream_length, 2, bit ? 0b01 : 0b10);
            }
        };
        manchester_encode(preamble, 20);
        manchester_encode(indicator, 2);
        for (uint8_t byte = 0; byte < (has_data ? 8 : 5); ++byte) manchester_encode(packet[byte], 8);
        encoders::bitstream_append(bitstream_length, blank_size, 0);
    };

    if (encode_v2(data.rolling_code, data.fixed_code, data.data, data.has_data, packet1, packet2) < 0) {
        nav_.display_modal("Error", "Invalid rolling/fixed code");
        return;
    }
    encode_packet(packet1_indicator, packet1, has_data.value());
    encode_packet(packet2_indicator, packet2, has_data.value());

    if (!learn_mode.value()) {
        field_rolling.set_value(++data.rolling_code);
        field_rolling.set_dirty();
    }

    progressbar.set_max(secplus_repeat_count);
    progressbar.set_value(1);

    tx_view.set_transmitting(true);
    transmitter_model.enable();
    baseband::set_ook_data(
        bitstream_length,
        secplus_sample_rate,
        secplus_repeat_count,
        0);
}

void SecplusTXView::stop_tx() {
    transmitter_model.disable();
    progressbar.set_value(0);
    tx_view.set_transmitting(false);
}

void SecplusTXView::on_tx_progress(uint32_t progress, bool done) {
    progressbar.set_value(progress + 1);
    if (done) {
        stop_tx();
        save_data();
    }
}

SecplusTXView::~SecplusTXView() {
    transmitter_model.disable();
    baseband::shutdown();
    save_data();
}

}  // namespace ui::external_app::ui_secplustx
