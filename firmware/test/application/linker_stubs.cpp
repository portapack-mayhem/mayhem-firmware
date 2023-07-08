/*
 * Copyright (C) 2023 Kyle Reed
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

/* This file contains stub functions necessary to enable linking.
 * Try to minimize dependecies by breaking code into separate files
 * or using templates and mock types. Because the test code is built
 * and executed on the dev machine, a lot of core firmware code
 * will not or cannot work (e.g. filesystem). We could build abstractions
 * but that's just device overhead that only supports testing. */

#include <string>

/* FatFS stubs */
#include "ff.h"
FRESULT f_close(FIL*) {
    return FR_OK;
}
FRESULT f_closedir(DIR*) {
    return FR_OK;
}
FRESULT f_findfirst(DIR*, FILINFO*, const TCHAR*, const TCHAR*) {
    return FR_OK;
}
FRESULT f_findnext(DIR*, FILINFO*) {
    return FR_OK;
}
FRESULT f_getfree(const TCHAR*, DWORD*, FATFS**) {
    return FR_OK;
}
FRESULT f_lseek(FIL*, FSIZE_t) {
    return FR_OK;
}
FRESULT f_mkdir(const TCHAR*) {
    return FR_OK;
}
FRESULT f_open(FIL*, const TCHAR*, BYTE) {
    return FR_OK;
}
FRESULT f_read(FIL*, void*, UINT, UINT*) {
    return FR_OK;
}
FRESULT f_rename(const TCHAR*, const TCHAR*) {
    return FR_OK;
}
FRESULT f_stat(const TCHAR*, FILINFO*) {
    return FR_OK;
}
FRESULT f_sync(FIL*) {
    return FR_OK;
}
FRESULT f_truncate(FIL*) {
    return FR_OK;
}
FRESULT f_unlink(const TCHAR*) {
    return FR_OK;
}
FRESULT f_write(FIL*, const void*, UINT, UINT*) {
    return FR_OK;
}

/* Debug */
void __debug_log(const std::string&) {}
