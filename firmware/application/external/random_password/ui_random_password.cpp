/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
 * copyleft zxkmm
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

#include "ui_random_password.hpp"
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

namespace ui::external_app::random_password {

void RandomPasswordLogger::log_raw_data(const std::string& data) {
    log_file.write_entry(data);
}

void RandomPasswordView::focus() {
    field_digits.focus();
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
                  &button_pause,
                  &button_send,
                  &field_digits,
                  &check_allow_confusable_chars,
                  &text_seed});

    // no idea what's these, i copied from afsk rx app and they seems needed'
    auto def_bell202 = &modem_defs[0];
    persistent_memory::set_modem_baudrate(def_bell202->baudrate);
    serial_format_t serial_format;
    serial_format.data_bits = 7;
    serial_format.parity = EVEN;
    serial_format.stop_bits = 1;
    serial_format.bit_order = LSB_FIRST;
    persistent_memory::set_serial_format(serial_format);

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

    button_pause.on_select = [this](Button&) {
        if (paused) {
            paused = false;
            button_pause.set_text("pause");
        } else {
            paused = true;
            button_pause.set_text("resume");
        }
    };
    button_send.on_select = [this, &nav](Button&) {
        portapack::async_tx_enabled = true;
        UsbSerialAsyncmsg::asyncmsg(password);
        portapack::async_tx_enabled = false;
    };

    field_digits.on_change = [this](int32_t) {
        this->new_password();
    };

    /// v check defauly val init
    check_digits.set_value(true);
    check_latin_lower.set_value(true);
    check_latin_upper.set_value(true);
    check_punctuation.set_value(true);
    check_show_seeds.set_value(true);
    field_digits.set_value(8);
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
    if (paused)
        return;
    if (is_data) {
        seed = static_cast<unsigned int>(value);
        text_seed.set(to_string_dec_uint(check_show_seeds.value() ? seed : 0));
    } else {
        text_generated_passwd.set("Baudrate estimation: ~");
        text_char_type_hints.set(to_string_dec_uint(value));
    }
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

void RandomPasswordView::new_password() {
    password = "";
    std::string charset = "";
    std::string char_type_hints = "";

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

        if (std::isdigit(c)) {
            char_type_hints += "1";
        } else if (std::islower(c)) {
            char_type_hints += "a";
        } else if (std::isupper(c)) {
            char_type_hints += "A";
        } else {
            char_type_hints += ",";
        }
    }

    text_generated_passwd.set(password);
    text_char_type_hints.set(char_type_hints);

    paint_password_hints();  // TODO: why flash and disappeared

    if (logger && logging) {
        str_log += generate_log_line();
        logger->log_raw_data(str_log);
        str_log = "";
    }

    if (check_auto_send.value()) {
        portapack::async_tx_enabled = true;
        UsbSerialAsyncmsg::asyncmsg(password);
        portapack::async_tx_enabled = false;
    }
}

// TODO: why flash and disappeared
// tried:
// 1. paint inline in new_password func
// 2. paint in a seperate func and call from new_password
// 3. override nav's paint func (i think it can tried to capture same obj) and paint, hoping set_dirty handle it correctly
// 4. override nav's paint func (i think it can tried to capture same obj) and paint in a seperate func, hoping set_dirty handle it correctly
// all these methods failed, and all of them only flash and disappeared. only when set_dirty in on_data (which seems incorrect), and it keep flashing never stop. but see painted content (flashing too)
// btw this is not caused by the seed text set thing, cuz commented it out not helping.
void RandomPasswordView::paint_password_hints() {
    Painter painter;
    const int char_width = 8;
    const int char_height = 16;
    const int start_y = 6 * char_height + 5;
    const int rect_height = 4;

    for (size_t i = 0; i < password.length(); i++) {
        char c = password[i];
        Color color;
        if (std::isdigit(c)) {
            color = Color::red();
        } else if (std::islower(c)) {
            color = Color::green();
        } else if (std::isupper(c)) {
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
    std::string line = "\npassword=" + password +
                       "\nseed=" + std::to_string(seed) +
                       "\n";
    return line;
}

RandomPasswordView::~RandomPasswordView() {
    receiver_model.disable();
    baseband::shutdown();
}

}  // namespace ui::external_app::random_password
