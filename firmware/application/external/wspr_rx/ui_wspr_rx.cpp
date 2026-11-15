/*
 * Copyright (C) 2026 Ivan
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

#include "ui_wspr_rx.hpp"

#include "audio.hpp"
#include "baseband_api.hpp"
#include "file_path.hpp"
#include "rtc_time.hpp"
#include "string_format.hpp"

using namespace portapack;

namespace ui::external_app::wspr_rx {

void WSPRRxView::focus() {
    field_frequency.focus();
}

WSPRRxView::~WSPRRxView() {
    receiver_model.disable();
    audio::output::stop();
    baseband::shutdown();
}

WSPRRxView::WSPRRxView(NavigationView& nav)
    : nav_{nav} {
    add_children({&field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &rssi,
                  &channel,
                  &field_volume,
                  &field_frequency,
                  &options_band,
                  &text_status,
                  &text_decodes,
                  &text_min,
                  &field_min_agr,
                  &console});

    channel.set_overload_threshold(-3);

    // A WSPR window is two minutes, so the console stays mostly empty and any
    // rows the hardware scroll region inherited from the previous screen show
    // through as frame rows nothing has written. Clear the whole region once
    // at startup.
    console.clear(false);

    field_frequency.set_step(100);

    /* Going through the frequency field rather than the model keeps the two in step:
     * the field retunes the receiver itself and redraws with the new dial. */
    options_band.on_change = [this](size_t, OptionsField::value_t v) {
        if (v != 0)
            field_frequency.set_value(v);
    };
    field_frequency.updated = [this](rf::Frequency f) {
        options_band.set_by_value(f);
    };
    options_band.set_by_value(receiver_model.target_frequency());

    baseband::run_image(portapack::spi_flash::image_tag_wspr_rx);

    /* WSPR occupies a few Hz per signal inside a 200 Hz sub-band, but the decoder needs
     * the whole sub-band at once, so the receiver runs wide and the channel filter
     * narrows it. set_modulation() is deliberately not called: it would replace the WSPR
     * baseband image. */
    receiver_model.set_sampling_rate(sampling_rate);
    receiver_model.set_baseband_bandwidth(baseband_bandwidth);
    receiver_model.enable();

    field_min_agr.set_value(min_agreement_, false);
    field_min_agr.on_change = [this](int32_t v) {
        min_agreement_ = v;
    };

    audio::output::start();

    log_file_.append(logs_dir / u"WSPR.TXT");

    /* Debug log opened once like WSPR.TXT; if it fails (no SD card), the flag
     * keeps the per-window writes from hitting a never-opened file. */
    debug_log_ok_ = !debug_log_file_.append(logs_dir / u"WSPR_DEBUG.TXT");
}

void WSPRRxView::on_packet(const WSPRPacketMessage* message) {
    if (message->callsign[0] == '\0')
        return;

    /* The confirmation table first: a repeat of the same station turns this
     * decode into a confirmed one. Everything is displayed, at three trust
     * levels - confirmed repeats in green and logged to WSPR.TXT, strong
     * single decodes (high re-encode agreement) in the plain colour, weak
     * singles in grey with a question mark. An empty screen reads as a dead
     * band; a grey line reads as "something is there, text not stable yet";
     * only the green lines are station reports. Unconfirmed decodes are also
     * logged raw to the debug file for the session analysis. */
    const bool confirmed = confirm_decode(message);
    if (!confirmed && debug_log_ok_) {
        rtc::RTC datetime;
        rtcGetTime(&RTCD1, &datetime);
        const std::string entry = std::string{"unconf "} + message->callsign + " " +
                                  message->grid + " " +
                                  to_string_dec_int(message->dbm) + " " +
                                  to_string_dec_int(message->snr_db) + " " +
                                  to_string_dec_uint(message->freq_hz);
        debug_log_file_.write_entry(datetime, entry);
    }

    /* Display threshold: confirmed repeats always show (physical evidence);
     * singles below the user's agreement floor stay off the screen - but they
     * were stored above, so a later repeat still confirms and appears. */
    if (!confirmed && message->agreement_pct < min_agreement_)
        return;

    /* Strong singles: high re-encode agreement, trusted without a repeat. */
    const bool strong = message->agreement_pct >= 85;

    /* "callsign grid report SNR" takes the left of the line and the audio frequency the
     * right, so the frequencies line up in a column down the screen and the passband can
     * be read at a glance. A line too long to leave room for the column keeps its text,
     * and the frequency moves down a row and stays in the column. */
    std::string text{message->callsign};
    text += " ";
    text += message->grid;
    text += " ";
    text += to_string_dec_int(message->dbm);
    text += "dB ";
    text += to_string_dec_int(message->snr_db);
    text += "dB";
    const std::string freq = to_string_dec_uint(message->freq_hz) + "Hz";
    /* screen_width is set at runtime, so the column count is taken here rather than
     * fixed at compile time. The font is the 8 px fixed one the rest of the layout uses. */
    const size_t columns = screen_width / 8;

    /* Weak singles carry the question mark: two columns, counted in the padding. */
    if (!confirmed && !strong)
        text += " ?";

    /* Padding is measured before any colour escape would go on, because an escape
     * occupies bytes of the string and no columns on screen. */
    if (text.length() + freq.length() < columns)
        text.append(columns - freq.length() - text.length(), ' ');
    else
        text += '\n' + std::string(columns - freq.length(), ' ');

    /* Trust levels: confirmed repeats in green, strong singles plain, weak
     * singles in grey with their question mark. */
    if (confirmed)
        console.writeln(STR_COLOR_GREEN + text + STR_COLOR_WHITE + freq);
    else if (strong)
        console.writeln(text + freq);
    else
        console.writeln(STR_COLOR_LIGHT_GREY + text + " ?" + STR_COLOR_WHITE + freq);

    decodes_total++;
    text_decodes.set(to_string_dec_uint(decodes_total) + (decodes_total < 10000 ? " RX" : "RX"));

    /* One line per report: callsign, grid, the station's own dBm, the decoder's rough
     * SNR and the audio frequency, prefixed by the logger with the RTC timestamp — the
     * fields wsprnet.org reports are built from, minus the upload. */
    rtc::RTC datetime;
    rtcGetTime(&RTCD1, &datetime);
    const std::string entry = std::string{message->callsign} + " " + message->grid + " " +
                              to_string_dec_int(message->dbm) + " " +
                              to_string_dec_int(message->snr_db) + " " +
                              to_string_dec_uint(message->freq_hz);
    log_file_.write_entry(datetime, entry);
}

void WSPRRxView::on_status(const WSPRRxStatusMessage* message) {
    /* One status message arrives per window, so this is where the confirmation table
     * ages: every occupied slot counts one window older, and slots past the limit are
     * freed - a station that stopped repeating can no longer be confirmed. */
    confirm_age_all();

    /* The sync score is the only live signal-quality readout: noise windows
     * score a couple of units, a real transmission several times that. It is
     * the number to watch when deciding whether a decode is believable. */
    const std::string score = " s=" + to_string_dec_int(message->sync_score);

    /* The state name is reused verbatim for the debug log line, so the file
     * reads the same words the screen shows. */
    const char* state_name = "Searching";

    switch (message->state) {
        case WSPRRxStatusMessage::SyncState::Searching:
            state_name = "Searching";
            text_status.set_style(Theme::getInstance()->fg_red);
            text_status.set("Searching");
            break;
        case WSPRRxStatusMessage::SyncState::Heard:
            state_name = "Heard";
            text_status.set_style(Theme::getInstance()->fg_yellow);
            text_status.set("Heard" + score);
            break;
        case WSPRRxStatusMessage::SyncState::Syncing:
            state_name = "Syncing";
            text_status.set_style(Theme::getInstance()->fg_yellow);
            text_status.set("Syncing" + score);
            break;
        case WSPRRxStatusMessage::SyncState::Locked:
            state_name = "Locked";
            text_status.set_style(Theme::getInstance()->fg_green);
            text_status.set("Locked" + score);
            break;
    }

    /* One line per 120 s window into WSPR_DEBUG.TXT: the state, the decode count and
     * the sync score, plus the rich telemetry of the same window - the candidate
     * count, the best path metric, the tracker's phase correction and every non-zero
     * candidate slot - which arrives on the WSPRDebugMessage just ahead of this
     * status message. The RTC timestamp is prefixed by the logger exactly as the
     * decode lines in WSPR.TXT get one. */
    if (debug_log_ok_) {
        rtc::RTC datetime;
        rtcGetTime(&RTCD1, &datetime);
        std::string entry = std::string{"win "} + state_name +
                            " decodes=" + to_string_dec_uint(message->decode_count) +
                            " score=" + to_string_dec_int(message->sync_score) +
                            " cand=" + to_string_dec_uint(last_debug_.num_candidates) +
                            " metric=" + to_string_dec_int(last_debug_.best_metric) +
                            " skip=" + to_string_dec_int(last_debug_.last_skip);
        /* Only slots backed by a result are printed: an unused slot is zero-filled,
         * and a real decode always sits in the 1400-1600 Hz passband, never at
         * frequency zero. Kept on one line so a session reads as one row per window. */
        for (size_t i = 0; i < WSPRDebugMessage::max_candidates; i++) {
            const auto& candidate = last_debug_.candidates[i];
            if (candidate.freq_hz == 0)
                continue;
            entry += " c" + to_string_dec_uint(i) + "=" +
                     to_string_dec_int(candidate.sync_excess) + "/" +
                     to_string_dec_int(candidate.row_offset) + "/" +
                     to_string_dec_int(candidate.freq_hz);
        }
        debug_log_file_.write_entry(datetime, entry);
    }
}

/* The decode's key matches a table slot when callsign, grid and dBm are exact and the
 * audio frequency stayed within the tolerance a stable station holds between slots. */
bool WSPRRxView::confirm_match(const ConfirmSlot& slot, const WSPRPacketMessage* message) const {
    if (slot.callsign[0] == '\0')
        return false;
    if (std::strncmp(slot.callsign, message->callsign, sizeof(slot.callsign)) != 0)
        return false;
    if (std::strncmp(slot.grid, message->grid, sizeof(slot.grid)) != 0)
        return false;
    if (slot.dbm != message->dbm)
        return false;
    const int16_t diff = message->freq_hz - slot.freq_hz;
    return diff >= -CONFIRM_FREQ_TOLERANCE && diff <= CONFIRM_FREQ_TOLERANCE;
}

/* Second look at a decode that did not match outright: same station, drifted
 * frequency. Updating that slot in place keeps one wandering station from multiplying
 * across the table and evicting the others. */
WSPRRxView::ConfirmSlot* WSPRRxView::confirm_stale_slot(const WSPRPacketMessage* message) {
    for (size_t i = 0; i < CONFIRM_SLOTS; i++) {
        ConfirmSlot& slot = confirm_slots_[i];
        if (slot.callsign[0] != '\0' &&
            std::strncmp(slot.callsign, message->callsign, sizeof(slot.callsign)) == 0 &&
            std::strncmp(slot.grid, message->grid, sizeof(slot.grid)) == 0 &&
            slot.dbm == message->dbm)
            return &slot;
    }
    return nullptr;
}

/* A decode that matched nothing: a free slot takes it, else the oldest entry is
 * replaced, so a table full of recently heard stations still makes room. */
void WSPRRxView::confirm_store(const WSPRPacketMessage* message) {
    ConfirmSlot* slot = nullptr;
    ConfirmSlot* oldest = &confirm_slots_[0];
    for (size_t i = 0; i < CONFIRM_SLOTS; i++) {
        ConfirmSlot& candidate = confirm_slots_[i];
        if (candidate.callsign[0] == '\0') {
            slot = &candidate;
            break;
        }
        if (candidate.age > oldest->age)
            oldest = &candidate;
    }
    if (slot == nullptr) {
        slot = confirm_stale_slot(message);
        if (slot == nullptr) slot = oldest;
    }

    /* strncpy pads the remainder with terminators, and the source arrays are always
     * terminated within their sizes, so the slots stay valid C strings. */
    std::strncpy(slot->callsign, message->callsign, sizeof(slot->callsign));
    std::strncpy(slot->grid, message->grid, sizeof(slot->grid));
    slot->dbm = message->dbm;
    slot->freq_hz = message->freq_hz;
    slot->age = 0;
}

/* The gate itself: a hit re-arms the slot and confirms the decode; a miss is stored
 * unconfirmed and waits for a later window to repeat it. */
bool WSPRRxView::confirm_decode(const WSPRPacketMessage* message) {
    for (size_t i = 0; i < CONFIRM_SLOTS; i++) {
        if (confirm_match(confirm_slots_[i], message)) {
            confirm_slots_[i].age = 0;
            return true;
        }
    }
    confirm_store(message);
    return false;
}

void WSPRRxView::confirm_age_all() {
    for (size_t i = 0; i < CONFIRM_SLOTS; i++) {
        ConfirmSlot& slot = confirm_slots_[i];
        if (slot.callsign[0] == '\0')
            continue;
        if (++slot.age > CONFIRM_MAX_AGE)
            slot.callsign[0] = '\0';  // zeroed callsign marks the slot free again
    }
}

void WSPRRxView::on_debug(const WSPRDebugMessage* message) {
    /* The rich telemetry rides one message ahead of the status message that writes
     * it; stashing here keeps on_status the single place the debug line is built. */
    last_debug_.num_candidates = message->num_candidates;
    last_debug_.best_metric = message->best_metric;
    last_debug_.last_skip = message->last_skip;
    for (size_t i = 0; i < WSPRDebugMessage::max_candidates; i++)
        last_debug_.candidates[i] = message->candidates[i];
}

}  // namespace ui::external_app::wspr_rx
