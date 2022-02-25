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
 * @file    MSP430/pal_lld.h
 * @brief   MSP430 Digital I/O low level driver header.
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
#undef PAL_MODE_INPUT_ANALOG
#undef PAL_MODE_OUTPUT_OPENDRAIN

/*===========================================================================*/
/* I/O Ports Types and constants.                                            */
/*===========================================================================*/

/**
 * @brief   Simple MSP430 I/O port.
 */
struct msp430_port_simple_t {
  volatile uint8_t              in;
  volatile uint8_t              out;
  volatile uint8_t              dir;
  volatile uint8_t              sel;
};

/**
 * @brief   Full MSP430 I/O port.
 */
struct msp430_port_full_t {
  volatile uint8_t              in;
  volatile uint8_t              out;
  volatile uint8_t              dir;
  volatile uint8_t              ifg;
  volatile uint8_t              ies;
  volatile uint8_t              ie;
  volatile uint8_t              sel;
#if defined(__MSP430_HAS_PORT1_R__) || defined(__MSP430_HAS_PORT2_R__)
  volatile uint8_t             ren;
#endif
};

/**
 * @brief   Simplified MSP430 I/O port representation.
 * @details This structure represents the common part of all the MSP430 I/O
 *          ports.
 */
struct msp430_port_common {
  volatile uint8_t              in;
  volatile uint8_t              out;
  volatile uint8_t              dir;
};

/**
 * @brief   Generic MSP430 I/O port.
 */
typedef union {
  struct msp430_port_common     iop_common;
  struct msp430_port_simple_t   iop_simple;
  struct msp430_port_full_t     iop_full;
} msp430_ioport_t;

/**
 * @brief   Setup registers common to all the MSP430 ports.
 */
typedef struct  {
  volatile uint8_t              out;
  volatile uint8_t              dir;
} msp430_dio_setup_t;

/**
 * @brief   MSP430 I/O ports static initializer.
 * @details An instance of this structure must be passed to @p palInit() at
 *          system startup time in order to initialize the digital I/O
 *          subsystem. This represents only the initial setup, specific pads
 *          or whole ports can be reprogrammed at later time.
 */
typedef struct {
#if defined(__MSP430_HAS_PORT1__) ||                                    \
    defined(__MSP430_HAS_PORT1_R__) ||                                  \
    defined(__DOXYGEN__)
  /** @brief Port 1 setup data.*/
  msp430_dio_setup_t    P1Data;
#endif
#if defined(__MSP430_HAS_PORT2__) ||                                    \
    defined(__MSP430_HAS_PORT2_R__) ||                                  \
    defined(__DOXYGEN__)
  /** @brief Port 2 setup data.*/
  msp430_dio_setup_t    P2Data;
#endif
#if defined(__MSP430_HAS_PORT3__) ||                                    \
    defined(__MSP430_HAS_PORT3_R__) ||                                  \
    defined(__DOXYGEN__)
  /** @brief Port 3 setup data.*/
  msp430_dio_setup_t    P3Data;
#endif
#if defined(__MSP430_HAS_PORT4__) ||                                    \
    defined(__MSP430_HAS_PORT4_R__) ||                                  \
    defined(__DOXYGEN__)
  /** @brief Port 4 setup data.*/
  msp430_dio_setup_t    P4Data;
#endif
#if defined(__MSP430_HAS_PORT5__) ||                                    \
    defined(__MSP430_HAS_PORT5_R__) ||                                  \
    defined(__DOXYGEN__)
  /** @brief Port 5 setup data.*/
  msp430_dio_setup_t    P5Data;
#endif
#if defined(__MSP430_HAS_PORT6__) ||                                    \
    defined(__MSP430_HAS_PORT6_R__) ||                                  \
    defined(__DOXYGEN__)
  /** @brief Port 6 setup data.*/
  msp430_dio_setup_t    P6Data;
#endif
} PALConfig;

/**
 * @brief   Width, in bits, of an I/O port.
 */
#define PAL_IOPORTS_WIDTH 8

/**
 * @brief   Whole port mask.
 * @details This macro specifies all the valid bits into a port.
 */
#define PAL_WHOLE_PORT ((ioportmask_t)0xFF)

/**
 * @brief   Digital I/O port sized unsigned type.
 */
typedef uint8_t ioportmask_t;

/**
 * @brief   Digital I/O modes.
 */
typedef uint16_t iomode_t;

/**
 * @brief   Port Identifier.
 * @details This type can be a scalar or some kind of pointer, do not make
 *          any assumption about it, use the provided macros when populating
 *          variables of this type.
 */
typedef msp430_ioport_t *ioportid_t;

/*===========================================================================*/
/* I/O Ports Identifiers.                                                    */
/*===========================================================================*/

/**
 * @brief   I/O port A identifier.
 * @details This port identifier is mapped on the MSP430 port 1 (P1).
 */
#if defined(__MSP430_HAS_PORT1__) ||                                    \
    defined(__MSP430_HAS_PORT1_R__) ||                                  \
    defined(__DOXYGEN__)
#define IOPORT1         ((ioportid_t)P1IN_)
#endif

/**
 * @brief   I/O port B identifier.
 * @details This port identifier is mapped on the MSP430 port 2 (P2).
 */
#if defined(__MSP430_HAS_PORT2__) ||                                    \
    defined(__MSP430_HAS_PORT2_R__) ||                                  \
    defined(__DOXYGEN__)
#define IOPORT2         ((ioportid_t)P2IN_)
#endif

/**
 * @brief   I/O port C identifier.
 * @details This port identifier is mapped on the MSP430 port 3 (P3).
 */
#if defined(__MSP430_HAS_PORT3__) ||                                    \
    defined(__MSP430_HAS_PORT3_R__) ||                                  \
    defined(__DOXYGEN__)
#define IOPORT3         ((ioportid_t)P3IN_)
#endif

/**
 * @brief   I/O port D identifier.
 * @details This port identifier is mapped on the MSP430 port 4 (P4).
 */
#if defined(__MSP430_HAS_PORT4__) ||                                    \
    defined(__MSP430_HAS_PORT4_R__) ||                                  \
    defined(__DOXYGEN__)
#define IOPORT4         ((ioportid_t)P4IN_)
#endif

/**
 * @brief   I/O port E identifier.
 * @details This port identifier is mapped on the MSP430 port 5 (P5).
 */
#if defined(__MSP430_HAS_PORT5__) ||                                    \
    defined(__MSP430_HAS_PORT5_R__) ||                                  \
    defined(__DOXYGEN__)
#define IOPORT5         ((ioportid_t)P5IN_)
#endif

/**
 * @brief   I/O port F identifier.
 * @details This port identifier is mapped on the MSP430 port 6 (P6).
 */
#if defined(__MSP430_HAS_PORT6__) ||                                    \
    defined(__MSP430_HAS_PORT6_R__) ||                                  \
    defined(__DOXYGEN__)
#define IOPORT6         ((ioportid_t)P6IN_)
#endif

/*===========================================================================*/
/* Implementation, some of the following macros could be implemented as      */
/* functions, if so please put them in pal_lld.c.                            */
/*===========================================================================*/

/**
 * @brief   Low level PAL subsystem initialization.
 * @details In MSP430 programs all the ports as input.
 *
 * @param[in] config the MSP430 ports configuration
 *
 * @notapi
 */
#define pal_lld_init(config) _pal_lld_init(config)

/**
 * @brief   Reads the physical I/O port states.
 * @details This function is implemented by reading the PxIN register, the
 *          implementation has no side effects.
 *
 * @param[in] port      port identifier
 * @return              The port bits.
 *
 * @notapi
 */
#define pal_lld_readport(port) ((port)->iop_common.in)

/**
 * @brief   Reads the output latch.
 * @details This function is implemented by reading the PxOUT register, the
 *          implementation has no side effects.
 *
 * @param[in] port      port identifier
 * @return              The latched logical states.
 *
 * @notapi
 */
#define pal_lld_readlatch(port) ((port)->iop_common.out)

/**
 * @brief   Writes a bits mask on a I/O port.
 * @details This function is implemented by writing the PxOUT register, the
 *          implementation has no side effects.
 *
 * @param[in] port      port identifier
 * @param[in] bits      bits to be written on the specified port
 *
 * @notapi
 */
#define pal_lld_writeport(port, bits) ((port)->iop_common.out = (bits))

/**
 * @brief   Pads group mode setup.
 * @details This function programs a pads group belonging to the same port
 *          with the specified mode.
 * @note    @p PAL_MODE_UNCONNECTED is implemented as output as recommended by
 *          the MSP430x1xx Family User's Guide.
 * @note    This function does not alter the @p PxSEL registers. Alternate
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

#endif /* _PAL_LLD_H_ */

#endif /* HAL_USE_PAL */

/** @} */
