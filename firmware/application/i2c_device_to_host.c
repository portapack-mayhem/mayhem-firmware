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

#include "i2c_device_to_host.h"

#include <string.h>

I2CShellDriver I2CD1;

static void onotify(GenericQueue* qp) {
    I2CShellDriver* sdp = chQGetLink(qp);
    uint8_t buff[I2CSHELL_BUFFERS_SIZE];
    int n = chOQGetFullI(&sdp->oqueue);
    if (n > I2CSHELL_BUFFERS_SIZE) n = I2CSHELL_BUFFERS_SIZE;  // don't overflow
    if (n > 0) {
        for (int i = 0; i < n; i++) {
            buff[i] = chOQGetI(&sdp->oqueue);
        }

        int ret;
        chSysUnlock();
        do {
            ret = 0;  // todo htotoo i2c_write!!!
            if (ret == -1)
                chThdSleepMilliseconds(1);

        } while (ret == -1);
        chSysLock();
    }
}

static size_t write(void* ip, const uint8_t* bp, size_t n) {
    return chOQWriteTimeout(&((I2CShellDriver*)ip)->oqueue, bp,
                            n, TIME_INFINITE);
}

static size_t read(void* ip, uint8_t* bp, size_t n) {
    return chIQReadTimeout(&((I2CShellDriver*)ip)->iqueue, bp,
                           n, TIME_INFINITE);
}

static msg_t put(void* ip, uint8_t b) {
    return chOQPutTimeout(&((I2CShellDriver*)ip)->oqueue, b, TIME_INFINITE);
}

static msg_t get(void* ip) {
    return chIQGetTimeout(&((I2CShellDriver*)ip)->iqueue, TIME_INFINITE);
}

static msg_t putt(void* ip, uint8_t b, systime_t timeout) {
    return chOQPutTimeout(&((I2CShellDriver*)ip)->oqueue, b, timeout);
}

static msg_t gett(void* ip, systime_t timeout) {
    return chIQGetTimeout(&((I2CShellDriver*)ip)->iqueue, timeout);
}

static size_t writet(void* ip, const uint8_t* bp, size_t n, systime_t time) {
    return chOQWriteTimeout(&((I2CShellDriver*)ip)->oqueue, bp, n, time);
}

static size_t readt(void* ip, uint8_t* bp, size_t n, systime_t time) {
    return chIQReadTimeout(&((I2CShellDriver*)ip)->iqueue, bp, n, time);
}

static const struct I2CShellDriverVMT vmt = {
    write, read, put, get,
    putt, gett, writet, readt};

void init_i2c_shell_driver(I2CShellDriver* sdp) {
    sdp->vmt = &vmt;
    chIQInit(&sdp->iqueue, sdp->ib, I2CSHELL_BUFFERS_SIZE, NULL, sdp);
    chOQInit(&sdp->oqueue, sdp->ob, I2CSHELL_BUFFERS_SIZE, onotify, sdp);
}

// queue handler from ch
static msg_t qwait(GenericQueue* qp, systime_t time) {
    if (TIME_IMMEDIATE == time)
        return Q_TIMEOUT;
    currp->p_u.wtobjp = qp;
    queue_insert(currp, &qp->q_waiting);
    return chSchGoSleepTimeoutS(THD_STATE_WTQUEUE, time);
}

void complete_i2chost_to_device_transfer() {
    // todo htotoo, do it to handle input from i2c
    /* for (; !usb_bulk_buffer_queue.empty(); usb_bulk_buffer_queue.pop()) {
         usb_bulk_buffer_t* transfer_data = usb_bulk_buffer_queue.front();

         while (transfer_data->completed == false)
             return;

         chSysLock();
         for (unsigned int i = 0; i < transfer_data->length; i++) {
             msg_t ret;
             do {
                 ret = chIQPutI(&SUSBD1.iqueue, transfer_data->data[i]);

                 if (ret == Q_FULL) {
                     chSysUnlock();
                     chThdSleepMilliseconds(1);  // wait for shell thread when buffer is full
                     chSysLock();
                 }

             } while (ret == Q_FULL);
         }
         chSysUnlock();

         usb_bulk_buffer_spare.push(transfer_data);*/
}
}
