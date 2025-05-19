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

#include "usb_serial_cdc.h"
#include "usb_serial_endpoints.h"
#include "usb_serial_event.hpp"

uint32_t EVT_MASK_USB = EVENT_MASK(8);

extern void usb0_isr(void);

static Thread* thread_usb_event = NULL;

CH_IRQ_HANDLER(USB0_IRQHandler) {
    CH_IRQ_PROLOGUE();

    const uint32_t status = USB0_USBSTS_D & USB0_USBINTR_D;

    usb0_isr();

    if (status & USB0_USBSTS_D_UI) {
        chSysLockFromIsr();
        chEvtSignalI(thread_usb_event, EVT_MASK_USB);
        chSysUnlockFromIsr();
    }

    if (status & USB0_USBSTS_D_SLI) {
        on_channel_closed();
    }

    CH_IRQ_EPILOGUE();
}

uint32_t __ldrex(volatile uint32_t* addr) {
    __disable_irq();
    return *addr;
}

uint32_t __strex(uint32_t val, volatile uint32_t* addr) {
    (void)val;
    (void)addr;

    *addr = val;
    __enable_irq();
    return 0;
}

void nvic_enable_irq(uint8_t irqn) {
    NVIC_ISER(irqn / 32) = (1 << (irqn % 32));
    thread_usb_event = chThdSelf();
}

void usb_configuration_changed(usb_device_t* const device) {
    (void)device;

    usb_endpoint_init(&usb_endpoint_int_in);
    usb_endpoint_init(&usb_endpoint_bulk_in);
    usb_endpoint_init(&usb_endpoint_bulk_out);
}

void setup_usb_serial_controller(void) {
    usb_set_configuration_changed_cb(usb_configuration_changed);
    usb_peripheral_reset();

    usb_device_init(0, &usb_device);

    usb_queue_init(&usb_endpoint_control_out_queue);
    usb_queue_init(&usb_endpoint_control_in_queue);
    usb_queue_init(&usb_endpoint_int_in_queue);
    usb_queue_init(&usb_endpoint_bulk_out_queue);
    usb_queue_init(&usb_endpoint_bulk_in_queue);

    usb_endpoint_init(&usb_endpoint_control_out);
    usb_endpoint_init(&usb_endpoint_control_in);

    usb_run(&usb_device);
}

const usb_request_handlers_t usb_request_handlers = {
    .standard = usb_standard_request,
    .class = usb_class_request,
    .vendor = 0,
    .reserved = 0};

usb_request_status_t usb_class_request(usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage) {
    usb_request_status_t status = USB_REQUEST_STATUS_STALL;

    volatile uint8_t request = endpoint->setup.request;

    if (request == 0x21)  // GET LINE CODING REQUEST
        return usb_get_line_coding_request(endpoint, stage);

    if (request == 0x22)  // SET CONTROL LINE STATE REQUEST
        return usb_set_control_line_state_request(endpoint, stage);

    if (request == 0x20)  // SET LINE CODING REQUEST
        return usb_set_line_coding_request(endpoint, stage);

    return USB_REQUEST_STATUS_OK;

    return status;
}

usb_request_status_t usb_get_line_coding_request(usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage) {
    if (stage == USB_TRANSFER_STAGE_SETUP) {
        usb_transfer_schedule_block(
            endpoint->in,
            &endpoint->buffer,
            0,
            NULL,
            NULL);
    } else if (stage == USB_TRANSFER_STAGE_DATA) {
        usb_transfer_schedule_ack(endpoint->out);
    }

    return USB_REQUEST_STATUS_OK;
}
usb_request_status_t usb_set_control_line_state_request(usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage) {
    if (stage == USB_TRANSFER_STAGE_SETUP) {
        on_channel_opened();

        usb_transfer_schedule_ack(endpoint->in);
    }

    return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_set_line_coding_request(usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage) {
    if (stage == USB_TRANSFER_STAGE_SETUP) {
        usb_transfer_schedule_block(
            endpoint->out,
            &endpoint->buffer,
            32,
            NULL,
            NULL);
    } else if (stage == USB_TRANSFER_STAGE_DATA) {
        usb_transfer_schedule_ack(endpoint->in);
    }

    return USB_REQUEST_STATUS_OK;
}
