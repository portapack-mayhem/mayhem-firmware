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
#include "usb_serial_device_to_host.h"

#include "chprintf.h"
#include "string_format.hpp"
#include <cstring>

#include "crc.hpp"

static File* shell_file = nullptr;

static bool report_on_error(BaseSequentialStream* chp, File::Error& error) {
    if (error.ok() == false) {
        chprintf(chp, "Error calling delete_file: %d %s\r\n", error.code(), error.what().c_str());
        return true;
    }

    return false;
}

static bool report_on_error(BaseSequentialStream* chp, FRESULT error_code) {
    std::filesystem::filesystem_error error = error_code;
    return report_on_error(chp, error);
}

static bool report_on_error(BaseSequentialStream* chp, Optional<File::Error>& error) {
    if (error.is_valid())
        return report_on_error(chp, error.value());

    return false;
}

static bool report_on_error(BaseSequentialStream* chp, File::Result<uint64_t> error) {
    if (error.is_error()) {
        return report_on_error(chp, error.error());
    }

    return false;
}

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

void cmd_sd_unlink(BaseSequentialStream* chp, int argc, char* argv[]) {
    if (argc != 1) {
        chprintf(chp, "usage: unlink <path>\r\n");
        return;
    }

    auto path = path_from_string8(argv[0]);
    auto error = delete_file(path);
    if (report_on_error(chp, error)) return;

    chprintf(chp, "ok\r\n");
}

void cmd_sd_mkdir(BaseSequentialStream* chp, int argc, char* argv[]) {
    if (argc != 1) {
        chprintf(chp, "usage: mkdir <path>\r\n");
        return;
    }

    auto path = path_from_string8(argv[0]);

    if (std::filesystem::is_directory(path)) {
        chprintf(chp, "directory already exists.\r\n");
        return;
    }

    auto error = make_new_directory(path);
    if (report_on_error(chp, error)) return;

    chprintf(chp, "ok\r\n");
}

void cmd_sd_filesize(BaseSequentialStream* chp, int argc, char* argv[]) {
    if (argc != 1) {
        chprintf(chp, "usage: filesize <path>\r\n");
        return;
    }
    auto path = path_from_string8(argv[0]);
    FILINFO res;
    auto stat = f_stat(path.tchar(), &res);
    if (report_on_error(chp, stat)) return;

    chprintf(chp, "%lu\r\n", res.fsize);
    chprintf(chp, "ok\r\n");
}

void cmd_sd_open(BaseSequentialStream* chp, int argc, char* argv[]) {
    if (argc != 1) {
        chprintf(chp, "usage: fopen <path>\r\n");
        return;
    }

    if (shell_file != nullptr) {
        chprintf(chp, "file already open\r\n");
        return;
    }

    auto path = path_from_string8(argv[0]);
    shell_file = new File();
    auto error = shell_file->open(path, false, true);
    if (report_on_error(chp, error)) return;

    chprintf(chp, "ok\r\n");
}

void cmd_sd_seek(BaseSequentialStream* chp, int argc, char* argv[]) {
    if (argc != 1) {
        chprintf(chp, "usage: fseek <offset>\r\n");
        return;
    }

    if (shell_file == nullptr) {
        chprintf(chp, "no open file\r\n");
        return;
    }

    int address = (int)strtol(argv[0], NULL, 10);
    auto error = shell_file->seek(address);
    if (report_on_error(chp, error)) return;

    chprintf(chp, "ok\r\n");
}

void cmd_sd_close(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)argv;

    if (argc != 0) {
        chprintf(chp, "usage: fclose\r\n");
        return;
    }

    if (shell_file == nullptr) {
        chprintf(chp, "no open file\r\n");
        return;
    }

    auto error = shell_file->sync();
    if (report_on_error(chp, error)) return;

    delete shell_file;
    shell_file = nullptr;

    chprintf(chp, "ok\r\n");
}

void cmd_sd_truncate(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)argv;

    if (argc != 0) {
        chprintf(chp, "usage: ftruncate\r\n");
        return;
    }

    if (shell_file == nullptr) {
        chprintf(chp, "no open file\r\n");
        return;
    }

    auto error = shell_file->truncate();
    if (report_on_error(chp, error)) return;

    chprintf(chp, "ok\r\n");
}

void cmd_sd_sync(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)argv;

    if (argc != 0) {
        chprintf(chp, "usage: fsync\r\n");
        return;
    }

    if (shell_file == nullptr) {
        chprintf(chp, "no open file\r\n");
        return;
    }

    auto error = shell_file->sync();
    if (report_on_error(chp, error)) return;

    chprintf(chp, "ok\r\n");
}

void cmd_sd_tell(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)argv;

    if (argc != 0) {
        chprintf(chp, "usage: ftell\r\n");
        return;
    }

    if (shell_file == nullptr) {
        chprintf(chp, "no open file\r\n");
        return;
    }

    auto current_position = shell_file->tell();

    chprintf(chp, "%d\r\n", current_position);
    chprintf(chp, "ok\r\n");
}

void cmd_sd_read(BaseSequentialStream* chp, int argc, char* argv[]) {
    if (argc != 1) {
        chprintf(chp, "usage: fread <number of bytes>\r\n");
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
        if (report_on_error(chp, bytes_read)) return;

        std::string res = to_string_hex_array(buffer, bytes_read.value());
        res += "\r\n";
        fillOBuffer(&((SerialUSBDriver*)chp)->oqueue, (const uint8_t*)res.c_str(), res.size());
        if (bytes_to_read != bytes_read.value())
            return;

        size -= bytes_to_read;
    } while (size > 0);
    chprintf(chp, "ok\r\n");
}

void cmd_sd_read_binary(BaseSequentialStream* chp, int argc, char* argv[]) {
    if (argc != 1) {
        chprintf(chp, "usage: frb <number of bytes>\r\n");
        return;
    }

    if (shell_file == nullptr) {
        chprintf(chp, "no open file\r\n");
        return;
    }

    int size = (int)strtol(argv[0], NULL, 10);

    uint8_t buffer[64];

    do {
        File::Size bytes_to_read = size > 64 ? 64 : size;
        auto bytes_read = shell_file->read(buffer, bytes_to_read);
        if (report_on_error(chp, bytes_read)) return;

        if (bytes_read.value() > 0)
            fillOBuffer(&((SerialUSBDriver*)chp)->oqueue, buffer, bytes_read.value());

        if (bytes_to_read != bytes_read.value())
            return;

        size -= bytes_to_read;
    } while (size > 0);
    chprintf(chp, "\r\nok\r\n");
}

void cmd_sd_write(BaseSequentialStream* chp, int argc, char* argv[]) {
    const char* usage = "usage: fwrite 0123456789ABCDEF\r\n";
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
        auto error = shell_file->write(&value, 1);
        if (report_on_error(chp, error)) return;
    }

    chprintf(chp, "ok\r\n");
}

void cmd_sd_write_binary(BaseSequentialStream* chp, int argc, char* argv[]) {
    const char* usage = "usage: fwb <number of bytes>\r\nfollowed by <number of bytes> of data";
    if (argc != 1) {
        chprintf(chp, usage);
        return;
    }

    if (shell_file == nullptr) {
        chprintf(chp, "no open file\r\n");
        return;
    }

    size_t size = (size_t)strtol(argv[0], NULL, 10);

    chprintf(chp, "send %d bytes\r\n", size);

    uint8_t buffer[USB_BULK_BUFFER_SIZE];

    do {
        size_t bytes_to_read = size > USB_BULK_BUFFER_SIZE ? USB_BULK_BUFFER_SIZE : size;
        size_t bytes_read = chSequentialStreamRead(chp, &buffer[0], bytes_to_read);
        if (bytes_read != bytes_to_read)
            return;

        auto error = shell_file->write(&buffer[0], bytes_read);
        if (report_on_error(chp, error)) return;

        size -= bytes_read;
    } while (size > 0);

    chprintf(chp, "ok\r\n");
}

void cmd_sd_crc32(BaseSequentialStream* chp, int argc, char* argv[]) {
    if (argc != 1) {
        chprintf(chp, "usage: crc32 <path>\r\n");
        return;
    }

    auto path = path_from_string8(argv[0]);
    File* crc_file = new File();
    auto error = crc_file->open(path, true, false);
    if (report_on_error(chp, error)) return;

    uint8_t buffer[64];
    CRC<32> crc{0x04c11db7, 0xffffffff, 0xffffffff};

    while (true) {
        auto bytes_read = crc_file->read(buffer, 64);
        if (report_on_error(chp, bytes_read)) return;

        if (bytes_read.value() > 0) {
            crc.process_bytes((void*)buffer, bytes_read.value());
        }

        if (64 != bytes_read.value()) {
            chprintf(chp, "CRC32: 0x%08X\r\n", crc.checksum());
            return;
        }
    }

    delete crc_file;
}
