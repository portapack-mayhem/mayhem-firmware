/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
                 2011,2012,2013 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

                                      ---

    A special exception to the GPL can be applied should you wish to distribute
    a combined work that includes ChibiOS/RT, without being obliged to provide
    the source code for any proprietary components. See the file exception.txt
    for full details of how and when the exception can be applied.
*/

/**
 * @file    templates/chtypes.h
 * @brief   System types template.
 * @details The types defined in this file may change depending on the target
 *          architecture. You may also try to optimize the size of the various
 *          types in order to privilege size or performance, be careful in
 *          doing so.
 *
 * @addtogroup types
 * @details System types and macros.
 * @{
 */

#ifndef _CHTYPES_H_
#define _CHTYPES_H_

#define __need_NULL
#define __need_size_t
#include <stddef.h>

#if !defined(_STDINT_H) && !defined(__STDINT_H_)
#include <stdint.h>
#endif

/**
 * @brief   Boolean, recommended the fastest signed.
 */
typedef int32_t         bool_t;

/**
 * @brief   Thread mode flags, uint8_t is ok.
 */
typedef uint8_t         tmode_t;

/**
 * @brief   Thread state, uint8_t is ok.
 */
typedef uint8_t         tstate_t;

/**
 * @brief   Thread references counter, uint8_t is ok.
 */
typedef uint8_t         trefs_t;

/**
 * @brief   Priority, use the fastest unsigned type.
 */
typedef uint32_t        tprio_t;

/**
 * @brief   Message, use signed pointer equivalent.
 */
typedef int32_t         msg_t;

/**
 * @brief   Event Id, use fastest signed.
 */
typedef int32_t         eventid_t;

/**
 * @brief   Event Mask, recommended fastest unsigned.
 */
typedef uint32_t        eventmask_t;

/**
 * @brief   System Time, recommended fastest unsigned.
 */
typedef uint32_t        systime_t;

/**
 * @brief   Counter, recommended fastest signed.
 */
typedef int32_t         cnt_t;

/**
 * @brief   Inline function modifier.
 */
#define INLINE inline

/**
 * @brief   ROM constant modifier.
 * @note    This is required because some compilers require a custom keyword,
 *          usually this macro is just set to "const" for the GCC compiler.
 * @note    This macro is not used to place constants in different address
 *          spaces (like AVR requires for example) because it is assumed that
 *          a pointer to a ROMCONST constant is compatible with a pointer
 *          to a normal variable. It is just like the "const" keyword but
 *          requires that the constant is placed in ROM if the architecture
 *          supports it.
 */
#define ROMCONST const

/**
 * @brief   Packed structure modifier (within).
 */
#define PACK_STRUCT_STRUCT __attribute__((packed))

/**
 * @brief   Packed structure modifier (before).
 */
#define PACK_STRUCT_BEGIN

/**
 * @brief   Packed structure modifier (after).
 */
#define PACK_STRUCT_END

#endif /* _CHTYPES_H_ */

/** @} */
