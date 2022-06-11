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

#include <cstring>
#include "io.hpp"
#include "error.hpp"
#include "event_m0.hpp"
#include "io_exchange.hpp"

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

namespace stream
{

    class StreamReader
    {
    public:
        StreamReader(IoExchange *io_exchange, std::unique_ptr<Reader> reader);
        ~StreamReader();

        StreamReader(const StreamReader &) = delete;
        StreamReader(StreamReader &&) = delete;
        StreamReader &operator=(const StreamReader &) = delete;
        StreamReader &operator=(StreamReader &&) = delete;

        inline static constexpr Error END_OF_STREAM{0, "End of stream"};
        inline static constexpr Error NO_READER{1, "No reader"};
        inline static constexpr Error READ_ERROR{2, "Read error"};
        inline static constexpr Error WRITE_ERROR{3, "Write error"};
        inline static constexpr Error TERMINATED{4, "Terminated"};

    private:
        IoExchange *io_exchange;
        std::unique_ptr<Reader> reader{nullptr};
        Thread *thread{nullptr};

        static msg_t static_fn(void *arg);
        const Error run();
    };

} /* namespace stream */