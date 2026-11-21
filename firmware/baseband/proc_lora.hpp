/*
 * LoRa RX baseband (BW 62.5 kHz, SF8, explicit header) for the "MeshCore RX" external app.
 *
 * 2 MS/s c8 -> FIR + fs/4 shift, /8 -> 250 kS/s -> channel filter /2 -> 125 kS/s (2 samples/chip)
 * -> sample ring (DMA thread) -> lora::Receiver state machine (worker thread).
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

#ifndef __PROC_LORA_H__
#define __PROC_LORA_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"
#include "dsp_decimate.hpp"
#include "filtered_spectrum_collector.hpp"
#include "message.hpp"

#include "lora_core.hpp"
#include "thread_base.hpp"

#include <atomic>

#include <array>
#include <cstdint>

class LoraProcessor;

// Runs the dechirp / FFT state machine off the DMA thread: one window takes longer than the
// DMA ring can absorb, so doing it inline loses transfers (and the symbol timing with them).
class LoraWorker : public ThreadBase {
   public:
    explicit LoraWorker(LoraProcessor* p)
        : processor_{p} {}
    ~LoraWorker();
    void start() override;
    void signal() { chBSemSignal(&sem_); }

   private:
    void run() override;
    LoraProcessor* processor_;
    BinarySemaphore sem_{};
    std::atomic<bool> terminate_{false};
    static Thread* thread_;
};

class LoraProcessor : public BasebandProcessor {
   public:
    LoraProcessor();
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    static constexpr size_t baseband_fs = 2000000;

    std::array<complex16_t, 512> dst{};
    const buffer_c16_t dst_buffer{dst.data(), dst.size()};

    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};
    dsp::decimate::FIRAndDecimateComplex channel_filter{};

    lora::Receiver rx{};

    // channel waterfall: 256-point spectrum of the 62.5 kHz LoRa channel (125 kS/s / 2), 30 per second
    static constexpr size_t channel_fs = 125000;
    static constexpr size_t spectrum_interval_samples = channel_fs / 30;
    FilteredSpectrumCollector channel_spectrum{};
    size_t spectrum_samples{0};
    bool spectrum_capture_active{false};
    uint32_t agc_clipped{0}, agc_buffers{0}, agc_holdoff{0}, agc_loud{0};
    uint16_t agc_clip_history{0};
    bool was_busy{false};  // one bit per 1 ms DMA buffer: ADC clipped in it
    int agc_peak{0};

    static void on_packet(const lora::Packet& p, void* ctx);
    void work();  // worker thread body (one wake-up)
    friend class LoraWorker;

    LoraWorker worker{this};

    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive, /*auto_start*/ false};
    RSSIThread rssi_thread{};
};

#endif
