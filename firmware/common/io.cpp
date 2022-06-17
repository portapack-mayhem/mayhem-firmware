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

#include "io.hpp"
#include "ch.h"

namespace stream
{

    Result<size_t> Reader::fully_read(void *p, const size_t count)
    {
        return fully_read_up_to_max_iterations(p, count, 10);
    };

    Result<size_t> Reader::fully_read_up_to_max_iterations(void *p, const size_t count, const size_t max_zero_iterations)
    {
        uint8_t *b = static_cast<uint8_t *>(p);
        size_t inner_bytes_read = 0;
        size_t zero_iterations = 0;

        while (
            !chThdShouldTerminate() &&
            zero_iterations < max_zero_iterations &&
            inner_bytes_read < count)
        {
            auto read_result = read(&b[inner_bytes_read], count - inner_bytes_read);

            if (read_result.is_error())
                return read_result; // propagate error

            inner_bytes_read += read_result.value();

            if (read_result.value() == 0)
                zero_iterations++;
            else
                zero_iterations = 0;
        }

        return {inner_bytes_read};
    };

    Result<size_t> Writer::fully_write(const void *p, const size_t count)
    {
        return fully_write_up_to_max_iterations(p, count, 10);
    };

    Result<size_t> Writer::fully_write_up_to_max_iterations(const void *p, const size_t count, const size_t max_zero_iterations)
    {
        const uint8_t *b = static_cast<const uint8_t *>(p);

        size_t inner_bytes_written = 0;
        size_t zero_iterations = 0;

        while (
            !chThdShouldTerminate() &&
            zero_iterations < max_zero_iterations &&
            inner_bytes_written < count)
        {
            auto write_result = write(&b[inner_bytes_written], count - inner_bytes_written);

            if (write_result.is_error())
                return write_result; // propagate error

            inner_bytes_written += write_result.value();

            if (write_result.value() == 0)
                zero_iterations++;
            else
                zero_iterations = 0;
        }

        return {inner_bytes_written};
    }

} /* namespace stream */
