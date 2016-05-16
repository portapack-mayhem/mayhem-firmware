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
#include <cstring>

#define MSGPACK_NIL			0xC0

#define MSGPACK_FALSE		0xC2
#define MSGPACK_TRUE		0xC3

#define MSGPACK_TYPE_F32	0xCA
#define MSGPACK_TYPE_F64	0xCB

#define MSGPACK_TYPE_U8		0xCC
#define MSGPACK_TYPE_U16	0xCD
#define MSGPACK_TYPE_U32	0xCE
#define MSGPACK_TYPE_U64	0xCF

#define MSGPACK_TYPE_S8		0xD0
#define MSGPACK_TYPE_S16	0xD1
#define MSGPACK_TYPE_S32	0xD2
#define MSGPACK_TYPE_S64	0xD3

#define MSGPACK_TYPE_STR8	0xD9
#define MSGPACK_TYPE_STR16	0xDA
#define MSGPACK_TYPE_STR32	0xDB

#define MSGPACK_TYPE_ARR16	0xDC
#define MSGPACK_TYPE_ARR32	0xDD

#define MSGPACK_TYPE_MAP16	0xDE
#define MSGPACK_TYPE_MAP32	0xDF

class MsgPack {
public:

	enum RecID {
		TestListA = 0,
		TestListB = 1,
		TestListC = 2,
		TestListD = 3,
		TestListE = 4
	};

	// Read
	bool msgpack_get(const void * buffer, const size_t size, const RecID record_id, bool * value);
	bool msgpack_get(const void * buffer, const size_t size, const RecID record_id, uint8_t * value);
	bool msgpack_get(const void * buffer, const size_t size, const RecID record_id, int64_t * value);
	bool msgpack_get(const void * buffer, const size_t size, const RecID record_id, std::string& value);
	
	// Write
	void msgpack_init(const void * buffer, size_t * ptr);
	void msgpack_add(const void * buffer, size_t * ptr, const RecID record_id, bool value);
	void msgpack_add(const void * buffer, size_t * ptr, const RecID record_id, uint8_t value);
	void msgpack_add(const void * buffer, size_t * ptr, const RecID record_id, int64_t value);
	bool msgpack_add(const void * buffer, size_t * ptr, const RecID record_id, std::string value);

private:
	bool get_raw_byte(const void * buffer, const bool inc, uint8_t * byte);
	bool get_raw_word(const void * buffer, const bool inc, uint16_t * word);
	bool get_bool(const void * buffer, const bool inc, bool * value);
	bool get_u8(const void * buffer, const bool inc, uint8_t * value);
	bool get_u16(const void * buffer, const bool inc, uint16_t * value);
	bool get_s32(const void * buffer, const bool inc, int32_t * value);
	bool get_string(const void * buffer, const bool inc, std::string& value);
	
	void add_key(const void * buffer, size_t * ptr, const RecID record_id);
	
	bool init_search(const void * buffer, const size_t size);
	bool search_key(const void * buffer, const RecID record_id);
	bool skip(const void * buffer);
	
	size_t seek_ptr = 0;
	size_t buffer_size;
};

#endif
