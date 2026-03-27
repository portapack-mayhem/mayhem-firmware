#include "ui_morse_radiotx.hpp"

using namespace portapack;

namespace ui::external_app::morseradiotx {

static msg_t tx_thread_fn(void* arg) {
    auto view = reinterpret_cast<MorseRadiotxView*>(arg);
    chRegSetThreadName("morse_tx_thread");
    view->transmit_morse_message();
    return 0;
}

MorseRadiotxView::MorseRadiotxView(ui::NavigationView& nav)
    : current_timings(calculate_morse_timings(20)),
      nav_(nav) {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());
    add_children({&tx_view,
                  &field_volume,
                  &labels,
                  &options_mode,
                  &tone_,
                  &wpm_,
                  &txt_msg,
                  &btn_message,
                  &btn_calls,
                  &chk_trans,
                  &bandwidth,
                  &chk_callsgn,
                  &chk_loop,
                  &wait_time,
                  &progressbar,
                  &txt_last,
                  &console_text,
                  &btn_clear,
                  &btn_ptt});

    initial_switch_config_ = get_switches_repeat_config();

    SwitchesState config = initial_switch_config_;
    config[toUType(Switch::Sel)] = false;
    set_switches_repeat_config(config);

    audio::set_rate(audio::Rate::Hz_24000);

    btn_clear.on_select = [this](Button&) {
        console_text.clear(true);
        txt_last.set("");
    };

    options_mode.on_change = [this](size_t, int32_t value) {
        current_mode = (uint8_t)value;
        baseband::set_morsetx_config(current_mode, tone, band);
        ui_toggle();
    };

    tone_.on_change = [this](int32_t v) {
        tone = static_cast<uint32_t>(v);
        baseband::set_morsetx_config(current_mode, tone, band);
    };

    wpm_.on_change = [this](int16_t v) {
        wpm = v;
        current_timings = calculate_morse_timings(wpm);
    };

    bandwidth.on_change = [this](float v) {
        band = v;
        baseband::set_morsetx_config(current_mode, tone, band);
    };

    options_mode.set_selected_index(current_mode, true);
    tone_.set_value(tone, true);
    wpm_.set_value(wpm, true);
    wait_time.set_value(1, true);
    bandwidth.set_value(band, true);

    btn_ptt.on_select = [this](Button&) {
        if (button_touch) {
            button_touch = false;
            return;
        }
        button_was_selected = true;
        onPress();
    };

    btn_ptt.on_touch_press = [this](Button&) {
        button_touch = true;
        button_was_selected = false;
        onPress();
    };
    btn_ptt.on_touch_release = [this](Button&) {
        if (chk_trans.value() && transmit) {
            button_touch = true;
            button_was_selected = false;
            onRelease();
        }
    };

    btn_message.on_select = [this, &nav](Button&) {
        if (!transmit)
            text_prompt(nav, msg_buffer, 27, ENTER_KEYBOARD_MODE_ALPHA, [this](std::string& buffer) {
                msg_buffer = buffer;
                txt_msg.set("[" + msg_buffer + "] ");
            });
    };

    btn_calls.on_select = [this, &nav](Button&) {
        if (!transmit) {
            text_prompt(nav, call_sign, 10, ENTER_KEYBOARD_MODE_ALPHA, [this](std::string& buffer) {
                call_sign = buffer;
                if (call_sign.empty())
                    btn_calls.set_text("call sign?");
                else
                    btn_calls.set_text(call_sign);
            });
        }
    };

    audio::output::start();
    auto vol = field_volume.value();
    field_volume.set_value(0);
    field_volume.set_value(vol);

    tx_view.on_start = [this]() {
        if (!chk_trans.value() && msg_buffer.empty()) {
            tx_view.set_transmitting(false);
            return;
        }
        tx_view.focus();
        if (!tx_thread && !thread_running) {
            thread_running = true;
            tx_thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO + 5, tx_thread_fn, this);
            tx_view.set_transmitting(true);
        }

        else
            tx_view.set_transmitting(false);
    };

    tx_view.on_stop = [this]() {
        baseband::set_morsetx_key(false);
        transmitter_model.disable();

        if (tx_thread) {
            chThdTerminate(tx_thread);
            chThdWait(tx_thread);
        }
        transmit = false;
        transmit_time = 0;
        tx_thread = nullptr;
        thread_running = false;
        tx_view.set_transmitting(false);
        ui_toggle();
    };

    tx_view.on_edit_frequency = [this, &nav]() {
        auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.target_frequency());
        new_view->on_changed = [this](rf::Frequency f) {
            transmitter_model.set_target_frequency(f);
        };
    };

    chk_trans.on_select = [this](Checkbox&, bool v) {
        if (v) chk_loop.set_value(false);
    };

    chk_loop.on_select = [this](Checkbox&, bool v) {
        if (v) chk_trans.set_value(false);
    };
}

MorseRadiotxView::~MorseRadiotxView() {
    set_switches_repeat_config(initial_switch_config_);
    if (tx_thread) {
        chThdTerminate(tx_thread);
        chThdWait(tx_thread);
        if (!thread_running) tx_thread = nullptr;
    }

    transmitter_model.disable();
    baseband::shutdown();
    audio::output::stop();
}

void MorseRadiotxView::on_show() {
    ptt_button_visibility(true);
    start_time = 0;
    end_time = 0;
    transmit_time = 0;
}

void MorseRadiotxView::focus() {
    options_mode.focus();
}

MorseRadiotxView::MorseTimings MorseRadiotxView::calculate_morse_timings(uint32_t wpm) {
    MorseRadiotxView::MorseTimings t;
    if (wpm < 10) wpm = 10;
    if (wpm > 45) wpm = 45;
    t.dot_ms = 1200 / wpm;

    t.dash_ms = 3 * t.dot_ms;
    t.symbol_gap = t.dot_ms;
    t.char_gap = 3 * t.dot_ms;
    t.word_gap = 7 * t.dot_ms;
    return t;
}

void MorseRadiotxView::transmit_morse_message() {
    std::string full_message = "";

    if (chk_trans.value()) {
        ptt_button_visibility(false);
        return;
    } else {
        full_message = msg_buffer;
        if (chk_callsgn.value() && !call_sign.empty()) {  // call sign
            full_message += " " + call_sign;
        }
    }

    // enable transmit
    if (!transmit) {
        transmit = true;
        transmitter_model.enable();
        ui_toggle();
    }

    uint8_t loop_seconds = static_cast<uint8_t>(wait_time.value());
    progressbar.set_max(loop_seconds * 2);

    do {
        for (size_t i = 0; i < full_message.length(); i++) {
            if (chThdShouldTerminate()) break;

            char c = full_message[i];

            std::string s_char(1, c);
            // space
            if (c == ' ') {
                console_text.write(" ");
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
                    if (chThdShouldTerminate()) break;
                    // pause between signs
                    baseband::set_morsetx_key(false);
                    chThdSleepMilliseconds(current_timings.symbol_gap);
                }
                if (chThdShouldTerminate()) break;
                if (current_timings.char_gap > current_timings.symbol_gap) {
                    chThdSleepMilliseconds(current_timings.char_gap - current_timings.symbol_gap);
                }
            }
        }
        if (chThdShouldTerminate()) break;
        if (chk_loop.value()) {
            console_text.write(" ");
            for (uint8_t s = 0; s < (loop_seconds * 2); s++) {
                if (chThdShouldTerminate()) break;
                progressbar.set_value(s + 1);
                if (!chk_loop.value()) break;
                chThdSleepMilliseconds(500);
            }
            progressbar.set_value(0);
        }
        if (!chk_loop.value()) break;
    } while (chk_loop.value() && !chThdShouldTerminate());

    if (chk_trans.value()) ptt_button_visibility(false);
    baseband::set_morsetx_key(false);
    transmit_time = chTimeNow();
    decode_timeout_calc = false;
    thread_running = false;
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
        if (confidence == 1.5)
            color_id = 4;
        else if (confidence < 0.8)
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

void MorseRadiotxView::ptt_button_visibility(bool hidden) {
    bool hiddenorig = btn_ptt.hidden();
    if (hiddenorig != hidden) {
        btn_ptt.hidden(hidden);
        if (!hidden) btn_ptt.focus();
        if (hidden) {
            // hack fix until widget hidig problem is not solved.
            auto r = btn_ptt.screen_rect();
            Painter p;
            p.fill_rectangle_unrolled8(r, Theme::getInstance()->fg_light->background);
        }
    }
}

void MorseRadiotxView::ui_toggle() {
    // 0=AM, 1=FM, 2=DSB, 3=USB, 4=LSB

    if (transmit) {
        options_mode.set_style(Theme::getInstance()->fg_dark);
        options_mode.set_focusable(false);
        tone_.set_style(Theme::getInstance()->fg_dark);
        tone_.set_focusable(false);
        wpm_.set_style(Theme::getInstance()->fg_dark);
        wpm_.set_focusable(false);
        btn_message.set_style(Theme::getInstance()->fg_dark);
        btn_message.set_focusable(false);
        btn_calls.set_style(Theme::getInstance()->fg_dark);
        btn_calls.set_focusable(false);
        chk_trans.set_style(Theme::getInstance()->fg_dark);
        chk_trans.set_focusable(false);
        chk_callsgn.set_style(Theme::getInstance()->fg_dark);
        chk_callsgn.set_focusable(false);
        wait_time.set_style(Theme::getInstance()->fg_dark);
        wait_time.set_focusable(false);
        bandwidth.set_style(Theme::getInstance()->fg_dark);
        bandwidth.set_focusable(false);

    } else {
        options_mode.set_style(Theme::getInstance()->bg_darker);
        options_mode.set_focusable(true);
        wpm_.set_style(Theme::getInstance()->bg_darker);
        wpm_.set_focusable(true);
        btn_message.set_style(Theme::getInstance()->bg_darker);
        btn_message.set_focusable(true);
        btn_calls.set_style(Theme::getInstance()->bg_darker);
        btn_calls.set_focusable(true);
        chk_trans.set_style(Theme::getInstance()->bg_darker);
        chk_trans.set_focusable(true);
        chk_callsgn.set_style(Theme::getInstance()->bg_darker);
        chk_callsgn.set_focusable(true);
        wait_time.set_style(Theme::getInstance()->bg_darker);
        wait_time.set_focusable(true);
        ptt_button_visibility(true);
        tone_.set_style(Theme::getInstance()->bg_darker);
        tone_.set_focusable(true);

        if (current_mode == 1) {  // FM mode
            bandwidth.set_style(Theme::getInstance()->bg_darker);
            bandwidth.set_focusable(true);
        } else {
            bandwidth.set_style(Theme::getInstance()->fg_dark);
            bandwidth.set_focusable(false);
        }
    }  // transmit false
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
    if (transmit_time != 0 && transmit) {  // Tx disable if time is up
        int64_t gap_delta = (chTimeNow() - transmit_time);
        if (gap_delta >= ((morse_decoder_.getInterWordThreshold() * ((chk_trans.value()) ? 10 : 1)))) {
            if (tx_thread) {
                chThdTerminate(tx_thread);
                chThdWait(tx_thread);
            }
            transmit = false;
            tx_view.set_transmitting(false);
            transmitter_model.disable();
            thread_running = false;
            tx_thread = nullptr;
            transmit_time = 0;
            if (!chk_trans.value()) writeCharToConsole(" ", 1.0);
            ui_toggle();
        }
    }
}

}  // namespace ui::external_app::morseradiotx
