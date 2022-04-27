*****************************************************************************
** ChibiOS/RT HAL - IRQ_STORM_FPU stress test demo for STM32F4xx.          **
*****************************************************************************

** TARGET **

The demo runs on an STMicroelectronics STM32F4-Discovery board.

** The Demo **

The application demonstrates the use of the STM32F4xx GPT, PAL and Serial
drivers in order to implement a system stress demo involving the FPU.

** Board Setup **

None.

** Build Procedure **

The demo has been tested using YAGARTO 4.6.2.
Just modify the TRGT line in the makefile in order to use different GCC ports.

** Notes **

Some files used by the demo are not part of ChibiOS/RT but are copyright of
ST Microelectronics and are licensed under a different license.
Also note that not all the files present in the ST library are distributed
with ChibiOS/RT, you can find the whole library on the ST web site:

                             http://www.st.com
