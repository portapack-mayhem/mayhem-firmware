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
#include "performance_counter.hpp"

#include "usb_serial_device_to_host.h"
#include "chprintf.h"
#include "chqueues.h"
#include "ui_external_items_menu_loader.hpp"
#include "ui_flash_utility.hpp"
#include "untar.hpp"
#include "ui_widget.hpp"
#include "file_path.hpp"

#include "ui_navigation.hpp"
#include "usb_serial_shell_filesystem.hpp"

#include "portapack_persistent_memory.hpp"

#include <string>
#include <cstring>
#include <libopencm3/lpc43xx/wwdt.h>

#define SHELL_WA_SIZE THD_WA_SIZE(1024 * 3)
#define palOutputPad(port, pad) (LPC_GPIO->DIR[(port)] |= 1 << (pad))

static EventDispatcher* _eventDispatcherInstance = NULL;
static EventDispatcher* getEventDispatcherInstance() {
    return _eventDispatcherInstance;
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
        Theme::getInstance()->fg_yellow->background);

    painter.draw_bitmap(
        {portapack::display.width() / 2 - 8, portapack::display.height() / 2 - 8},
        ui::bitmap_icon_hackrf,
        Theme::getInstance()->fg_yellow->foreground,
        Theme::getInstance()->fg_yellow->background);

    sdcDisconnect(&SDCD1);
    sdcStop(&SDCD1);

    m4_request_shutdown();
    chThdSleepMilliseconds(50);
    portapack::shutdown(true);
    m4_init(portapack::spi_flash::image_tag_usb_sd, portapack::memory::map::m4_code, false);
    m0_halt();
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

    auto evtd = getEventDispatcherInstance();
    if (!evtd) return;
    auto top_widget = evtd->getTopWidget();
    if (!top_widget) return;
    auto nav = static_cast<ui::SystemView*>(top_widget)->get_navigation_view();
    if (!nav) return;
    nav->home(false);

    // call nav with flash
    auto open_view = nav->push<ui::FlashUtilityView>();
    chprintf(chp, "Flashing started\r\n");
    chThdSleepMilliseconds(150);  // to give display some time to paint the screen
    if (!open_view->flash_firmware(path.native())) {
        chprintf(chp, "error\r\n");
    }
}

static void cmd_screenshot(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    ensure_directory(screenshots_dir);
    auto path = next_filename_matching_pattern(screenshots_dir / u"SCR_????.PNG");

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
            char buffer[5 * 3 * 2 + 1];
            sprintf(buffer, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", row[px].r, row[px].g, row[px].b, row[px + 1].r, row[px + 1].g, row[px + 1].b, row[px + 2].r, row[px + 2].g, row[px + 2].b, row[px + 3].r, row[px + 3].g, row[px + 3].b, row[px + 4].r, row[px + 4].g, row[px + 4].b);
            fillOBuffer(&((SerialUSBDriver*)chp)->oqueue, (const uint8_t*)buffer, 5 * 3 * 2);
        }
        chprintf(chp, "\r\n");
    }

    evtd->exit_shell_working_mode();

    chprintf(chp, "ok\r\n");
}

// calculates the 1 byte rgb value, and add 32 to it, so it can be a printable character.
static char getChrFromRgb(uint8_t r, uint8_t g, uint8_t b) {
    uint8_t chR = r >> 6;  // 3bit
    uint8_t chG = g >> 6;  // 3bit
    uint8_t chB = b >> 6;  // 3bit
    uint8_t res = chR << 4 | chG << 2 | chB;
    res += 32;
    return res;
}

// keep track of a buffer, and sends only if full. not only line by line
static void screenbuffer_helper_add(BaseSequentialStream* chp, char* buffer, size_t& wp, char ch) {
    buffer[wp++] = ch;
    if (wp > USBSERIAL_BUFFERS_SIZE - 1) {
        fillOBuffer(&((SerialUSBDriver*)chp)->oqueue, (const uint8_t*)buffer, USBSERIAL_BUFFERS_SIZE);
        wp = 0;
    }
}

// sends only 1 byte (printable only) per pixel, so around 96 colors
static void cmd_screenframeshort(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    auto evtd = getEventDispatcherInstance();
    evtd->enter_shell_working_mode();
    char buffer[USBSERIAL_BUFFERS_SIZE];
    size_t wp = 0;
    for (int y = 0; y < ui::screen_height; y++) {
        std::array<ui::ColorRGB888, ui::screen_width> row;
        portapack::display.read_pixels({0, y, ui::screen_width, 1}, row);
        for (int i = 0; i < 240; ++i) {
            screenbuffer_helper_add(chp, buffer, wp, getChrFromRgb(row[i].r, row[i].g, row[i].b));
        }
        screenbuffer_helper_add(chp, buffer, wp, '\r');
        screenbuffer_helper_add(chp, buffer, wp, '\n');
    }
    if (wp > 0) {
        // send remaining
        fillOBuffer(&((SerialUSBDriver*)chp)->oqueue, (const uint8_t*)buffer, wp);
    }
    evtd->exit_shell_working_mode();
    chprintf(chp, "\r\nok\r\n");
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

static void cmd_rtcget(BaseSequentialStream* chp, int argc, char* argv[]) {
    (void)chp;
    (void)argc;
    (void)argv;

    rtc::RTC datetime;
    rtc_time::now(datetime);

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

    // TODO: additional commands/parameters for DST?
    rtc::RTC new_datetime{
        (uint16_t)strtol(argv[0], NULL, 10), (uint8_t)strtol(argv[1], NULL, 10),
        (uint8_t)strtol(argv[2], NULL, 10), (uint32_t)strtol(argv[3], NULL, 10),
        (uint32_t)strtol(argv[4], NULL, 10), (uint32_t)strtol(argv[5], NULL, 10)};
    rtc_time::set(new_datetime);

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
    std::string appwithpath = "/" + apps_dir.string() + "/";
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

static void printAppInfo(BaseSequentialStream* chp, const ui::AppInfo& element) {
    if (strlen(element.id) == 0) return;
    chprintf(chp, element.id);
    chprintf(chp, " ");
    chprintf(chp, element.displayName);
    chprintf(chp, " ");
    switch (element.menuLocation) {
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
    // TODO(u-foka): Somehow order static and dynamic app lists together
    for (auto& element : ui::NavigationView::appMap) {  // Use the map as its ordered by id
        printAppInfo(chp, element.second);
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

static void cmd_gotgps(BaseSequentialStream* chp, int argc, char* argv[]) {
    const char* usage = "usage: gotgps <lat> <lon> [altitude] [speed] [satinuse]\r\n";
    if (argc < 2 || argc > 5) {
        chprintf(chp, usage);
        return;
    }
    float lat = atof(argv[0]);
    float lon = atof(argv[1]);
    int32_t altitude = 0;
    int32_t speed = 0;
    uint8_t satinuse = 0;
    if (argc >= 3) altitude = strtol(argv[2], NULL, 10);
    if (argc >= 4) speed = strtol(argv[3], NULL, 10);
    if (argc >= 5) satinuse = strtol(argv[4], NULL, 10);
    GPSPosDataMessage msg{lat, lon, altitude, speed, satinuse};
    EventDispatcher::send_message(msg);
    chprintf(chp, "ok\r\n");
}

static void cmd_gotorientation(BaseSequentialStream* chp, int argc, char* argv[]) {
    const char* usage = "usage: gotorientation <angle> [tilt]\r\n";
    if (argc != 1 && argc != 2) {
        chprintf(chp, usage);
        return;
    }
    uint16_t angle = strtol(argv[0], NULL, 10);
    int16_t tilt = 400;
    if (argc >= 2) tilt = strtol(argv[1], NULL, 10);
    OrientationDataMessage msg{angle, tilt};
    EventDispatcher::send_message(msg);
    chprintf(chp, "ok\r\n");
}

static void cmd_gotenv(BaseSequentialStream* chp, int argc, char* argv[]) {
    const char* usage = "usage: gotenv <temperature> [humidity] [pressure]  [light]\r\n";
    if (argc < 1 || argc > 4) {
        chprintf(chp, usage);
        return;
    }
    float temp = atof(argv[0]);
    float humi = 0;
    float pressure = 0;
    uint16_t light = 0;
    if (argc > 1) humi = atof(argv[1]);
    if (argc > 2) pressure = atof(argv[2]);
    if (argc > 3) light = strtol(argv[3], NULL, 10);
    EnvironmentDataMessage msg{temp, humi, pressure, light};
    EventDispatcher::send_message(msg);
    chprintf(chp, "ok\r\n");
}

static void cmd_sysinfo(BaseSequentialStream* chp, int argc, char* argv[]) {
    const char* usage = "usage: sysinfo\r\n";
    (void)argv;
    if (argc > 0) {
        chprintf(chp, usage);
        return;
    }
    auto utilisation = get_cpu_utilisation_in_percent();
    std::string info =
        "M0 heap: " + to_string_dec_uint(chCoreStatus()) + "\r\n" +
        "M0 stack: " + to_string_dec_uint((uint32_t)get_free_stack_space()) + "\r\n" +
        "M0 cpu%: " + to_string_dec_uint(utilisation) + "\r\n" +
        "M4 heap: " + to_string_dec_uint(shared_memory.m4_heap_usage) + "\r\n" +
        "M4 stack: " + to_string_dec_uint(shared_memory.m4_stack_usage) + "\r\n" +
        "M0 cpu%: " + to_string_dec_uint(shared_memory.m4_performance_counter) + "\r\n" +
        "M4 miss: " + to_string_dec_uint(shared_memory.m4_buffer_missed) + "\r\n" +
        "uptime: " + to_string_dec_uint(chTimeNow() / 1000) + "\r\n";

    fillOBuffer(&((SerialUSBDriver*)chp)->oqueue, (const uint8_t*)info.c_str(), info.length());
    return;
}

static void cmd_radioinfo(BaseSequentialStream* chp, int argc, char* argv[]) {
    const char* usage = "usage: radioinfo\r\n";
    (void)argv;
    if (argc > 0) {
        chprintf(chp, usage);
        return;
    }
    std::string info =
        "receiver_model.target_frequency: " + to_string_dec_uint(portapack::receiver_model.target_frequency()) + "\r\n" +
        "receiver_model.baseband_bandwidth: " + to_string_dec_uint(portapack::receiver_model.baseband_bandwidth()) + "\r\n" +
        "receiver_model.sampling_rate: " + to_string_dec_uint(portapack::receiver_model.sampling_rate()) + "\r\n" +
        "receiver_model.modulation: " + to_string_dec_uint((uint32_t)portapack::receiver_model.modulation()) + "\r\n" +
        "receiver_model.am_configuration: " + to_string_dec_uint(portapack::receiver_model.am_configuration()) + "\r\n" +
        "receiver_model.nbfm_configuration: " + to_string_dec_uint(portapack::receiver_model.nbfm_configuration()) + "\r\n" +
        "receiver_model.wfm_configuration: " + to_string_dec_uint(portapack::receiver_model.wfm_configuration()) + "\r\n" +
        "transmitter_model.target_frequency: " + to_string_dec_uint(portapack::transmitter_model.target_frequency()) + "\r\n" +
        "transmitter_model.baseband_bandwidth: " + to_string_dec_uint(portapack::transmitter_model.baseband_bandwidth()) + "\r\n" +
        "transmitter_model.sampling_rate: " + to_string_dec_uint(portapack::transmitter_model.sampling_rate()) + "\r\n";

    fillOBuffer(&((SerialUSBDriver*)chp)->oqueue, (const uint8_t*)info.c_str(), info.length());
    return;
}

static void cmd_pmemreset(BaseSequentialStream* chp, int argc, char* argv[]) {
    const char* usage = "usage: pmemreset yes\r\nThis will reset pmem to defaults!\r\n";
    (void)argv;
    if (argc != 1 || strcmp(argv[0], "yes") != 0) {
        chprintf(chp, usage);
        return;
    }
    auto evtd = getEventDispatcherInstance();
    if (!evtd) return;
    auto top_widget = evtd->getTopWidget();
    if (!top_widget) return;
    auto nav = static_cast<ui::SystemView*>(top_widget)->get_navigation_view();
    if (!nav) return;
    nav->home(true);

    portapack::persistent_memory::cache::defaults();
    // system refresh
    StatusRefreshMessage message{};
    EventDispatcher::send_message(message);
    chprintf(chp, "ok\r\n");
}

static void cmd_settingsreset(BaseSequentialStream* chp, int argc, char* argv[]) {
    const char* usage = "usage: settingsreset yes\r\nThis will reset all app settings to defaults!\r\n";
    (void)argv;
    if (argc != 1 || strcmp(argv[0], "yes") != 0) {
        chprintf(chp, usage);
        return;
    }
    auto evtd = getEventDispatcherInstance();
    if (!evtd) return;
    auto top_widget = evtd->getTopWidget();
    if (!top_widget) return;
    auto nav = static_cast<ui::SystemView*>(top_widget)->get_navigation_view();
    if (!nav) return;
    nav->home(true);  // to exit all running apps

    for (const auto& entry : std::filesystem::directory_iterator(settings_dir, u"*.ini")) {
        if (std::filesystem::is_regular_file(entry.status())) {
            std::filesystem::path pth = settings_dir;
            pth += u"/" + entry.path();
            chprintf(chp, pth.string().c_str());
            chprintf(chp, "\r\n");
            f_unlink(pth.tchar());
        }
    }
    // system refresh
    StatusRefreshMessage message{};
    EventDispatcher::send_message(message);
    chprintf(chp, "ok\r\n");
}

static void cmd_sendpocsag(BaseSequentialStream* chp, int argc, char* argv[]) {
    const char* usage = "usage: sendpocsag <addr> <msglen> [baud] [type] [function] [phase] \r\n";
    (void)argv;
    if (argc < 2) {
        chprintf(chp, usage);
        return;
    }
    uint64_t addr = atol(argv[0]);
    int msglen = atoi(argv[1]);  // without minimum limit, since addr only don't send anything
    if (msglen > 30 || msglen < 0) {
        chprintf(chp, "error, msglen max is 30\r\n");
        return;
    }

    int baud = 1200;
    if (argc >= 3) {
        baud = atoi(argv[2]);
        if (baud != 512 && baud != 1200 && baud != 2400) {
            chprintf(chp, "error, baud can only be 512, 1200 or 2400\r\n");
            return;
        }
    }

    int type = 2;
    if (argc >= 4) {
        type = atoi(argv[3]);
        if (type > 2 || type < 0) {
            chprintf(chp, "error, type can be 0 (ADDRESS_ONLY) 1 (NUMERIC_ONLY) 2 (ALPHANUMERIC)\r\n");
            return;
        }
    }

    char function = 'D';
    if (argc >= 5) {
        function = *argv[4];
        if (function < 'A' && function > 'D') {
            chprintf(chp, "error, function can be A, B, C or D\r\n");
            return;
        }
    }

    char phase = 'P';
    if (argc >= 6) {
        phase = *argv[5];
        if (phase != 'P' && phase != 'N') {
            chprintf(chp, "error, phase can be P or N\r\n");
            return;
        }
    }

    uint8_t msg[31] = {0};
    if (msglen > 0) {
        chprintf(chp, "send %d bytes\r\n", msglen);
        do {
            size_t bytes_to_read = msglen > USB_BULK_BUFFER_SIZE ? USB_BULK_BUFFER_SIZE : msglen;
            size_t bytes_read = chSequentialStreamRead(chp, &msg[0], bytes_to_read);
            if (bytes_read != bytes_to_read)
                return;
            msglen -= bytes_read;
        } while (msglen > 0);
    }

    auto evtd = getEventDispatcherInstance();
    if (!evtd) return;
    auto top_widget = evtd->getTopWidget();
    if (!top_widget) return;
    auto nav = static_cast<ui::SystemView*>(top_widget)->get_navigation_view();
    if (!nav) return;
    if (!nav->StartAppByName("pocsagtx")) {
        chprintf(chp, "error starting pocsagtx\r\n");
        return;
    }
    chThdSleepMilliseconds(1000);  // wait for app to start
    PocsagTosendMessage message{(uint16_t)baud, (uint8_t)type, function, phase, (uint8_t)msglen, msg, addr};
    EventDispatcher::send_message(message);
    chprintf(chp, "ok\r\n");
}

static void cmd_asyncmsg(BaseSequentialStream* chp, int argc, char* argv[]) {
    const char* usage = "usage: asyncmsg x, x can be enable or disable\r\n";
    if (argc != 1) {
        chprintf(chp, usage);
        return;
    }
    if (strcmp(argv[0], "disable") == 0) {
        portapack::async_tx_enabled = false;
        chprintf(chp, "ok\r\n");
    } else if (strcmp(argv[0], "enable") == 0) {
        portapack::async_tx_enabled = true;
        chprintf(chp, "ok\r\n");
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
    USB_SERIAL_SHELL_SD_COMMANDS,
    {"rtcget", cmd_rtcget},
    {"rtcset", cmd_rtcset},
    {"cpld_info", cpld_info},
    {"cpld_read", cmd_cpld_read},
    {"accessibility_readall", cmd_accessibility_readall},
    {"accessibility_readcurr", cmd_accessibility_readcurr},
    {"applist", cmd_applist},
    {"appstart", cmd_appstart},
    {"gotgps", cmd_gotgps},
    {"gotorientation", cmd_gotorientation},
    {"gotenv", cmd_gotenv},
    {"sysinfo", cmd_sysinfo},
    {"radioinfo", cmd_radioinfo},
    {"pmemreset", cmd_pmemreset},
    {"settingsreset", cmd_settingsreset},
    {"sendpocsag", cmd_sendpocsag},
    {"asyncmsg", cmd_asyncmsg},
    {NULL, NULL}};

static const ShellConfig shell_cfg1 = {
    (BaseSequentialStream*)&SUSBD1,
    commands};

void create_shell(EventDispatcher* evtd) {
    _eventDispatcherInstance = evtd;
    shellCreate(&shell_cfg1, SHELL_WA_SIZE, NORMALPRIO + 10);
}
