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
 * @file	AVR/pal_lld.h
 * @brief	AVR GPIO low level driver header.
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

#undef PAL_MODE_INPUT_PULLDOWN
#undef PAL_MODE_OUTPUT_OPENDRAIN

/*===========================================================================*/
/* I/O Ports Types and constants.                                            */
/*===========================================================================*/

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
 * @brief   AVR setup registers.
 */
typedef struct  {
  uint8_t  out;
  uint8_t  dir;
} avr_gpio_setup_t;

/**
 * @brief   AVR registers block.
 * @note    On some devices registers do not follow this layout on some
 *          ports, the ports with abnormal layout cannot be used through
 *          PAL driver. Example: PORT F on Mega128.
 */
typedef struct  {
  volatile uint8_t  in;
  volatile uint8_t  dir;
  volatile uint8_t  out;
} avr_gpio_registers_t;

/**
 * @brief   Generic I/O ports static initializer.
 * @details An instance of this structure must be passed to @p palInit() at
 *          system startup time in order to initialized the digital I/O
 *          subsystem. This represents only the initial setup, specific pads
 *          or whole ports can be reprogrammed at later time.
 */
typedef struct {
#if defined(PORTA) || defined(__DOXYGEN__)
  avr_gpio_setup_t porta;
#endif
#if defined(PORTB) || defined(__DOXYGEN__)
  avr_gpio_setup_t portb;
#endif
#if defined(PORTC) || defined(__DOXYGEN__)
  avr_gpio_setup_t portc;
#endif
#if defined(PORTD) || defined(__DOXYGEN__)
  avr_gpio_setup_t portd;
#endif
#if defined(PORTE) || defined(__DOXYGEN__)
  avr_gpio_setup_t porte;
#endif
#if defined(PORTF) || defined(__DOXYGEN__)
  avr_gpio_setup_t portf;
#endif
#if defined(PORTG) || defined(__DOXYGEN__)
  avr_gpio_setup_t portg;
#endif
#if defined(PORTH) || defined(__DOXYGEN__)
  avr_gpio_setup_t porth;
#endif
#if defined(PORTJ) || defined(__DOXYGEN__)
  avr_gpio_setup_t portj;
#endif
#if defined(PORTK) || defined(__DOXYGEN__)
  avr_gpio_setup_t portk;
#endif
#if defined(PORTL) || defined(__DOXYGEN__)
  avr_gpio_setup_t portl;
#endif
} PALConfig;

/**
 * @brief   Digital I/O port sized unsigned type.
 */
typedef uint8_t ioportmask_t;

/**
 * @brief   Digital I/O modes.
 */
typedef uint8_t iomode_t;

/**
 * @brief   Port Identifier.
 * @details This type can be a scalar or some kind of pointer, do not make
 *          any assumption about it, use the provided macros when populating
 *          variables of this type.
 */
typedef avr_gpio_registers_t *ioportid_t;

/*===========================================================================*/
/* I/O Ports Identifiers.                                                    */
/*===========================================================================*/

#if defined(PORTA) || defined(__DOXYGEN__)
/**
 * @brief   GPIO port A identifier.
 */
#define IOPORT1			((volatile avr_gpio_registers_t *)&PINA)
#endif

#if defined(PORTB) || defined(__DOXYGEN__)
/**
 * @brief   GPIO port B identifier.
 */
#define IOPORT2			((volatile avr_gpio_registers_t *)&PINB)
#endif

#if defined(PORTC) || defined(__DOXYGEN__)
/**
 * @brief   GPIO port C identifier.
 */
#define IOPORT3			((volatile avr_gpio_registers_t *)&PINC)
#endif

#if defined(PORTD) || defined(__DOXYGEN__)
/**
 * @brief   GPIO port D identifier.
 */
#define IOPORT4			((volatile avr_gpio_registers_t *)&PIND)
#endif

#if defined(PORTE) || defined(__DOXYGEN__)
/**
 * @brief   GPIO port E identifier.
 */
#define IOPORT5			((volatile avr_gpio_registers_t *)&PINE)
#endif

#if defined(PORTF) || defined(__DOXYGEN__)
/**
 * @brief   GPIO port F identifier.
 */
#define IOPORT6			((volatile avr_gpio_registers_t *)&PINF)
#endif

#if defined(PORTG) || defined(__DOXYGEN__)
/**
 * @brief   GPIO port G identifier.
 */
#define IOPORT7			((volatile avr_gpio_registers_t *)&PING)
#endif

#if defined(PORTH) || defined(__DOXYGEN__)
/**
 * @brief   GPIO port H identifier.
 */
#define IOPORT8         ((volatile avr_gpio_registers_t *)&PINH)
#endif

#if defined(PORTJ) || defined(__DOXYGEN__)
/**
 * @brief   GPIO port J identifier.
 */
#define IOPORT9         ((volatile avr_gpio_registers_t *)&PINJ)
#endif

#if defined(PORTK) || defined(__DOXYGEN__)
/**
 * @brief   GPIO port K identifier.
 */
#define IOPORT10        ((volatile avr_gpio_registers_t *)&PINK)
#endif

#if defined(PORTL) || defined(__DOXYGEN__)
/**
 * @brief   GPIO port L identifier.
 */
#define IOPORT11        ((volatile avr_gpio_registers_t *)&PINL)
#endif

/*===========================================================================*/
/* Implementation, some of the following macros could be implemented as      */
/* functions, if so please put them in pal_lld.c.                            */
/*===========================================================================*/

/**
 * @brief   Low level PAL subsystem initialization.
 *
 * @param[in] config the architecture-dependent ports configuration
 *
 * @notapi
 */
#define pal_lld_init(config) _pal_lld_init(config)

/**
 * @brief   Reads the physical I/O port states.
 *
 * @param[in] port      port identifier
 * @return              The port bits.
 *
 * @notapi
 */
#define pal_lld_readport(port) ((port)->in)

/**
 * @brief   Reads the output latch.
 * @details The purpose of this function is to read back the latched output
 *          value.
 *
 * @param[in] port      port identifier
 * @return              The latched logical states.
 *
 * @notapi
 */
#define pal_lld_readlatch(port) ((port)->out)

/**
 * @brief   Writes a bits mask on a I/O port.
 *
 * @param[in] port      port identifier
 * @param[in] bits      bits to be written on the specified port
 *
 * @notapi
 */
#define pal_lld_writeport(port, bits) ((port)->out = bits)

/**
 * @brief   Pads group mode setup.
 * @details This function programs a pads group belonging to the same port
 *          with the specified mode.
 * @note    Programming an unknown or unsupported mode is silently ignored.
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
 * @brief   Sets a pad logical state to @p PAL_HIGH.
 *
 * @param[in] port      port identifier
 * @param[in] pad       pad number within the port
 *
 * @notapi
 */
#define pal_lld_setpad(port, pad)                                           \
__asm__ __volatile__                                                        \
(                                                                           \
	"sbi %0,%1\n\t"                                                         \
	:                                                                       \
	: "I" (_SFR_IO_ADDR(port->out)),                                        \
	  "I" (pad)                                                             \
								                                            \
)

/**
 * @brief   Clears a pad logical state to @p PAL_LOW.
 *
 * @param[in] port      port identifier
 * @param[in] pad       pad number within the port
 *
 * @notapi
 */
#define pal_lld_clearpad(port, pad)                                         \
__asm__ __volatile__                                                        \
(                                                                           \
	"cbi %0,%1\n\t"                                                         \
	:                                                                       \
	: "I" (_SFR_IO_ADDR(port->out)),                                        \
	  "I" (pad)                                                             \
								                                            \
)

extern ROMCONST PALConfig pal_default_config;

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
