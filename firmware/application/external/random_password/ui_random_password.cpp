/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
 * Copyright (C) 2024 zxkmm
 * Copyright (C) 2024 HTotoo
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

/* Notes about safety/randomness:
 * - Radio data is from nature, so if it's evenly tiled in space, it's random enough, but usually it's not
 * - A unsafe point is the freq that fetch data is from LCG (from Cpp STL) using time as seed, but should be spread out by later random calculation
 * - A unsafe point is the init data is still using LCG to generate, but used following methods to spread out:
 * - - Radio data
 * - - save a buffer of seeds and generate char by char, this cover all the possible chars combination
 * - - Rollout with teo seeds and get a final char, this spread out again
 * - - Hash out the generated password and spread out again if you choose. SHA-512 is proven very spreading out in random space by math
 * - But still, this isn't meant to become a TRNG generator, don't use at critical/high safety requited/important system, this FOSS has absolutely no warranty
 */

#include "ui_random_password.hpp"
#include "ui_modemsetup.hpp"

#include "modems.hpp"
#include "rtc_time.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"
#include "file_path.hpp"
#include "sha512.h"

using namespace portapack;
using namespace modems;
using namespace ui;

namespace ui::external_app::random_password {

void RandomPasswordLogger::log_raw_data(const std::string& data) {
    log_file.write_entry(data);
}

void RandomPasswordView::focus() {
    button_refresh.focus();
}

RandomPasswordView::RandomPasswordView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&rssi,
                  &channel,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_frequency,
                  &check_log,
                  &button_modem_setup,
                  &labels,
                  &text_generated_passwd,
                  &text_char_type_hints,
                  &check_digits,
                  &check_latin_lower,
                  &check_latin_upper,
                  &check_punctuation,
                  &check_show_seeds,
                  &check_auto_send,
                  &button_refresh,
                  &button_show_qr,
                  &button_flood,
                  &button_send,
                  &field_digits,
                  &field_method,
                  &check_allow_confusable_chars,
                  &text_seed,
                  &progressbar});

    // no idea what's these, i copied from afsk rx app and they seems needed'
    auto def_bell202 = &modem_defs[0];
    persistent_memory::set_modem_baudrate(def_bell202->baudrate);
    serial_format_t serial_format;
    serial_format.data_bits = 7;
    serial_format.parity = EVEN;
    serial_format.stop_bits = 1;
    serial_format.bit_order = LSB_FIRST;
    persistent_memory::set_serial_format(serial_format);

    progressbar.set_max(MAX_DIGITS * 2);

    check_log.set_value(logging);
    check_log.on_select = [this](Checkbox&, bool v) {
        if (v) {
            nav_.display_modal(
                "Warning",
                "Sure?\n"
                "this will save all generated\n"
                "password to sdcard\n"
                "in plain text\n"
                "those which generated before\n"
                "you check me, will lost",
                YESNO,
                [this, v](bool c) {
                    if (c) {
                        logging = v;
                    } else {
                        check_log.set_value(false);
                        // this is needed to check back to false cuz when trigger by human, the check to true already happened
                        // this blocked interface so won't accidently saved even if user checked but selected no later here,
                        // but take care of here if in the future implemented ticking/auto/batch save etc
                    }
                });
        } else {
            logging = v;
        }
    };

    button_modem_setup.on_select = [&nav](Button&) {  // copied from afsk rx app
        nav.push<ModemSetupView>();
    };

    check_digits.on_select = [this](Checkbox&, bool) {
        this->new_password();
    };

    check_latin_lower.on_select = [this](Checkbox&, bool) {
        this->new_password();
    };

    check_latin_upper.on_select = [this](Checkbox&, bool) {
        this->new_password();
    };

    check_punctuation.on_select = [this](Checkbox&, bool) {
        this->new_password();
    };

    check_allow_confusable_chars.on_select = [this](Checkbox&, bool) {
        this->new_password();
    };

    button_refresh.on_select = [this](Button&) {
        this->set_random_freq();
        this->new_password();
    };

    button_show_qr.on_select = [this, &nav](Button&) {
        nav.push<QRCodeView>(password.data());
    };

    button_flood.on_select = [this](Button&) {
        if (flooding) {
            flooding = false;
            button_flood.set_text(LanguageHelper::currentMessages[LANG_FLOOD]);
        } else {
            flooding = true;
            button_flood.set_text(LanguageHelper::currentMessages[LANG_STOP]);
        }
    };
    button_send.on_select = [this, &nav](Button&) {
        async_prev_val = portapack::async_tx_enabled;
        portapack::async_tx_enabled = true;
        UsbSerialAsyncmsg::asyncmsg(password);
        portapack::async_tx_enabled = async_prev_val;
    };

    field_digits.on_change = [this](int32_t) {
        clean_buffer();
        this->new_password();
    };

    /// v check defauly val init
    check_digits.set_value(true);
    check_latin_lower.set_value(true);
    check_latin_upper.set_value(true);
    check_punctuation.set_value(true);
    check_show_seeds.set_value(true);
    field_digits.set_value(16);
    field_method.set_by_value(Method::RADIO_LCG_ROLL_HASH);
    ///^ check defauly val init

    logger = std::make_unique<RandomPasswordLogger>();
    if (logger)
        logger->append(logs_dir / u"random.TXT");

    // Auto-configure modem for LCR RX (will be removed later), copied from afsk rx app
    baseband::set_afsk(persistent_memory::modem_baudrate(), 8, 0, false);

    receiver_model.enable();
    receiver_model.set_rf_amp(false);
    set_random_freq();
    new_password();
}

void RandomPasswordView::on_data(uint32_t value, bool is_data) {
    if (is_data) {
        seed = static_cast<unsigned int>(value);
        text_seed.set(to_string_dec_uint(check_show_seeds.value() ? seed : 0));

        /// v feed deque
        seeds_deque.push_back(value);
        if (seeds_deque.size() > MAX_DIGITS * 2) {
            seeds_deque.pop_front();
        }

        ///^ feed deque

        progressbar.set_value(seeds_deque.size());

        if (flooding && seeds_deque.size() >= MAX_DIGITS * 2) {
            new_password();
        }

    } else {
        text_generated_passwd.set("Baudrate estimation: ~");
        text_char_type_hints.set(to_string_dec_uint(value));
    }
}

void RandomPasswordView::clean_buffer() {
    seeds_deque.clear();
}

void RandomPasswordView::on_freqchg(int64_t freq) {
    field_frequency.set_value(freq);
}

void RandomPasswordView::set_random_freq() {
    std::srand(LPC_RTC->CTIME0);
    // this is only for seed to visit random freq, the radio is still real random

    auto random_freq = 100000000 + (std::rand() % 900000000);  // 100mhz to 1ghz
    receiver_model.set_target_frequency(random_freq);
    field_frequency.set_value(random_freq);
}

bool RandomPasswordView::islower(char c) {
    return (c >= 'a' && c <= 'z');
}

bool RandomPasswordView::isupper(char c) {
    return (c >= 'A' && c <= 'Z');
}

bool RandomPasswordView::isdigit(char c) {
    return (c >= '0' && c <= '9');
}

void RandomPasswordView::new_password() {
    if (seeds_deque.size() < MAX_DIGITS * 2) {
        seeds_buffer_not_full = true;
        text_generated_passwd.set("wait seeds buffer full");
        text_char_type_hints.set("then press generate");
        return;
    }
    password = "";
    std::string charset = "";
    std::string char_type_hints = "";
    std::string initial_password = "";
    int password_length = field_digits.value();

    /// charset worker
    if (check_digits.value()) {
        charset += "23456789";
        if (check_allow_confusable_chars.value()) charset += "01";
    }
    if (check_latin_lower.value()) {
        charset += "abcdefghijkmnpqrstuvwxyz";
        if (check_allow_confusable_chars.value()) charset += "ol";
    }
    if (check_latin_upper.value()) {
        charset += "ABCDEFGHIJKLMNPQRSTUVWXYZ";
        if (check_allow_confusable_chars.value()) charset += "O";
    }
    if (check_punctuation.value())
        charset += ".,-!?";

    if (charset.empty()) {
        text_generated_passwd.set("generate failed,");
        text_char_type_hints.set("select at least 1 type");
        return;
    }

    /// roll worker
    for (int i = 0; i < password_length * 2; i += 2) {
        unsigned int seed = seeds_deque[i];
        std::srand(seed);
        uint8_t rollnum = (uint8_t)(seeds_deque[i + 1] % 128);
        uint8_t nu = 0;
        for (uint8_t o = 0; o < rollnum; ++o) nu = std::rand();
        nu++;
        char c = charset[std::rand() % charset.length()];
        initial_password += c;
    }

    // hash worker
    std::string hashed_password = sha512(initial_password);

    std::vector<char> password_chars(password_length);
    for (int i = 0; i < password_length; i++) {
        unsigned int index = std::stoul(hashed_password.substr(i * 2, 2), nullptr, 16) % charset.length();  // result from 0 to charset.length()-1,should be very evenly tiled in the charset space
        password_chars[i] = charset[index];
    }

    /// hint text worker
    for (char c : password_chars) {
        password += c;
        if (isdigit(c)) {
            char_type_hints += "1";
        } else if (islower(c)) {
            char_type_hints += "a";
        } else if (isupper(c)) {
            char_type_hints += "A";
        } else {
            char_type_hints += ",";
        }
    }

    /// decision worker
    switch (field_method.selected_index_value()) {
        case Method::RADIO_LCG_ROLL:
            password = initial_password;
            break;
        case Method::RADIO_LCG_ROLL_HASH:
            break;
        default:
            break;
    }

    /// give out result worker
    text_generated_passwd.set(password);
    text_char_type_hints.set(char_type_hints);

    paint_password_hints();

    if (logger && logging) {
        str_log += generate_log_line();
        logger->log_raw_data(str_log);
        str_log = "";
    }

    if (check_auto_send.value() || flooding) {
        async_prev_val = portapack::async_tx_enabled;
        portapack::async_tx_enabled = true;
        UsbSerialAsyncmsg::asyncmsg(password);
        portapack::async_tx_enabled = async_prev_val;
    }

    clean_buffer();
}

void RandomPasswordView::paint_password_hints() {
    Painter painter;
    const int char_width = 8;
    const int char_height = 16;
    const int start_y = 6 * char_height;
    const int rect_height = 4;

    for (size_t i = 0; i < password.length(); i++) {
        char c = password[i];
        Color color;
        if (isdigit(c)) {
            color = Color::red();
        } else if (islower(c)) {
            color = Color::green();
        } else if (isupper(c)) {
            color = Color::blue();
        } else {
            color = Color::white();
        }

        painter.fill_rectangle(
            {{static_cast<int>(i) * char_width, start_y},
             {char_width, rect_height}},
            color);
    }
}

std::string RandomPasswordView::generate_log_line() {
    std::string seeds_set = "";
    for (unsigned int seed : seeds_deque) {
        seeds_set += to_string_dec_uint(seed);
        seeds_set += " ";
    }
    std::string line = "\npassword=" + password +
                       "\nseeds=" + seeds_set +
                       "\n";
    return line;
}

RandomPasswordView::~RandomPasswordView() {
    receiver_model.disable();
    baseband::shutdown();
}

}  // namespace ui::external_app::random_password