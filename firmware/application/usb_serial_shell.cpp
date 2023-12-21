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

#include "usb_serial_shell.hpp"
#include "event_m0.hpp"
#include "baseband_api.hpp"
#include "core_control.hpp"
#include "bitmap.hpp"
#include "png_writer.hpp"
#include "irq_controls.hpp"

#include "usb_serial_io.h"
#include "ff.h"
#include "chprintf.h"
#include "chqueues.h"

#include <string>
#include <codecvt>
#include <cstring>
#include <locale>

#define SHELL_WA_SIZE THD_WA_SIZE(1024 * 3)
#define palOutputPad(port, pad) (LPC_GPIO->DIR[(port)] |= 1 << (pad))

// queue handler from ch
static msg_t qwait(GenericQueue* qp, systime_t time) {
    if (TIME_IMMEDIATE == time)
        return Q_TIMEOUT;
    currp->p_u.wtobjp = qp;
    queue_insert(currp, &qp->q_waiting);
    return chSchGoSleepTimeoutS(THD_STATE_WTQUEUE, time);
}

// This function fills the output buffer, and sends all data in 1 packet
static size_t fillOBuffer(OutputQueue* oqp, const uint8_t* bp, size_t n) {
    qnotify_t nfy = oqp->q_notify;
    size_t w = 0;

    chDbgCheck(n > 0, "chOQWriteTimeout");
    chSysLock();
    while (TRUE) {
        while (chOQIsFullI(oqp)) {
            if (qwait((GenericQueue*)oqp, TIME_INFINITE) != Q_OK) {
                chSysUnlock();
                return w;
            }
        }
        while (!chOQIsFullI(oqp) && n > 0) {
            oqp->q_counter--;
            *oqp->q_wrptr++ = *bp++;
            if (oqp->q_wrptr >= oqp->q_top)
                oqp->q_wrptr = oqp->q_buffer;
            w++;
            --n;
        }
        if (nfy) nfy(oqp);

        chSysUnlock(); /* Gives a preemption chance in a controlled point.*/
        if (n == 0) return w;
        chSysLock();
    }
}

static void cmd_reboot(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)chp;
    (void)argc;
    (void)argv;

    m4_request_shutdown();
    chThdSleepMilliseconds(50);

    LPC_RGU->RESET_CTRL[0] = (1 << 0);
}

static void cmd_dfu(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)chp;
    (void)argc;
    (void)argv;

    m4_request_shutdown();
    chThdSleepMilliseconds(50);

    LPC_SCU->SFSP2_8 = (LPC_SCU->SFSP2_8 & ~(7)) | 4;
    palOutputPad(5, 7);
    palSetPad(5, 7);

    LPC_RGU->RESET_CTRL[0] = (1 << 0);
}

static void cmd_hackrf(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)chp;
    (void)argc;
    (void)argv;

    m4_request_shutdown();
    chThdSleepMilliseconds(50);
    EventDispatcher::request_stop();
}

static void cmd_sd_over_usb(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)chp;
    (void)argc;
    (void)argv;

    ui::Painter painter;
    painter.fill_rectangle(
        {0, 0, portapack::display.width(), portapack::display.height()},
        ui::Color::black());

    painter.draw_bitmap(
        {portapack::display.width() / 2 - 8, portapack::display.height() / 2 - 8},
        ui::bitmap_icon_hackrf,
        ui::Color::yellow(),
        ui::Color::black());

    sdcDisconnect(&SDCD1);
    sdcStop(&SDCD1);

    m4_request_shutdown();
    chThdSleepMilliseconds(50);
    portapack::shutdown(true);
    m4_init(portapack::spi_flash::image_tag_usb_sd, portapack::memory::map::m4_code, false);
    m0_halt();
}

std::filesystem::path path_from_string8(char* path) {
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;
    return conv.from_bytes(path);
}

static void cmd_flash(BaseSequentialStream* chp, int argc, char* argv[]) {
    if (argc != 1) {
        chprintf(chp, "Usage: flash /FIRMWARE/portapack-h1_h2-mayhem.bin\r\n");
        return;
    }

    auto path = path_from_string8(argv[0]);
    size_t filename_length = strlen(argv[0]);

    if (!std::filesystem::file_exists(path)) {
        chprintf(chp, "file not found.\r\n");
        return;
    }

    std::memcpy(&shared_memory.bb_data.data[0], path.c_str(), (filename_length + 1) * 2);

    m4_request_shutdown();
    chThdSleepMilliseconds(50);
    m4_init(portapack::spi_flash::image_tag_flash_utility, portapack::memory::map::m4_code, false);
    m0_halt();
}

static void cmd_screenshot(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    ensure_directory("SCREENSHOTS");
    auto path = next_filename_matching_pattern(u"SCREENSHOTS/SCR_????.PNG");

    if (path.empty())
        return;

    PNGWriter png;
    auto error = png.create(path);
    if (error)
        return;

    for (int i = 0; i < ui::screen_height; i++) {
        std::array<ui::ColorRGB888, ui::screen_width> row;
        portapack::display.read_pixels({0, i, ui::screen_width, 1}, row);
        png.write_scanline(row);
    }

    chprintf(chp, "generated %s\r\n", path.string().c_str());
}

// gives full color.
static void cmd_screenframe(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    for (int i = 0; i < ui::screen_height; i++) {
        std::array<ui::ColorRGB888, ui::screen_width> row;
        portapack::display.read_pixels({0, i, ui::screen_width, 1}, row);
        for (int px = 0; px < ui::screen_width; px += 5) {
            char buffer[5 * 3 * 2];
            sprintf(buffer, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", row[px].r, row[px].g, row[px].b, row[px + 1].r, row[px + 1].g, row[px + 1].b, row[px + 2].r, row[px + 2].g, row[px + 2].b, row[px + 3].r, row[px + 3].g, row[px + 3].b, row[px + 4].r, row[px + 4].g, row[px + 4].b);
            fillOBuffer(&((SerialUSBDriver*)chp)->oqueue, (const uint8_t*)buffer, 5 * 3 * 2);
        }
        chprintf(chp, "\r\n");
    }
    chprintf(chp, "ok\r\n");
}

static char getChrFromRgb(uint8_t r, uint8_t g, uint8_t b) {
    uint8_t chR = r >> 6;  // 3bit
    uint8_t chG = g >> 6;  // 3bit
    uint8_t chB = b >> 6;  // 3bit
    uint8_t res = chR << 4 | chG << 2 | chB;
    res += 32;
    return res;
}

// sends only 1 byte (printable only) per pixel, so around 96 colors
static void cmd_screenframeshort(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    for (int y = 0; y < ui::screen_height; y++) {
        std::array<ui::ColorRGB888, ui::screen_width> row;
        portapack::display.read_pixels({0, y, ui::screen_width, 1}, row);
        for (int px = 0; px < ui::screen_width; px += 60) {
            char buffer[60];
            for (int i = 0; i < 60; ++i) {
                buffer[i] = getChrFromRgb(row[px + i].r, row[px + i].g, row[px + i].b);
            }
            fillOBuffer(&((SerialUSBDriver*)chp)->oqueue, (const uint8_t*)buffer, 60);
        }
        chprintf(chp, "\r\n");
    }

    chprintf(chp, "ok\r\n");
}
static void cmd_write_memory(BaseSequentialStream* chp, int argc, char* argv[]) {
    if (argc != 2) {
        chprintf(chp, "usage: write_memory <address> <value (1 or 4 bytes)>\r\n");
        chprintf(chp, "example: write_memory 0x40004008 0x00000002\r\n");
        return;
    }

    int value_length = strlen(argv[1]);
    if (value_length != 10 && value_length != 4) {
        chprintf(chp, "usage: write_memory <address> <value (1 or 4 bytes)>\r\n");
        chprintf(chp, "example: write_memory 0x40004008 0x00000002\r\n");
        return;
    }

    uint32_t address = (uint32_t)strtol(argv[0], NULL, 16);
    uint32_t value = (uint32_t)strtol(argv[1], NULL, 16);

    if (value_length == 10) {
        uint32_t* data_pointer = (uint32_t*)address;
        *data_pointer = value;
    } else {
        uint8_t* data_pointer = (uint8_t*)address;
        *data_pointer = (uint8_t)value;
    }

    chprintf(chp, "ok\r\n");
}

static void cmd_read_memory(BaseSequentialStream* chp, int argc, char* argv[]) {
    if (argc != 1) {
        chprintf(chp, "usage: read_memory 0x40004008\r\n");
        return;
    }

    int address = (int)strtol(argv[0], NULL, 16);
    uint32_t* data_pointer = (uint32_t*)address;

    chprintf(chp, "%x\r\n", *data_pointer);
}

static void cmd_button(BaseSequentialStream* chp, int argc, char* argv[]) {
    if (argc != 1) {
        chprintf(chp, "usage: button 1\r\n");
        return;
    }

    int button = (int)strtol(argv[0], NULL, 10);
    if (button < 1 || button > 8) {
        chprintf(chp, "usage: button <number 1 to 8>\r\n");
        return;
    }

    control::debug::inject_switch(button);

    chprintf(chp, "ok\r\n");
}

static void cmd_sd_list_dir(BaseSequentialStream* chp, int argc, char* argv[]) {
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

static void cmd_sd_delete(BaseSequentialStream* chp, int argc, char* argv[]) {
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

File* shell_file = nullptr;

static void cmd_sd_filesize(BaseSequentialStream* chp, int argc, char* argv[]) {
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

static void cmd_sd_open(BaseSequentialStream* chp, int argc, char* argv[]) {
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

static void cmd_sd_seek(BaseSequentialStream* chp, int argc, char* argv[]) {
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

static void cmd_sd_close(BaseSequentialStream* chp, int argc, char* argv[]) {
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

static void cmd_sd_read(BaseSequentialStream* chp, int argc, char* argv[]) {
    if (argc != 1) {
        chprintf(chp, "usage: read <number of bytes>\r\n");
        return;
    }

    if (shell_file == nullptr) {
        chprintf(chp, "no open file\r\n");
        return;
    }

    int size = (int)strtol(argv[0], NULL, 10);

    uint8_t buffer[16];

    do {
        File::Size bytes_to_read = size > 16 ? 16 : size;
        auto bytes_read = shell_file->read(buffer, bytes_to_read);
        if (bytes_read.is_error()) {
            chprintf(chp, "error %d\r\n", bytes_read.error());
            return;
        }

        for (size_t i = 0; i < bytes_read.value(); i++)
            chprintf(chp, "%02X", buffer[i]);

        chprintf(chp, "\r\n");

        if (bytes_to_read != bytes_read.value())
            return;

        size -= bytes_to_read;
    } while (size > 0);
    chprintf(chp, "ok\r\n");
}

static void cmd_sd_write(BaseSequentialStream* chp, int argc, char* argv[]) {
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

static const ShellCommand commands[] = {
    {"reboot", cmd_reboot},
    {"dfu", cmd_dfu},
    {"hackrf", cmd_hackrf},
    {"sd_over_usb", cmd_sd_over_usb},
    {"flash", cmd_flash},
    {"screenshot", cmd_screenshot},
    {"screenframe", cmd_screenframe},
    {"screenframeshort", cmd_screenframeshort},
    {"write_memory", cmd_write_memory},
    {"read_memory", cmd_read_memory},
    {"button", cmd_button},
    {"ls", cmd_sd_list_dir},
    {"rm", cmd_sd_delete},
    {"open", cmd_sd_open},
    {"seek", cmd_sd_seek},
    {"close", cmd_sd_close},
    {"read", cmd_sd_read},
    {"write", cmd_sd_write},
    {"filesize", cmd_sd_filesize},
    {NULL, NULL}};

static const ShellConfig shell_cfg1 = {
    (BaseSequentialStream*)&SUSBD1,
    commands};

void create_shell() {
    shellCreate(&shell_cfg1, SHELL_WA_SIZE, NORMALPRIO);
}
