/*
 * Copyright (C) 2026 zxkmm
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

#include "proc_spec_an.hpp"

#include "event_m4.hpp"

/* Waterfall history, hosted in M4 RAM (the M0 is short on heap). Only this
 * image has one; the engine itself is shared with the wideband spectrum. */
static uint8_t waterfall_mem[spec_an::kWaterfallRows * spec_an::kPoints];

void SpecAnProcessor::execute(const buffer_c8_t& buffer) {
    engine.execute(buffer);
}

void SpecAnProcessor::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::SpecAnConfig:
            engine.configure(*reinterpret_cast<const SpecAnConfigMessage*>(message), waterfall_mem, baseband_thread);
            break;

        case Message::ID::UpdateSpectrum:
            engine.process_pending();
            break;

        default:
            break;
    }
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<SpecAnProcessor>()};
    event_dispatcher.run();
    return 0;
}
