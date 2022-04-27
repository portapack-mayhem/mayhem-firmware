*****************************************************************************
** ChibiOS/RT port for ARM7TDMI AT91SAM7X256.                              **
*****************************************************************************

** TARGET **

The demo runs on an Olimex SAM7-EX256 board.

** The Demo **

The demo currently just flashes the LCD background using a thread and serves
HTTP requests at address 192.168.1.20 on port 80 (remember to change it IP
address into webthread.c in order to adapt it to your network settings).
The button SW1 prints an "Hello World!" string on COM1, the button SW2
activates che ChibiOS/RT test suite, output on COM1.

** Build Procedure **

The demo was built using the YAGARTO toolchain but any toolchain based on GCC
and GNU userspace programs will work.
The demo requires the patcher uIP 1.0 stack, see: ./ext/readme.txt

** Notes **

Some files used by the demo are not part of ChibiOS/RT but are Atmel copyright
and are licensed under a different license, see the header present in all the
source files under ./demos/os/hal/platform/AT91SAM7/at91lib for details.
Also note that not all the files present in the Atmel library are distribuited
with ChibiOS/RT, you can find the whole library on the Atmel web site:

                             http://www.atmel.com

The uIP stack also has its own license, please read the info into the included
uIP distribution files.
