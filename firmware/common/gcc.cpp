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

#include "gcc.hpp"

#include <ch.h>

#if defined(TOOLCHAIN_GCC)

/* Note: Added to deal with static variables:
 * undefined reference to `__dso_handle'.
 * Comes up when using random C++ features, e.g. lambdas or std::function?
 */
void *__dso_handle;

/* prevents the exception handling name demangling code getting pulled in */
namespace __gnu_cxx {
	void __verbose_terminate_handler() {
	}
}

/* NOTE: Hack to address bloat when using C++ class virtual destructors.
 */
extern "C" __attribute__((weak)) void __cxa_pure_virtual(void) {
	chSysHalt();
}

#endif

/* Implementing abort() eliminates requirement for _getpid(), _kill(),
 * _exit().
 */
extern "C" void abort() {
	/* while() loop to avoid noreturn-is-returning warning. */
	while(1) { chSysHalt(); }
}
