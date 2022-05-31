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

StreamWriter::StreamWriter(std::unique_ptr<stream::Writer> writer) : writer{std::move(writer)}
{
    thread = chThdCreateFromHeap(NULL, 512, NORMALPRIO + 10, StreamWriter::static_fn, this);
};

StreamWriter::~StreamWriter()
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

uint32_t StreamWriter::run()
{
    uint8_t *buffer_block = new uint8_t[128];

    while (!chThdShouldTerminate())
    {
        size_t block_bytes = 0;
        size_t block_bytes_written = 0;

        if (!writer)
            return NO_WRITER;

        data_exchange.setup_baseband_stream();

        // read from reader
        auto read_result = data_exchange.read(buffer_block, sizeof(*buffer_block));

        if (read_result.is_error())
            return READ_ERROR;

        if (read_result.value() == 0) // end of stream
            return END_OF_STREAM;

        block_bytes = read_result.value();

        while (block_bytes_written < block_bytes && !chThdShouldTerminate())
        {
            // write to baseband
            auto write_result = writer->write(buffer_block + block_bytes_written, block_bytes - block_bytes_written);

            if (read_result.is_error())
                return WRITE_ERROR;

            if (write_result.value() > 0)
                block_bytes_written += write_result.value();
        }
    }

    return TERMINATED;
};

msg_t StreamWriter::static_fn(void *arg)
{
    auto obj = static_cast<StreamWriter *>(arg);

    const uint32_t return_code = obj->run();

    // adapt this to the new stream reader interface
    StreamWriterDoneMessage message{return_code};
    EventDispatcher::send_message(message);

    return 0;
};
