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

#include "stream_writer.hpp"
#include "message.hpp"
#include "errors.hpp"

namespace stream
{
    StreamWriter::StreamWriter(IoExchange *io_exchange, std::unique_ptr<Writer> _writer) : io_exchange{io_exchange}, writer{std::move(_writer)}
    {
        thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO + 10, StreamWriter::static_fn, this);
    };

    StreamWriter::~StreamWriter()
    {
        if (io_exchange)
            io_exchange->config.application->is_ready = false;

        if (thread)
        {
            if (thread->p_state != THD_STATE_FINAL)
            {
                chThdTerminate(thread);
                chThdWait(thread);
            }

            thread = nullptr;
        }

        if (io_exchange)
        {
            io_exchange->clear();
            io_exchange = nullptr;
        }
    };

    const Error StreamWriter::run()
    {
        uint8_t *buffer_block = new uint8_t[BASE_BLOCK_SIZE];

        while (!chThdShouldTerminate())
        {
            if (!io_exchange)
                return errors::NO_IO_EXCHANGE;

            if (!writer)
                return errors::NO_WRITER;

            // read from reader
            auto read_result = io_exchange->fully_read(buffer_block, BASE_BLOCK_SIZE);

            // handle thd terminate flag
            if (chThdShouldTerminate())
                break;

            if (read_result.is_error())
                return read_result.error();

            if (read_result.value() == 0) // end of stream
                return errors::END_OF_STREAM;

            // write to writer
            auto write_result = writer->fully_write(buffer_block, read_result.value());

            if (write_result.is_error())
                return write_result.error();

            // we're going to loop, no need to handle thd terminate flag
        }

        return errors::THREAD_TERMINATED;
    };

    msg_t StreamWriter::static_fn(void *arg)
    {
        auto obj = static_cast<StreamWriter *>(arg);
        obj->io_exchange->config.application->is_ready = true;
        const Error error = obj->run();
        obj->io_exchange->config.application->is_ready = false;

        // adapt this to the new stream reader interface
        StreamWriterDoneMessage message{error};
        EventDispatcher::send_message(message);

        return 0;
    };

} /* namespace stream */