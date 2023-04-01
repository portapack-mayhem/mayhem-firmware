#include "sd_over_usb.h"
#include "scsi.h"

//extern usb_request_handlers_t usb_request_handlers;
#define HALT_UNTIL_DEBUGGING()                                \
  while (!((*(volatile uint32_t *)0xE000EDF0) & (1 << 0))) {} \
  __asm__ __volatile__("bkpt 1")

// uint8_t usb_buffer[USB_TRANSFER_SIZE];


bool scsi_running = false;

usb_request_status_t report_max_lun(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	if (stage == USB_TRANSFER_STAGE_SETUP) {
		endpoint->buffer[0] = 0;
		usb_transfer_schedule_block(
			endpoint->in,
			&endpoint->buffer,
			1,
			NULL,
			NULL);

		usb_transfer_schedule_ack(endpoint->out);

		/* start loop here?*/
		scsi_running = true;
		// Host to Device. receive inquiry LUN
		return USB_REQUEST_STATUS_OK;
	}
	else if (stage == USB_TRANSFER_STAGE_DATA) {
		return USB_REQUEST_STATUS_OK;
	}
	else if (stage == USB_TRANSFER_STAGE_STATUS) {
		// response here?
		return USB_REQUEST_STATUS_OK;
	}
}


usb_request_status_t report_magic_scsi(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{

	// return USB_REQUEST_STATUS_OK;


	// if (endpoint->address != 0x02) {
	// 	HALT_UNTIL_DEBUGGING();
	// 	return USB_REQUEST_STATUS_OK;
	// }

	if (stage == USB_TRANSFER_STAGE_SETUP) {
		return USB_REQUEST_STATUS_OK;
		// usb_bulk_buffer[0] = 0x55;
		// usb_bulk_buffer[1] = 0x55;
		// usb_bulk_buffer[2] = 0x55;
		// usb_bulk_buffer[3] = 0x55;
		// usb_bulk_buffer[4] = 0;

		// usb_transfer_schedule_block(
		// 	&usb_endpoint_bulk_out,
		// 	&usb_bulk_buffer[0x0000],
		// 	5,
		// 	NULL,
		// 	NULL);

		// usb_transfer_schedule_ack(endpoint->out);


		
	}
	else if (stage == USB_TRANSFER_STAGE_DATA) {
		HALT_UNTIL_DEBUGGING();
	}
	else if (stage == USB_TRANSFER_STAGE_STATUS) {
		HALT_UNTIL_DEBUGGING();
	}
	return USB_REQUEST_STATUS_OK;


    (void)(endpoint);
    (void)(stage);
	return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_vendor_request(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	usb_request_status_t status = USB_REQUEST_STATUS_STALL;

	// volatile uint_fast8_t address = endpoint->address;
	volatile uint8_t request = endpoint->setup.request;
	// volatile uint32_t b = 0;

	if (request == 25) { // unknown code
		return report_magic_scsi(endpoint, stage);
	}

	// b = request + (address << 16);
	// HALT_UNTIL_DEBUGGING();


	return status;
}

usb_request_status_t usb_class_request(
	usb_endpoint_t* const endpoint,
	const usb_transfer_stage_t stage)
{
	usb_request_status_t status = USB_REQUEST_STATUS_STALL;

	volatile uint8_t request = endpoint->setup.request;
	// volatile uint32_t b = 0;

	if (request == 0xFE) {
		return report_max_lun(endpoint, stage);
	}

	// b = request + (address << 16);
	// HALT_UNTIL_DEBUGGING();

	// if (address == 0x80) {

	// 	HALT_UNTIL_DEBUGGING();
	// 	return USB_REQUEST_STATUS_OK;
	// }

	// else if (address == 0x00) {
	// 	if (request == 0xFE) {
	// 		return USB_REQUEST_STATUS_OK; // ???????????????????
	// 	}
	// 	b = request + (address << 16);
	// 	HALT_UNTIL_DEBUGGING();
	// }

	// else if (address == 0x02) {
	// 	if (request == 0xFE) {
	// 		return USB_REQUEST_STATUS_OK; // ???????????????????
	// 	}
	// 	//return USB_REQUEST_STATUS_OK;
	// }


	// HALT_UNTIL_DEBUGGING();
	return status;
}

const usb_request_handlers_t usb_request_handlers = {
	.standard = usb_standard_request,
	.class = usb_class_request,
	.vendor = usb_vendor_request,
	.reserved = 0
};

void usb_configuration_changed(usb_device_t* const device)
{
	usb_endpoint_init(&usb_endpoint_bulk_in);
	usb_endpoint_init(&usb_endpoint_bulk_out);
}

void start_usb(void) {
	detect_hardware_platform();
	pin_setup();
	cpu_clock_init(); // required

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

volatile bool transfer_complete = false;
void scsi_bulk_transfer_complete(void* user_data, unsigned int bytes_transferred)
{
	transfer_complete = true;
}

void usb_transfer(void) {
	if (scsi_running) {
		transfer_complete = false;
		usb_transfer_schedule_block(
			&usb_endpoint_bulk_out,
			&usb_bulk_buffer[0],
			USB_TRANSFER_SIZE,
			scsi_bulk_transfer_complete,
			NULL);

		while (!transfer_complete);

		msd_cbw_t *msd_cbw_data = (msd_cbw_t *) &usb_bulk_buffer[0];

		if (msd_cbw_data->signature == MSD_CBW_SIGNATURE){
			scsi_command(msd_cbw_data);
		}

	}
}