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

#include "usb_serial_shell_filesystem.hpp"
#include "usb_serial_io.h"

#include "chprintf.h"
#include "string_format.hpp"
#include <cstring>

void cmd_sd_list_dir(BaseSequentialStream* chp, int argc, char* argv[]) {
    if (argc != 1) {
        chprintf(chp, "usage: ls /\r\n");
        return;
    }

    auto path = path_from_string8(argv[0]);

    for (const auto& entry : std::filesystem::directory_iterator(path, "*")) {
        if (std::filesystem::is_directory(entry.status())) {
            chprintf(chp, "%s/\r\n", entry.path().string().c_str());
        } else if (std::filesystem::is_regular_file(entry.status())) {
            chprintf(chp, "%s\r\n", entry.path().string().c_str());
        } else {
            chprintf(chp, "%s *\r\n", entry.path().string().c_str());
        }
    }
}

void cmd_sd_delete(BaseSequentialStream* chp, int argc, char* argv[]) {
    if (argc != 1) {
        chprintf(chp, "usage: rm <path>\r\n");
        return;
    }

    auto path = path_from_string8(argv[0]);

    if (!std::filesystem::file_exists(path)) {
        chprintf(chp, "file not found.\r\n");
        return;
    }

    delete_file(path);

    chprintf(chp, "ok\r\n");
}

static File* shell_file = nullptr;

void cmd_sd_filesize(BaseSequentialStream* chp, int argc, char* argv[]) {
    if (argc != 1) {
        chprintf(chp, "usage: filesize <path>\r\n");
        return;
    }
    auto path = path_from_string8(argv[0]);
    FILINFO res;
    auto stat = f_stat(path.tchar(), &res);
    if (stat == FR_OK) {
        chprintf(chp, "%lu\r\n", res.fsize);
        chprintf(chp, "ok\r\n");
    } else {
        chprintf(chp, "error\r\n");
    }
}

void cmd_sd_open(BaseSequentialStream* chp, int argc, char* argv[]) {
    if (argc != 1) {
        chprintf(chp, "usage: open <path>\r\n");
        return;
    }

    if (shell_file != nullptr) {
        chprintf(chp, "file already open\r\n");
        return;
    }

    auto path = path_from_string8(argv[0]);
    shell_file = new File();
    shell_file->open(path, false, true);

    chprintf(chp, "ok\r\n");
}

void cmd_sd_seek(BaseSequentialStream* chp, int argc, char* argv[]) {
    if (argc != 1) {
        chprintf(chp, "usage: seek <offset>\r\n");
        return;
    }

    if (shell_file == nullptr) {
        chprintf(chp, "no open file\r\n");
        return;
    }

    int address = (int)strtol(argv[0], NULL, 10);
    shell_file->seek(address);

    chprintf(chp, "ok\r\n");
}

void cmd_sd_close(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)argv;

    if (argc != 0) {
        chprintf(chp, "usage: close\r\n");
        return;
    }

    if (shell_file == nullptr) {
        chprintf(chp, "no open file\r\n");
        return;
    }

    delete shell_file;
    shell_file = nullptr;

    chprintf(chp, "ok\r\n");
}

void cmd_sd_read(BaseSequentialStream* chp, int argc, char* argv[]) {
    if (argc != 1) {
        chprintf(chp, "usage: read <number of bytes>\r\n");
        return;
    }

    if (shell_file == nullptr) {
        chprintf(chp, "no open file\r\n");
        return;
    }

    int size = (int)strtol(argv[0], NULL, 10);

    uint8_t buffer[62];

    do {
        File::Size bytes_to_read = size > 62 ? 62 : size;
        auto bytes_read = shell_file->read(buffer, bytes_to_read);
        if (bytes_read.is_error()) {
            chprintf(chp, "error %d\r\n", bytes_read.error());
            return;
        }
        std::string res = to_string_hex_array(buffer, bytes_read.value());
        res += "\r\n";
        fillOBuffer(&((SerialUSBDriver*)chp)->oqueue, (const uint8_t*)res.c_str(), res.size());
        if (bytes_to_read != bytes_read.value())
            return;

        size -= bytes_to_read;
    } while (size > 0);
    chprintf(chp, "ok\r\n");
}

void cmd_sd_write(BaseSequentialStream* chp, int argc, char* argv[]) {
    const char* usage = "usage: write 0123456789ABCDEF\r\n";
    if (argc != 1) {
        chprintf(chp, usage);
        return;
    }

    if (shell_file == nullptr) {
        chprintf(chp, "no open file\r\n");
        return;
    }

    size_t data_string_len = strlen(argv[0]);
    if (data_string_len % 2 != 0) {
        chprintf(chp, usage);
        return;
    }

    for (size_t i = 0; i < data_string_len; i++) {
        char c = argv[0][i];
        if ((c < '0' || c > '9') && (c < 'A' || c > 'F')) {
            chprintf(chp, usage);
            return;
        }
    }

    char buffer[3] = {0, 0, 0};

    for (size_t i = 0; i < data_string_len / 2; i++) {
        buffer[0] = argv[0][i * 2];
        buffer[1] = argv[0][i * 2 + 1];
        uint8_t value = (uint8_t)strtol(buffer, NULL, 16);
        shell_file->write(&value, 1);
    }

    chprintf(chp, "ok\r\n");
}
