/*
    SPC5 HAL - Copyright (C) 2013 STMicroelectronics

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

/**
 * @file    eTimer_v1/etimer.h
 * @brief   SPC5xx eTimer header file.
 *
 * @addtogroup ICU
 * @{
 */

#ifndef _ETIMER_H_
#define _ETIMER_H_

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   SPC5 FlexPWM registers block.
 * @note    Redefined from the SPC5 headers because the non uniform
 *          declaration of the SubModules registers among the various
 *          sub-families.
 */
struct spc5_etimer_submodule {

  union {
    vuint16_t R;
    struct {
      vuint16_t COMP1 :16;
    } B;
  } COMP1; /* Compare Register 1 */

  union {
    vuint16_t R;
    struct {
      vuint16_t COMP2 :16;
    } B;
  } COMP2; /* Compare Register 2 */

  union {
    vuint16_t R;
    struct {
      vuint16_t CAPT1 :16;
    } B;
  } CAPT1; /* Capture Register 1 */

  union {
    vuint16_t R;
    struct {
      vuint16_t CAPT2 :16;
    } B;
  } CAPT2; /* Capture Register 2 */

  union {
    vuint16_t R;
    struct {
      vuint16_t LOAD :16;
    } B;
  } LOAD; /* Load Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t HOLD :16;
    } B;
  } HOLD; /* Hold Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t CNTR :16;
    } B;
  } CNTR; /* Counter Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t CNTMODE :3;
      vuint16_t PRISRC :5;
      vuint16_t ONCE :1;
      vuint16_t LENGTH :1;
      vuint16_t DIR :1;
      vuint16_t SECSRC :5;
    } B;
  } CTRL; /* Control Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t OEN :1;
      vuint16_t RDNT :1;
      vuint16_t INPUT :1;
      vuint16_t VAL :1;
      vuint16_t FORCE :1;
      vuint16_t COFRC :1;
      vuint16_t COINIT :2;
      vuint16_t SIPS :1;
      vuint16_t PIPS :1;
      vuint16_t OPS :1;
      vuint16_t MSTR :1;
      vuint16_t OUTMODE :4;
    } B;
  } CTRL2; /* Control Register 2 */

  union {
    vuint16_t R;
    struct {
      vuint16_t STPEN :1;
      vuint16_t ROC :2;
      vuint16_t FMODE :1;
      vuint16_t FDIS :4;
      vuint16_t C2FCNT :3;
      vuint16_t C1FCNT :3;
      vuint16_t DBGEN :2;
    } B;
  } CTRL3; /* Control Register 3 */

  union {
    vuint16_t R;
    struct {
      vuint16_t :6;
      vuint16_t WDF :1;
      vuint16_t RCF :1;
      vuint16_t ICF2 :1;
      vuint16_t ICF1 :1;
      vuint16_t IEHF :1;
      vuint16_t IELF :1;
      vuint16_t TOF :1;
      vuint16_t TCF2 :1;
      vuint16_t TCF1 :1;
      vuint16_t TCF :1;
    } B;
  } STS; /* Status Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t ICF2DE :1;
      vuint16_t ICF1DE :1;
      vuint16_t CMPLD2DE :1;
      vuint16_t CMPLD1DE :1;
      vuint16_t :2;
      vuint16_t WDFIE :1;
      vuint16_t RCFIE :1;
      vuint16_t ICF2IE :1;
      vuint16_t ICF1IE :1;
      vuint16_t IEHFIE :1;
      vuint16_t IELFIE :1;
      vuint16_t TOFIE :1;
      vuint16_t TCF2IE :1;
      vuint16_t TCF1IE :1;
      vuint16_t TCFIE :1;
    } B;
  } INTDMA; /* Interrupt and DMA Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t CMPLD1 :16;
    } B;
  } CMPLD1; /* Compare Load Register 1 */

  union {
    vuint16_t R;
    struct {
      vuint16_t CMPLD2 :16;
    } B;
  } CMPLD2; /* Compare Load Register 2 */

  union {
    vuint16_t R;
    struct {
      vuint16_t CLC2 :3;
      vuint16_t CLC1 :3;
      vuint16_t CMPMODE :2;
      vuint16_t CPT2MODE :2;
      vuint16_t CPT1MODE :2;
      vuint16_t CFWM :2;
      vuint16_t ONESHOT :1;
      vuint16_t ARM :1;
    } B;
  } CCCTRL; /* Compare and Capture Control Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t :5;
      vuint16_t FILTCNT :3;
      vuint16_t FILTPER :8;
    } B;
  } FILT; /* Input Filter Register */

};
/* end of ETIMER_CHANNEL_tag */

struct spc5_etimer {

  struct spc5_etimer_submodule CHANNEL[8];

  union {
    vuint16_t R;
    struct {
      vuint16_t WDTOL :16;
    } B;
  } WDTOL; /* Watchdog Time-out Low Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t WDTOH :16;
    } B;
  } WDTOH; /* Watchdog Time-out High Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t :3;
      vuint16_t FTEST :1;
      vuint16_t FIE :4;
      vuint16_t :4;
      vuint16_t FLVL :4;
    } B;
  } FCTRL; /* Fault Control Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t :4;
      vuint16_t FFPIN :4;
      vuint16_t :4;
      vuint16_t FFLAG :4;
    } B;
  } FSTS; /* Fault Status Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t :5;
      vuint16_t FFILTCNT :3;
      vuint16_t FFILTPER :8;
    } B;
  } FFILT; /* Fault Filter Register */

  int16_t ETIMER_reserved1;

  union {
    vuint16_t R;
    struct {
      vuint16_t :8;
      vuint16_t ENBL :8;
    } B;
  } ENBL; /* Channel Enable Register */

  int16_t ETIMER_reserved2;

  union {
    vuint16_t R;
    struct {
      vuint16_t :11;
      vuint16_t DREQ :5;
    } B;
  } DREQ[4]; /* DMA Request 0->3 Select Register */

};
/* end of ETIMER_tag */

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @name    FlexPWM units references
 * @{
 */
#if SPC5_HAS_ETIMER0
#define SPC5_ETIMER_0      (*(volatile struct spc5_etimer *)0xFFE18000UL)
#endif

#if SPC5_HAS_ETIMER1
#define SPC5_ETIMER_1      (*(volatile struct spc5_etimer *)0xFFE1C000UL)
#endif

#if SPC5_HAS_ETIMER2
#define SPC5_ETIMER_2      (*(volatile struct spc5_etimer *)0xFFE20000UL)
#endif
/** @} */

#endif /* _FLEXPWM_H_ */

/** @} */
