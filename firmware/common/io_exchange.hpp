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
#include "io_exchange_config.hpp"

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

#if defined(LPC43XX_M0)
#include "baseband_api.hpp"
#endif

namespace stream
{

    class IoExchange : public stream::Duplex

    {
    public:
        IoExchange();
        IoExchange(const IoExchangeDirection direction, void *const buffer, const size_t buffer_size);
        IoExchange(const IoExchangeConfig &config);
        ~IoExchange();

        IoExchange(const IoExchange &) = delete;
        IoExchange(IoExchange &&) = delete;
        IoExchange &operator=(const IoExchange &) = delete;
        IoExchange &operator=(IoExchange &&) = delete;

        const IoExchangeConfig config;
        void clear();

        Result<size_t>
        read(void *p, const size_t count);
        Result<size_t> write(const void *p, const size_t count);

#if defined(LPC43XX_M0)
        // isr thread suspension/resumption
        Thread *isr_thread{nullptr};
        void wait_for_isr_event();
        void wakeup_isr();

        inline static IoExchange *last_instance;
        static void handle_isr()
        {
            if (last_instance)
                last_instance->wakeup_isr();
        }
#endif

    private:
        void initialize();
    };

} /* namespace stream */
