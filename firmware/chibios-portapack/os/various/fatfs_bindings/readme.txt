This directory contains the ChibiOS/RT "official" bindings with the FatFS
library by ChaN: http://elm-chan.org

In order to use FatFS within ChibiOS/RT project, unzip FatFS under
./ext/fatfs then include $(CHIBIOS)/os/various/fatfs_bindings/fatfs.mk
in your makefile.
