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
 * @file    AT91SAM7/ext_lld.h
 * @brief   AT91SAM7 EXT subsystem low level driver header.
 *
 * @addtogroup EXT
 * @{
 */

#ifndef _EXT_LLD_H_
#define _EXT_LLD_H_

#if HAL_USE_EXT || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief    Pointer to the SAM7 AIC register block.
 */
#define SAM7_EXT_AIC                  ((AT91PS_AIC *)AT91C_BASE_AIC)

/**
 * @brief    Number of channels within one ext driver.
 */
#define EXT_MAX_CHANNELS              32

/**
 * @brief    Mask of priority bits in interrupt mode register.
 */
#define SAM7_EXT_PRIORITY_MASK        0x00000007

/**
 * @brief    Shifter for priority bits in interrupt mode register.
 */
#define SAM7_EXT_PRIORITY_SHIFTER     0

/**
 * @brief    Shifter for mode bits in interrupt mode register.
 */
#define SAM7_EXT_MODE_SHIFTER         5

/*
 * On the SAM7 architecture, a single channel can only be enables or disabled
 * Hence, undefine the other channel mode constants
 */
#ifdef EXT_CH_MODE_RISING_EDGE
#undef EXT_CH_MODE_RISING_EDGE
#endif

#ifdef EXT_CH_MODE_FALLING_EDGE
#undef EXT_CH_MODE_FALLING_EDGE
#endif

#ifdef EXT_CH_MODE_BOTH_EDGES
#undef EXT_CH_MODE_BOTH_EDGES
#endif

/**
 * @name    EXT channels mode
 * @{
 */
#define EXT_CH_MODE_ENABLED           1     /**< @brief Channel is enabled. */
/** @} */

/**
 * @name    EXT drivers mode
 * @{
 */
/**
 * @brief   Mask for modes.
 */
#define SAM7_EXT_MODE_MASK            AT91C_AIC_SRCTYPE
/**
 * @brief   Falling edge callback.
 */
#define SAM7_EXT_MODE_FALLING_EDGE    AT91C_AIC_SRCTYPE_EXT_NEGATIVE_EDGE
/**
 * @brief   Rising edge callback.
 */
#define SAM7_EXT_MODE_RISING_EDGE     AT91C_AIC_SRCTYPE_POSITIVE_EDGE
/**
 * @brief   High-level callback.
 */
#define SAM7_EXT_MODE_HIGH_LEVEL      AT91C_AIC_SRCTYPE_HIGH_LEVEL
/**
 * @brief   Low-level callback.
 */
#define SAM7_EXT_MODE_LOW_LEVEL       AT91C_AIC_SRCTYPE_EXT_LOW_LEVEL
/** @} */

/**
 * @name    EXT drivers priorities
 * @{
 */
#define SAM7_EXT_PRIOR_HIGHEST        AT91C_AIC_PRIOR_HIGHEST
#define SAM7_EXT_PRIOR_LOWEST         AT91C_AIC_PRIOR_LOWEST
/** @} */

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
 * @brief   EXT channel identifier.
 */
typedef uint32_t expchannel_t;

/**
 * @brief   Type of an EXT generic notification callback.
 *
 * @param[in] extp      pointer to the @p EXPDriver object triggering the
 *                      callback
 */
typedef void (*extcallback_t)(EXTDriver *extp, expchannel_t channel);

/**
 * @brief   Channel configuration structure.
 */
typedef struct {
  /**
   * @brief Channel mode.
   */
  uint32_t              mode;
  /**
   * @brief Channel callback.
   */
  extcallback_t         cb;
} EXTChannelConfig;

/**
 * @brief   Driver configuration structure.
 * @note    It could be empty on some architectures.
 */
typedef struct {
  /**
   * @brief Channel configurations.
   */
  EXTChannelConfig      channels[EXT_MAX_CHANNELS];
  /* End of the mandatory fields.*/
  /**
   * @brief interrupt mode.
   */
  uint32_t              mode;
  /**
   * @brief interrupt priority.
   */
  uint32_t              priority;
} EXTConfig;

/**
 * @brief   Structure representing an EXT driver.
 */
struct EXTDriver {
  /**
   * @brief Driver state.
   */
  extstate_t            state;
  /**
   * @brief Current configuration data.
   */
  const EXTConfig       *config;
  /* End of the mandatory fields.*/

  /**
   * @brief Pointer to the corresponding PIO registers block.
   */
  AT91PS_PIO            pio;
  /**
   * @brief peripheral ID of the corresponding PIO block.
   */
  uint32_t              pid;
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief    Computes the content for the interrupt source mode register.
 */
#define SAM7_computeSMR(mode, prio) (                                       \
    ((mode & SAM7_EXT_MODE_MASK) << SAM7_EXT_MODE_SHIFTER) |                \
    ((prio & SAM7_EXT_PRIORITY_MASK) << SAM7_EXT_PRIORITY_SHIFTER)          \
)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if !defined(__DOXYGEN__)
extern EXTDriver EXTDA;
#if (SAM7_PLATFORM == SAM7X128) || (SAM7_PLATFORM == SAM7X256) ||           \
    (SAM7_PLATFORM == SAM7X512) || (SAM7_PLATFORM == SAM7A3)
extern EXTDriver EXTDB;
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void ext_lld_init(void);
  void ext_lld_start(EXTDriver *extp);
  void ext_lld_stop(EXTDriver *extp);
  void ext_lld_channel_enable(EXTDriver *extp, expchannel_t channel);
  void ext_lld_channel_disable(EXTDriver *extp, expchannel_t channel);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_EXT */

#endif /* _EXT_LLD_H_ */

/** @} */
