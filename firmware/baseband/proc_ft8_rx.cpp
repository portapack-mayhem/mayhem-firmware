/*
 * Copyright (C) 2026 Dmytro Onyshko
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

#include "proc_ft8_rx.hpp"
#include "portapack_shared_memory.hpp"
#include "audio_dma.hpp"
#include "dsp_fir_taps.hpp"
#include "dsp_iir_config.hpp"
#include "event_m4.hpp"

#include <cstring>

// 4 KB is enough: decode.c and ldpc.c keep log174, plain174, tov and toc in static
// buffers, leaving only small locals on the stack.
WORKING_AREA(ft8_decode_thread_wa, 4096);

Thread* FT8DecodeThread::thread_ = nullptr;

FT8DecodeThread::FT8DecodeThread(ft8_decoder_state_t* state, FT8RxProcessor* processor)
    : state_{state}, processor_{processor} {
    chBSemInit(&sem_, TRUE);  // start taken
    start();
}

FT8DecodeThread::~FT8DecodeThread() {
    if (thread_) {
        terminate_.store(true, std::memory_order_release);
        chBSemSignal(&sem_);
        chThdWait(thread_);
        thread_ = nullptr;
    }
}

void FT8DecodeThread::start() {
    if (!thread_) {
        thread_ = chThdCreateStatic(
            ft8_decode_thread_wa, sizeof(ft8_decode_thread_wa),
            NORMALPRIO - 10,  // Lower than baseband (NORMALPRIO) so DMA processing wins.
            ThreadBase::fn, this);
    }
}

bool FT8DecodeThread::signal_decode() {
    // Don't double-trigger if we're still working on the previous slot.
    bool expected = false;
    if (!busy_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        return false;
    }
    chBSemSignal(&sem_);
    return true;
}

void FT8DecodeThread::run() {
    while (true) {
        chBSemWait(&sem_);
        if (terminate_.load(std::memory_order_acquire)) break;

        ft8_portapack_decode(state_);

        // Close the timing loop first: the Costas measurement is only valid for the
        // window that was just decoded.
        processor_->on_slot_decoded();

        for (int i = 0; i < state_->num_messages; i++) {
            char text[FTX_MAX_MESSAGE_LENGTH];
            ft8_portapack_message_text(&state_->messages[i], text);
            shared_memory.application_queue.push(
                FT8PacketMessage{text, state_->message_scores[i], state_->message_freqs[i]});
        }

        // One update per slot, so the UI can show whether the slot clock is locked.
        FT8RxStatusMessage status{
            (FT8RxStatusMessage::SyncState)processor_->sync_state_code(),
            (uint8_t)(state_->num_messages > 255 ? 255 : state_->num_messages)};
        shared_memory.application_queue.push(status);

        // Release the waterfall so the baseband thread can start the next slot.
        ft8_portapack_reset_slot(state_);

        busy_.store(false, std::memory_order_release);
    }
}

FT8RxProcessor::FT8RxProcessor() {
    decim_0.configure(taps_11k0_decim_0.taps);
    decim_1.configure(taps_11k0_decim_1.taps);
    channel_filter.configure(taps_11k0_channel.taps, 2);

    audio_output.configure(false);

    ft8_portapack_init(&decoder_state);

    configured = true;
}

FT8RxProcessor::~FT8RxProcessor() {
    ft8_portapack_free(&decoder_state);
}

void FT8RxProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
    const auto channel_out = channel_filter.execute(decim_1_out, dst_buffer);
    feed_channel_stats(channel_out);
    auto audio = demod_ssb.execute(channel_out, audio_buffer);

    // Decode ahead of the headphone gain stage, so clipping cannot reach the decoder.
    process_ft8_audio(audio);

    for (size_t i = 0; i < audio.count; i++) {
        float sample = audio.p[i] * 2.0f;
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        audio.p[i] = sample;
    }
    audio_output.write(audio);
}

void FT8RxProcessor::process_ft8_audio(const buffer_f32_t& audio) {
    // 24 kHz to 12 kHz. The 11 kHz channel filter already bandlimits the input, so the
    // decimation aliases nothing back into the 200-2500 Hz FT8 band.
    static bool decimate_phase = false;

    for (size_t i = 0; i < audio.count; i++) {
        float sample = audio.p[i];

        decimate_phase = !decimate_phase;
        if (!decimate_phase) continue;

        // Tail of the slot: the decoder runs here, and any phase correction the sync
        // loop asked for is already folded into this count.
        if (skip_samples > 0) {
            skip_samples--;
            continue;
        }

        // Inf passes the NaN check, since Inf == Inf holds.
        if (sample != sample || sample > 1e15f || sample < -1e15f) sample = 0.0f;

        if (ft8_portapack_feed_sample(&decoder_state, sample)) {
            if (decode_thread.signal_decode()) {
                skip_samples = next_slot_gap();
            } else {
                // Back off by exactly one symbol so the sub-symbol phase Fine established
                // survives and Track absorbs the whole-block shift on the next decode. An
                // arbitrary retry length would move the phase by an amount the
                // block-quantised loop cannot see or undo.
                skip_samples = FT8_SAMPLES_PER_SYMBOL;
            }
        }
    }
}

uint32_t FT8RxProcessor::next_slot_gap() {
    const int32_t delta = pending_phase_adjust.exchange(0, std::memory_order_acq_rel);
    // Corrections are applied as extra waiting: consumed samples cannot be recovered, so
    // pulling the window earlier means pushing it later by the rest of a slot.
    int32_t gap = (BASE_GAP + delta) % SLOT_SAMPLES;
    if (gap < 0) gap += SLOT_SAMPLES;
    // Never shorten the tail the decoder runs in. Phase repeats every SLOT_SAMPLES, so a
    // whole extra slot restores the margin and changes nothing.
    if (gap < BASE_GAP) gap += SLOT_SAMPLES;
    return (uint32_t)gap;
}

void FT8RxProcessor::on_slot_decoded() {
    const int16_t score = decoder_state.sync_score;
    const int16_t offset = decoder_state.sync_time_offset;
    const bool have_signal = (score >= SYNC_MIN_SCORE);
    // Acquisition trusts only a decode. Tracking may use the Costas match, because once
    // the window is aligned the alias is outside the search range and cannot win.
    const bool confirmed = decoder_state.sync_confirmed;
    int32_t delta = 0;

    switch (sync_state) {
        case SyncState::Acquire:
            if (confirmed) {
                // A message came out of this window, so the alignment is the real one.
                quiet_slots = 0;
                delta = (int32_t)(offset - TARGET_OFFSET) * FT8_SAMPLES_PER_SYMBOL;
                dither_index = 0;
                dither_best = 0;
                dither_best_score = 0;
                sync_state = SyncState::Fine;
            } else if (++quiet_slots >= SWEEP_SLOTS) {
                // Nothing at this phase. Four steps cover the slot, so a cold start
                // costs at most about two minutes.
                quiet_slots = 0;
                delta = SWEEP_STEP;
            }
            break;

        case SyncState::Fine:
            // Walk one symbol in DITHER_STEPS steps, keeping the best-scoring phase. A
            // step is issued every window, but this window's score belongs to the phase
            // that landed SYNC_LATENCY_SLOTS windows ago, hence the credit to
            // dither_index - 1 and the skipped first window. A low score here does not
            // mean the band went quiet, because the walk detunes the window on purpose;
            // silence is judged once at the end from dither_best_score.
            if (dither_index >= 1 && score > dither_best_score) {
                dither_best_score = score;
                dither_best = dither_index - 1;
            }
            if (dither_index < DITHER_STEPS) {
                delta = DITHER_STEP;
                dither_index++;
            } else if (dither_best_score >= SYNC_MIN_SCORE) {
                delta = (int32_t)dither_best * DITHER_STEP;
                lock_score = dither_best_score;
                weak_slots = 0;
                quiet_slots = 0;
                settle_slots = SYNC_LATENCY_SLOTS;
                sync_state = SyncState::Track;
            } else {
                // No phase produced a usable score, so the signal that triggered
                // acquisition is gone. Locking here would set lock_score from noise.
                quiet_slots = 0;
                sync_state = SyncState::Acquire;
            }
            break;

        case SyncState::Track:
            // Wait out a correction already on its way. Judging a window it has not
            // reached yet issues the same correction twice and makes the loop ring.
            if (settle_slots > 0) {
                settle_slots--;
                break;
            }
            if (have_signal) {
                quiet_slots = 0;
                delta = (int32_t)(offset - TARGET_OFFSET) * FT8_SAMPLES_PER_SYMBOL;
                if (delta != 0) settle_slots = SYNC_LATENCY_SLOTS;
                // The block offset is corrected every slot, the sub-symbol phase is not,
                // and the free-running clock walks it out over roughly two hours. A score
                // well below what the dither search reached is the symptom.
                if (score * 8 < lock_score * 5) {
                    if (++weak_slots >= WEAK_SLOTS_LIMIT) {
                        weak_slots = 0;
                        settle_slots = 0;
                        dither_index = 0;
                        dither_best = 0;
                        dither_best_score = 0;
                        sync_state = SyncState::Fine;
                    }
                } else {
                    weak_slots = 0;
                    if (score > lock_score) lock_score = score;
                }
            } else if (++quiet_slots >= TRACK_LOST_SLOTS) {
                // Two minutes of an empty band. Search from scratch.
                quiet_slots = 0;
                weak_slots = 0;
                settle_slots = 0;
                sync_state = SyncState::Acquire;
            }
            break;
    }

    pending_phase_adjust.store(delta, std::memory_order_release);
}

void FT8RxProcessor::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::CaptureConfig:
            capture_config(*reinterpret_cast<const CaptureConfigMessage*>(message));
            break;

        default:
            break;
    }
}

void FT8RxProcessor::capture_config(const CaptureConfigMessage& message) {
    if (message.config) {
        audio_output.set_stream(std::make_unique<StreamInput>(message.config));
    } else {
        audio_output.set_stream(nullptr);
    }
}

int main() {
    audio::dma::init_audio_out();

    EventDispatcher event_dispatcher{std::make_unique<FT8RxProcessor>()};
    event_dispatcher.run();
    return 0;
}
