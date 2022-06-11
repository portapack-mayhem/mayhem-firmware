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

#pragma once

#include "circular_buffer.hpp"

namespace stream
{

    enum IoExchangeDirection
    {
        APP_TO_BB = 1,
        BB_TO_APP = 2,
        DUPLEX = 0
    };

    struct IoExchangeBucket
    {
        CircularBuffer *buffer{nullptr};
        size_t bytes_read{0};
        size_t bytes_written{0};

        bool is_setup{false};
        bool is_ready{false};
    };

    struct IoExchangeConfig
    {
        stream::IoExchangeDirection direction{stream::DUPLEX};
        stream::IoExchangeBucket *baseband{nullptr};    // bucket_from_baseband_to_application;
        stream::IoExchangeBucket *application{nullptr}; // bucket_from_application_to_baseband;
    };

} /* namespace stream */
