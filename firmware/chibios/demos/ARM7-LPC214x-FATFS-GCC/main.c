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
#include "test.h"
#include "chprintf.h"
#include "evtimer.h"
#include "buzzer.h"

#include "ff.h"

/*===========================================================================*/
/* Card insertion monitor.                                                   */
/*===========================================================================*/

#define POLLING_INTERVAL                10
#define POLLING_DELAY                   10

/**
 * @brief   Card monitor timer.
 */
static VirtualTimer tmr;

/**
 * @brief   Debounce counter.
 */
static unsigned cnt;

/**
 * @brief   Card event sources.
 */
static EventSource inserted_event, removed_event;

/**
 * @brief   Insertion monitor timer callback function.
 *
 * @param[in] p         pointer to the @p BaseBlockDevice object
 *
 * @notapi
 */
static void tmrfunc(void *p) {
  BaseBlockDevice *bbdp = p;

  /* The presence check is performed only while the driver is not in a
     transfer state because it is often performed by changing the mode of
     the pin connected to the CS/D3 contact of the card, this could disturb
     the transfer.*/
  blkstate_t state = blkGetDriverState(bbdp);
  chSysLockFromIsr();
  if ((state != BLK_READING) && (state != BLK_WRITING)) {
    /* Safe to perform the check.*/
    if (cnt > 0) {
      if (blkIsInserted(bbdp)) {
        if (--cnt == 0) {
          chEvtBroadcastI(&inserted_event);
        }
      }
      else
        cnt = POLLING_INTERVAL;
    }
    else {
      if (!blkIsInserted(bbdp)) {
        cnt = POLLING_INTERVAL;
        chEvtBroadcastI(&removed_event);
      }
    }
  }
  chVTSetI(&tmr, MS2ST(POLLING_DELAY), tmrfunc, bbdp);
  chSysUnlockFromIsr();
}

/**
 * @brief   Polling monitor start.
 *
 * @param[in] p         pointer to an object implementing @p BaseBlockDevice
 *
 * @notapi
 */
static void tmr_init(void *p) {

  chEvtInit(&inserted_event);
  chEvtInit(&removed_event);
  chSysLock();
  cnt = POLLING_INTERVAL;
  chVTSetI(&tmr, MS2ST(POLLING_DELAY), tmrfunc, p);
  chSysUnlock();
}

/*===========================================================================*/
/* FatFs related.                                                            */
/*===========================================================================*/

/**
 * @brief FS object.
 */
FATFS MMC_FS;

/**
 * MMC driver instance.
 */
MMCDriver MMCD1;

/* FS mounted and ready.*/
static bool_t fs_ready = FALSE;

/* Maximum speed SPI configuration (18MHz, CPHA=0, CPOL=0).*/
static SPIConfig hs_spicfg = {
  NULL,
  IOPORT1,
  PA_SSEL1,
  CR0_DSS8BIT | CR0_FRFSPI | CR0_CLOCKRATE(0),
  2
};

/* Low speed SPI configuration (281.250kHz, CPHA=0, CPOL=0).*/
static SPIConfig ls_spicfg = {
  NULL,
  IOPORT1,
  PA_SSEL1,
  CR0_DSS8BIT | CR0_FRFSPI | CR0_CLOCKRATE(0),
  254
};

/* MMC/SD over SPI driver configuration.*/
static MMCConfig mmccfg = {&SPID1, &ls_spicfg, &hs_spicfg};

/* Generic large buffer.*/
uint8_t fbuff[1024];

static FRESULT scan_files(BaseSequentialStream *chp, char *path) {
  FRESULT res;
  FILINFO fno;
  DIR dir;
  int i;
  char *fn;

#if _USE_LFN
  fno.lfname = 0;
  fno.lfsize = 0;
#endif
  res = f_opendir(&dir, path);
  if (res == FR_OK) {
    i = strlen(path);
    for (;;) {
      res = f_readdir(&dir, &fno);
      if (res != FR_OK || fno.fname[0] == 0)
        break;
      if (fno.fname[0] == '.')
        continue;
      fn = fno.fname;
      if (fno.fattrib & AM_DIR) {
        path[i++] = '/';
        strcpy(&path[i], fn);
        res = scan_files(chp, path);
        if (res != FR_OK)
          break;
        path[--i] = 0;
      }
      else {
        chprintf(chp, "%s/%s\r\n", path, fn);
      }
    }
  }
  return res;
}

/*===========================================================================*/
/* Main and generic code.                                                    */
/*===========================================================================*/

/*
 * Red LEDs blinker thread, times are in milliseconds.
 */
static WORKING_AREA(waThread1, 128);
static msg_t Thread1(void *arg) {

  (void)arg;
  chRegSetThreadName("blinker1");
  while (TRUE) {
    palClearPort(IOPORT1, PAL_PORT_BIT(PA_LED2));
    chThdSleepMilliseconds(200);
    palSetPort(IOPORT1, PAL_PORT_BIT(PA_LED1) | PAL_PORT_BIT(PA_LED2));
    chThdSleepMilliseconds(800);
    palClearPort(IOPORT1, PAL_PORT_BIT(PA_LED1));
    chThdSleepMilliseconds(200);
    palSetPort(IOPORT1, PAL_PORT_BIT(PA_LED1) | PAL_PORT_BIT(PA_LED2));
    chThdSleepMilliseconds(800);
  }
  return 0;
}

/*
 * Yellow LED blinker thread, times are in milliseconds.
 */
static WORKING_AREA(waThread2, 128);
static msg_t Thread2(void *arg) {

  (void)arg;
  chRegSetThreadName("blinker2");
  while (TRUE) {
    palClearPad(IOPORT1, PA_LEDUSB);
    chThdSleepMilliseconds(200);
    palSetPad(IOPORT1, PA_LEDUSB);
    chThdSleepMilliseconds(300);
  }
  return 0;
}

/*
 * Executed as event handler at 500mS intervals.
 */
static void TimerHandler(eventid_t id) {

  (void)id;
  if (!palReadPad(IOPORT1, PA_BUTTON1)) {
    if (fs_ready) {
      FRESULT err;
      uint32_t clusters;
      FATFS *fsp;

      err = f_getfree("/", &clusters, &fsp);
      if (err != FR_OK) {
        chprintf((BaseSequentialStream *)&SD1, "FS: f_getfree() failed\r\n");
        return;
      }
      chprintf((BaseSequentialStream *)&SD1,
               "FS: %lu free clusters, %lu sectors per cluster, %lu bytes free\r\n",
               clusters, (uint32_t)MMC_FS.csize,
               clusters * (uint32_t)MMC_FS.csize * (uint32_t)MMCSD_BLOCK_SIZE);
      fbuff[0] = 0;
      scan_files((BaseSequentialStream *)&SD1, (char *)fbuff);
    }
  }
  else if (!palReadPad(IOPORT1, PA_BUTTON2)) {
    static WORKING_AREA(waTestThread, 256);
    Thread *tp = chThdCreateStatic(waTestThread, sizeof(waTestThread),
                                   NORMALPRIO, TestThread, &SD1);
    chThdWait(tp);
    buzzPlay(500, MS2ST(100));
  }
}

/*
 * MMC card insertion event.
 */
static void InsertHandler(eventid_t id) {
  FRESULT err;

  (void)id;
  buzzPlayWait(1000, MS2ST(100));
  buzzPlayWait(2000, MS2ST(100));
  chprintf((BaseSequentialStream *)&SD1, "MMC: inserted\r\n");
  /*
   * On insertion MMC initialization and FS mount.
   */
  chprintf((BaseSequentialStream *)&SD1, "MMC: initialization ");
  if (mmcConnect(&MMCD1)) {
    chprintf((BaseSequentialStream *)&SD1, "failed\r\n");
    return;
  }
  chprintf((BaseSequentialStream *)&SD1, "ok\r\n");
  chprintf((BaseSequentialStream *)&SD1, "FS: mount ");
  err = f_mount(0, &MMC_FS);
  if (err != FR_OK) {
    chprintf((BaseSequentialStream *)&SD1, "failed\r\n");
    mmcDisconnect(&MMCD1);
    return;
  }
  fs_ready = TRUE;
  chprintf((BaseSequentialStream *)&SD1, "ok\r\n");
  buzzPlay(440, MS2ST(200));
}

/*
 * MMC card removal event.
 */
static void RemoveHandler(eventid_t id) {

  (void)id;
  chprintf((BaseSequentialStream *)&SD1, "MMC: removed\r\n");
  mmcDisconnect(&MMCD1);
  fs_ready = FALSE;
  buzzPlayWait(2000, MS2ST(100));
  buzzPlayWait(1000, MS2ST(100));
}

/*
 * Application entry point.
 */
int main(void) {
  static const evhandler_t evhndl[] = {
    TimerHandler,
    InsertHandler,
    RemoveHandler
  };
  static EvTimer evt;
  struct EventListener el0, el1, el2;

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * Activates the serial driver 1 using the driver default configuration.
   */
  sdStart(&SD1, NULL);

  /*
   * Buzzer driver initialization.
   */
  buzzInit();

  /*
   * Initializes the MMC driver to work with SPI2.
   */
  mmcObjectInit(&MMCD1);
  mmcStart(&MMCD1, &mmccfg);

  /*
   * Activates the card insertion monitor.
   */
  tmr_init(&MMCD1);

  /*
   * Creates the blinker threads.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);
  chThdCreateStatic(waThread2, sizeof(waThread2), NORMALPRIO, Thread2, NULL);

  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and listen for events.
   */
  evtInit(&evt, MS2ST(500));            /* Initializes an event timer object.   */
  evtStart(&evt);                       /* Starts the event timer.              */
  chEvtRegister(&evt.et_es, &el0, 0);   /* Registers on the timer event source. */
  chEvtRegister(&inserted_event, &el1, 1);
  chEvtRegister(&removed_event, &el2, 2);
  while (TRUE)
    chEvtDispatch(evhndl, chEvtWaitOne(ALL_EVENTS));
  return 0;
}
