/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __SIGNAL_H__
#define __SIGNAL_H__

#include <cstdint>
#include <list>
#include <functional>
#include <memory>

#include "utility.hpp"

/* Tweaked and vastly simplified implementation of Simple::Signal, from
 * https://testbit.eu/cpp11-signal-system-performance/
 *
 * Original license:
 *   CC0 Public Domain
 *   http://creativecommons.org/publicdomain/zero/1.0/
 */

using SignalToken = uint32_t;

template<class... Args>
struct Signal {
	using Callback = std::function<void (Args...)>;

	SignalToken operator+=(const Callback& callback) {
		const SignalToken token = next_token++;
		entries.emplace_back(std::make_unique<CallbackEntry>(callback, token));
		return token;
	}

	bool operator-=(const SignalToken token) {
		entries.remove_if([token](EntryType& entry) {
			return entry.get()->token == token;
		});
		return true;
	}

	void emit(Args... args) {
		for(auto& entry : entries) {
			entry.get()->callback(args...);
		};
	}

private:
	struct CallbackEntry {
		const Callback callback;
		const SignalToken token;

		constexpr CallbackEntry(
			const Callback& callback,
			const SignalToken token
		) : callback { callback },
			token { token }
		{
		}
	};

	using EntryType = std::unique_ptr<CallbackEntry>;
	
	std::list<EntryType> entries { };
	SignalToken next_token = 1;
};

#endif/*__SIGNAL_H__*/
