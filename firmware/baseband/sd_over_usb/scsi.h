#ifndef __SCSI_H__
#define __SCSI_H__

#include <stddef.h>
#include <common/usb.h>
#include <common/usb_request.h>
#include <common/usb_standard_request.h>
#include "usb_device.h"
#include "usb_endpoint.h"
#include <libopencm3/lpc43xx/m4/nvic.h>
#include <libopencm3/lpc43xx/cgu.h>
#include "platform_detect.h"
#include "hackrf_core.h"
#include "usb_bulk_buffer.h"

#define SCSI_CMD_TEST_UNIT_READY                0x00
#define SCSI_CMD_REQUEST_SENSE                  0x03
#define SCSI_CMD_INQUIRY                        0x12
#define SCSI_CMD_MODE_SENSE_6                   0x1A
#define SCSI_CMD_START_STOP_UNIT                0x1B
#define SCSI_CMD_SEND_DIAGNOSTIC                0x1D
#define SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL   0x1E
#define SCSI_CMD_READ_CAPACITY_10               0x25
#define SCSI_CMD_READ_FORMAT_CAPACITIES         0x23
#define SCSI_CMD_READ_10                        0x28
#define SCSI_CMD_WRITE_10                       0x2A
#define SCSI_CMD_VERIFY_10                      0x2F

#define MSD_CBW_SIGNATURE               0x43425355
#define MSD_CSW_SIGNATURE               0x53425355

#define USB_TRANSFER_SIZE 0x2000

typedef struct {
  uint32_t  signature;
  uint32_t  tag;
  uint32_t  data_len;
  uint8_t   flags;
  uint8_t   lun;
  uint8_t   cmd_len;
  uint8_t   cmd_data[16];
} __attribute__((packed)) msd_cbw_t;

void scsi_command(msd_cbw_t *msd_cbw_data);

#endif /* __SCSI_H__ */