#include "usb_serial.h"

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

#define USB_CONTROL_IN_EP_ADDR (0x80)
#define USB_CONTROL_OUT_EP_ADDR (0x00)

#define USB_INT_IN_EP_ADDR (0x82)

#define USB_BULK_OUT_EP_ADDR (0x01)
#define USB_BULK_IN_EP_ADDR (0x81)

usb_endpoint_t usb_endpoint_control_out = {
    .address = USB_CONTROL_OUT_EP_ADDR,
    .device = &usb_device,
    .in = &usb_endpoint_control_in,
    .out = &usb_endpoint_control_out,
    .setup_complete = usb_setup_complete,
    .transfer_complete = usb_control_out_complete,
};
USB_DEFINE_QUEUE(usb_endpoint_control_out, 4);

usb_endpoint_t usb_endpoint_control_in = {
    .address = USB_CONTROL_IN_EP_ADDR,
    .device = &usb_device,
    .in = &usb_endpoint_control_in,
    .out = &usb_endpoint_control_out,
    .setup_complete = 0,
    .transfer_complete = usb_control_in_complete,
};
static USB_DEFINE_QUEUE(usb_endpoint_control_in, 4);

// NOTE: Endpoint number for IN and OUT are different. I wish I had some
// evidence that having BULK IN and OUT on separate endpoint numbers was
// actually a good idea. Seems like everybody does it that way, but why?

usb_endpoint_t usb_endpoint_int_in = {
    .address = USB_INT_IN_EP_ADDR,
    .device = &usb_device,
    .in = &usb_endpoint_int_in,
    .out = 0,
    .setup_complete = 0,
    .transfer_complete = usb_queue_transfer_complete};
static USB_DEFINE_QUEUE(usb_endpoint_int_in, 1);

usb_endpoint_t usb_endpoint_bulk_in = {
    .address = USB_BULK_IN_EP_ADDR,
    .device = &usb_device,
    .in = &usb_endpoint_bulk_in,
    .out = 0,
    .setup_complete = 0,
    .transfer_complete = usb_queue_transfer_complete};
static USB_DEFINE_QUEUE(usb_endpoint_bulk_in, 1);

usb_endpoint_t usb_endpoint_bulk_out = {
    .address = USB_BULK_OUT_EP_ADDR,
    .device = &usb_device,
    .in = 0,
    .out = &usb_endpoint_bulk_out,
    .setup_complete = 0,
    .transfer_complete = usb_queue_transfer_complete};
static USB_DEFINE_QUEUE(usb_endpoint_bulk_out, 1);

extern void usb0_isr(void);

CH_IRQ_HANDLER(USB0_IRQHandler) {
    // CH_IRQ_PROLOGUE();
    usb0_isr();
    // CH_IRQ_EPILOGUE();
}

void usb_configuration_changed(usb_device_t* const device) {
    (void)device;

    usb_endpoint_init(&usb_endpoint_int_in);
    usb_endpoint_init(&usb_endpoint_bulk_in);
    usb_endpoint_init(&usb_endpoint_bulk_out);
}

// void usb_transfer(void) {
//     if (scsi_running) {
//         transfer_complete = false;
//         usb_transfer_schedule_block(
//             &usb_endpoint_bulk_out,
//             &usb_bulk_buffer[0x4000],
//             USB_TRANSFER_SIZE,
//             scsi_bulk_transfer_complete,
//             NULL);

// while (!transfer_complete)
//     ;

// msd_cbw_t* msd_cbw_data = (msd_cbw_t*)&usb_bulk_buffer[0x4000];

// if (msd_cbw_data->signature == MSD_CBW_SIGNATURE) {
//     scsi_command(msd_cbw_data);
// }
// }
// }

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

    // while (true) {
    //     usb_transfer();
    // }
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

usb_request_status_t usb_get_control_line_state_request(usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage) {
    // usb_transfer_schedule_block(
    //     endpoint->in,
    //     &endpoint->buffer,
    //     0,
    //     NULL,
    //     NULL);

    if (stage == USB_TRANSFER_STAGE_SETUP) {
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

// volatile bool transfer_complete = false;
// void setup_transfer_complete(void* user_data, unsigned int bytes_transferred) {
//     (void)user_data;
//     (void)bytes_transferred;

// transfer_complete = true;
// }

// usb_request_status_t set_line_coding(usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage, uint16_t length) {
//     uint8_t buf[7];

// usb_transfer_schedule_block(
//     &usb_endpoint_control_out,
//     &buf[0],
//     7,
//     setup_transfer_complete,
//     NULL);

// while (!transfer_complete)
//     ;

// usb_transfer_schedule_block(
//     endpoint->in,
//     &endpoint->buffer,
//     0,
//     NULL,
//     NULL);

// usb_transfer_schedule_ack(endpoint->out);

// return USB_REQUEST_STATUS_OK;
// }

usb_request_status_t __attribute__((optimize("O0"))) usb_class_request(usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage) {
    usb_request_status_t status = USB_REQUEST_STATUS_STALL;

    volatile uint8_t request = endpoint->setup.request;

    if (request == 0x21)  // GET LINE CODING REQUEST
        return usb_get_line_coding_request(endpoint, stage);

    if (request == 0x22)  // GET CONTROL LINE STATE REQUEST
        return usb_get_control_line_state_request(endpoint, stage);

    if (request == 0x20)  // SET LINE CODING REQUEST
        return usb_set_line_coding_request(endpoint, stage);

    while (1) {
        chThdSleepMilliseconds(100);
    }

    return USB_REQUEST_STATUS_OK;

    // if (stage == USB_TRANSFER_STAGE_SETUP) {
    //     if (request == 0x20)  // SET LINE CODING REQUEST
    //         return send_usb_ack(endpoint, stage);
    //     if (request == 0x21)  // GET LINE CODING REQUEST
    //         return send_usb_ack(endpoint, stage);
    //     if (request == 0x22)  // GET CONTROL LINE STATE REQUEST
    //         return send_usb_ack(endpoint, stage);

    // while (1) {
    //     ;
    // }

    // } else if (stage == USB_TRANSFER_STAGE_DATA) {
    //     if (request == 0x20)  // SET LINE CODING REQUEST
    //         return send_usb_ack(endpoint, stage);
    //     if (request == 0x21)  // GET LINE CODING REQUEST
    //         return send_usb_ack(endpoint, stage);
    //     if (request == 0x22)  // GET CONTROL LINE STATE REQUEST
    //         return send_usb_ack(endpoint, stage);

    // while (1) {
    //     chThdSleepMilliseconds(100);
    // }
    // } else if (stage == USB_TRANSFER_STAGE_STATUS) {
    // if (request == 0x20)  // SET LINE CODING REQUEST
    //     return send_usb_ack(endpoint, stage);
    // if (request == 0x21)  // GET LINE CODING REQUEST
    //     return send_usb_ack(endpoint, stage);
    // if (request == 0x22)  // GET CONTROL LINE STATE REQUEST
    //     return send_usb_ack(endpoint, stage);

    // while (1) {
    //     chThdSleepMilliseconds(100);
    // }
    // }

    return status;
}

usb_request_status_t __attribute__((optimize("O0"))) usb_vendor_request(usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage) {
    usb_request_status_t status = USB_REQUEST_STATUS_STALL;

    volatile uint8_t request = endpoint->setup.request;

    // if (request == 0xFE)
    //     return report_max_lun(endpoint, stage);

    while (1) {
        chThdSleepMilliseconds(100);
    }

    return status;
}

const usb_request_handlers_t usb_request_handlers = {
    .standard = usb_standard_request,
    .class = usb_class_request,
    .vendor = usb_vendor_request,
    .reserved = 0};

uint32_t __ldrex(volatile uint32_t* addr) {
    // uint32_t res;
    // __asm__ volatile("ldrex %0, [%1]" : "=r"(res) : "r"(addr));
    // return res;
    // TODO: disable/enable interrupts
    return *addr;
}

uint32_t __strex(uint32_t val, volatile uint32_t* addr) {
    (void)val;
    (void)addr;

    *addr = val;
    // uint32_t res;
    // __asm__ volatile("strex %0, %2, [%1]"
    //                  : "=&r"(res) : "r"(addr), "r"(val));
    // return res;
    return 0;
}

#define USB_VENDOR_ID (0x1D50)
#define USB_PRODUCT_ID (0x6018)
#define USB_API_VERSION (0x0100)

#define USB_WORD(x) (x & 0xFF), ((x >> 8) & 0xFF)
#define USB_MAX_PACKET0 (64)
#define USB_MAX_PACKET_BULK_FS (64)
#define USB_MAX_PACKET_BULK_HS (16)
#define USB_STRING_LANGID (0x0409)

uint8_t usb_descriptor_device[] = {
    18,                          // bLength
    USB_DESCRIPTOR_TYPE_DEVICE,  // bDescriptorType
    USB_WORD(0x0200),            // bcdUSB
    0xef,                        // bDeviceClass
    0x02,                        // bDeviceSubClass
    0x01,                        // bDeviceProtocol
    USB_MAX_PACKET0,             // bMaxPacketSize0
    USB_WORD(USB_VENDOR_ID),     // idVendor
    USB_WORD(USB_PRODUCT_ID),    // idProduct
    USB_WORD(USB_API_VERSION),   // bcdDevice
    0x01,                        // iManufacturer
    0x02,                        // iProduct
    0x03,                        // iSerialNumber
    0x01                         // bNumConfigurations
};

uint8_t usb_descriptor_device_qualifier[] = {
    10,                                    // bLength
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,  // bDescriptorType
    USB_WORD(0x0200),                      // bcdUSB
    0xef,                                  // bDeviceClass
    0x02,                                  // bDeviceSubClass
    0x01,                                  // bDeviceProtocol
    USB_MAX_PACKET0,                       // bMaxPacketSize0
    0x01,                                  // bNumOtherSpeedConfigurations
    0x00                                   // bReserved
};
uint8_t usb_descriptor_configuration_full_speed[] = {
    9,                                                   // bLength
    USB_DESCRIPTOR_TYPE_CONFIGURATION,                   // bDescriptorType
    USB_WORD((16 + 8 + 9 + 5 + 5 + 4 + 5 + 9 + 7 + 7)),  // wTotalLength
    0x02,                                                // bNumInterfaces
    0x01,                                                // bConfigurationValue
    0x00,                                                // iConfiguration
    0x80,                                                // bmAttributes: USB-powered
    250,                                                 // bMaxPower: 500mA

    8,    // bLength
    0xb,  // bDescriptorType
    0,
    2,
    0x02,
    0x02,
    0x00,
    4,

    9,                              // bLength
    USB_DESCRIPTOR_TYPE_INTERFACE,  // bDescriptorType
    0x00,                           // bInterfaceNumber
    0x00,                           // bAlternateSetting
    0x01,                           // bNumEndpoints
    0x02,                           // bInterfaceClass: vendor-specific
    0x02,                           // bInterfaceSubClass
    0x00,                           // bInterfaceProtocol: vendor-specific
    0x04,                           // iInterface

    5,
    0x24,  // ACM
    0x00,
    USB_WORD(0x0110),

    5,
    0x24,
    0x01,
    0x00,
    0x01,

    4,
    0x24,
    0x02,
    0x02,

    5,
    0x24,
    0x06,
    0x00,
    0x01,

    7,                             // bLength
    USB_DESCRIPTOR_TYPE_ENDPOINT,  // bDescriptorType
    USB_INT_IN_EP_ADDR,            // bEndpointAddress
    0x03,                          // bmAttributes: BULK
    USB_WORD(16),                  // wMaxPacketSize
    0xFF,                          // bInterval: no NAK

    9,                              // bLength
    USB_DESCRIPTOR_TYPE_INTERFACE,  // bDescriptorType
    0x01,                           // bInterfaceNumber
    0x00,                           // bAlternateSetting
    0x02,                           // bNumEndpoints
    0x0a,                           // bInterfaceClass: vendor-specific
    0x00,                           // bInterfaceSubClass
    0x00,                           // bInterfaceProtocol: vendor-specific
    0x00,                           // iInterface

    7,                                 // bLength
    USB_DESCRIPTOR_TYPE_ENDPOINT,      // bDescriptorType
    USB_BULK_OUT_EP_ADDR,              // bEndpointAddress
    0x02,                              // bmAttributes: BULK
    USB_WORD(USB_MAX_PACKET_BULK_FS),  // wMaxPacketSize
    0x01,                              // bInterval: no NAK

    7,                                 // bLength
    USB_DESCRIPTOR_TYPE_ENDPOINT,      // bDescriptorType
    USB_BULK_IN_EP_ADDR,               // bEndpointAddress
    0x02,                              // bmAttributes: BULK
    USB_WORD(USB_MAX_PACKET_BULK_FS),  // wMaxPacketSize
    0x01,                              // bInterval: NAK

    0,  // TERMINATOR
};
uint8_t usb_descriptor_configuration_high_speed[] = {
    9,                                                   // bLength
    USB_DESCRIPTOR_TYPE_CONFIGURATION,                   // bDescriptorType
    USB_WORD((16 + 8 + 9 + 5 + 5 + 4 + 5 + 9 + 7 + 7)),  // wTotalLength
    0x02,                                                // bNumInterfaces
    0x01,                                                // bConfigurationValue
    0x00,                                                // iConfiguration
    0x80,                                                // bmAttributes: USB-powered
    250,                                                 // bMaxPower: 500mA

    8,    // bLength
    0xb,  // bDescriptorType
    0,
    2,
    0x02,
    0x02,
    0x00,
    4,

    9,                              // bLength
    USB_DESCRIPTOR_TYPE_INTERFACE,  // bDescriptorType
    0x00,                           // bInterfaceNumber
    0x00,                           // bAlternateSetting
    0x01,                           // bNumEndpoints
    0x02,                           // bInterfaceClass: vendor-specific
    0x02,                           // bInterfaceSubClass
    0x00,                           // bInterfaceProtocol: vendor-specific
    0x04,                           // iInterface

    5,
    0x24,
    0x00,
    USB_WORD(0x0110),

    5,
    0x24,
    0x01,
    0x00,
    0x01,

    4,
    0x24,
    0x02,
    0x02,

    5,
    0x24,
    0x06,
    0x00,
    0x01,

    7,                             // bLength
    USB_DESCRIPTOR_TYPE_ENDPOINT,  // bDescriptorType
    USB_INT_IN_EP_ADDR,            // bEndpointAddress
    0x03,                          // bmAttributes: BULK
    USB_WORD(16),                  // wMaxPacketSize
    0xFF,                          // bInterval: no NAK

    9,                              // bLength
    USB_DESCRIPTOR_TYPE_INTERFACE,  // bDescriptorType
    0x01,                           // bInterfaceNumber
    0x00,                           // bAlternateSetting
    0x02,                           // bNumEndpoints
    0x0a,                           // bInterfaceClass: vendor-specific
    0x00,                           // bInterfaceSubClass
    0x00,                           // bInterfaceProtocol: vendor-specific
    0x00,                           // iInterface

    7,                                 // bLength
    USB_DESCRIPTOR_TYPE_ENDPOINT,      // bDescriptorType
    USB_BULK_OUT_EP_ADDR,              // bEndpointAddress
    0x02,                              // bmAttributes: BULK
    USB_WORD(USB_MAX_PACKET_BULK_HS),  // wMaxPacketSize
    0x01,                              // bInterval: no NAK

    7,                                 // bLength
    USB_DESCRIPTOR_TYPE_ENDPOINT,      // bDescriptorType
    USB_BULK_IN_EP_ADDR,               // bEndpointAddress
    0x02,                              // bmAttributes: BULK
    USB_WORD(USB_MAX_PACKET_BULK_HS),  // wMaxPacketSize
    0x01,                              // bInterval: NAK

    0,  // TERMINATOR
};
uint8_t usb_descriptor_string_languages[] = {
    0x04,                         // bLength
    USB_DESCRIPTOR_TYPE_STRING,   // bDescriptorType
    USB_WORD(USB_STRING_LANGID),  // wLANGID
};
// clang-format off
 uint8_t usb_descriptor_string_manufacturer[] = {
 	40,                         // bLength
 	USB_DESCRIPTOR_TYPE_STRING, // bDescriptorType
 	'G', 0x00,
 	'r', 0x00,
 	'e', 0x00,
 	'a', 0x00,
 	't', 0x00,
 	' ', 0x00,
 	'S', 0x00,
 	'c', 0x00,
 	'o', 0x00,
 	't', 0x00,
 	't', 0x00,
 	' ', 0x00,
 	'G', 0x00,
 	'a', 0x00,
 	'd', 0x00,
 	'g', 0x00,
 	'e', 0x00,
 	't', 0x00,
 	's', 0x00,
 };
 uint8_t usb_descriptor_string_product[] = {
 	43,                         // bLength
 	USB_DESCRIPTOR_TYPE_STRING, // bDescriptorType
 	'P', 0x00,
 	'o', 0x00,
 	'r', 0x00,
 	't', 0x00,
 	'a', 0x00,
 	'P', 0x00,
 	'a', 0x00,
 	'c', 0x00,
 	'k', 0x00,
 	' ', 0x00,
 	'M', 0x00,
 	'a', 0x00,
 	'y', 0x00,
 	'h', 0x00,
 	'e', 0x00,
 	'm', 0x00,
 };
 uint8_t usb_descriptor_string_config_description[] = {
 	24,                         // bLength
 	USB_DESCRIPTOR_TYPE_STRING, // bDescriptorType
 	'T', 0x00,
 	'r', 0x00,
 	'a', 0x00,
 	'n', 0x00,
 	's', 0x00,
 	'c', 0x00,
 	'e', 0x00,
 	'i', 0x00,
 	'v', 0x00,
 	'e', 0x00,
 	'r', 0x00,
 };
 uint8_t usb_descriptor_string_serial_number[USB_DESCRIPTOR_STRING_SERIAL_BUF_LEN];
 uint8_t* usb_descriptor_strings[] = {
 	usb_descriptor_string_languages,
 	usb_descriptor_string_manufacturer,
 	usb_descriptor_string_product,
 	usb_descriptor_string_config_description,
 	usb_descriptor_string_serial_number,
 	0, // TERMINATOR
 };
 uint8_t wcid_string_descriptor[] = {
 	18,                          // bLength
 	USB_DESCRIPTOR_TYPE_STRING,  // bDescriptorType
 	'M', 0x00,
 	'S', 0x00,
 	'F', 0x00,
 	'T', 0x00,
 	'1', 0x00,
 	'0', 0x00,
 	'0', 0x00,
 	USB_WCID_VENDOR_REQ, // vendor request code for further descriptor
 	0x00
 };
 uint8_t wcid_feature_descriptor[] = {
 	0x28, 0x00, 0x00, 0x00,                  // bLength
 	USB_WORD(0x0100),                        // WCID version
 	USB_WORD(0x0004),                        // WICD descriptor index
 	0x01,                                    // bNumSections
 	0x00,0x00,0x00,0x00,0x00,0x00,0x00,      // Reserved
 	0x00,                                    // bInterfaceNumber
 	0x01,                                    // Reserved
 	'W', 'I', 'N', 'U', 'S', 'B', 0x00,0x00, // Compatible ID, padded with zeros
 	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // Sub-compatible ID
 	0x00,0x00,0x00,0x00,0x00,0x00            // Reserved
 };
