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
 * @file    ARMCMx/crt0.c
 * @brief   Generic ARMvx-M (Cortex-M0/M1/M3/M4) startup file for ChibiOS/RT.
 *
 * @addtogroup ARMCMx_STARTUP
 * @{
 */

#include <stdint.h>

#if !defined(FALSE)
#define FALSE       0
#endif

#if !defined(TRUE)
#define TRUE        (!FALSE)
#endif

#define SCB_CPACR               *((uint32_t *)0xE000ED88U)
#define SCB_FPCCR               *((uint32_t *)0xE000EF34U)
#define SCB_FPDSCR              *((uint32_t *)0xE000EF3CU)
#define FPCCR_ASPEN             (0x1U << 31)
#define FPCCR_LSPEN             (0x1U << 30)

typedef void (*funcp_t)(void);
typedef funcp_t * funcpp_t;

#define SYMVAL(sym) (uint32_t)(((uint8_t *)&(sym)) - ((uint8_t *)0))

/*
 * Area fill code, it is a macro because here functions cannot be called
 * until stacks are initialized.
 */
#define fill32(start, end, filler) {                                        \
  uint32_t *p1 = start;                                                     \
  uint32_t *p2 = end;                                                       \
  while (p1 < p2)                                                           \
    *p1++ = filler;                                                         \
}

/*===========================================================================*/
/**
 * @name    Startup settings
 * @{
 */
/*===========================================================================*/

/**
 * @brief   Control special register initialization value.
 * @details The system is setup to run in privileged mode using the PSP
 *          stack (dual stack mode).
 */
#if !defined(CRT0_CONTROL_INIT) || defined(__DOXYGEN__)
#define CRT0_CONTROL_INIT           0x00000002
#endif

/**
 * @brief   Stack segments initialization switch.
 */
#if !defined(CRT0_STACKS_FILL_PATTERN) || defined(__DOXYGEN__)
#define CRT0_STACKS_FILL_PATTERN    0x55555555
#endif

/**
 * @brief   Stack segments initialization switch.
 */
#if !defined(CRT0_INIT_STACKS) || defined(__DOXYGEN__)
#define CRT0_INIT_STACKS            TRUE
#endif

/**
 * @brief   DATA segment initialization switch.
 */
#if !defined(CRT0_INIT_DATA) || defined(__DOXYGEN__)
#define CRT0_INIT_DATA              TRUE
#endif

/**
 * @brief   BSS segment initialization switch.
 */
#if !defined(CRT0_INIT_BSS) || defined(__DOXYGEN__)
#define CRT0_INIT_BSS               TRUE
#endif

/**
 * @brief   Constructors invocation switch.
 */
#if !defined(CRT0_CALL_CONSTRUCTORS) || defined(__DOXYGEN__)
#define CRT0_CALL_CONSTRUCTORS      TRUE
#endif

/**
 * @brief   Destructors invocation switch.
 */
#if !defined(CRT0_CALL_DESTRUCTORS) || defined(__DOXYGEN__)
#define CRT0_CALL_DESTRUCTORS       TRUE
#endif

/** @} */

/*===========================================================================*/
/**
 * @name    Symbols from the scatter file
 */
/*===========================================================================*/

/**
 * @brief   Main stack lower boundary.
 * @details This symbol must be exported by the linker script and represents
 *          the main stack lower boundary.
 */
extern uint32_t __main_stack_base__;

/**
 *
 * @brief   Main stack initial position.
 * @details This symbol must be exported by the linker script and represents
 *          the main stack initial position.
 */
extern uint32_t __main_stack_end__;

/**
 * @brief   Process stack lower boundary.
 * @details This symbol must be exported by the linker script and represents
 *          the process stack lower boundary.
 */
extern uint32_t __process_stack_base__;

/**
 * @brief   Process stack initial position.
 * @details This symbol must be exported by the linker script and represents
 *          the process stack initial position.
 */
extern uint32_t __process_stack_end__;

/**
 * @brief   ROM image of the data segment start.
 * @pre     The symbol must be aligned to a 32 bits boundary.
 */
extern uint32_t _textdata;

/**
 * @brief   Data segment start.
 * @pre     The symbol must be aligned to a 32 bits boundary.
 */
extern uint32_t _data;

/**
 * @brief   Data segment end.
 * @pre     The symbol must be aligned to a 32 bits boundary.
 */
extern uint32_t _edata;

/**
 * @brief   BSS segment start.
 * @pre     The symbol must be aligned to a 32 bits boundary.
 */
extern uint32_t _bss_start;

/**
 * @brief   BSS segment end.
 * @pre     The symbol must be aligned to a 32 bits boundary.
 */
extern uint32_t _bss_end;

/**
 * @brief   Constructors table start.
 * @pre     The symbol must be aligned to a 32 bits boundary.
 */
extern funcp_t __init_array_start;

/**
 * @brief   Constructors table end.
 * @pre     The symbol must be aligned to a 32 bits boundary.
 */
extern funcp_t __init_array_end;

/**
 * @brief   Destructors table start.
 * @pre     The symbol must be aligned to a 32 bits boundary.
 */
extern funcp_t __fini_array_start;

/**
 * @brief   Destructors table end.
 * @pre     The symbol must be aligned to a 32 bits boundary.
 */
extern funcp_t __fini_array_end;

/** @} */

/**
 * @brief   Application @p main() function.
 */
extern void main(void);

/**
 * @brief   Early initialization.
 * @details This hook is invoked immediately after the stack initialization
 *          and before the DATA and BSS segments initialization. The
 *          default behavior is to do nothing.
 * @note    This function is a weak symbol.
 */
#if !defined(__DOXYGEN__)
__attribute__((weak))
#endif
void __early_init(void) {}

/**
 * @brief   Late initialization.
 * @details This hook is invoked after the DATA and BSS segments
 *          initialization and before any static constructor. The
 *          default behavior is to do nothing.
 * @note    This function is a weak symbol.
 */
#if !defined(__DOXYGEN__)
__attribute__((weak))
#endif
void __late_init(void) {}

/**
 * @brief   Default @p main() function exit handler.
 * @details This handler is invoked or the @p main() function exit. The
 *          default behavior is to enter an infinite loop.
 * @note    This function is a weak symbol.
 */
#if !defined(__DOXYGEN__)
__attribute__((weak, naked))
#endif
void _default_exit(void) {
  while (1)
    ;
}

/**
 * @brief   Reset vector.
 */
#if !defined(__DOXYGEN__)
__attribute__((naked))
#endif
void ResetHandler(void) {
  uint32_t psp, reg;

  /* Process Stack initialization, it is allocated starting from the
     symbol __process_stack_end__ and its lower limit is the symbol
     __process_stack_base__.*/
  asm volatile ("cpsid   i");
  psp = SYMVAL(__process_stack_end__);
  asm volatile ("msr     PSP, %0" : : "r" (psp));

#if CORTEX_USE_FPU
  /* Initializing the FPU context save in lazy mode.*/
  SCB_FPCCR = FPCCR_ASPEN | FPCCR_LSPEN;

  /* CP10 and CP11 set to full access.*/
  SCB_CPACR |= 0x00F00000;

  /* FPSCR and FPDSCR initially zero.*/
  reg = 0;
  asm volatile ("vmsr    FPSCR, %0" : : "r" (reg) : "memory");
  SCB_FPDSCR = reg;

  /* CPU mode initialization, enforced FPCA bit.*/
  reg = CRT0_CONTROL_INIT | 4;
#else
  /* CPU mode initialization.*/
  reg = CRT0_CONTROL_INIT;
#endif
  asm volatile ("msr     CONTROL, %0" : : "r" (reg));
  asm volatile ("isb");

#if CRT0_INIT_STACKS
  /* Main and Process stacks initialization.*/
  fill32(&__main_stack_base__,
         &__main_stack_end__,
         CRT0_STACKS_FILL_PATTERN);
  fill32(&__process_stack_base__,
         &__process_stack_end__,
         CRT0_STACKS_FILL_PATTERN);
#endif

  /* Early initialization hook invocation.*/
  __early_init();

#if CRT0_INIT_DATA
  /* DATA segment initialization.*/
  {
    uint32_t *tp, *dp;

    tp = &_textdata;
    dp = &_data;
    while (dp < &_edata)
      *dp++ = *tp++;
  }
#endif

#if CRT0_INIT_BSS
  /* BSS segment initialization.*/
  fill32(&_bss_start, &_bss_end, 0);
#endif

  /* Late initialization hook invocation.*/
  __late_init();

#if CRT0_CALL_CONSTRUCTORS
  /* Constructors invocation.*/
  {
    funcpp_t fpp = &__init_array_start;
    while (fpp < &__init_array_end) {
      (*fpp)();
      fpp++;
    }
  }
#endif

  /* Invoking application main() function.*/
  main();

#if CRT0_CALL_DESTRUCTORS
  /* Destructors invocation.*/
  {
    funcpp_t fpp = &__fini_array_start;
    while (fpp < &__fini_array_end) {
      (*fpp)();
      fpp++;
    }
  }
#endif

  /* Invoking the exit handler.*/
  _default_exit();
}

/** @} */
