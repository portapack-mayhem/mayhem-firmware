/*
 * RollJam — Automated rolling-code capture tool for PortaPack Mayhem
 *
 * This file is part of PortaPack.
 * This program is free software; see GPLv2 or later.
 */

#include "ui_rolljam.hpp"
#include "ui_receiver.hpp"
#include "ui_freqman.hpp"
#include "ui/ui_spectrum.hpp"

#include "baseband_api.hpp"
#include "portapack.hpp"
#include "string_format.hpp"
#include "file_reader.hpp"
#include "io_convert.hpp"

using namespace portapack;

namespace ui {

/* ------------------------------------------------------------------ */

RollJamView::RollJamView(NavigationView& nav)
    : nav_{nav} {

    make_new_directory(rolljam_dir_);

    add_children({
        &labels,
        &field_frequency,
        &field_frequency_step,
        &field_rf_amp_tx,
        &text_status,
        &text_codes,
        &field_max_codes,
        &text_max_label,
        &button_start_stop,
        &button_capture,
        &button_replay,
        &record_view,
    });

    field_frequency.set_value(433'920'000);
    field_frequency.set_step(10'000);
    field_frequency.updated = [this](rf::Frequency f) {
        if (state_ == RollJamState::JAMMING)
            transmitter_model.set_target_frequency(f);
    };

    field_frequency_step.set_by_value(receiver_model.frequency_step());
    field_frequency_step.on_change = [this](size_t, OptionsField::value_t v) {
        receiver_model.set_frequency_step(v);
        field_frequency.set_step(v);
    };

    field_max_codes.set_value(max_codes_);
    field_max_codes.on_change = [this](int32_t v) {
        max_codes_ = v;
    };

    button_capture.hidden(true);
    button_replay.hidden(true);
    record_view.hidden(true);

    button_start_stop.on_select = [this](Button&) {
        if (state_ == RollJamState::IDLE || state_ == RollJamState::DONE) {
            start_jamming();
        } else {
            stop_jamming();
        }
    };

    button_capture.on_select = [this](Button&) {
        if (state_ == RollJamState::JAMMING && captured_codes_.size() < max_codes_) {
            start_capture();
        }
    };

    button_replay.on_select = [this](Button&) {
        if (!captured_codes_.empty() && state_ != RollJamState::REPLAYING) {
            start_replay();
        }
    };

    update_ui();
}

RollJamView::~RollJamView() {
    if (state_ == RollJamState::JAMMING) {
        baseband::set_jammer(false, jammer::JammerType::TYPE_TONE, 0);
        transmitter_model.disable();
    } else if (state_ == RollJamState::CAPTURING) {
        receiver_model.disable();
    } else if (state_ == RollJamState::REPLAYING) {
        replay_thread_.reset();
        transmitter_model.disable();
    }
    baseband::shutdown();
}

/* ------------------------------------------------------------------ */
/* state machine                                                       */
/* ------------------------------------------------------------------ */

void RollJamView::start_jamming() {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    transmitter_model.set_rf_amp(1);
    transmitter_model.set_tx_gain(tx_gain);
    transmitter_model.set_baseband_bandwidth(jammer_bandwidth);
    transmitter_model.set_sampling_rate(jammer_sample_rate);
    transmitter_model.set_target_frequency(field_frequency.value());
    transmitter_model.enable();

    baseband::set_jammer(true, jammer::JammerType::TYPE_TONE, 0);

    state_ = RollJamState::JAMMING;
    text_status.set("JAMMING — door blocked");
    button_capture.hidden(false);
    button_replay.hidden(true);
    record_view.hidden(true);
    button_start_stop.set_text("STOP");
    update_ui();
}

void RollJamView::stop_jamming() {
    baseband::set_jammer(false, jammer::JammerType::TYPE_TONE, 0);
    transmitter_model.disable();
    baseband::shutdown();

    state_ = RollJamState::IDLE;
    text_status.set("IDLE");
    button_capture.hidden(true);
    button_replay.hidden(!captured_codes_.empty());
    record_view.hidden(true);
    button_start_stop.set_text("START");
    update_ui();
}

/* ------------------------------------------------------------------ */
/* capture                                                             */
/* ------------------------------------------------------------------ */

void RollJamView::start_capture() {
    if (state_ == RollJamState::JAMMING) {
        baseband::set_jammer(false, jammer::JammerType::TYPE_TONE, 0);
        transmitter_model.disable();
    }
    baseband::shutdown();

    baseband::run_image(portapack::spi_flash::image_tag_capture);

    receiver_model.set_target_frequency(field_frequency.value());
    receiver_model.set_sampling_rate(capture_sample_rate);
    receiver_model.set_baseband_bandwidth(capture_bandwidth);
    receiver_model.set_lna(rx_gain_lna);
    receiver_model.set_vga(rx_gain_vga);
    receiver_model.enable();

    baseband::set_sample_rate(capture_sample_rate);

    // build a unique filename
    auto ts = to_string_timestamp(rtc_time::now());
    std::string fname = "RJ_" + ts + ".C16";
    for (auto& c : fname)
        if (c == ' ' || c == ':') c = '_';

    std::filesystem::path file_path = rolljam_dir_ / fname;

    auto writer = std::make_unique<FileConvertWriter>();
    auto open_result = writer->create(file_path);
    if (open_result) {
        nav_.display_modal("Error", "Cannot create:\n" + file_path.string());
        receiver_model.disable();
        baseband::shutdown();
        start_jamming();
        return;
    }

    // save path + metadata before thread takes ownership of writer
    captured_codes_.push_back({file_path, {field_frequency.value(), capture_sample_rate}});

    auto cap = std::make_unique<CaptureThread>(
        std::move(writer),
        16384, 3,
        []() {},              // success callback — we use message handler instead
        [](File::Error) {}    // error callback   — we use message handler instead
    );

    // CaptureThread starts immediately; we hold a ref to stop it later
    (void)cap;  // TODO: store to stop on demand

    state_ = RollJamState::CAPTURING;
    text_status.set("CAPTURING " + std::to_string(captured_codes_.size()) +
                   "/" + std::to_string(max_codes_));

    button_capture.hidden(true);
    record_view.hidden(true);
    update_ui();
}

void RollJamView::on_capture_done(std::optional<File::Error> error) {
    receiver_model.disable();
    baseband::capture_stop();
    baseband::shutdown();

    if (error) {
        text_status.set("CAPTURE ERROR");
        // remove last entry if failed
        if (!captured_codes_.empty()) captured_codes_.pop_back();
        start_jamming();
        button_capture.hidden(false);
    } else {
        std::string info;
        for (size_t i = 0; i < captured_codes_.size(); i++) {
            info += "Code " + std::to_string(i + 1) + ": " +
                    captured_codes_[i].first.filename().string() + "\n";
        }
        text_codes.set(info);

        if (captured_codes_.size() >= max_codes_) {
            stop_jamming();
            state_ = RollJamState::DONE;
            text_status.set("DONE — " + std::to_string(captured_codes_.size()) + " codes");
            button_capture.hidden(true);
            button_replay.hidden(false);
        } else {
            start_jamming();
            button_capture.hidden(false);
        }
    }

    button_replay.hidden(captured_codes_.empty());
    update_ui();
}

/* ------------------------------------------------------------------ */
/* replay                                                              */
/* ------------------------------------------------------------------ */

void RollJamView::start_replay() {
    if (captured_codes_.empty()) return;

    if (state_ == RollJamState::JAMMING) {
        baseband::set_jammer(false, jammer::JammerType::TYPE_TONE, 0);
        transmitter_model.disable();
    } else if (state_ == RollJamState::CAPTURING) {
        receiver_model.disable();
    }
    baseband::shutdown();

    baseband::run_image(portapack::spi_flash::image_tag_replay);

    transmitter_model.set_rf_amp(1);
    transmitter_model.set_tx_gain(tx_gain);
    transmitter_model.set_baseband_bandwidth(
        filter_bandwidth_for_sampling_rate(capture_sample_rate));
    transmitter_model.set_sampling_rate(capture_sample_rate);
    transmitter_model.set_target_frequency(captured_codes_[0].second.center_frequency);
    transmitter_model.enable();

    baseband::set_sample_rate(capture_sample_rate);

    auto reader = std::make_unique<FileConvertReader>();
    auto open_result = reader->open(captured_codes_[0].first);
    if (open_result) {
        transmitter_model.disable();
        baseband::shutdown();
        nav_.display_modal("Error", "Cannot open:\n" + captured_codes_[0].first.string());
        return;
    }

    replay_ready_signal_ = false;
    replay_thread_ = std::make_unique<ReplayThread>(
        std::move(reader),
        0x4000,
        3,
        &replay_ready_signal_,
        [](uint32_t return_code) {
            ReplayThreadDoneMessage msg{return_code};
            EventDispatcher::send_message(msg);
        });

    state_ = RollJamState::REPLAYING;
    text_status.set("REPLAYING Code 1…");
    button_replay.hidden(true);
    button_capture.hidden(true);
    update_ui();
}

void RollJamView::on_replay_done(uint32_t return_code) {
    replay_thread_.reset();
    transmitter_model.disable();
    baseband::shutdown();

    if (return_code == 0)
        text_status.set("REPLAY DONE. Code 2+ saved.");
    else
        text_status.set("REPLAY ERR " + std::to_string(return_code));

    state_ = RollJamState::DONE;
    button_replay.hidden(false);
    update_ui();
}

/* ------------------------------------------------------------------ */
/* UI helpers                                                          */
/* ------------------------------------------------------------------ */

void RollJamView::focus() {
    button_start_stop.focus();
}

void RollJamView::update_ui() {
    field_rf_amp_tx.set_value(1);
}

}  // namespace ui
