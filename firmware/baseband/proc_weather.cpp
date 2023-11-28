/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "proc_weather.hpp"
#include "portapack_shared_memory.hpp"
#include "event_m4.hpp"

void WeatherProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    for (size_t i = 0; i < buffer.count; i++) {
        int8_t re = buffer.p[i].real();
        int8_t im = buffer.p[i].imag();
        uint32_t mag = ((uint32_t)re * (uint32_t)re) + ((uint32_t)im * (uint32_t)im);
        bool meashl = (mag > threshold);
        tm += mag;
        if (meashl == currentHiLow && currentDuration < 20'000'000)  // allow pass 'end' signal
        {
            if (currentDuration < UINT32_MAX) currentDuration += msperTick;
        } else {  // called on change, so send the last duration and dir.
            protoList.feed(currentHiLow, currentDuration / 2);
            currentDuration = msperTick;
            currentHiLow = meashl;
        }
    }
    cnt += buffer.count;
    if (cnt > 30'000) {
        threshold = (tm / cnt) / 2;
        cnt = 0;
        tm = 0;
        if (threshold < 50) threshold = 50;
        if (threshold > 1700) threshold = 1700;
    }
}

void WeatherProcessor::on_message(const Message* const message) {
    if (message->id == Message::ID::WeatherRxConfigure)
        configure(*reinterpret_cast<const WeatherRxConfigureMessage*>(message));
}

void WeatherProcessor::configure(const WeatherRxConfigureMessage& message) {
    (void)message;
    configured = true;
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<WeatherProcessor>()};
    event_dispatcher.run();
    return 0;
}
