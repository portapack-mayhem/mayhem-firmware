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

#include "standalone_app.hpp"
#include "digitalrain.hpp"
#include <memory>

const standalone_application_api_t* _api;

extern "C" {
__attribute__((section(".standalone_application_information"), used)) standalone_application_information_t _standalone_application_information = {
    /*.header_version = */ 2,

    /*.app_name = */ "DigitalRain",
    /*.bitmap_data = */ {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0xFC,
        0x3F,
        0xFE,
        0x7F,
        0x02,
        0x40,
        0xBA,
        0x45,
        0x02,
        0x40,
        0xFE,
        0x7F,
        0xFE,
        0x7F,
        0x92,
        0x7C,
        0x92,
        0x7C,
        0xFC,
        0x3F,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
    },
    /*.icon_color = 16 bit: 5R 6G 5B*/ 0x0000FFE0,
    /*.menu_location = */ app_location_t::GAMES,

    /*.initialize_app = */ initialize,
    /*.on_event = */ on_event,
    /*.shutdown = */ shutdown,
    /*PaintViewMirror */ PaintViewMirror,
    /*OnTouchEvent */ OnTouchEvent,
    /*OnFocus */ OnFocus,
    /*OnKeyEvent */ OnKeyEvent,
    /*OnEncoder */ OnEncoder,
    /*OnKeyboard */ OnKeyboad};
}

/* Implementing abort() eliminates requirement for _getpid(), _kill(), _exit(). */
extern "C" void abort() {
    while (true);
}

// replace memory allocations to use heap from chibios
extern "C" void* malloc(size_t size) {
    return _api->malloc(size);
}
extern "C" void* calloc(size_t num, size_t size) {
    return _api->calloc(num, size);
}
extern "C" void* realloc(void* p, size_t size) {
    return _api->realloc(p, size);
}
extern "C" void free(void* p) {
    _api->free(p);
}

// redirect std lib memory allocations (sprintf, etc.)
extern "C" void* __wrap__malloc_r(size_t size) {
    return _api->malloc(size);
}
extern "C" void __wrap__free_r(void* p) {
    _api->free(p);
}

// redirect file I/O

extern "C" FRESULT f_open(FIL* fp, const TCHAR* path, BYTE mode) {
    return _api->f_open(fp, path, mode);
}
extern "C" FRESULT f_close(FIL* fp) {
    return _api->f_close(fp);
}
extern "C" FRESULT f_read(FIL* fp, void* buff, UINT btr, UINT* br) {
    return _api->f_read(fp, buff, btr, br);
}
extern "C" FRESULT f_write(FIL* fp, const void* buff, UINT btw, UINT* bw) {
    return _api->f_write(fp, buff, btw, bw);
}
extern "C" FRESULT f_lseek(FIL* fp, FSIZE_t ofs) {
    return _api->f_lseek(fp, ofs);
}
extern "C" FRESULT f_truncate(FIL* fp) {
    return _api->f_truncate(fp);
}
extern "C" FRESULT f_sync(FIL* fp) {
    return _api->f_sync(fp);
}
extern "C" FRESULT f_opendir(DIR* dp, const TCHAR* path) {
    return _api->f_opendir(dp, path);
}
extern "C" FRESULT f_closedir(DIR* dp) {
    return _api->f_closedir(dp);
}
extern "C" FRESULT f_readdir(DIR* dp, FILINFO* fno) {
    return _api->f_readdir(dp, fno);
}
extern "C" FRESULT f_findfirst(DIR* dp, FILINFO* fno, const TCHAR* path, const TCHAR* pattern) {
    return _api->f_findfirst(dp, fno, path, pattern);
}
extern "C" FRESULT f_findnext(DIR* dp, FILINFO* fno) {
    return _api->f_findnext(dp, fno);
}
extern "C" FRESULT f_mkdir(const TCHAR* path) {
    return _api->f_mkdir(path);
}
extern "C" FRESULT f_unlink(const TCHAR* path) {
    return _api->f_unlink(path);
}
extern "C" FRESULT f_rename(const TCHAR* path_old, const TCHAR* path_new) {
    return _api->f_rename(path_old, path_new);
}
extern "C" FRESULT f_stat(const TCHAR* path, FILINFO* fno) {
    return _api->f_stat(path, fno);
}
extern "C" FRESULT f_utime(const TCHAR* path, const FILINFO* fno) {
    return _api->f_utime(path, fno);
}
extern "C" FRESULT f_getfree(const TCHAR* path, DWORD* nclst, FATFS** fatfs) {
    return _api->f_getfree(path, nclst, fatfs);
}
extern "C" FRESULT f_mount(FATFS* fs, const TCHAR* path, BYTE opt) {
    return _api->f_mount(fs, path, opt);
}
extern "C" int f_putc(TCHAR c, FIL* fp) {
    return _api->f_putc(c, fp);
}
extern "C" int f_puts(const TCHAR* str, FIL* cp) {
    return _api->f_puts(str, cp);
}
extern "C" int f_printf(FIL* fp, const TCHAR* str, ...) {
    return _api->f_printf(fp, str);
}
extern "C" TCHAR* f_gets(TCHAR* buff, int len, FIL* fp) {
    return _api->f_gets(buff, len, fp);
}

uint16_t screen_height = 320;
uint16_t screen_width = 240;
