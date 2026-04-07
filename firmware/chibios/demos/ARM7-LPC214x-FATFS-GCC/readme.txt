*****************************************************************************
** ChibiOS/RT + FatFS demo for LPC214x.                                    **
*****************************************************************************

** TARGET **

The demo runs on an Olimex LPC-P2148 board. The port on other boards or other
members of the LPC2000 family should be an easy task.

** The Demo **

The demo blinks the leds on the board by using multiple threads.
By pressing button 1 a directory scan on the MMC slot is performed, by
pressing the button 2 the test suite is activated on serial port 1.

** Build Procedure **

The demo was built using the YAGARTO toolchain but any toolchain based on GCC
and GNU userspace programs will work.
