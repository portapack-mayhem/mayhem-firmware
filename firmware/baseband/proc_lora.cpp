/*
 * LoRa RX baseband, see proc_lora.hpp.
 *
 * To the application: LoraPacketMessage for each decoded packet, LoraLevelMessage for the AGC
 * (at once when the ADC clips, otherwise every 250 ms).
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

#include "proc_lora.hpp"
#include "portapack_shared_memory.hpp"
#include "event_m4.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace {
// 2 MS/s -> 250 kS/s (passband 31 kHz: -0.7 dB, aliases > 215 kHz: < -55 dB). Sum 32768.
constexpr std::array<int16_t, 24> taps_lora_decim_0{{26, 85, 198, 382, 645, 985, 1388, 1824, 2253, 2629, 2909, 3059,
                                                     3059, 2909, 2629, 2253, 1824, 1388, 985, 645, 382, 198, 85, 26}};
// 250 kS/s -> 125 kS/s channel filter: flat to 25 kHz, -0.6 dB at 31 kHz, -87 dB at 62.5 kHz. Sum 65536.
constexpr std::array<int16_t, 64> taps_lora_channel{{-1, 5, 13, 13, -8, -40, -53, -14, 71, 137, 99, -65, -253, -284, -51, 342,
                                                     581, 366, -286, -934, -965, -103, 1198, 1905, 1112, -1093, -3315, -3477, -144, 6299, 13490, 18221,
                                                     18221, 13490, 6299, -144, -3477, -3315, -1093, 1112, 1905, 1198, -103, -965, -934, -286, 366, 581,
                                                     342, -51, -284, -253, -65, 99, 137, 71, -14, -53, -40, -8, 13, 13, 5, -1}};
// Mean symbol quality (FFT peak / mean of the other bins, as computed by lora::Receiver) at
// SNR = -15, -14, ... +20 dB in the 62.5 kHz band. Simulated with this receiver's parameters
// (SF8, 2 samples/chip, band-limited noise); the ratio saturates above about +20 dB.
constexpr float quality_at_snr[36] = {2.7f, 2.9f, 3.1f, 3.5f, 3.9f, 4.3f, 4.8f, 5.4f, 5.9f, 6.6f, 7.3f, 8.2f,
                                      9.1f, 10.1f, 11.2f, 12.4f, 13.7f, 15.1f, 16.6f, 18.2f, 20.0f, 21.8f, 23.7f, 25.6f,
                                      27.9f, 29.9f, 31.9f, 34.1f, 36.2f, 38.2f, 40.3f, 41.8f, 44.3f, 45.3f, 47.9f, 48.1f};

int8_t snr_from_quality(float q) {
    if (q <= quality_at_snr[0]) return -15;
    for (int i = 1; i < 36; i++)
        if (q < quality_at_snr[i]) {
            const float f = (q - quality_at_snr[i - 1]) / (quality_at_snr[i] - quality_at_snr[i - 1]);
            return (int8_t)lroundf(i - 15 - 1 + f);
        }
    return 21;  // saturated: > 20 dB
}
}  // namespace

LoraProcessor::LoraProcessor() {
    decim_0.configure(taps_lora_decim_0);
    channel_filter.configure(taps_lora_channel, 2);
    rx.configure(8, 0x12, on_packet, this);
    channel_spectrum.set_decimation_factor(1);
    worker.start();
    baseband_thread.start();
}

// 2 KB: the deepest path is on_packet() with a ~270 byte message on the stack.
static WORKING_AREA(lora_worker_wa, 2048);
Thread* LoraWorker::thread_ = nullptr;

void LoraWorker::start() {
    chBSemInit(&sem_, TRUE);
    if (!thread_)
        thread_ = chThdCreateStatic(lora_worker_wa, sizeof(lora_worker_wa),
                                    NORMALPRIO - 10,  // below the DMA thread
                                    ThreadBase::fn, this);
}

LoraWorker::~LoraWorker() {
    if (thread_) {
        terminate_.store(true, std::memory_order_release);
        chBSemSignal(&sem_);
        chThdWait(thread_);
        thread_ = nullptr;
    }
}

void LoraWorker::run() {
    while (true) {
        chBSemWait(&sem_);
        if (terminate_.load(std::memory_order_acquire)) break;
        processor_->work();
    }
}

void LoraProcessor::execute(const buffer_c8_t& buffer) {
    // Raw ADC level for the AGC: peak and number of clipped samples (|I| or |Q| >= 124).
    // (one complex sample in 4 is enough for a level estimate: keeps this loop cheap)
    const int8_t* raw = reinterpret_cast<const int8_t*>(buffer.p);
    uint32_t clipped = 0;
    int peak = 0;
    for (size_t i = 0; i < buffer.count * 2; i += 8) {
        int a = raw[i] < 0 ? -raw[i] : raw[i];
        int b = raw[i + 1] < 0 ? -raw[i + 1] : raw[i + 1];
        int v = a > b ? a : b;
        if (v > peak) peak = v;
        if (v >= 124) clipped++;
    }
    agc_clipped += clipped;
    if (peak > agc_peak) agc_peak = peak;
    if (peak >= 40) agc_loud++;  // above about -10 dBFS
    agc_buffers++;
    if (agc_holdoff) agc_holdoff--;

    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto channel_out = channel_filter.execute(decim_0_out, dst_buffer);

    // Only count clipping caused by a strong signal IN the LoRa channel: the ADC sees the whole
    // front-end bandwidth (~2 MHz), and lowering the gain for an out-of-channel signal would only
    // deafen the receiver. In-channel level = mean |I|+|Q| after the channel filter (noise: tens;
    // a signal that clips the ADC: many thousands).
    uint32_t level = 0;
    for (size_t i = 0; i < channel_out.count; i += 4)
        level += std::abs(channel_out.p[i].real()) + std::abs(channel_out.p[i].imag());
    level = level * 4 / (channel_out.count ? channel_out.count : 1);
    agc_clip_history = (agc_clip_history << 1) | ((clipped && level > 2000) ? 1 : 0);
    spectrum_samples += channel_out.count;
    if (!spectrum_capture_active && spectrum_samples >= spectrum_interval_samples) {
        spectrum_samples -= spectrum_interval_samples;
        spectrum_capture_active = channel_spectrum.start_capture(2);  // (minimum 2) -> 62.5 kHz span
    }
    // marks at the edges of the 62.5 kHz LoRa channel
    if (spectrum_capture_active && channel_spectrum.feed(channel_out, -31250, 31250, 3000))
        spectrum_capture_active = false;

    if (rx.push(channel_out.p, channel_out.count)) worker.signal();

    // Overload = sustained in-channel clipping (12 of the last 16 ms), as during a strong packet.
    // Reported at once (then 16 ms for the new gain to settle), otherwise every 250 ms.
    const bool overload = __builtin_popcount(agc_clip_history) >= 12 && agc_holdoff == 0;
    const uint8_t loud_pct = (uint8_t)(agc_loud * 100 / agc_buffers);
    // also report at once when a packet ends (decoded or not): the app restores the gain
    const bool busy = rx.busy();
    const bool packet_ended = was_busy && !busy;
    was_busy = busy;
    if (overload || packet_ended || agc_buffers >= 244) {
        LoraLevelMessage msg;
        msg.peak = (uint8_t)agc_peak;
        msg.clipped = (uint8_t)std::min<uint32_t>(agc_clipped, 255);
        msg.busy = rx.busy();
        msg.overruns = rx.n_ovr;
        msg.preambles = rx.n_pre;
        msg.syncs = rx.n_sync;
        msg.headers = rx.n_hdr;
        msg.overload = overload;
        msg.loud_pct = loud_pct;
        shared_memory.application_queue.push(msg);
        if (overload) {
            agc_holdoff = 16;
            agc_clip_history = 0;
        }
        agc_loud = 0;
        agc_clipped = 0;
        agc_peak = 0;
        agc_buffers = 0;
    }
}

void LoraProcessor::work() {
    rx.process();
}

void LoraProcessor::on_packet(const lora::Packet& p, void*) {
    LoraPacketMessage msg;
    msg.length = p.length;
    msg.sf = 8;
    msg.cr = p.cr;
    msg.has_crc = p.has_crc;
    msg.crc_ok = p.crc_ok;
    msg.snr_db = snr_from_quality(p.q_mean);
    msg.cfo_hz = (int16_t)(p.cfo_bins * (62500.0f / 256.0f));  // 1 bin = BW / 2^SF
    memcpy(msg.data, p.data, p.length);
    shared_memory.application_queue.push(msg);
}

void LoraProcessor::on_message(const Message* const message) {
    if (message->id == Message::ID::UpdateSpectrum || message->id == Message::ID::SpectrumStreamingConfig)
        channel_spectrum.on_message(message);
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<LoraProcessor>()};
    event_dispatcher.run();
    return 0;
}
