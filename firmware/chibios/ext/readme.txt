*****************************************************************************
** External Libraries.                                                     **
*****************************************************************************

The libraries under this directory are not part of ChibiOS/RT but are used by
some of the demo applications.
Please note that each item is covered by its own license, please read the
instructions contained in the various distributions.

The currently included items are:

1. uip-1.0, a minimal TCP/IP implementation: http://www.sics.se/~adam/uip/
2. lwip-1.4.0, lightweight TCP/IP stack: http://savannah.nongnu.org/projects/lwip/
3. FatFS 0.9 (patched), the original version is available from
   http://elm-chan.org/fsw/ff/00index_e.html

The above files are included packed as downloaded from the original repository
and without any modification, in order to use the libraries unpack them
under ./ext as:

./ext/uip-1.0
./ext/lwip
./ext/fatfs
./ext/stm32lib (you also need to copy stm32f10x_conf.h in your project)

Some patches are also present:

1. uip-1.0 patches, small fixes to the uIP required to make it work with
   ChibiOS/RT, unpack the archive over the uIP distribution and replace the
   files.
