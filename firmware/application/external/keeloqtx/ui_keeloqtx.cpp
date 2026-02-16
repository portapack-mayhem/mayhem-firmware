#include "ui_keeloqtx.hpp"
#include "ui_fileman.hpp"
#include "baseband_api.hpp"
#include "../../keeloq_common.hpp"
#include "../../baseband/fprotos/fprotogeneral.hpp"

using namespace portapack;
using namespace ui;

namespace fs = std::filesystem;

namespace ui::external_app::ui_keeloqtx {

void KeeloqTXView::focus() {
    button_open.focus();
}

void KeeloqTXView::update_hop() {
    hop = data.btn << 28 | (data.serial & 0x3FF) << 16 | data.counter;

    if (data.mf_name == "Aprimatic") {
        uint32_t apri_serial = data.serial;
        uint8_t apr1 = 0;

        for (uint16_t i = 1; i != 0b10000000000; i <<= 1) {
            if (apri_serial & i) apr1++;
        }

        apri_serial &= 0b00001111111111;

        if (apr1 % 2 == 0) {
            apri_serial |= 0b110000000000;
        }

        hop = data.btn << 28 | (apri_serial & 0xFFF) << 16 | data.counter;
    } else if (
        data.mf_name == "DTM_Neo" || data.mf_name == "FAAC_RC,XT" || data.mf_name == "Mutanco_Mutancode" || data.mf_name == "Came_Space" || data.mf_name == "Genius_Bravo" || data.mf_name == "GSN" || data.mf_name == "Rosh" || data.mf_name == "Rossi" || data.mf_name == "Peccinin" || data.mf_name == "Steelmate" || data.mf_name == "Cardin_S449") {
        hop = data.btn << 28 | (data.serial & 0xFFF) << 16 | data.counter;
    } else if (
        data.mf_name == "NICE_Smilo" || data.mf_name == "NICE_MHOUSE" || data.mf_name == "JCM_Tech") {
        hop = data.btn << 28 | (data.serial & 0xFF) << 16 | data.counter;
    } else if (data.mf_name == "Merlin") {
        hop = data.btn << 28 | (0x000) << 16 | data.counter;
    } else if (data.mf_name == "Centurion") {
        hop = data.btn << 28 | (0x1CE) << 16 | data.counter;
    } else if (data.mf_name == "Monarch") {
        hop = data.btn << 28 | (0x100) << 16 | data.counter;
    } else if (data.mf_name == "Dea_Mio") {
        uint8_t first_disc_num = (data.serial >> 8) & 0xF;
        uint8_t result_disc = (0xC + (first_disc_num % 4));

        uint32_t dea_serial = (data.serial & 0xFF) | (((uint32_t)result_disc) << 8);

        hop = data.btn << 28 | (dea_serial & 0xFFF) << 16 | data.counter;
    }

    text_hop.set(to_string_hex(hop));
}

void KeeloqTXView::update_payload() {
    uint64_t encrypt = 0;

    switch (current_key.type) {
        case KEELOQ_SIMPLE_LEARNING: {
            encrypt = keeloq_encrypt(hop, current_key.key);

            break;
        }
        case KEELOQ_NORMAL_LEARNING: {
            uint64_t man = keeloq_normal_learning(fix, current_key.key);

            encrypt = keeloq_encrypt(hop, man);

            break;
        }
    }

    payload = (uint64_t)fix << 32 | encrypt;

    uint64_t preview_payload = FProtoGeneral::subghz_protocol_blocks_reverse_key(payload, 64);

    text_payload.set(to_string_hex(preview_payload));
    encode_data();
}

void KeeloqTXView::encode_data() {
    std::string fragments{};

    for (uint32_t i = 0; i < 64; ++i) {
        fragments += keeloq_fragments[bit(payload, i)];
    }

    encoded_data = KEELOQ_HEADER + fragments + "1001";

    if (data.mf_name == "Sommer") {
        pause_duration = 28;
    } else {
        pause_duration = 39;
    }
}

KeeloqTXView::KeeloqTXView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_ook);

    add_children({&labels,
                  &text_mf_name,
                  &text_serial,
                  &text_fix,
                  &text_hop,
                  &field_counter,
                  &field_button,
                  &field_repeat,
                  &text_payload,
                  &button_open,
                  &text_status,
                  &progressbar,
                  &tx_view});

    field_counter.on_change = [this](uint32_t value) {
        data.counter = (uint16_t)value;

        update_hop();
        update_payload();
    };

    field_button.on_change = [this](uint32_t value) {
        data.btn = (uint8_t)value;

        fix &= 0xFFFFFFF;
        fix |= data.btn << 28;

        text_fix.set(to_string_hex(fix));

        update_hop();
        update_payload();
    };

    field_repeat.on_change = [this](uint32_t value) {
        repeat = value;
    };

    field_repeat.set_value(4, true);

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

    button_open.on_select = [this](const Button&) {
        auto open_view = nav_.push<FileLoadView>(".KEELOQ");

        ensure_directory(keeloq_remotes_dir);
        open_view->push_dir(keeloq_remotes_dir);
        open_view->on_changed = [this](fs::path path) {
            nav_.set_on_pop([this, path]() {
                if (!read_keeloq_file(path, data)) {
                    return;
                }

                file_path = path;

                for (const auto& key : keystore.get_keys()) {
                    if (key.mf_name == data.mf_name) {
                        current_key = key;

                        break;
                    }
                }

                fix = data.btn << 28 | data.serial;

                update_hop();

                text_mf_name.set(data.mf_name);
                text_serial.set(to_string_hex(data.serial));
                text_fix.set(to_string_hex(fix));

                field_counter.set_value(data.counter, false);
                field_button.set_value(data.btn, false);

                update_payload();

                field_counter.focus();
            });
        };
    };
}

void KeeloqTXView::start_tx() {
    size_t bitstream_length = encoders::make_bitstream(encoded_data);

    progressbar.set_max(repeat);
    tx_view.set_transmitting(true);

    transmitter_model.enable();

    baseband::set_ook_data(
        bitstream_length,
        OOK_SAMPLERATE * (400.0 / 1000000.0),
        repeat,
        pause_duration);
}

void KeeloqTXView::stop_tx() {
    transmitter_model.disable();
    text_status.set("Ready");
    progressbar.set_value(0);
    tx_view.set_transmitting(false);

    data.counter += 1;

    field_counter.set_value(data.counter, false);

    update_hop();
    update_payload();

    write_keeloq_file(file_path, data);
}

void KeeloqTXView::on_tx_progress(uint32_t progress, bool done) {
    if (!done) {
        text_status.set(to_string_dec_uint(progress + 1) + "/" + to_string_dec_uint(repeat));
        progressbar.set_value(progress + 1);
    } else {
        chThdSleepMilliseconds(10);

        stop_tx();
    }
}

KeeloqTXView::~KeeloqTXView() {
    transmitter_model.disable();
    baseband::shutdown();
}

}  // namespace ui::external_app::ui_keeloqtx
