#include "ui_morse_radio.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;

namespace ui::external_app::morse_radio {

MorseRadioView::MorseRadioView(ui::NavigationView& nav)
    : nav_(nav) {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());
    add_children({&rssi,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_volume,
                  &field_frequency,
                  &txt_last,
                  &txt_speed,
                  &txt_freq,
                  &txt_clip,
                  &options_mode,
                  &btn_clear,
                  &console_text,
                  &labels,
                  &chk_log,
                  &field_squelch});
    field_frequency.set_step(100);

    btn_clear.on_select = [this](Button&) {
        console_text.clear(true);
        txt_last.set("");
        time_stamp = false;
        if (logger && save_log)
            logger->init_daily_log(logs_dir);
    };

    field_squelch.on_change = [this](int32_t v) {
        receiver_model.set_squelch_level(v);
    };

    options_mode.on_change = [this](size_t, int32_t mode) {
        audio::output::stop();
        receiver_model.disable();
        morse_decoder_.resetLearning();
        if (mode == MORSE_NFM) {
            receiver_model.set_am_configuration(4);
            receiver_model.set_modulation(ReceiverModel::Mode::NarrowbandFMAudio);
            field_squelch.set_style(Theme::getInstance()->option_active);
            field_squelch.set_focusable(true);
            field_squelch.set_value(receiver_model.squelch_level());
            audio::set_rate(audio::Rate::Hz_24000);
        } else {
            audio::set_rate(audio::Rate::Hz_12000);
            receiver_model.set_modulation(ReceiverModel::Mode::AMAudio);
            if (mode == MORSE_AM_CW || mode == MORSE_AM_DSB)
                receiver_model.set_am_configuration(7);
            else if (mode == MORSE_AM_USB)
                receiver_model.set_am_configuration(9);
            else  // LSB
                receiver_model.set_am_configuration(10);
            field_squelch.set_style(Theme::getInstance()->fg_dark);
            field_squelch.set_focusable(false);
        }
        baseband::set_moreserx_config(mode);
        saved_mode = mode;

        audio::output::start();
        receiver_model.set_headphone_volume(receiver_model.headphone_volume());  // WM8731 hack.

        receiver_model.set_sampling_rate(3072000);
        receiver_model.set_baseband_bandwidth(1750000);
        receiver_model.enable();
    };
    options_mode.set_selected_index(saved_mode);

    logger = std::make_unique<MorseLogger>();
    chk_log.on_select = [this](Checkbox&, bool save) {
        save_log = save;
        if (logger && save_log)
            logger->init_daily_log(logs_dir);
    };
}

void MorseLogger::init_daily_log(const std::filesystem::path& log_dir) {
    auto now = rtc_time::now();
    std::string pattern = "morse_";
    uint16_t y = now.year();
    pattern += (char)('0' + (y / 1000) % 10);
    pattern += (char)('0' + (y / 100) % 10);
    pattern += (char)('0' + (y / 10) % 10);
    pattern += (char)('0' + (y % 10));

    uint8_t m = now.month();
    pattern += (char)('0' + (m / 10) % 10);
    pattern += (char)('0' + (m % 10));

    uint8_t d = now.day();
    pattern += (char)('0' + (d / 10) % 10);
    pattern += (char)('0' + (d % 10));

    pattern += "_???.TXT";

    auto full_pattern_path = log_dir / pattern;
    auto final_path = next_filename_matching_pattern(full_pattern_path);
    if (!final_path.empty()) {
        this->append(final_path);
    }
}

void MorseLogger::radio_set_log(const std::string& morse_mode) {
    int64_t freq = receiver_model.target_frequency();
    std::string header = "Freq:" + to_string_rounded_freq(freq, 4);
    header += "MHz, ";
    header += "RX MODE:" + morse_mode;
    header += "\r\nMessage:";
    log_file.write_raw(header);
}

bool MorseLogger::on_packet(const std::string& content, bool time, const std::string& morse_mode) {
    if (!time) {
        log_file.write_raw_no_newline("\r\n\r\n");
        auto timestamp = to_string_datetime(rtc_time::now(), YMDHMS);
        log_file.write_raw("[" + timestamp + "] ");
        radio_set_log(morse_mode);
        char_count = 0;
        time = true;
    }

    if (content.empty()) return time;
    std::string s;
    for (char c : content) {
        s = c;
        if ((char_count >= 35 && c == ' ') || (char_count >= 47)) {
            log_file.write_raw_no_newline("\r\n");

            if (c != ' ') {
                log_file.write_raw_no_newline(s);
                char_count = 1;
            } else {
                char_count = 0;
            }
        } else {
            log_file.write_raw_no_newline(s);
            char_count++;
        }
    }

    return time;
}

MorseRadioView::~MorseRadioView() {
    receiver_model.disable();
    baseband::shutdown();
    audio::output::stop();
}

void MorseRadioView::focus() {
    field_frequency.focus();
    txt_clip.set_style(Theme::getInstance()->fg_red);
    txt_clip.hidden(true);
}

void MorseRadioView::check_for_timeout() {
    uint64_t now = chTimeNow();
    if (last_activity_time == 0) {
        last_activity_time = now;
        return;
    }
    uint64_t quiet_duration = now - last_activity_time;
    if (quiet_duration >= 60000) {
        morse_decoder_.resetLearning();
        time_stamp = false;
        last_activity_time = now;
    }
}

int32_t MorseRadioView::ProcessSignal(int32_t sig_time_us) {
    int8_t sign = (sig_time_us > 0) ? 1 : ((sig_time_us < 0) ? -1 : 0);
    int32_t result = 0;

    if (last_sign_ == 0) {
        last_sign_ = sign;
        accumulator_us_ = sig_time_us;
        long_pause_sent_ = false;
        return 0;
    }
    if (sign == last_sign_) {
        accumulator_us_ += sig_time_us;

        if (sign < 0) {
            int32_t threshold_us = morse_decoder_.getInterWordThreshold() * 1000;
            if (!long_pause_sent_ && accumulator_us_ <= -threshold_us) {
                result = accumulator_us_ / 1000;
                long_pause_sent_ = true;
            }
        }
    } else {
        // state change
        if (!long_pause_sent_) {
            result = accumulator_us_ / 1000;
        } else {
            result = 0;
        }
        accumulator_us_ = sig_time_us;
        last_sign_ = sign;
        long_pause_sent_ = false;
    }
    return result;
}

void MorseRadioView::on_data(const MorseRXDataMessage* message) {
    int32_t r;

    txt_clip.hidden(!message->clipped);

    for (uint8_t i = 0; i <= message->state_cnt; ++i) {
        r = ProcessSignal(message->state_durations[i]);
        auto result = morse_decoder_.handleInput((r != 0) ? r : 0);
        if (result.isValid()) {
            last_activity_time = chTimeNow();  // start reset timer on valid input
            writeCharToConsole(result.text, result.confidence);
            if (logger && save_log) {
                time_stamp = logger->on_packet(result.text, time_stamp, options_mode.selected_index_name());
            }
            float dah_time = morse_decoder_.getCurrentTimeUnit() * 3.0f;
            if (dah_time > 0) {
                uint16_t wpm = (uint16_t)(3600.0f / (dah_time + 18.0f) + 0.5f);
                txt_speed.set(to_string_dec_uint(wpm));
            }
        }
    }
}

void MorseRadioView::on_freq(const MorseRXfreqMessage* message) {
    std::string ret = " ";
    uint32_t freq = message->measured_frequency;
    if (freq < 301) {
        ret = "<";
        freq = 300;
    }
    if (freq > 2299) {
        freq = 2300;
        ret = ">";
    }
    if (freq < 400 || freq > 1400)
        txt_freq.set_style(Theme::getInstance()->fg_red);
    else if (freq <= 580 || freq >= 1220)
        txt_freq.set_style(Theme::getInstance()->fg_yellow);
    else
        txt_freq.set_style(Theme::getInstance()->fg_green);
    ret += to_string_dec_uint(freq);
    txt_freq.set(ret);
}

void MorseRadioView::writeCharToConsole(const std::string& ch, double confidence) {
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

}  // namespace ui::external_app::morse_radio
