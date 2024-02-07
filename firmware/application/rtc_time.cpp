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

#include "rtc_time.hpp"

using namespace lpc43xx;
using namespace portapack;

namespace pmem = portapack::persistent_memory;

namespace rtc_time {

Signal<> signal_tick_second;

bool dst_enabled{false};
bool dst_in_range{false};
uint16_t dst_start_doy;
uint16_t dst_end_doy;
uint16_t dst_current_doy{0xFFFF};
uint16_t dst_current_year{0xFFFF};

static const uint16_t months_with_31 = 0x0AD5;  // Bits represents months with 31 days (bit 0 for January, etc)

void on_tick_second() {
    signal_tick_second.emit();
}

// Check if DST is configured and active (called at power-up)
void dst_init() {
    rtc::RTC datetime;
    rtcGetTime(&RTCD1, &datetime);
    dst_update_date_range(datetime.year(), LPC_RTC->DOY);
}

// Return current time & date (adjusted for DST if enabled)
rtc::RTC now() {
    rtc::RTC datetime;
    rtcGetTime(&RTCD1, &datetime);
    if (dst_enabled) {
        datetime = dst_adjust_returned_time(datetime);
    }
    return datetime;
}

rtc::RTC now(rtc::RTC& out_datetime) {
    rtcGetTime(&RTCD1, &out_datetime);
    if (dst_enabled) {
        out_datetime = dst_adjust_returned_time(out_datetime);
    }
    return out_datetime;
}

// Add 1 hour (spring forward) if needed for DST (called whenever RTC is read if DST is enabled)
rtc::RTC dst_adjust_returned_time(rtc::RTC& datetime) {
    // Read day of year from RTC and check for change
    uint32_t doy = LPC_RTC->DOY;

    // Update DST start/end day-of-year only when year changes, otherwise just check if in range when day changes
    if (doy != dst_current_doy) {
        if (datetime.year() != dst_current_year)
            dst_update_date_range(datetime.year(), doy);
        else {
            dst_current_doy = doy;
            dst_in_range = dst_check_date_range(doy, dst_start_doy, dst_end_doy);
        }
    }

    // Not in DST date range
    if (!dst_in_range)
        return datetime;

    // Add 1 hour to RTC time
    uint16_t year = datetime.year();
    uint8_t month = datetime.month();
    uint8_t day = datetime.day();
    uint8_t hour = datetime.hour();

    if (++hour > 23) {
        hour = 0;
        if (++day > days_per_month(year, month)) {
            day = 1;
            if (++month > 12) {
                year++;
            }
        }
    }
    rtc::RTC dst_datetime{year, month, day, hour, datetime.minute(), datetime.second()};
    return dst_datetime;
}

// Check if current date is within the DST range (called when date changes)
bool dst_check_date_range(uint16_t doy, uint16_t start_doy, uint16_t end_doy) {
    // Check if date is within DST range
    // (note that dates are reversed in Southern hemisphere because Summer starts in December)
    if (start_doy <= end_doy)
        return ((doy >= start_doy) && (doy < end_doy));
    else
        return ((doy >= start_doy) || (doy < end_doy));
}

// Update DST parameters (called at power-up, when year changes, or DST settings are changed)
void dst_update_date_range(uint16_t year, uint16_t doy) {
    dst_enabled = pmem::dst_enabled();
    if (dst_enabled) {
        const pmem::dst_config_t dst = pmem::config_dst();
        dst_current_year = year;
        dst_start_doy = day_of_year_of_nth_weekday(dst_current_year, dst.b.start_month, dst.b.start_which, dst.b.start_weekday);
        dst_end_doy = day_of_year_of_nth_weekday(dst_current_year, dst.b.end_month, dst.b.end_which, dst.b.end_weekday);

        dst_current_doy = doy;
        dst_in_range = dst_check_date_range(doy, dst_start_doy, dst_end_doy);
    } else {
        dst_in_range = false;
    }
}

// Test date range for display purposes only when setting Time
bool dst_test_date_range(uint16_t year, uint16_t doy, pmem::dst_config_t dst) {
    uint16_t start_doy = day_of_year_of_nth_weekday(year, dst.b.start_month, dst.b.start_which, dst.b.start_weekday);
    uint16_t end_doy = day_of_year_of_nth_weekday(year, dst.b.end_month, dst.b.end_which, dst.b.end_weekday);
    return dst_check_date_range(doy, start_doy, end_doy);
}

// Set RTC clock.
// If the user has enabled DST, the time entered and passed to this function is interpreted as having been adjusted for DST.
// When DST functionality is enabled in pmem, the value stored in the RTC hardware is non-DST time, and we only fudge the time when read.
// Thus, it's necessary to subtract an hour before storing it in the RTC if we're in the DST date range.
// (If RTC is desired to represent UTC, then DST should obviously be disabled in settings.)
// NB: Firmware should not change RTC without going through this function.
void set(rtc::RTC& new_datetime) {
    uint16_t year = new_datetime.year();
    uint8_t month = new_datetime.month();
    uint8_t day = new_datetime.day();
    uint8_t hour = new_datetime.hour();

    // NB: DST code relies on the Day of Year (DOY) value to be initialized in the RTC by firmware (RTC hardware only does increment and wrap)
    uint16_t doy = day_of_year(year, month, day);
    dst_update_date_range(year, doy);

    // NB: Currently we only support the DST transition at midnight RTC time (not configurable to hour granularity).
    // Note on handling the the hour before/after DST time change:
    //
    //   If entered hour==0 on dst_start_day:
    //      Time entered is INVALID due to spring-forward; code below rolls back RTC to last hour of previous day.
    //
    //   If entered hour==0 on dst_end_doy:
    //      This hour occurs twice due to fall-back; code below treats as if DST has ended (the second occurrence) and doesn't roll back RTC
    //
    if (dst_in_range) {
        // Subtract 1 hour from requested time before storing in RTC
        if (hour-- == 0) {
            hour = 23;
            if (day-- == 0) {
                if (month-- == 0) {
                    month = 12;
                    year--;
                }
                day = days_per_month(year, month);
            }
        }

        // Update day-of-year if date was changed above
        if (day != new_datetime.day()) {
            doy = day_of_year(year, month, day);
            dst_update_date_range(year, doy);
        }
    }

    // NB: Writing RTC twice takes a second, but ensures that new value can be read back immediately
    // (if not written twice, the old value will be returned if read back in the following 1-2 seconds)
    // (you will notice with older Mayhem versions that running the Date/Time Settings app again quickly will show the old time)
    LPC_RTC->DOY = doy;
    rtc::RTC adjusted_datetime{year, month, day, hour, new_datetime.minute(), new_datetime.second()};
    rtcSetTime(&RTCD1, &adjusted_datetime);
    rtcSetTime(&RTCD1, &adjusted_datetime);
}

// Calculates 1-based day of year for Nth weekday in month (weekday and n are 0-based, month is 1-based)
uint16_t day_of_year_of_nth_weekday(uint16_t year, uint8_t month, uint8_t n, uint8_t weekday) {
    uint8_t w = day_of_week(year, month, 1);
    uint8_t nn = n;

    // special handling for "last" weekday - are there 4 or 5 in the month?
    if (n == 4) {
        if (month == 2) {
            // February
            // only weekday w occurs 5 times on the 29th on leap years only
            if ((weekday != w) || !leap_year(year))
                nn = 3;
        } else {
            // Other months have either 30 or 31 days;
            // weekdays w and w+1 occur 5 times (on the 29th & 30th); weekday w+2 occurs 5 times only if month has 31 days
            if ((weekday != w) && (weekday != (w + 1) % 7) &&
                ((weekday != (w + 2) % 7) || ((months_with_31 & (1 << (month - 1))) == 0)))
                nn = 3;
        }
    }
    return day_of_year(year, month, 1) + (nn * 7) + weekday + ((weekday < w) ? 7 : 0) - w;
}

// Calculates 1-based day of year (input month & day are 1-based)
uint16_t day_of_year(uint16_t year, uint8_t month, uint8_t day) {
    // 1-based day of year for 1st day or month (index is 1-based month)
    static uint16_t month_to_day_in_year[1 + 12] = {
        0,  // placeholder for 1-based indexing
        1,
        1 + 31,
        1 + 31 + 28,
        1 + 31 + 28 + 31,
        1 + 31 + 28 + 31 + 30,
        1 + 31 + 28 + 31 + 30 + 31,
        1 + 31 + 28 + 31 + 30 + 31 + 30,
        1 + 31 + 28 + 31 + 30 + 31 + 30 + 31,
        1 + 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
        1 + 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
        1 + 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
        1 + 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30,
    };
    return month_to_day_in_year[month] + day - (((month > 2) && leap_year(year)) ? 0 : 1);
}

bool leap_year(uint16_t year) {
    // Technically should be: ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0)
    // but year 2100 is a long way off and the LPC43xx RTC doesn't bother handling it either
    return ((year & 3) == 0);
}

uint8_t days_per_month(uint16_t year, uint8_t month) {
    if (month == 2)
        return leap_year(year) ? 29 : 28;
    else
        return ((months_with_31 & (1 << (month - 1))) != 0) ? 31 : 30;
}

uint8_t current_day_of_week() {
    rtc::RTC datetime;
    now(datetime);
    return day_of_week(datetime.year(), datetime.month(), datetime.day());
}

// Returns 0-based weekday for date (0=Sunday, 1=Monday, etc) - using Zeller's Congruence formula
// (month and day are 1-based)
uint8_t day_of_week(uint16_t year, uint8_t month, uint8_t day) {
    int m = (month < 3) ? month + 13 : month + 1;
    int y = (month < 3) ? year - 1 : year;
    return (day - 1 + (13 * m / 5) + y + (y / 4) - (y / 100) + (y / 400)) % 7;
}

} /* namespace rtc_time */
