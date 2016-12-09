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

#ifndef __UI_H__
#define __UI_H__

#include <cstdint>

namespace ui {

using Coord = int16_t;
using Dim = int16_t;

struct Color {
	uint16_t v;			// rrrrrGGGGGGbbbbb

	constexpr Color(
	) : v { 0 }
	{
	}
	
	constexpr Color(
		uint16_t v
	) : v { v }
	{
	}

	constexpr Color(
		uint8_t r,
		uint8_t g,
		uint8_t b
	) : v {
		static_cast<uint16_t>(
			  ((r & 0xf8) << 8)
			| ((g & 0xfc) << 3)
			| ((b & 0xf8) >> 3)
		)}
	{
	}
	
	Color operator-() const {
		return (v ^ 0xffff);
	}

	static constexpr Color black() {
		return {   0,   0,   0 };
	}

	static constexpr Color red() {
		return { 255,   0,   0 };
	}
	
	static constexpr Color orange() {
		return { 255, 127,   0 };
	}

	static constexpr Color yellow() {
		return { 255, 255,   0 };
	}

	static constexpr Color green() {
		return {   0, 255,   0 };
	}

	static constexpr Color blue() {
		return {   0,   0, 255 };
	}
	
	static constexpr Color cyan() {
		return {   0,   255, 255 };
	}

	static constexpr Color white() {
		return { 255, 255, 255 };
	}
	
	static constexpr Color light_grey() {
		return { 127, 127, 127 };
	}
	
	static constexpr Color grey() {
		return { 91, 91, 91 };
	}
	
	static constexpr Color dark_grey() {
		return { 63, 63, 63 };
	}
	
	static constexpr Color purple() {
		return { 204, 0, 102 };
	}
};

struct ColorRGB888 {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

struct Point {
	Coord x;
	Coord y;

	constexpr Point(
	) : x { 0 },
		y { 0 }
	{
	}

	constexpr Point(
		int x,
		int y
	) : x { static_cast<Coord>(x) },
		y { static_cast<Coord>(y) }
	{
	}

	Point operator-() const {
		return { -x, -y };
	}

	Point operator+(const Point& p) const {
		return { x + p.x, y + p.y };
	}

	Point operator-(const Point& p) const {
		return { x - p.x, y - p.y };
	}

	Point& operator+=(const Point& p) {
		x += p.x;
		y += p.y;
		return *this;
	}
};

struct Size {
	Dim w;
	Dim h;

	constexpr Size(
	) : w { 0 },
		h { 0 }
	{
	}

	constexpr Size(
		int w,
		int h
	) : w { static_cast<Dim>(w) },
		h { static_cast<Dim>(h) }
	{
	}

	bool is_empty() const {
		return (w < 1) || (h < 1);
	}
};

struct Rect {
	Point pos;
	Size size;

	constexpr Rect(
	) : pos { },
		size { }
	{
	}

	constexpr Rect(
		int x, int y,
		int w, int h
	) : pos { x, y },
		size { w, h }
	{
	}

	constexpr Rect(
		Point pos,
		Size size
	) : pos(pos),
		size(size)
	{
	}
	
	int top() const {
		return pos.y;
	}

	int bottom() const {
		return pos.y + size.h;
	}

	int left() const {
		return pos.x;
	}

	int right() const {
		return pos.x + size.w;
	}

	int width() const {
		return size.w;
	}

	int height() const {
		return size.h;
	}

	Point center() const {
		return { pos.x + size.w / 2, pos.y + size.h / 2 };
	}

	bool is_empty() const {
		return size.is_empty();
	}

	bool contains(const Point p) const;

	Rect intersect(const Rect& o) const;

	Rect operator+(const Point& p) const {
		return { pos + p, size };
	}

	Rect& operator+=(const Rect& p);

	operator bool() const {
		return !size.is_empty();
	}
};

struct Bitmap {
	const Size size;
	const uint8_t* const data;
};

enum class KeyEvent {
	/* Ordinals map to bit positions reported by CPLD */
	Right = 0,
	Left = 1,
	Down = 2,
	Up = 3,
	Select = 4,
};

using EncoderEvent = int32_t;

struct TouchEvent {
	enum class Type : uint32_t {
		Start = 0,
		Move = 1,
		End = 2,
	};

	Point point;
	Type type;
};

} /* namespace ui */

#endif/*__UI_H__*/
