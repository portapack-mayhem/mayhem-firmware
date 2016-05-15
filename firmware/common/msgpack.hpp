/*
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

#ifndef __MSGPACK_H__
#define __MSGPACK_H__

#include "ui.hpp"
#include <memory>

#define MSGPACK_TYPE_U16	0xcd
#define MSGPACK_TYPE_U32	0xce
#define MSGPACK_TYPE_S32	0xd2
#define MSGPACK_TYPE_S64	0xd3
#define MSGPACK_TYPE_STR8	0xd9
#define MSGPACK_TYPE_STR16	0xda

#define MSGPACK_TYPE_MAP16	0xde

#define MSGPACK_FALSE		0xc2
#define MSGPACK_TRUE		0xc3

class MsgPack {
public:

	enum RecID {
		FrequencyList = 0,
		TestList = 1
	};

	bool msgpack_get(const void * buffer, const size_t size, const MsgPack::RecID record_id, bool * value);
	bool msgpack_get(const void * buffer, const size_t size, const RecID record_id, int32_t * value);
	bool msgpack_get(const void * buffer, const size_t size, const RecID record_id, int64_t * value);
	bool msgpack_get(const void * buffer, const size_t size, const RecID record_id, char * value);

private:
	bool get_raw_byte(const void * buffer, const bool inc, uint8_t * byte);
	bool get_raw_word(const void * buffer, const bool inc, uint16_t * word);
	bool get_bool(const void * buffer, const bool inc, bool * value);
	bool get_u16(const void * buffer, const bool inc, uint16_t * value);
	bool get_s32(const void * buffer, const bool inc, int32_t * value);
	bool get_s64(const void * buffer, const bool inc, int64_t * value);
	bool get_chars(const void * buffer, const bool inc, char * value);
	
	bool init_search(const void * buffer, const size_t size);
	bool search_key(const void * buffer, const RecID record_id);
	bool skip(const void * buffer);
	
	size_t seek_ptr = 0;
	size_t buffer_size;
};

#endif
