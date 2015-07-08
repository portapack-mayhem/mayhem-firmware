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
 * @file    STM32/GPIOv2/pal_lld.c
 * @brief   STM32L1xx/STM32F2xx/STM32F4xx GPIO low level driver code.
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

#if defined(STM32L1XX)
#define AHB_EN_MASK     (RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN |          \
                         RCC_AHBENR_GPIOCEN | RCC_AHBENR_GPIODEN |          \
                         RCC_AHBENR_GPIOEEN | RCC_AHBENR_GPIOHEN)
#define AHB_LPEN_MASK   AHB_EN_MASK

#elif defined(STM32F030) || defined(STM32F0XX_MD)
#define AHB_EN_MASK     (RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN |          \
                         RCC_AHBENR_GPIOCEN | RCC_AHBENR_GPIODEN |          \
                         RCC_AHBENR_GPIOFEN)

#elif defined(STM32F0XX_LD)
#define AHB_EN_MASK     (RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN |          \
                         RCC_AHBENR_GPIOCEN | RCC_AHBENR_GPIOFEN)

#elif defined(STM32F2XX)
#define AHB1_EN_MASK    (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN |        \
                         RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN |        \
                         RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOFEN |        \
                         RCC_AHB1ENR_GPIOGEN | RCC_AHB1ENR_GPIOHEN |        \
                         RCC_AHB1ENR_GPIOIEN)
#define AHB1_LPEN_MASK  AHB1_EN_MASK

#elif defined(STM32F30X) || defined(STM32F37X)
#define AHB_EN_MASK     (RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN |          \
                         RCC_AHBENR_GPIOCEN | RCC_AHBENR_GPIODEN |          \
                         RCC_AHBENR_GPIOEEN | RCC_AHBENR_GPIOFEN)

#elif defined(STM32F4XX)
#if STM32_HAS_GPIOF && STM32_HAS_GPIOG && STM32_HAS_GPIOI
#define AHB1_EN_MASK    (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN |        \
                         RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN |        \
                         RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOFEN |        \
                         RCC_AHB1ENR_GPIOGEN | RCC_AHB1ENR_GPIOHEN |        \
                         RCC_AHB1ENR_GPIOIEN)
#else
#define AHB1_EN_MASK    (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN |        \
                         RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN |        \
                         RCC_AHB1ENR_GPIOEEN)
#endif /* STM32_HAS_GPIOF && STM32_HAS_GPIOG && STM32_HAS_GPIOI */
#define AHB1_LPEN_MASK  AHB1_EN_MASK

#else
#error "missing or unsupported platform for GPIOv2 PAL driver"
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

static void initgpio(GPIO_TypeDef *gpiop, const stm32_gpio_setup_t *config) {

  gpiop->OTYPER  = config->otyper;
  gpiop->OSPEEDR = config->ospeedr;
  gpiop->PUPDR   = config->pupdr;
  gpiop->ODR     = config->odr;
  gpiop->AFRL    = config->afrl;
  gpiop->AFRH    = config->afrh;
  gpiop->MODER   = config->moder;
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   STM32 I/O ports configuration.
 * @details Ports A-D(E, F, G, H) clocks enabled.
 *
 * @param[in] config    the STM32 ports configuration
 *
 * @notapi
 */
void _pal_lld_init(const PALConfig *config) {

  /*
   * Enables the GPIO related clocks.
   */
#if defined(STM32L1XX)
  rccEnableAHB(AHB_EN_MASK, TRUE);
  RCC->AHBLPENR |= AHB_LPEN_MASK;
#elif defined(STM32F0XX)
  rccEnableAHB(AHB_EN_MASK, TRUE);
#elif defined(STM32F30X) || defined(STM32F37X)
  rccEnableAHB(AHB_EN_MASK, TRUE);
#elif defined(STM32F2XX) || defined(STM32F4XX)
  RCC->AHB1ENR   |= AHB1_EN_MASK;
  RCC->AHB1LPENR |= AHB1_LPEN_MASK;
#endif

  /*
   * Initial GPIO setup.
   */
#if STM32_HAS_GPIOA
  initgpio(GPIOA, &config->PAData);
#endif
#if STM32_HAS_GPIOB
  initgpio(GPIOB, &config->PBData);
#endif
#if STM32_HAS_GPIOC
  initgpio(GPIOC, &config->PCData);
#endif
#if STM32_HAS_GPIOD
  initgpio(GPIOD, &config->PDData);
#endif
#if STM32_HAS_GPIOE
  initgpio(GPIOE, &config->PEData);
#endif
#if STM32_HAS_GPIOF
  initgpio(GPIOF, &config->PFData);
#endif
#if STM32_HAS_GPIOG
  initgpio(GPIOG, &config->PGData);
#endif
#if STM32_HAS_GPIOH
  initgpio(GPIOH, &config->PHData);
#endif
#if STM32_HAS_GPIOI
  initgpio(GPIOI, &config->PIData);
#endif
}

/**
 * @brief   Pads mode setup.
 * @details This function programs a pads group belonging to the same port
 *          with the specified mode.
 * @note    @p PAL_MODE_UNCONNECTED is implemented as push pull at minimum
 *          speed.
 *
 * @param[in] port      the port identifier
 * @param[in] mask      the group mask
 * @param[in] mode      the mode
 *
 * @notapi
 */
#if 1
void _pal_lld_setgroupmode(ioportid_t port,
                           ioportmask_t mask,
                           iomode_t mode) {

  uint32_t moder   = (mode & PAL_STM32_MODE_MASK) >> 0;
  uint32_t otyper  = (mode & PAL_STM32_OTYPE_MASK) >> 2;
  uint32_t ospeedr = (mode & PAL_STM32_OSPEED_MASK) >> 3;
  uint32_t pupdr   = (mode & PAL_STM32_PUDR_MASK) >> 5;
  uint32_t altr    = (mode & PAL_STM32_ALTERNATE_MASK) >> 7;
  uint32_t bit     = 0;
  while (TRUE) {
    if ((mask & 1) != 0) {
      uint32_t altrmask, m1, m2, m4;

      altrmask = altr << ((bit & 7) * 4);
      m4 = 15 << ((bit & 7) * 4);
      if (bit < 8)
        port->AFRL = (port->AFRL & ~m4) | altrmask;
      else
        port->AFRH = (port->AFRH & ~m4) | altrmask;
      m1 = 1 << bit;
      port->OTYPER  = (port->OTYPER & ~m1) | otyper;
      m2 = 3 << (bit * 2);
      port->OSPEEDR = (port->OSPEEDR & ~m2) | ospeedr;
      port->PUPDR   = (port->PUPDR & ~m2) | pupdr;
      port->MODER   = (port->MODER & ~m2) | moder;
    }
    mask >>= 1;
    if (!mask)
      return;
    otyper <<= 1;
    ospeedr <<= 2;
    pupdr <<= 2;
    moder <<= 2;
    bit++;
  }
}
#else
void _pal_lld_setgroupmode(ioportid_t port,
                           ioportmask_t mask,
                           iomode_t mode) {
  uint32_t afrm, moderm, pupdrm, otyperm, ospeedrm;
  uint32_t m1 = (uint32_t)mask;
  uint32_t m2 = 0;
  uint32_t m4l = 0;
  uint32_t m4h = 0;
  uint32_t bit = 0;
  do {
    if ((mask & 1) != 0) {
      m2 |= 3 << bit;
      if (bit < 16)
        m4l |= 15 << ((bit & 14) * 2);
      else
        m4h |= 15 << ((bit & 14) * 2);
    }
    bit += 2;
    mask >>= 1;
  } while (mask);

  afrm = ((mode & PAL_STM32_ALTERNATE_MASK) >> 7) * 0x1111;
  port->AFRL = (port->AFRL & ~m4l) | (afrm & m4l);
  port->AFRH = (port->AFRH & ~m4h) | (afrm & m4h);

  ospeedrm = ((mode & PAL_STM32_OSPEED_MASK) >> 3) * 0x5555;
  port->OSPEEDR = (port->OSPEEDR & ~m2) | (ospeedrm & m2);

  otyperm = ((mode & PAL_STM32_OTYPE_MASK) >> 2) * 0xffff;
  port->OTYPER = (port->OTYPER & ~m1) | (otyperm & m1);

  pupdrm = ((mode & PAL_STM32_PUDR_MASK) >> 5) * 0x5555;
  port->PUPDR = (port->PUPDR & ~m2) | (pupdrm & m2);

  moderm = ((mode & PAL_STM32_MODE_MASK) >> 0) * 0x5555;
  port->MODER = (port->MODER & ~m2) | (moderm & m2);
}
#endif

#endif /* HAL_USE_PAL */

/** @} */
