#include "usb_serial.h"

// #include "usb_standard_request.h"
// #include "usb_queue.h"

#pragma GCC diagnostic push
// external code, so ignore warnings
#pragma GCC diagnostic ignored "-Wstrict-prototypes"

#include <common/usb.h>
#include <common/usb_request.h>
#include <common/usb_standard_request.h>
#include <hackrf_usb/usb_device.h>
#include <hackrf_usb/usb_endpoint.h>

#pragma GCC diagnostic pop

////////////
// #include <stddef.h>

// #include <libopencm3/lpc43xx/m4/nvic.h>
// #include <libopencm3/lpc43xx/cgu.h>
// #include "platform_detect.h"
// #include "hackrf_core.h"
// #include "usb_bulk_buffer.h"

extern void usb0_isr(void);

CH_IRQ_HANDLER(Vector60) {
    usb0_isr();
}

void usb_configuration_changed(usb_device_t* const device) {
    (void)device;

    usb_endpoint_init(&usb_endpoint_bulk_in);
    usb_endpoint_init(&usb_endpoint_bulk_out);
}

void setup_usb_serial_controller(void) {
    usb_set_configuration_changed_cb(usb_configuration_changed);
    usb_peripheral_reset();

    // usb_phy_enable();
    // usb_controller_reset();
    // usb_controller_set_device_mode();

    // // Set interrupt threshold interval to 0
    // USB0_USBCMD_D &= ~USB0_USBCMD_D_ITC_MASK;

    // // Configure endpoint list address
    // USB0_ENDPOINTLISTADDR = (uint32_t)usb_qh;

    // // Enable interrupts
    // USB0_USBINTR_D = USB0_USBINTR_D_UE | USB0_USBINTR_D_UEE |
    //                  USB0_USBINTR_D_PCE |
    //                  USB0_USBINTR_D_URE
    //                  //| USB0_USBINTR_D_SRE
    //                  | USB0_USBINTR_D_SLE
    //     //| USB0_USBINTR_D_NAKE
    //     ;

    usb_device_init(0, &usb_device);

    usb_queue_init(&usb_endpoint_control_out_queue);
    usb_queue_init(&usb_endpoint_control_in_queue);
    usb_queue_init(&usb_endpoint_bulk_out_queue);
    usb_queue_init(&usb_endpoint_bulk_in_queue);

    usb_endpoint_init(&usb_endpoint_control_out);
    usb_endpoint_init(&usb_endpoint_control_in);
}

usb_request_status_t usb_class_request(usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage) {
    usb_request_status_t status = USB_REQUEST_STATUS_STALL;

    volatile uint8_t request = endpoint->setup.request;

    // if (request == 0xFE)
    //     return report_max_lun(endpoint, stage);

    return status;
}

const usb_request_handlers_t usb_request_handlers = {
    .standard = usb_standard_request,
    .class = usb_class_request,
    .vendor = 0,
    .reserved = 0};

uint32_t __ldrex(volatile uint32_t* addr) {
    // uint32_t res;
    // __asm__ volatile("ldrex %0, [%1]" : "=r"(res) : "r"(addr));
    // return res;
    return *addr;
}

uint32_t __strex(uint32_t val, volatile uint32_t* addr) {
    (void)val;
    (void)addr;
    // uint32_t res;
    // __asm__ volatile("strex %0, %2, [%1]"
    //                  : "=&r"(res) : "r"(addr), "r"(val));
    // return res;
    return 0;
}

#define USB_VENDOR_ID (0x0781) /* SanDisk Corp. */

#ifdef HACKRF_ONE
#define USB_PRODUCT_ID (0xa7a8) /* SD card reader */
#elif JAWBREAKER
#define USB_PRODUCT_ID (0x604B)
#elif RAD1O
#define USB_PRODUCT_ID (0xCC15)
#else
#define USB_PRODUCT_ID (0xFFFF)
#endif

#define USB_API_VERSION (0x0127) /* hardware revision */

#define USB_WORD(x) (x & 0xFF), ((x >> 8) & 0xFF)

#define USB_MAX_PACKET0 (64)
#define USB_MAX_PACKET_BULK_FS (64)
#define USB_MAX_PACKET_BULK_HS (512)

#define USB_BULK_IN_EP_ADDR (0x81)
#define USB_BULK_OUT_EP_ADDR (0x02)

#define USB_STRING_LANGID (0x0409)

uint8_t usb_descriptor_device[] = {
    18,                          // bLength
    USB_DESCRIPTOR_TYPE_DEVICE,  // bDescriptorType
    USB_WORD(0x0200),            // bcdUSB
    0x00,                        // bDeviceClass
    0x00,                        // bDeviceSubClass
    0x00,                        // bDeviceProtocol
    USB_MAX_PACKET0,             // bMaxPacketSize0
    USB_WORD(USB_VENDOR_ID),     // idVendor
    USB_WORD(USB_PRODUCT_ID),    // idProduct
    USB_WORD(USB_API_VERSION),   // bcdDevice
    0x01,                        // iManufacturer
    0x02,                        // iProduct
    0x04,                        // iSerialNumber
    0x01                         // bNumConfigurations
};

uint8_t usb_descriptor_device_qualifier[] = {
    10,                                    // bLength
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,  // bDescriptorType
    USB_WORD(0x0200),                      // bcdUSB
    0x00,                                  // bDeviceClass
    0x00,                                  // bDeviceSubClass
    0x00,                                  // bDeviceProtocol
    64,                                    // bMaxPacketSize0
    0x01,                                  // bNumOtherSpeedConfigurations
    0x00                                   // bReserved
};

uint8_t usb_descriptor_configuration_full_speed[] = {
    9,                                  // bLength
    USB_DESCRIPTOR_TYPE_CONFIGURATION,  // bDescriptorType
    USB_WORD(32),                       // wTotalLength
    0x01,                               // bNumInterfaces
    0x01,                               // bConfigurationValue
    0x00,                               // iConfiguration
    0x80,                               // bmAttributes: USB-powered
    250,                                // bMaxPower: 500mA

    9,                              // bLength
    USB_DESCRIPTOR_TYPE_INTERFACE,  // bDescriptorType
    0x00,                           // bInterfaceNumber
    0x00,                           // bAlternateSetting
    0x02,                           // bNumEndpoints
    0x08,                           // bInterfaceClass: vendor-specific
    0x06,                           // bInterfaceSubClass
    0x50,                           // bInterfaceProtocol: vendor-specific
    0x00,                           // iInterface

    7,                                 // bLength
    USB_DESCRIPTOR_TYPE_ENDPOINT,      // bDescriptorType
    USB_BULK_IN_EP_ADDR,               // bEndpointAddress
    0x02,                              // bmAttributes: BULK
    USB_WORD(USB_MAX_PACKET_BULK_FS),  // wMaxPacketSize
    0x00,                              // bInterval: no NAK

    7,                                 // bLength
    USB_DESCRIPTOR_TYPE_ENDPOINT,      // bDescriptorType
    USB_BULK_OUT_EP_ADDR,              // bEndpointAddress
    0x02,                              // bmAttributes: BULK
    USB_WORD(USB_MAX_PACKET_BULK_FS),  // wMaxPacketSize
    0x00,                              // bInterval: no NAK

    0,  // TERMINATOR
};

uint8_t usb_descriptor_configuration_high_speed[] = {
    9,                                  // bLength
    USB_DESCRIPTOR_TYPE_CONFIGURATION,  // bDescriptorType
    USB_WORD(32),                       // wTotalLength
    0x01,                               // bNumInterfaces
    0x01,                               // bConfigurationValue
    0x00,                               // iConfiguration
    0x80,                               // bmAttributes: USB-powered
    250,                                // bMaxPower: 500mA

    9,                              // bLength
    USB_DESCRIPTOR_TYPE_INTERFACE,  // bDescriptorType
    0x00,                           // bInterfaceNumber
    0x00,                           // bAlternateSetting
    0x02,                           // bNumEndpoints
    0x08,                           // bInterfaceClass: vendor-specific
    0x06,                           // bInterfaceSubClass
    0x50,                           // bInterfaceProtocol: vendor-specific
    0x00,                           // iInterface

    7,                                 // bLength
    USB_DESCRIPTOR_TYPE_ENDPOINT,      // bDescriptorType
    USB_BULK_IN_EP_ADDR,               // bEndpointAddress
    0x02,                              // bmAttributes: BULK
    USB_WORD(USB_MAX_PACKET_BULK_HS),  // wMaxPacketSize
    0x00,                              // bInterval: no NAK

    7,                                 // bLength
    USB_DESCRIPTOR_TYPE_ENDPOINT,      // bDescriptorType
    USB_BULK_OUT_EP_ADDR,              // bEndpointAddress
    0x02,                              // bmAttributes: BULK
    USB_WORD(USB_MAX_PACKET_BULK_HS),  // wMaxPacketSize
    0x00,                              // bInterval: no NAK

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
#ifdef HACKRF_ONE
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

#elif JAWBREAKER
	36,                         // bLength
	USB_DESCRIPTOR_TYPE_STRING, // bDescriptorType
	'H', 0x00,
	'a', 0x00,
	'c', 0x00,
	'k', 0x00,
	'R', 0x00,
	'F', 0x00,
	' ', 0x00,
	'J', 0x00,
	'a', 0x00,
	'w', 0x00,
	'b', 0x00,
	'r', 0x00,
	'e', 0x00,
	'a', 0x00,
	'k', 0x00,
	'e', 0x00,
	'r', 0x00,
#elif RAD1O
	12,                         // bLength
	USB_DESCRIPTOR_TYPE_STRING, // bDescriptorType
	'r', 0x00,
	'a', 0x00,
	'd', 0x00,
	'1', 0x00,
	'o', 0x00,
#else
	14,                         // bLength
	USB_DESCRIPTOR_TYPE_STRING, // bDescriptorType
	'H', 0x00,
	'a', 0x00,
	'c', 0x00,
	'k', 0x00,
	'R', 0x00,
	'F', 0x00,
#endif
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

#ifdef DFU_MODE
uint8_t usb_descriptor_string_serial_number[] = {
	30,                         // bLength
	USB_DESCRIPTOR_TYPE_STRING, // bDescriptorType
	'R', 0x00,
	'u', 0x00,
	'n', 0x00,
	'n', 0x00,
	'i', 0x00,
	'n', 0x00,
	'g', 0x00,
	'F', 0x00,
	'r', 0x00,
	'o', 0x00,
	'm', 0x00,
	'R', 0x00,
	'A', 0x00,
	'M', 0x00,
};
#else
uint8_t usb_descriptor_string_serial_number[USB_DESCRIPTOR_STRING_SERIAL_BUF_LEN];
#endif

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
