#include "ui_morse_practice.hpp"

using namespace portapack;

namespace ui::external_app::morse_practice {

MorsePracticeView::MorsePracticeView(ui::NavigationView& nav)
    : nav_(nav) {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());
    add_children({&btn_tt,
                  &txt_last,
                  &btn_clear,
                  &console_text,
                  &field_volume});
    audio::set_rate(audio::Rate::Hz_24000);

    btn_tt.on_select = [this](Button&) {
        if (button_touch) {
            button_touch = false;
            return;
        }
        button_was_selected = true;
        onPress();
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
    audio::output::start();
    auto vol = field_volume.value();
    field_volume.set_value(0);
    field_volume.set_value(vol);
}

MorsePracticeView::~MorsePracticeView() {
    receiver_model.disable();
    baseband::shutdown();
    audio::output::stop();
}

void MorsePracticeView::on_show() {
    console_text.write("Morse Practice ready\n");
    start_time = 0;
    end_time = 0;
}

void MorsePracticeView::focus() {
    btn_tt.focus();
}

void MorsePracticeView::onPress() {
    start_time = chTimeNow();
    if (end_time != 0) {
        int64_t gap_delta = (chTimeNow() - end_time);
        auto result = morse_decoder_.handleInput(-gap_delta);
        if (result.isValid()) {
            writeCharToConsole(result.text, result.confidence);
        }
    }
    end_time = 0;
    decode_timeout_calc = false;
    baseband::request_audio_beep(1000, 24000, 2000);
}

void MorsePracticeView::onRelease() {
    end_time = chTimeNow();
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

void MorsePracticeView::writeCharToConsole(const std::string& ch, double confidence) {
    if (ch.empty()) {
        return;
    }

    txt_last.set(morse_decoder_.getLastSequence().c_str());

    last_color_id = color_id;
    std::string color = "";

    if (ch == " ") {
        color_id = 0;
    } else if (ch == MORSEDEC_ERROR) {
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

inline bool MorsePracticeView::tx_button_held() {
    const auto switches_state = get_switches_state();
    return switches_state[(size_t)ui::KeyEvent::Select];
}

void MorsePracticeView::on_framesync() {
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
}

}  // namespace ui::external_app::morse_practice
