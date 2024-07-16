/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "ch.h"
#include "hal.h"
#include <libopencm3/lpc43xx/usb.h>

extern "C" {
void start_usb(void);
void irq_usb(void);
void usb_transfer(void);

extern volatile bool scsi_running;

CH_IRQ_HANDLER(Vector60) {
    const uint32_t status = USB0_USBSTS_D & USB0_USBINTR_D;
    if (status & USB0_USBSTS_D_SLI) {
        // USB reset received.
        if (scsi_running) {
            LPC_RGU->RESET_CTRL[0] = (1 << 0);
        }
    }
    irq_usb();
}
}

int main() {
    sdcStart(&SDCD1, nullptr);
    if (sdcConnect(&SDCD1) == CH_FAILED) chDbgPanic("no sd card #1");

    start_usb();

    while (true) {
        usb_transfer();
    }

    return 0;
}

void update_performance_counters() {}
