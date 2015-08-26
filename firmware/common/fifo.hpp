/*
 * Copyright (C) 2013 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __FIFO_H__
#define __FIFO_H__

#include <algorithm>
#include <cstring>

/* FIFO implementation inspired by Linux kfifo. */

template<typename T, size_t K>
class FIFO {
public:
	constexpr FIFO(
	) : _in { 0 },
		_out { 0 }
	{
	}

	void reset() {
		_in = _out = 0;
	}

	void reset_out() {
		_out = _in;
	}

	size_t len() const {
		return _in - _out;
	}

	size_t unused() const {
		return size() - (_in - _out);
	}

	bool is_empty() const {
		return _in == _out;
	}

	bool is_full() const {
		return len() > mask();
	}
/*
	bool in(const T& val) {
		const bool is_not_full = !is_full();
		if( is_not_full ) {
			_data[_in & mask()] = val;
			smp_wmb();
			_in++;
		}
		return is_not_full;
	}
*/
	size_t in(const T* const buf, size_t len) {
		const size_t l = unused();
		if( len > l ) {
			len = l;
		}

		copy_in(buf, len, _in);
		_in += len;
		return len;
	}

	size_t in_r(const void* const buf, const size_t len) {
		if( (len + recsize()) > unused() ) {
			return 0;
		}

		poke_n(len);
		copy_in((const T*)buf, len, _in + recsize());
		_in += len + recsize();
		return len;
	}
/*
	bool out(T& val) {
		if( is_empty() ) {
			return false;
		}

		val = _data[_out & mask()];
		smp_wmb();
		_out += 1;

		return true;
	}
*/
	size_t out(T* const buf, size_t len) {
		len = out_peek(buf, len);
		_out += len;
		return len;
	}

	size_t out_r(void* const buf, size_t len) {
		if( is_empty() ) {
			return 0;
		}

		size_t n;
		len = out_copy_r((T*)buf, len, &n);
		_out += n + recsize();
		return len;
	}

private:
	static constexpr size_t size() {
		return (1UL << K);
	}

	static constexpr size_t esize() {
		return sizeof(T);
	}

	static constexpr size_t mask() {
		return size() - 1;
	}

	static constexpr size_t recsize() {
		return 2;
	}

	void smp_wmb() {
		__DMB();
	}

	size_t peek_n() {
		size_t l = _data[_out & mask()];
		if( recsize() > 1 ) {
			l |= _data[(_out + 1) & mask()] << 8;
		}
		return l;
	}

	void poke_n(const size_t n) {
		_data[_in & mask()] = n & 0xff;
		if( recsize() > 1 ) {
			_data[(_in + 1) & mask()] = (n >> 8) & 0xff;
		}
	}

	void copy_in(const T* const src, const size_t len, size_t off) {
		off &= mask();
		const size_t l = std::min(len, size() - off);

		memcpy(&_data[off], &src[0], l * esize());
		memcpy(&_data[0], &src[l], (len - l) * esize());
		smp_wmb();
	}

	void copy_out(T* const dst, const size_t len, size_t off) {
		off &= mask();
		const size_t l = std::min(len, size() - off);

		memcpy(&dst[0], &_data[off], l * esize());
		memcpy(&dst[l], &_data[0], (len - l) * esize());
		smp_wmb();
	}

	size_t out_copy_r(void *buf, size_t len, size_t* const n) {
		*n = peek_n();

		if( len > *n ) {
			len = *n;
		}

		copy_out((T*)buf, len, _out + recsize());
		return len;
	}

	size_t out_peek(T* const buf, size_t buf_len) {
		const size_t l = len();
		if( buf_len > l ) {
			buf_len = l;
		}

		copy_out(buf, buf_len, _out);
		return buf_len;
	}

	T _data[size()];
	size_t _in;
	size_t _out;
};

#endif/*__FIFO_H__*/
