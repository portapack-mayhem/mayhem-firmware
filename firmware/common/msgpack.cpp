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

bool MsgPack::get_u8(const void * buffer, const bool inc, uint8_t * value) {
	uint8_t v;
	
	if (seek_ptr >= buffer_size) return false;	// End of buffer
	
	v = ((uint8_t*)buffer)[seek_ptr];

	if (!(v & 0x80))
		*value = ((uint8_t*)buffer)[seek_ptr];	// Fixnum
	else if (v == MSGPACK_TYPE_U8)
		*value = ((uint8_t*)buffer)[seek_ptr + 1];	// u8
	else
		return false;		// Value isn't a u8 or fixnum
	
	if (inc) seek_ptr++;
	return true;
}

// TODO: Typecheck function

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

bool MsgPack::get_string(const void * buffer, const bool inc, std::string& value) {
	size_t length;
	uint8_t byte;
	
	// Todo: Set max length !
	if ((seek_ptr + 3) >= buffer_size) return false;	// End of buffer
	if ((get_raw_byte(buffer, true, &byte)) && (byte != MSGPACK_TYPE_STR8)
			&& (byte != MSGPACK_TYPE_STR16)) return false;		// Value isn't a str8 or str16

	if (byte == MSGPACK_TYPE_STR8) {
		if (!get_raw_byte(buffer, true, (uint8_t*)&length)) return false;		// Couldn't get str8 length
	} else if (byte == MSGPACK_TYPE_STR16) {
		if (!get_raw_word(buffer, true, (uint16_t*)&length)) return false;		// Couldn't get str16 length
	}
	
	memcpy(&value[0], ((uint8_t*)buffer), length); //std::string(

	if (inc) seek_ptr += length;
	return true;
}

bool MsgPack::init_search(const void * buffer, const size_t size) {
	uint8_t byte;
	uint16_t map_size;
	
	if (!size) return false;
	buffer_size = size;
	seek_ptr = 0;
	if ((get_raw_byte(buffer, true, &byte)) && (byte != MSGPACK_TYPE_MAP16)) return false;		// First record isn't a map16
	if (!get_raw_word(buffer, true, &map_size)) return false;		// Couldn't get map16 size
	if (!map_size) return false;
	
	return true;
}

bool MsgPack::skip(const void * buffer) {
	uint8_t byte, c;
	size_t length;
	
	if (!get_raw_byte(buffer, true, &byte)) return false;		// Couldn't get type
	
	if (!(byte & 0x80)) return true;			// Positive fixnum, already skipped by get_raw_byte
	if ((byte & 0xE0) == 0xE0) return true;		// Negative fixnum, already skipped by get_raw_byte
	if ((byte & 0xE0) == 0xA0) {				// Fixstr
		seek_ptr += (byte & 0x1F);
		return true;
	}
	if ((byte & 0xF0) == 0x80) {				// Fixmap
		length = (byte & 0x0F) * 2;
		for (c = 0; c < length; c++)
			skip(buffer);
		return true;
	}
	if ((byte & 0xF0) == 0x90) {				// Fixarray
		length = byte & 0x0F;
		for (c = 0; c < length; c++)
			skip(buffer);
		return true;
	}
	
	switch (byte) {
		case MSGPACK_NIL:
		case MSGPACK_FALSE:
		case MSGPACK_TRUE:		// Already skipped by get_raw_byte
			break;
		case MSGPACK_TYPE_U8:
		case MSGPACK_TYPE_S8:
			seek_ptr++;
			break;
		case MSGPACK_TYPE_U16:
		case MSGPACK_TYPE_S16:
			seek_ptr += 2;
			break;
		case MSGPACK_TYPE_U32:
		case MSGPACK_TYPE_S32:
			seek_ptr += 4;
			break;
		case MSGPACK_TYPE_U64:
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
		
		case MSGPACK_TYPE_ARR16:
			if (!get_raw_word(buffer, true, (uint16_t*)&length)) return false;		// Couldn't get arr16 length
			for (c = 0; c < length; c++)
				skip(buffer);
			break;
			
		case MSGPACK_TYPE_MAP16:
			if (!get_raw_word(buffer, true, (uint16_t*)&length)) return false;		// Couldn't get map16 length
			for (c = 0; c < (length * 2); c++)
				skip(buffer);
			break;
			
		default:
			return false;	// Type unsupported
	}
	
	return true;
}

bool MsgPack::search_key(const void * buffer, const MsgPack::RecID record_id) {
	uint8_t byte;
	uint16_t key;
	
	while (get_raw_byte(buffer, false, &byte)) {
		if (!get_u16(buffer, true, &key)) return false;	// Couldn't get key
		if (key == record_id) return true;				// Found record
		if (!skip(buffer)) return false;				// Can't skip to next key
	};
	return false;
}

bool MsgPack::msgpack_get(const void * buffer, const size_t size, const RecID record_id, bool * value) {
	init_search(buffer, size);
	if (!search_key(buffer, record_id)) return false;	// Record not found
	if (!get_bool(buffer, false, value)) return false;	// Value isn't a bool
	
	return true;
}

bool MsgPack::msgpack_get(const void * buffer, const size_t size, const RecID record_id, uint8_t * value) {
	if (!init_search(buffer, size)) return false;
	if (!search_key(buffer, record_id)) return false;	// Record not found
	if (!get_u8(buffer, false, value)) return false;	// Value isn't a u8
	
	return true;
}

bool MsgPack::msgpack_get(const void * buffer, const size_t size, const RecID record_id, int64_t * value) {
	uint8_t byte;
	
	init_search(buffer, size);
	if (!search_key(buffer, record_id)) return false;	// Record not found
	
	if ((seek_ptr + 3) >= buffer_size) return false;	// End of buffer
	if ((get_raw_byte(buffer, true, &byte)) && (byte != MSGPACK_TYPE_S64)) return false;		// Value isn't a s64
	*value = ((int64_t)((uint8_t*)buffer)[seek_ptr] << 56) | ((int64_t)((uint8_t*)buffer)[seek_ptr + 1] << 48) |
				((int64_t)((uint8_t*)buffer)[seek_ptr + 2] << 40) | ((int64_t)((uint8_t*)buffer)[seek_ptr + 3] << 32) |
				(((uint8_t*)buffer)[seek_ptr + 4] << 24) | (((uint8_t*)buffer)[seek_ptr + 5] << 16) |
				(((uint8_t*)buffer)[seek_ptr + 6] << 8) | ((uint8_t*)buffer)[seek_ptr + 7];
	
	return true;
}

bool MsgPack::msgpack_get(const void * buffer, const size_t size, const RecID record_id, std::string& value) {
	init_search(buffer, size);
	if (!search_key(buffer, record_id)) return false;	// Record not found
	if (!get_string(buffer, false, value)) return false;	// Value isn't a char array
	
	return true;
}



void MsgPack::msgpack_init(const void * buffer, size_t * ptr) {
	((uint8_t*)buffer)[0] = MSGPACK_TYPE_MAP16;
	((uint8_t*)buffer)[1] = 0;
	((uint8_t*)buffer)[2] = 0;
	
	*ptr = 3;
}

void MsgPack::add_key(const void * buffer, size_t * ptr, const RecID record_id) {
	uint16_t key;
	
	((uint8_t*)buffer)[(*ptr)++] = MSGPACK_TYPE_U16;
	((uint8_t*)buffer)[(*ptr)++] = record_id >> 8;
	((uint8_t*)buffer)[(*ptr)++] = record_id & 0xFF;
	
	// Auto-inc MAP16 size which should be at the beginning of the buffer
	
	key = (((uint8_t*)buffer)[1] << 8) | ((uint8_t*)buffer)[2];
	key++;
	
	((uint8_t*)buffer)[1] = key >> 8;
	((uint8_t*)buffer)[2] = key & 0xFF;
}

void MsgPack::msgpack_add(const void * buffer, size_t * ptr, const RecID record_id, bool value) {
	add_key(buffer, ptr, record_id);
	
	if (value)
		((uint8_t*)buffer)[(*ptr)++] = MSGPACK_TRUE;
	else
		((uint8_t*)buffer)[(*ptr)++] = MSGPACK_FALSE;
}

void MsgPack::msgpack_add(const void * buffer, size_t * ptr, const RecID record_id, uint8_t value) {
	add_key(buffer, ptr, record_id);
	
	if (value < 128) {
		((uint8_t*)buffer)[(*ptr)++] = value;
	} else {
		((uint8_t*)buffer)[(*ptr)++] = MSGPACK_TYPE_U8;
		((uint8_t*)buffer)[(*ptr)++] = value;
	}
}

void MsgPack::msgpack_add(const void * buffer, size_t * ptr, const RecID record_id, int64_t value) {
	uint8_t c;
	
	add_key(buffer, ptr, record_id);
	
	((uint8_t*)buffer)[(*ptr)++] = MSGPACK_TYPE_S64;
	
	for (c = 0; c < 8; c++)
		((uint8_t*)buffer)[(*ptr)++] = (value >> (8 * (7 - c))) & 0xFF;
}

bool MsgPack::msgpack_add(const void * buffer, size_t * ptr, const RecID record_id, std::string value) {
	uint8_t c;
	size_t length;
	
	add_key(buffer, ptr, record_id);
	
	length = value.size();
	
	if (length < 32) {
		((uint8_t*)buffer)[(*ptr)++] = length | 0xA0;			// Fixstr
	} else if ((length >= 32) && (length < 256)) {
		((uint8_t*)buffer)[(*ptr)++] = MSGPACK_TYPE_STR8;
		((uint8_t*)buffer)[(*ptr)++] = length;
	} else if ((length >= 256) && (length < 65536)) {
		((uint8_t*)buffer)[(*ptr)++] = MSGPACK_TYPE_STR16;
		((uint8_t*)buffer)[(*ptr)++] = length >> 8;
		((uint8_t*)buffer)[(*ptr)++] = length & 0xFF;
	} else {
		return false;
	}
	
	for (c = 0; c < length; c++)
		((uint8_t*)buffer)[(*ptr)++] = value[c];
		
	return true;
}
