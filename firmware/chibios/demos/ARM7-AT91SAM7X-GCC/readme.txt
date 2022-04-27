*****************************************************************************
** ChibiOS/RT port for ARM7TDMI AT91SAM7X256.                              **
*****************************************************************************

** TARGET **

The demo runs on an Olimex SAM7-EX256 board.

** The Demo **

The demo currently just flashes the LCD background using a thread.
The button SW1 prints an "Hello World!" string on COM1, the button SW2
activates che ChibiOS/RT test suite, output on COM1.

** Build Procedure **

The demo was built using the YAGARTO toolchain but any toolchain based on GCC
and GNU userspace programs will work.

** Notes **

Some files used by the demo are not part of ChibiOS/RT but are Atmel copyright
and are licensed under a different license, see the header present in all the
source files under ./demos/os/hal/platform/AT91SAM7/at91lib for details.
Also note that not all the files present in the Atmel library are distribuited
with ChibiOS/RT, you can find the whole library on the Atmel web site:

                             http://www.atmel.com
