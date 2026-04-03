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

/**
 * @file    STM32/GPIOv1/pal_lld.c
 * @brief   STM32F1xx GPIO low level driver code.
 *
 * @addtogroup PAL
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_PAL || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#if STM32_HAS_GPIOG
#define APB2_EN_MASK  (RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN |            \
                       RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPDEN |            \
                       RCC_APB2ENR_IOPEEN | RCC_APB2ENR_IOPFEN |            \
                       RCC_APB2ENR_IOPGEN | RCC_APB2ENR_AFIOEN)
#elif STM32_HAS_GPIOE
#define APB2_EN_MASK  (RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN |            \
                       RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPDEN |            \
                       RCC_APB2ENR_IOPEEN | RCC_APB2ENR_AFIOEN)
#else
#define APB2_EN_MASK  (RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN |            \
                       RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPDEN |            \
                       RCC_APB2ENR_AFIOEN)
#endif

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   STM32 I/O ports configuration.
 * @details Ports A-D(E, F, G) clocks enabled, AFIO clock enabled.
 *
 * @param[in] config    the STM32 ports configuration
 *
 * @notapi
 */
void _pal_lld_init(const PALConfig *config) {

  /*
   * Enables the GPIO related clocks.
   */
  rccEnableAPB2(APB2_EN_MASK, FALSE);

  /*
   * Initial GPIO setup.
   */
  GPIOA->ODR = config->PAData.odr;
  GPIOA->CRH = config->PAData.crh;
  GPIOA->CRL = config->PAData.crl;
  GPIOB->ODR = config->PBData.odr;
  GPIOB->CRH = config->PBData.crh;
  GPIOB->CRL = config->PBData.crl;
  GPIOC->ODR = config->PCData.odr;
  GPIOC->CRH = config->PCData.crh;
  GPIOC->CRL = config->PCData.crl;
  GPIOD->ODR = config->PDData.odr;
  GPIOD->CRH = config->PDData.crh;
  GPIOD->CRL = config->PDData.crl;
#if STM32_HAS_GPIOE || defined(__DOXYGEN__)
  GPIOE->ODR = config->PEData.odr;
  GPIOE->CRH = config->PEData.crh;
  GPIOE->CRL = config->PEData.crl;
#if STM32_HAS_GPIOF || defined(__DOXYGEN__)
  GPIOF->ODR = config->PFData.odr;
  GPIOF->CRH = config->PFData.crh;
  GPIOF->CRL = config->PFData.crl;
#if STM32_HAS_GPIOG || defined(__DOXYGEN__)
  GPIOG->ODR = config->PGData.odr;
  GPIOG->CRH = config->PGData.crh;
  GPIOG->CRL = config->PGData.crl;
#endif
#endif
#endif
}

/**
 * @brief   Pads mode setup.
 * @details This function programs a pads group belonging to the same port
 *          with the specified mode.
 * @note    @p PAL_MODE_UNCONNECTED is implemented as push pull output at 2MHz.
 * @note    Writing on pads programmed as pull-up or pull-down has the side
 *          effect to modify the resistor setting because the output latched
 *          data is used for the resistor selection.
 *
 * @param[in] port      the port identifier
 * @param[in] mask      the group mask
 * @param[in] mode      the mode
 *
 * @notapi
 */
void _pal_lld_setgroupmode(ioportid_t port,
                           ioportmask_t mask,
                           iomode_t mode) {
  static const uint8_t cfgtab[] = {
    4,          /* PAL_MODE_RESET, implemented as input.*/
    2,          /* PAL_MODE_UNCONNECTED, implemented as push pull output 2MHz.*/
    4,          /* PAL_MODE_INPUT */
    8,          /* PAL_MODE_INPUT_PULLUP */
    8,          /* PAL_MODE_INPUT_PULLDOWN */
    0,          /* PAL_MODE_INPUT_ANALOG */
    3,          /* PAL_MODE_OUTPUT_PUSHPULL, 50MHz.*/
    7,          /* PAL_MODE_OUTPUT_OPENDRAIN, 50MHz.*/
    8,          /* Reserved.*/
    8,          /* Reserved.*/
    8,          /* Reserved.*/
    8,          /* Reserved.*/
    8,          /* Reserved.*/
    8,          /* Reserved.*/
    8,          /* Reserved.*/
    8,          /* Reserved.*/
    0xB,        /* PAL_MODE_STM32_ALTERNATE_PUSHPULL, 50MHz.*/
    0xF,        /* PAL_MODE_STM32_ALTERNATE_OPENDRAIN, 50MHz.*/
  };
  uint32_t mh, ml, crh, crl, cfg;
  unsigned i;

  if (mode == PAL_MODE_INPUT_PULLUP)
    port->BSRR = mask;
  else if (mode == PAL_MODE_INPUT_PULLDOWN)
    port->BRR = mask;
  cfg = cfgtab[mode];
  mh = ml = crh = crl = 0;
  for (i = 0; i < 8; i++) {
    ml <<= 4;
    mh <<= 4;
    crl <<= 4;
    crh <<= 4;
    if ((mask & 0x0080) == 0)
      ml |= 0xf;
    else
      crl |= cfg;
    if ((mask & 0x8000) == 0)
      mh |= 0xf;
    else
      crh |= cfg;
    mask <<= 1;
  }
  port->CRH = (port->CRH & mh) | crh;
  port->CRL = (port->CRL & ml) | crl;
}

#endif /* HAL_USE_PAL */

/** @} */
