/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include "audio_dma.hpp"

#include <cstdint>
#include <cstddef>
#include <array>
#include <cstring>

#include "hal.h"
#include "gpdma.hpp"
#include "tone_gen.hpp"

using namespace lpc43xx;

#include "portapack_dma.hpp"

namespace audio {
namespace dma {

ToneGen tone_gen{};

constexpr uint32_t gpdma_ahb_master_peripheral = 1;
constexpr uint32_t gpdma_ahb_master_memory = 0;
constexpr uint32_t gpdma_ahb_master_lli_fetch = 0;

constexpr uint32_t gpdma_rx_peripheral = 0x9; /* I2S0 DMA request 1 */
constexpr uint32_t gpdma_rx_src_peripheral = gpdma_rx_peripheral;
constexpr uint32_t gpdma_rx_dest_peripheral = gpdma_rx_peripheral;

constexpr uint32_t gpdma_tx_peripheral = 0xa; /* I2S0 DMA request 2 */
constexpr uint32_t gpdma_tx_src_peripheral = gpdma_tx_peripheral;
constexpr uint32_t gpdma_tx_dest_peripheral = gpdma_tx_peripheral;

constexpr gpdma::channel::LLIPointer lli_pointer(const void* lli) {
    return {
        .lm = gpdma_ahb_master_lli_fetch,
        .r = 0,
        .lli = reinterpret_cast<uint32_t>(lli),
    };
}

constexpr gpdma::channel::Control control_tx(const size_t transfer_bytes) {
    return {
        .transfersize = gpdma::buffer_words(transfer_bytes, 4),
        .sbsize = 4, /* Burst size: 32 */
        .dbsize = 4, /* Burst size: 32 */
        .swidth = 2, /* Source transfer width: word (32 bits) */
        .dwidth = 2, /* Destination transfer width: word (32 bits) */
        .s = gpdma_ahb_master_memory,
        .d = gpdma_ahb_master_peripheral,
        .si = 1,
        .di = 0,
        .prot1 = 0,
        .prot2 = 0,
        .prot3 = 0,
        .i = 1,
    };
}

constexpr gpdma::channel::Config config_tx() {
    return {
        .e = 0,
        .srcperipheral = gpdma_tx_src_peripheral,
        .destperipheral = gpdma_tx_dest_peripheral,
        .flowcntrl = gpdma::FlowControl::MemoryToPeripheral_DMAControl,
        .ie = 1,
        .itc = 1,
        .l = 0,
        .a = 0,
        .h = 0,
    };
}

constexpr gpdma::channel::Control control_rx(const size_t transfer_bytes) {
    return {
        .transfersize = gpdma::buffer_words(transfer_bytes, 4),
        .sbsize = 4, /* Burst size: 32 */
        .dbsize = 4, /* Burst size: 32 */
        .swidth = 2, /* Source transfer width: word (32 bits) */
        .dwidth = 2, /* Destination transfer width: word (32 bits) */
        .s = gpdma_ahb_master_peripheral,
        .d = gpdma_ahb_master_memory,
        .si = 0,
        .di = 1,
        .prot1 = 0,
        .prot2 = 0,
        .prot3 = 0,
        .i = 1,
    };
}

constexpr gpdma::channel::Config config_rx() {
    return {
        .e = 0,
        .srcperipheral = gpdma_rx_src_peripheral,
        .destperipheral = gpdma_rx_dest_peripheral,
        .flowcntrl = gpdma::FlowControl::PeripheralToMemory_DMAControl,
        .ie = 1,
        .itc = 1,
        .l = 0,
        .a = 0,
        .h = 0,
    };
}

/* TODO: Clean up terminology around "buffer", "transfer", "samples" */

constexpr size_t buffer_samples_log2n = 7;
constexpr size_t buffer_samples = (1 << buffer_samples_log2n);  // 2^7 = 128 byte circular DMA buffer
constexpr size_t transfers_per_buffer_log2n = 2;
constexpr size_t transfers_per_buffer = (1 << transfers_per_buffer_log2n);  // 2^2 = 4 transfer buffers in the circular buffer
constexpr size_t transfer_samples = buffer_samples / transfers_per_buffer;  // 128/4 = 32 samples in each transfer buffer
constexpr size_t transfers_mask = transfers_per_buffer - 1;

constexpr size_t buffer_bytes = buffer_samples * sizeof(sample_t);
constexpr size_t transfer_bytes = transfer_samples * sizeof(sample_t);

static std::array<sample_t, buffer_samples> buffer_tx;
static std::array<sample_t, buffer_samples> buffer_rx;

static std::array<gpdma::channel::LLI, transfers_per_buffer> lli_tx_loop;
static std::array<gpdma::channel::LLI, transfers_per_buffer> lli_rx_loop;

static constexpr auto& gpdma_channel_i2s0_tx = gpdma::channels[portapack::i2s0_tx_gpdma_channel_number];
static constexpr auto& gpdma_channel_i2s0_rx = gpdma::channels[portapack::i2s0_rx_gpdma_channel_number];

static volatile const gpdma::channel::LLI* tx_next_lli = nullptr;
static volatile const gpdma::channel::LLI* rx_next_lli = nullptr;

static bool single_tx_buffer = false;
static uint32_t beep_duration_downcounter = 0;

static void tx_transfer_complete() {
    tx_next_lli = gpdma_channel_i2s0_tx.next_lli();

    if (beep_duration_downcounter != 0)
        if (--beep_duration_downcounter == 0)
            beep_stop();
}

static void tx_error() {
    disable();
}

static void rx_transfer_complete() {
    rx_next_lli = gpdma_channel_i2s0_rx.next_lli();
}

static void rx_error() {
    disable();
}

static void configure_tx() {
    const auto peripheral = reinterpret_cast<uint32_t>(&LPC_I2S0->TXFIFO);
    const auto control_value = control_tx(transfer_bytes);
    for (size_t i = 0; i < lli_tx_loop.size(); i++) {
        const auto memory = reinterpret_cast<uint32_t>(&buffer_tx[i * transfer_samples]);
        lli_tx_loop[i].srcaddr = memory;
        lli_tx_loop[i].destaddr = peripheral;
        lli_tx_loop[i].lli = lli_pointer(&lli_tx_loop[(i + 1) % lli_tx_loop.size()]);
        lli_tx_loop[i].control = control_value;
    }
}

static void configure_rx() {
    const auto peripheral = reinterpret_cast<uint32_t>(&LPC_I2S0->RXFIFO);
    const auto control_value = control_rx(transfer_bytes);
    for (size_t i = 0; i < lli_rx_loop.size(); i++) {
        const auto memory = reinterpret_cast<uint32_t>(&buffer_rx[i * transfer_samples]);
        lli_rx_loop[i].srcaddr = peripheral;
        lli_rx_loop[i].destaddr = memory;
        lli_rx_loop[i].lli = lli_pointer(&lli_rx_loop[(i + 1) % lli_rx_loop.size()]);
        lli_rx_loop[i].control = control_value;
    }
}

static void enable_tx() {
    const auto gpdma_config_tx = config_tx();
    gpdma_channel_i2s0_tx.configure(lli_tx_loop[0], gpdma_config_tx);
    gpdma_channel_i2s0_tx.enable();
}

static void enable_rx() {
    const auto gpdma_config_rx = config_rx();
    gpdma_channel_i2s0_rx.configure(lli_rx_loop[0], gpdma_config_rx);
    gpdma_channel_i2s0_rx.enable();
}

void init_audio_out() {
    gpdma_channel_i2s0_tx.set_handlers(tx_transfer_complete, tx_error);
    // LPC_GPDMA->SYNC |= (1 << gpdma_tx_peripheral);
    configure_tx();
    enable_tx();
}

void init_audio_in() {
    gpdma_channel_i2s0_rx.set_handlers(rx_transfer_complete, rx_error);
    // LPC_GPDMA->SYNC |= (1 << gpdma_rx_peripheral);
    configure_rx();
    enable_rx();
}

void disable() {
    gpdma_channel_i2s0_tx.disable();
    gpdma_channel_i2s0_rx.disable();
}

void shrink_tx_buffer(bool shrink) {
    single_tx_buffer = shrink;

    if (transfers_per_buffer == 1)
        return;

    if (single_tx_buffer)
        lli_tx_loop[0].lli = lli_pointer(&lli_tx_loop[0]);
    else
        lli_tx_loop[0].lli = lli_pointer(&lli_tx_loop[1]);
}

void beep_start(uint32_t freq, uint32_t sample_rate, uint32_t beep_duration_ms) {
    // Prevent divide-by-0
    if (freq == 0 || sample_rate == 0)
        return;

    // Fill entire buffer with sine waves
    tone_gen.configure_beep(freq, sample_rate);
    for (size_t i = 0; i < buffer_samples; i++)
        buffer_tx[i].left = buffer_tx[i].right = tone_gen.process_beep();

    // Try to adjust DMA transfer count to align with full sine waves for a better tone
    float samples_per_sine_wave = float(sample_rate) / freq;
    uint32_t sine_waves_per_buffer = buffer_samples / samples_per_sine_wave;
    size_t sample_count = (sine_waves_per_buffer == 0) ? buffer_samples : sine_waves_per_buffer * samples_per_sine_wave + 0.5;

    // Use single larger transfer buffer with sample count determined above
    lli_tx_loop[0].lli = lli_pointer(&lli_tx_loop[0]);
    lli_tx_loop[0].control = control_tx(sample_count * sizeof(sample_t));

    // Convert duration ms to number of buffers to send before stopping
    // NB: beep_duration_ms==0 means beep continuously until stopped
    uint32_t beep_interrupt_count = beep_duration_ms * sample_rate / (1000 * sample_count);
    if ((beep_duration_ms != 0) && (beep_interrupt_count == 0))
        beep_interrupt_count = 1;
    beep_duration_downcounter = beep_interrupt_count;
}

void beep_stop() {
    // Clear audio DMA buffer
    memset(&buffer_tx, 0, buffer_bytes);

    // Restore DMA linked list to use multiple smaller buffers
    lli_tx_loop[0].control = control_tx(transfer_bytes);
    if (!single_tx_buffer && (transfers_per_buffer > 1)) {
        lli_tx_loop[0].lli = lli_pointer(&lli_tx_loop[1]);
    }
}

buffer_t tx_empty_buffer() {
    const auto next_lli = tx_next_lli;
    if (next_lli) {
        const size_t next_index = next_lli - &lli_tx_loop[0];
        const size_t free_index = (single_tx_buffer) ? 0 : (next_index + transfers_per_buffer - 2) & transfers_mask;
        return {reinterpret_cast<sample_t*>(lli_tx_loop[free_index].srcaddr), transfer_samples};
    } else {
        return {nullptr, 0};
    }
}

buffer_t rx_empty_buffer() {
    const auto next_lli = rx_next_lli;
    if (next_lli) {
        const size_t next_index = next_lli - &lli_rx_loop[0];
        const size_t free_index = (next_index + transfers_per_buffer - 2) & transfers_mask;
        return {reinterpret_cast<sample_t*>(lli_rx_loop[free_index].destaddr), transfer_samples};
    } else {
        return {nullptr, 0};
    }
}

} /* namespace dma */
} /* namespace audio */
