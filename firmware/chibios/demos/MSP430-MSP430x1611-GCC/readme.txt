*****************************************************************************
** ChibiOS/RT port for Texas Instruments MSP430.                           **
*****************************************************************************

** TARGET **

The demo runs on an Olimex MSP430-P1611 board with a 8MHz xtal installed. In
order to execute the demo without an crystal you need to edit mcuconf.h
and change:
#define MSP430_USE_CLOCK            MSP430_CLOCK_SOURCE_XT2CLK
in:
#define MSP430_USE_CLOCK            MSP430_CLOCK_SOURCE_DCOCLK

** The Demo **

The demo flashes the board LED using a thread, by pressing the button located
on the board the test procedure is activated with output on the serial port
COM1 (USART0).

** Build Procedure **

The demo was built using the MSPGCC toolchain.

** Notes **

The demo requires include files from MSPGCC that are not part of the ChibiOS/RT
distribution, please install MSPGCC.

                         http://mspgcc.sourceforge.net/
