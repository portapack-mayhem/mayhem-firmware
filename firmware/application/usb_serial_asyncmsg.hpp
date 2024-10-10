/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
 * Copyleft (ɔ) 2024 zxkmm with the GPL license
 * Copyright (C) 2024 HTotoo
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

#ifndef USB_SERIAL_AYNCMSG_HPP
#define USB_SERIAL_AYNCMSG_HPP

#include <vector>
#include <string>
#include <sstream>

#include "portapack.hpp"
#include "ch.h"
#include "chprintf.h"
#include "hal.h"
#include "usb_serial_device_to_host.h"

class UsbSerialAsyncmsg {
   public:
    template <typename STRINGCOVER>
    static void asyncmsg(const STRINGCOVER& data);

    template <typename VECTORCOVER>
    static void asyncmsg(const std::vector<VECTORCOVER>& data);

    static void asyncmsg(const char* data);  // string literal
};

/*Notes:
 * - Don't use MayhemHub since it currently not support real time serial output (to hide some instructions from default features call (like screenframe) it filtered out by the response type)
 * - use this client to debug with serial: https://github.com/portapack-mayhem/mayhem-cli
 * - usage:
 *        portapack::async_tx_enabled = true; // note that use this when debugging, unless the msg would be forbidden. but don't use this in production, since it's not real async and multiple serial transmittions will broken each other. if this class is used in other scene in the future, just use command to cover (protect your serial tramsnitton) in your extern thing: asyncmsg enable --- your cmd --- asyncmsg disable
 *        #include "usb_serial_asyncmsg.hpp"
 *        UsbSerialAsyncmsg::asyncmsg("Hello PP");
 * */

/// vec worker
// ussgae:    UsbSerialAsyncmsg::asyncmsg(vec);
template <typename VECTORCOVER>
void UsbSerialAsyncmsg::asyncmsg(const std::vector<VECTORCOVER>& data) {
    if (!portapack::async_tx_enabled || !portapack::usb_serial.serial_connected()) {
        return;
    }
    for (const auto& item : data) {
        asyncmsg(item);
    }
}

#endif  // USB_SERIAL_AYNCMSG_HPP
