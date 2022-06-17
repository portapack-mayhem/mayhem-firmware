/*
 * Copyright (C) 2022 José Moreira @cusspvz
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

#include "io_exchange.hpp"

namespace stream
{

    IoExchangeConfig::IoExchangeConfig(const IoExchangeDirection direction, void *const buffer, const size_t buffer_size) : direction{direction} {
        baseband = {
            .buffer = nullptr,
            .bytes_read = 0,
            .bytes_written = 0,
            .is_configured = false,
            .is_ready = false,
        };

        application = {
            .buffer = nullptr,
            .bytes_read = 0,
            .bytes_written = 0,
            .is_configured = false,
            .is_ready = false,
        };

        uint8_t *rbuffer = static_cast<uint8_t *>(buffer);

        if (direction == stream::DUPLEX)
        {
            const size_t half_buffer_size = buffer_size / 2;
            application.buffer = new CircularBuffer(&rbuffer[0], half_buffer_size);
            baseband.buffer = new CircularBuffer(&rbuffer[half_buffer_size], half_buffer_size);
        }

        if (direction == stream::APP_TO_BB)
            application.buffer = new CircularBuffer(&rbuffer[0], buffer_size);

        if (direction == stream::BB_TO_APP)
            baseband.buffer = new CircularBuffer(&rbuffer[0], buffer_size);

        reset();
    };

    void IoExchangeConfig::reset() {
        // reset the buckets
        application.bytes_read = 0;
        application.bytes_written = 0;
        baseband.bytes_read = 0;
        baseband.bytes_written = 0;

        if (application.buffer != nullptr){
            application.buffer->reset();
            application.buffer->clear_data();
        }

        if (baseband.buffer != nullptr){
            application.buffer->reset();
            baseband.buffer->clear_data();
        }
    };

}