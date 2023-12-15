extern "C" {
#include "usb_serial.h"
}

#include "usb_serial.hpp"
#include "portapack.hpp"

#include "shell.h"

#include <libopencm3/cm3/common.h>
#include <libopencm3/lpc43xx/usb.h>

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
    setup_usb_serial_controller();

    init_SerialUSBDriver(&SUSBD1);
    shellInit();
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

void USBSerial::usb_controller_run() {
    USB0_USBCMD_D |= USB0_USBCMD_D_RS;
}

}  // namespace portapack
