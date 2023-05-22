/*
 * Copyright (C) 2023 Kyle Reed
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

#include <stddef.h>  // For size_t
#include <memory>

/* Implements a fixed-size, circular buffer.
 * NB: Holds Capacity - 1 items.
 * There are no bounds checks on accessors so ensure there are
 * items in the buffer before accessing front/back/operator[]. */
template <typename T, size_t Capacity>
class CircularBuffer {
   public:
    CircularBuffer() = default;

    CircularBuffer(const CircularBuffer&) = delete;
    CircularBuffer(CircularBuffer&&) = delete;
    CircularBuffer& operator=(const CircularBuffer&) = delete;
    CircularBuffer& operator=(CircularBuffer&&) = delete;

    void push_front(T val) {
        head_ = head_ > 0 ? head_ - 1 : last_index;
        if (head_ == end_)
            pop_back_internal();

        data_[head_] = std::move(val);
    }

    void pop_front() {
        if (!empty())
            pop_front_internal();
    }

    void push_back(T val) {
        data_[end_] = std::move(val);

        end_ = end_ < last_index ? end_ + 1 : 0;
        if (head_ == end_)
            pop_front_internal();
    }

    void pop_back() {
        if (!empty())
            pop_back_internal();
    }

    T& operator[](size_t ix) & {
        ix += head_;
        if (ix >= Capacity)
            ix -= Capacity;
        return data_[ix];
    }

    const T& operator[](size_t ix) const& {
        return const_cast<CircularBuffer*>(this)->operator[](ix);
    }

    const T& front() const& {
        return data_[head_];
    }

    const T& back() const& {
        auto end = end_ > 0 ? end_ - 1 : last_index;
        return data_[end];
    }

    size_t size() const& {
        auto end = end_;
        if (end < head_)
            end += Capacity;
        return end - head_;
    }

    bool empty() const {
        return head_ == end_;
    }

    void clear() {
        head_ = 0;
        end_ = 0;
    }

   private:
    void pop_front_internal() {
        head_ = head_ < last_index ? head_ + 1 : 0;
    }

    void pop_back_internal() {
        end_ = end_ > 0 ? end_ - 1 : last_index;
    }

    static constexpr size_t last_index = Capacity - 1;
    size_t head_{0};
    size_t end_{0};

    T data_[Capacity]{};
};

#endif /*__CIRCULAR_BUFFER_H__*/