/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __DIRTY_REGISTERS_H__
#define __DIRTY_REGISTERS_H__

#include <cstddef>
#include <bitset>

#include "utility.hpp"

template<typename RegisterType, size_t RegisterCount>
class DirtyRegisters {
public:
	using mask_t = std::bitset<RegisterCount>;

	/* TODO: I feel like I might regret implementing this cast operator... */
	operator bool() const {
		return mask.any();
	}

	void set() {
		mask.set();
	}

	void clear() {
		mask.reset();
	}

	void clear(const size_t reg_num) {
		mask.reset(reg_num);
	}

	typename mask_t::reference operator[](const size_t reg_num) {
		return mask[reg_num];
	}

	typename mask_t::reference operator[](const RegisterType reg) {
		return mask[toUType(reg)];
	}

private:
	mask_t mask { };
};

#endif/*__DIRTY_REGISTERS_H__*/
