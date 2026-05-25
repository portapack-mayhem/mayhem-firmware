/*
 * Copyright (C) 2026
 */
#include "meteor_lrpt_m0_configure_ipc.hpp"

#include "message.hpp"
#include "portapack_shared_memory.hpp"

namespace {
uint32_t g_m0_configure_seq_seen{0};
}  // namespace

void meteor_lrpt_rx_reset_m0_configure_state() {
    g_m0_configure_seq_seen = 0;
    shared_memory.meteor_lrpt_rx_m0_command.ack = 0;
    __DMB();
}

void meteor_lrpt_rx_poll_m0_configure(BasebandProcessor* const processor) {
    if (!processor)
        return;

    auto& cmd = shared_memory.meteor_lrpt_rx_m0_command;
    __DMB();
    if (cmd.magic != SharedMemory::MeteorLrptRxM0Command::kMagic)
        return;

    const uint32_t seq = cmd.seq;
    if (seq == 0 || seq == g_m0_configure_seq_seen)
        return;

    g_m0_configure_seq_seen = seq;
    cmd.ack = 1;
    __DMB();

    const MeteorLrptRxConfigureMessage message{cmd.flags, cmd.symbol_rate_k};
    processor->on_message(&message);
}
