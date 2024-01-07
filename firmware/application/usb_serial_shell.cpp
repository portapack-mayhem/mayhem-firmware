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

#include "portapack.hpp"
#include "portapack_hal.hpp"
#include "hackrf_gpio.hpp"
#include "jtag_target_gpio.hpp"
#include "cpld_max5.hpp"
#include "portapack_cpld_data.hpp"
#include "crc.hpp"
#include "hackrf_cpld_data.hpp"

#include "usb_serial_io.h"
#include "ff.h"
#include "chprintf.h"
#include "chqueues.h"
#include "ui_external_items_menu_loader.hpp"
#include "untar.hpp"
#include "ui_widget.hpp"

#include "ui_navigation.hpp"

#include <string>
#include <codecvt>
#include <cstring>
#include <locale>
#include <libopencm3/lpc43xx/wwdt.h>

#define SHELL_WA_SIZE THD_WA_SIZE(1024 * 3)
#define palOutputPad(port, pad) (LPC_GPIO->DIR[(port)] |= 1 << (pad))

static EventDispatcher* _eventDispatcherInstance = NULL;
static EventDispatcher* getEventDispatcherInstance() {
    return _eventDispatcherInstance;
}

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

bool strEndsWith(const std::u16string& str, const std::u16string& suffix) {
    if (str.length() >= suffix.length()) {
        std::u16string endOfString = str.substr(str.length() - suffix.length());
        return endOfString == suffix;
    } else {
        return false;
    }
}

static void cmd_flash(BaseSequentialStream* chp, int argc, char* argv[]) {
    if (argc != 1) {
        chprintf(chp, "Usage: flash /FIRMWARE/portapack-h1_h2-mayhem.bin\r\n");
        return;
    }

    auto path = path_from_string8(argv[0]);

    if (!std::filesystem::file_exists(path)) {
        chprintf(chp, "file not found.\r\n");
        return;
    }

    // check file extensions
    if (strEndsWith(path.native(), u".ppfw.tar")) {
        // extract tar
        chprintf(chp, "Extracting TAR file.\r\n");
        auto res = UnTar::untar(
            path.native(), [chp](const std::string fileName) {
                chprintf(chp, fileName.c_str());
                chprintf(chp, "\r\n");
            });
        if (res.empty()) {
            chprintf(chp, "error bad TAR file.\r\n");
            return;
        }
        path = res;  // it will contain the last bin file in tar
    } else if (strEndsWith(path.native(), u".bin")) {
        // nothing to do for this case yet.
    } else {
        chprintf(chp, "error only .bin or .ppfw.tar files canbe flashed.\r\n");
        return;
    }
    chprintf(chp, "Flashing: ");
    chprintf(chp, path.string().c_str());
    chprintf(chp, "\r\n");
    chThdSleepMilliseconds(50);
    std::memcpy(&shared_memory.bb_data.data[0], path.native().c_str(), (path.native().length() + 1) * 2);
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

    auto evtd = getEventDispatcherInstance();
    evtd->enter_shell_working_mode();

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

    evtd->exit_shell_working_mode();

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

    auto evtd = getEventDispatcherInstance();
    evtd->enter_shell_working_mode();

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

    evtd->exit_shell_working_mode();
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

    // Wait two frame syncs to ensure action has painted
    auto evtd = getEventDispatcherInstance();
    evtd->wait_finish_frame();
    evtd->wait_finish_frame();

    chprintf(chp, "ok\r\n");
}

static void cmd_touch(BaseSequentialStream* chp, int argc, char* argv[]) {
    if (argc != 2) {
        chprintf(chp, "usage: touch x y\r\n");
        return;
    }

    int x = (int)strtol(argv[0], NULL, 10);
    int y = (int)strtol(argv[1], NULL, 10);
    if (x < 0 || x > ui::screen_width || y < 0 || y > ui::screen_height) {
        chprintf(chp, "usage: touch x y\r\n");
        return;
    }

    auto evtd = getEventDispatcherInstance();
    if (evtd == NULL) {
        chprintf(chp, "error\r\n");
    }
    evtd->emulateTouch({{x, y}, ui::TouchEvent::Type::Start});
    evtd->emulateTouch({{x, y}, ui::TouchEvent::Type::End});

    // Wait two frame syncs to ensure action has painted
    evtd->wait_finish_frame();
    evtd->wait_finish_frame();

    chprintf(chp, "ok\r\n");
}

// send ascii keys in 2 char hex representation. Can send multiple keys at once like: keyboard 414243 (this will be ABC)
static void cmd_keyboard(BaseSequentialStream* chp, int argc, char* argv[]) {
    if (argc != 1) {
        chprintf(chp, "usage: keyboard XX\r\n");
        return;
    }

    auto evtd = getEventDispatcherInstance();
    if (evtd == NULL) {
        chprintf(chp, "error\r\n");
    }

    size_t data_string_len = strlen(argv[0]);
    if (data_string_len % 2 != 0) {
        chprintf(chp, "usage: keyboard XXXX\r\n");
        return;
    }

    for (size_t i = 0; i < data_string_len; i++) {
        char c = argv[0][i];
        if ((c < '0' || c > '9') && (c < 'A' || c > 'F')) {
            chprintf(chp, "usage: keyboard XX\r\n");
            return;
        }
    }

    char buffer[3] = {0, 0, 0};

    for (size_t i = 0; i < data_string_len / 2; i++) {
        buffer[0] = argv[0][i * 2];
        buffer[1] = argv[0][i * 2 + 1];
        uint8_t chr = (uint8_t)strtol(buffer, NULL, 16);
        evtd->emulateKeyboard(chr);
    }

    // Wait two frame syncs to ensure action has painted
    evtd->wait_finish_frame();
    evtd->wait_finish_frame();

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

static void cmd_rtcget(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)chp;
    (void)argc;
    (void)argv;

    rtc::RTC datetime;
    rtcGetTime(&RTCD1, &datetime);

    chprintf(chp, "Current time: %04d-%02d-%02d %02d:%02d:%02d\r\n", datetime.year(), datetime.month(), datetime.day(), datetime.hour(), datetime.minute(), datetime.second());
}

static void cmd_rtcset(BaseSequentialStream* chp, int argc, char* argv[]) {
    const char* usage =
        "usage: rtcset [year] [month] [day] [hour] [minute] [second]\r\n"
        "  all fields are required; milliseconds zero when set\r\n"
        "  (fractional seconds are not supported)\r\n";

    if (argc != 6) {
        chprintf(chp, usage);
        return;
    }

    rtc::RTC new_datetime{
        (uint16_t)strtol(argv[0], NULL, 10), (uint8_t)strtol(argv[1], NULL, 10),
        (uint8_t)strtol(argv[2], NULL, 10), (uint32_t)strtol(argv[3], NULL, 10),
        (uint32_t)strtol(argv[4], NULL, 10), (uint32_t)strtol(argv[5], NULL, 10)};
    rtcSetTime(&RTCD1, &new_datetime);

    chprintf(chp, "ok\r\n");
}

static void cpld_info(BaseSequentialStream* chp, int argc, char* argv[]) {
    const char* usage =
        "usage: cpld_info <device>\r\n"
        "  supported modes:\r\n"
        "    cpld_info hackrf\r\n"
        "    cpld_info portapack\r\n";
    if (argc != 1) {
        chprintf(chp, usage);
        return;
    }

    if (strncmp(argv[0], "hackrf", 5) == 0) {
        jtag::GPIOTarget jtag_target_hackrf_cpld{
            hackrf::one::gpio_cpld_tck,
            hackrf::one::gpio_cpld_tms,
            hackrf::one::gpio_cpld_tdi,
            hackrf::one::gpio_cpld_tdo,
        };

        hackrf::one::cpld::CPLD hackrf_cpld{jtag_target_hackrf_cpld};
        {
            CRC<32> crc{0x04c11db7, 0xffffffff, 0xffffffff};

            hackrf_cpld.prepare_read_eeprom();

            for (const auto& block : hackrf::one::cpld::verify_blocks) {
                auto from_device = hackrf_cpld.read_block_eeprom(block.id);

                for (std::array<bool, 274UL>::reverse_iterator i = from_device.rbegin(); i != from_device.rend(); ++i) {
                    auto bit = *i;
                    crc.process_bit(bit);
                }
            }

            hackrf_cpld.finalize_read_eeprom();
            chprintf(chp, "CPLD eeprom firmware checksum: 0x%08X\r\n", crc.checksum());
        }

        {
            CRC<32> crc{0x04c11db7, 0xffffffff, 0xffffffff};

            hackrf_cpld.prepare_read_sram();

            for (const auto& block : hackrf::one::cpld::verify_blocks) {
                auto from_device = hackrf_cpld.read_block_sram(block);

                for (std::array<bool, 274UL>::reverse_iterator i = from_device.rbegin(); i != from_device.rend(); ++i) {
                    auto bit = *i;
                    crc.process_bit(bit);
                }
            }

            hackrf_cpld.finalize_read_sram(hackrf::one::cpld::verify_blocks[0].id);
            chprintf(chp, "CPLD sram firmware checksum: 0x%08X\r\n", crc.checksum());
        }

    } else if (strncmp(argv[0], "portapack", 5) == 0) {
        jtag::GPIOTarget target{
            portapack::gpio_cpld_tck,
            portapack::gpio_cpld_tms,
            portapack::gpio_cpld_tdi,
            portapack::gpio_cpld_tdo};
        jtag::JTAG jtag{target};
        portapack::cpld::CPLD cpld{jtag};

        cpld.reset();
        cpld.run_test_idle();
        uint32_t idcode = cpld.get_idcode();

        chprintf(chp, "CPLD IDCODE: 0x%08X\r\n", idcode);

        if (idcode == 0x20A50DD) {
            chprintf(chp, "CPLD Model: Altera MAX V 5M40Z\r\n");
            cpld.reset();
            cpld.run_test_idle();
            cpld.sample();
            cpld.bypass();
            cpld.enable();

            CRC<32> crc{0x04c11db7, 0xffffffff, 0xffffffff};
            cpld.prepare_read(0x0000);

            for (size_t i = 0; i < 3328; i++) {
                uint16_t data = cpld.read();
                crc.process_byte((data >> 0) & 0xff);
                crc.process_byte((data >> 8) & 0xff);
                crc.process_byte((data >> 16) & 0xff);
                crc.process_byte((data >> 24) & 0xff);
            }

            cpld.prepare_read(0x0001);

            for (size_t i = 0; i < 512; i++) {
                uint16_t data = cpld.read();
                crc.process_byte((data >> 0) & 0xff);
                crc.process_byte((data >> 8) & 0xff);
                crc.process_byte((data >> 16) & 0xff);
                crc.process_byte((data >> 24) & 0xff);
            }

            chprintf(chp, "CPLD firmware checksum: 0x%08X\r\n", crc.checksum());

            m4_request_shutdown();
            chThdSleepMilliseconds(1000);

            WWDT_MOD = WWDT_MOD_WDEN | WWDT_MOD_WDRESET;
            WWDT_TC = 100000 & 0xFFFFFF;
            WWDT_FEED_SEQUENCE;

        } else if (idcode == 0x00025610) {
            chprintf(chp, "CPLD Model: AGM AG256SL100\r\n");

            if (cpld.AGM_enter_maintenance_mode() == false) {
                return;
            }

            cpld.AGM_enter_read_mode();

            CRC<32> crc{0x04c11db7, 0xffffffff, 0xffffffff};
            for (size_t i = 0; i < 2048; i++) {
                uint32_t data = cpld.AGM_read(i);
                crc.process_byte((data >> 0) & 0xff);
                crc.process_byte((data >> 8) & 0xff);
                crc.process_byte((data >> 16) & 0xff);
                crc.process_byte((data >> 24) & 0xff);
            }

            cpld.AGM_exit_maintenance_mode();

            chprintf(chp, "CPLD firmware checksum: 0x%08X\r\n", crc.checksum());

            m4_request_shutdown();
            chThdSleepMilliseconds(1000);

            WWDT_MOD = WWDT_MOD_WDEN | WWDT_MOD_WDRESET;
            WWDT_TC = 100000 & 0xFFFFFF;
            WWDT_FEED_SEQUENCE;

        } else {
            chprintf(chp, "CPLD Model: unknown\r\n");
        }

    } else {
        chprintf(chp, usage);
    }
}

// walks throught the given widget's childs in recurse to get all support text and pass it to a callback function
static void widget_collect_accessibility(BaseSequentialStream* chp, ui::Widget* w, void (*callback)(BaseSequentialStream*, const std::string&, const std::string&), ui::Widget* focusedWidget) {
    for (auto child : w->children()) {
        if (!child->hidden()) {
            std::string res = "";
            child->getAccessibilityText(res);
            std::string strtype = "";
            child->getWidgetName(strtype);
            if (child == focusedWidget) strtype += "*";
            if (callback != NULL && !res.empty()) callback(chp, res, strtype);
            widget_collect_accessibility(chp, child, callback, focusedWidget);
        }
    }
}

// callback when it found any response from a widget
static void accessibility_callback(BaseSequentialStream* chp, const std::string& strResult, const std::string& wgType) {
    if (!wgType.empty()) {
        chprintf(chp, "[");
        chprintf(chp, wgType.c_str());
        chprintf(chp, "] ");
    }
    chprintf(chp, "%s\r\n", strResult.c_str());
}

// gets all widget's accessibility helper text
static void cmd_accessibility_readall(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    auto evtd = getEventDispatcherInstance();
    if (evtd == NULL) {
        chprintf(chp, "error Can't get Event Dispatcherr\n");
        return;
    }
    auto wg = evtd->getTopWidget();
    if (wg == NULL) {
        chprintf(chp, "error Can't get top Widget\r\n");
        return;
    }
    auto focused = evtd->getFocusedWidget();
    widget_collect_accessibility(chp, wg, accessibility_callback, focused);
    chprintf(chp, "ok\r\n");
}

// gets focused widget's accessibility helper text
static void cmd_accessibility_readcurr(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    auto evtd = getEventDispatcherInstance();
    if (evtd == NULL) {
        chprintf(chp, "error Can't get Event Dispatcher\r\n");
        return;
    }
    auto wg = evtd->getFocusedWidget();
    if (wg == NULL) {
        chprintf(chp, "error Can't get focused Widget\r\n");
        return;
    }
    std::string res = "";
    wg->getAccessibilityText(res);
    if (res.empty()) {
        // try with parent
        wg = wg->parent();
        if (wg == NULL) {
            chprintf(chp, "error Widget not providing accessibility info\r\n");
            return;
        }
        wg->getAccessibilityText(res);
        if (res.empty()) {
            chprintf(chp, "error Widget not providing accessibility info\r\n");
            return;
        }
    }
    std::string strtype = "";
    wg->getWidgetName(strtype);
    accessibility_callback(chp, res, strtype);
    chprintf(chp, "\r\nok\r\n");
}

static void cmd_appstart(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    if (argc != 1) {
        chprintf(chp, "Usage: appstart APPCALLNAME");
        return;
    }
    auto evtd = getEventDispatcherInstance();
    if (!evtd) return;
    auto top_widget = evtd->getTopWidget();
    if (!top_widget) return;
    auto nav = static_cast<ui::SystemView*>(top_widget)->get_navigation_view();
    if (!nav) return;
    if (nav->StartAppByName(argv[0])) {
        chprintf(chp, "ok\r\n");
        return;
    }
    // since ext app loader changed, we can just pass the string to it, and it"ll return if started or not.
    std::string appwithpath = "/APPS/";
    appwithpath += argv[0];
    appwithpath += ".ppma";
    bool ret = ui::ExternalItemsMenuLoader::run_external_app(*nav, path_from_string8((char*)appwithpath.c_str()));
    if (!ret) {
        chprintf(chp, "error\r\n");
        return;
    }
    chprintf(chp, "ok\r\n");
}

static void printAppInfo(BaseSequentialStream* chp, ui::AppInfoConsole& element) {
    if (strlen(element.appCallName) == 0) return;
    chprintf(chp, element.appCallName);
    chprintf(chp, " ");
    chprintf(chp, element.appFriendlyName);
    chprintf(chp, " ");
    switch (element.appLocation) {
        case RX:
            chprintf(chp, "[RX]\r\n");
            break;
        case TX:
            chprintf(chp, "[TX]\r\n");
            break;
        case UTILITIES:
            chprintf(chp, "[UTIL]\r\n");
            break;
        case DEBUG:
            chprintf(chp, "[DEBUG]\r\n");
            break;
        default:
            break;
    }
}

// returns the installed apps, those can be called by appstart APPNAME
static void cmd_applist(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    auto evtd = getEventDispatcherInstance();
    if (!evtd) return;
    auto top_widget = evtd->getTopWidget();
    if (!top_widget) return;
    auto nav = static_cast<ui::SystemView*>(top_widget)->get_navigation_view();
    if (!nav) return;
    for (auto element : ui::NavigationView::fixedAppListFC) {
        printAppInfo(chp, element);
    }
    ui::ExternalItemsMenuLoader::load_all_external_items_callback([chp](ui::AppInfoConsole& info) {
        printAppInfo(chp, info);
    });
    chprintf(chp, "ok\r\n");
}

static void cmd_cpld_read(BaseSequentialStream* chp, int argc, char* argv[]) {
    const char* usage =
        "usage: cpld_read <device> <target>\r\n"
        "  device can be: hackrf, portapack\r\n"
        "  target can be: sram (hackrf only), eeprom\r\n";

    if (argc != 2) {
        chprintf(chp, usage);
        return;
    }

    if (strncmp(argv[0], "hackrf", 5) == 0) {
        if (strncmp(argv[1], "eeprom", 5) == 0) {
            jtag::GPIOTarget jtag_target_hackrf_cpld{
                hackrf::one::gpio_cpld_tck,
                hackrf::one::gpio_cpld_tms,
                hackrf::one::gpio_cpld_tdi,
                hackrf::one::gpio_cpld_tdo,
            };

            hackrf::one::cpld::CPLD hackrf_cpld{jtag_target_hackrf_cpld};

            hackrf_cpld.prepare_read_eeprom();

            for (const auto& block : hackrf::one::cpld::verify_blocks) {
                auto from_device = hackrf_cpld.read_block_eeprom(block.id);

                chprintf(chp, "bank %04X: ", block.id);
                uint32_t n = 6;
                uint8_t byte = 0;
                for (std::array<bool, 274UL>::reverse_iterator i = from_device.rbegin(); i != from_device.rend(); ++i) {
                    auto bit = *i;
                    byte |= bit << (7 - (n % 8));
                    if (n % 8 == 7) {
                        chprintf(chp, "%02X ", byte);
                        byte = 0;
                    }
                    n++;
                }
                chprintf(chp, "\r\n");
            }

            hackrf_cpld.finalize_read_eeprom();
        }

        else if (strncmp(argv[1], "sram", 5) == 0) {
            jtag::GPIOTarget jtag_target_hackrf_cpld{
                hackrf::one::gpio_cpld_tck,
                hackrf::one::gpio_cpld_tms,
                hackrf::one::gpio_cpld_tdi,
                hackrf::one::gpio_cpld_tdo,
            };

            hackrf::one::cpld::CPLD hackrf_cpld{jtag_target_hackrf_cpld};

            hackrf_cpld.prepare_read_sram();

            for (const auto& block : hackrf::one::cpld::verify_blocks) {
                auto from_device = hackrf_cpld.read_block_sram(block);

                chprintf(chp, "bank %04X: ", block.id);
                uint32_t n = 6;
                uint8_t byte = 0;
                for (std::array<bool, 274UL>::reverse_iterator i = from_device.rbegin(); i != from_device.rend(); ++i) {
                    auto bit = *i;
                    byte |= bit << (7 - (n % 8));
                    if (n % 8 == 7) {
                        chprintf(chp, "%02X ", byte);
                        byte = 0;
                    }
                    n++;
                }
                chprintf(chp, "\r\n");
            }

            hackrf_cpld.finalize_read_sram(hackrf::one::cpld::verify_blocks[0].id);
        }
    } else if (strncmp(argv[0], "portapack", 5) == 0) {
        jtag::GPIOTarget target{
            portapack::gpio_cpld_tck,
            portapack::gpio_cpld_tms,
            portapack::gpio_cpld_tdi,
            portapack::gpio_cpld_tdo};
        jtag::JTAG jtag{target};
        portapack::cpld::CPLD cpld{jtag};

        cpld.reset();
        cpld.run_test_idle();
        uint32_t idcode = cpld.get_idcode();

        chprintf(chp, "CPLD IDCODE: 0x%08X\r\n", idcode);

        if (idcode == 0x20A50DD) {
            chprintf(chp, "CPLD Model: Altera MAX V 5M40Z\r\n");
            cpld.reset();
            cpld.run_test_idle();
            cpld.sample();
            cpld.bypass();
            cpld.enable();

            cpld.prepare_read(0x0000);

            for (size_t i = 0; i < 3328; i++) {
                uint16_t data = cpld.read();
                chprintf(chp, "%d: 0x%04X\r\n", i, data);
            }

            cpld.prepare_read(0x0001);

            for (size_t i = 0; i < 512; i++) {
                uint16_t data = cpld.read();
                chprintf(chp, "%d: 0x%04X\r\n", i, data);
            }

            m4_request_shutdown();
            chThdSleepMilliseconds(1000);

            WWDT_MOD = WWDT_MOD_WDEN | WWDT_MOD_WDRESET;
            WWDT_TC = 100000 & 0xFFFFFF;
            WWDT_FEED_SEQUENCE;

        } else if (idcode == 0x00025610) {
            chprintf(chp, "CPLD Model: AGM AG256SL100\r\n");

            if (cpld.AGM_enter_maintenance_mode() == false) {
                return;
            }

            cpld.AGM_enter_read_mode();

            for (size_t i = 0; i < 2048; i++) {
                uint32_t data = cpld.AGM_read(i);
                if (i % 4 == 0)
                    chprintf(chp, "%5d: ", i * 4);

                chprintf(chp, "0x%08X", data);

                if (i % 4 == 3)
                    chprintf(chp, "\r\n");
                else
                    chprintf(chp, " ");
            }

            cpld.AGM_exit_maintenance_mode();

            m4_request_shutdown();
            chThdSleepMilliseconds(1000);

            WWDT_MOD = WWDT_MOD_WDEN | WWDT_MOD_WDRESET;
            WWDT_TC = 100000 & 0xFFFFFF;
            WWDT_FEED_SEQUENCE;

        } else {
            chprintf(chp, "CPLD Model: unknown\r\n");
        }

    } else {
        chprintf(chp, usage);
    }
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
    {"touch", cmd_touch},
    {"keyboard", cmd_keyboard},
    {"ls", cmd_sd_list_dir},
    {"rm", cmd_sd_delete},
    {"open", cmd_sd_open},
    {"seek", cmd_sd_seek},
    {"close", cmd_sd_close},
    {"read", cmd_sd_read},
    {"write", cmd_sd_write},
    {"filesize", cmd_sd_filesize},
    {"rtcget", cmd_rtcget},
    {"rtcset", cmd_rtcset},
    {"cpld_info", cpld_info},
    {"cpld_read", cmd_cpld_read},
    {"accessibility_readall", cmd_accessibility_readall},
    {"accessibility_readcurr", cmd_accessibility_readcurr},
    {"applist", cmd_applist},
    {"appstart", cmd_appstart},
    {NULL, NULL}};

static const ShellConfig shell_cfg1 = {
    (BaseSequentialStream*)&SUSBD1,
    commands};

void create_shell(EventDispatcher* evtd) {
    _eventDispatcherInstance = evtd;
    shellCreate(&shell_cfg1, SHELL_WA_SIZE, NORMALPRIO + 10);
}
