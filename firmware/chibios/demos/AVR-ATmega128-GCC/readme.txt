*****************************************************************************
** ChibiOS/RT port for Atmel AVR ATmega128.                                **
*****************************************************************************

** TARGET **

The demo runs on an Olimex AVR-MT-128 board.

** The Demo **

The demo currently just writes a hello world on the LCD and toggles the relay
using a thread while button 2 is pressed.
By pressing the button 1 the test suite is activated, output on serial port 2.

** Build Procedure **

The demo was built using the WinAVR toolchain.

** Notes **

The demo requires include files from WinAVR that are not part of the ChibiOS/RT
distribution, please install WinAVR.

                         http://winavr.sourceforge.net/
