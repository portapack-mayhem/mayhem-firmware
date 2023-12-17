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

#pragma once

#include "ch.h"
#include "hal.h"

namespace portapack {

class USBSerial {
   public:
    void initialize();
    void dispatch();
    void on_channel_opened();
    void on_channel_closed();

   private:
    void enable_xtal();
    void disable_pll0();
    void setup_pll0();
    void enable_pll0();

    void setup_usb_clock();

    bool connected{false};
    bool shell_created{false};
};

}  // namespace portapack