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
 * @file    FlexPWM_v1/spc5_flexpwm.h
 * @brief   SPC5xx FlexPWM header file.
 *
 * @addtogroup PWM
 * @{
 */

#ifndef _SPC5_FLEXPWM_H_
#define _SPC5_FLEXPWM_H_

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
struct spc5_flexpwm_submodule {

  union {
    vuint16_t R;
  } CNT; /* Counter Register */

  union {
    vuint16_t R;
  } INIT; /* Initial Count Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t DBGEN :1;
      vuint16_t WAITEN :1;
      vuint16_t INDEP :1;
      vuint16_t PWMA_INIT :1;
      vuint16_t PWMB_INIT :1;
      vuint16_t PWMX_INIT :1;
      vuint16_t INIT_SEL :2;
      vuint16_t FRCEN :1;
      vuint16_t FORCE :1;
      vuint16_t FORCE_SEL :3;
      vuint16_t RELOAD_SEL :1;
      vuint16_t CLK_SEL :2;
    } B;
  } CTRL2; /* Control 2 Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t LDFQ :4;
      vuint16_t HALF :1;
      vuint16_t FULL :1;
      vuint16_t DT :2;
      vuint16_t :1;
      vuint16_t PRSC :3;
      vuint16_t :3;
      vuint16_t DBLEN :1;
    } B;
  } CTRL; /* Control Register */

  union {
    vuint16_t R;
  } VAL[6]; /* Value Register 0->5 */

  union {
    vuint16_t R;
    struct {
      vuint16_t FRACAEN :1;
      vuint16_t :10;
      vuint16_t FRACADLY :5;
    } B;
  } FRACA; /* Fractional Delay Register A */

  union {
    vuint16_t R;
    struct {
      vuint16_t FRACBEN :1;
      vuint16_t :10;
      vuint16_t FRACBDLY :5;
    } B;
  } FRACB; /* Fractional Delay Register B */

  union {
    vuint16_t R;
    struct {
      vuint16_t PWMA_IN :1;
      vuint16_t PWMB_IN :1;
      vuint16_t PWMX_IN :1;
      vuint16_t :2;
      vuint16_t POLA :1;
      vuint16_t POLB :1;
      vuint16_t POLX :1;
      vuint16_t :2;
      vuint16_t PWMAFS :2;
      vuint16_t PWMBFS :2;
      vuint16_t PWMXFS :2;
    } B;
  } OCTRL; /* Output Control Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t :1;
      vuint16_t RUF :1;
      vuint16_t REF :1;
      vuint16_t RF :1;
      vuint16_t CFA1 :1;
      vuint16_t CFA0 :1;
      vuint16_t CFB1 :1;
      vuint16_t CFB0 :1;
      vuint16_t CFX1 :1;
      vuint16_t CFX0 :1;
      vuint16_t CMPF :6;
    } B;
  } STS; /* Status Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t :2;
      vuint16_t REIE :1;
      vuint16_t RIE :1;
      vuint16_t :4;
      vuint16_t CX1IE :1;
      vuint16_t CX0IE :1;
      vuint16_t CMPIE :6;
    } B;
  } INTEN; /* Interrupt Enable Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t :6;
      vuint16_t VALDE :1;
      vuint16_t FAND :1;
      vuint16_t CAPTDE :2;
      vuint16_t CA1DE :1;
      vuint16_t CA0DE :1;
      vuint16_t CB1DE :1;
      vuint16_t CB0DE :1;
      vuint16_t CX1DE :1;
      vuint16_t CX0DE :1;
    } B;
  } DMAEN; /* DMA Enable Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t :10;
      vuint16_t OUT_TRIG_EN :6;
    } B;
  } TCTRL; /* Output Trigger Control Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t :4;
      vuint16_t DISX :4;
      vuint16_t DISB :4;
      vuint16_t DISA :4;
    } B;
  } DISMAP; /* Fault Disable Mapping Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t :5;
      vuint16_t DTCNT0 :11;
    } B;
  } DTCNT0; /* Deadtime Count Register 0 */

  union {
    vuint16_t R;
    struct {
      vuint16_t :5;
      vuint16_t DTCNT1 :11;
    } B;
  } DTCNT1; /* Deadtime Count Register 1 */

  union {
    vuint16_t R;
    struct {
      vuint16_t CA1CNT :3;
      vuint16_t CA0CNT :3;
      vuint16_t CFAWM :2;
      vuint16_t EDGCNTAEN :1;
      vuint16_t INPSELA :1;
      vuint16_t EDGA1 :2;
      vuint16_t EDGA0 :2;
      vuint16_t ONESHOTA :1;
      vuint16_t ARMA :1;
    } B;
  } CAPTCTRLA; /* Capture Control Register A */

  union {
    vuint16_t R;
    struct {
      vuint16_t EDGCNTA :8;
      vuint16_t EDGCMPA :8;
    } B;
  } CAPTCOMPA; /* Capture Compare Register A */

  union {
    vuint16_t R;
    struct {
      vuint16_t CB1CNT :3;
      vuint16_t CB0CNT :3;
      vuint16_t CFBWM :2;
      vuint16_t EDGCNTBEN :1;
      vuint16_t INPSELB :1;
      vuint16_t EDGB1 :2;
      vuint16_t EDGB0 :2;
      vuint16_t ONESHOTB :1;
      vuint16_t ARMB :1;
    } B;
  } CAPTCTRLB; /* Capture Control Register B */

  union {
    vuint16_t R;
    struct {
      vuint16_t EDGCNTB :8;
      vuint16_t EDGCMPB :8;
    } B;
  } CAPTCOMPB; /* Capture Compare Register B */

  union {
    vuint16_t R;
    struct {
      vuint16_t CX1CNT :3;
      vuint16_t CX0CNT :3;
      vuint16_t CFXWM :2;
      vuint16_t EDGCNTX_EN :1;
      vuint16_t INP_SELX :1;
      vuint16_t EDGX1 :2;
      vuint16_t EDGX0 :2;
      vuint16_t ONESHOTX :1;
      vuint16_t ARMX :1;
    } B;
  } CAPTCTRLX; /* Capture Control Register B */

  union {
    vuint16_t R;
    struct {
      vuint16_t EDGCNTX :8;
      vuint16_t EDGCMPX :8;
    } B;
  } CAPTCOMPX; /* Capture Compare Register X */

  union {
    vuint16_t R;
    struct {
      vuint16_t CAPTVAL0 :16;
    } B;
  } CVAL0; /* Capture Value 0 Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t :12;
      vuint16_t CVAL0CYC :4;
    } B;
  } CVAL0C; /* Capture Value 0 Cycle Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t CAPTVAL1 :16;
    } B;
  } CVAL1; /* Capture Value 1 Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t :12;
      vuint16_t CVAL1CYC :4;
    } B;
  } CVAL1C; /* Capture Value 1 Cycle Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t CAPTVAL2 :16;
    } B;
  } CVAL2; /* Capture Value 2 Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t :12;
      vuint16_t CVAL2CYC :4;
    } B;
  } CVAL2C; /* Capture Value 2 Cycle Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t CAPTVAL3 :16;
    } B;
  } CVAL3; /* Capture Value 3 Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t :12;
      vuint16_t CVAL3CYC :4;
    } B;
  } CVAL3C; /* Capture Value 3 Cycle Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t CAPTVAL4 :16;
    } B;
  } CVAL4; /* Capture Value 4 Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t :12;
      vuint16_t CVAL4CYC :4;
    } B;
  } CVAL4C; /* Capture Value 4 Cycle Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t CAPTVAL5 :16;
    } B;
  } CVAL5; /* Capture Value 5 Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t :12;
      vuint16_t CVAL5CYC :4;
    } B;
  } CVAL5C; /* Capture Value 5 Cycle Register */

  uint32_t FLEXPWM_SUB_reserved0; /* (0x04A - 0x050)/4 = 0x01 */

};
/* end of FLEXPWM_SUB_tag */

struct spc5_flexpwm {

  struct spc5_flexpwm_submodule SUB[4];

  union {
    vuint16_t R;
    struct {
      vuint16_t :4;
      vuint16_t PWMA_EN :4;
      vuint16_t PWMB_EN :4;
      vuint16_t PWMX_EN :4;
    } B;
  } OUTEN; /* Output Enable Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t :4;
      vuint16_t MASKA :4;
      vuint16_t MASKB :4;
      vuint16_t MASKX :4;
    } B;
  } MASK; /* Output Mask Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t :8;
      vuint16_t OUTA_3 :1;
      vuint16_t OUTB_3 :1;
      vuint16_t OUTA_2 :1;
      vuint16_t OUTB_2 :1;
      vuint16_t OUTA_1 :1;
      vuint16_t OUTB_1 :1;
      vuint16_t OUTA_0 :1;
      vuint16_t OUTB_0 :1;
    } B;
  } SWCOUT; /* Software Controlled Output Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t SELA_3 :2;
      vuint16_t SELB_3 :2;
      vuint16_t SELA_2 :2;
      vuint16_t SELB_2 :2;
      vuint16_t SELA_1 :2;
      vuint16_t SELB_1 :2;
      vuint16_t SELA_0 :2;
      vuint16_t SELB_0 :2;
    } B;
  } DTSRCSEL; /* Deadtime Source Select Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t IPOL :4;
      vuint16_t RUN :4;
      vuint16_t CLDOK :4;
      vuint16_t LDOK :4;
    } B;
  } MCTRL; /* Master Control Register */

  int16_t FLEXPWM_reserved1;

  union {
    vuint16_t R;
    struct {
      vuint16_t FLVL :4;
      vuint16_t FAUTO :4;
      vuint16_t FSAFE :4;
      vuint16_t FIE :4;
    } B;
  } FCTRL; /* Fault Control Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t :3;
      vuint16_t FTEST :1;
      vuint16_t FFPIN :4;
      vuint16_t :4;
      vuint16_t FFLAG :4;
    } B;
  } FSTS; /* Fault Status Register */

  union {
    vuint16_t R;
    struct {
      vuint16_t :5;
      vuint16_t FILT_CNT :3;
      vuint16_t FILT_PER :8;
    } B;
  } FFILT; /* Fault FilterRegister */

};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @name    FlexPWM units references
 * @{
 */
#if SPC5_HAS_FLEXPWM0 || defined(__DOXYGEN__)
#define SPC5_FLEXPWM_0      (*(volatile struct spc5_flexpwm *)0xFFE24000UL)
#endif

#if SPC5_HAS_FLEXPWM1 || defined(__DOXYGEN__)
#define SPC5_FLEXPWM_1      (*(volatile struct spc5_flexpwm *)0xFFE28000UL)
#endif
/** @} */

#endif /* _SPC5_FLEXPWM_H_ */

/** @} */
