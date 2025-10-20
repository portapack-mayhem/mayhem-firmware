
#include "ui_adult_toys_controller.hpp"

#include "ui_modemsetup.hpp"
#include "modems.hpp"
#include "audio.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"
#include "ui_navigation.hpp"

using namespace portapack;
using namespace modems;
using namespace ui;

namespace ui::external_app::adult_toys_controller {

void ADULT_toys::focus() {
    button_on.focus();
}

ADULT_toys::ADULT_toys(NavigationView& nav)
    : nav_{nav} {
    add_children({
        &button_on,
        &btn_adult,
        &btn_child,
        &Left_arrow,
        &Right_arrow,
        &Plus,
        &Minus,
        &Random_mode,
        &Play_mode,
        &Stop_mode,
        &N_message,
        &Inf_message,
        &field_frequency,
        &txt_last,
        &message_num,
        &tx_view,
        &screen_message_1,
        &screen_message_2,
        &screen_message_3,
        &screen_message_4,
        &screen_message_5,
        &screen_message_6,
        &screen_message_7,
        &screen_message_8,
        &screen_message_9,
        &screen_message_10,
    });

    btn_adult.on_select = [this](Button&) {
        start_screen = true;
        hidden_program(!start_screen);
    };

    btn_child.on_select = [this](Button&) {
        nav_.pop();
    };

    button_on.on_select = [this](Button&) {
        if (play_running || stop_running) {
            if (button_onstate) {
                button_onstate = false;
                stop();
                button_on.set_text(LanguageHelper::currentMessages[LANG_START]);
            } else {
                button_onstate = true;
                printCurrentModes();
                start();
                button_on.set_text(LanguageHelper::currentMessages[LANG_STOP]);
            }
        }  // if play_running || stop_running
    };

    Left_arrow.on_select = [this](Button&) {
        if (toy_packet > 0) {
            toy_packet--;
            printCurrentModes();
        }
    };

    Right_arrow.on_select = [this](Button&) {
        if (stop_running || play_running) {
            if (play_running && (toy_packet < max_plays - 1)) {
                toy_packet++;
            } else {
                if (stop_running && toy_packet < max_stops - 1) {
                    toy_packet++;
                }
            }
            printCurrentModes();
        }
    };

    Plus.on_select = [this](Button&) {
        if (n_message_running && !button_onstate) {
            if (n_message_set < n_message_max) {
                n_message_set++;
            } else {
                n_message_set = 0;
            }
            message_num.set_value(n_message_set);
            n_message_current = n_message_set;
        }
    };

    Minus.on_select = [this](Button&) {
        if (n_message_running && !button_onstate) {
            if (n_message_set > 0) {
                n_message_set--;
            } else {
                n_message_set = n_message_max;
            }
            message_num.set_value(n_message_set);
            n_message_current = n_message_set;
        }
    };

    message_num.on_change = [this](int32_t v) {
        if (n_message_running && !button_onstate) {
            n_message_set = static_cast<uint16_t>(v);
            n_message_current = n_message_set;
        } else {
            if (!button_onstate) message_num.set_value(n_message_current);
        }
    };

    Random_mode.on_select = [this](Checkbox&, bool v) {
        random_running = v;
    };

    Play_mode.on_select = [this](Checkbox&, bool v) {
        play_running = v;
        if (play_running && stop_running) {
            toy_packet = 0;
            stop_running = false;
            Stop_mode.set_value(stop_running);
        }
        if (play_running) printCurrentModes();
    };

    Stop_mode.on_select = [this](Checkbox&, bool v) {
        stop_running = v;
        if (play_running && stop_running) {
            toy_packet = 0;
            play_running = false;
            Play_mode.set_value(play_running);
        }
        if (stop_running) printCurrentModes();
    };

    N_message.on_select = [this](Checkbox&, bool v) {
        n_message_running = v;
        if (n_message_running && Inf_message_running) {
            Inf_message_running = false;
            Inf_message.set_value(Inf_message_running);
        }
    };

    Inf_message.on_select = [this](Checkbox&, bool v) {
        Inf_message_running = v;
        if (n_message_running && Inf_message_running) {
            n_message_running = false;
            N_message.set_value(n_message_running);
        }
    };
    Inf_message.set_value(true);
    Play_mode.set_value(true);
}

void ADULT_toys::hidden_program(bool hide) {
    button_on.hidden(hide);
    Left_arrow.hidden(hide);
    Right_arrow.hidden(hide);
    Plus.hidden(hide);
    Minus.hidden(hide);
    Random_mode.hidden(hide);
    Play_mode.hidden(hide);
    Stop_mode.hidden(hide);
    N_message.hidden(hide);
    Inf_message.hidden(hide);
    field_frequency.hidden(hide);
    txt_last.hidden(hide);
    message_num.hidden(hide);
    tx_view.hidden(hide);
    btn_adult.hidden(!hide);
    btn_child.hidden(!hide);
    screen_message_1.hidden(!hide);
    screen_message_2.hidden(!hide);
    screen_message_3.hidden(!hide);
    screen_message_4.hidden(!hide);
    screen_message_5.hidden(!hide);
    screen_message_6.hidden(!hide);
    screen_message_7.hidden(!hide);
    screen_message_8.hidden(!hide);
    screen_message_9.hidden(!hide);
    screen_message_10.hidden(!hide);
    set_dirty();
    if (hide)
        btn_child.focus();
    else
        button_on.focus();
}

void ADULT_toys::on_show() {
    hidden_program(!start_screen);
}

void ADULT_toys::createPacket(uint32_t mode) {
    const uint8_t size = 22;
    uint8_t packet[size];
    uint8_t i = 0;

    // 1. AD Flags
    packet[i++] = 2;
    packet[i++] = 0x01;
    packet[i++] = 0x1A;

    // 2. Manufacturer Specific
    packet[i++] = 14;
    packet[i++] = 0xFF;
    packet[i++] = 0xFF;
    packet[i++] = 0x00;
    packet[i++] = 0x6D;
    packet[i++] = 0xB6;
    packet[i++] = 0x43;
    packet[i++] = 0xCE;
    packet[i++] = 0x97;
    packet[i++] = 0xFE;
    packet[i++] = 0x42;
    packet[i++] = 0x7C;
    packet[i++] = (mode >> 0x10) & 0xFF;
    packet[i++] = (mode >> 0x8) & 0xFF;
    packet[i++] = (mode >> 0x0) & 0xFF;

    // 3. Service UUID List
    packet[i++] = 3;
    packet[i++] = 0x03;
    packet[i++] = 0x8F;
    packet[i++] = 0xAE;

    std::string res = to_string_hex_array(packet, i);

    memset(advertisementData, 0, sizeof(advertisementData));
    std::copy(res.begin(), res.end(), advertisementData);
}

ADULT_toys::~ADULT_toys() {
    stop();
}

void ADULT_toys::stop() {
    transmitter_model.disable();
    baseband::shutdown();
}

void ADULT_toys::start() {
    baseband::run_image(portapack::spi_flash::image_tag_btle_tx);
    transmitter_model.enable();
    on_tx_progress(true);
}

void ADULT_toys::reset() {
    stop();
    start();
}

uint8_t ADULT_toys::randomize(uint8_t max) {
    return static_cast<uint8_t>(rand() % max);
}

void ADULT_toys::printCurrentModes() {
    uint8_t max_val = play_running ? max_plays : max_stops;
    std::string* arr = play_running ? play_names : stop_names;
    std::string current = to_string_dec_uint(toy_packet + 1);
    std::string max_val_str = to_string_dec_uint(max_val);

    std::string result_str = current;
    result_str += "/";
    result_str += max_val_str;
    result_str += " ";
    result_str += arr[toy_packet];
    txt_last.set(result_str.c_str());
}

void ADULT_toys::on_tx_progress(const bool done) {
    if (!done) return;
    if (!button_onstate) return;

    if (random_running) {
        toy_packet = randomize(play_running ? max_plays : max_stops);
    }
    createPacket(play_running ? play_values[toy_packet] : stop_values[toy_packet]);
    printCurrentModes();

    if (n_message_running) {
        if (n_message_current > 0) {
            n_message_current--;
            message_num.set_value(n_message_current);
        } else {
            stop();
            message_num.set_value(n_message_set);
            button_on.set_text(LanguageHelper::currentMessages[LANG_START]);
            button_onstate = false;
            n_message_current = n_message_set;
            return;
        }
    }  // if n_message

    baseband::set_btletx(channel_number, mac, advertisementData, pduType);
}

}  // namespace ui::external_app::adult_toys_controller
