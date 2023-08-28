/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include "debug.hpp"

#include <ch.h>
#include <hal.h>
#include <string>

#include "log_file.hpp"
#include "portapack.hpp"
#include "string_format.hpp"
#include "ui_styles.hpp"
#include "irq_controls.hpp"

using namespace ui;

#define DEBUG_LOG_FILE "debug_log.txt"
LogFile* pg_debug_log = nullptr;
void __debug_log(const std::string& msg) {
    static LogFile s_log;
    if (pg_debug_log == nullptr) {
        delete_file(DEBUG_LOG_FILE);
        s_log.append(DEBUG_LOG_FILE);
        pg_debug_log = &s_log;
    }

    pg_debug_log->write_entry(msg);
}

void runtime_error(uint8_t source);
std::string number_to_hex_string(uint32_t);
void draw_line(int32_t, const char*, regarm_t);
void draw_stack_dump();
static bool error_shown = false;

void draw_guru_meditation_header(uint8_t source, const char* hint) {
    Painter painter;

    painter.fill_rectangle(
        {0, 0, portapack::display.width(), portapack::display.height()},
        Color::red());

    constexpr int border = 8;
    painter.fill_rectangle(
        {border, border, portapack::display.width() - (border * 2), portapack::display.height() - (border * 2)},
        Color::black());

    // NOTE: in situations like a hard fault it seems not possible to write strings longer than 16 characters.
    painter.draw_string({48, 24}, Styles::white, "M? Guru");
    painter.draw_string({48 + 8 * 8, 24}, Styles::white, "Meditation");

    if (source == CORTEX_M0) {
        painter.draw_string({48 + 8, 24}, Styles::white, "0");
        painter.draw_string({24, 320 - 32}, Styles::white, "Press DFU for Stack Dump");
    }

    if (source == CORTEX_M4)
        painter.draw_string({48 + 8, 24}, Styles::white, "4");

    painter.draw_string({15, 55}, Styles::white, "Hint: ");
    painter.draw_string({15 + 8 * 8, 55}, Styles::white, hint);
}

void draw_guru_meditation(uint8_t source, const char* hint) {
    if (error_shown == false) {
        error_shown = true;
        draw_guru_meditation_header(source, hint);
    }

    runtime_error(source);
}

void draw_guru_meditation(uint8_t source, const char* hint, struct extctx* ctxp, uint32_t cfsr = 0) {
    if (error_shown == false) {
        error_shown = true;
        draw_guru_meditation_header(source, hint);

        if (ctxp != nullptr) {
            int i = 0;
            draw_line(80 + i++ * 20, "r0:", ctxp->r0);
            draw_line(80 + i++ * 20, "r1:", ctxp->r1);
            draw_line(80 + i++ * 20, "r2:", ctxp->r2);
            draw_line(80 + i++ * 20, "r3:", ctxp->r3);
            draw_line(80 + i++ * 20, "r12:", ctxp->r12);

            // NOTE: run for M0
            // # arm-none-eabi-gdb --batch --eval-command="x/8i 0x<link_register_value_here>" firmware/application/application.elf
            // or for M4
            // # arm-none-eabi-gdb --batch --eval-command="x/8i 0x<link_register_value_here>" firmware/baseband/<image_name_here>.elf
            // to see whats causing the fault.
            draw_line(80 + i++ * 20, "lr:", ctxp->lr_thd);
            draw_line(80 + i++ * 20, "pc:", ctxp->pc);

            // see SCB_CFSR_* in libopencm3/cm3/scb.h for details
            if (cfsr != 0)
                draw_line(80 + i++ * 20, "cfsr:", (void*)cfsr);
        }
    }

    runtime_error(source);
}

void draw_line(int32_t y_offset, const char* label, regarm_t value) {
    Painter painter;

    painter.draw_string({15, y_offset}, Styles::white, label);
    painter.draw_string({15 + 8 * 8, y_offset}, Styles::white, to_string_hex((uint32_t)value, 8));
}

void runtime_error(uint8_t source) {
    LED led = (source == CORTEX_M0) ? hackrf::one::led_rx : hackrf::one::led_tx;

    led.off();

    while (true) {
        volatile size_t n = 1000000U;
        while (n--)
            ;
        led.toggle();

        // Stack dump will cover entire screen, so wait for DFU button press to attempt it
        if (swizzled_switches() & (1 << (int)Switch::Dfu)) {
            draw_stack_dump();
        }
    }
}

// Using the stack while dumping the stack isn't ideal, but hopefully anything imporant is still on the call stack.
void draw_stack_dump() {
    Painter painter;
    uint32_t* p;
    constexpr int border = 8;
    int x, y;
    int n{0};
    bool clear_rect{true};
    bool data_found{false};

    for (p = &__process_stack_base__; p < &__process_stack_end__; p++) {
        if (clear_rect) {
            painter.fill_rectangle(
                {border, border, portapack::display.width() - (border * 2), portapack::display.height() - (border * 2)},
                Color::black());
            x = y = border;
            clear_rect = false;
        }

        // skip past unused stack words
        if (!data_found) {
            if (*p == CRT0_STACKS_FILL_PATTERN)
                continue;
            else {
                data_found = true;
                auto stack_space_left = p - &__process_stack_base__;

                // NOTE: in situations like a hard fault it seems not possible to write strings longer than 16 characters.
                painter.draw_string({x, y}, Styles::white_small, to_string_hex((uint32_t)&__process_stack_base__, 8));
                x += 8 * 5;
                painter.draw_string({x, y}, Styles::white_small, ": M0 STACK");
                x = border;
                y += 8;

                // align subsequent lines to start on 16-byte boundaries
                p -= (stack_space_left & 3);
            }
        }

        // show address
        if (n++ == 0) {
            painter.draw_string({x, y}, Styles::white_small, to_string_hex((uint32_t)p, 8) + ":");
            x += 9 * 5;
        }

        // show stack word -- highlight if a possible code address (low bit will be set too for !thumb)
        bool code_addr = (*p > 0x1400) && (*p < 0x80000) && (((*p) & 0x1) == 0x1);  // approximate address range of code .text region in ROM
        painter.draw_string({x, y}, code_addr ? Styles::bg_white_small : Styles::white_small, " " + to_string_hex(*p, 8));
        x += 9 * 5;

        // new line?
        if (n == 4) {
            n = 0;
            x = border;
            y += 8;

            // out of room on the screen - prompt for more
            if ((y >= portapack::display.height() - border - 8) && (p + 1 < &__process_stack_end__)) {
                while (swizzled_switches() & ((1 << (int)Switch::Up) | (1 << (int)Switch::Down)))
                    ;
                painter.draw_string({x, y}, Styles::white_small, "Use UP/DOWN key");
                while (1) {
                    if (swizzled_switches() & (1 << (int)Switch::Up)) {
                        // back up pointer by 2 screens of 8 pixels per line & 4 words per line
                        p -= 2 * (y - border) / (8 / 4);
                        if (p <= &__process_stack_base__)
                            p = &__process_stack_base__ - 1;  // -1 for p++ in for loop
                        break;
                    }
                    if (swizzled_switches() & (1 << (int)Switch::Down))
                        break;
                }
                clear_rect = true;
            }
        }
    }

    painter.draw_string({border, portapack::display.height() - border - 8}, Styles::white_small, "End! DFU=repeat");
}

// Disk I/O in this function doesn't work after a fault
// Using the stack while dumping the stack isn't ideal, but hopefully anything imporant is still on the call stack.
bool stack_dump() {
    Painter painter;
    std::string debug_dir = "DEBUG";
    std::filesystem::path filename{};
    File stack_dump_file{};
    bool error;
    std::string str;
    uint32_t* p;
    int n{0};
    bool data_found{false};

    make_new_directory(debug_dir);
    filename = next_filename_matching_pattern(debug_dir + "/STACK_DUMP_????.TXT");
    error = filename.empty();
    if (!error)
        error = stack_dump_file.create(filename) != 0;
    if (error) {
        painter.draw_string({16, 320 - 32}, ui::Styles::red, "ERROR DUMPING " + filename.filename().string() + "!");
        return false;
    }

    for (p = &__process_stack_base__; p < &__process_stack_end__; p++) {
        // skip past unused stack words
        if (!data_found) {
            if (*p == CRT0_STACKS_FILL_PATTERN)
                continue;
            else {
                data_found = true;
                auto stack_space_left = p - &__process_stack_base__;
                stack_dump_file.write_line(to_string_hex((uint32_t)&__process_stack_base__, 8) + ": Unused words " + to_string_dec_uint(stack_space_left));

                // align subsequent lines to start on 16-byte boundaries
                p -= (stack_space_left & 3);
            }
        }

        // write address
        if (n++ == 0) {
            str = to_string_hex((uint32_t)p, 8) + ":";
            stack_dump_file.write(str.data(), 9);
        }

        // write stack dword
        str = " " + to_string_hex(*p, 8);
        stack_dump_file.write(str.data(), 9);

        if (n == 4) {
            stack_dump_file.write("\r\n", 2);
            n = 0;
        }
    }

    painter.draw_string({16, 320 - 32}, ui::Styles::green, filename.filename().string() + " dumped!");
    return true;
}

extern "C" {
#if CH_DBG_ENABLED
void port_halt(void) {
    port_disable();

    if (dbg_panic_msg == nullptr)
        dbg_panic_msg = "system halted";

    draw_guru_meditation(CORTEX_M0, dbg_panic_msg);
}
#endif

CH_IRQ_HANDLER(MemManageVector) {
#if CH_DBG_ENABLED
    chDbgPanic("MemManage");
#else
    chSysHalt();
#endif
}

CH_IRQ_HANDLER(BusFaultVector) {
#if CH_DBG_ENABLED
    chDbgPanic("BusFault");
#else
    chSysHalt();
#endif
}

CH_IRQ_HANDLER(UsageFaultVector) {
#if CH_DBG_ENABLED
    chDbgPanic("UsageFault");
#else
    chSysHalt();
#endif
}

CH_IRQ_HANDLER(HardFaultVector) {
#if CH_DBG_ENABLED
    CH_IRQ_PROLOGUE();
    struct extctx* ctxp;
    port_lock_from_isr();

    if ((uint32_t)_saved_lr & 0x04)
        ctxp = (struct extctx*)__get_PSP();
    else
        ctxp = (struct extctx*)__get_MSP();

    port_disable();

    auto stack_space_left = get_free_stack_space();
    if (stack_space_left < 16)
        draw_guru_meditation(CORTEX_M0, "Stack Overflow", ctxp);

    draw_guru_meditation(CORTEX_M0, "Hard Fault", ctxp);

    CH_IRQ_EPILOGUE();
#else
    chSysHalt();
#endif
}

} /* extern "C" */
