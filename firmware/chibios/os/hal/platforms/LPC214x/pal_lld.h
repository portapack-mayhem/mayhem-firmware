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
 * @file    LPC214x/pal_lld.h
 * @brief   LPC214x FIO low level driver header.
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

#undef PAL_MODE_INPUT_PULLUP
#undef PAL_MODE_INPUT_PULLDOWN
#undef PAL_MODE_OUTPUT_OPENDRAIN

/*===========================================================================*/
/* I/O Ports Types and constants.                                            */
/*===========================================================================*/

/**
 * @brief   FIO port setup info.
 */
typedef struct {
  /** Initial value for FIO_PIN register.*/
  uint32_t      pin;
  /** Initial value for FIO_DIR register.*/
  uint32_t      dir;
} lpc214x_fio_setup_t;

/**
 * @brief   LPC214x FIO static initializer.
 * @details An instance of this structure must be passed to @p palInit() at
 *          system startup time in order to initialize the digital I/O
 *          subsystem. This represents only the initial setup, specific pads
 *          or whole ports can be reprogrammed at later time.
 */
typedef struct {
  /** @brief PINSEL0 initial value.*/
  uint32_t              pinsel0;
  /** @brief PINSEL1 initial value.*/
  uint32_t              pinsel1;
  /** @brief PINSEL2 initial value.*/
  uint32_t              pinsel2;
  /** @brief Port 0 setup data.*/
  lpc214x_fio_setup_t   P0Data;
  /** @brief Port 1 setup data.*/
  lpc214x_fio_setup_t   P1Data;
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
 */
typedef FIO * ioportid_t;

/*===========================================================================*/
/* I/O Ports Identifiers.                                                    */
/*===========================================================================*/

/**
 * @brief   FIO port 0 identifier.
 */
#define IOPORT1        FIO0Base

/**
 * @brief   FIO port 1 identifier.
 */
#define IOPORT2        FIO1Base

/*===========================================================================*/
/* Implementation, some of the following macros could be implemented as      */
/* functions, if so please put them in pal_lld.c.                            */
/*===========================================================================*/

/**
 * @brief   FIO subsystem initialization.
 * @details Enables the access through the fast registers.
 *
 * @notapi
 */
#define pal_lld_init(config) _pal_lld_init(config)

/**
 * @brief   Reads an I/O port.
 * @details This function is implemented by reading the FIO PIN register, the
 *          implementation has no side effects.
 *
 * @param[in] port      port identifier
 * @return              The port bits.
 *
 * @notapi
 */
#define pal_lld_readport(port) ((port)->FIO_PIN)

/**
 * @brief   Reads the output latch.
 * @details This function is implemented by reading the FIO SET register, the
 *          implementation has no side effects.
 *
 * @param[in] port      port identifier
 * @return              The latched logical states.
 *
 * @notapi
 */
#define pal_lld_readlatch(port) ((port)->FIO_SET)

/**
 * @brief   Writes a bits mask on a I/O port.
 * @details This function is implemented by writing the FIO PIN register, the
 *          implementation has no side effects.
 *
 * @param[in] port      port identifier
 * @param[in] bits      bits to be written on the specified port
 *
 * @notapi
 */
#define pal_lld_writeport(port, bits) ((port)->FIO_PIN = (bits))

/**
 * @brief   Sets a bits mask on a I/O port.
 * @details This function is implemented by writing the FIO SET register, the
 *          implementation has no side effects.
 *
 * @param[in] port      port identifier
 * @param[in] bits      bits to be ORed on the specified port
 *
 * @notapi
 */
#define pal_lld_setport(port, bits) ((port)->FIO_SET = (bits))

/**
 * @brief   Clears a bits mask on a I/O port.
 * @details This function is implemented by writing the FIO CLR register, the
 *          implementation has no side effects.
 *
 * @param[in] port      port identifier
 * @param[in] bits      bits to be cleared on the specified port
 *
 * @notapi
 */
#define pal_lld_clearport(port, bits) ((port)->FIO_CLR = (bits))

/**
 * @brief   Writes a value on an I/O bus.
 * @details This function is implemented by writing the FIO PIN and MASK
 *          registers, the implementation is not atomic because the multiple
 *          accesses.
 *
 * @param[in] port      port identifier
 * @param[in] mask      group mask, a logical AND is performed on the
 *                      output data
 * @param[in] offset    the group bit offset within the port
 * @param[in] bits      bits to be written. Values exceeding the group
 *                      width are masked.
 *
 * @notapi
 */
#define pal_lld_writegroup(port, mask, offset, bits)                    \
  ((port)->FIO_MASK = ~((mask) << (offset)),                            \
   (port)->FIO_PIN = (bits) << (offset),                                \
   (port)->FIO_MASK = 0)

/**
 * @brief   Pads group mode setup.
 * @details This function programs a pads group belonging to the same port
 *          with the specified mode.
 * @note    @p PAL_MODE_UNCONNECTED is implemented as push pull output with
 *          high state.
 * @note    This function does not alter the @p PINSELx registers. Alternate
 *          functions setup must be handled by device-specific code.
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

/**
 * @brief   FIO port setup.
 * @details This function programs the pins direction within a port.
 *
 * @notapi
 */
#define pal_lld_lpc214x_set_direction(port, dir) ((port)->FIO_DIR = (dir))

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
