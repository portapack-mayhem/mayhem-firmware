extern "C" {
#include "usb_serial.h"
}

#include "usb_serial.hpp"
#include "portapack.hpp"

#include "shell.h"

// #include <libopencm3/lpc43xx/m4/nvic.h>
#include <libopencm3/cm3/common.h>
#include <libopencm3/lpc43xx/usb.h>

// #include <libopencm3/cm3/nvic.h>
// #include "nvic.h"

SerialUSBDriver SUSBD1;

#define NVIC_USB0_IRQ 8
#define SCS_SHPR(ipr_id) MMIO8(SCS_BASE + 0xD18 + ipr_id)

extern "C" {
void nvic_enable_irq(uint8_t irqn) {
    NVIC_ISER(irqn / 32) = (1 << (irqn % 32));
}
void usb_serial_create_bulk_out_thread();
}

namespace portapack {

// #define MMIO32(addr) (*(volatile uint32_t*)(addr))
#define SHELL_WA_SIZE THD_WA_SIZE(1024)

static void cmd_test(BaseSequentialStream* chp, int argc, char* argv[]) {
    Thread* tp;
    chDbgPanic("cmd_test");
    (void)argv;
}

static const ShellCommand commands[] = {
    {"test", cmd_test},
    {NULL, NULL}};

static const ShellConfig shell_cfg1 = {
    (BaseSequentialStream*)&SUSBD1,
    commands};

void USBSerial::initialize() {
    enable_xtal();
    disable_pll0();

    setup_pll0();
    enable_pll0();

    setup_usb_clock();

    // LPC_CGU->BASE_OUT_CLK.PD = 1;
    // LPC_CGU->BASE_OUT_CLK.AUTOBLOCK = 0;
    // LPC_CGU->BASE_OUT_CLK.CLK_SEL = 0x07;
    // LPC_CGU->BASE_OUT_CLK.PD = 0;

    // LPC_SCU->SFSCLK0 = (LPC_SCU->SFSCLK0 & ~0xff) | (1 |
    //                                                  1 << 4 |
    //                                                  1 << 5 |
    //                                                  1 << 7);

    // palSetPadMode(24, 0, PAL_MODE_OUTPUT_PUSHPULL);
    // MMIO32(PIN_GROUP24 + PIN0) = (SCU_GPIO_FAST | SCU_CONF_FUNCTION1);

    // CLK0 = (SCU_BASE + 0xC00),    MMIO32(group_pin) = scu_conf;(CLK0, SCU_CLK_IN | SCU_CONF_FUNCTION7);

    setup_usb_serial_controller();

    // nvicEnableVector(RITIMER_OR_WWDT_IRQn, CORTEX_PRIORITY_MASK(CORTEX_PRIORITY_SYSTICK));
    // SCS_SHPR((NVIC_USB0_IRQ & 0xF) - 4) = 255;
    // NVIC_ISER(NVIC_USB0_IRQ / 32) = (1 << (NVIC_USB0_IRQ % 32));

    // usb_controller_run();

    init_SerialUSBDriver(&SUSBD1);
    shellInit();
    //
}

extern "C" {
bool channelOpened = false;
bool shellCreated = false;
void onChannelOpened() {
    channelOpened = true;
}
}

void createShellOnDemand() {
    if (channelOpened && !shellCreated) {
        shellCreated = true;
        shellCreate(&shell_cfg1, SHELL_WA_SIZE, NORMALPRIO);
        usb_serial_create_bulk_out_thread();
    }
}

// void USBSerial::irq_usb() {
//     const uint32_t status = usb_get_status();

// if (status == 0) {
//     // Nothing to do.
//     return;
// }

// if (status & USB0_USBSTS_D_UI) {
//     // USB:
//     // - Completed transaction transfer descriptor has IOC set.
//     // - Short packet detected.
//     // - SETUP packet received.

// usb_check_for_setup_events();
// usb_check_for_transfer_events();

// // TODO: Reset ignored ENDPTSETUPSTAT and ENDPTCOMPLETE flags?
// }

// if (status & USB0_USBSTS_D_SRI) {
//     // Start Of Frame received.
// }

// if (status & USB0_USBSTS_D_PCI) {
//     // Port change detect:
//     // Port controller entered full- or high-speed operational state.
// }

// if (status & USB0_USBSTS_D_SLI) {
//     // Device controller suspend.
// }

// if (status & USB0_USBSTS_D_URI) {
//     // USB reset received.
//     usb_bus_reset(usb_device_usb0);
// }

// if (status & USB0_USBSTS_D_UEI) {
//     // USB error:
//     // Completion of a USB transaction resulted in an error condition.
//     // Set along with USBINT if the TD on which the error interrupt
//     // occurred also had its interrupt on complete (IOC) bit set.
//     // The device controller detects resume signalling only.
// }

// if (status & USB0_USBSTS_D_NAKI) {
//     // Both the TX/RX endpoint NAK bit and corresponding TX/RX endpoint
//     // NAK enable bit are set.
// }
// }

// uint32_t USBSerial::usb_get_status() {
//     // Mask status flags with enabled flag interrupts.
//     const uint32_t status = USB0_USBSTS_D & USB0_USBINTR_D;

// // Clear flags that were just read, leaving alone any flags that
// // were just set (after the read). It's important to read and
// // reset flags atomically! :-)
// USB0_USBSTS_D = status;

// return status;
// }

void USBSerial::enable_xtal() {
    LPC_CGU->XTAL_OSC_CTRL.ENABLE = 0;  // Yep, zero is enabled
    LPC_CGU->XTAL_OSC_CTRL.HF = 0;
}

void USBSerial::disable_pll0() {
    LPC_CGU->PLL0USB_CTRL.PD = 1;  // PLL0 powered down
    LPC_CGU->PLL0USB_CTRL.AUTOBLOCK = 1;

    LPC_CGU->PLL0USB_CTRL.BYPASS = 0;
    LPC_CGU->PLL0USB_CTRL.DIRECTI = 0;
    LPC_CGU->PLL0USB_CTRL.DIRECTO = 0;
    LPC_CGU->PLL0USB_CTRL.CLKEN = 0;
    LPC_CGU->PLL0USB_CTRL.FRM = 0;
    LPC_CGU->PLL0USB_CTRL.CLK_SEL = 0x06;  // 12MHz internal XTAL
}

void USBSerial::setup_pll0() {
    /* use XTAL_OSC as clock source for PLL0USB */

    while (LPC_CGU->PLL0USB_STAT.LOCK) {
    }

    // /* configure PLL0USB to produce 480 MHz clock from 12 MHz XTAL_OSC */
    // /* Values from User Manual v1.4 Table 94, for 12MHz oscillator. */
    LPC_CGU->PLL0USB_MDIV = 0x06167FFA;
    LPC_CGU->PLL0USB_NP_DIV = 0x00302062;
    LPC_CGU->PLL0USB_CTRL.PD = 1;  // PLL0 still powered down
    LPC_CGU->PLL0USB_CTRL.DIRECTI = 1;
    LPC_CGU->PLL0USB_CTRL.DIRECTO = 1;
    LPC_CGU->PLL0USB_CTRL.CLKEN = 1;
}

void USBSerial::enable_pll0() {
    // /* power on PLL0USB and wait until stable */
    LPC_CGU->PLL0USB_CTRL.PD = 0;

    while (!LPC_CGU->PLL0USB_STAT.LOCK) {
    }
}

void USBSerial::setup_usb_clock() {
    /* use PLL0USB as clock source for USB0 */
    // LPC_CGU->BASE_USB0_CLK.PD = 1;
    LPC_CGU->BASE_USB0_CLK.AUTOBLOCK = 1;
    LPC_CGU->BASE_USB0_CLK.CLK_SEL = 0x07;
    LPC_CGU->BASE_USB0_CLK.PD = 0;
}

void USBSerial::reset_usb() {
    // usb_peripheral_reset
    LPC_RGU->RESET_CTRL[0] = 1 << 17;  // RESET_CTRL0_USB0_RST

    while ((LPC_RGU->RESET_ACTIVE_STATUS[0] & (1 << 17)) == 0) {
    }
}

void USBSerial::usb_phy_enable() {
    // enable usb output
    LPC_CREG->CREG0 &= ~(1 << 5);
}

// void USBSerial::usb_controller_reset() {
//     // TODO: Good to disable some USB interrupts to avoid priming new
//     // new endpoints before the controller is reset?
//     usb_reset_all_endpoints();
//     usb_controller_stop();

// // Reset controller. Resets internal pipelines, timers, counters, state
// // machines to initial values. Not recommended when device is in attached
// // state -- effect on attached host is undefined. Detach first by flushing
// // all primed endpoints and stopping controller.
// USB0_USBCMD_D = USB0_USBCMD_D_RST;

// while (usb_controller_is_resetting()) {
// }
// }

// void USBSerial::usb_controller_set_device_mode() {
//     // Set USB0 peripheral mode
//     USB0_USBMODE_D = USB0_USBMODE_D_CM1_0(2);

// // Set device-related OTG flags
// // OTG termination: controls pull-down on USB_DM
// USB0_OTGSC = USB0_OTGSC_OT;
// }

// void USBSerial::usb_reset_all_endpoints() {
//     usb_disable_all_endpoints();
//     usb_clear_all_pending_interrupts();
//     usb_flush_all_primed_endpoints();
// }

// void USBSerial::usb_controller_stop() {
//     USB0_USBCMD_D &= ~USB0_USBCMD_D_RS;
// }

// bool USBSerial::usb_controller_is_resetting() {
//     return (USB0_USBCMD_D & USB0_USBCMD_D_RST) != 0;
// }

// void USBSerial::usb_disable_all_endpoints() {
//     // Endpoint 0 is always enabled. TODO: So why set ENDPTCTRL0?
//     USB0_ENDPTCTRL0 &= ~(USB0_ENDPTCTRL0_RXE | USB0_ENDPTCTRL0_TXE);
//     USB0_ENDPTCTRL1 &= ~(USB0_ENDPTCTRL1_RXE | USB0_ENDPTCTRL1_TXE);
//     USB0_ENDPTCTRL2 &= ~(USB0_ENDPTCTRL2_RXE | USB0_ENDPTCTRL2_TXE);
//     USB0_ENDPTCTRL3 &= ~(USB0_ENDPTCTRL3_RXE | USB0_ENDPTCTRL3_TXE);
//     USB0_ENDPTCTRL4 &= ~(USB0_ENDPTCTRL4_RXE | USB0_ENDPTCTRL4_TXE);
//     USB0_ENDPTCTRL5 &= ~(USB0_ENDPTCTRL5_RXE | USB0_ENDPTCTRL5_TXE);
// }

// void USBSerial::usb_clear_all_pending_interrupts() {
//     usb_clear_pending_interrupts(0xFFFFFFFF);
// }

// void USBSerial::usb_flush_all_primed_endpoints() {
//     usb_flush_primed_endpoints(0xFFFFFFFF);
// }

// void USBSerial::usb_clear_pending_interrupts(const uint32_t mask) {
//     USB0_ENDPTNAK = mask;
//     USB0_ENDPTNAKEN = mask;
//     USB0_USBSTS_D = mask;
//     USB0_ENDPTSETUPSTAT = USB0_ENDPTSETUPSTAT & mask;
//     USB0_ENDPTCOMPLETE = USB0_ENDPTCOMPLETE & mask;
// }

// void USBSerial::usb_flush_primed_endpoints(const uint32_t mask) {
//     usb_wait_for_endpoint_priming_to_finish(mask);
//     usb_flush_endpoints(mask);
//     usb_wait_for_endpoint_flushing_to_finish(mask);
// }

// void USBSerial::usb_wait_for_endpoint_priming_to_finish(const uint32_t mask) {
//     // Wait until controller has parsed new transfer descriptors and prepared
//     // receive buffers.
//     while (USB0_ENDPTPRIME & mask) {
//     }
// }

// void USBSerial::usb_flush_endpoints(const uint32_t mask) {
//     // Clear any primed buffers. If a packet is in progress, that transfer
//     // will continue until completion.
//     USB0_ENDPTFLUSH = mask;
// }

// void USBSerial::usb_wait_for_endpoint_flushing_to_finish(const uint32_t mask) {
//     // Wait until controller has flushed all endpoints / cleared any primed
//     // buffers.
//     while (USB0_ENDPTFLUSH & mask) {
//     }
// }

// // void USBSerial::usb_queue_init(usb_queue_t* const queue) {
// // }

void USBSerial::usb_controller_run() {
    USB0_USBCMD_D |= USB0_USBCMD_D_RS;
}

// void USBSerial::usb_endpoint_init(const usb_endpoint_t* const endpoint) {
//     usb_endpoint_flush(endpoint);

// uint_fast16_t max_packet_size = endpoint->device->descriptor[7];
// usb_transfer_type_t transfer_type = USB_TRANSFER_TYPE_CONTROL;
// const uint8_t* const endpoint_descriptor = usb_endpoint_descriptor(endpoint);
// if (endpoint_descriptor) {
//     max_packet_size =
//         usb_endpoint_descriptor_max_packet_size(endpoint_descriptor);
//     transfer_type =
//         usb_endpoint_descriptor_transfer_type(endpoint_descriptor);
// }

// // TODO: There are more capabilities to adjust based on the endpoint
// // descriptor.
// usb_queue_head_t* const qh = usb_queue_head(endpoint->address);
// qh->capabilities = USB_QH_CAPABILITIES_MULT(0) | USB_QH_CAPABILITIES_ZLT |
//                    USB_QH_CAPABILITIES_MPL(max_packet_size) |
//                    ((transfer_type == USB_TRANSFER_TYPE_CONTROL) ? USB_QH_CAPABILITIES_IOS : 0);
// qh->current_dtd_pointer = 0;
// qh->next_dtd_pointer = USB_TD_NEXT_DTD_POINTER_TERMINATE;
// qh->total_bytes = USB_TD_DTD_TOKEN_TOTAL_BYTES(0) | USB_TD_DTD_TOKEN_MULTO(0);
// qh->buffer_pointer_page[0] = 0;
// qh->buffer_pointer_page[1] = 0;
// qh->buffer_pointer_page[2] = 0;
// qh->buffer_pointer_page[3] = 0;
// qh->buffer_pointer_page[4] = 0;

// // This is how we look up an endpoint structure from an endpoint address:
// qh->_reserved_0 = (uint32_t)endpoint;

// // TODO: Should NAK be enabled? I'm kinda squishy on this...
// // USB0_ENDPTNAKEN |=
// //	USB0_ENDPTNAKEN_EPRNE(1 << endpoint_out->number);

// usb_endpoint_set_type(endpoint, transfer_type);

// usb_endpoint_enable(endpoint);
// }

// void USBSerial::usb_endpoint_flush(const usb_endpoint_t* const endpoint) {
//     const uint_fast8_t endpoint_number = usb_endpoint_number(endpoint->address);
//     usb_queue_flush_endpoint(endpoint);
//     if (usb_endpoint_is_in(endpoint->address)) {
//         usb_flush_primed_endpoints(USB0_ENDPTFLUSH_FETB(1 << endpoint_number));
//     } else {
//         usb_flush_primed_endpoints(USB0_ENDPTFLUSH_FERB(1 << endpoint_number));
//     }
// }

// #define USB_QH_INDEX(endpoint_address)
//     (((endpoint_address & 0xF) * 2) + ((endpoint_address >> 7) & 1))

// usb_queue_head_t* USBSerial::usb_queue_head(const uint_fast8_t endpoint_address) {
//     return &usb_qh[USB_QH_INDEX(endpoint_address)];
// }

// void USBSerial::usb_endpoint_set_type(const usb_endpoint_t* const endpoint, const usb_transfer_type_t transfer_type) {
//     // NOTE: UM10503 section 23.6.24 "Endpoint 1 to 5 control registers" says
//     // that the disabled side of an endpoint must be set to a non-control type
//     // (e.g. bulk, interrupt, or iso).
//     const uint_fast8_t endpoint_number = usb_endpoint_number(endpoint->address);
//     USB0_ENDPTCTRL(endpoint_number) =
//         (USB0_ENDPTCTRL(endpoint_number) &
//          ~(USB0_ENDPTCTRL_TXT1_0_MASK | USB0_ENDPTCTRL_RXT_MASK)) |
//         (USB0_ENDPTCTRL_TXT1_0(transfer_type) |
//          USB0_ENDPTCTRL_RXT(transfer_type));
// }

// void USBSerial::usb_endpoint_enable(const usb_endpoint_t* const endpoint) {
//     const uint_fast8_t endpoint_number = usb_endpoint_number(endpoint->address);
//     if (usb_endpoint_is_in(endpoint->address)) {
//         USB0_ENDPTCTRL(endpoint_number) |=
//             (USB0_ENDPTCTRL_TXE | USB0_ENDPTCTRL_TXR);
//     } else {
//         USB0_ENDPTCTRL(endpoint_number) |=
//             (USB0_ENDPTCTRL_RXE | USB0_ENDPTCTRL_RXR);
//     }
// }

// uint_fast8_t USBSerial::usb_endpoint_number(const uint_fast8_t endpoint_address) {
//     return (endpoint_address & 0xF);
// }

// bool USBSerial::usb_endpoint_is_in(const uint_fast8_t endpoint_address) {
//     return (endpoint_address & 0x80) ? true : false;
// }

}  // namespace portapack
