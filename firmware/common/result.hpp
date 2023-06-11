/*
 * Copyright (C) 2023 Kyle Reed
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include <utility>

template <typename TValue, typename TError>
struct Result {
    enum class Type {
        Success,
        Error,
    } type;

    union {
        TValue value_;
        TError error_;
    };

    bool is_ok() const {
        return type == Type::Success;
    }

    operator bool() const {
        return is_ok();
    }

    bool is_error() const {
        return type == Type::Error;
    }

    const TValue& value() const& {
        return value_;
    }

    const TValue& operator*() const& {
        return value_;
    }

    TValue&& operator*() && {
        return std::move(value_);
    }

    const TError& error() const& {
        return error_;
    }

    Result() = delete;

    constexpr Result(TValue&& value)
        : type{Type::Success},
          value_{std::forward<TValue>(value)} {
    }

    constexpr Result(TError error)
        : type{Type::Error},
          error_{error} {
    }

    ~Result() {
        if (is_ok())
            value_.~TValue();
        else
            error_.~TError();
    }
};