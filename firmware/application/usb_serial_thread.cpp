/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
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

#include "usb_serial_thread.hpp"
#include "buffer_exchange.hpp"

// UsbSerialThread //////////////////////////////////////////////////////////

UsbSerialThread::UsbSerialThread() {
    create_thread();
}

UsbSerialThread::~UsbSerialThread() {
    stop();
}

void UsbSerialThread::create_thread() {
    thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO + 10, UsbSerialThread::static_fn, this);
}

void UsbSerialThread::stop() {
    if (thread) {
        chThdTerminate(thread);
        chThdWait(thread);
        thread = nullptr;
    }
}

msg_t UsbSerialThread::static_fn(void* arg) {
    auto obj = static_cast<UsbSerialThread*>(arg);
    obj->run();
    return 0;
}

void UsbSerialThread::run() {
    while (!chThdShouldTerminate()) {
        if (str_ready) {
            str_ready = false;
            chprintf((BaseSequentialStream*)&SUSBD1, serial_str.c_str());
        }

        chThdSleepMilliseconds(50);
    }
}
