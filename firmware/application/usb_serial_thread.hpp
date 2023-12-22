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

#ifndef __USB_SERIAL_THREAD_H__
#define __USB_SERIAL_THREAD_H__

#include "ch.h"

#include "event_m0.hpp"

#include "io.hpp"
#include "optional.hpp"

#include "chprintf.h"
#include "usb_serial_io.h"

#include <cstdint>
#include <cstddef>
#include <utility>

class UsbSerialThread {
   public:
    UsbSerialThread();
    ~UsbSerialThread();

    void stop();

    UsbSerialThread(const UsbSerialThread&) = delete;
    UsbSerialThread(UsbSerialThread&&) = delete;
    UsbSerialThread& operator=(const UsbSerialThread&) = delete;
    UsbSerialThread& operator=(UsbSerialThread&&) = delete;
    bool str_ready = false;
    std::string serial_str{};

   private:
    Thread* thread{nullptr};
    static msg_t static_fn(void* arg);
    void run();
    void create_thread();
};

#endif /*__CAPTURE_THREAD_H__*/
