# FATFS files.
set(FATFSSRC
	${CHIBIOS_PORTAPACK}/os/various/fatfs_bindings/fatfs_diskio.c
	${CHIBIOS_PORTAPACK}/os/various/fatfs_bindings/fatfs_syscall.c
	${CHIBIOS_PORTAPACK}/ext/fatfs/src/ff.c
	${CHIBIOS_PORTAPACK}/ext/fatfs/src/option/unicode.c
)

set(FATFSINC
	${CHIBIOS_PORTAPACK}/ext/fatfs/src
)
