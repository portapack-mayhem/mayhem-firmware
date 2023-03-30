#include "sd_over_usb.h"

//extern usb_request_handlers_t usb_request_handlers;


usb_request_status_t dummy(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
    (void)(endpoint);
    (void)(stage);
	return USB_REQUEST_STATUS_OK;
}

static usb_request_handler_fn vendor_request_handler[] = {
	NULL,
	dummy // USB_WCID_VENDOR_REQ
};

static const uint32_t vendor_request_handler_count =
	sizeof(vendor_request_handler) / sizeof(vendor_request_handler[0]);

usb_request_status_t usb_vendor_request(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	usb_request_status_t status = USB_REQUEST_STATUS_STALL;

	if (endpoint->setup.request < vendor_request_handler_count) {
		usb_request_handler_fn handler =
			vendor_request_handler[endpoint->setup.request];
		if (handler) {
			status = handler(endpoint, stage);
		}
	}

	return status;
}

const usb_request_handlers_t usb_request_handlers = {
	.standard = usb_standard_request,
	.class = 0,
	.vendor = usb_vendor_request,
	.reserved = 0,
};


void usb_configuration_changed(usb_device_t* const device)
{
	/* Reset transceiver to idle state until other commands are received */
	// request_transceiver_mode(TRANSCEIVER_MODE_OFF);
	// if (device->configuration->number == 1) {
	// 	// transceiver configuration
	// 	led_on(LED1);
	// } else {
	// 	/* Configuration number equal 0 means usb bus reset. */
	// 	led_off(LED1);
	// }
	usb_endpoint_init(&usb_endpoint_bulk_in);
	usb_endpoint_init(&usb_endpoint_bulk_out);
}


void start_usb(void) {

	/* use XTAL_OSC as clock source for PLL0USB */
	// CGU_PLL0USB_CTRL = 
	// 	CGU_PLL0USB_CTRL_PD(1) /* PLL0 power down */ | 
	// 	CGU_PLL0USB_CTRL_AUTOBLOCK(1) /* Block clock automatically during frequency change */ |
	// 	CGU_PLL0USB_CTRL_CLK_SEL(CGU_SRC_XTAL); /* Clock source selection */

	// while (CGU_PLL0USB_STAT & CGU_PLL0USB_STAT_LOCK_MASK) {}

	/* configure PLL0USB to produce 480 MHz clock from 12 MHz XTAL_OSC */
	/* Values from User Manual v1.4 Table 94, for 12MHz oscillator. */
	
	// CGU_PLL0USB_MDIV = 0x06167FFA;
	// CGU_PLL0USB_NP_DIV = 0x00302062;

	// CGU_PLL0USB_CTRL |=
	// 	CGU_PLL0USB_CTRL_PD(1)  /* PLL0 power down */ | 
	// 	CGU_PLL0USB_CTRL_DIRECTI(1) /* PLL0 direct input */ |
	// 	CGU_PLL0USB_CTRL_DIRECTO(1) /* PLL0 direct output */ | 
	// 	CGU_PLL0USB_CTRL_CLKEN(1) /* PLL0 clock enable */;

	// /* power on PLL0USB and wait until stable */
	// CGU_PLL0USB_CTRL &= ~CGU_PLL0USB_CTRL_PD_MASK /* PLL0 power down */ ;

	// while (!(CGU_PLL0USB_STAT & CGU_PLL0USB_STAT_LOCK_MASK)) {}

	/* use PLL0USB as clock source for USB0 */
	// CGU_BASE_USB0_CLK = CGU_BASE_USB0_CLK_AUTOBLOCK(1) |
	// 	CGU_BASE_USB0_CLK_CLK_SEL(CGU_SRC_PLL0USB);

	detect_hardware_platform();
	pin_setup();
	cpu_clock_init();


	usb_set_configuration_changed_cb(usb_configuration_changed);
	usb_peripheral_reset();

	usb_device_init(0, &usb_device);

	usb_queue_init(&usb_endpoint_control_out_queue);
	usb_queue_init(&usb_endpoint_control_in_queue);
	usb_queue_init(&usb_endpoint_bulk_out_queue);
	usb_queue_init(&usb_endpoint_bulk_in_queue);

	usb_endpoint_init(&usb_endpoint_control_out);
	usb_endpoint_init(&usb_endpoint_control_in);

	nvic_set_priority(NVIC_USB0_IRQ, 255);

	usb_run(&usb_device);


}


void stop_usb(void) {
	usb_peripheral_reset();

}

void irq_usb(void) {
	usb0_isr();
}
