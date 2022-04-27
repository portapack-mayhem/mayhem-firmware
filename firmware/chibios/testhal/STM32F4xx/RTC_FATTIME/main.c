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
#include <time.h>

#include "ch.h"
#include "hal.h"

#include "shell.h"
#include "chprintf.h"
#include "chrtclib.h"

#include "ff.h"

/* FS object.*/
static FATFS SDC_FS;

/* FS mounted and ready.*/
static bool_t fs_ready = FALSE;

/**
 *
 */
bool_t sdc_lld_is_write_protected(SDCDriver *sdcp) {
  (void)sdcp;
  return FALSE;
}

/**
 *
 */
bool_t sdc_lld_is_card_inserted(SDCDriver *sdcp) {
  (void)sdcp;
  return !palReadPad(GPIOE, GPIOE_SDIO_DETECT);
}

/**
 *
 */
void cmd_sdiotest(BaseSequentialStream *chp, int argc, char *argv[]){
  (void)argc;
  (void)argv;
  FRESULT err;
  uint32_t clusters;
  FATFS *fsp;
  FIL FileObject;
  //FILINFO FileInfo;
  size_t bytes_written;
  struct tm timp;

#if !HAL_USE_RTC
  chprintf(chp, "ERROR! Chibios compiled without RTC support.");
  chprintf(chp, "Enable HAL_USE_RCT in you halconf.h");
  chThdSleepMilliseconds(100);
  return;
#endif

  chprintf(chp, "Trying to connect SDIO... ");
  chThdSleepMilliseconds(100);

  if (!sdcConnect(&SDCD1)) {
    chprintf(chp, "OK\r\n");
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

    chprintf(chp, "Mounting filesystem... ");
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

    rtcGetTimeTm(&RTCD1, &timp);
    chprintf(chp, "Current RTC time is: ");
    chprintf(chp, "%u-%u-%u %u:%u:%u\r\n",
      timp.tm_year+1900, timp.tm_mon+1, timp.tm_mday, timp.tm_hour, timp.tm_min,
      timp.tm_sec);

    chprintf(chp, "Creating empty file 'tmstmp.tst'... ");
    chThdSleepMilliseconds(100);
    err = f_open(&FileObject, "0:tmstmp.tst", FA_WRITE | FA_OPEN_ALWAYS);
    if (err != FR_OK) {
      chSysHalt();
    }
    chprintf(chp, "OK\r\n");

    chprintf(chp, "Write some data in it... ");
    chThdSleepMilliseconds(100);
    err = f_write(&FileObject, "tst", sizeof("tst"), (void *)&bytes_written);
    if (err != FR_OK) {
      chSysHalt();
    }
    else
      chprintf(chp, "OK\r\n");

    chprintf(chp, "Closing file 'tmstmp.tst'... ");
    chThdSleepMilliseconds(100);
    err = f_close(&FileObject);
    if (err != FR_OK) {
      chSysHalt();
    }
    else
      chprintf(chp, "OK\r\n");

//    chprintf(chp, "Obtaining file info ... ");
//    chThdSleepMilliseconds(100);
//    err = f_stat("0:tmstmp.tst", &FileInfo);
//    if (err != FR_OK) {
//      chSysHalt();
//    }
//    else{
//      chprintf(chp, "OK\r\n");
//      chprintf(chp, "    Timestamp: %u-%u-%u %u:%u:%u\r\n",
//                         ((FileInfo.fdate >> 9) & 127) + 1980,
//                         (FileInfo.fdate >> 5) & 15,
//                         FileInfo.fdate & 31,
//                         (FileInfo.ftime >> 11) & 31,
//                         (FileInfo.ftime >> 5) & 63,
//                         (FileInfo.ftime & 31) * 2);
//    }

    chprintf(chp, "Umounting filesystem... ");
    f_mount(0, NULL);
    chprintf(chp, "OK\r\n");

    chprintf(chp, "Disconnecting from SDIO...");
    chThdSleepMilliseconds(100);
    if (sdcDisconnect(&SDCD1))
      chSysHalt();
    chprintf(chp, " OK\r\n");
    chprintf(chp, "------------------------------------------------------\r\n");
    chprintf(chp, "Now you can remove memory card and check timestamp on PC.\r\n");
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
  static WORKING_AREA(waShell, 4096);
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
