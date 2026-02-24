/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __OPTIONAL_H__
#define __OPTIONAL_H__

#include <utility>

template <typename T>
class Optional {
   public:
    constexpr Optional()
        : value_{}, valid_{false} {}
    constexpr Optional(const T& value)
        : value_{value}, valid_{true} {}
    constexpr Optional(T&& value)
        : value_{std::move(value)}, valid_{true} {}

    constexpr Optional& operator=(const T& value) {
        value_ = value;
        valid_ = true;
        return *this;
    }
    constexpr Optional& operator=(T&& value) {
        value_ = std::move(value);
        valid_ = true;
        return *this;
    }

    bool is_valid() const { return valid_; }
    operator bool() const { return valid_; }

    // TODO: Throw if not valid?
    T& value() & { return value_; }
    T& operator*() & { return value_; }
    const T& value() const& { return value_; }
    const T& operator*() const& { return value_; }

    T&& value() && { return std::move(value_); }
    T&& operator*() && { return std::move(value_); }

    T* operator->() { return &value_; }
    const T* operator->() const { return &value_; }

   private:
    T value_;
    bool valid_;
};

#endif /*__OPTIONAL_H__*/
