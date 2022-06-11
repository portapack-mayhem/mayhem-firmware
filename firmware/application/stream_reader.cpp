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

#include "stream_reader.hpp"
#include "message.hpp"
#include "errors.hpp"

namespace stream
{

    StreamReader::StreamReader(IoExchange *io_exchange, std::unique_ptr<Reader> _reader) : io_exchange{io_exchange}, reader{std::move(_reader)}
    {
        thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO + 10, StreamReader::static_fn, this);
    };

    StreamReader::~StreamReader()
    {
        if (thread)
        {
            if (thread->p_state != THD_STATE_FINAL)
            {
                chThdTerminate(thread);
                chThdWait(thread);
            }

            thread = nullptr;
        }

        // // in case there's a io_exchange instance, reset the counters and data
        // if (io_exchange)
        //     io_exchange->clear();
    };

    const Error StreamReader::run()
    {
        uint8_t *buffer_block = new uint8_t[BASE_BLOCK_SIZE];

        while (!chThdShouldTerminate())
        {
            if (!reader)
                return errors::NO_READER;

            // read from reader
            auto read_result = reader->read_full(buffer_block, BASE_BLOCK_SIZE);

            // handle thd terminate flag
            if (chThdShouldTerminate())
                break;

            if (read_result.is_error())
                return read_result.error();

            if (read_result.value() == 0) // end of stream
                return errors::END_OF_STREAM;

            // write to baseband
            auto write_result = io_exchange->write_full(buffer_block, read_result.value());

            if (write_result.is_error())
                return write_result.error();

            // we're going to loop, no need to handle thd terminate flag
        }

        return errors::THREAD_TERMINATED;
    };

    msg_t StreamReader::static_fn(void *arg)
    {
        auto obj = static_cast<StreamReader *>(arg);
        obj->io_exchange->config.application->is_ready = true;
        const Error error = obj->run();
        obj->io_exchange->config.application->is_ready = false;

        // TODO: adapt this to the new stream reader interface
        StreamReaderDoneMessage message{error};
        EventDispatcher::send_message(message);

        return 0;
    };

} /* namespace stream */