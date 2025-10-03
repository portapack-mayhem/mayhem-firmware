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

#include "scsi.h"
#include "diskio.h"
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/rgu.h>
#include <libopencm3/lpc43xx/wwdt.h>

volatile bool usb_bulk_block_done = false;

void usb_bulk_block_cb(void* user_data, unsigned int bytes_transferred) {
    usb_bulk_block_done = true;

    (void)user_data;
    (void)bytes_transferred;
}

void usb_send_bulk(void* const data, const uint32_t maximum_length) {
    usb_bulk_block_done = false;

    usb_transfer_schedule_block(
        &usb_endpoint_bulk_in,
        data,
        maximum_length,
        usb_bulk_block_cb,
        NULL);

    while (!usb_bulk_block_done)
        ;
}

void usb_receive_bulk(void* const data, const uint32_t maximum_length) {
    usb_bulk_block_done = false;

    usb_transfer_schedule_block(
        &usb_endpoint_bulk_out,
        data,
        maximum_length,
        usb_bulk_block_cb,
        NULL);

    while (!usb_bulk_block_done)
        ;
}

void usb_send_csw(msd_cbw_t* msd_cbw_data, uint8_t status) {
    msd_csw_t csw = {
        .signature = MSD_CSW_SIGNATURE,
        .tag = msd_cbw_data->tag,
        .data_residue = 0,
        .status = status};

    memcpy(&usb_bulk_buffer[0], &csw, sizeof(msd_csw_t));
    usb_send_bulk(&usb_bulk_buffer[0], sizeof(msd_csw_t));
}

uint8_t handle_inquiry(msd_cbw_t* msd_cbw_data) {
    (void)msd_cbw_data;

    scsi_inquiry_response_t ret = {
        0x00,  /* direct access block device     */
        0x80,  /* removable                      */
        0x00,  // 0x04,           /* SPC-2                          */
        0x00,  // 0x02,           /* response data format           */
        0x20,  /* response has 0x20 + 4 bytes    */
        0x00,
        0x00,
        0x00,
        "Mayhem",
        "Portapack MSD",
        {'v', '1', '.', '6'}};

    memcpy(&usb_bulk_buffer[0], &ret, sizeof(scsi_inquiry_response_t));
    usb_send_bulk(&usb_bulk_buffer[0], sizeof(scsi_inquiry_response_t));

    return 0;
}

uint8_t handle_inquiry_serial_number(msd_cbw_t* msd_cbw_data) {
    (void)msd_cbw_data;

    scsi_unit_serial_number_inquiry_response_t ret = {
        .peripheral = 0x00,
        .page_code = 0x80,
        .reserved = 0,
        .page_length = 0x08,
        .serialNumber = "Mayhem"};

    memcpy(&usb_bulk_buffer[0], &ret, sizeof(scsi_unit_serial_number_inquiry_response_t));
    usb_send_bulk(&usb_bulk_buffer[0], sizeof(scsi_unit_serial_number_inquiry_response_t));

    return 0;
}

uint8_t read_format_capacities(msd_cbw_t* msd_cbw_data) {
    uint16_t len = msd_cbw_data->cmd_data[7] << 8 | msd_cbw_data->cmd_data[8];

    if (len != 0) {
        size_t num_blocks = get_capacity();

        scsi_read_format_capacities_response_t ret = {
            .header = {0, 0, 0, 1 * 8 /* num_entries * 8 */},
            .blocknum = {((num_blocks) >> 24) & 0xff, ((num_blocks) >> 16) & 0xff, ((num_blocks) >> 8) & 0xff, num_blocks & 0xff},
            .blocklen = {0b10 /* formated */, 0, (512) >> 8, 0},
        };

        memcpy(&usb_bulk_buffer[0], &ret, sizeof(scsi_read_format_capacities_response_t));
        usb_send_bulk(&usb_bulk_buffer[0], sizeof(scsi_read_format_capacities_response_t));
    }

    return 0;
}

uint8_t read_capacity10(msd_cbw_t* msd_cbw_data) {
    (void)msd_cbw_data;

    size_t num_blocks = get_capacity();

    scsi_read_capacity10_response_t ret = {
        .last_block_addr = cpu_to_be32(num_blocks - 1),
        .block_size = cpu_to_be32(512)};

    memcpy(&usb_bulk_buffer[0], &ret, sizeof(scsi_read_capacity10_response_t));
    usb_send_bulk(&usb_bulk_buffer[0], sizeof(scsi_read_capacity10_response_t));

    return 0;
}

uint8_t request_sense(msd_cbw_t* msd_cbw_data) {
    (void)msd_cbw_data;

    scsi_sense_response_t ret = {
        .byte = {0x70, 0, SCSI_SENSE_KEY_GOOD, 0,
                 0, 0, 0, 8,
                 0, 0, 0, 0,
                 SCSI_ASENSE_NO_ADDITIONAL_INFORMATION, SCSI_ASENSEQ_NO_QUALIFIER, 0, 0,
                 0, 0}};

    memcpy(&usb_bulk_buffer[0], &ret, sizeof(scsi_sense_response_t));
    usb_send_bulk(&usb_bulk_buffer[0], sizeof(scsi_sense_response_t));

    return 0;
}

uint8_t mode_sense6(msd_cbw_t* msd_cbw_data) {
    (void)msd_cbw_data;

    scsi_mode_sense6_response_t ret = {
        .byte = {
            sizeof(scsi_mode_sense6_response_t) - 1,
            0,
            0,
            0}};

    memcpy(&usb_bulk_buffer[0], &ret, sizeof(scsi_mode_sense6_response_t));
    usb_send_bulk(&usb_bulk_buffer[0], sizeof(scsi_mode_sense6_response_t));

    return 0;
}

uint8_t mode_sense10(msd_cbw_t* msd_cbw_data) {
    (void)msd_cbw_data;

    scsi_mode_sense10_response_t ret = {
        .byte = {
            cpu_to_be16(sizeof(scsi_mode_sense6_response_t) - 2),
            0,
            0,
            0}};

    memcpy(&usb_bulk_buffer[0], &ret, sizeof(scsi_mode_sense10_response_t));
    usb_send_bulk(&usb_bulk_buffer[0], sizeof(scsi_mode_sense10_response_t));

    return 0;
}

static data_request_t decode_data_request(const uint8_t* cmd) {
    data_request_t req;
    uint32_t lba;
    uint16_t blk;

    memcpy(&lba, &cmd[2], sizeof(lba));
    memcpy(&blk, &cmd[7], sizeof(blk));

    req.first_lba = be32_to_cpu(lba);
    req.blk_cnt = be16_to_cpu(blk);

    return req;
}

uint8_t data_read10(msd_cbw_t* msd_cbw_data) {
    data_request_t req = decode_data_request(msd_cbw_data->cmd_data);

    for (size_t block_index = 0; block_index < req.blk_cnt; block_index++) {
        read_block(req.first_lba + block_index, &usb_bulk_buffer[0], 1 /* n blocks */);
        usb_send_bulk(&usb_bulk_buffer[0], 512);
    }

    return 0;
}

uint8_t data_write10(msd_cbw_t* msd_cbw_data) {
    data_request_t req = decode_data_request(msd_cbw_data->cmd_data);

    for (size_t block_index = 0; block_index < req.blk_cnt; block_index++) {
        usb_receive_bulk(&usb_bulk_buffer[0], 512);
        write_block(req.first_lba + block_index, &usb_bulk_buffer[0], 1 /* n blocks */);
    }

    return 0;
}

void scsi_command(msd_cbw_t* msd_cbw_data) {
    uint8_t status = 1;

    switch (msd_cbw_data->cmd_data[0]) {
        case SCSI_CMD_INQUIRY:
            if ((msd_cbw_data->cmd_data[1] & 0b1) && msd_cbw_data->cmd_data[2] == 0x80) {
                status = handle_inquiry_serial_number(msd_cbw_data);
            } else if ((msd_cbw_data->cmd_data[1] & 0b11) || msd_cbw_data->cmd_data[2] != 0) {
                status = 1;
            } else {
                status = handle_inquiry(msd_cbw_data);
            }

            break;

        case SCSI_CMD_REQUEST_SENSE:
            status = request_sense(msd_cbw_data);
            break;

        case SCSI_CMD_READ_CAPACITY_10:
            status = read_capacity10(msd_cbw_data);
            break;

        case SCSI_CMD_READ_10:
            status = data_read10(msd_cbw_data);
            break;

        case SCSI_CMD_WRITE_10:
            status = data_write10(msd_cbw_data);
            break;

        case SCSI_CMD_TEST_UNIT_READY:
            status = 0;
            break;

        case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
            status = 0;
            break;

        case SCSI_CMD_MODE_SENSE_6:
            status = mode_sense6(msd_cbw_data);
            break;

        case SCSI_CMD_MODE_SENSE_10:
            status = mode_sense10(msd_cbw_data);
            break;

        case SCSI_CMD_READ_FORMAT_CAPACITIES:
            status = read_format_capacities(msd_cbw_data);
            break;

        case SCSI_CMD_VERIFY_10:
            status = 0;
            break;

        case SCSI_CMD_START_STOP_UNIT:
            SCU_SFSP2_8 = (SCU_SFSP2_8 & ~(7)) | 4;
            struct gpio_t dfu = GPIO(5, 7);
            gpio_output(&dfu);
            gpio_clear(&dfu);

            delay(50 * 40800);

            RESET_CTRL0 = (1 << 0);
            break;
    }

    usb_send_csw(msd_cbw_data, status);
}