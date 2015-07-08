*****************************************************************************
** ChibiOS/RT port for x86 into a Linux process                            **
*****************************************************************************

** TARGET **

The demo runs under x86 Linux as an application program. The serial
I/O is simulated over TCP/IP sockets.

** The Demo **

The demo listens on the two serial ports, when a connection is detected a
thread is started that serves a small command shell.
The demo shows how to create/terminate threads at runtime, how to listen to
events, how to work with serial ports, how to use the messages.
You can develop your ChibiOS/RT application using this demo as a simulator
then you can recompile it for a different architecture.
See demo.c for details.

** Build Procedure **

GCC required.  The Makefile defaults to building for a Linux host.
To build on OS X, use the following command: `make HOST_OSX=yes`

** Connect to the demo **

In order to connect to the demo use telnet on the listening ports.
