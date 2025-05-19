extern "C" {
#include "usb_serial_device_to_host.h"
#include "usb_serial_cdc.h"
}

#include "usb_serial_shell.hpp"
#include "usb_serial.hpp"
#include "portapack.hpp"
#include "usb_serial_host_to_device.hpp"

#include <libopencm3/cm3/common.h>
#include <libopencm3/lpc43xx/usb.h>

namespace portapack {

void USBSerial::initialize() {
    enable_xtal();
    disable_pll0();

    setup_pll0();
    enable_pll0();

    setup_usb_clock();
    setup_usb_serial_controller();

    init_serial_usb_driver(&SUSBD1);
    shellInit();
    init_host_to_device();
}

void USBSerial::dispatch() {
    if (!connected)
        return;

    if (shell_created == false) {
        shell_created = true;
        create_shell(_eventDispatcher);
    }

    schedule_host_to_device_transfer();
}

void USBSerial::dispatch_transfer() {
    complete_host_to_device_transfer();
}

void USBSerial::on_channel_opened() {
    connected = true;
}

void USBSerial::on_channel_closed() {
    reset_transfer_queues();
    connected = false;
}

void USBSerial::enable_xtal() {
    LPC_CGU->XTAL_OSC_CTRL.ENABLE = 0;
    LPC_CGU->XTAL_OSC_CTRL.HF = 0;
}

void USBSerial::disable_pll0() {
    LPC_CGU->PLL0USB_CTRL.PD = 1;
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
    LPC_CGU->PLL0USB_CTRL.PD = 1;
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
    LPC_CGU->BASE_USB0_CLK.AUTOBLOCK = 1;
    LPC_CGU->BASE_USB0_CLK.CLK_SEL = 0x07;
    LPC_CGU->BASE_USB0_CLK.PD = 0;
}

}  // namespace portapack
