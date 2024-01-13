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

#include "usb_serial_device_to_host.h"

#include "usb_serial_endpoints.h"

#pragma GCC diagnostic push
// external code, so ignore warnings
#pragma GCC diagnostic ignored "-Wstrict-prototypes"

#include <common/usb.h>
#include <common/usb_request.h>
#include <common/usb_standard_request.h>
#include <hackrf_usb/usb_device.h>
#include <hackrf_usb/usb_endpoint.h>

#pragma GCC diagnostic pop

#include <usb_request.h>
#include <string.h>

SerialUSBDriver SUSBD1;

static void onotify(GenericQueue* qp) {
    SerialUSBDriver* sdp = chQGetLink(qp);
    uint8_t buff[USBSERIAL_BUFFERS_SIZE];
    int n = chOQGetFullI(&sdp->oqueue);
    if (n > USBSERIAL_BUFFERS_SIZE) n = USBSERIAL_BUFFERS_SIZE;  // don't overflow
    if (n > 0) {
        for (int i = 0; i < n; i++) {
            buff[i] = chOQGetI(&sdp->oqueue);
        }

        int ret;
        chSysUnlock();
        do {
            ret = usb_transfer_schedule(
                &usb_endpoint_bulk_in,
                &buff[0],
                n,
                NULL,
                NULL);

            if (ret == -1)
                chThdSleepMilliseconds(1);

        } while (ret == -1);
        chSysLock();
    }
}

static size_t write(void* ip, const uint8_t* bp, size_t n) {
    return chOQWriteTimeout(&((SerialUSBDriver*)ip)->oqueue, bp,
                            n, TIME_INFINITE);
}

static size_t read(void* ip, uint8_t* bp, size_t n) {
    return chIQReadTimeout(&((SerialUSBDriver*)ip)->iqueue, bp,
                           n, TIME_INFINITE);
}

static msg_t put(void* ip, uint8_t b) {
    return chOQPutTimeout(&((SerialUSBDriver*)ip)->oqueue, b, TIME_INFINITE);
}

static msg_t get(void* ip) {
    return chIQGetTimeout(&((SerialUSBDriver*)ip)->iqueue, TIME_INFINITE);
}

static msg_t putt(void* ip, uint8_t b, systime_t timeout) {
    return chOQPutTimeout(&((SerialUSBDriver*)ip)->oqueue, b, timeout);
}

static msg_t gett(void* ip, systime_t timeout) {
    return chIQGetTimeout(&((SerialUSBDriver*)ip)->iqueue, timeout);
}

static size_t writet(void* ip, const uint8_t* bp, size_t n, systime_t time) {
    return chOQWriteTimeout(&((SerialUSBDriver*)ip)->oqueue, bp, n, time);
}

static size_t readt(void* ip, uint8_t* bp, size_t n, systime_t time) {
    return chIQReadTimeout(&((SerialUSBDriver*)ip)->iqueue, bp, n, time);
}

static const struct SerialUSBDriverVMT vmt = {
    write, read, put, get,
    putt, gett, writet, readt};

void init_serial_usb_driver(SerialUSBDriver* sdp) {
    sdp->vmt = &vmt;
    chIQInit(&sdp->iqueue, sdp->ib, USBSERIAL_BUFFERS_SIZE, NULL, sdp);
    chOQInit(&sdp->oqueue, sdp->ob, USBSERIAL_BUFFERS_SIZE, onotify, sdp);
}

// queue handler from ch
static msg_t qwait(GenericQueue* qp, systime_t time) {
    if (TIME_IMMEDIATE == time)
        return Q_TIMEOUT;
    currp->p_u.wtobjp = qp;
    queue_insert(currp, &qp->q_waiting);
    return chSchGoSleepTimeoutS(THD_STATE_WTQUEUE, time);
}

// This function fills the output buffer, and sends all data in 1 packet
size_t fillOBuffer(OutputQueue* oqp, const uint8_t* bp, size_t n) {
    qnotify_t nfy = oqp->q_notify;
    size_t w = 0;

    chDbgCheck(n > 0, "chOQWriteTimeout");
    chSysLock();
    while (TRUE) {
        while (chOQIsFullI(oqp)) {
            if (qwait((GenericQueue*)oqp, TIME_INFINITE) != Q_OK) {
                chSysUnlock();
                return w;
            }
        }
        while (!chOQIsFullI(oqp) && n > 0) {
            oqp->q_counter--;
            *oqp->q_wrptr++ = *bp++;
            if (oqp->q_wrptr >= oqp->q_top)
                oqp->q_wrptr = oqp->q_buffer;
            w++;
            --n;
        }
        if (nfy) nfy(oqp);

        chSysUnlock(); /* Gives a preemption chance in a controlled point.*/
        if (n == 0) return w;
        chSysLock();
    }
}
