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

#include "ui_painter.hpp"
#include "portapack.hpp"
#include "ui_font_fixed_8x16.hpp"

void runtime_error(LED);
std::string number_to_hex_string(uint32_t);
void draw_line(int32_t, const char *, regarm_t);
static bool error_shown = false;

extern void draw_guru_meditation_header(uint8_t source, const char *hint) {
    ui::Painter painter;
    ui::Style style_default {
        .font = ui::font::fixed_8x16,
        .background = ui::Color::black(),
        .foreground = ui::Color::white()
    };

    painter.fill_rectangle(
        { 0, 0, portapack::display.width(), portapack::display.height() },
        ui::Color::red()
    );

    constexpr int border = 8;
    painter.fill_rectangle(
        { border, border, portapack::display.width() - (border * 2), portapack::display.height() - (border * 2) },
        ui::Color::black()
    );

    // NOTE: in situations like a hard fault it seems not possible to write strings longer than 16 characters.
    painter.draw_string({ 48, 24 }, style_default, "M? Guru");
    painter.draw_string({ 48 + 8*8, 24 }, style_default, "Meditation");

    if (source == CORTEX_M0)
        painter.draw_string({ 48 + 8, 24 }, style_default, "0");
    
    if (source == CORTEX_M4)
        painter.draw_string({ 48 + 8, 24 }, style_default, "4");
    
    painter.draw_string({ 15, 55 }, style_default, "Hint: ");
    painter.draw_string({ 15 + 8*8, 55 }, style_default, hint);
}

void draw_guru_meditation(uint8_t source, const char *hint) {
    if(error_shown == false){
        error_shown = true;
        draw_guru_meditation_header(source, hint);
    }

    if (source == CORTEX_M0)
        runtime_error(hackrf::one::led_rx);

    if (source == CORTEX_M4)
    	runtime_error(hackrf::one::led_tx);
}

void draw_guru_meditation(uint8_t source, const char *hint, struct extctx *ctxp) {
    if(error_shown == false)
    {
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
        }
    }
    
    if (source == CORTEX_M0)
        runtime_error(hackrf::one::led_rx);

    if (source == CORTEX_M4)
    	runtime_error(hackrf::one::led_tx);
}

void draw_line(int32_t y_offset, const char *label, regarm_t value){
    ui::Painter painter;
    ui::Style style_default {
        .font = ui::font::fixed_8x16,
        .background = ui::Color::black(),
        .foreground = ui::Color::white()
    };

    painter.draw_string({ 15, y_offset }, style_default, label);
    painter.draw_string({ 15 + 8*8, y_offset }, style_default, number_to_hex_string((uint32_t)value));
}

std::string number_to_hex_string(uint32_t number){
    char str[16];
    char* p = &str[16];
    do {
        p--;
        uint32_t digit = number % 16; 
        number /= 16;
        *p = digit>=10 ? 'A' + (digit-10) : '0' + digit;
    } while ( number > 0 );
    p--;
    *p = 'x';
    p--;
    *p = '0';
    return std::string(p, &str[16]-p);
}

void runtime_error(LED led) {
    led.off();

    while(true) {
        volatile size_t n = 1000000U;
        while(n--);
        led.toggle();
    }
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
    struct extctx *ctxp;
    port_lock_from_isr();

    if ((uint32_t)_saved_lr & 0x04)
        ctxp = (struct extctx *)__get_PSP();
    else
        ctxp = (struct extctx *)__get_MSP();

    port_disable();
    draw_guru_meditation(CORTEX_M0, "Hard Fault", ctxp);

    CH_IRQ_EPILOGUE();
#else
    chSysHalt();
#endif
}

} /* extern "C" */
