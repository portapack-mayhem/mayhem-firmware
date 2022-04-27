*****************************************************************************
** ChibiOS/RT port for ARM-Cortex-M3 STM32F100xB.                          **
*****************************************************************************

** TARGET **

The demo runs on an ST STM32VL-Discovery board.

** The Demo **

The demo shows how to use the ADC, PWM and SPI drivers using asynchronous
APIs. The ADC samples two channels (temperature sensor and PC0) and modulates
the PWM using the sampled values. The sample data is also transmitted using
the SPI port 1.
By pressing the button located on the board the test procedure is activated
with output on the serial port COM1 (USART1).

** Build Procedure **

The demo has been tested by using the free Codesourcery GCC-based toolchain
and YAGARTO. just modify the TRGT line in the makefile in order to use
different GCC toolchains.

** Notes **

Some files used by the demo are not part of ChibiOS/RT but are copyright of
ST Microelectronics and are licensed under a different license.
Also note that not all the files present in the ST library are distributed
with ChibiOS/RT, you can find the whole library on the ST web site:

                             http://www.st.com
