/*
 * Copyright (C) 2026 Belousov Oleg aka R1CBU
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

// DMR voice receiver: PHY processing on the RX thread, codec on the event loop.

#include "baseband_processor.hpp"
#include "dmr/rx_thread.hpp"
#include "dmr/voice_output.hpp"
#include "dmr/receiver.h"
#include "dsp_decimate.hpp"
#include "dsp_fir_taps.hpp"
#include "event_m4.hpp"
#include "portapack_shared_memory.hpp"
#include <atomic>

namespace {

constexpr uint32_t sample_rate = 2457600;

// 38.4-to-19.2 kHz channel filter: passband 0..5800 Hz, stopband from 9600 Hz.

constexpr std::array<int16_t, 32> channel_taps{{-65, -202, -151, 217, 540, 131, -857, -1050, 504, 2253, 1206, -2807, -4874, 725, 13200, 23858, 23858, 13200, 725, -4874, -2807, 1206, 2253, 504, -1050, -857, 131, 540, 217, -151, -202, -65}};

class DmrVoiceProcessor : public BasebandProcessor {
   public:
    DmrVoiceProcessor() {
        decim0.configure(taps_16k0_decim_0.taps);
        decim1.configure(taps_16k0_decim_1.taps);
        channel.configure(channel_taps, 2);
        rx = mayhem_dmr_create(on_burst, this);
        baseband_thread.start();
    }

    void on_message(const Message* message) override {
        if (message->id == Message::ID::UpdateSpectrum) voice.service();
        if (message->id == Message::ID::DmrVoiceConfigure) reset_requested.store(true);
    }

    void execute(const buffer_c8_t& buffer) override {
        if (reset_requested.exchange(false)) {
            reset();
            report_samples = 0;
            settling = sample_rate / 50;
        }

        // Keep loss detection: discarded IQ must invalidate timing and audio.

        const uint32_t missed = shared_memory.m4_buffer_missed;

        if (missed != previous_missed) {
            previous_missed = missed;
            reset();
            if (settling < sample_rate / 500) settling = sample_rate / 500;
        }

        const buffer_c16_t dst{reinterpret_cast<complex16_t*>(shared_memory.bb_data.dmr_voice.iq), 256};
        const auto a = decim0.execute(buffer, dst);
        const auto b = decim1.execute(a, dst);
        const auto c = channel.execute(b, dst);

        voice.advance(buffer.count);
        age = age < sample_rate ? age + buffer.count : sample_rate;

        if (settling) {
            settling = settling > buffer.count ? settling - buffer.count : 0;
        } else if (rx) {
            for (size_t i = 0; i < c.count; ++i)
                mayhem_dmr_sample(rx, c.p[i].real(), c.p[i].imag());
        }

        report_samples += buffer.count;
        const bool audio_active = voice.audio_active();

        if (report_samples >= sample_rate / 2 || audio_active != last_audio_active) {
            if (report_samples >= sample_rate / 2) report_samples -= sample_rate / 2;

            last_audio_active = audio_active;
            DmrVoiceStatusMessage status;
            status.ready = rx != nullptr;
            status.recent = age < sample_rate / 2;

            if (!status.recent) color_code = -1;

            status.color_code = color_code;
            status.kind = kind;
            status.ids_valid = voice.call.valid && status.recent;
            status.source_id = voice.call.source;
            status.destination_id = voice.call.destination;
            status.private_call = voice.call.private_call;
            status.audio_active = audio_active;
            shared_memory.application_queue.push(status);
        }
    }

   private:
    void reset() {
        if (rx) mayhem_dmr_reset(rx);

        voice.reset();
        age = sample_rate;
        color_code = -1;
        kind = 0;
    }

    static void on_burst(void* context, const dmr_burst_t* burst) {
        auto& self = *static_cast<DmrVoiceProcessor*>(context);

        if (burst->has_sync) self.kind = burst->sync_kind;

        const int cc = mayhem_dmr_color_code(burst);

        if (cc >= 0) self.color_code = cc;
        self.age = 0;
        self.voice.burst(*burst);
    }

    dmr_phy_rx* rx = nullptr;  // Image-owned allocation, reclaimed when M4 RAM is reloaded.
    std::atomic<bool> reset_requested{true};
    int16_t color_code = -1;
    uint8_t kind = 0;
    bool last_audio_active = false;
    uint32_t previous_missed = 0;
    uint32_t report_samples = 0;
    uint32_t age = sample_rate;
    uint32_t settling = 0;
    DmrVoiceOutput voice{};
    dsp::decimate::FIRC8xR16x24FS4Decim8 decim0{};
    dsp::decimate::FIRC16xR16x32Decim8 decim1{};
    dsp::decimate::FIRAndDecimateComplex channel{};
    DmrRxThread baseband_thread{sample_rate, this};
};

static_assert(sizeof(DmrVoiceProcessor) <= 704, "Update DMR Voice linker heap budget");

}  // namespace

int main() {
    EventDispatcher dispatcher{std::make_unique<DmrVoiceProcessor>()};

    dispatcher.run();
    return 0;
}
