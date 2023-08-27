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
        painter.draw_string({12, 320 - 32}, Styles::white, "Press DFU to try Stack Dump");
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
    bool dumped{false};
    LED led = (source == CORTEX_M0) ? hackrf::one::led_rx : hackrf::one::led_tx;

    led.off();

    while (true) {
        volatile size_t n = 1000000U;
        while (n--)
            ;
        led.toggle();

        // Stack dump is not guaranteed to work in this state, so wait for DFU button press to attempt it
        if (!dumped && (swizzled_switches() & (1 << (int)Switch::Dfu))) {
            dumped = true;
            stack_dump();
        }
    }
}

bool stack_dump() {
    ui::Painter painter{};
    std::string debug_dir = "DEBUG";
    std::filesystem::path filename{};
    File stack_dump_file{};
    bool error;
    std::string str;
    uint32_t* p;
    int n;

    make_new_directory(debug_dir);
    filename = next_filename_matching_pattern(debug_dir + "/STACK_DUMP_????.TXT");
    error = filename.empty();
    if (!error)
        error = stack_dump_file.create(filename) != 0;
    if (error) {
        painter.draw_string({16, 320 - 32}, ui::Styles::red, "ERROR DUMPING " + filename.filename().string() + "!");
        return false;
    }

    for (p = &__process_stack_base__, n = 0; p < &__process_stack_end__; p++) {
        if (n++ == 0) {
            str = to_string_hex((uint32_t)p, 8) + ": ";
            stack_dump_file.write(str.data(), 10);
        }

        str = to_string_hex(*p, 8) + " ";
        stack_dump_file.write(str.data(), 9);

        if (n==4) {
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
