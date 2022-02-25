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
 * @file    AT91SAM7/pal_lld.h
 * @brief   AT91SAM7 PIO low level driver header.
 *
 * @addtogroup PAL
 * @{
 */

#ifndef _PAL_LLD_H_
#define _PAL_LLD_H_

#if HAL_USE_PAL || defined(__DOXYGEN__)

/*===========================================================================*/
/* Unsupported modes and specific modes                                      */
/*===========================================================================*/

// AT91: Port direction
#define AT91_PAL_DIR_MASK		0x03
#define AT91_PAL_DIR_INPUT		0x00
#define AT91_PAL_DIR_OUTPUT		0x01
#define AT91_PAL_DIR_PERIPH_A	0x02
#define AT91_PAL_DIR_PERIPH_B	0x03

// AT91: Additional flags
#define AT91_PAL_OUT_SET		0x04
#define AT91_PAL_OUT_CLEAR		0x08
#define AT91_PAL_PULLUP			0x10
#define AT91_PAL_OPENDRAIN		0x20

// Unsupported
#undef PAL_MODE_INPUT_PULLDOWN

// Redefine the standard io modes
#undef PAL_MODE_RESET
#undef PAL_MODE_UNCONNECTED
#undef PAL_MODE_INPUT
#undef PAL_MODE_INPUT_PULLUP
#undef PAL_MODE_INPUT_ANALOG
#undef PAL_MODE_OUTPUT_PUSHPULL
#undef PAL_MODE_OUTPUT_OPENDRAIN
#define PAL_MODE_RESET                  (AT91_PAL_DIR_INPUT|AT91_PAL_PULLUP)
#define PAL_MODE_UNCONNECTED            (AT91_PAL_DIR_OUTPUT|AT91_PAL_OUT_SET)
#define PAL_MODE_INPUT                  (AT91_PAL_DIR_INPUT)
#define PAL_MODE_INPUT_PULLUP           (AT91_PAL_DIR_INPUT|AT91_PAL_PULLUP)
#define PAL_MODE_INPUT_ANALOG           (AT91_PAL_DIR_INPUT)
#define PAL_MODE_OUTPUT_PUSHPULL        (AT91_PAL_DIR_OUTPUT)
#define PAL_MODE_OUTPUT_OPENDRAIN       (AT91_PAL_DIR_OUTPUT|AT91_PAL_PULLUP|AT91_PAL_OPENDRAIN)

// Add the AT91 specific ones
#define PAL_MODE_PERIPH_A_PUSHPULL      (AT91_PAL_DIR_PERIPH_A)
#define PAL_MODE_PERIPH_A_OPENDRAIN     (AT91_PAL_DIR_PERIPH_A|AT91_PAL_PULLUP|AT91_PAL_OPENDRAIN)
#define PAL_MODE_PERIPH_B_PUSHPULL      (AT91_PAL_DIR_PERIPH_A)
#define PAL_MODE_PERIPH_B_OPENDRAIN     (AT91_PAL_DIR_PERIPH_A|AT91_PAL_PULLUP|AT91_PAL_OPENDRAIN)

/*===========================================================================*/
/* I/O Ports Types and constants.                                            */
/*===========================================================================*/

/**
 * @brief   PIO port setup info.
 */
typedef struct {
  /** Initial value for ODSR register (data).*/
  uint32_t      odsr;
  /** Initial value for OSR register (direction).*/
  uint32_t      osr;
  /** Initial value for PUSR register (Pull-ups).*/
  uint32_t      pusr;
} at91sam7_pio_setup_t;

/**
 * @brief   AT91SAM7 PIO static initializer.
 * @details An instance of this structure must be passed to @p palInit() at
 *          system startup time in order to initialize the digital I/O
 *          subsystem. This represents only the initial setup, specific pads
 *          or whole ports can be reprogrammed at later time.
 */
typedef struct {
  /** @brief Port 0 setup data.*/
  at91sam7_pio_setup_t P0Data;
#if (SAM7_PLATFORM == SAM7X128) || (SAM7_PLATFORM == SAM7X256) || \
    (SAM7_PLATFORM == SAM7X512) || (SAM7_PLATFORM == SAM7A3) || \
    defined(__DOXYGEN__)
  /** @brief Port 1 setup data.*/
  at91sam7_pio_setup_t P1Data;
#endif
} PALConfig;

/**
 * @brief   Width, in bits, of an I/O port.
 */
#define PAL_IOPORTS_WIDTH 32

/**
 * @brief   Whole port mask.
 * @details This macro specifies all the valid bits into a port.
 */
#define PAL_WHOLE_PORT ((ioportmask_t)0xFFFFFFFF)

/**
 * @brief   Digital I/O port sized unsigned type.
 */
typedef uint32_t ioportmask_t;

/**
 * @brief   Digital I/O modes.
 */
typedef uint32_t iomode_t;

/**
 * @brief   Port Identifier.
 * @details This type can be a scalar or some kind of pointer, do not make
 *          any assumption about it, use the provided macros when populating
 *          variables of this type.
 */
typedef AT91PS_PIO ioportid_t;

/*===========================================================================*/
/* I/O Ports Identifiers.                                                    */
/*===========================================================================*/

/**
 * @brief   PIO port A identifier.
 */
#define IOPORT1         AT91C_BASE_PIOA

/**
 * @brief   PIO port B identifier.
 */
#if (SAM7_PLATFORM == SAM7X128) || (SAM7_PLATFORM == SAM7X256) || \
    (SAM7_PLATFORM == SAM7X512) || (SAM7_PLATFORM == SAM7A3) || \
    defined(__DOXYGEN__)
#define IOPORT2         AT91C_BASE_PIOB
#endif

/*===========================================================================*/
/* Implementation, some of the following macros could be implemented as      */
/* functions, if so please put them in pal_lld.c.                            */
/*===========================================================================*/

/**
 * @brief   Low level PAL subsystem initialization.
 *
 * @param[in] config    architecture-dependent ports configuration
 *
 * @notapi
 */
#define pal_lld_init(config) _pal_lld_init(config)

/**
 * @brief   Reads the physical I/O port states.
 * @details This function is implemented by reading the PIO_PDSR register, the
 *          implementation has no side effects.
 *
 * @param[in] port      port identifier
 * @return              The port bits.
 *
 * @notapi
 */
#define pal_lld_readport(port) ((port)->PIO_PDSR)

/**
 * @brief   Reads the output latch.
 * @details This function is implemented by reading the PIO_ODSR register, the
 *          implementation has no side effects.
 *
 * @param[in] port      port identifier
 * @return              The latched logical states.
 *
 * @notapi
 */
#define pal_lld_readlatch(port) ((port)->PIO_ODSR)

/**
 * @brief   Writes a bits mask on a I/O port.
 * @details This function is implemented by writing the PIO_ODSR register, the
 *          implementation has no side effects.
 *
 * @param[in] port the port identifier
 * @param[in] bits the bits to be written on the specified port
 *
 * @notapi
 */
#define pal_lld_writeport(port, bits) ((port)->PIO_ODSR = (bits))

/**
 * @brief   Sets a bits mask on a I/O port.
 * @details This function is implemented by writing the PIO_SODR register, the
 *          implementation has no side effects.
 *
 * @param[in] port      port identifier
 * @param[in] bits      bits to be ORed on the specified port
 *
 * @notapi
 */
#define pal_lld_setport(port, bits) ((port)->PIO_SODR = (bits))

/**
 * @brief   Clears a bits mask on a I/O port.
 * @details This function is implemented by writing the PIO_CODR register, the
 *          implementation has no side effects.
 *
 * @param[in] port      port identifier
 * @param[in] bits      bits to be cleared on the specified port
 *
 * @notapi
 */
#define pal_lld_clearport(port, bits) ((port)->PIO_CODR = (bits))

/**
 * @brief   Writes a group of bits.
 * @details This function is implemented by writing the PIO_OWER, PIO_ODSR and
 *          PIO_OWDR registers, the implementation is not atomic because the
 *          multiple accesses.
 *
 * @param[in] port      port identifier
 * @param[in] mask      group mask
 * @param[in] offset    the group bit offset within the port
 * @param[in] bits      bits to be written. Values exceeding the group
 *                      width are masked.
 *
 * @notapi
 */
#define pal_lld_writegroup(port, mask, offset, bits)                    \
  ((port)->PIO_OWER = (mask) << (offset),                               \
   (port)->PIO_ODSR = (bits) << (offset),                               \
   (port)->PIO_OWDR = (mask) << (offset))

/**
 * @brief   Pads group mode setup.
 * @details This function programs a pads group belonging to the same port
 *          with the specified mode.
 * @note    @p PAL_MODE_UNCONNECTED is implemented as push pull output with
 *          high state.
 *
 * @param[in] port      port identifier
 * @param[in] mask      group mask
 * @param[in] offset    group bit offset within the port
 * @param[in] mode      group mode
 *
 * @notapi
 */
#define pal_lld_setgroupmode(port, mask, offset, mode)                      \
  _pal_lld_setgroupmode(port, mask << offset, mode)

/**
 * @brief   Writes a logical state on an output pad.
 *
 * @param[in] port      port identifier
 * @param[in] pad       pad number within the port
 * @param[in] bit       logical value, the value must be @p PAL_LOW or
 *                      @p PAL_HIGH
 *
 * @notapi
 */
#define pal_lld_writepad(port, pad, bit) pal_lld_writegroup(port, 1, pad, bit)

extern const PALConfig pal_default_config;

#ifdef __cplusplus
extern "C" {
#endif
  void _pal_lld_init(const PALConfig *config);
  void _pal_lld_setgroupmode(ioportid_t port,
                             ioportmask_t mask,
                             iomode_t mode);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_PAL */

#endif /* _PAL_LLD_H_ */

/** @} */
