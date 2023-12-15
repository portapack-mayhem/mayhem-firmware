#include "usb_serial.h"

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
bool serial_running = false;

extern void usb0_isr(void);

CH_IRQ_HANDLER(USB0_IRQHandler) {
    CH_IRQ_PROLOGUE();
    usb0_isr();
    CH_IRQ_EPILOGUE();
}

void usb_configuration_changed(usb_device_t* const device) {
    (void)device;

    usb_endpoint_init(&usb_endpoint_int_in);
    usb_endpoint_init(&usb_endpoint_bulk_in);
    usb_endpoint_init(&usb_endpoint_bulk_out);

    serial_running = false;
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
    } else if (stage == USB_TRANSFER_STAGE_STATUS) {
    }

    return USB_REQUEST_STATUS_OK;
}

extern void onChannelOpened(void);
uint8_t usb_buffer[64] = {0};

void serial_bulk_transfer_complete(void* user_data, unsigned int bytes_transferred) {
    (void)user_data;

    for (unsigned int i = 0; i < bytes_transferred; i++) {
        chSysLockFromIsr();
        chIQPutI(&SUSBD1.iqueue, usb_buffer[i]);
        chSysUnlockFromIsr();
    }
}

static msg_t bulk_out_thread(void* p) {
    while (true) {
        int ret = usb_transfer_schedule(
            &usb_endpoint_bulk_out,
            &usb_buffer[0x0000],
            64,
            serial_bulk_transfer_complete,
            NULL);

        if (ret == -1)
            chThdSleepMilliseconds(1);
    }
}

void usb_serial_create_bulk_out_thread() {
    chThdCreateFromHeap(NULL, 1024, LOWPRIO, bulk_out_thread, NULL);
}

usb_request_status_t usb_set_control_line_state_request(usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage) {
    if (stage == USB_TRANSFER_STAGE_SETUP) {
        if (endpoint->setup.value == 3) {
            if (serial_running == false) {
                serial_running = true;

                onChannelOpened();
            }
        }

        usb_transfer_schedule_ack(endpoint->in);
    } else if (stage == USB_TRANSFER_STAGE_DATA) {
    } else if (stage == USB_TRANSFER_STAGE_STATUS) {
    }

    return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_set_line_coding_request(usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage) {
    uint8_t buf[7];
    if (stage == USB_TRANSFER_STAGE_SETUP) {
        usb_transfer_schedule_block(
            endpoint->out,
            &buf,
            7,
            NULL,
            NULL);
    } else if (stage == USB_TRANSFER_STAGE_DATA) {
        usb_transfer_schedule_ack(endpoint->in);
    } else if (stage == USB_TRANSFER_STAGE_STATUS) {
    }

    return USB_REQUEST_STATUS_OK;
}

usb_request_status_t __attribute__((optimize("O0"))) usb_class_request(usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage) {
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

const usb_request_handlers_t usb_request_handlers = {
    .standard = usb_standard_request,
    .class = usb_class_request,
    .vendor = 0,
    .reserved = 0};

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

/*
 * Interface implementation.
 */

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

static void onotify(GenericQueue* qp) {
    SerialUSBDriver* sdp = chQGetLink(qp);
    int n = chOQGetFullI(&sdp->oqueue);
    if (n > 0) {
        for (size_t i = 0; i < n; i++) {
            usb_endpoint_bulk_in.buffer[i] = chOQGetI(&sdp->oqueue);
        }

        int ret;
        chSysUnlock();
        do {
            ret = usb_transfer_schedule(
                &usb_endpoint_bulk_in,
                &usb_endpoint_bulk_in.buffer[0],
                n,
                NULL,
                NULL);

            chThdSleepMilliseconds(1);

        } while (ret == -1);
        chSysLock();
    }
}

void init_SerialUSBDriver(SerialUSBDriver* sdp) {
    sdp->vmt = &vmt;
    chIQInit(&sdp->iqueue, sdp->ib, SERIAL_BUFFERS_SIZE, NULL, sdp);
    chOQInit(&sdp->oqueue, sdp->ob, SERIAL_BUFFERS_SIZE, onotify, sdp);
}
