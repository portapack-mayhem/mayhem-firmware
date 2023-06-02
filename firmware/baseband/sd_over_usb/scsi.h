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

#ifndef __SCSI_H__
#define __SCSI_H__

#include <stddef.h>
#include <common/usb.h>
#include <common/usb_request.h>
#include <common/usb_standard_request.h>
#include <hackrf_usb/usb_device.h>
#include <hackrf_usb/usb_endpoint.h>
#include <libopencm3/lpc43xx/m4/nvic.h>
#include <libopencm3/lpc43xx/cgu.h>
#include "platform_detect.h"
#include "hackrf_core.h"
#include "usb_bulk_buffer.h"

#define SCSI_CMD_TEST_UNIT_READY 0x00
#define SCSI_CMD_REQUEST_SENSE 0x03
#define SCSI_CMD_INQUIRY 0x12
#define SCSI_CMD_MODE_SENSE_6 0x1A
#define SCSI_CMD_MODE_SENSE_10 0x5A
#define SCSI_CMD_START_STOP_UNIT 0x1B
#define SCSI_CMD_SEND_DIAGNOSTIC 0x1D
#define SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL 0x1E
#define SCSI_CMD_READ_CAPACITY_10 0x25
#define SCSI_CMD_READ_FORMAT_CAPACITIES 0x23
#define SCSI_CMD_READ_10 0x28
#define SCSI_CMD_WRITE_10 0x2A
#define SCSI_CMD_VERIFY_10 0x2F

#define SCSI_SENSE_KEY_GOOD 0x00
#define SCSI_SENSE_KEY_RECOVERED_ERROR 0x01
#define SCSI_SENSE_KEY_NOT_READY 0x02
#define SCSI_SENSE_KEY_MEDIUM_ERROR 0x03
#define SCSI_SENSE_KEY_HARDWARE_ERROR 0x04
#define SCSI_SENSE_KEY_ILLEGAL_REQUEST 0x05
#define SCSI_SENSE_KEY_UNIT_ATTENTION 0x06
#define SCSI_SENSE_KEY_DATA_PROTECT 0x07
#define SCSI_SENSE_KEY_BLANK_CHECK 0x08
#define SCSI_SENSE_KEY_VENDOR_SPECIFIC 0x09
#define SCSI_SENSE_KEY_COPY_ABORTED 0x0A
#define SCSI_SENSE_KEY_ABORTED_COMMAND 0x0B
#define SCSI_SENSE_KEY_VOLUME_OVERFLOW 0x0D
#define SCSI_SENSE_KEY_MISCOMPARE 0x0E

#define SCSI_ASENSE_NO_ADDITIONAL_INFORMATION 0x00
#define SCSI_ASENSE_LOGICAL_UNIT_NOT_READY 0x04
#define SCSI_ASENSE_INVALID_FIELD_IN_CDB 0x24
#define SCSI_ASENSE_NOT_READY_TO_READY_CHANGE 0x28
#define SCSI_ASENSE_WRITE_PROTECTED 0x27
#define SCSI_ASENSE_FORMAT_ERROR 0x31
#define SCSI_ASENSE_INVALID_COMMAND 0x20
#define SCSI_ASENSE_LBA_OUT_OF_RANGE 0x21
#define SCSI_ASENSE_MEDIUM_NOT_PRESENT 0x3A

#define SCSI_ASENSEQ_NO_QUALIFIER 0x00
#define SCSI_ASENSEQ_FORMAT_COMMAND_FAILED 0x01
#define SCSI_ASENSEQ_INIT_COMMAND_REQUIRED 0x02
#define SCSI_ASENSEQ_OPERATION_IN_PROGRESS 0x07

#define MSD_CBW_SIGNATURE 0x43425355  // USBC
#define MSD_CSW_SIGNATURE 0x53425355  // USBS

#define USB_TRANSFER_SIZE 0x2000

typedef struct {
    uint32_t signature;
    uint32_t tag;
    uint32_t data_len;
    uint8_t flags;
    uint8_t lun;
    uint8_t cmd_len;
    uint8_t cmd_data[16];
} __attribute__((packed)) msd_cbw_t;

typedef struct {
    uint8_t peripheral;
    uint8_t removable;
    uint8_t version;
    uint8_t response_data_format;
    uint8_t additional_length;
    uint8_t sccstp;
    uint8_t bqueetc;
    uint8_t cmdque;
    uint8_t vendorID[8];
    uint8_t productID[16];
    uint8_t productRev[4];
} scsi_inquiry_response_t;

typedef struct {
    uint32_t signature;
    uint32_t tag;
    uint32_t data_residue;
    uint8_t status;
} __attribute__((packed)) msd_csw_t;

typedef struct {
    uint8_t header[4];
    uint8_t blocknum[4];
    uint8_t blocklen[4];
} scsi_read_format_capacities_response_t;

typedef struct {
    uint32_t last_block_addr;
    uint32_t block_size;
} scsi_read_capacity10_response_t;

typedef struct {
    uint8_t byte[18];
} scsi_sense_response_t;

typedef struct {
    uint8_t byte[4];
} scsi_mode_sense6_response_t;

typedef struct {
    uint16_t byte[4];
} scsi_mode_sense10_response_t;

typedef struct {
    uint32_t first_lba;
    uint16_t blk_cnt;
} data_request_t;

typedef struct {
    uint8_t peripheral;
    uint8_t page_code;
    uint8_t reserved;
    uint8_t page_length;
    uint8_t serialNumber[8];
} scsi_unit_serial_number_inquiry_response_t;

static inline uint16_t bswap_16(const uint16_t x)
    __attribute__((warn_unused_result))
    __attribute__((const))
    __attribute__((always_inline));

static inline uint16_t bswap_16(const uint16_t x) {
    uint8_t tmp;
    union {
        uint16_t x;
        uint8_t b[2];
    } data;

    data.x = x;
    tmp = data.b[0];
    data.b[0] = data.b[1];
    data.b[1] = tmp;

    return data.x;
}

static inline uint32_t bswap_32(const uint32_t x)
    __attribute__((warn_unused_result))
    __attribute__((const))
    __attribute__((always_inline));

static inline uint32_t bswap_32(const uint32_t x) {
    uint8_t tmp;
    union {
        uint32_t x;
        uint8_t b[4];
    } data;

    data.x = x;
    tmp = data.b[0];
    data.b[0] = data.b[3];
    data.b[3] = tmp;
    tmp = data.b[1];
    data.b[1] = data.b[2];
    data.b[2] = tmp;

    return data.x;
}

#define be16_to_cpu(x) bswap_16(x)
#define be32_to_cpu(x) bswap_32(x)

#define cpu_to_be16(x) bswap_16(x)
#define cpu_to_be32(x) bswap_32(x)

void scsi_command(msd_cbw_t* msd_cbw_data);

#endif /* __SCSI_H__ */