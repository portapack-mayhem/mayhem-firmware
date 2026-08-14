/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "sd_over_usb.h"
#include "scsi.h"
#include "usb_descriptor.h"
#include <rom_iap.h>
#include "delay.h"

#include <string.h>

/* Defined in hackrf/firmware/hackrf_usb/usb_device.c. After upstream commit
 * 85dfacf6 ("Universalize: firmware/hackrf_usb"), the global `usb_device` is
 * left zero-initialized and is populated at runtime from per-board
 * `usb_device_*` constants inside hackrf_usb.c. The sd_over_usb baseband
 * does not link hackrf_usb.c, so populate `usb_device` ourselves before
 * usb_run(). The descriptor fields are const-qualified, so use the upstream
 * memcpy-from-template pattern. */
extern usb_configuration_t* usb_configurations[];

static const usb_device_t usb_device_sd_over_usb = {
#ifdef IS_NOT_PRALINE
.descriptor = usb_descriptor_device_hackrf,
.descriptor_strings = usb_descriptor_strings_hackrf_one,
#endif
#ifdef IS_PRALINE
.descriptor = usb_descriptor_device_hackrf,
.descriptor_strings = usb_descriptor_strings_praline,
#endif
    .qualifier_descriptor = usb_descriptor_device_qualifier,
    .configurations = &usb_configurations,
    .configuration = 0,
    .wcid_string_descriptor = wcid_string_descriptor,
    .wcid_feature_descriptor = wcid_feature_descriptor,
};

volatile bool scsi_running = false;

usb_request_status_t report_max_lun(
    usb_endpoint_t* const endpoint,
    const usb_transfer_stage_t stage) {
    if (stage == USB_TRANSFER_STAGE_SETUP) {
        endpoint->buffer[0] = 0;
        usb_transfer_schedule_block(
            endpoint->in,
            &endpoint->buffer,
            1,
            NULL,
            NULL);
    } else if (stage == USB_TRANSFER_STAGE_DATA) {
        usb_transfer_schedule_ack(endpoint->out);

        scsi_running = true;
    }

    return USB_REQUEST_STATUS_OK;
}

usb_request_status_t usb_class_request(usb_endpoint_t* const endpoint, const usb_transfer_stage_t stage) {
    usb_request_status_t status = USB_REQUEST_STATUS_STALL;

    volatile uint8_t request = endpoint->setup.request;

    if (request == 0xFE)
        return report_max_lun(endpoint, stage);

    return status;
}

const usb_request_handlers_t usb_request_handlers = {
    .standard = usb_standard_request,
    .class = usb_class_request,
    .vendor = 0,
    .reserved = 0};

void usb_configuration_changed(usb_device_t* const device) {
    (void)device;

    usb_endpoint_init(&usb_endpoint_bulk_in, false);
    usb_endpoint_init(&usb_endpoint_bulk_out, false);
}

void usb_set_descriptor_by_serial_number(void)
{
	iap_cmd_res_t iap_cmd_res;

	/* Read IAP Serial Number Identification */
	iap_cmd_res.cmd_param.command_code = IAP_CMD_READ_SERIAL_NO;
	iap_cmd_call(&iap_cmd_res);

	if (iap_cmd_res.status_res.status_ret == CMD_SUCCESS) {
		usb_descriptor_string_serial_number[0] =
			USB_DESCRIPTOR_STRING_SERIAL_BUF_LEN;
		usb_descriptor_string_serial_number[1] = USB_DESCRIPTOR_TYPE_STRING;

		/* 32 characters of serial number, convert to UTF-16LE */
		for (size_t i = 0; i < USB_DESCRIPTOR_STRING_SERIAL_LEN; i++) {
			const uint_fast8_t nibble =
				(iap_cmd_res.status_res.iap_result[i >> 3] >>
				 (28 - (i & 7) * 4)) &
				0xf;
			const char c =
				(nibble > 9) ? ('a' + nibble - 10) : ('0' + nibble);
			usb_descriptor_string_serial_number[2 + i * 2] = c;
			usb_descriptor_string_serial_number[3 + i * 2] = 0x00;
		}
	} else {
		usb_descriptor_string_serial_number[0] = 2;
		usb_descriptor_string_serial_number[1] = USB_DESCRIPTOR_TYPE_STRING;
	}
}

void start_usb(void) {
	// Copy M0 image from ROM before SPIFI is disabled
	// m0_rom_to_ram();

	// This will be cleared if any self-test check fails.
	// selftest.report.pass = true;

	// Detect hardware platform before we do anything else.
	detect_hardware_platform();
	board_id_t board_id = detected_platform();

	// i2c_bus_start(&i2c0, &i2c_config_fast_clock);

	pins_shutdown();
	sgpio_pin_shutdown(&sgpio_config);
	rf_path_pin_shutdown();
	if (board_id != BOARD_ID_RAD1O) {
		clock_gen_shutdown();
	}
	delay_ms(10);
	pins_setup();
	cpld_jtag_pin_setup();
	// mixer_bus_setup(&mixer);
	// sgpio_configure_pin_functions(&sgpio_config);
	// rf_path_pin_setup(&rf_path);
// #ifdef IS_PRALINE
// 	if (IS_PRALINE) {
// 		enable_3v3aux_power();
// 	#if !defined(DFU_MODE) && !defined(RAM_MODE)
// 		enable_1v2_power();
// 		enable_rf_power();
// 		/*
// 		 * On Praline, the clock generator power supply comes from 3V3FPGA
// 		 * which is enabled when 1V2FPGA is turned on.
// 		 */
// 		clock_gen_init();
// 	#endif
// 	}
// #endif
// #ifdef IS_NOT_PRALINE
// 	if (IS_NOT_PRALINE) {
// 		enable_1v8_power();
// 	#ifdef IS_NOT_RAD1O
// 		if (IS_NOT_RAD1O) {
// 			/*
// 			 * On rad1o, the clock generator power supply comes from the RF supply
// 			 * which is enabled later. On H1 and Jawbreaker, the clock generator is
// 			 * on the main 3V3 supply.
// 			 */
// 			clock_gen_init();
// 		}
// 	#endif
// 	}
// #endif
// 	tuning_setup();
// #ifdef IS_HACKRF_ONE
// 	if (IS_HACKRF_ONE) {
// 		// Set up mixer before enabling RF power, because its
// 		// GPO is used to control the antenna bias tee.
// 		mixer_setup(&mixer, RFFC5071_VARIANT);
// 	}
// #endif
// #ifdef IS_H1_OR_RAD1O
// 	if (IS_H1_OR_RAD1O) {
// 		enable_rf_power();
// 	}
// #endif
// #ifdef IS_RAD1O
// 	if (IS_RAD1O) {
// 		clock_gen_init();
// 	}
// #endif
	cpu_clock_init();

	/* Clock speed has changed, adjust I2C clock */
	// i2c_bus_start(&i2c0, &i2c_config_fast_clock);

// 	/* Wake the M0 */
// 	ipc_halt_m0();
// 	ipc_start_m0((uint32_t) &__ram_m0_start__);

// #ifdef IS_NOT_PRALINE
// 	if (IS_NOT_PRALINE) {
// 		if (!cpld_jtag_sram_load(&jtag_cpld)) {
// 			halt_and_flash(1000);
// 		}
// 	}
// #endif
// #ifdef IS_PRALINE
// 	if (IS_PRALINE) {
// 	#if defined(DFU_MODE) || defined(RAM_MODE)
// 		selftest.fpga_image_load = SKIPPED;
// 		selftest.report.pass = false;
// 	#else
// 		fpga_image_load(&fpga_loader, 0);
// 	#endif
// 		delay_us(100);
// 		fpga_spi_selftest();
// 		fpga_sgpio_selftest();
// 	}
// #endif

	// radio.update_cb = radio_changed;
	// radio_init(&radio);

// #ifdef IS_EXPANSION_COMPATIBLE
// 	if (IS_EXPANSION_COMPATIBLE) {
// 		portapack_init();
// 	}
// #endif

#ifndef DFU_MODE
	usb_set_descriptor_by_serial_number();
#endif

	usb_set_configuration_changed_cb(usb_configuration_changed);
	usb_peripheral_reset();

#ifdef IS_HACKRF_ONE
	if (IS_HACKRF_ONE) {
		memcpy(&usb_device,
		       &usb_device_sd_over_usb,
		       sizeof(usb_device_sd_over_usb));
	}
#endif
#ifdef IS_JAWBREAKER
	if (IS_JAWBREAKER) {
		memcpy(&usb_device,
		       &usb_device_jawbreaker,
		       sizeof(usb_device_jawbreaker));
	}
#endif
#ifdef IS_RAD1O
	if (IS_RAD1O) {
		memcpy(&usb_device, &usb_device_rad1o, sizeof(usb_device_rad1o));
	}
#endif
#ifdef IS_PRALINE
	if (IS_PRALINE) {
		memcpy(&usb_device, &usb_device_sd_over_usb, sizeof(usb_device_sd_over_usb));
	}
#endif
	usb_device_init(0, &usb_device);

	usb_queue_init(&usb_endpoint_control_out_queue);
	usb_queue_init(&usb_endpoint_control_in_queue);
	usb_queue_init(&usb_endpoint_bulk_out_queue);
	usb_queue_init(&usb_endpoint_bulk_in_queue);

	usb_endpoint_init(&usb_endpoint_control_out, false);
	usb_endpoint_init(&usb_endpoint_control_in, true);

	nvic_set_priority(NVIC_USB0_IRQ, 255);

	// hackrf_ui()->init();

	usb_run(&usb_device);
}

void stop_usb(void) {
    usb_peripheral_reset();
}

void irq_usb(void) {
    usb0_isr();
}

volatile bool transfer_complete = false;
void scsi_bulk_transfer_complete(void* user_data, unsigned int bytes_transferred) {
    (void)user_data;
    (void)bytes_transferred;

    transfer_complete = true;
}

void usb_transfer(void) {
    if (scsi_running) {
        transfer_complete = false;
        usb_transfer_schedule_block(
            &usb_endpoint_bulk_out,
            &usb_bulk_buffer[0x4000],
            USB_TRANSFER_SIZE,
            scsi_bulk_transfer_complete,
            NULL);

        while (!transfer_complete);

        msd_cbw_t* msd_cbw_data = (msd_cbw_t*)&usb_bulk_buffer[0x4000];

        if (msd_cbw_data->signature == MSD_CBW_SIGNATURE) {
            scsi_command(msd_cbw_data);
        }
    }
}
