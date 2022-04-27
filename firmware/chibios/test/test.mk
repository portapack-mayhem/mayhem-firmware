# List of all the ChibiOS/RT test files.
TESTSRC = ${CHIBIOS}/test/test.c \
          ${CHIBIOS}/test/testthd.c \
          ${CHIBIOS}/test/testsem.c \
          ${CHIBIOS}/test/testmtx.c \
          ${CHIBIOS}/test/testmsg.c \
          ${CHIBIOS}/test/testmbox.c \
          ${CHIBIOS}/test/testevt.c \
          ${CHIBIOS}/test/testheap.c \
          ${CHIBIOS}/test/testpools.c \
          ${CHIBIOS}/test/testdyn.c \
          ${CHIBIOS}/test/testqueues.c \
          ${CHIBIOS}/test/testbmk.c

# Required include directories
TESTINC = ${CHIBIOS}/test
