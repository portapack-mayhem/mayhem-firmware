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

StreamReader::StreamReader(std::unique_ptr<stream::Reader> _reader) : reader{std::move(_reader)}
{
    thread = chThdCreateFromHeap(NULL, 512, NORMALPRIO + 10, StreamReader::static_fn, this);
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
};

Error StreamReader::run()
{
    uint8_t *buffer_block = new uint8_t[128];

    while (!chThdShouldTerminate())
    {
        size_t block_bytes = 0;
        size_t block_bytes_written = 0;

        if (!reader)
            return NO_READER;

        // read from reader
        auto read_result = reader->read(buffer_block, sizeof(*buffer_block));

        if (read_result.is_error())
            return READ_ERROR;

        if (read_result.value() == 0) // end of stream
            return END_OF_STREAM;

        block_bytes = read_result.value();

        while (block_bytes_written < block_bytes && !chThdShouldTerminate())
        {
            // write to baseband
            auto write_result = data_exchange.write(buffer_block + block_bytes_written, block_bytes - block_bytes_written);

            if (read_result.is_error())
                return WRITE_ERROR;

            if (write_result.value() > 0)
                block_bytes_written += write_result.value();

            if (data_exchange.buffer_from_application_to_baseband->is_full())
                data_exchange.setup_baseband_stream();
        }

        data_exchange.setup_baseband_stream();
    }

    data_exchange.teardown_baseband_stream();

    return TERMINATED;
};

msg_t StreamReader::static_fn(void *arg)
{
    auto obj = static_cast<StreamReader *>(arg);
    const auto error = obj->run();

    // TODO: adapt this to the new stream reader interface
    StreamReaderDoneMessage message{error};
    EventDispatcher::send_message(message);

    return 0;
};
