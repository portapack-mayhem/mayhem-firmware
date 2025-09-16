/*
 * Copyright (C) 2024 Bernd Herzog
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

#ifndef __UI_STANDALONE_APP_H__
#define __UI_STANDALONE_APP_H__

#include <cerrno>
#include <cstdint>
#include <stddef.h>

#include "ui.hpp"
#include "file.hpp"

#define CURRENT_STANDALONE_APPLICATION_API_VERSION 4

struct standalone_application_api_t {
    // Version 1
    void* (*malloc)(size_t size);
    void* (*calloc)(size_t num, size_t size);
    void* (*realloc)(void* p, size_t size);
    void (*free)(void* p);
    void (*create_thread)(int32_t (*fn)(void*), void* arg, size_t stack_size, int priority);
    void (*fill_rectangle)(int x, int y, int width, int height, uint16_t color);
    uint8_t (*swizzled_switches)();
    uint64_t (*get_switches_state)();

    // Version 2
    const uint8_t* fixed_5x8_glyph_data;
    const uint8_t* fixed_8x16_glyph_data;

    void (*fill_rectangle_unrolled8)(int x, int y, int width, int height, uint16_t color);
    void (*draw_bitmap)(int x, int y, int width, int height, const uint8_t* pixels, uint16_t foreground, uint16_t background);

    ui::Coord (*scroll_area_y)(const ui::Coord y);
    void (*scroll_set_area)(const ui::Coord top_y, const ui::Coord bottom_y);
    void (*scroll_disable)();
    ui::Coord (*scroll_set_position)(const ui::Coord position);
    ui::Coord (*scroll)(const int32_t delta);

    bool (*i2c_read)(uint8_t* cmd, size_t cmd_len, uint8_t* data, size_t data_len);
    void (*panic)(const char* msg);
    void (*set_dirty)();

    // Version 3
    FRESULT(*f_open)
    (FIL* fp, const TCHAR* path, BYTE mode);
    FRESULT(*f_close)
    (FIL* fp);
    FRESULT(*f_read)
    (FIL* fp, void* buff, UINT btr, UINT* br);
    FRESULT(*f_write)
    (FIL* fp, const void* buff, UINT btw, UINT* bw);
    FRESULT(*f_lseek)
    (FIL* fp, FSIZE_t ofs);
    FRESULT(*f_truncate)
    (FIL* fp);
    FRESULT(*f_sync)
    (FIL* fp);
    FRESULT(*f_opendir)
    (DIR* dp, const TCHAR* path);
    FRESULT(*f_closedir)
    (DIR* dp);
    FRESULT(*f_readdir)
    (DIR* dp, FILINFO* fno);
    FRESULT(*f_findfirst)
    (DIR* dp, FILINFO* fno, const TCHAR* path, const TCHAR* pattern);
    FRESULT(*f_findnext)
    (DIR* dp, FILINFO* fno);
    FRESULT(*f_mkdir)
    (const TCHAR* path);
    FRESULT(*f_unlink)
    (const TCHAR* path);
    FRESULT(*f_rename)
    (const TCHAR* path_old, const TCHAR* path_new);
    FRESULT(*f_stat)
    (const TCHAR* path, FILINFO* fno);
    FRESULT(*f_utime)
    (const TCHAR* path, const FILINFO* fno);
    FRESULT(*f_getfree)
    (const TCHAR* path, DWORD* nclst, FATFS** fatfs);
    FRESULT(*f_mount)
    (FATFS* fs, const TCHAR* path, BYTE opt);
    int (*f_putc)(TCHAR c, FIL* fp);
    int (*f_puts)(const TCHAR* str, FIL* cp);
    int (*f_printf)(FIL* fp, const TCHAR* str, ...);
    TCHAR* (*f_gets)(TCHAR* buff, int len, FIL* fp);
    void (*draw_pixels)(const ui::Rect r, const ui::Color* const colors, const size_t count);
    void (*draw_pixel)(const ui::Point p, const ui::Color color);
    void (*exit_app)();

    // Version 4
    uint16_t* screen_height;
    uint16_t* screen_width;

    // TODO: add baseband access functions

    // HOW TO extend this interface:
    // to keep everything backward compatible: add new fields at the end
    // and increment CURRENT_STANDALONE_APPLICATION_API_VERSION
};

enum app_location_t : uint32_t {
    UTILITIES = 0,
    RX,
    TX,
    DEBUG,
    HOME,
    SETTINGS,
    GAMES,
    TRX
};

struct standalone_application_information_t {
    uint32_t header_version;

    uint8_t app_name[16];
    uint8_t bitmap_data[32];
    uint32_t icon_color;
    app_location_t menu_location;

    /// @brief gets called once at application start
    void (*initialize)(const standalone_application_api_t& api);

    /// @brief gets called when an event occurs
    /// @param events bitfield of events
    /// @note events are defined in firmware/application/event_m0.hpp
    void (*on_event)(const uint32_t& events);

    /// @brief gets called once at application shutdown
    void (*shutdown)();

    void (*PaintViewMirror)();
    void (*OnTouchEvent)(int x, int y, uint32_t type);
    void (*OnFocus)();
    bool (*OnKeyEvent)(uint8_t key);
    bool (*OnEncoder)(int32_t delta);
    bool (*OnKeyboard)(uint8_t key);
};

#endif /*__UI_STANDALONE_APP_H__*/
