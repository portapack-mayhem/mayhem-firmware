*****************************************************************************
** ChibiOS/RT port for ARM-Cortex-M3 STM32F107.                            **
*****************************************************************************

** TARGET **

The demo runs on an Olimex STM32-P107 board.

** The Demo **

This demo shows how to integrate the FatFs file system and use the SPI and MMC
drivers.
The demo flashes the board LED using a thread and monitors the MMC slot for
a card insertion. When a card is inserted then the file system is mounted
and the LED flashes faster.
A command line shell is spawned on SD3, all the interaction with the demo is
performed using the command shell, type "help" for a list of the available
commands.

** Build Procedure **

The demo has been tested by using the free Codesourcery GCC-based toolchain,
YAGARTO and an experimental WinARM build including GCC 4.3.0.
Just modify the TRGT line in the makefile in order to use different GCC ports.

** Notes **

Some files used by the demo are not part of ChibiOS/RT but are copyright of
ST Microelectronics and are licensed under a different license.
Also note that not all the files present in the ST library are distributed
with ChibiOS/RT, you can find the whole library on the ST web site:

                             http://www.st.com
