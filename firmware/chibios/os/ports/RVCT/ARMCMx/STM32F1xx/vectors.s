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

#if !defined(STM32F10X_LD) && !defined(STM32F10X_LD_VL) &&                  \
    !defined(STM32F10X_MD) && !defined(STM32F10X_MD_VL) &&                  \
    !defined(STM32F10X_HD) && !defined(STM32F10X_XL) &&                     \
    !defined(STM32F10X_CL)
#define _FROM_ASM_
#include "board.h"
#endif

                PRESERVE8

                AREA    RESET, DATA, READONLY

                IMPORT  __initial_msp
                IMPORT  Reset_Handler
                EXPORT  __Vectors

__Vectors
                DCD     __initial_msp
                DCD     Reset_Handler
                DCD     NMIVector
                DCD     HardFaultVector
                DCD     MemManageVector
                DCD     BusFaultVector
                DCD     UsageFaultVector
                DCD     Vector1C
                DCD     Vector20
                DCD     Vector24
                DCD     Vector28
                DCD     SVCallVector
                DCD     DebugMonitorVector
                DCD     Vector34
                DCD     PendSVVector
                DCD     SysTickVector
                DCD     Vector40
                DCD     Vector44
                DCD     Vector48
                DCD     Vector4C
                DCD     Vector50
                DCD     Vector54
                DCD     Vector58
                DCD     Vector5C
                DCD     Vector60
                DCD     Vector64
                DCD     Vector68
                DCD     Vector6C
                DCD     Vector70
                DCD     Vector74
                DCD     Vector78
                DCD     Vector7C
                DCD     Vector80
                DCD     Vector84
                DCD     Vector88
                DCD     Vector8C
                DCD     Vector90
                DCD     Vector94
                DCD     Vector98
                DCD     Vector9C
                DCD     VectorA0
                DCD     VectorA4
                DCD     VectorA8
                DCD     VectorAC
                DCD     VectorB0
                DCD     VectorB4
                DCD     VectorB8
                DCD     VectorBC
                DCD     VectorC0
                DCD     VectorC4
                DCD     VectorC8
                DCD     VectorCC
                DCD     VectorD0
                DCD     VectorD4
                DCD     VectorD8
                DCD     VectorDC
                DCD     VectorE0
                DCD     VectorE4
                DCD     VectorE8
#if defined(STM32F10X_MD_VL) || defined(STM32F10X_HD) ||                    \
    defined(STM32F10X_XL)    || defined(STM32F10X_CL)
                DCD     VectorEC
                DCD     VectorF0
                DCD     VectorF4
#endif
#if defined(STM32F10X_HD)    || defined(STM32F10X_XL) || defined(STM32F10X_CL)
                DCD     VectorF8
                DCD     VectorFC
                DCD     Vector100
                DCD     Vector104
                DCD     Vector108
                DCD     Vector10C
                DCD     Vector110
                DCD     Vector114
                DCD     Vector118
                DCD     Vector11C
                DCD     Vector120
                DCD     Vector124
                DCD     Vector128
                DCD     Vector12C
#endif
#if defined(STM32F10X_CL)
                DCD     Vector130
                DCD     Vector134
                DCD     Vector138
                DCD     Vector13C
                DCD     Vector140
                DCD     Vector144
                DCD     Vector148
                DCD     Vector14C
#endif

                AREA    |.text|, CODE, READONLY
                THUMB

/*
 * Default interrupt handlers.
 */
                EXPORT  _unhandled_exception
_unhandled_exception PROC 
                EXPORT  NMIVector               [WEAK]
                EXPORT  HardFaultVector         [WEAK]
                EXPORT  MemManageVector         [WEAK]
                EXPORT  BusFaultVector          [WEAK]
                EXPORT  UsageFaultVector        [WEAK]
                EXPORT  Vector1C                [WEAK]
                EXPORT  Vector20                [WEAK]
                EXPORT  Vector24                [WEAK]
                EXPORT  Vector28                [WEAK]
                EXPORT  SVCallVector            [WEAK]
                EXPORT  DebugMonitorVector      [WEAK]
                EXPORT  Vector34                [WEAK]
                EXPORT  PendSVVector            [WEAK]
                EXPORT  SysTickVector           [WEAK]
                EXPORT  Vector40                [WEAK]
                EXPORT  Vector44                [WEAK]
                EXPORT  Vector48                [WEAK]
                EXPORT  Vector4C                [WEAK]
                EXPORT  Vector50                [WEAK]
                EXPORT  Vector54                [WEAK]
                EXPORT  Vector58                [WEAK]
                EXPORT  Vector5C                [WEAK]
                EXPORT  Vector60                [WEAK]
                EXPORT  Vector64                [WEAK]
                EXPORT  Vector68                [WEAK]
                EXPORT  Vector6C                [WEAK]
                EXPORT  Vector70                [WEAK]
                EXPORT  Vector74                [WEAK]
                EXPORT  Vector78                [WEAK]
                EXPORT  Vector7C                [WEAK]
                EXPORT  Vector80                [WEAK]
                EXPORT  Vector84                [WEAK]
                EXPORT  Vector88                [WEAK]
                EXPORT  Vector8C                [WEAK]
                EXPORT  Vector90                [WEAK]
                EXPORT  Vector94                [WEAK]
                EXPORT  Vector98                [WEAK]
                EXPORT  Vector9C                [WEAK]
                EXPORT  VectorA0                [WEAK]
                EXPORT  VectorA4                [WEAK]
                EXPORT  VectorA8                [WEAK]
                EXPORT  VectorAC                [WEAK]
                EXPORT  VectorB0                [WEAK]
                EXPORT  VectorB4                [WEAK]
                EXPORT  VectorB8                [WEAK]
                EXPORT  VectorBC                [WEAK]
                EXPORT  VectorC0                [WEAK]
                EXPORT  VectorC4                [WEAK]
                EXPORT  VectorC8                [WEAK]
                EXPORT  VectorCC                [WEAK]
                EXPORT  VectorD0                [WEAK]
                EXPORT  VectorD4                [WEAK]
                EXPORT  VectorD8                [WEAK]
                EXPORT  VectorDC                [WEAK]
                EXPORT  VectorE0                [WEAK]
                EXPORT  VectorE4                [WEAK]
                EXPORT  VectorE8                [WEAK]
                EXPORT  VectorEC                [WEAK]
                EXPORT  VectorF0                [WEAK]
                EXPORT  VectorF4                [WEAK]
                EXPORT  VectorF8                [WEAK]
                EXPORT  VectorFC                [WEAK]
                EXPORT  Vector100               [WEAK]
                EXPORT  Vector104               [WEAK]
                EXPORT  Vector108               [WEAK]
                EXPORT  Vector10C               [WEAK]
                EXPORT  Vector110               [WEAK]
                EXPORT  Vector114               [WEAK]
                EXPORT  Vector118               [WEAK]
                EXPORT  Vector11C               [WEAK]
                EXPORT  Vector120               [WEAK]
                EXPORT  Vector124               [WEAK]
                EXPORT  Vector128               [WEAK]
                EXPORT  Vector12C               [WEAK]
                EXPORT  Vector130               [WEAK]
                EXPORT  Vector134               [WEAK]
                EXPORT  Vector138               [WEAK]
                EXPORT  Vector13C               [WEAK]
                EXPORT  Vector140               [WEAK]
                EXPORT  Vector144               [WEAK]
                EXPORT  Vector148               [WEAK]
                EXPORT  Vector14C               [WEAK]

NMIVector
HardFaultVector
MemManageVector
BusFaultVector
UsageFaultVector
Vector1C
Vector20
Vector24
Vector28
SVCallVector
DebugMonitorVector
Vector34
PendSVVector
SysTickVector
Vector40
Vector44
Vector48
Vector4C
Vector50
Vector54
Vector58
Vector5C
Vector60
Vector64
Vector68
Vector6C
Vector70
Vector74
Vector78
Vector7C
Vector80
Vector84
Vector88
Vector8C
Vector90
Vector94
Vector98
Vector9C
VectorA0
VectorA4
VectorA8
VectorAC
VectorB0
VectorB4
VectorB8
VectorBC
VectorC0
VectorC4
VectorC8
VectorCC
VectorD0
VectorD4
VectorD8
VectorDC
VectorE0
VectorE4
VectorE8
VectorEC
VectorF0
VectorF4
VectorF8
VectorFC
Vector100
Vector104
Vector108
Vector10C
Vector110
Vector114
Vector118
Vector11C
Vector120
Vector124
Vector128
Vector12C
Vector130
Vector134
Vector138
Vector13C
Vector140
Vector144
Vector148
Vector14C
                b       _unhandled_exception
                ENDP

                END
