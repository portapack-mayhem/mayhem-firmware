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
#include <utility>

template <typename T, typename E>
struct Result
{
    enum class Type
    {
        Success,
        Error,
    } type;
    union
    {
        T value_;
        E error_;
    };

    bool is_ok() const
    {
        return type == Type::Success;
    }

    bool is_error() const
    {
        return type == Type::Error;
    }

    const T &value() const
    {
        return value_;
    }

    E error() const
    {
        return error_;
    }

    Result() = delete;

    constexpr Result(
        T value) : type{Type::Success},
                   value_{value}
    {
    }

    constexpr Result(
        E error) : type{Type::Error},
                   error_{error}
    {
    }

    ~Result()
    {
        if (type == Type::Success)
        {
            value_.~T();
        }
    }
};