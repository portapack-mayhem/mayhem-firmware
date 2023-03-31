#include "scsi.h"


#define HALT_UNTIL_DEBUGGING()                                \
  while (!((*(volatile uint32_t *)0xE000EDF0) & (1 << 0))) {} \
  __asm__ __volatile__("bkpt 1")

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


static const scsi_inquiry_response_t default_scsi_inquiry_response = {
    0x00,           /* direct access block device     */
    0x80,           /* removable                      */
    0x04,           /* SPC-2                          */
    0x02,           /* response data format           */
    0x20,           /* response has 0x20 + 4 bytes    */
    0x00,
    0x00,
    0x00,
    "Mayhem",
    "Mass Storage",
    {'v','1','.','6'}
};

typedef struct {
  uint32_t  signature;
  uint32_t  tag;
  uint32_t  data_residue;
  uint8_t   status;
} __attribute__((packed)) msd_csw_t;

#define MSD_CBW_SIGNATURE               0x43425355
#define MSD_CSW_SIGNATURE               0x53425355

// void handle_inquiry_4(void* user_data, unsigned int bytes_transferred) {
//     HALT_UNTIL_DEBUGGING();
// }

// void handle_inquiry_3(void* user_data, unsigned int bytes_transferred)
// {
//     //msd_cbw_t *msd_cbw_data = (msd_cbw_t *)user_data;

//     //HALT_UNTIL_DEBUGGING();


// }


volatile bool inquiry_stage_one_send = false;
void inquiry_cb(void* user_data, unsigned int bytes_transferred)
{
    inquiry_stage_one_send = true;

    //HALT_UNTIL_DEBUGGING();

    //faulty
    // usb_transfer_schedule_block(
    //     &usb_endpoint_bulk_out,
    //     &usb_bulk_buffer[0],
    //     USB_TRANSFER_SIZE,
    //     handle_inquiry_3,
    //     msd_cbw_data);


//does not send

}

void handle_inquiry(msd_cbw_t *msd_cbw_data){
    inquiry_stage_one_send = false;
    memcpy(&usb_bulk_buffer[0], &default_scsi_inquiry_response, sizeof(scsi_inquiry_response_t));
    usb_transfer_schedule_block(
        &usb_endpoint_bulk_in,
        &usb_bulk_buffer[0],
        sizeof(scsi_inquiry_response_t),
        inquiry_cb,
        msd_cbw_data);

    while (!inquiry_stage_one_send);

    msd_csw_t csw = {
        .signature = MSD_CSW_SIGNATURE,
        .tag = msd_cbw_data->tag,
        .data_residue = 0,
        .status = 0
    };

    memcpy(&usb_bulk_buffer[0x4000], &csw, sizeof(msd_csw_t));
    // does not gets send
    usb_transfer_schedule_block(
        &usb_endpoint_bulk_in,
        &usb_bulk_buffer[0x4000],
        sizeof(msd_csw_t),
        NULL, //handle_inquiry_3,
        NULL); //msd_cbw_data);

}

void scsi_command(msd_cbw_t *msd_cbw_data) {

  switch (msd_cbw_data->cmd_data[0]) {
    case SCSI_CMD_INQUIRY:
        handle_inquiry(msd_cbw_data);


        break;

    default:
        HALT_UNTIL_DEBUGGING();
        break;
/*
  case SCSI_CMD_REQUEST_SENSE:
    ret = request_sense(scsip, cmd);
    break;

  case SCSI_CMD_READ_CAPACITY_10:
    ret = read_capacity10(scsip, cmd);
    break;

  case SCSI_CMD_READ_10:
    ret = data_read_write10(scsip, cmd);
    break;

  case SCSI_CMD_WRITE_10:
    ret = data_read_write10(scsip, cmd);
    break;

  case SCSI_CMD_TEST_UNIT_READY:
    ret = test_unit_ready(scsip, cmd);
    break;

  case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
    ret = cmd_ignored(scsip, cmd);
    break;

  case SCSI_CMD_MODE_SENSE_6:
    ret = mode_sense6(scsip, cmd);
    break;

  case SCSI_CMD_READ_FORMAT_CAPACITIES:
    ret = read_format_capacities(scsip, cmd);
    break;

  case SCSI_CMD_VERIFY_10:
    ret = cmd_ignored(scsip, cmd);
    break;

  default:
    ret = cmd_unhandled(scsip, cmd);
    break;
    */
  }

//   if (ret == SCSI_SUCCESS)
//     set_sense_ok(scsip);


}