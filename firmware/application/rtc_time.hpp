/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2024 Mark Thompson
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

#ifndef __RTC_TIME_H__
#define __RTC_TIME_H__

#include "signal.hpp"

#include "lpc43xx_cpp.hpp"
#include "portapack_persistent_memory.hpp"

namespace rtc_time {

extern Signal<> signal_tick_second;

void on_tick_second();

/* Sets the current RTCTime in the RTC. */
void set(rtc::RTC& new_datetime);

/* Returns the current RTCTime from the RTC. */
rtc::RTC now();

/* Returns the current RTCTime from the RTC. */
rtc::RTC now(rtc::RTC& out_datetime);

/* Daylight Savings Time functions */
void dst_init();
rtc::RTC dst_adjust_returned_time(rtc::RTC& datetime);
bool dst_check_date_range(uint16_t doy, uint16_t start_doy, uint16_t end_doy);
void dst_update_date_range(uint16_t year, uint16_t doy);
bool dst_test_date_range(uint16_t year, uint16_t doy, portapack::persistent_memory::dst_config_t dst);
uint8_t days_per_month(uint16_t year, uint8_t month);
uint8_t current_day_of_week();
uint8_t day_of_week(uint16_t year, uint8_t month, uint8_t day);
bool leap_year(uint16_t year);
uint16_t day_of_year(uint16_t year, uint8_t month, uint8_t day);
uint16_t day_of_year_of_nth_weekday(uint16_t year, uint8_t month, uint8_t n, uint8_t weekday);

} /* namespace rtc_time */

#endif /*__RTC_TIME_H__*/
