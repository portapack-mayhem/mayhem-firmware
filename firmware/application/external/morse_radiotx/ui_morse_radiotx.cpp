#include "ui_morse_radiotx.hpp"

using namespace portapack;

namespace ui::external_app::morse_radiotx {

MorseRadiotxView::MorseRadiotxView(ui::NavigationView& nav)
    : current_timings(calculate_morse_timings(20)),
      nav_(nav) {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());
    add_children({&field_frequency,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_volume,
                  &labels,
                  &options_mode,
                  &tone_,
                  &wpm_,
                  &txt_msg,
                  &btn_message,
                  &btn_calls,
                  &chk_trans,
                  &chk_callsgn,
                  &txt_last,
                  &console_text,
                  &btn_clear,
                  &btn_tt});
    audio::set_rate(audio::Rate::Hz_24000);

    options_mode.on_change = [this](size_t, int32_t value) {
        current_mode = (uint8_t)value;
        if (current_mode != 0)
            tone_.set_style(Theme::getInstance()->option_active);
        else {
            tone_.set_style(Theme::getInstance()->fg_dark);
            tone_.set_focusable(false);
        }
        baseband::set_morsetx_config(current_mode, tone, 0, false);
    };

    tone_.on_change = [this](int32_t v) {
        tone = static_cast<uint32_t>(v * 1398);
        baseband::set_morsetx_config(current_mode, tone, 0, false);
    };

    wpm_.on_change = [this](int16_t v) {
        wpm = v;
        current_timings = calculate_morse_timings(wpm);
    };

    options_mode.set_selected_index(current_mode, true);
    tone_.set_value(tone, true);
    wpm_.set_value(wpm, true);

    btn_tt.on_select = [this](Button&) {
        if (!chk_trans.value())
            transmit_morse_message();
        else {
            if (button_touch) {
                button_touch = false;
                return;
            }
            button_was_selected = true;
            onPress();
        }
    };
    btn_tt.on_touch_press = [this](Button&) {
        button_touch = true;
        button_was_selected = false;
        onPress();
    };
    btn_tt.on_touch_release = [this](Button&) {
        button_touch = true;
        button_was_selected = false;
        onRelease();
    };
    btn_clear.on_select = [this](Button&) {
        console_text.clear(true);
        txt_last.set("");
    };

    btn_message.on_select = [this, &nav](Button&) {
        this->on_set_text(nav);
    };

    btn_calls.on_select = [this, &nav](Button&) {
        this->on_set_call(nav);
    };

    audio::output::start();
    auto vol = field_volume.value();
    field_volume.set_value(0);
    field_volume.set_value(vol);

    chk_trans.on_select = [this](Checkbox&, bool v) {
        if (v)
            btn_tt.set_text("KEY");
        else
            btn_tt.set_text("Msg.");
    };
}

MorseRadiotxView::~MorseRadiotxView() {
    transmitter_model.disable();
    baseband::shutdown();
    audio::output::stop();
}

void MorseRadiotxView::on_show() {
    start_time = 0;
    end_time = 0;
    transmit_time = 0;
}

void MorseRadiotxView::focus() {
    btn_tt.focus();
}

void MorseRadiotxView::paint(Painter&) {
    txt_msg.set("[" + msg_buffer + "] ");
    btn_calls.set_text(call_sign);
}

void MorseRadiotxView::on_set_text(NavigationView& nav) {
    text_prompt(nav, msg_buffer, 27, ENTER_KEYBOARD_MODE_ALPHA);
}

void MorseRadiotxView::on_set_call(NavigationView& nav) {
    text_prompt(nav, call_sign, 10, ENTER_KEYBOARD_MODE_ALPHA);
}

MorseRadiotxView::MorseTimings MorseRadiotxView::calculate_morse_timings(uint32_t wpm) {
    MorseRadiotxView::MorseTimings t;

    if (wpm == 0) wpm = 1;

    t.dot_ms = 1200 / wpm;

    t.dash_ms = 3 * t.dot_ms;
    t.symbol_gap = t.dot_ms;
    t.char_gap = 3 * t.dot_ms;
    t.word_gap = 7 * t.dot_ms;

    return t;
}

void MorseRadiotxView::transmit_morse_message() {
    std::string full_message = msg_buffer;

    // cal sign
    if (chk_callsgn.value() && !call_sign.empty()) {
        full_message = call_sign + " " + full_message;
    }

    // enable transmit
    if (!transmit) {
        transmit = true;
        transmitter_model.enable();
    }

    for (size_t i = 0; i < full_message.length(); i++) {
        char c = full_message[i];

        std::string s_char(1, c);

        // space
        if (c == ' ') {
            console_text.write(" ");
            // space pause
            chThdSleepMilliseconds(current_timings.word_gap);
            continue;
        }

        const char* pattern = morse_decoder_.get_morse_pattern(c);

        if (pattern != nullptr) {
            txt_last.set(pattern);

            // write blue char to console
            console_text.write(STR_COLOR_BLUE + s_char);

            // Morze (dih/dah) send
            for (int j = 0; pattern[j] != '\0'; j++) {
                baseband::set_morsetx_key(true);

                if (pattern[j] == '.') {
                    chThdSleepMilliseconds(current_timings.dot_ms);
                } else if (pattern[j] == '-') {
                    chThdSleepMilliseconds(current_timings.dash_ms);
                }

                // pause between signs
                baseband::set_morsetx_key(false);
                chThdSleepMilliseconds(current_timings.symbol_gap);
            }

            if (current_timings.char_gap > current_timings.symbol_gap) {
                chThdSleepMilliseconds(current_timings.char_gap - current_timings.symbol_gap);
            }
        }
    }

    baseband::set_morsetx_key(false);

    transmit_time = chTimeNow();
    decode_timeout_calc = false;
}

void MorseRadiotxView::onPress() {
    start_time = chTimeNow();
    if (!transmit) {
        transmit = true;
        transmitter_model.enable();
    }
    baseband::set_morsetx_key(true);
    if (end_time != 0) {
        int64_t gap_delta = (chTimeNow() - end_time);
        auto result = morse_decoder_.handleInput(-gap_delta);
        if (result.isValid()) {
            writeCharToConsole(result.text, result.confidence);
        }
    }
    end_time = 0;
    transmit_time = 0;
    decode_timeout_calc = false;
    baseband::request_audio_beep(1000, 24000, 2000);
}

void MorseRadiotxView::onRelease() {
    end_time = chTimeNow();
    transmit_time = end_time;
    baseband::set_morsetx_key(false);
    if (start_time != 0) {
        int32_t press_delta = (end_time - start_time);
        auto result = morse_decoder_.handleInput(press_delta);
        if (result.isValid()) {
            writeCharToConsole(result.text, result.confidence);
        }
    }
    start_time = 0;
    decode_timeout_calc = true;
    baseband::request_beep_stop();
}

void MorseRadiotxView::writeCharToConsole(const std::string& ch, double confidence) {
    if (ch.empty()) {
        return;
    }

    txt_last.set(morse_decoder_.getLastSequence().c_str());

    last_color_id = color_id;
    std::string color = "";

    if (ch == " ") {
        color_id = 0;
    } else if (ch[0] == '{') {  // no match
        color_id = 0;
    } else {
        if (confidence < 0.8)
            color_id = 1;
        else if (confidence < 0.9)
            color_id = 2;
        else
            color_id = 3;
    }
    color = arr_color[color_id];
    last_color_id = color_id;
    console_text.write(color + ch);
}

inline bool MorseRadiotxView::tx_button_held() {
    const auto switches_state = get_switches_state();
    return switches_state[(size_t)ui::KeyEvent::Select];
}

void MorseRadiotxView::on_framesync() {
    if (button_was_selected && !button_touch && !tx_button_held()) {
        button_was_selected = false;
        onRelease();
    }

    if (end_time != 0 && decode_timeout_calc) {
        int64_t gap_delta = (chTimeNow() - end_time);

        if (gap_delta >= morse_decoder_.getInterCharThreshold()) {
            auto result = morse_decoder_.handleInput(-(int32_t)gap_delta);
            if (result.isValid()) {
                writeCharToConsole(result.text, result.confidence);
            }
        }
        if (gap_delta >= morse_decoder_.getInterWordThreshold()) {
            writeCharToConsole(" ", 1.0);
            end_time = 0;
            decode_timeout_calc = false;
        }
    }
    if (transmit_time != 0 && transmit) {
        int64_t gap_delta = (chTimeNow() - transmit_time);
        if (gap_delta >= (morse_decoder_.getInterWordThreshold() * 3)) {
            transmit = false;
            transmitter_model.disable();
            transmit_time = 0;
        }
    }
}

}  // namespace ui::external_app::morse_radiotx
