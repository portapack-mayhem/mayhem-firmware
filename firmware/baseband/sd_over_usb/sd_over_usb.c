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



void start_usb(void) {
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
