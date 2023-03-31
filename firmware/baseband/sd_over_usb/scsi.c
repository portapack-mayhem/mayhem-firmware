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
}

void handle_inquiry(msd_cbw_t *msd_cbw_data) {
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
    usb_transfer_schedule_block(
        &usb_endpoint_bulk_in,
        &usb_bulk_buffer[0x4000],
        sizeof(msd_csw_t),
        NULL,
        NULL);

}

typedef struct {
  uint8_t header[4];
  uint8_t blocknum[4];
  uint8_t blocklen[4];
} scsi_read_format_capacities_response_t;

volatile bool capacities_stage_one_send = false;
void capacities_cb(void* user_data, unsigned int bytes_transferred)
{
    capacities_stage_one_send = true;
}

void read_format_capacities(msd_cbw_t *msd_cbw_data) {
    uint16_t len = msd_cbw_data->cmd_data[7] << 8 | msd_cbw_data->cmd_data[8];

    if (len != 0) {
        scsi_read_format_capacities_response_t ret = {
            .header = {0, 0, 0, 1 * 8 /* num_entries * 8 */},
            .blocknum = {0, 0, (1024 * 8) >> 8, 0}, // 32GB
            .blocklen = {0b10 /* formated */, 0, (512) >> 8, 0}
        };

        capacities_stage_one_send = false;
        memcpy(&usb_bulk_buffer[0], &ret, sizeof(scsi_read_format_capacities_response_t));
        usb_transfer_schedule_block(
            &usb_endpoint_bulk_in,
            &usb_bulk_buffer[0],
            sizeof(scsi_inquiry_response_t),
            capacities_cb,
            msd_cbw_data);

        while (!capacities_stage_one_send);
    }


    msd_csw_t csw = {
        .signature = MSD_CSW_SIGNATURE,
        .tag = msd_cbw_data->tag,
        .data_residue = 0,
        .status = 0
    };

    memcpy(&usb_bulk_buffer[0x4000], &csw, sizeof(msd_csw_t));
    usb_transfer_schedule_block(
        &usb_endpoint_bulk_in,
        &usb_bulk_buffer[0x4000],
        sizeof(msd_csw_t),
        NULL,
        NULL);

}

typedef struct {
  uint32_t last_block_addr;
  uint32_t block_size;
} scsi_read_capacity10_response_t;

volatile bool capacity10_stage_one_send = false;
void capacity10_cb(void* user_data, unsigned int bytes_transferred)
{
    capacity10_stage_one_send = true;
}

void read_capacity10(msd_cbw_t *msd_cbw_data) {
    capacity10_stage_one_send = false;

    scsi_read_capacity10_response_t ret = {
        .last_block_addr = cpu_to_be32(8 * 1024 * 1024 - 1),
        .block_size = cpu_to_be32(512)
    };

    memcpy(&usb_bulk_buffer[0], &ret, sizeof(scsi_read_capacity10_response_t));
    usb_transfer_schedule_block(
        &usb_endpoint_bulk_in,
        &usb_bulk_buffer[0],
        sizeof(scsi_read_capacity10_response_t),
        capacity10_cb,
        msd_cbw_data);

    while (!capacity10_stage_one_send);

    msd_csw_t csw = {
        .signature = MSD_CSW_SIGNATURE,
        .tag = msd_cbw_data->tag,
        .data_residue = 0,
        .status = 0
    };

    memcpy(&usb_bulk_buffer[0x4000], &csw, sizeof(msd_csw_t));
    usb_transfer_schedule_block(
        &usb_endpoint_bulk_in,
        &usb_bulk_buffer[0x4000],
        sizeof(msd_csw_t),
        NULL,
        NULL);

}


typedef struct {
  uint8_t byte[18];
} scsi_sense_response_t;

volatile bool sense_stage_one_send = false;
void sense_cb(void* user_data, unsigned int bytes_transferred)
{
    sense_stage_one_send = true;
}


void request_sense(msd_cbw_t *msd_cbw_data) {
    sense_stage_one_send = false;

    scsi_sense_response_t ret = {
        .byte = { 0x70, 0, SCSI_SENSE_KEY_GOOD, 0,
                0, 0, 0, 8,
                0, 0 ,0 ,0,
                SCSI_ASENSE_NO_ADDITIONAL_INFORMATION, SCSI_ASENSEQ_NO_QUALIFIER, 0, 0,
                0, 0 }
    };

    memcpy(&usb_bulk_buffer[0], &ret, sizeof(scsi_sense_response_t));
    usb_transfer_schedule_block(
        &usb_endpoint_bulk_in,
        &usb_bulk_buffer[0],
        sizeof(scsi_sense_response_t),
        sense_cb,
        msd_cbw_data);

    while (!sense_stage_one_send);

    msd_csw_t csw = {
        .signature = MSD_CSW_SIGNATURE,
        .tag = msd_cbw_data->tag,
        .data_residue = 0,
        .status = 0
    };

    memcpy(&usb_bulk_buffer[0x4000], &csw, sizeof(msd_csw_t));
    usb_transfer_schedule_block(
        &usb_endpoint_bulk_in,
        &usb_bulk_buffer[0x4000],
        sizeof(msd_csw_t),
        NULL,
        NULL);
}

typedef struct {
  uint8_t   byte[4];
} scsi_mode_sense6_response_t;

volatile bool sense6_stage_one_send = false;
void sense6_cb(void* user_data, unsigned int bytes_transferred)
{
    sense6_stage_one_send = true;
}

void mode_sense6 (msd_cbw_t *msd_cbw_data) {
    sense6_stage_one_send = false;

    scsi_mode_sense6_response_t ret = {
        .byte = { 
            sizeof(scsi_mode_sense6_response_t) - 1,
            0,
            0x01 << 7, // 0 for not write protected
            0 }
    };

    memcpy(&usb_bulk_buffer[0], &ret, sizeof(scsi_mode_sense6_response_t));
    usb_transfer_schedule_block(
        &usb_endpoint_bulk_in,
        &usb_bulk_buffer[0],
        sizeof(scsi_mode_sense6_response_t),
        sense6_cb,
        msd_cbw_data);

    while (!sense6_stage_one_send);

    msd_csw_t csw = {
        .signature = MSD_CSW_SIGNATURE,
        .tag = msd_cbw_data->tag,
        .data_residue = 0,
        .status = 0
    };

    memcpy(&usb_bulk_buffer[0x4000], &csw, sizeof(msd_csw_t));
    usb_transfer_schedule_block(
        &usb_endpoint_bulk_in,
        &usb_bulk_buffer[0x4000],
        sizeof(msd_csw_t),
        NULL,
        NULL);
}

typedef struct {
  uint32_t first_lba;
  uint16_t blk_cnt;
} data_request_t;

static data_request_t decode_data_request(const uint8_t *cmd) {

  data_request_t req;
  uint32_t lba;
  uint16_t blk;

  memcpy(&lba, &cmd[2], sizeof(lba));
  memcpy(&blk, &cmd[7], sizeof(blk));

  req.first_lba = be32_to_cpu(lba);
  req.blk_cnt = be16_to_cpu(blk);

  return req;
}

volatile uint32_t read10_blocks_send = 0;
void read10_cb(void* user_data, unsigned int bytes_transferred)
{
    read10_blocks_send++;
}


void data_read10(msd_cbw_t *msd_cbw_data) {
    read10_blocks_send = 0;

    data_request_t req = decode_data_request(msd_cbw_data->cmd_data);

    for (size_t block_index = 0; block_index < req.blk_cnt; block_index++) {
        memset(&usb_bulk_buffer[0], 0, 512);

        usb_transfer_schedule_block(
            &usb_endpoint_bulk_in,
            &usb_bulk_buffer[0],
            512,
            read10_cb,
            msd_cbw_data);

        while (read10_blocks_send <= block_index);
    }

    msd_csw_t csw = {
        .signature = MSD_CSW_SIGNATURE,
        .tag = msd_cbw_data->tag,
        .data_residue = 0,
        .status = 0
    };

    memcpy(&usb_bulk_buffer[0x4000], &csw, sizeof(msd_csw_t));
    usb_transfer_schedule_block(
        &usb_endpoint_bulk_in,
        &usb_bulk_buffer[0x4000],
        sizeof(msd_csw_t),
        NULL,
        NULL);
}




void test_unit_ready(msd_cbw_t *msd_cbw_data) {
    msd_csw_t csw = {
        .signature = MSD_CSW_SIGNATURE,
        .tag = msd_cbw_data->tag,
        .data_residue = 0,
        .status = 0
    };

    memcpy(&usb_bulk_buffer[0x4000], &csw, sizeof(msd_csw_t));
    usb_transfer_schedule_block(
        &usb_endpoint_bulk_in,
        &usb_bulk_buffer[0x4000],
        sizeof(msd_csw_t),
        NULL,
        NULL);
}

void scsi_command(msd_cbw_t *msd_cbw_data) {

  switch (msd_cbw_data->cmd_data[0]) {
    case SCSI_CMD_INQUIRY:
        handle_inquiry(msd_cbw_data);


        break;

    case SCSI_CMD_REQUEST_SENSE:
        request_sense(msd_cbw_data);
        break;

    case SCSI_CMD_READ_CAPACITY_10:
        read_capacity10(msd_cbw_data);
        break;

    case SCSI_CMD_READ_10:
        data_read10(msd_cbw_data);
        break;

    /*
    case SCSI_CMD_WRITE_10:
        ret = data_read_write10(scsip, cmd);
        break;
*/
    case SCSI_CMD_TEST_UNIT_READY:
        test_unit_ready(msd_cbw_data);
        break;

    case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
        test_unit_ready(msd_cbw_data);
        // ret = cmd_ignored(scsip, cmd);
        break;

    case SCSI_CMD_MODE_SENSE_6:
        mode_sense6(msd_cbw_data);
        break;
    
    case SCSI_CMD_READ_FORMAT_CAPACITIES:
        read_format_capacities(msd_cbw_data);
        break;
    
    case SCSI_CMD_VERIFY_10:
        test_unit_ready(msd_cbw_data);
        break;
/*
    default:
        ret = cmd_unhandled(scsip, cmd);
        break;
        */
  }

//   if (ret == SCSI_SUCCESS)
//     set_sense_ok(scsip);


}