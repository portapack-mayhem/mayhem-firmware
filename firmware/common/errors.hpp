/*
 * Copyright (C) 2022 José Moreira @cusspvz
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

#include <string>

#pragma once

namespace errors
{
    static constexpr Error FR_OK{0, "FR_OK"};                                    /* (0) Succeeded */
    static constexpr Error FR_DISK_ERR{1, "FR_DISK_ERR"};                        /* (1) A hard error occurred in the low level disk I/O layer */
    static constexpr Error FR_INT_ERR{2, "FR_INT_ERR"};                          /* (2) Assertion failed */
    static constexpr Error FR_NOT_READY{3, "FR_NOT_READY"};                      /* (3) The physical drive cannot work */
    static constexpr Error FR_NO_FILE{4, "FR_NO_FILE"};                          /* (4) Could not find the file */
    static constexpr Error FR_NO_PATH{5, "FR_NO_PATH"};                          /* (5) Could not find the path */
    static constexpr Error FR_INVALID_NAME{6, "FR_INVALID_NAME"};                /* (6) The path name format is invalid */
    static constexpr Error FR_DENIED{7, "FR_DENIED"};                            /* (7) Access denied due to prohibited access or directory full */
    static constexpr Error FR_EXIST{8, "FR_EXIST"};                              /* (8) Access denied due to prohibited access */
    static constexpr Error FR_INVALID_OBJECT{9, "FR_INVALID_OBJECT"};            /* (9) The file/directory object is invalid */
    static constexpr Error FR_WRITE_PROTECTED{10, "FR_WRITE_PROTECTED"};         /* (10) The physical drive is write protected */
    static constexpr Error FR_INVALID_DRIVE{11, "FR_INVALID_DRIVE"};             /* (11) The logical drive number is invalid */
    static constexpr Error FR_NOT_ENABLED{12, "FR_NOT_ENABLED"};                 /* (12) The volume has no work area */
    static constexpr Error FR_NO_FILESYSTEM{13, "FR_NO_FILESYSTEM"};             /* (13) There is no valid FAT volume */
    static constexpr Error FR_MKFS_ABORTED{14, "FR_MKFS_ABORTED"};               /* (14) The f_mkfs() aborted due to any problem */
    static constexpr Error FR_TIMEOUT{15, "FR_TIMEOUT"};                         /* (15) Could not get a grant to access the volume within defined period */
    static constexpr Error FR_LOCKED{16, "FR_LOCKED"};                           /* (16) The operation is rejected according to the file sharing policy */
    static constexpr Error FR_NOT_ENOUGH_CORE{17, "FR_NOT_ENOUGH_CORE"};         /* (17) LFN working buffer could not be allocated */
    static constexpr Error FR_TOO_MANY_OPEN_FILES{18, "FR_TOO_MANY_OPEN_FILES"}; /* (18) Number of open files > _FS_LOCK */
    static constexpr Error FR_INVALID_PARAMETER{19, "FR_INVALID_PARAMETER"};     /* (19) Given parameter is invalid */
    static constexpr Error FILE_ALREADY_OPENED{20, "FILE_ALREADY_OPENED"};

    static constexpr Error END_OF_STREAM{99, "End of stream"};
    static constexpr Error THREAD_TERMINATED{100, "Terminated"};
    static constexpr Error NO_IO_EXCHANGE{101, "No IO exchange"};
    static constexpr Error NO_READER{102, "No reader"};
    static constexpr Error NO_WRITER{103, "No writer"};
    static constexpr Error READ_ERROR_GENERIC{104, "Read error"};
    static constexpr Error WRITE_ERROR_GENERIC{105, "Write error"};
    static constexpr Error READ_ERROR_CANNOT_READ_FROM_BASEBAND{106, "Cannot read from baseband"};
    static constexpr Error READ_ERROR_CANNOT_READ_FROM_APP{107, "Cannot read from app"};
    static constexpr Error WRITE_ERROR_CANNOT_WRITE_TO_BASEBAND{108, "Cannot write to baseband"};
    static constexpr Error WRITE_ERROR_CANNOT_WRITE_TO_APP{109, "Cannot write to app"};
    static constexpr Error TARGET_BUFFER_FULL{110, "Target buffer is full"};
    static constexpr Error TARGET_BUFFER_EMPTY{111, "Target buffer is empty"};

} // namespace errors