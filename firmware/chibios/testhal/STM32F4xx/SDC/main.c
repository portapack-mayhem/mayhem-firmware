/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <string.h>
#include "ch.h"
#include "hal.h"

#include "shell.h"
#include "chprintf.h"

#include "ff.h"

#define SDC_DATA_DESTRUCTIVE_TEST   FALSE

#define SDC_BURST_SIZE      8 /* how many sectors reads at once */
static uint8_t outbuf[MMCSD_BLOCK_SIZE * SDC_BURST_SIZE + 1];
static uint8_t  inbuf[MMCSD_BLOCK_SIZE * SDC_BURST_SIZE + 1];

/* FS object.*/
static FATFS SDC_FS;

/* FS mounted and ready.*/
static bool_t fs_ready = FALSE;

/**
 * @brief   Parody of UNIX badblocks program.
 *
 * @param[in] start       first block to check
 * @param[in] end         last block to check
 * @param[in] blockatonce number of blocks to check at once
 * @param[in] pattern     check pattern
 *
 * @return              The operation status.
 * @retval SDC_SUCCESS  operation succeeded, the requested blocks have been
 *                      read.
 * @retval SDC_FAILED   operation failed, the state of the buffer is uncertain.
 */
bool_t badblocks(uint32_t start, uint32_t end, uint32_t blockatonce, uint8_t pattern){
  uint32_t position = 0;
  uint32_t i = 0;

  chDbgCheck(blockatonce <= SDC_BURST_SIZE, "badblocks");

  /* fill control buffer */
  for (i=0; i < MMCSD_BLOCK_SIZE * blockatonce; i++)
    outbuf[i] = pattern;

  /* fill SD card with pattern. */
  position = start;
  while (position < end){
    if (sdcWrite(&SDCD1, position, outbuf, blockatonce))
      goto ERROR;
    position += blockatonce;
  }

  /* read and compare. */
  position = start;
  while (position < end){
    if (sdcRead(&SDCD1, position, inbuf, blockatonce))
      goto ERROR;
    if (memcmp(inbuf, outbuf, blockatonce * MMCSD_BLOCK_SIZE) != 0)
      goto ERROR;
    position += blockatonce;
  }
  return FALSE;

ERROR:
  return TRUE;
}

/**
 *
 */
void fillbuffer(uint8_t pattern, uint8_t *b){
  uint32_t i = 0;
  for (i=0; i < MMCSD_BLOCK_SIZE * SDC_BURST_SIZE; i++)
    b[i] = pattern;
}

/**
 *
 */
void fillbuffers(uint8_t pattern){
  fillbuffer(pattern, inbuf);
  fillbuffer(pattern, outbuf);
}

/**
 *
 */
void cmd_sdiotest(BaseSequentialStream *chp, int argc, char *argv[]){
  (void)argc;
  (void)argv;
  uint32_t i = 0;

  chprintf(chp, "Trying to connect SDIO... ");
  chThdSleepMilliseconds(100);

  if (!sdcConnect(&SDCD1)) {

    chprintf(chp, "OK\r\n");
    chprintf(chp, "*** Card CSD content is: ");
    chprintf(chp, "%X %X %X %X \r\n", (&SDCD1)->csd[3], (&SDCD1)->csd[2],
                                      (&SDCD1)->csd[1], (&SDCD1)->csd[0]);

    chprintf(chp, "Single aligned read...");
    chThdSleepMilliseconds(100);
    if (sdcRead(&SDCD1, 0, inbuf, 1))
      chSysHalt();
    chprintf(chp, " OK\r\n");
    chThdSleepMilliseconds(100);


    chprintf(chp, "Single unaligned read...");
    chThdSleepMilliseconds(100);
    if (sdcRead(&SDCD1, 0, inbuf + 1, 1))
      chSysHalt();
    if (sdcRead(&SDCD1, 0, inbuf + 2, 1))
      chSysHalt();
    if (sdcRead(&SDCD1, 0, inbuf + 3, 1))
      chSysHalt();
    chprintf(chp, " OK\r\n");
    chThdSleepMilliseconds(100);


    chprintf(chp, "Multiple aligned reads...");
    chThdSleepMilliseconds(100);
    fillbuffers(0x55);
    /* fill reference buffer from SD card */
    if (sdcRead(&SDCD1, 0, inbuf, SDC_BURST_SIZE))
      chSysHalt();
    for (i=0; i<1000; i++){
      if (sdcRead(&SDCD1, 0, outbuf, SDC_BURST_SIZE))
        chSysHalt();
      if (memcmp(inbuf, outbuf, SDC_BURST_SIZE * MMCSD_BLOCK_SIZE) != 0)
        chSysHalt();
    }
    chprintf(chp, " OK\r\n");
    chThdSleepMilliseconds(100);


    chprintf(chp, "Multiple unaligned reads...");
    chThdSleepMilliseconds(100);
    fillbuffers(0x55);
    /* fill reference buffer from SD card */
    if (sdcRead(&SDCD1, 0, inbuf + 1, SDC_BURST_SIZE))
      chSysHalt();
    for (i=0; i<1000; i++){
      if (sdcRead(&SDCD1, 0, outbuf + 1, SDC_BURST_SIZE))
        chSysHalt();
      if (memcmp(inbuf, outbuf, SDC_BURST_SIZE * MMCSD_BLOCK_SIZE) != 0)
        chSysHalt();
    }
    chprintf(chp, " OK\r\n");
    chThdSleepMilliseconds(100);

#if SDC_DATA_DESTRUCTIVE_TEST

    chprintf(chp, "Single aligned write...");
    chThdSleepMilliseconds(100);
    fillbuffer(0xAA, inbuf);
    if (sdcWrite(&SDCD1, 0, inbuf, 1))
      chSysHalt();
    fillbuffer(0, outbuf);
    if (sdcRead(&SDCD1, 0, outbuf, 1))
      chSysHalt();
    if (memcmp(inbuf, outbuf, MMCSD_BLOCK_SIZE) != 0)
      chSysHalt();
    chprintf(chp, " OK\r\n");

    chprintf(chp, "Single unaligned write...");
    chThdSleepMilliseconds(100);
    fillbuffer(0xFF, inbuf);
    if (sdcWrite(&SDCD1, 0, inbuf+1, 1))
      chSysHalt();
    fillbuffer(0, outbuf);
    if (sdcRead(&SDCD1, 0, outbuf+1, 1))
      chSysHalt();
    if (memcmp(inbuf+1, outbuf+1, MMCSD_BLOCK_SIZE) != 0)
      chSysHalt();
    chprintf(chp, " OK\r\n");

    chprintf(chp, "Running badblocks at 0x10000 offset...");
    chThdSleepMilliseconds(100);
    if(badblocks(0x10000, 0x11000, SDC_BURST_SIZE, 0xAA))
      chSysHalt();
    chprintf(chp, " OK\r\n");
#endif /* !SDC_DATA_DESTRUCTIVE_TEST */


    /**
     * Now perform some FS tests.
     */

    FRESULT err;
    uint32_t clusters;
    FATFS *fsp;
    FIL FileObject;
    uint32_t bytes_written;
    uint32_t bytes_read;
    FILINFO filinfo;
    uint8_t teststring[] = {"This is test file\r\n"};

    chprintf(chp, "Register working area for filesystem... ");
    chThdSleepMilliseconds(100);
    err = f_mount(0, &SDC_FS);
    if (err != FR_OK){
      chSysHalt();
    }
    else{
      fs_ready = TRUE;
      chprintf(chp, "OK\r\n");
    }


#if SDC_DATA_DESTRUCTIVE_TEST
    chprintf(chp, "Formatting... ");
    chThdSleepMilliseconds(100);
    err = f_mkfs (0,0,0);
    if (err != FR_OK){
      chSysHalt();
    }
    else{
      chprintf(chp, "OK\r\n");
    }
#endif /* SDC_DATA_DESTRUCTIVE_TEST */


    chprintf(chp, "Mount filesystem... ");
    chThdSleepMilliseconds(100);
    err = f_getfree("/", &clusters, &fsp);
    if (err != FR_OK) {
      chSysHalt();
    }
    chprintf(chp, "OK\r\n");
    chprintf(chp,
             "FS: %lu free clusters, %lu sectors per cluster, %lu bytes free\r\n",
             clusters, (uint32_t)SDC_FS.csize,
             clusters * (uint32_t)SDC_FS.csize * (uint32_t)MMCSD_BLOCK_SIZE);


    chprintf(chp, "Create file \"chtest.txt\"... ");
    chThdSleepMilliseconds(100);
    err = f_open(&FileObject, "0:chtest.txt", FA_WRITE | FA_OPEN_ALWAYS);
    if (err != FR_OK) {
      chSysHalt();
    }
    chprintf(chp, "OK\r\n");
    chprintf(chp, "Write some data in it... ");
    chThdSleepMilliseconds(100);
    err = f_write(&FileObject, teststring, sizeof(teststring), (void *)&bytes_written);
    if (err != FR_OK) {
      chSysHalt();
    }
    else
      chprintf(chp, "OK\r\n");

    chprintf(chp, "Close file \"chtest.txt\"... ");
    err = f_close(&FileObject);
    if (err != FR_OK) {
      chSysHalt();
    }
    else
      chprintf(chp, "OK\r\n");

    chprintf(chp, "Check file size \"chtest.txt\"... ");
    err = f_stat("0:chtest.txt", &filinfo);
    chThdSleepMilliseconds(100);
    if (err != FR_OK) {
      chSysHalt();
    }
    else{
      if (filinfo.fsize == sizeof(teststring))
        chprintf(chp, "OK\r\n");
      else
        chSysHalt();
    }

    chprintf(chp, "Check file content \"chtest.txt\"... ");
    err = f_open(&FileObject, "0:chtest.txt", FA_READ | FA_OPEN_EXISTING);
    chThdSleepMilliseconds(100);
    if (err != FR_OK) {
      chSysHalt();
    }
    uint8_t buf[sizeof(teststring)];
    err = f_read(&FileObject, buf, sizeof(teststring), (void *)&bytes_read);
    if (err != FR_OK) {
      chSysHalt();
    }
    else{
      if (memcmp(teststring, buf, sizeof(teststring)) != 0){
        chSysHalt();
      }
      else{
        chprintf(chp, "OK\r\n");
      }
    }

    chprintf(chp, "Umount filesystem... ");
    f_mount(0, NULL);
    chprintf(chp, "OK\r\n");

    chprintf(chp, "Disconnecting from SDIO...");
    chThdSleepMilliseconds(100);
    if (sdcDisconnect(&SDCD1))
      chSysHalt();
    chprintf(chp, " OK\r\n");
    chprintf(chp, "------------------------------------------------------\r\n");
    chprintf(chp, "All tests passed successfully.\r\n");
    chThdSleepMilliseconds(100);
  }
  else{
    chSysHalt();
  }
}


/*
 * SDIO configuration.
 */
static const SDCConfig sdccfg = {
  0
};

/**
 *
 */
static SerialConfig ser_cfg = {
    115200,
    0,
    0,
    0,
};
static const ShellCommand commands[] = {
  {"sdiotest", cmd_sdiotest},
  {NULL, NULL}
};
static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream *)&SD2,
  commands
};

/*
 * Application entry point.
 */
int main(void) {
  halInit();
  chSysInit();

  /* start debugging serial link */
  sdStart(&SD2, &ser_cfg);
  shellInit();
  static WORKING_AREA(waShell, 2048);
  shellCreateStatic(&shell_cfg1, waShell, sizeof(waShell), NORMALPRIO);

  /*
   * Initializes the SDIO drivers.
   */
  sdcStart(&SDCD1, &sdccfg);

  /*
   * Normal main() thread activity.
   * Blinking signaling about successful passing.
   */
  while (TRUE) {
    palTogglePad(GPIOB, GPIOB_LED_R);
    chThdSleepMilliseconds(100);
  }
}
