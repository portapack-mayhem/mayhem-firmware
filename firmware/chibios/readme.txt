*****************************************************************************
*** Files Organization                                                    ***
*****************************************************************************

--{root}                - ChibiOS/RT directory.
  +--readme.txt         - This file.
  +--documentation.html - Shortcut to the web documentation page.
  +--todo.txt           - Current plan (development/unstable versions only).
  +--license.txt        - GPL license text.
  +--exception.txt      - GPL exception text (stable releases only).
  +--boards/            - Board support files.
  +--demos/             - Demo projects.
  +--docs/              - Documentation.
  |  +--html/           - Local HTML documentation (after rebuild).
  |  +--reports/        - Test reports.
  |  +--src/            - Documentation source files (required for rebuild).
  |  +--rsc/            - Documentation resource files (required for rebuild).
  |  +--Doxyfile        - Doxygen project file (required for rebuild).
  |  +--index.html      - Local documentation access (after rebuild).
  +--ext/               - External libraries, not part of ChibiOS/RT.
  +--os/                - ChibiOS/RT files.
  |  +--hal/            - Hardware Abstraction Layer.
  |  |  +--include/     - HAL high level headers.
  |  |  +--src/         - HAL high level source.
  |  |  +--platforms/   - HAL low level drivers implementations.
  |  |  |  +--AT91SAM7/ - Drivers for AT91SAM7 platform.
  |  |  |  +--AVR/      - Drivers for AVR platform.
  |  |  |  +--LPC11xx/  - Drivers for LPC11xx platform.
  |  |  |  +--LPC13xx/  - Drivers for LPC13xx platform.
  |  |  |  +--LPC214x/  - Drivers for LPC214x platform.
  |  |  |  +--MSP430/   - Drivers for MSP430 platform.
  |  |  |  +--SPC56x/   - Drivers for SPC56x/MPC563xx platforms.
  |  |  |  +--STM32/    - Drivers for STM32 platform (common).
  |  |  |  +--STM32F1xx/- Drivers for STM32F1xx platform.
  |  |  |  +--STM32F2xx/- Drivers for STM32F2xx platform.
  |  |  |  +--STM32F4xx/- Drivers for STM32F4xx platform.
  |  |  |  +--STM32L1xx/- Drivers for STM32L1xx platform.
  |  |  |  +--Posix/    - Drivers for x86 Linux/OSX simulator platform.
  |  |  |  +--Win32/    - Drivers for x86 Win32 simulator platform.
  |  |  +--templates/   - Driver template files.
  |  |     +--meta/     - Driver meta templates.
  |  +--ports/          - Port files for the various architectures.
  |  |  +--GCC/         - Ports for the GCC compiler.
  |  |  |  +--ARM/      - Port files for ARM7 and ARM9 architectures.
  |  |  |  +--ARMCMx/   - Port files for ARMCMx architectures (ARMv6/7-M).
  |  |  |  +--PPC/      - Port files for PowerPC architecture.
  |  |  |  +--AVR/      - Port files for AVR architecture.
  |  |  |  +--MSP430/   - Port files for MSP430 architecture.
  |  |  |  +--SIMIA32/  - Port files for SIMIA32 simulator architecture.
  |  |  +--IAR/         - Ports for the IAR compiler.
  |  |  |  +--ARMCMx/   - Port files for ARMCMx architectures (ARMv6/7-M).
  |  |  +--RVCT/        - Ports for the Keil RVCT compiler.
  |  |  |  +--ARMCMx/   - Port files for ARMCMx architectures (ARMv6/7-M).
  |  +--kernel/         - Kernel portable files.
  |  |  +--include/     - Kernel headers.
  |  |  +--src/         - Kernel source.
  |  |  +--templates/   - Kernel port template files.
  |  +--various/        - Various portable support files.
  +--test/              - Kernel test suite source code.
  |  +--coverage/       - Code coverage project.
  +--testhal/           - HAL integration test demos.
  |  +--LPC11xx/        - LPC11xx HAL demos.
  |  +--LPC13xx/        - LPC11xx HAL demos.
  |  +--STM32F1xx/      - STM32F1xx HAL demos.
  |  +--STM32F4xx/      - STM32F4xx HAL demos (valid for STM32F2xx too).
  |  +--STM32L1xx/      - STM32L1xx HAL demos.
  +--tools              - Various tools.
     +--eclipse         - Eclipse enhancements.

*****************************************************************************
*** Releases                                                              ***
*****************************************************************************

*** 2.6.8 ***
- FIX: Fixed spurious TC interrupt in STM32 UART (v1 and v2) driver (bug #584).
- FIX: Fixed invalid checks on STM32L1xx LSI and LSE clocks (bug #583).
- FIX: Fixed RCC CAN2 macros missing in STM32F1xx platform (bug #582).
- FIX: Fixed misplaced __set_BASEPRI() in STM32 OTGv1 driver (bug #566).
- FIX: Fixed typo in code doc of chOQWriteTimeout function (bug #565).
- FIX: Fixed wrong condition check in MMC_SPI driver (bug #564).
- FIX: Fixed wrong boolean operators in mutexes module (bug #563).
- FIX: Fixed missing macro in STM32 USBv1 and OTGv1 drivers (bug #562).
- FIX: Fixed wrong paths for OLIMEX_STM32-E407_REV_D board (bug #561).
- FIX: Fixed wrong TIM1/8_CLK on STM32F30x when using PCLK2 with prescaler
  (bug #557).
- FIX: Fixed STM32 USARTv1: serial interrupt hang on overrun (and other) error
  (bug #558).

*** 2.6.7 ***
- FIX: Fixed random R0 register corruption in Keil ARMCMx port when FPU
  is enabled (bug #556).
- FIX: Fixed wrong CORTEX_PRIORITY_PENDSV value in ARMCMx ports (bug #555).
- FIX: Safer ADC start for STM32F4 and STM32L1.
- FIX: AT91SAM7 SPI1 undocumented errata on manual CS manipulation.
- FIX: AT91SAM7 SPI set CS pin mode.
- FIX: Fixed AT91SAM7 gpio modes.
- FIX: AT91SAM7 i2c and spi driver using wrong pins for many cpu variants
- FIX: Fixed STM32 USB driver randomly unable to transition to VALID state
  (bug #554).
- FIX: Fixed OLIMEX STM32-E407 board.h errors (FAULT input swap and wrong
  BUSON initial output state) (bug #551).
- FIX: Fixed STM32F3xx clock init fails if PLL is enabled at startup
  (bug #550).
- FIX: Fixed problem in GCC scatter files (bug #548).
- FIX: Fixed kernel function chEvtWaitOne() malfunctioning (bug #547).
- FIX: Fixed minor warnings while building with IAR 7.3.0 (bug #542).
- FIX: Fixed STM32 OTGv1 usb_lld_pump improper stack filling (bug #541).
- FIX: Fixed chsnprintf sometimes doesn't terminate str (bug #538).
- FIX: Fixed wrong wait states for STM32F401 (bug #537).
- FIX: Fixed failure to compile EXT driver on STM32F401, re-opened
  ticket (bug #517).

*** 2.6.6 ***
- FIX: Fixed error in STM32F30x adc_lld_stop() (bug #535).
- FIX: Fixed STM32 DIER register setting in PWM and ICU drivers (bug #534).
- FIX: Fixed ARMCM4 FPU exception randomly triggered (bug #533).
- FIX: Fixed control transfers larger than 127 bytes don't work (bug #531).
- FIX: Fixed STM32F0xx ADC driver enforces continuous mode (bug #528).
       Note, this bug enforced a change, now the bit ADC_CFGR1_CONT must be
       manually specified in the cfgr1 field of the ADCConversionGroup
       structure when applicable.
- FIX: Fixed STM32F30x ADC driver enforces continuous mode (bug #527).
       Note, this bug enforced a change, now the bit ADC_CFGR_CONT must be
       manually specified in the cfgr field of the ADCConversionGroup
       structure when applicable.
- FIX: Fixed double clock mode for TIM1/TIM8 on STM32F30x fails (bug #525).
- FIX: Fixed SDC initialization error with V1.1 cards (bug #523).
- FIX: Fixed Race condition in STM32 SDC driver (bug #522).
- FIX: Fixed failure to compile EXT driver on STM32F401 (bug #517).
- FIX: Fixed wrong DMA channels for STM32L1 I2C1 unit (bug #516).
- FIX: Fixed EXT driver compile error on STM32F030 (bug #514).

*** 2.6.5 ***
- FIX: Fixed race condition in Cortex-M4 port with FPU and fast interrupts
  (bug #513).
- FIX: Fixed STM32F1xx warning in stm32_dma.c (bug #512).
- FIX: Fixed invalid checks in canSTM32SetFilters() function (bug #511).
- FIX: Fixed missing check when disabling STM32F1 shared DMA IRQs (bug #510).
- FIX: Fixed STM32L1 Medium Density Plus RTC Subseconds (bug #509).
- FIX: Fixed stm32 CCM .ld file needs NOLOAD (bug #506).
- FIX: Fixed dereference possibly null pointer before checking for nulliness
  in STM32 RTCv2 driver (bug #505).
- FIX: Fixed race condition in STM32 (F1, F2, F4, L1) serial driver
  implementation (bug #503).
- FIX: Fixed missing make dependencies for asm files (bug #501).
- FIX: STM32L1 Plus Clock and I2C (bug #495).
- NEW: Added support for STMicroelectronics STEVAL-MKI121V1 also known as
  INEMO-M1 Discovery board. A simple demo application using USB has been
  added.
- NEW: Added support for STMicroelectronics NUCLEO-F103RB board.
- NEW: Added support for STMicroelectronics NUCLEO-F401RE board.
- NEW: Added support for STMicroelectronics NUCLEO-F030R8 board.
- NEW: Added support for STMicroelectronics NUCLEO-L152RE board (not tested
  because lack of support in OpenOCD).
- CHANGE: Made mii_read() and mii_write() public in the STM32 MAC driver.

*** 2.6.4 ***
- FIX: Fixed insufficient ISR-reserved stack in ARMCMx port when
  optimizations are disabled (bug #494).
- FIX: Fixed configuration descriptors larger than 127 bytes don't
  work (bug #373).
- FIX: Fixed invalid cast in PWM_FRACTION_TO_WIDTH() macro (bug #487).
- FIX: Fixed wrong STM32 TIM9 clock source in PWM and ICU drivers (bug #486).
- FIX: Fixed MMC_SPI driver block_addresses is not initialized after
  reconnection (bug #485).
- FIX: Fixed STM32L1 Plus Compilation Problems (bug #484).
- FIX: Fixed OTG HS failure when WFI instruction is enabled (bug #482).
- FIX: Fixed wrong STM32F4 TIM6 vector number symbol (bug #480).
- FIX: Fixed problem in STM32 SDADC driver initialization (bug #479).
- FIX: Fixed chThdShouldTerminate() documentation incorrect (bug #478).
- FIX: Fixed spurious callback in STM32 EXT driver (bug #477).
- FIX: Fixed several macro errors in STM32L1xx HAL driver (bug #476).
- FIX: Fixed wrong STM32 RTCv2 alarms implementation (bug #475).
- FIX: Fixed wrong ADC34 macros in STM32F30x HAL driver (bug #474).
- FIX: Fixed wrong TIM1 and TIM8 macros in STM32F30x HAL driver (bug #473).
- FIX: Fixed chprintf()/chSequentialStreamWrite() crash with size of 0
  or NULL (bug #472).
- FIX: Fixed simulated IO message is corrupted in simulator (bug #468).
- FIX: Fixed typo in STMxx demo makefiles (bug #466).
- FIX: Fixed wrong multilib handling in ChibiOS buildsystem (bug #465).
- NEW: Improved makefiles backported from the 3.0 branch, make sure to use
  Makefiles taken from this version in your projects.
- CHANGE: Made optional the STM32 MAC DMABMR SR reset procedure.

*** 2.6.3 ***
- FIX: Fixed TM32 SDC driver clock activation issue (bug #464).
- FIX: Fixed spurious callback in ICU driver (bug #461).
- FIX: Fixed compile error in STM32F0xx ADC driver when STM32F0XX_LD devices
  are selected (bug #460).
- FIX: Fixed race condition in STM32 SDC driver (bug #458).
- FIX: Fixed race condition in STM32 OTG driver (bug #457).
- FIX: Fixed switch to LAN8710A PHY for STM32-E407 boards (bug #456).
- FIX: Fixed add PHY id of LAN8710A (bug #455).
- FIX: Fixed memstreams.c missing from simulator makefiles (bug #454).
- FIX: Fixed chprintf() does not compile in strict C90 mode (bug #453).
- FIX: Fixed corrupted mcuconf.h file in ARMCM4-STM32F407-DISCOVERY demo
  (bug #452).

*** 2.6.2 ***
- FIX: Fixed wrong vector names for STM32Lxx.
- FIX: Fixed wrong STM32_TIM_CCMR2_OCxM macros on STM32F30x (bug #449).
- FIX: Fixed STM32F30x TIM1/TIM8 alternate clock source setting not
  recognized (bug #448).
- FIX: Fixed wrong MCO2 check in STM32F4xx HAL driver (bug #447).
- FIX: Fixed spurious half buffer callback in STM32 ADC drivers (bug #446).
- FIX: Fixed callbacks changes to the ADC high level driver (bug #445).
- FIX: Fixed wrong definition in STM32F37x ADC driver (bug #444).
- FIX: Fixed wrong CORTEX_PRIORITY_PENDSV value (bug #443).
- FIX: Fixed lost incoming characters in STM32 USARTv1 driver (bug #442).
- FIX: Fixed STM32 OTG-FS wrong upper memory limit (bug #437).
- FIX: Fixed race condition in STM32 DMA interrupt (bug #439).
- FIX: Fixed timing issue in the STM32 OTGv1 USB driver (bug #436).
- FIX: Fixed STM32L1 remove reset flag (bug #435).
- FIX: Fixed unaligned data access in USB LLD (bug #434).
- FIX: Fixed add RTC to STM32L1 (bug #433).
- FIX: Fixed support 10-bit addresses in STM32 I2C driver (bug #432).
- FIX: Fixed duplicate STM32_GPT_USE_TIM8 definition in some mcuconf.h files
  (bug #431).
- FIX: Fixed possible unalignment in GCC Cortex-M scatter files (bug #430).
- NEW: Added support for STM32F030xx/050xx/060xx devices.
- NEW: Added BOARD_OTG_NOVBUSSENS board option for STM32 OTG.
- NEW: Added SPI4/SPI5/SPI6 support to the STM32v1 SPIv1 low level driver.
- NEW: Added chvprintf() and chsnprintf() functions to the chprintf module.
- NEW: Added a new function shellExit() to the shell. It allows to exit the
  shell from any command handler.
- NEW: Added support for STM32F401/STM32F42x/STM32F43x devices.
- NEW: Improved time range check in the kernel, new API chTimeElapsedSince()
  introduced. The API chTimeIsWithin() is now a macro.
- NEW: Added support for STM32F0xx platform in RTCv2 driver.
- NEW: Improvements to the STM32F4xx backup domain initialization.
- NEW: Added support for STM32F4xx backup RAM.
- NEW: Added support of UART4 and UART5 (STM32F4xx and STM32F2xx platforms)
  (feature request #28).

*** 2.6.1 ***
- FIX: Fixed PAL driver documentation error (bug #427).
- FIX: Fixed UART4 and 5 marked as not present in STM32F30x devices (bug #426).
- FIX: Fixed warning in STM32 ICU/PWM drivers when used on STM32F3xx (bug
  #425).
- FIX: Fixed conditional code error in STM32 PWM driver (bug #424).
- FIX: Fixed error in Guards of pwm_lld.h from STM32 (bug #423).
- FIX: Fixed wrong RTC macro names in STM32L1xx HAL (bug #422).
- FIX: Fixed CodeSourcery personal version fails to build with undefined
  errno_r (bug #421).
- FIX: Fixed FSMC reset on STM32F4xx (bug #420).
- FIX: Fixed invalid directory links in the demo files (bug #419).
- FIX: Fixed missing casts in time-conversion macros (bug #418).
- FIX: Fixed PLL2 activation condition is wrong in STM32F107 HAL (bug #417).
- NEW: Improvements to the STM32F4xx backup domain initialization.
- NEW: Added initializer for the DIER register to the STM32 GPT, ICU and
  PWM drivers.
- NEW: Added support for 32bits counters to the STM32 GPT driver.
- NEW: Added port support for SCP560B64.
- CHANGE: Moved the STM32 GPT, ICU and PWM low level drivers under
  ./os/hal/platform/STM32/TIMv1. Updated all the impacted project files.

*** 2.6.0 ***
- FIX: Fixed  MS2ST() and US2ST() macros error (bug #415).
- NEW: Added new pwmIsChannelEnabledI() API to the PWM driver, implemented
  in the STM32 driver.
- NEW: Added support for timers 6, 7, 9, 11, 12, 14 to the STM32 GPT driver.
- NEW: Added support for timer 9 to the STM32 PWM and ICU drivers.
- NEW: Relicensed parts of the distribution tree under the Apache 2.0
  license in order to make specific parts of the code more accessible
  to the open source community and adopters.
- NEW: Added ADC(EQADC), HAL, ICU, PAL, PWM, Serial drivers for SPC5xx
  platforms, tests to be added on the various sub-families.
- NEW: Added support for SPC56ELxx, SPC560BCxx, SPC560Pxx, SPC560Mxx and
  SPC564Axx platforms.
- NEW: Now the general documentation includes data extracted from the low
  level driver templates. Per-platform/architecture documents are no more
  required and will be replaced with technical articles and examples for
  each specific driver.
- NEW: Added a build test project for low level device driver templates.
- NEW: Enhanced CAN driver model, support for mailboxes has been added. STM32
  driver implementation upgraded.
- NEW: Added ADC and PWM drivers for the AT91SAM7 platform, both donated
  by Andrew Hannam.
- NEW: Added kernel support for the SAM4L, an Atmel Studio 6 demo for the
  SAM4L-EK board has been added.
- NEW: CAN2 support for STM32 added.
- NEW: Updated STM32L1xx header to the latest version.
- NEW: Added an option to lwipthread to change the link status poll interval.
- NEW: Added new C++ demo for the STM32F4-Discovery.
- NEW: Updated C++ wrapper with a much more logical classes structure.
- NEW: ADC driver implementation for the STM32F3xx, the driver supports also
  the dual-ADC mode allowing for a very high combined bandwidth.
- NEW: Added zero-copy capability to the STM32 MAC driver (experimental and
  not tested yet).
- NEW: Added an optional zero-copy mode API to the MAC driver model.
- NEW: Added EXT driver to the STM32F3xx platform.
- NEW: Improved the STM32 EXT driver to support more than 32 channels.
- NEW: Added support for Olimex board STM32-LCD.
- NEW: Support for STM32F30x and STM32F37x.
- NEW: AT91SAM7A3 support.
- NEW: Unified the STM32F4xx and STM32F2xx platform code. The STM32F2xx now is
  only supported as an STM32F4xx variant and not tested separately.
- NEW: Updated STM32F1, F2, F4, L1 ADC drivers to allow HW triggering.
- NEW: Added a new option STM32_ETH1_CHANGE_PHY_STATE to the STM32 MAC driver,
  this change is connected to bug 3570335.
- NEW: Modified the CAN drivers to use the new event flags mechanism, the
  previous flags handling has been removed.
- NEW: Modified serial and serial_usb drivers to use the new event flags
  mechanism, the previous flags handling in BaseAsynchronousChannel has
  been removed.
- NEW: Improved the kernel events subsystem, now event sources can associate
  source-specific flags to the listener, the flags can then be retrieved
  using the new APIs chEvtGetAndClearFlags() and chEvtGetAndClearFlagsI().
  Some old APIs have been renamed to increase consistency of the module.
- NEW: Added VLE support to the Power Architecture GCC port.
- NEW: Reorganized the Power Architecture GCC port along the lines of the
  ARMCMx port, now it can support multiple core types.
- NEW: Updated the Power Architecture rules.mk file to put object and listing
  files into a ./build directory like ARM ports already do.
- NEW: Added Eclipse project files to most demos. The project are setup to
  have paths relative to a variable named CHIBIOS that must point to the
  ChibiOS/RT installation path. The variable must be defined under
  Window->Preferences->General->Workspace->Linked_Resources and must contain
  a path without the trailing slash character.
- NEW: Added memory signature record to the registry in order to simplify
  the implementation of ad-hoc debuggers.
- NEW: Small andjustment in chcore.h files under ./os/ports/GCC required by a
  difference in GCC 4.7.x.
- NEW: Added another STM32F4-Discovery demo using the on-board MEMS, SPI
  and PWM. Removed MEMS handling from the old demo because code size limits
  on non-free compilers.
- NEW: Added USART6 support to the STM32 UARTv1 driver, contributed by Erik
  van der Zalm.
- NEW: Added demo for Arduino Mega, contributed by Fabio Utzig.
- NEW: Added support for ATmega1280, contributed by Fabio Utzig.
- NEW: Added I2C driver for AVR, contributed by Fabio Utzig.
- NEW: Added FatFs demo for the Olimex STM32-P107 board.
- NEW: Added support for the Olimex STM32-E407 board. Added an integrated
  demo including USB-CDC, lwIP with web server, FatFs and shell, all running
  together.
- NEW: Added an experimental and unsupported STM8 port for the IAR compiler,
  contributed by "king2".
- NEW: Reorganized the STM32 EXT driver to have a sub-platform specific
  part containing all the ISR related code, this has been necessary because
  the significant differences among the various sub-families.
- NEW: Validated CAN driver on STM32F2/F4 (backported to 2.4.2).
- NEW: USB implementation for STM32F105/F107/2xx/F4xx devices.
- NEW: Improved SerialUSB driver using the new queued mode, much smaller
  than the previous driver.
- NEW: Improved USB driver model supporting also queues for endpoint I/O,
  packet mode removed.
- NEW: Added an application-defined field to I/O queues (a void pointer).
- NEW: Added board files for Maple Mini STM32F103, contributed by Wagner
  Sartori Junior.
- NEW: Added SSP1 capability to the LPC13xx SPI driver.
- NEW: Updated vendor headers for LPC11xx and LPC13xx, the new headers
  support several new devices.
- NEW: Demo for STM32F0-Discovery board.
- NEW: Initial support for STM32F0xx devices, added a specific ADC driver.
  Validated EXT, GPT, ICU, PAL, PWM, Serial, SPI, UART drivers.
- NEW: Added a common ancestor class to the SDC and MMC_SPI drivers. This
  allows to share code and definitions.
- NEW: Modified the SDC driver to implement the new block devices abstract
  interface.
- NEW: Added two new functions to the MMC_SPI driver: mmcSync() and
  mmcGetInfo(). Also implemented the new block devices abstract
  interface. Moved the configuration parameters from mmcObjectInit() to
  the configuration structure saving some RAM space. Updated demos.
- NEW: Added an abstract interface for block devices in the HAL. This
  abstraction layer is meant to unify the access protocol to the SDC and
  MMC_SPI (and potentially others) device drivers.
- NEW: Added an abstract interface for serial devices in the HAL. This
  interface is meant to replace the equivalent class already present in the
  kernel. access macros are similar except for the prefix, "chn" instead
  of "chIO".
- NEW: Updated the MSP port to work with the latest MSPGCC compiler (4.6.3
  LTS 20120406 unpatched), now the old MSPGCC 3.2.3 is no more supported
  (backported to 2.4.1).
- NEW: EXT driver improved, now it is possible to reprogram channels at
  runtime without necessarily specifying a new configuration.
  TODO: Update AT91SAM7 EXT driver.
- NEW: Integrated FatFS 0.9, now the FatFS integration files are centralized
  under ./os/various/fatfs_bindings and shared among all demos. The FatFS
  file ffconf.h is now application-specific like all the other configuration
  files.
- NEW: Added an new option CORTEX_PRIGROUP_INIT to the Cortex-Mx ports in
  order to make priority organization configurable, the default is to
  assign all the available priority bits to preemption priority with no
  sub-priorities.
- NEW: Added a new function chPoolLoadArray() to the Memory Pools subsystem,
  it allows to load an entire array element's into a pool with a single
  operation.
- NEW: Addes support for .S patch in the GCC ARM ports, by Ayman El-Khashab.
- NEW: Added a switch to the STM32F4 Makefile files in order to enable or
  disable the FPU support in a single place.
- NEW: Added float support (optional) to chprintf(), by Fabio Utzig.
- NEW: Added overflow handling in the ICU driver (contributed by Xo).
- NEW: Updated debug plugin 1.0.8 (backported to 2.4.0).
- NEW: Added more accurate UBRR calculation in AVR serial driver (backported
  to 2.4.0).
- NEW: Revision of the round-robin scheduling, now threads do not lose their
  time slice when preempted. Each thread has its own time slices counter.
  TODO: Seek optimizations.
- NEW: Modified the Virtual Timers management, now the callback is invoked
  not in lock mode. This change reduces the interrupt jitter caused by
  multiple timers used at same time.
- NEW: Added board files and demo for Olimex LPC-P1343 (contributed by
  Johnny Halfmoon).
- NEW: Added handling of input 2 to the STM32 ICU driver (contributed by
  Fabio).
- NEW: STM32 Ethernet driver completed. Added STM32F107 and STM32F407
  lwIP demos (backported to 2.4.1).
- NEW: lwIP related code is not centralized into a single place, no need to
  duplicate the code in each application or demo (backported to 2.4.1).
- CHANGE: Added two new methods to the BaseSequentialStream interface:
  chSequentialStreamPut() and chSequentialStreamGet().
- CHANGE: Removed the chioch.h header from the kernel, now channels interface
  is exported by the HAL. Removed functions chPutWouldBlock() and
  chGetWouldBlock().
- CHANGE: Removed macro chMsgGetS(), chMsgGet() is still available.
- CHANGE: chprintf() now takes a BaseSequentialStream as parameter instead
  of a BaseChannel making it more generic.
- CHANGE: Now the shell requires a BaseSequentialStream instead of a
  BaseChannel for communications making it more generic.
- CHANGE: Kernel memory pools now do not check the alignment of the inserted
  objects, it is responsibility of the application to insert properly
  aligned objects.
- CHANGE: The PORT_INT_REQUIRED_STACK parameter for the Cortex-Mx ports has
  been increased to 32 from 16 because the stack frame sizes are increased
  when compiling with optimizations disabled, which is common during
  debugging. In order to save RAM trim back this value when compiling with
  optimizations enabled (backported to 2.4.1).
- CHANGE: Renamed Ethernet driver in AT91 HAL ETHD1 (backported to 2.4.1).
- CHANGE: Macros icuGetWidthI() and icuGetPeriodI() renamed to icuGetWidth()
  and icuGetPeriod().
- Various documentation fixes and improvements.

*** 2.4.4 ***
- FIX: Fixed wrong Keil project in ARMCM3-STM32F107 demo (bug #408).
- FIX: Fixed wrong macro in PWM driver (bug #407).
- FIX: Fixed USB driver possible deadlock under certain configurations (bug
  #406).
- FIX: Fixed USB driver cannot be stopped (bug #405).
- FIX: Fixed fixed I2C malfunction after fixing bug 3607518 (bug 3607549).
- FIX: Fixed spurious interrupt disabling an STM32 DMA stream (bug 3607518).
- FIX: Fixed start of any ADC disables VREF and VBAT (bug 3607467).
- FIX: Fixed CAN_USE_SLEEP_MODE compilation problem (bug 3606616).
- FIX: Fixed misplaced brace in icu_lld.c (bug 3605832).
- FIX: Fixed bug prevents calling adcStartConversionI() within ISR (bug
  3605053).
- FIX: Fixed typo in platforms/STM32/can_lld.c (bug 3604657).
- FIX: Fixed duplicated code in hal_lld.h (STM32F4xx) (bug 3602544).
- FIX: Fixed compile errors in Posix-GCC demo (bug 3601621).
- FIX: Fixed state checker error in MSP430 port (bug 3601460).
- FIX: Fixed wrong assertion in UART driver (bug 3600789).
- FIX: Fixed small bug in shell argument parsing code in shell_thread (bug
  3599328).
- FIX: Fixed wrong condition in checksum offload of STM32 MAC driver (bug
  3598720).
- FIX: Fixed error in STM32 MAC driver degrades performance (bug 3598719).
- NEW: Updated STM32L1xx header to the latest version.

*** 2.4.3 ***
- FIX: Fixed warning in STM32 ICU driver using IAR compiler (bug 3598177).
- FIX: Fixed typo in chOQGetEmptyI() macro (bug 3595910).
- FIX: Fixed possible false detect of loaded prescaler in RTCv1 driver (bug 
  3595489).
- FIX: Fixed unneeded RTC initialization when HAL_USE_RTC disabled
  (bug 3594620).
- FIX: Fixed compilation issue with HAL_USE_RTC disabled (bug 3594083).
- FIX: Fixed wasting of BKP registers in RTCv1 driver (bug 3594005).
- FIX: Fixed potential problem with RTC_CRL_RSF bit (bug 3593972).
- FIX: Fixed STM32F1x rtc_lld_init not functional (bug 3592817).
- FIX: Fixed DMA reconfiguration problem in STM32 SPI driver (bug 3592809).
- FIX: Fixed STM32 UART driver redundant initialization (bug 3592764).
- FIX: Fixed wrong stack initializations in GCC STM32L1xx port files (bug
  3591321).
- FIX: Fixed different redefinition for __main_stack_end__ symbol (bug
  3591317).
- FIX: Fixed errors in STM32F0xx UART driver (bug 3589412).
- FIX: Fixed MSP430 port_switch code for MSPGCC issue (bug 3587633).
- FIX: Fixed workaround for errata in STM32F4-A devices (bug 3586425).
- FIX: Fixed error in palWritePad() macro (bug 3586230).
- FIX: Fixed missing ; in testmbox.c (bug 3585979).
- FIX: Fixed STM32_P407: implement mmc_lld_is_card_inserted (bug 3581929).
- FIX: Fixed double chSysInit() call in MSP430F1611 demo (bug 3581304).
- FIX: Fixed bug in abstract file interface (bug 3579660).
- FIX: Fixed various typos and wrong limits in the STM32F4/F2 HAL driver
  (bug 3578944).
- FIX: Fixed ARM CMx crt0.c fails at low optimization levels (bug 3578927).
- FIX: Fixed compilation issue in syscalls.c (bug 3576771).
- FIX: Fixed superfluous pack #defines cause nasty warning (bug 3575662).
- FIX: Fixed mac.c won't compile due to misplaced declarations (bug 3575657).
- FIX: Fixed STM32F4 ADC prescaler incorrectly initialized (bug 3575297).
- FIX: Fixed RCC_APB2ENR_IOPEEN undeclared on STM32F10X_LD_VL devices (bug
  3575098).
- FIX: Fixed optimization disable (-O0) breaks kernel in CortexM/RVCT (bug
  3573123).
- FIX: Fixed misplaced declarations in lwip_bindings sys_arch.c (bug 3571053).
- FIX: Fixed FatFS won't compile with _FS_REENTRANT enabled (bug 3570135).
- FIX: Fixed mmc_spi.c won't compile due to misplaced declaration (bug
  3570035).
- FIX: Fixed GPIO glitch during PAL initialization (bug 3569347).
- FIX: Fixed STM32F1x rtc_lld_init glitches rtc on hard reset (bug 3567597).
- FIX: Fixed STM8L, cosmic compiler: c_lreg not saved (bug 3566342).
- FIX: Fixed anomaly in USB enumeration (bug 3565325).
- FIX: Fixed problem with lwIP statistics (bug 3564134).
- FIX: Fixed packed structures macros not functional in IAR and RVCT port
  (bug 3561279).
- FIX: Fixed Problem in FatFs demos related to LFN (bug 3560980).
- NEW: Added memory signature record to the registry in order to simplify
  the implementation of ad-hoc debuggers.
- NEW: I2C workaround allowing to read single byte on all STM32 platforms 
  except STM32F1xx.
- NEW: Small adjustment in chcore.h files under ./os/ports/GCC required by a
  difference in GCC 4.7.x.

*** 2.4.2 ***
- FIX: Fixed problem in STM32 DMA1 stream1 IRQ handler (bug 3538468).
- FIX: Fixed wrong priority assigned to TIM8 in STM32 ICU driver (bug 3536950).
- FIX: Fixed TIM8 not working in STM32 GPT driver (bug 3536523).
- FIX: Fixed timer overflow not working in STM32 ICU driver for TIM1/TIM8 (bug
  3536522).
- FIX: Fixed wrong DMA channels on USART2 in STM32F10X_MD_VL devices (bug
  3536070).
- FIX: Fixed issue with DMA channel init in STM32 ADC and SPI drivers (bug
  3535938).
- FIX: Fixed unreliable PHY initialization (bug 3534819).
- FIX: Fixed wrong ADC callback buffer pointer in ADC driver (bug 3534767).
- FIX: Fixed lwIP-related files missing from version 2.4.1 (bug 3533887).
- FIX: Fixed problem with arm-v6m and state checker (bug 3532591).
- FIX: Fixed wrong MAC divider setting in STM32 MAC driver (bug 3531290).
- FIX: Fixed wrong MCO1 divider in STM32F2/F4 HAL (bug 3531289).
- FIX: Fixed missing "break" in AVR PAL driver (bug 3530924).
- FIX: Fixed timeout related race condition in STM32 I2C driver (bug 3530043).
- FIX: Fixed wrong macro check in STM32 MAC driver (bug 3527179).
- FIX: Fixed error in STM32L-Discovery board.h file (bug 3526918).
- NEW: Validated CAN driver on STM32F2/F4.
- Small fixes to the STM32F4 board files.
- Various documentation fixes and improvements.

*** 2.4.1 ***
- FIX: Fixed inconsistent LPCxxx Internal RC oscillator names (bug 3524138).
- FIX: Fixed wrong frequency limit checks vs VDD in STM32F2xx HAL (bug
  3524094).
- FIX: Fixed STM32 I2C1 wrong alternate TX DMA setting (bug 3524088).
- FIX: Fixed three testhal builds fail (bug 3523322).
- FIX: Fixed MAC driver functions with invalid name (bug 3522808).
- FIX: Fixed code coverage crashes with Linux/gcc-4.4.5 (bug 3522301).
- FIX: Fixed macro dmaWaitCompletion() fails to compile in STM32 HAL (bug
  3519202).
- FIX: Fixed ARM addresses generated in vectors table (bug 3519037).
- FIX: Fixed missing serial driver functionality for SAM7S64, SAM7S128 and
  SAM7S512 (bug 3517648).
- FIX: Fixed a few more spelling fixes (bug 3515531).
- FIX: Fixed spurious ) char in STM32 serial_lld.h (bug 3514138).
- FIX: Fixed problem with FPU initialization in GCC Cortex-M4 port (bug
  3513897).
- FIX: Spelling fixes (bug 3510812).
- FIX: Fixed STM32 ICUD8 not functional because wrong initialization (bug
  3508758).
- FIX: Fixed chMBFetchI does not decrement mb_fullsem (bug 3504450).
- FIX: Fixed STM32 PLLI2S initialization error (bug 3503490).
- FIX: Fixed USART3 not working on STM32F2/F4 UART driver (bug 3496981).
- FIX: Fixed stack misalignment on Posix-MacOSX (bug 3495487).
- FIX: Fixed STM8S HSI clock initialization error (bug 3489727).
- FIX: Fixed MMC over SPI driver performs an unnecessary SPI read (bug
  3486930).
- FIX: Fixed Realtime counter initialization in STM32 HALs (bug 3485500).
- FIX: Fixed PPC port broken when CH_DBG_SYSTEM_STATE_CHECK is activated
  (bug 3485667).
- FIX: Fixed missing PLL3 check in STM32F107 HAL (bug 3485278).
- FIX: Fixed ADC maximum frequency limit in STM32F2/F4 ADC drivers (bug
  3484947).
- FIX: Fixed various minor documentation errors (bug 3484942).
- NEW: STM32 Ethernet driver completed. Added STM32F107 and STM32F407
  lwIP demos.
- NEW: lwIP related code is not centralized into a single place, no need to
  duplicate the code in each application or demo.
- NEW: Updated the MSP port to work with the latest MSPGCC compiler (4.6.3
  LTS 20120406 unpatched), now the old MSPGCC 3.2.3 is no more supported.
- CHANGE: The PORT_INT_REQUIRED_STACK parameter for the Cortex-Mx ports has
  been increased to 32 from 16 because the stack frame sizes are increased
  when compiling with optimizations disabled, which is common during
  debugging. In order to save RAM trim back this value when compiling with
  optimizations enabled.
- CHANGE: Renamed Ethernet driver in AT91 HAL ETHD1.

*** 2.4.0 ***
- NEW: Implemented new makefile system for ARM GCC ports, now objects,
  listings and output files are generated into a "build" directory and not
  together with sources, also implemented a simplified output log mode.
  Now makefiles and load script files are requirements and trigger a
  rebuild if touched.
- NEW: Improved Cortex-Mx port, now the Cortex-M4 are Cortex-M4F are also
  supported. Improved startup files.
- NEW: Added a new hook THREAD_CONTEXT_SWITCH_HOOK() that allows to insert
  code just before a context switch. For example this hook could be used
  in oder to implement advanced power management schemes.
- NEW: Improved Event Flags subsystems in the kernel.
- NEW: Added a macro THD_STATE_NAMES to chthreads.h. This macro is an
  initializer for string arrays containing thread state names.
- NEW: Added a new debug option CH_DBG_SYSTEM_STATE_CHECK that ensures the
  correct API call protocol. If an API is invoked out of the correct context
  then the kernel panics with a debug message.
- NEW: Added the new CMSIS 2.1 headers, now CMSIS resides into a shared
  location: ./os/ports/common/ARMCMx/CMSIS. Old CMSIS files have been
  removed from the various platforms.
- NEW: Removed all the ch.ld files from the ARMCMx demos, now the makefiles
  point to common ld files under the various ports. Less duplication and
  easier maintenance.
- NEW: Improved stack checking and reorganized memory map for the Cortex-Mx
  demos. Now stacks are allocated at the start of the RAM, an overflow of the
  exception stack now triggers an exception (it could go unnoticed before).
  The process stack is organized to be checked on context switch like other
  threads. Now all threads have an explicit stack boundary pointer.
- NEW: Now the port layer exports info regarding the compiler and the port
  options. The info are printed into the test reports. Date and time also
  added.
- NEW: Added initialization of the NVIC VTOR register to all Cortex-Mx (v7M)
  ports. Also added a port option CORTEX_VTOR_INIT to enforce a different
  default value into the register.
- NEW: Added "IRQ_STORM" test applications to those platforms supporting
  ISR preemption.
- NEW: Added a chprintf() function to ./os/various, it can print on any
  BaseChannel interface.
- NEW: Improved the mini shell, enhanced info command, optimizations and
  removed the shellPrint() and shellPrintLine() functions, now it uses
  chprintf() for output.
- NEW: USB driver model and USB implementation for STM32F1xx and STM32L1xx.
- NEW: USB CDC abstract driver.
- NEW: New driver models EXT, GPT, I2C, ICU, RTC, SDC, TM.
- NEW: Improved PWM and MAC driver models.
- NEW: Added support for STM32L1xx, STM32F2xx and STM32F4xx.
- NEW: Updated the STM32F1xx header file to the latest version 3.5.0 and fixed
  it in order to correct several bugs related to the XL family.
- NEW: Added an unified registers file for STM32: stm32.h. This file includes
  the appropriate vendor files then adds its own additional definitions.
- NEW: Added TIM8 support to the STM32 GPT, ICU and PWM drivers.
- NEW: DMA sharing in the STM32 HAL.
- NEW: ADC specific drivers for STM32L1xx, STM32F2xx and STM32F4xx.
- NEW: EXT driver AT91SAM7x and STM32(all).
- NEW: PAL driver for AVR.
- NEW: GPT driver for LPC11xx, LPC13xx and STM32(all).
- NEW: I2C driver for STM32(all).
- NEW: ICU driver for STM32(all).
- NEW: RTC driver for STM32F1xx.
- NEW: SDC driver for STM32F1xx.
- NEW: Added handling of USART6 to the STM32 serial driver.
- NEW: Added demos for the ST STM32F4-Discovery and STM32L1-Discovery kits.
- NEW: Updated AVR demos to use the new PAL driver.
- NEW: Now an error is generated at compile time when trying to enable the
  options CH_DBG_ENABLE_STACK_CHECK on ports that do not support it.
- NEW: Added a kernel-only Cortex-Mx demo as reference project for users not
  interested in the HAL but just want to use the ChibiOS/RT kernel.
  The demo is named ARMCM3-GENERIC-KERNEL and is defaulted to the STM32, in
  order to use it on other families or on the ARM Cortex-M0 just change the
  inclusion paths in the makefile.
- NEW: Integrated new FatFs version 0.8b.
- NEW: lwIP 1.4.0 has been integrated, this new version does not require
  custom hooks into the Thread structure and is thus much lighter.
- CHANGE: Now the callback associated to input queues is invoked before
  reading each character. Previously it was invoked only before going
  to sleep into the THD_STATE_WTQUEUE state.
- CHANGE: Removed the option CH_USE_NESTED_LOCK, lwIP no more requires it and
  it would have conflicted with CH_DBG_SYSTEM_STATE_CHECK which is far more
  useful.
- CHANGE: Renamed the scheduler functions chSchIsRescRequiredExI() to
  chSchIsPreemptionRequired(), chSchDoRescheduleI() to chSchDoReschedule(),
  chSysSwitchI() to chSysSwitch(). All those functions were special cases
  and not regular I-class APIs.
- CHANGE: Renamed the macros IDLE_THREAD_STACK_SIZE and INT_REQUIRED_STACK
  to PORT_IDLE_THREAD_STACK_SIZE and PORT_INT_REQUIRED_STACK for consistency.
- CHANGE: Removed the "old" Cortex-M3 port from the code, the current port
  has no drawbacks and the old port is now just a maintenance cost.
- CHANGE: Removed the CH_CURRP_REGISTER_CACHE option, it is GCC-specific so
  it does not belong to the kernel options. The feature will be eventually
  reimplemented as a port-specific option.
- CHANGE: Renamed the demo ARMCM3-STM32F107-GCC in ARMCM3-STM32F107 and added
  IAR and Keil projects.
- CHANGE: Now the ARMCM3-STM32F107 demo targets the board Olimex STM32-P107
  as default.
- CHANGE: Removed all the prefixes from the structure/union field names
  in the HAL subsystem.
- CHANGE: Updated the documentation to use Doxygen 1.7.4 which produces a much
  more readable output. Also modified the documentation layout to put functions
  and variables ahead of everything else in the group pages.
  Doxygen version below 1.7.4 cannot be used anymore because differences in
  templates. Note that now there are two Doxygen projects, one for generating
  the CHM file the other for plain HTML.

*** 2.2.8 ***
- NEW: Added new API chThdExitS() in order to allow atomic operations on
  thead exit.
- FIX: Fixed Extra initialization in STM32 SPI driver (bug 3436127).
- FIX: Fixed DMA priority setting error in STM32 UART driver (bug 3436125).
- FIX: Fixed DMA priority setting error in STM32 SPI driver (bug 3436124).
- FIX: Fixed broken support for UART5 in STM32 serial driver (bug 3434094).
- FIX: Fixed misplaced chRegSetThreadName() in ARM7-AT91SAM7S-FATFS-GCC demo
  (bug 3411780).
- FIX: Fixed missing UART5 definition in STM32 HAL (bug 3411774).
- FIX: The function chThdExit() triggers an error on shell return when the
  system state checker is enabled (bug 3411207).
- FIX: Some ARMCMx makefiles refer the file rules.mk in the ARM7 port (bug
  3411180).

*** 2.2.7 ***
- INFO: GCC test runs performed with YAGARTO 4.6.0.
- NEW: Added debug plugin for Eclipse under ./tools/eclipse.
- NEW: The debug macros chDbgCheck() and chDbgAssert() now can be externally
  redefined. The macro chDbgCheck() no more includes the line number in the
  description because incompatibility with the Cosmic compiler.
- NEW: Added a new functionality to the registry subsystem, now it is possible
  to associate a name to the threads using chRegSetThreadName. The main and
  idle threads have their name assigned by default.
- FIX: Fixed wrong check on CH_DBG_ENABLE_STACK_CHECK setting (bug 3387671).
- FIX: Fixed wrong APB1 frequency check (bug 3361039).
- FIX: Fixed missing state in shell demos (bug 3351556).
- CHANGE: Removed todo.txt file, it does not belong to the stable version.

*** 2.2.6 ***
- FIX: Fixed race condition in Cortex-Mx ports (bug 3317500).
- FIX: Fixed wrong macro check in STM32 UART driver (bug 3311999).

*** 2.2.5 ***
- FIX: Fixed STM32F107 demo build failure (bug 3294998).
- NEW: Reorganization of the Cortex-Mx ports in order to reduced code and
  comments duplication in the various headers (backported to 2.2.5).
- NEW: Improved the ARMv7-M sub-port now there are two modes: Compact and
  Advanced.
  The advanced mode is equivalent to the previous versions, the compact mode
  is new and makes the kernel *much* smaller and generally faster but does
  not support fast interrupts.
- NEW: Added to the ARMv6-M sub-port an option to use the PendSV exception
  instead of NMI for preemption.

*** 2.2.4 ***
- FIX: Fixed CodeBlocks demo broken (bug 3304718).
- FIX: Fixed race condition in output queues (bug 3303908).
- FIX: Fixed CH_USE_HEAP and CH_USE_MALLOC_HEAP conflict (bug 3303841).
- FIX: Fixed timeout problem in the lwIP interface layer (bug 3302420).
- FIX: Fixed invalid BRR() macro in AVR serial driver (bug 3299306).
- FIX: Fixed missing IRQ vectors amicable names for STM32 XL devices (bug
  3298889).
- FIX: Fixed wrong identifier in AVR serial driver (bug 3292084).
- FIX: Fixed wrong macro check for STM32 XL devices (bug 3291898).
- FIX: Fixed SPI driver restart in STM32 SPI driver implementation, also
  applied the same fix to the STM8S SPI driver (bug 3288758).
- FIX: Fixed missing state transition in ADC driver (bug 3288149).
- FIX: Fixed missing state transition in SPI driver (bug 3288112).
- NEW: Added an option to the kernel to not spawn the Idle Thread from within
  chSysInit(), this way the application can spawn a custom idle thread or
  even use the main() thread as idle thread.
- CHANGE: chiQGetFullI() and chOQGetFullI() become macros. The queues
  subsystem has been optimized and is no more dependent on semaphores.
  Note that the queues callbacks invocation policy has been slightly
  changed, see the documentation.

*** 2.2.3 ***
- FIX: Fixed insufficient idle thread stack in Cortex-M0-GCC port
  (bug 3226671).
- FIX: Fixed stack checking in Cortex-M0-GCC port (bug 3226657).
- FIX: Fixed wrong checks in PAL driver (bug 3224681).
- FIX: Fixed wrong checks in I/O Queues (bug 3219197).
- FIX: Fixed invalid assertion in adcConvert() (bug 3205410).
- NEW: Implemented stack checking in the Cortex-Mx RVCT port.
- NEW: Improved preemption implementation for the Cortex-M0, now it uses
  the NMI vector in order to restore the original context. The change makes
  IRQ handling faster and also saves some RAM/ROM space. The GCC port code
  now does not inline the epilogue code in each ISR saving significant ROM
  space for each interrupt handler in the system.

*** 2.2.2 ***
- FIX: Fixed race condition in CM0 ports, the fix also improves the
  ISR latency (bug 3193062).
- FIX: Fixed Cortex-Mx linker scripts alignment of __heap_base__, the
  correct alignment is now enforced at runtime into core_init() in order
  to make the OS integration easier (bug 3191112).
- FIX: Fixed error in function chCoreAllocI() function documentation (bug
  3191107).
- FIX: Fixed minor problem with memory pools (bug 3190512).
- NEW: Added I-Class functions to the MailBoxes subsystem, now it is
  possible to use them as a transport layer between ISRs and Threads.
- CHANGE: Swapped the numeric values of the TIME_IMMEDIATE and TIME_INFINITE
  constants. Fixed the relative documentation in various places.
- Documentation related fixes.

*** 2.2.1 ***
- FIX: Stack overflow in CM0 ports when nearing interrupts saturation (bug
  3187105).
- FIX: Fixed missing e200z test report (bug 3182611).
- FIX: Fixed error in _BSEMAPHORE_DATA macro (bug 3184139).
- FIX: Error in MAC driver (bug 3179783).
- FIX: Fixed wrong serial driver macros (bug 3173336).

*** 2.2.0 ***
- NEW: The Cortex-Mx port now also supports the IAR and Keil compilers.
- NEW: Improvements to the Cortex-Mx port.
- NEW: Improved ARM port supporting both ARM7 and ARM9.
- NEW: Support for binary semaphores in the kernel.
- NEW: Improved kernel hooks.
- NEW: Extensive improvements to the STM32 platform support.
- NEW: SPI drivers for the AT91SAM7x, LPC11xx, LPC13xx, LPC214x, STM8S,
  platforms.
- NEW: Unified STM8 port for both the Cosmic and the Raisonance compilers.
- NEW: Demos for the STM8S-Discovery, STM8L-Discovery, STM32VL-Discovery
  boards.
- NEW: Improved almost all the existing device driver models in the HAL.
- NEW: Added test/example applications for all the device drivers in the HAL.
- NEW: Added support for the STM8L platform.
- NEW: New UART unbuffered serial device driver model.
- NEW: Greatly improved documentation.
- NEW: Lots of other minor improvements and optimizations, see the change
  log of the 2.1.x development branch for details.

*** 2.0.10 ***
- FIX: Fixed missing lines from the STM32 PWM driver (bug 3154403).
- FIX: Fixed error in chIOGetxxxxxEventSource() macros (bug 3153550).
- FIX: Fixed switch condition error in STM32 PWM driver (bug 3152482).

*** 2.0.9 ***
- FIX: Fixed error in output queues static initializer (bug 3149141).
- FIX: Fixed extra notifications in input queues (bug 3148525).
- FIX: Fixed error in sdPutTimeout() macro (bug 3138763).
- FIX: Fixed preprocessing crt0.s fail (bug 3132293).
- Documentation related fixes.

*** 2.0.8 ***
- FIX: Fixed failed memory recovery by registry scan, improved the related
  test case (bug 3116888).
- FIX: Fixed PWM channels going to ACTIVE state when the pulse width is
  set to zero in the STM32 PWM driver (bug 3114481).
- FIX: Fixed PWM channels return to IDLE state in STM32 PWM driver (bug
  3114467).
- CHANGE: Bugs 3114467 and 3114481 have been fixed by backporting the 2.1.x
  PWM driver, there is a difference in the PWM callback parameters.

*** 2.0.7 ***
- FIX: Fixed typo in board name (bug 3113574).
- FIX: Fixed defective event wait functions with timeout (bug 3113443).

*** 2.0.6 ***
- FIX: Fixed typo in memstreams.h (bug 3089567).
- FIX: Fixed wrong macro check in LPC214x and AT91SAM7 serial drivers (bug
  3088776).
- FIX: Fixed non functioning option SPI_USE_MUTUAL_EXCLUSION=FALSE (bug
  3084764).
- FIX: Fixed wrong macro check in STM32 serial support (but 3078891).
- FIX: Fixed lwIP demo not working (bug 3076354).
- FIX: Fixed non functioning option CH_USE_NESTED_LOCKS (bug 3075544).
- CHANGE: The API chThdInit() has been renamed to chThdCreateI().

*** 2.0.5 ***
- FIX: Incorrect AT91SAM7X initialization, thanks Leszek (bug 3075354).
- FIX: Fixed race condition in function chSchGoSleepTimeoutS, thanks Balzs
  (bug 3074984).
- FIX: Fixed race condition in threads creation (bug 3069854).
- FIX: Fixed broken CH_DBG_ENABLE_STACK_CHECK option in legacy CM3 port (bug
  3064274).
- FIX: Fixed CAN_USE_SLEEP_MODE setting (bug 3064204).

*** 2.0.4 ***
- FIX: Fixed potential issue with GCC reorganizing instructions around "asm
  volatile" statements (bug 3058731).
- FIX: Fixed reduced ARM7 performance with GCC 4.5.x (bug 3056866).

*** 2.0.3 ***
- Tests reports regenerated using GCC 4.5.1, small performance improvements
  in all benchmarks.
- FIX: Fixed crash of the Posix simulator under Ubuntu 10.4 (bug 3055329).
- FIX: Fixed incorrect PLL2 setting in STM32 HAL (bug 3044770).
- FIX: Fixed wrong check on STM32_HCLK (bug 3044758).
- FIX: Fixed wrong condition check in STM32 PWM driver (bug 3041414).
- FIX: Corrupted IRQ stack in Cortex-Mx port (bug 3041117).
- FIX: Fixed a documentation error regarding the ADC driver function
  adcStartConversion() (bug 3039890).
- FIX: Fixed insufficient stack size for idle thread (bug 3033624).
- FIX: Fixed misspelled word in some chioch.h and chstreams.h macros (bug
  3031534).
- FIX: Fixed wrong macro check in the STM32 SPI driver (bug 3028562).


*** 2.0.2 ***
- FIX: Fixed invalid context restore in MSP430 port (bug 3027975).
- FIX: Fixed STM32 vectors file (bug 3026528).
- FIX: Fixed race condition in STM32 SPI driver (bug 3025854).
- FIX: Fixed H_LOCK and H_UNLOCK redefined with CH_USE_MALLOC_HEAP (bug
  3025549).
- FIX: Added option to enforce the stack alignment to 32 or 64 bits in the
  Cortex-Mx port (bug 3025133).
- NEW: Added friendly interrupt vectors names to the STM32 HAL (change request
  3023944).
- CHANGE: Removed the option -mabi=apcs-gnu from all the Cortex-Mx demos. The
  option is not compatible with the 64 bits stack alignment now default in
  the Cortex-Mx port. Note that the 64 bits alignment has a cost both as
  performance and as space but it is the "standard".

*** 2.0.1 ***
- FIX: Fixed notification order in input queues (bug 3020708).
- FIX: Fixed non functional CH_CURRP_REGISTER_CACHE option in the Cortex-M3
  port (bug 3020702).
- FIX: Fixed non functional CH_DBG_ENABLE_STACK_CHECK option in the Cortex-M3
  caused by GCC 4.5.0, the fix also improves the context switch performance
  because GCC 4.5.0 apparently was generating useless instructions within the
  very critical context switch code (bug 3019738).
- FIX: Fixed insufficient stack space assigned to the idle thread in
  Cortex-M3 port (bug 3019594).
- FIX: Fixed missing check in chIQReadTimeout() and chIQWriteTimeout() (bug
  3019158).
- FIX: Fixed instability in Mutexes subsystem (bug 3019099).
- NEW: Added timers clock macros to the STM32 clock tree HAL driver.

*** 2.0.0 ***
- NEW: Implemented the concept of thread references, this mechanism ensures
  that a dynamic thread's memory is not freed while some other thread still
  owns a reference to the thread. Static threads are not affected by the new
  mechanism. Two new APIs have been added: chThdAddRef() and chThdRelease().
- NEW: Now more than one thread can be waiting in chThdWait() as long they
  own a reference.
- NEW: Implemented a new threads registry subsystem, the registry allows to
  enumerate the active threads at runtime and/or from a debugger. This is
  a preparatory step for a dedicated ChibiOS/RT debugger.
- NEW: New chCoreFree() API that returns the core memory left.
- NEW: Added a PowerPC port and demo targeting the SPC563M/MPC563xM
  ST/Freescale automotive SOCs.
- NEW: Added STM8 port and demo targeting the Raisonance REva board
  with STM8S208RB piggyback.
- NEW: New unified ARM Cortex-Mx port, this port supports both the ARMv6M
  and ARMv7-M architectures (Cortex-M0/M1/M3/M4 so far). The new port also
  allow to easily add to new Cortex-M implementations by simply adding a
  parameters file (cmparams.h).
- NEW: Improved clock initialization for the STM32, now it is possible to
  configure the clock using any clock source and any HSE frequency.
- NEW: The STM32 clock tree parameters and checks are now calculated into
  a separate file in order to support multiple clock trees for different
  sub-families of the STM32 platform.
- NEW: Added separated clock trees for the STM32 LD/MD/HD sub-family and
  the CL sub-family. Now the selection of the sub-family is done in the
  board.h file, there is no more the need to put -DSTM32F10X_xx into
  the makefile.
- NEW: Added support for STM32/HD/CL UART4 and UART5, thanks Egon for the
  patch.
- NEW: Embedded Artists LPCxpresso Base Board support files added.
- NEW: LPC11xx support, drivers (Serial, PAL, HAL) and demo.
- NEW: LPC13xx support, drivers (Serial, PAL, HAL), demo and reports.
- NEW: The port layer now can "capture" the implementation of individual
  scheduler API functions in order to provide architecture-optimized
  versions. This is done because further scheduler optimizations are
  becoming increasingly pointless without considering architecture and
  compiler related constraints.
- NEW: Updated the STM32 FW Library files to latest version 3.3.0.
- NEW: AT91SAM7 HAL support for the DGBU UART peripheral, as SD3.
- NEW: Added a demo for the AT91SAM7S256 and board files for the Olimex
  SAM7-P256. The demo has been contributed by Alexander Kozaruk.
- NEW: Added core variant name macro in chcore.h and platform name in
  hal_lld.h, the info are printed in the test report and from the "info"
  shell command.
- NEW: Added BOARD_NAME macro to the various board.h files.
- NEW: Added a MemoryStream class under ./os/various.
- NEW: Added Mac OS-X support for the simulator. The Linux simulator has
  been renamed to Posix simulator in order to include this change in a
  single project.
- NEW: New articles, sections and various improvements to the documentation.
- NEW: Added to the simulators shell demos two new commands: threads and mem,
  that show the currently active threads (using the new registry) and the
  memory allocators state.
- NEW: New articles and guides in the documentation.
- OPT: New Cortex-M3 port code, *huge* performance improvements in all the
  context switching related benchmarks (up to 18% depending on the benchmark).
  The new code does no more require the use of the PendSV vector that is
  thus available to the user, it also saves four RAM bytes for each thread
  in the system. The old code is still available as a fall back option while
  the new one is being hardened by peer review and time, the two ports are
  perfectly interchangeable.
- OPT: Speed/size optimization to the events subsystem.
- OPT: Speed/size optimization to the mutexes subsystem.
- OPT: Speed/size optimization to the condvars subsystem.
- OPT: Speed/size optimization to the synchronous messages subsystem.
- OPT: Small size optimization in the semaphores subsystem.
- OPT: Minor optimizations in the "compact" code path.
- OPT: Optimization on the interface between scheduler and port layer, now
  the kernel is even smaller and the context switch performance improved
  quite a bit on all the supported architectures.
- OPT: Simplified the implementation of chSchYieldS() and made it a macro.
  The previous implementation was probably overkill and took too much space
  even if a bit faster.
- OPT: Internal optimization in the serial driver, it now is a bit smaller
  and uses less RAM (all architectures).

*** 1.4.3 ***
- FIX: Fixed centralized ARM makefile (bug 2992747).
- FIX: Fixed write problems in MMC_SPI driver (bug 2991714).
- FIX: Fixed wrong macro check in serial.h (bug 2989459).

*** 1.4.2 ***
- FIX: Fixed missing reschedule in chEvtSignal() (bug 2961208).
- FIX: Removed C99-style variables declarations (bug 2964418).
- Minor documentation fixes.

*** 1.4.1 ***
- FIX: Fixed wrong UART deinitialization sequence in LPC214x serial driver
  (bug 2953985).
- FIX: Fixed wrong PINSEL2 offset into lpc214x.h (bug 2953981).
- FIX: Fixed invalid UART-related macro in the LPC214x HAL (bug 2953195).
- FIX: Wrong prototype in template file chcore.c (bug 2951529).
- FIX: Fixed insufficient stack space for the idle thread in the ARMCM3 port
  when compiling without optimizations (bug 2946233).
- FIX: Fixed wrong notes on function chThdResume() (bug 2943160).
- FIX: Fixed missing dependencies check for CH_USE_DYNAMIC (bug 2942757).
- FIX: Fixed swapped thread states descriptions (bug 2938445).
- FIX_ Fixed C99-style variable declaration (bug 2938444).

*** 1.4.0 ***
- Full test cycle and test reports updated.
- NEW: Reorganized and rationalized the distribution tree and the
  documentation.
- NEW: Abstract Streams and I/O Channels mechanisms introduced.
- NEW: Added a new core memory manager.
- NEW: Improved Heap and Pools allocators.
- NEW: The I/O queues code has been improved, now there are 2 separate
  structures: InputQueue and OutputQueue.
- NEW: Added timeout specification to the I/O queues read/write primitives.
- NEW: Static initializers macros introduced for most kernel objects.
- NEW: Added new APIs chSchDoYieldS() and chThdYield().
- NEW: Improved and simplified kernel configuration files.
- MEW: Added new benchmarks and test cases.
- NEW: Added more test cases in order to improve the test suite code coverage
  (it was 74% in version 1.2.0, it is now close to 100%).
- NEW: Added a code coverage analysis application under ./tests/coverage.
- NEW: Added the test suite documentation to the general documentation.
- NEW: Linux x86 simulator demo added.
- NEW: Improved the Cortex-M3 preemption code.
- NEW: Added standard CMSIS 1.2.0 support to the Cortex-M3 port.
- NEW: Added support for the ST firmware library to the STM32 port.
- NEW: Added support for HD and CL STM32 devices.
- NEW: Improvements to the AT91SAM7 support.
- NEW: Improved makefiles and makefile fragments, now the paths are not fixed.
- NEW: Unified the initialization of the various drivers from a single HAL
  driver. The single drivers can be enabled or disabled from a HAL
  configuration file halconf.h.
- NEW: Hardware Abstraction Layer (HAL) with support for PAL, ADC, CAN, MAC,
  MMC/SD, PWM, Serial, SPI drivers. Added driver implementations to the
  various platforms.
- NEW: Added support for uIP, lwIP, FatFS external libraries, added demos.
- Many many other improvements and minor features.

*** 1.2.4 ***
- FIX: Fixed GCC 4.4.x aliasing warnings (bug 2846336).
- FIX: Modified linker scripts for GCC 4.4.x (bug 2846302).
- FIX: Fixed the CH_OPTIMIZE_SPEED option in the CM3 port (bug 2846278).
- FIX: Fixed GCC 4.4.x related problems in CM3 port (bug 2846162).
- FIX: Fixed LPC214x UART problem (bug 2841088).

*** 1.2.3 ***
- FIX: Fixed C99-style variable declarations (bug 2792919).
- FIX: Fixed instance of obsolete CH_USE_TERMINATE option in the C++ wrapper
  (bug 2796065).
- FIX: Insufficient stack allocated to the C++ LPC2148 demo (bug 2796069).
- FIX: Fixed errors in events test case (bug 2796081).
- CHANGE: Increased main stack size to 1KiB for all the ARMx demos, 2KiB for
  the C++ LPC2148 demo. This should make things easier for unexperienced
  users.

*** 1.2.2 ***
- FIX: Fixed macro in test.h (bug 2781176).
- FIX: Fixed @file tag in sam7x_serial.c (bug 2788573).
- FIX: Fixed sequence assertion in test.c (bug 2789377).
- FIX: Fixed test_cpu_pulse() incorrect behavior (bug 2789383).
- FIX: Fixed missing volatile modifier for p_time field in Thread structure
  (bug 2789501).
- CHANGE: Made the option CH_DBG_THREADS_PROFILING default to TRUE because it
  is now required in order to execute the whole test suite. Note that this
  option is very light so there is no real overhead in the system.
- Added a (harmless) workaround to the Cortex-M3 startup file in order to
  make the RIDE7 demo compile on an unmodified distribution.

*** 1.2.1 ***
- FIX: Fixed regression in MinGW demo (bug 2745153).
- FIX: Fixed problem with the timeout constant TIME_IMMEDIATE (bug 2755170).
- FIX: Fixed a problem in semaphores test case #2 (bug  2755195).
- FIX: Removed unused list functions (bug 2755230).
- FIX: Added the exception notes into the source headers (bug 2772129).
- FIX: Added license notice to several files (bug 2772160).
- FIX: Found new instances of the obsolete function chSysGetTime() in the
  C++ wrapper and in the WEB demo (bug 2772237).

*** 1.2.0 ***
- Full test cycle and test reports updated.
- NEW: Better separation between the port code and the system APIs, now an
  architecture-specific "driver" contains all the port related code.
  Port functions/macros are no more directly exposed as APIs to the user code.
- NEW: Added a configuration option to enable nested system locks/unlocks.
- NEW: Improved the interrupt handlers related code. Now interrupts are
  handled in a very similar way in every architecture. See the "Concepts"
  section and the "Writing interrupt handlers under ChibiOS/RT" article in the
  documentation.
- NEW: Added the chEvtSignal() and chEvtSignalI() APIs that allows direct
  thread signaling, much more efficient that chEvtBroadcast() when the target
  is a known single thread.
- NEW: Added a configuration option that enables the priority enqueuing on
  semaphores. It is defaulted to off because usually semaphores are used for
  I/O related tasks without hard realtime requirements.
- NEW: Now the all the options in chconf.h and the various driver headers
  can be overridden externally, as example from within the Makefile.
  The options are no mode a simple define but a define with an assigned
  TRUE/FALSE value within an #ifndef block.
- NEW: Idle thread hook macro added to the configuration file.
- NEW: Changed the ARM7 and Cortex-M3 startup files, now the action when
  the main() function returns can be overridden by redefining the symbol
  MainExitHandler.
- NEW: Mailboxes (asynchronous messages) subsystem and test cases added.
- NEW: Most APIs with a timeout specification now accept the constant
  TIME_IMMEDIATE (-1) that triggers an immediate timeout when trying to enter
  a sleep state.
- NEW: Mode flexible debug configuration options, removed the old CH_USE_DEBUG
  and CH_USE_TRACE. Replaced with CH_DBG_ENABLE_CHECKS, SCH_DBG_ENABLE_ASSERTS,
  CH_DBG_ENABLE_TRACE  and CH_DBG_FILL_THREADS.
- NEW: Added a debug option CH_DBG_THREADS_PROFILING for threads profiling.
  A field into the Thread structure counts the consumed time. The information
  is not used into the kernel, it is meant for debugging.
- NEW: Added a debug option CH_DBG_ENABLE_STACK_CHECK for stack overflow
  checking. The check is not performed in the kernel but in the port code.
  Currently only the ARM7 and ARMCM3 ports implements it.
- NEW: Unified makefiles for ARM7, ARMCM3 MSP430 projects, the new makefiles
  share a common part making them easier to maintain. Also reorganized the
  demo-specific part of the makefile, now it is easier to configure and the
  option can be overridden from outside.
- OPT: Improved ARM7 thumb port code, thanks to some GCC tricks involving
  registers usage now the kernel is much smaller, faster and most OS APIs
  use less RAM in stack frames (note, this is an ARM7 thumb mode specific
  optimization).
- OPT: Small optimization to the Cortex-M3 thread startup code, improved thread
  related performance scores and smaller code.
- OPT: Alternative, non-inlined and more compact, implementations for
  port_lock() and port_unlock() in the Cortex-M3 port when CH_OPTIMIZE_SPEED
  is FALSE.
- OPT: Improved ready list and priority ordered lists code, some space saved,
  better context switch performance.
- CHANGE: Now the API chThdSetPriority() returns the old priority instead
  of void.
- CHANGE: Modified the signature of the chMsgSendWithEvent() API, it now uses
  a more efficient event signaling method.
- CHANGE: Removed the field p_tid from the Thread structure and the related
  code, this improved the thread creation scores (~2%) and saves some RAM.
  The trace buffer field cse_tid is now populated with a simple hash of the
  thread pointer as thread identifier.
- CHANGE: Renamed the macros chSysIRQEnter() and chSysIRQExit() in
  CH_IRQ_PROLOGUE() and CH_IRQ_EPILOGUE() in order to make very clear that
  those are not functions but inlined code. Also introduced a new macro
  CH_IRQ_HANDLER that should be used when declaring an interrupt handler.
- CHANGE: Renamed several internal initialization functions by removing the
  "ch" prefix because could not be considered system APIs.
- CHANGE: Changed the chSemFastWaitS() macro in chSemFastWaitI() and
  chSemGetCounter() in chSemGetCounterI().
- Improved ARM7 and Cortex-M3 support, new configuration options.
- Introduced the concept of interrupt classes, see the documentation.
- Introduced the concept of system states, see the documentation.
- Huge improvements to the documentation.
- Articles and notes previously in the wiki now merged in the general
  documentation and updated, the wiki entries are obsolete and will be removed.
- New application notes and articles added.
- Added kernel size metrics to the test reports.
- Removed the inclusion graph from the documentation because the little
  info they add and the size of all the images. It is possible to configure
  Doxygen to have them again (and more graph types).
- Improvements to the test suite, added a new level of indirection that allows
  to make tests depend on the configuration options without have to put #ifs
  into the test main module. New benchmarks about semaphores and mutexes.
- Modified the test thread function to return the global test result flag.
- Removed testcond.c|h and moved the test cases into testmtx.c. Mutexes and
  condvars have to be tested together.
- Added architecture diagram to the documentation.

*** 1.0.2 ***
- FIX: Fixed priority inheritance problem with condvars (bug 2674756).
- FIX: Fixed a problem in time ranges (bug 2680425).
- Replaced ./docs/index.html with a direct shortcut to the documentation.

*** 1.0.1 ***
- NEW: Added to the STM32 demo makefile an option to build ChibiOS/RT with the
  full STM32 FWLib 2.03.
  Note that, except for the compile option, the library is not used by the
  OS nor supported.
- FIX: Fixed a problem into the STACK_ALIGN() macro.
- FIX: Fixed a problem with a wrong declaration of the PLL structure in the
  file lpc214x.h.
- FIX: Modified the default value for the STM32 HSI setup it was 1, it should
  be 0x10.
- FIX: Removed an obsolete constant (P_SUSPENDED) from thread.h.
- FIX: Removed unused field mp_grow in the MemoryPool structure.
- FIX: Fixed wrong assertions in chThdWait() and chHeapFree().
- FIX: Fixed a problem with some event APIs not showing in the documentation.

*** 1.0.0 ***
- License switch, added GPL exception, see exception.txt.
- Full test cycle and test reports updated.
- Renamed some occurrences of "Conditional Variable" in "Condition Variable" in
  the documentation.
- FIX: Fixed some images in the documentation because problems when seen in
  Internet Explorer.
