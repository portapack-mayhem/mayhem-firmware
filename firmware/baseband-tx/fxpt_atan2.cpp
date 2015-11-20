/*
 * fxpt_atan2.c
 *
 * Copyright (C) 2012, Xo Wang
 *
 * Hacked up to be a bit more ARM-friendly by:
 * Copyright (C) 2013 Jared Boone, ShareBrained Technology, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * Convert floating point to Q15 (1.0.15 fixed point) format.
 *
 * @param d floating-point value within range -1 to (1 - (2**-15)), inclusive
 * @return Q15 value representing d; same range
 */
/*
static inline int16_t q15_from_double(const double d) {
	return lrint(d * 32768);
}
*/
/**
 * Negative absolute value. Used to avoid undefined behavior for most negative
 * integer (see C99 standard 7.20.6.1.2 and footnote 265 for the description of
 * abs/labs/llabs behavior).
 *
 * @param i 16-bit signed integer
 * @return negative absolute value of i; defined for all values of i
 */
 /*
static inline int16_t s16_nabs(const int16_t j) {
#if (((int16_t)-1) >> 1) == ((int16_t)-1)
	// signed right shift sign-extends (arithmetic)
	const int16_t negSign = ~(j >> 15); // splat sign bit into all 16 and complement
	// if j is positive (negSign is -1), xor will invert j and sub will add 1
	// otherwise j is unchanged
	return (j ^ negSign) - negSign;
#else
	return (j < 0 ? j : -j);
#endif
}
*/
/**
 * Q15 (1.0.15 fixed point) multiplication. Various common rounding modes are in
 * the function definition for reference (and preference).
 *
 * @param j 16-bit signed integer representing -1 to (1 - (2**-15)), inclusive
 * @param k same format as j
 * @return product of j and k, in same format
 */
static inline int16_t q15_mul(const int16_t j, const int16_t k) {
	const int32_t intermediate = j * k;
#if 0 // don't round
	return intermediate >> 15;
#elif 0 // biased rounding
	return (intermediate + 0x4000) >> 15;
#else // unbiased rounding
	return (intermediate + ((intermediate & 0x7FFF) == 0x4000 ? 0 : 0x4000)) >> 15;
#endif
}

/**
 * Q15 (1.0.15 fixed point) division (non-saturating). Be careful when using
 * this function, as it does not behave well when the result is out-of-range.
 *
 * Value is not defined if numerator is greater than or equal to denominator.
 *
 * @param numer 16-bit signed integer representing -1 to (1 - (2**-15))
 * @param denom same format as numer; must be greater than numerator
 * @return numer / denom in same format as numer and denom
 */
static inline int16_t q15_div(const int16_t numer, const int16_t denom) {
	return (static_cast<int32_t>(numer) << 15) / denom;
}

/**
 * 16-bit fixed point four-quadrant arctangent. Given some Cartesian vector
 * (x, y), find the angle subtended by the vector and the positive x-axis.
 *
 * The value returned is in units of 1/65536ths of one turn. This allows the use
 * of the full 16-bit unsigned range to represent a turn. e.g. 0x0000 is 0
 * radians, 0x8000 is pi radians, and 0xFFFF is (65535 / 32768) * pi radians.
 *
 * Because the magnitude of the input vector does not change the angle it
 * represents, the inputs can be in any signed 16-bit fixed-point format.
 *
 * @param y y-coordinate in signed 16-bit
 * @param x x-coordinate in signed 16-bit
 * @return angle in (val / 32768) * pi radian increments from 0x0000 to 0xFFFF
 */

static inline int16_t nabs(const int16_t j) {
	//return -abs(x);
	return (j < 0 ? j : -j);
}

int16_t fxpt_atan2(const int16_t y, const int16_t x) {
	static const int16_t k1 = 2847;
	static const int16_t k2 = 11039;
	if (x == y) { // x/y or y/x would return -1 since 1 isn't representable
		if (y > 0) { // 1/8
			return 8192;
		} else if (y < 0) { // 5/8
			return 40960;
		} else { // x = y = 0
			return 0;
		}
	}
	const int16_t nabs_y = nabs(y);
	const int16_t nabs_x = nabs(x);
	if (nabs_x < nabs_y) { // octants 1, 4, 5, 8
		const int16_t y_over_x = q15_div(y, x);
		const int16_t correction = q15_mul(k1, nabs(y_over_x));
		const int16_t unrotated = q15_mul(k2 + correction, y_over_x);
		if (x > 0) { // octants 1, 8
			return unrotated;
		} else { // octants 4, 5
			return 32768 + unrotated;
		}
	} else { // octants 2, 3, 6, 7
		const int16_t x_over_y = q15_div(x, y);
		const int16_t correction = q15_mul(k1, nabs(x_over_y));
		const int16_t unrotated = q15_mul(k2 + correction, x_over_y);
		if (y > 0) { // octants 2, 3
			return 16384 - unrotated;
		} else { // octants 6, 7
			return 49152 - unrotated;
		}
	}
}
