*****************************************************************************
** ChibiOS/RT + FatFS demo for SAM7.                                    **
*****************************************************************************

** TARGET **

The demo runs on an Olimex SAM-EX256 board. The port on other boards or other
members of the SAM7 family should be an easy task.

** The Demo **

This demo shows how to integrate the FatFs file system and use the SPI and MMC
drivers.
The demo flashes the board LCD background using a thread and monitors the MMC
slot for a card insertion. When a card is inserted then the file system is
mounted and the LCD background flashes faster.
A command line shell is spawned on SD1, all the interaction with the demo is
performed using the command shell, type "help" for a list of the available
commands.

** Build Procedure **

The demo was built using the YAGARTO toolchain but any toolchain based on GCC
and GNU userspace programs will work.
