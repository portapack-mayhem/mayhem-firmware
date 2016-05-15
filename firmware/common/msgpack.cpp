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

#include "msgpack.hpp"

#include <cstring>

bool MsgPack::get_bool(const void * buffer, const bool inc, bool * value) {
	uint8_t v;
	
	if (seek_ptr >= buffer_size) return false;	// End of buffer
	
	v = ((uint8_t*)buffer)[seek_ptr];
	if (v == MSGPACK_FALSE)
		*value = false;
	else if (v == MSGPACK_TRUE)
		*value = true;
	else
		return false;		// Not a bool
	
	if (inc) seek_ptr++;
	return true;
}

bool MsgPack::get_raw_byte(const void * buffer, const bool inc, uint8_t * byte) {
	if (seek_ptr >= buffer_size) return false;	// End of buffer
	*byte = ((uint8_t*)buffer)[seek_ptr];
	if (inc) seek_ptr++;
	return true;
}

bool MsgPack::get_raw_word(const void * buffer, const bool inc, uint16_t * word) {
	if ((seek_ptr + 1) >= buffer_size) return false;	// End of buffer
	*word = (((uint8_t*)buffer)[seek_ptr] << 8) | ((uint8_t*)buffer)[seek_ptr + 1];
	if (inc) seek_ptr += 2;
	return true;
}

bool MsgPack::get_u16(const void * buffer, const bool inc, uint16_t * value) {
	uint8_t byte;
	
	if ((seek_ptr + 1) >= buffer_size) return false;	// End of buffer
	if ((get_raw_byte(buffer, true, &byte)) && (byte != MSGPACK_TYPE_U16)) return false;		// Value isn't a u16
	*value = (((uint8_t*)buffer)[seek_ptr] << 8) | ((uint8_t*)buffer)[seek_ptr + 1];
	if (inc) seek_ptr += 2;
	return true;
}

bool MsgPack::get_s32(const void * buffer, const bool inc, int32_t * value) {
	uint8_t byte;
	
	if ((seek_ptr + 3) >= buffer_size) return false;	// End of buffer
	if ((get_raw_byte(buffer, true, &byte)) && (byte != MSGPACK_TYPE_S32)) return false;		// Value isn't a s32
	*value = (((uint8_t*)buffer)[seek_ptr] << 24) | (((uint8_t*)buffer)[seek_ptr + 1] << 16) |
				(((uint8_t*)buffer)[seek_ptr + 2] << 8) | ((uint8_t*)buffer)[seek_ptr + 3];
	if (inc) seek_ptr += 4;
	return true;
}

bool MsgPack::get_s64(const void * buffer, const bool inc, int64_t * value) {
	uint8_t byte;
	
	if ((seek_ptr + 3) >= buffer_size) return false;	// End of buffer
	if ((get_raw_byte(buffer, true, &byte)) && (byte != MSGPACK_TYPE_S64)) return false;		// Value isn't a s64
	*value = ((int64_t)((uint8_t*)buffer)[seek_ptr] << 56) | ((int64_t)((uint8_t*)buffer)[seek_ptr + 1] << 48) |
				((int64_t)((uint8_t*)buffer)[seek_ptr + 2] << 40) | ((int64_t)((uint8_t*)buffer)[seek_ptr + 3] << 32) |
				(((uint8_t*)buffer)[seek_ptr + 4] << 24) | (((uint8_t*)buffer)[seek_ptr + 5] << 16) |
				(((uint8_t*)buffer)[seek_ptr + 6] << 8) | ((uint8_t*)buffer)[seek_ptr + 7];
	if (inc) seek_ptr += 8;
	return true;
}

bool MsgPack::get_chars(const void * buffer, const bool inc, char * value) {
	size_t length;
	uint8_t byte;
	
	// Todo: Set max length !
	if ((seek_ptr + 3) >= buffer_size) return false;	// End of buffer
	if ((get_raw_byte(buffer, true, &byte)) && (byte != MSGPACK_TYPE_STR8)
			&& (byte != MSGPACK_TYPE_STR16)) return false;		// Value isn't a str8 or str16

	if (byte == MSGPACK_TYPE_STR8) {
		if (!get_raw_byte(buffer, true, (uint8_t*)&length)) return false;		// Couldn't get str8 length
	} else {
		if (!get_raw_word(buffer, true, (uint16_t*)&length)) return false;		// Couldn't get str16 length
	}
	
	memcpy(value, ((uint8_t*)buffer), length);

	if (inc) seek_ptr += length;
	return true;
}

bool MsgPack::init_search(const void * buffer, const size_t size) {
	uint8_t byte;
	uint16_t map_size;	// Unused for now
	
	if (!size) return false;
	buffer_size = size;
	seek_ptr = 0;
	if ((get_raw_byte(buffer, true, &byte)) && (byte != MSGPACK_TYPE_MAP16)) return false;		// First record isn't a map16
	if (!get_raw_word(buffer, true, &map_size)) return false;		// Couldn't get map16 size
	
	return true;
}

bool MsgPack::skip(const void * buffer) {
	uint8_t byte;
	size_t length;
	
	if (!get_raw_byte(buffer, true, &byte)) return false;		// Couldn't get type
	
	switch (byte) {
		case MSGPACK_FALSE:
		case MSGPACK_TRUE:
			return true;	// Already skipped by get_raw_byte
			break;
		case MSGPACK_TYPE_U16:
			seek_ptr += 2;
			break;
		case MSGPACK_TYPE_S32:
			seek_ptr += 4;
			break;
		case MSGPACK_TYPE_S64:
			seek_ptr += 8;
			break;
			
		case MSGPACK_TYPE_STR8:
			if (!get_raw_byte(buffer, true, (uint8_t*)&length)) return false;		// Couldn't get str8 length
			seek_ptr += length;
			break;
		case MSGPACK_TYPE_STR16:
			if (!get_raw_word(buffer, true, (uint16_t*)&length)) return false;		// Couldn't get str16 length
			seek_ptr += length;
			break;
			
		default:
			return false;	// Type unsupported
	}

	return true;
}

bool MsgPack::search_key(const void * buffer, const MsgPack::RecID record_id) {
	uint8_t byte;
	uint16_t key;
	
	while (get_raw_byte(buffer, true, &byte)) {
		if (byte == MSGPACK_TYPE_U16) {
			if (!get_u16(buffer, true, &key)) return false;	// Couldn't get key
			if (key == record_id) return true;	// Found record
			if (!skip(buffer)) return false;	// Can't skip to next key
		} else {
			return false;		// Key wasn't a U16
		}
	};
	return false;
}

bool MsgPack::msgpack_get(const void * buffer, const size_t size, const MsgPack::RecID record_id, bool * value) {
	init_search(buffer, size);
	if (!search_key(buffer, record_id)) return false;	// Record not found
		*value = true;
	return true;
	if (!get_bool(buffer, false, value)) return false;	// Value isn't a bool
	
	return true;
}

bool MsgPack::msgpack_get(const void * buffer, size_t size, RecID record_id, int32_t * value) {
	init_search(buffer, size);
	if (!search_key(buffer, record_id)) return false;	// Record not found
	if (!get_s32(buffer, false, value)) return false;	// Value isn't a s32
	
	return true;
}

bool MsgPack::msgpack_get(const void * buffer, size_t size, RecID record_id, int64_t * value) {
	init_search(buffer, size);
	if (!search_key(buffer, record_id)) return false;	// Record not found
	if (!get_s64(buffer, false, value)) return false;	// Value isn't a s64
	
	return true;
}

bool MsgPack::msgpack_get(const void * buffer, size_t size, RecID record_id, char * value) {
	init_search(buffer, size);
	if (!search_key(buffer, record_id)) return false;	// Record not found
	if (!get_chars(buffer, false, value)) return false;	// Value isn't a char array
	
	return true;
}
