/*
 * Copyright (C) 2023 Bernd Herzog
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

#include "ch.h"
#include "hal.h"

#include <locale>
#include <codecvt>

#include "ff.h"
#include "file.hpp"

void cmd_sd_list_dir(BaseSequentialStream* chp, int argc, char* argv[]);
void cmd_sd_unlink(BaseSequentialStream* chp, int argc, char* argv[]);
void cmd_sd_mkdir(BaseSequentialStream* chp, int argc, char* argv[]);
void cmd_sd_filesize(BaseSequentialStream* chp, int argc, char* argv[]);
void cmd_sd_open(BaseSequentialStream* chp, int argc, char* argv[]);
void cmd_sd_seek(BaseSequentialStream* chp, int argc, char* argv[]);
void cmd_sd_close(BaseSequentialStream* chp, int argc, char* argv[]);
void cmd_sd_truncate(BaseSequentialStream* chp, int argc, char* argv[]);
void cmd_sd_sync(BaseSequentialStream* chp, int argc, char* argv[]);
void cmd_sd_tell(BaseSequentialStream* chp, int argc, char* argv[]);
void cmd_sd_read(BaseSequentialStream* chp, int argc, char* argv[]);
void cmd_sd_read_binary(BaseSequentialStream* chp, int argc, char* argv[]);
void cmd_sd_write(BaseSequentialStream* chp, int argc, char* argv[]);
void cmd_sd_write_binary(BaseSequentialStream* chp, int argc, char* argv[]);
void cmd_sd_crc32(BaseSequentialStream* chp, int argc, char* argv[]);

static std::filesystem::path path_from_string8(char* path) {
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;
    return conv.from_bytes(path);
}

// clang-format off
#define USB_SERIAL_SHELL_SD_COMMANDS   \
    {"ls", cmd_sd_list_dir},           \
    {"unlink", cmd_sd_unlink},         \
    {"mkdir", cmd_sd_mkdir},           \
    {"filesize", cmd_sd_filesize},     \
    {"fopen", cmd_sd_open},            \
    {"fseek", cmd_sd_seek},            \
    {"fclose", cmd_sd_close},          \
    {"ftruncate", cmd_sd_truncate},    \
    {"fsync", cmd_sd_sync},            \
    {"ftell", cmd_sd_tell},            \
    {"fread", cmd_sd_read},            \
    {"frb", cmd_sd_read_binary},       \
    {"fwrite", cmd_sd_write},          \
    {"fwb", cmd_sd_write_binary},      \
    {"crc32", cmd_sd_crc32}
// clang-format on
