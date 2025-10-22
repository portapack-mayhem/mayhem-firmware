
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

void AdultToysView::focus() {
    button_on.focus();
}

AdultToysView::AdultToysView(NavigationView& nav)
    : nav_{nav} {
    add_children({
        &button_on,
        &btn_adult,
        &btn_child,
        &btn_mode_decrease,
        &btn_mode_increase,
        &btn_pcknum_inc,
        &btn_pcknum_dec,
        &chk_mode_rnd,
        &chk_mode_play,
        &chk_mode_stop,
        &chk_mode_ncnt,
        &chk_mode_infinite,
        &field_frequency,
        &txt_selected_mode,
        &options_target,
        &labels,
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

    btn_mode_decrease.on_select = [this](Button&) {
        if (toy_packet > 0) {
            toy_packet--;
            printCurrentModes();
        }
    };

    btn_mode_increase.on_select = [this](Button&) {
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

    btn_pcknum_inc.on_select = [this](Button&) {
        if (!button_onstate) {
            chk_mode_ncnt.set_value(true);
            if (n_message_set < n_message_max) {
                n_message_set++;
            } else {
                n_message_set = 0;
            }
            message_num.set_value(n_message_set);
            n_message_current = n_message_set;
        }
    };

    btn_pcknum_dec.on_select = [this](Button&) {
        if (!button_onstate) {
            chk_mode_ncnt.set_value(true);
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
        if (!button_onstate) {
            chk_mode_ncnt.set_value(true);
            n_message_set = static_cast<uint16_t>(v);
            n_message_current = n_message_set;
        } else {
            if (!button_onstate) message_num.set_value(n_message_current);
        }
    };

    chk_mode_rnd.on_select = [this](Checkbox&, bool v) {
        random_running = v;
    };

    chk_mode_play.on_select = [this](Checkbox&, bool v) {
        play_running = v;
        if (play_running && stop_running) {
            toy_packet = 0;
            stop_running = false;
            chk_mode_stop.set_value(stop_running);
        }
        if (play_running) printCurrentModes();
    };

    chk_mode_stop.on_select = [this](Checkbox&, bool v) {
        stop_running = v;
        if (play_running && stop_running) {
            toy_packet = 0;
            play_running = false;
            chk_mode_play.set_value(play_running);
        }
        if (stop_running) printCurrentModes();
    };

    chk_mode_ncnt.on_select = [this](Checkbox&, bool v) {
        n_message_running = v;
        if (n_message_running && inf_message_running) {
            inf_message_running = false;
            chk_mode_infinite.set_value(inf_message_running);
        }
    };

    chk_mode_infinite.on_select = [this](Checkbox&, bool v) {
        inf_message_running = v;
        if (n_message_running && inf_message_running) {
            n_message_running = false;
            chk_mode_ncnt.set_value(n_message_running);
        }
    };
    options_target.on_change = [this](size_t, int32_t i) {
        target = (uint8_t)i;
    };

    chk_mode_infinite.set_value(true);
    chk_mode_play.set_value(true);
}

void AdultToysView::hidden_program(bool hide) {
    options_target.hidden(hide);
    labels.hidden(hide);
    button_on.hidden(hide);
    btn_mode_decrease.hidden(hide);
    btn_mode_increase.hidden(hide);
    btn_pcknum_inc.hidden(hide);
    btn_pcknum_dec.hidden(hide);
    chk_mode_rnd.hidden(hide);
    chk_mode_play.hidden(hide);
    chk_mode_stop.hidden(hide);
    chk_mode_ncnt.hidden(hide);
    chk_mode_infinite.hidden(hide);
    field_frequency.hidden(hide);
    txt_selected_mode.hidden(hide);
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

void AdultToysView::on_show() {
    hidden_program(!start_screen);
}

void AdultToysView::createPacket(bool regenerate) {
    if (regenerate) randomizeMac();
    randomChn();
    if (target == 1 || 1) {  // now only lovespouse supported, in the future, need to separee them
        uint32_t mode = play_running ? plays[toy_packet].value : stops[toy_packet].value;

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
}

AdultToysView::~AdultToysView() {
    stop();
}

void AdultToysView::stop() {
    transmitter_model.disable();
    baseband::shutdown();
}

void AdultToysView::start() {
    baseband::run_image(portapack::spi_flash::image_tag_btle_tx);
    transmitter_model.enable();
    on_tx_progress(true);
}

void AdultToysView::reset() {
    stop();
    start();
}

uint8_t AdultToysView::randomize(uint8_t max) {
    return static_cast<uint8_t>(rand() % max);
}

void AdultToysView::printCurrentModes() {
    uint8_t max_val = play_running ? max_plays : max_stops;
    auto name = play_running ? plays[toy_packet].name : stops[toy_packet].name;
    std::string current = to_string_dec_uint(toy_packet + 1);
    std::string max_val_str = to_string_dec_uint(max_val);

    std::string result_str = current;
    result_str += "/";
    result_str += max_val_str;
    result_str += " ";
    result_str += name;
    txt_selected_mode.set(result_str.c_str());
}

uint64_t AdultToysView::get_freq_by_channel_number(uint8_t channel_number) {
    uint64_t freq_hz;

    switch (channel_number) {
        case 37:
            freq_hz = 2'402'000'000ull;
            break;
        case 38:
            freq_hz = 2'426'000'000ull;
            break;
        case 39:
            freq_hz = 2'480'000'000ull;
            break;
        case 0 ... 10:
            freq_hz = 2'404'000'000ull + channel_number * 2'000'000ull;
            break;
        case 11 ... 36:
            freq_hz = 2'428'000'000ull + (channel_number - 11) * 2'000'000ull;
            break;
        default:
            freq_hz = UINT64_MAX;
    }

    return freq_hz;
}

void AdultToysView::randomizeMac() {
    const char hexDigits[] = "0123456789ABCDEF";
    // Generate 12 random hexadecimal characters
    for (int i = 0; i < 12; ++i) {
        int randomIndex = rand() % 16;
        mac[i] = hexDigits[randomIndex];
    }
    mac[12] = '\0';  // Null-terminate the string
}

void AdultToysView::randomChn() {
    channel_number = 37 + std::rand() % (39 - 37 + 1);
    field_frequency.set_value(get_freq_by_channel_number(channel_number));
}

void AdultToysView::on_tx_progress(const bool done) {
    if (!done) return;
    if (!button_onstate) return;
    bool regenerate = (counter++ % 600) == 0;
    if (random_running && regenerate) {
        toy_packet = randomize(play_running ? max_plays : max_stops);
    }
    createPacket(regenerate);
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
