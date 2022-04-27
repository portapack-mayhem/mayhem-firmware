*****************************************************************************
** ChibiOS/RT port for ARM7TDMI LPC214X using G++.                         **
*****************************************************************************

** TARGET **

The demo runs on an Olimex LPC-P2148 board. The port on other boards or other
members of the LPC2000 family should be an easy task.

** The Demo **

The demo blinks the leds on the board by using multiple threads implemented
as C++ classes. Pressing both buttons activates the test procedure on the
serial port 1.

NOTE: the C++ GNU compiler can produce code sizes comparable to C if you
      don't use RTTI and standard libraries, those are disabled by default
      in the makefile. You can enable them if you have a lot of program space
      available. It is possible to use a lot of C++ features without using
      runtimes, just see the demo.

** Build Procedure **

The demo was built using the YAGARTO toolchain but any toolchain based on GCC
and GNU userspace programs will work.
