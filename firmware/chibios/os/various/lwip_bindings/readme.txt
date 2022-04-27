This directory contains the ChibiOS/RT "official" bindings with the lwIP
TCP/IP stack: http://savannah.nongnu.org/projects/lwip

In order to use FatFS within ChibiOS/RT project, unzip FatFS under
./ext/lwip-1.4.0 then include $(CHIBIOS)/os/various/lwip_bindings/lwip.mk
in your makefile.
