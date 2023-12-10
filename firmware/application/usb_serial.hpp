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

// #include "usb_type.h"

namespace portapack {

class USBSerial {
   public:
    void initialize();
    // void irq_usb();

   private:
    // usb_queue_head_t usb_qh[12] __attribute__((aligned(2048)));

    void enable_xtal();
    void disable_pll0();
    void setup_pll0();
    void enable_pll0();

    void setup_usb_clock();
    void reset_usb();

    void usb_phy_enable();
    // void usb_controller_reset();
    // void usb_controller_set_device_mode();

    // void usb_reset_all_endpoints();
    // void usb_controller_stop();
    // bool usb_controller_is_resetting();

    // void usb_disable_all_endpoints();
    // void usb_clear_all_pending_interrupts();
    // void usb_flush_all_primed_endpoints();

    // void usb_clear_pending_interrupts(const uint32_t mask);
    // void usb_flush_primed_endpoints(const uint32_t mask);

    // void usb_wait_for_endpoint_priming_to_finish(const uint32_t mask);
    // void usb_flush_endpoints(const uint32_t mask);
    // void usb_wait_for_endpoint_flushing_to_finish(const uint32_t mask);

    // void usb_endpoint_init(const usb_endpoint_t* const endpoint);
    // void usb_endpoint_flush(const usb_endpoint_t* const endpoint);
    // usb_queue_head_t* usb_queue_head(const uint_fast8_t endpoint_address);
    // void usb_endpoint_set_type(const usb_endpoint_t* const endpoint, const usb_transfer_type_t transfer_type);
    // void usb_endpoint_enable(const usb_endpoint_t* const endpoint);

    // uint_fast8_t usb_endpoint_number(const uint_fast8_t endpoint_address);
    // bool usb_endpoint_is_in(const uint_fast8_t endpoint_address);

    void usb_controller_run();

    // uint32_t usb_get_status();
};

}  // namespace portapack