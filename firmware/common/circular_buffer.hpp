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

#ifndef __CIRCULAR_BUFFER_H__
#define __CIRCULAR_BUFFER_H__

#include <cstddef>
#include <algorithm>
#include <cstring>
#include <memory>

#include <hal.h>

class CircularBuffer
{
    uint8_t *_data;
    size_t _capacity;
    size_t _write_head{0};
    size_t _read_head{0};
    size_t _size{0};

public:
    constexpr CircularBuffer(
        void *const data = nullptr,
        const size_t capacity = 0) : _data{static_cast<uint8_t *>(data)},
                                     _capacity{capacity}
    {
        reset();
    }

    void reset()
    {
        _read_head = 0;
        _write_head = 0;
        _size = 0;
    }

    void clear_data()
    {
        for (size_t i = 0; i < _capacity; i++)
            _data[i] = 0;

        __DSB();
    }

    void *data() const
    {
        return _data;
    }

    size_t capacity() const
    {
        return _capacity;
    }

    bool is_full() const
    {
        return _capacity == _size;
    }

    bool is_empty() const
    {
        return _size == 0;
    }

    size_t used() const
    {
        return _size;
    }

    size_t unused() const
    {
        return _capacity - _size;
    }

    size_t write(const void *p, const size_t count)
    {
        size_t copy_size = std::min(_capacity - _size, count);

        if (copy_size == 0)
            return 0;

        memcpy(&_data[_write_head], p, copy_size);
        _write_head += copy_size;
        _size += copy_size;

        if (_write_head == _capacity)
            _write_head = 0;

        __DSB();

        return copy_size;
    }

    size_t read(void *p, const size_t count)
    {
        size_t copy_size = std::min(_size, count);

        if (copy_size == 0)
            return 0;

        memcpy(p, &_data[_read_head], copy_size);
        _read_head += copy_size;
        _size -= copy_size;

        if (_read_head == _capacity)
            _read_head = 0;

        __DSB();

        return copy_size;
    }
};

#endif /*__CIRCULAR_BUFFER_H__*/
