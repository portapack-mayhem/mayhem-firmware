*****************************************************************************
** ChibiOS/RT port for ARM7TDMI LPC214X.                                   **
*****************************************************************************

** TARGET **

The demo runs on an Olimex LPC-P2148 board. The port on other boards or other
members of the LPC2000 family should be an easy task.

** The Demo **

The demo blinks the leds on the board by using multiple threads. By pressing
the buttons on the board it is possible to send a message over the serial
port or activate the test procedure.
See main.c for details. Buzzer.c contains an interesting device driver
example that uses a physical timer for the waveform generation and a virtual
timer for the sound duration.

** Build Procedure **

The demo was built using the YAGARTO toolchain but any toolchain based on GCC
and GNU userspace programs will work.
