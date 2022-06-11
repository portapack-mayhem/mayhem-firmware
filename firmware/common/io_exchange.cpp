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

#include "portapack_shared_memory.hpp"
#include "io_exchange.hpp"
#include "errors.hpp"

namespace stream
{

    IoExchange::IoExchange() : config{
                                   .direction = IoExchangeDirection::DUPLEX,
                                   .baseband = new IoExchangeBucket{
                                       .buffer = nullptr,
                                       .bytes_read = 0,
                                       .bytes_written = 0,
                                       .is_setup = false,
                                       .is_ready = false,
                                   },
                                   .application = new IoExchangeBucket{
                                       .buffer = nullptr,
                                       .bytes_read = 0,
                                       .bytes_written = 0,
                                       .is_setup = false,
                                       .is_ready = false,
                                   },
                               } {
                                   // do nothing
                               };

    IoExchange::IoExchange(const IoExchangeDirection direction, void *const buffer, const size_t buffer_size) : config{
                                                                                                                    .direction = direction,
                                                                                                                    .baseband = new IoExchangeBucket{
                                                                                                                        .buffer = nullptr,
                                                                                                                        .bytes_read = 0,
                                                                                                                        .bytes_written = 0,
                                                                                                                        .is_setup = false,
                                                                                                                        .is_ready = false,
                                                                                                                    },
                                                                                                                    .application = new IoExchangeBucket{
                                                                                                                        .buffer = nullptr,
                                                                                                                        .bytes_read = 0,
                                                                                                                        .bytes_written = 0,
                                                                                                                        .is_setup = false,
                                                                                                                        .is_ready = false,
                                                                                                                    },
                                                                                                                }
    {
        uint8_t *rbuffer = static_cast<uint8_t *>(buffer);

        if (direction == stream::DUPLEX)
        {
            const size_t half_buffer_size = buffer_size / 2;
            config.application->buffer = new CircularBuffer(&rbuffer[0], half_buffer_size);
            config.baseband->buffer = new CircularBuffer(&rbuffer[half_buffer_size], half_buffer_size);
        }

        if (direction == stream::APP_TO_BB)
            config.application->buffer = new CircularBuffer(&rbuffer[0], buffer_size);

        if (direction == stream::BB_TO_APP)
            config.baseband->buffer = new CircularBuffer(&rbuffer[0], buffer_size);

        if (config.application->buffer != nullptr)
            config.application->buffer->clear_data();

        if (config.baseband->buffer != nullptr)
            config.baseband->buffer->clear_data();

        initialize();
    };

    IoExchange::IoExchange(const IoExchangeConfig &config) : config{config}
    {
        initialize();
    };

    void IoExchange::initialize()
    {
        // logic that is common to both constructors

#if defined(LPC43XX_M0)
        // control ISR wakeups on the application
        last_instance = this;
#endif

#if defined(LPC43XX_M0)
        // set the application bucket as ready
        config.application->is_setup = true;

        if (!config.baseband->is_setup)
        {
            baseband::set_stream_data_exchange(config);
        }
#endif
#if defined(LPC43XX_M4)
        // set the baseband bucket as ready
        config.baseband->is_setup = true;

        if (!config.application->is_setup)
        {
            const IoExchangeMessage message{config};
            shared_memory.application_queue.push(message);
        }
#endif
    }
    IoExchange::~IoExchange()
    {
#if defined(LPC43XX_M0)
        last_instance = nullptr;

        if (isr_thread)
        {
            chThdTerminate(isr_thread);
            isr_thread = nullptr;
        }
#endif
    };

    void IoExchange::clear()
    {
        if (config.application)
        {
            config.application->bytes_read = 0;
            config.application->bytes_written = 0;
            config.application->is_ready = false;
        }

        if (config.baseband)
        {
            // reset the buckets
            config.baseband->bytes_read = 0;
            config.baseband->bytes_written = 0;
            config.baseband->is_ready = false;
        }

        // resets both buffers
        if (config.application && config.application->buffer != nullptr)
            config.application->buffer->clear_data();

        if (config.baseband && config.baseband->buffer != nullptr)
            config.baseband->buffer->clear_data();
    }

// Methods for the Application
#if defined(LPC43XX_M0)
    Result<size_t> IoExchange::read(void *p, const size_t count)
    {
        // cannot read if we're only writing to the baseband
        if (!config.baseband->is_setup || !config.baseband->buffer || config.direction == stream::APP_TO_BB)
            return {errors::READ_ERROR_CANNOT_READ_FROM_BASEBAND};

        // suspend in case the target buffer is full
        while (config.baseband->buffer->is_empty())
        {
            wait_for_isr_event();

            if (chThdShouldTerminate())
                return {errors::THREAD_TERMINATED};
        }

        auto result = config.baseband->buffer->read(p, count);
        config.application->bytes_read += result;
        return {result};
    }

    Result<size_t> IoExchange::write(const void *p, const size_t count)
    {
        // cannot write if we're only reading from the baseband
        if (!config.baseband->is_setup || !config.application->buffer || config.direction == stream::BB_TO_APP)
            return {errors::WRITE_ERROR_CANNOT_WRITE_TO_BASEBAND};

        // suspend in case the target buffer is full
        while (config.application->buffer->is_full())
        {
            wait_for_isr_event();

            if (chThdShouldTerminate())
                return {errors::THREAD_TERMINATED};
        }

        auto result = config.application->buffer->write(p, count);
        config.application->bytes_written += result;
        return {result};
    }

    void IoExchange::wait_for_isr_event()
    {
        // Put thread to sleep, woken up by M4 IRQ
        chSysLock();
        isr_thread = chThdSelf();
        chSchGoSleepTimeoutS(THD_STATE_SUSPENDED, 2000);
        chSysUnlock();
    }

    void IoExchange::wakeup_isr()
    {
        auto thread_tmp = isr_thread;
        if (thread_tmp)
        {
            isr_thread = nullptr;
            // NOTE: no need to call the chSysLock here as it is already being called
            chSchReadyI(thread_tmp);
        }
    }
#endif

// Methods for the Baseband
#if defined(LPC43XX_M4)

    Result<size_t> IoExchange::read(void *p, const size_t count)
    {
        // cannot read if we're only writing to the baseband
        if (!config.application->is_ready || !config.application->buffer || config.direction == stream::BB_TO_APP)
            return {errors::READ_ERROR_CANNOT_READ_FROM_APP};

        // if (config.application->buffer->is_empty())
        //     return {errors::TARGET_BUFFER_EMPTY};

        auto result = config.application->buffer->read(p, count);
        config.baseband->bytes_read += result;

        if (result > 0)
            creg::m4txevent::assert_event();

        return {result};
    }

    Result<size_t> IoExchange::write(const void *p, const size_t count)
    {
        // cannot write if we're only reading from the baseband
        if (!config.application->is_ready || !config.baseband->buffer || config.direction == stream::APP_TO_BB)
            return {errors::WRITE_ERROR_CANNOT_WRITE_TO_APP};

        // if (config.baseband->buffer->is_full())
        //     return {errors::TARGET_BUFFER_FULL};

        auto result = config.baseband->buffer->write(p, count);
        config.baseband->bytes_written += result;

        if (result > 0)
            creg::m4txevent::assert_event();

        return {result};
    }
#endif

} /* namespace stream */
