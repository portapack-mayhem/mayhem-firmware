/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#include "ui_random.hpp"
#include "ui_modemsetup.hpp"

#include "modems.hpp"
#include "rtc_time.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"
#include "file_path.hpp"

using namespace portapack;
using namespace modems;
using namespace ui;

namespace ui::external_app::random {

void RandomLogger::log_raw_data(const std::string& data) {
    log_file.write_entry(data);
}

void RandomView::focus() {
    field_digits.focus();
}

RandomView::RandomView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&rssi,
                  &channel,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_frequency,
                  &check_log,
                  &text_debug,
                  &button_modem_setup,
                  &console,
                  &labels,
                  &text_generated_passwd,
                  &text_char_type_hints,
                  &check_digits,
                  &check_latin_lower,
                  &check_latin_upper,
                  &check_punctuation,
                  &button_refresh,
                  &button_show_qr,
                  &field_digits,
                  &check_allow_confusable_chars,
                  &text_seed});

    // Auto-configure modem for LCR RX (TODO remove)
    // field_frequency.set_value(467225500);
    auto def_bell202 = &modem_defs[0];
    persistent_memory::set_modem_baudrate(def_bell202->baudrate);
    serial_format_t serial_format;
    serial_format.data_bits = 7;
    serial_format.parity = EVEN;
    serial_format.stop_bits = 1;
    serial_format.bit_order = LSB_FIRST;
    persistent_memory::set_serial_format(serial_format);

    field_frequency.set_step(100);

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

    button_modem_setup.on_select = [&nav](Button&) {
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
        // std::string qr = text_generated_passwd.
        nav.push<QRCodeView>(password.data());
    };

    field_digits.on_change = [this](int32_t) {
        this->new_password();
    };

    /// v init setting
    check_digits.set_value(true);
    check_latin_lower.set_value(true);
    field_digits.set_value(8);
    ///^ init setting

    logger = std::make_unique<RandomLogger>();
    if (logger)
        logger->append(logs_dir / u"random.TXT");

    // Auto-configure modem for LCR RX (will be removed later)
    baseband::set_afsk(persistent_memory::modem_baudrate(), 8, 0, false);

    receiver_model.enable();
    set_random_freq();
    new_password();
}

void RandomView::on_data(uint32_t value, bool is_data) {
    std::string str_console = "\x1B";
    std::string str_byte = "";

    if (is_data) {
        seed = static_cast<unsigned int>(value);
        text_seed.set( to_string_dec_uint(seed));

        // Colorize differently after message splits
        // str_console += (char)((console_color & 3) + 9);

        // Directly print the received value without decoding
        str_console += "[" + to_string_hex(value, 2) + "]";
        str_byte += "[" + to_string_hex(value, 2) + "]";

        // console.write(str_console);

        // if (logger && logging) str_log += str_byte;

        if ((value != 0x7F) && (prev_value == 0x7F)) {
            // Message split
            // console.writeln("");
            // console_color++;
        }
        prev_value = value;
    } else {
        // Baudrate estimation
        text_debug.set("Baudrate estimation: ~" + to_string_dec_uint(value));
    }
}

void RandomView::on_freqchg(int64_t freq) {
    field_frequency.set_value(freq);
}

void RandomView::set_random_freq() {
    std::srand(LPC_RTC->CTIME0);
    // this is only for seed to visit random freq, the radio is still real random

    auto random_freq = 100000000 + (std::rand() % 900000000);  // 100mhz to 1ghz
    receiver_model.set_target_frequency(random_freq);
    field_frequency.set_value(random_freq);
}

void RandomView::new_password() {
    password = "";
    std::string charset;
    std::string char_type_hints;

    if (check_digits.value())
        charset += "0123456789";
    if (check_latin_lower.value())
        charset += "abcdefghijklmnopqrstuvwxyz";
    if (check_latin_upper.value())
        charset += "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if (check_punctuation.value())
        charset += ".,-!?";

    if (!check_allow_confusable_chars.value()) {
        charset.erase(std::remove_if(charset.begin(), charset.end(),
                                     [](char c) { return c == '0' || c == 'O' || c == 'o' || c == '1' || c == 'l'; }),
                      charset.end());
    }

    if (charset.empty()) {
        text_generated_passwd.set("generate failed,");
        text_char_type_hints.set("select at least 1 type");

        return;
    }

    if (seed == 0) {
        text_generated_passwd.set("generate failed,");
        text_char_type_hints.set("random seed exception");
    } else {
        std::srand(seed);  // extern void srand (unsigned int __seed) __THROW;
    }

    int password_length = field_digits.value();
    for (int i = 0; i < password_length; i++) {
        char c = charset[std::rand() % charset.length()];
        password += c;

        if (std::isdigit(c))
            char_type_hints += "1";
        else if (std::islower(c))
            char_type_hints += "a";
        else if (std::isupper(c))
            char_type_hints += "A";
        else
            char_type_hints += ",";
    }

    text_generated_passwd.set(password);
    text_char_type_hints.set(char_type_hints);

    if (logger && logging) {
        str_log += generate_log_line();
        logger->log_raw_data(str_log);
        str_log = "";
    }

    // portapack::async_tx_enabled = true;
    // UsbSerialAsyncmsg::asyncmsg(password);
    // portapack::async_tx_enabled = false;
}

std::string RandomView::generate_log_line() {
    std::string line = "\npassword=" + password +
                       "\nseed=" + std::to_string(seed) +
                       "\n";
    return line;
}

bool RandomView::seed_protect_helper() {}

RandomView::~RandomView() {
    receiver_model.disable();
    baseband::shutdown();
}

}  // namespace ui::external_app::random
