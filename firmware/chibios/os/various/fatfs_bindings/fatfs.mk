# FATFS files.
FATFSSRC = ${CHIBIOS}/os/various/fatfs_bindings/fatfs_diskio.c \
           ${CHIBIOS}/os/various/fatfs_bindings/fatfs_syscall.c \
           ${CHIBIOS}/ext/fatfs/src/ff.c \
           ${CHIBIOS}/ext/fatfs/src/option/unicode.c

FATFSINC = ${CHIBIOS}/ext/fatfs/src
