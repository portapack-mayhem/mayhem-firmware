/**************************************************************************//**
 * $Id: LPC122x.h 6933 2011-03-23 19:02:11Z nxp28548 $
 *
 * @file     LPC122x.h
 * @brief    CMSIS Cortex-M0 Core Peripheral Access Layer Header File for 
 *           NXP LPC122x Device Series
 * @version  1.1
 * @date     $Date:: 2011-03-23#$
 * @author   NXP MCU Team
 *
 * @note
 * Copyright (C) 2011 NXP Semiconductors(NXP). All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * products. This software is supplied "AS IS" without any warranties.
 * NXP Semiconductors assumes no responsibility or liability for the
 * use of the software, conveys no license or title under any patent,
 * copyright, or mask work right to the product. NXP Semiconductors
 * reserves the right to make changes in the software without
 * notification. NXP Semiconductors also make no representation or
 * warranty that such application will be suitable for the specified
 * use without further testing or modification.
 ******************************************************************************/



/** @addtogroup (null)
  * @{
  */

/** @addtogroup LPC122x
  * @{
  */

#ifndef __LPC122X_H__
#define __LPC122X_H__

#ifdef __cplusplus
extern "C" {
#endif 


#if defined ( __CC_ARM   )
  #pragma anon_unions
#endif

 /* Interrupt Number Definition */

typedef enum {
	// -------------------------  Cortex-M0 Processor Exceptions Numbers  -----------------------------
	Reset_IRQn                        = -15,  /*!<   1  Reset Vector, invoked on Power up and warm reset */
	NonMaskableInt_IRQn               = -14,  /*!<   2  Non maskable Interrupt, cannot be stopped or preempted */
	HardFault_IRQn                    = -13,  /*!<   3  Hard Fault, all classes of Fault */
	SVCall_IRQn                       = -5,   /*!<  11  System Service Call via SVC instruction */
	DebugMonitor_IRQn                 = -4,   /*!<  12  Debug Monitor                    */
	PendSV_IRQn                       = -2,   /*!<  14  Pendable request for system service */
	SysTick_IRQn                      = -1,   /*!<  15  System Tick Timer                */
	// ---------------------------  LPC122x Specific Interrupt Numbers  -------------------------------
	WAKEUP0_IRQn			= 0,	/*!< PIO0_0 to PIO0_11	Wakeup */
	WAKEUP1_IRQn			= 1,
	WAKEUP2_IRQn			= 2,
	WAKEUP3_IRQn			= 3,
	WAKEUP4_IRQn			= 4,
	WAKEUP5_IRQn			= 5,
	WAKEUP6_IRQn			= 6,
	WAKEUP7_IRQn			= 7,
	WAKEUP8_IRQn			= 8,
	WAKEUP9_IRQn			= 9,
	WAKEUP10_IRQn			= 10,
	WAKEUP11_IRQn			= 11,	/*!< PIO0_0 to PIO0_11	Wakeup */
	I2C_IRQn				=12,	/*!< I2C Interrupt	*/
	TIMER_16_0_IRQn			= 13,	/*!< 16-bit Timer0 Interrupt	*/
	TIMER_16_1_IRQn			= 14,	/*!< 16-bit Timer1 Interrupt	*/
	TIMER_32_0_IRQn			= 15,	/*!< 32-bit Timer0 Interrupt	*/
	TIMER_32_1_IRQn			= 16,	/*!< 32-bit Timer1 Interrupt	*/
	SSP_IRQn				= 17,	/*!< SSP Interrupt	*/
	UART0_IRQn				= 18,	/*!< UART0 Interrupt	*/
	UART1_IRQn				= 19,	/*!< UART1 Interrupt	*/
	CMP_IRQn				= 20,	/*!< Comparator Interrupt	*/
	ADC_IRQn				= 21,	/*!< A/D Converter Interrupt	*/
	WDT_IRQn				= 22,	/*!< Watchdog timer Interrupt	*/
	BOD_IRQn				= 23,	/*!< Brown Out Detect(BOD) Interrupt	*/
	EINT0_IRQn				= 25,	/*!< External Interrupt 0 Interrupt	*/
	EINT1_IRQn				= 26,	/*!< External Interrupt 1 Interrupt	*/
	EINT2_IRQn				= 27,	/*!< External Interrupt 2 Interrupt	*/
	DMA_IRQn				= 29,	/*!< DMA Interrupt	*/
	RTC_IRQn				= 30,	/*!< RTC Interrupt	*/
} IRQn_Type;


/** @addtogroup Configuration_of_CMSIS
  * @{
  */

/*
 * ==========================================================================
 * ----------- Processor and Core Peripheral Section ------------------------
 * ==========================================================================
 */

/* Configuration of the Cortex-M0 Processor and Core Peripherals */
#define __MPU_PRESENT             0         /*!< MPU present or not                               */
#define __NVIC_PRIO_BITS          2         /*!< Number of Bits used for Priority Levels          */
#define __Vendor_SysTickConfig    0         /*!< Set to 1 if different SysTick Config is used     */

/*@}*/ /* end of group LPC12xx_CMSIS */


#include "core_cm0.h"                       /* Cortex-M0 processor and core peripherals           */
#include "system_LPC122x.h"                 /* System Header									  */

/** @addtogroup Device_Peripheral_Registers
  * @{
  */


// ------------------------------------------------------------------------------------------------
// -----                                          I2C                                         -----
// ------------------------------------------------------------------------------------------------


/**
  * @brief Product name title=UM10441 Chapter title=LPC122x I2C-bus controller Modification date=2/2/2011 Major revision=0 Minor revision=17  (I2C)
  */

typedef struct {                            /*!< (@ 0x40000000) I2C Structure          */
  __IO uint32_t CONSET;                     /*!< (@ 0x40000000) I2C Control Set Register. When a one is written to a bit of this register, the corresponding bit in the I2C control register is set. Writing a zero has no effect on the corresponding bit in the I2C control register. */
  __I  uint32_t STAT;                       /*!< (@ 0x40000004) I2C Status Register. During I2C operation, this register provides detailed status codes that allow software to determine the next action needed. */
  __IO uint32_t DAT;                        /*!< (@ 0x40000008) I2C Data Register. During master or slave transmit mode, data to be transmitted is written to this register. During master or slave receive mode, data that has been received may be read from this register. */
  __IO uint32_t ADR0;                       /*!< (@ 0x4000000C) I2C Slave Address Register 0. Contains the 7-bit slave address for operation of the I2C interface in slave mode, and is not used in master mode. The least significant bit determines whether a slave responds to the General Call address. */
  __IO uint32_t SCLH;                       /*!< (@ 0x40000010) SCH Duty Cycle Register High Half Word. Determines the high time of the I2C clock. */
  __IO uint32_t SCLL;                       /*!< (@ 0x40000014) SCL Duty Cycle Register Low Half Word. Determines the low time of the I2C clock. I2nSCLL and I2nSCLH together determine the clock frequency generated by an I2C master and certain times used in slave mode. */
  __IO uint32_t CONCLR;                     /*!< (@ 0x40000018) I2C Control Clear Register. When a one is written to a bit of this register, the corresponding bit in the I2C control register is cleared. Writing a zero has no effect on the corresponding bit in the I2C control register. */
  __IO uint32_t MMCTRL;                     /*!< (@ 0x4000001C) Monitor mode control register. */
  __IO uint32_t ADR1;                       /*!< (@ 0x40000020) I2C Slave Address Register. Contains the 7-bit slave address for operation of the I2C interface in slave mode, and is not used in master mode. The least significant bit determines whether a slave responds to the General Call address. */
  __IO uint32_t ADR2;                       /*!< (@ 0x40000024) I2C Slave Address Register. Contains the 7-bit slave address for operation of the I2C interface in slave mode, and is not used in master mode. The least significant bit determines whether a slave responds to the General Call address. */
  __IO uint32_t ADR3;                       /*!< (@ 0x40000028) I2C Slave Address Register. Contains the 7-bit slave address for operation of the I2C interface in slave mode, and is not used in master mode. The least significant bit determines whether a slave responds to the General Call address. */
  __I  uint32_t DATA_BUFFER;                /*!< (@ 0x4000002C) Data buffer register. The contents of the 8 MSBs of the I2DAT shift register will be transferred to the DATA_BUFFER automatically after every nine bits (8 bits of data plus ACK or NACK) has been received on the bus. */
  __IO uint32_t MASK[4];                    /*!< (@ 0x40000030) I2C Slave address mask register. This mask register is associated with I2ADR0 to determine an address match. The mask register has no effect when comparing to the General Call address (0000000). */
} LPC_I2C_Type;


// ------------------------------------------------------------------------------------------------
// -----                                         WWDT                                         -----
// ------------------------------------------------------------------------------------------------


/**
  * @brief Product name title=UM10441 Chapter title=LPC122x Windowed Watchdog Timer (WWDT) Modification date=2/2/2011 Major revision=0 Minor revision=17  (WWDT)
  */

typedef struct {                            /*!< (@ 0x40004000) WWDT Structure         */
  __IO uint32_t MOD;                        /*!< (@ 0x40004000) Watchdog mode register. This register contains the basic mode and status of the Watchdog Timer. */
  __IO uint32_t TC;                         /*!< (@ 0x40004004) Watchdog timer constant register. This register determines the time-out value. */
  __IO uint32_t FEED;                       /*!< (@ 0x40004008) Watchdog feed sequence register. Writing 0xAA followed by 0x55 to this register reloads the Watchdog timer with the value contained in TC. */
  __I  uint32_t TV;                         /*!< (@ 0x4000400C) Watchdog timer value register. This register reads out the current value of the Watchdog timer. */
  __IO uint32_t CLKSEL;                     /*!< (@ 0x40004010) Watchdog clock source selection register. */
  __IO uint32_t WARNINT;                    /*!< (@ 0x40004014) Watchdog Warning Interrupt compare value. */
  __IO uint32_t WINDOW;                     /*!< (@ 0x40004018) Watchdog Window compare value. */
} LPC_WWDT_Type;


// ------------------------------------------------------------------------------------------------
// -----                                         UART0                                        -----
// ------------------------------------------------------------------------------------------------


/**
  * @brief Product name title=UM10441 Chapter title=LPC122x UART0 with modem control Modification date=2/2/2011 Major revision=0 Minor revision=17  (UART0)
  */

typedef struct {                            /*!< (@ 0x40008000) UART0 Structure        */
  
  union {
    __IO uint32_t DLL;                      /*!< (@ 0x40008000) Divisor Latch LSB. Least significant byte of the baud rate divisor value. The full divisor is used to generate a baud rate from the fractional rate divider. (DLAB = 1) */
    __IO uint32_t THR;                      /*!< (@ 0x40008000) Transmit Holding Register. The next character to be transmitted is written here. (DLAB=0) */
    __I  uint32_t RBR;                      /*!< (@ 0x40008000) Receiver Buffer Register. Contains the next received character to be read. (DLAB=0) */
  };
  
  union {
    __IO uint32_t IER;                      /*!< (@ 0x40008004) Interrupt Enable Register. Contains individual interrupt enable bits for the 7 potential UART interrupts. (DLAB=0) */
    __IO uint32_t DLM;                      /*!< (@ 0x40008004) Divisor Latch MSB. Most significant byte of the baud rate divisor value. The full divisor is used to generate a baud rate from the fractional rate divider. (DLAB = 1) */
  };
  
  union {
    __IO uint32_t FCR;                      /*!< (@ 0x40008008) FIFO Control Register. Controls UART FIFO usage and modes. */
    __I  uint32_t IIR;                      /*!< (@ 0x40008008) Interrupt ID Register. Identifies which interrupt(s) are pending. */
  };
  __IO uint32_t LCR;                        /*!< (@ 0x4000800C) Line Control Register. Contains controls for frame formatting and break generation. */
  __IO uint32_t MCR;                        /*!< (@ 0x40008010) Modem control register */
  __I  uint32_t LSR;                        /*!< (@ 0x40008014) Line Status Register. Contains flags for transmit and receive status, including line errors. */
  __I  uint32_t MSR;                        /*!< (@ 0x40008018) Modem status register  */
  __IO uint32_t SCR;                        /*!< (@ 0x4000801C) Scratch Pad Register. Eight-bit temporary storage for software. */
  __IO uint32_t ACR;                        /*!< (@ 0x40008020) Auto-baud Control Register. Contains controls for the auto-baud feature. */
  __I  uint32_t RESERVED0[1];
  __IO uint32_t FDR;                        /*!< (@ 0x40008028) Fractional Divider Register. Generates a clock input for the baud rate divider. */
  __I  uint32_t RESERVED1[1];
  __IO uint32_t TER;                        /*!< (@ 0x40008030) Transmit Enable Register. Turns off UART transmitter for use with software flow control. */
  __I  uint32_t RESERVED2[6];
  __IO uint32_t RS485CTRL;                  /*!< (@ 0x4000804C) RS-485/EIA-485 Control. Contains controls to configure various aspects of RS-485/EIA-485 modes. */
  __IO uint32_t RS485ADRMATCH;              /*!< (@ 0x40008050) RS-485/EIA-485 address match. Contains the address match value for RS-485/EIA-485 mode. */
  __IO uint32_t RS485DLY;                   /*!< (@ 0x40008054) RS-485/EIA-485 direction control delay. */
  __I  uint32_t FIFOLVL;                    /*!< (@ 0x40008058) FIFO Level register. Provides the current fill levels of the transmit and receive FIFOs. */
} LPC_UART0_Type;


// ------------------------------------------------------------------------------------------------
// -----                                         UART1                                        -----
// ------------------------------------------------------------------------------------------------


/**
  * @brief Product name title=UM10441 Chapter title=LPC122x UART1 Modification date=2/3/2011 Major revision=0 Minor revision=17  (UART1)
  */

typedef struct {                            /*!< (@ 0x4000C000) UART1 Structure        */
  
  union {
    __IO uint32_t DLL;                      /*!< (@ 0x4000C000) Divisor Latch LSB. Least significant byte of the baud rate divisor value. The full divisor is used to generate a baud rate from the fractional rate divider. */
    __IO uint32_t THR;                      /*!< (@ 0x4000C000) Transmit Holding Register. The next character to be transmitted is written here. */
    __I  uint32_t RBR;                      /*!< (@ 0x4000C000) Receiver Buffer Register. Contains the next received character to be read. */
  };
  
  union {
    __IO uint32_t IER;                      /*!< (@ 0x4000C004) Interrupt Enable Register. Contains individual interrupt enable bits for the 7 potential UART interrupts. */
    __IO uint32_t DLM;                      /*!< (@ 0x4000C004) Divisor Latch MSB. Most significant byte of the baud rate divisor value. The full divisor is used to generate a baud rate from the fractional rate divider. */
  };
  
  union {
    __IO uint32_t FCR;                      /*!< (@ 0x4000C008) FIFO Control Register. Controls UART FIFO usage and modes. */
    __I  uint32_t IIR;                      /*!< (@ 0x4000C008) Interrupt ID Register. Identifies which interrupt(s) are pending. */
  };
  __IO uint32_t LCR;                        /*!< (@ 0x4000C00C) Line Control Register. Contains controls for frame formatting and break generation. */
  __I  uint32_t RESERVED0[1];
  __I  uint32_t LSR;                        /*!< (@ 0x4000C014) Line Status Register. Contains flags for transmit and receive status, including line errors. */
  __I  uint32_t RESERVED1[1];
  __IO uint32_t SCR;                        /*!< (@ 0x4000C01C) Scratch Pad Register. Eight-bit temporary storage for software. */
  __IO uint32_t ACR;                        /*!< (@ 0x4000C020) Auto-baud Control Register. Contains controls for the auto-baud feature. */
  __IO uint32_t ICR;                        /*!< (@ 0x4000C024) IrDA Control Register. Enables and configures the IrDA mode. */
  __IO uint32_t FDR;                        /*!< (@ 0x4000C028) Fractional Divider Register. Generates a clock input for the baud rate divider. */
  __I  uint32_t RESERVED2[1];
  __IO uint32_t TER;                        /*!< (@ 0x4000C030) Transmit Enable Register. Turns off UART transmitter for use with software flow control. */
  __I  uint32_t RESERVED3[9];
  __I  uint32_t FIFOLVL;                    /*!< (@ 0x4000C058) FIFO Level register. Provides the current fill levels of the transmit and receive FIFOs. */
} LPC_UART1_Type;

// ------------------------------------------------------------------------------------------------
// -----                                        CT32B0                                        -----
// ------------------------------------------------------------------------------------------------


/**
  * @brief Product name title=UM10441 Chapter title=LPC122x 32-bit Counter/timer 0/1 (CT32B0/1) Modification date=2/3/2011 Major revision=0 Minor revision=17  (CT32B0)
  */

typedef struct {                            /*!< (@ 0x40018000) CT32B0 Structure       */
  __IO uint32_t IR;                         /*!< (@ 0x40018000) Interrupt Register. The IR can be written to clear interrupts. The IR can be read to identify which of eight possible interrupt sources are pending. */
  __IO uint32_t TCR;                        /*!< (@ 0x40018004) Timer Control Register. The TCR is used to control the Timer Counter functions. The Timer Counter can be disabled or reset through the TCR. */
  __IO uint32_t TC;                         /*!< (@ 0x40018008) Timer Counter. The 32-bit TC is incremented every PR+1 cycles of PCLK. The TC is controlled through the TCR. */
  __IO uint32_t PR;                         /*!< (@ 0x4001800C) Prescale Register. When the Prescale Counter (below) is equal to this value, the next clock increments the TC and clears the PC. */
  __IO uint32_t PC;                         /*!< (@ 0x40018010) Prescale Counter. The 32-bit PC is a counter which is incremented to the value stored in PR. When the value in PR is reached, the TC is incremented and the PC is cleared. The PC is observable and controllable through the bus interface. */
  __IO uint32_t MCR;                        /*!< (@ 0x40018014) Match Control Register. The MCR is used to control if an interrupt is generated and if the TC is reset when a Match occurs. */
  union {
  __IO uint32_t MR[4];                      /*!< (@ 0x40018018) Match Register. MR can be enabled through the MCR to reset the TC, stop both the TC and PC, and/or generate an interrupt every time MR matches the TC. */
  struct{
  __IO uint32_t MR0;                        /*!< (@ 0x40018018) Match Register. MR0 */
  __IO uint32_t MR1;                        /*!< (@ 0x40018018) Match Register. MR1 */
  __IO uint32_t MR2;                        /*!< (@ 0x40018018) Match Register. MR2 */
  __IO uint32_t MR3;                        /*!< (@ 0x40018018) Match Register. MR3 */
  };
  };
  __IO uint32_t CCR;                        /*!< (@ 0x40018028) Capture Control Register. The CCR controls which edges of the capture inputs are used to load the Capture Registers and whether or not an interrupt is generated when a capture takes place. */
  union{
  __I  uint32_t CR[4];                      /*!< (@ 0x4001802C) Capture Register. CR is loaded with the value of TC when there is an event on the CT32B_CAP input. */
  struct{
  __I  uint32_t CR0;						/*!< (@ 0x4001802C) Capture Register. CR 0 */
  __I  uint32_t CR1;						/*!< (@ 0x4001802C) Capture Register. CR 1 */
  __I  uint32_t CR2;						/*!< (@ 0x4001802C) Capture Register. CR 2 */
  __I  uint32_t CR3;						/*!< (@ 0x4001802C) Capture Register. CR 3 */
  };
  };
  __IO uint32_t EMR;                        /*!< (@ 0x4001803C) External Match Register. The EMR controls the match function and the external match pins CT32Bn_MAT[3:0]. */
  __I  uint32_t RESERVED0[12];
  __IO uint32_t CTCR;                       /*!< (@ 0x40018070) Count Control Register. The CTCR selects between Timer and Counter mode, and in Counter mode selects the signal and edge(s) for counting. */
  __IO uint32_t PWMC;                       /*!< (@ 0x40018074) PWM Control Register. The PWMCON enables PWM mode for the external match pins CT32Bn_MAT[3:0]. */
} LPC_CTxxBx_Type;


// ------------------------------------------------------------------------------------------------
// -----                                          ADC                                         -----
// ------------------------------------------------------------------------------------------------


/**
  * @brief Product name title=UM10441 Chapter title=LPC122x ADC Modification date=2/9/2011 Major revision=0 Minor revision=17  (ADC)
  */

typedef struct {                            /*!< (@ 0x40020000) ADC Structure          */
  __IO uint32_t CR;                         /*!< (@ 0x40020000) A/D Control Register. The CR register must be written to select the operating mode before A/D conversion can occur. */
  __IO uint32_t GDR;                        /*!< (@ 0x40020004) A/D Global Data Register. Contains the result of the most recent A/D conversion. */
  __I  uint32_t RESERVED0[1];
  __IO uint32_t INTEN;                      /*!< (@ 0x4002000C) A/D Interrupt Enable Register. This register contains enable bits that allow the DONE flag of each A/D channel to be included or excluded from contributing to the generation of an A/D interrupt. */
  union{
  __IO uint32_t DR[8];                  	/*!< (@ 0x40020010) A/D Channel Data Register. This register contains the result of the most recent conversion completed on channel. */
  struct{
  __IO uint32_t DR0;                      	/*!< (@ 0x40020010) A/D Channel Data Register 0*/
  __IO uint32_t DR1;                      	/*!< (@ 0x40020010) A/D Channel Data Register 1*/
  __IO uint32_t DR2;                      	/*!< (@ 0x40020010) A/D Channel Data Register 2*/
  __IO uint32_t DR3;                      	/*!< (@ 0x40020010) A/D Channel Data Register 3*/
  __IO uint32_t DR4;                      	/*!< (@ 0x40020010) A/D Channel Data Register 4*/
  __IO uint32_t DR5;                      	/*!< (@ 0x40020010) A/D Channel Data Register 5*/
  __IO uint32_t DR6;                      	/*!< (@ 0x40020010) A/D Channel Data Register 6*/
  __IO uint32_t DR7;                      	/*!< (@ 0x40020010) A/D Channel Data Register 7*/
  };
  };
  __I  uint32_t STAT;                       /*!< (@ 0x40020030) A/D Status Register. This register contains DONE and OVERRUN flags for all of the A/D channels, as well as the A/D interrupt flag. */
  __IO uint32_t TRM;                        /*!< (@ 0x40020034) A/D trim register      */
} LPC_ADC_Type;


// ------------------------------------------------------------------------------------------------
// -----                                          PMU                                         -----
// ------------------------------------------------------------------------------------------------


/**
  * @brief Product name title=UM10441 Chapter title=LPC122x Power Monitor Unit (PMU) Modification date=2/10/2011 Major revision=0 Minor revision=17  (PMU)
  */

typedef struct {                            /*!< (@ 0x40038000) PMU Structure          */
  __IO uint32_t PCON;                       /*!< (@ 0x40038000) Power control register */
  union{
  __IO uint32_t GPREG[4];                   /*!< (@ 0x40038004) General purpose register  */
  struct{
  __IO uint32_t GPREG0;                   	/*!< (@ 0x40038004) General purpose register 0 */
  __IO uint32_t GPREG1;                   	/*!< (@ 0x40038004) General purpose register 1 */
  __IO uint32_t GPREG2;                   	/*!< (@ 0x40038004) General purpose register 2 */
  __IO uint32_t GPREG3;                   	/*!< (@ 0x40038004) General purpose register 3 */
  };
  };
  __IO uint32_t SYSCFG;                     /*!< (@ 0x40038014) System configuration register (RTC clock control and hysteresis of the WAKEUP pin). */
} LPC_PMU_Type;


// ------------------------------------------------------------------------------------------------
// -----                                          SSP                                         -----
// ------------------------------------------------------------------------------------------------


/**
  * @brief Product name title=UM10441 Chapter title=LPC122x SSP controller Modification date=2/10/2011 Major revision=0 Minor revision=17  (SSP)
  */

typedef struct {                            /*!< (@ 0x40040000) SSP Structure          */
	union{
  __IO uint32_t CR[2];						/*!< (@ 0x40040000) Control Registers. */
  struct{
  __IO uint32_t CR0;                        /*!< (@ 0x40040000) Control Register 0. Selects the serial clock rate, bus type, and data size. */
  __IO uint32_t CR1;                        /*!< (@ 0x40040004) Control Register 1. Selects master/slave and other modes. */
  };
  };
  __IO uint32_t DR;                         /*!< (@ 0x40040008) Data Register. Writes fill the transmit FIFO, and reads empty the receive FIFO. */
  __I  uint32_t SR;                         /*!< (@ 0x4004000C) Status Register        */
  __IO uint32_t CPSR;                       /*!< (@ 0x40040010) Clock Prescale Register */
  __IO uint32_t IMSC;                       /*!< (@ 0x40040014) Interrupt Mask Set and Clear Register */
  __I  uint32_t RIS;                        /*!< (@ 0x40040018) Raw Interrupt Status Register */
  __I  uint32_t MIS;                        /*!< (@ 0x4004001C) Masked Interrupt Status Register */
  __IO uint32_t ICR;                        /*!< (@ 0x40040020) SSPICR Interrupt Clear Register */
  __IO uint32_t DMACR;                      /*!< (@ 0x40040024) DMA Control Register   */
} LPC_SSP_Type;


// ------------------------------------------------------------------------------------------------
// -----                                         IOCON                                        -----
// ------------------------------------------------------------------------------------------------


/**
  * @brief Product name title=UM10441 Chapter title=LPC122x /O configuration (IOCONFIG) Modification date=2/11/2011 Major revision=1 Minor revision=0  (IOCON)
  */

typedef struct {                            /*!< (@ 0x40044000) IOCON Structure        */
  __I  uint32_t RESERVED0[2];
  __IO uint32_t PIO0_19;                    /*!< (@ 0x40044008) Configures pin PIO0_19/ACMP0_I0/CT32B0_1. */
  __IO uint32_t PIO0_20;                    /*!< (@ 0x4004400C) Configures pin PIO0_20/ACMP0_I1/CT32B0_2. */
  __IO uint32_t PIO0_21;                    /*!< (@ 0x40044010) Configures pin PIO0_21/ACMP0_I2/CT32B0_3. */
  __IO uint32_t PIO0_22;                    /*!< (@ 0x40044014) Configures pin PIO0_22/ACMP0_I3. */
  __IO uint32_t PIO0_23;                    /*!< (@ 0x40044018) Configures pin PIO0_23/ACMP1_I0/CT32B1_0. */
  __IO uint32_t PIO0_24;                    /*!< (@ 0x4004401C) Configures pin PIO0_24/ACMP1_I1/CT32B1_1. */
  __IO uint32_t SWDIO_PIO0_25;              /*!< (@ 0x40044020) Configures pin SWDIO/ACMP1_I2/ CT32B1_2/PIO0_25. */
  __IO uint32_t SWCLK_PIO0_26;              /*!< (@ 0x40044024) Configures pin SWCLK/PIO0_26/ACMP1_I3/ CT32B1_3/PIO0_26 */
  __IO uint32_t PIO0_27;                    /*!< (@ 0x40044028) Configures pin PIO0_27/ACMP0_O. */
  __IO uint32_t PIO2_12;                    /*!< (@ 0x4004402C) Configures pin PIO2_12/RXD1. */
  __IO uint32_t PIO2_13;                    /*!< (@ 0x40044030) Configures pin PIO2_13/TXD1. */
  __IO uint32_t PIO2_14;                    /*!< (@ 0x40044034) Configures pin PIO2_14. */
  __IO uint32_t PIO2_15;                    /*!< (@ 0x40044038) Configures pin PIO2_15. */
  __IO uint32_t PIO0_28;                    /*!< (@ 0x4004403C) Configures pin PIO0_28/ACMP1_O/CT16B0_0. */
  __IO uint32_t PIO0_29;                    /*!< (@ 0x40044040) Configures pin PIO0_29/ROSC/CT16B0_1. */
  __IO uint32_t PIO0_0;                     /*!< (@ 0x40044044) Configures pin PIO0_0/ RTS0. */
  __IO uint32_t PIO0_1;                     /*!< (@ 0x40044048) Configures pin PIO0_1CT32B0_0/RXD0. */
  __IO uint32_t PIO0_2;                     /*!< (@ 0x4004404C) Configures pin PIO0_2/TXD0/CT32B0_1. */
  __I  uint32_t RESERVED1[1];
  __IO uint32_t PIO0_3;                     /*!< (@ 0x40044054) Configures pin PIO0_3/DTR0/CT32B0_2. */
  __IO uint32_t PIO0_4;                     /*!< (@ 0x40044058) Configures pin PIO0_4/ DSR0/CT32B0_3. */
  __IO uint32_t PIO0_5;                     /*!< (@ 0x4004405C) Configures pin PIO0_5. */
  __IO uint32_t PIO0_6;                     /*!< (@ 0x40044060) Configures pin PIO0_6/RI0/CT32B1_0. */
  __IO uint32_t PIO0_7;                     /*!< (@ 0x40044064) Configures pin PIO0_7CTS0/CT32B1_1. */
  __IO uint32_t PIO0_8;                     /*!< (@ 0x40044068) Configures pin PIO0_8/RXD1/CT32B1_2. */
  __IO uint32_t PIO0_9;                     /*!< (@ 0x4004406C) Configures pin PIO0_9/TXD1/CT32B1_3. */
  __IO uint32_t PIO2_0;                     /*!< (@ 0x40044070) Configures pin PIO2_0/CT16B0_0/ RTS0. */
  __IO uint32_t PIO2_1;                     /*!< (@ 0x40044074) Configures pin PIO2_1/CT16B0_1/RXD0. */
  __IO uint32_t PIO2_2;                     /*!< (@ 0x40044078) Configures pin PIO2_2/CT16B1_0/TXD0. */
  __IO uint32_t PIO2_3;                     /*!< (@ 0x4004407C) Configures pin PIO2_3/CT16B1_1/DTR0. */
  __IO uint32_t PIO2_4;                     /*!< (@ 0x40044080) Configures pin PIO2_4/CT32B0_0/CTS0. */
  __IO uint32_t PIO2_5;                     /*!< (@ 0x40044084) Configures pin PIO2_5/CT32B0_1/ DCD0. */
  __IO uint32_t PIO2_6;                     /*!< (@ 0x40044088) Configures pin PIO2_6/CT32B0_2/RI0. */
  __IO uint32_t PIO2_7;                     /*!< (@ 0x4004408C) Configures pin PIO2_7/CT32B0_3/DSR0. */
  __IO uint32_t PIO0_10;                    /*!< (@ 0x40044090) Configures pin PIO0_10/SCL. */
  __IO uint32_t PIO0_11;                    /*!< (@ 0x40044094) Configures pin PIO0_11/SDA/CT16B0_0. */
  __IO uint32_t PIO0_12;                    /*!< (@ 0x40044098) Configures pin PIO0_12/CLKOUT/CT16B0_1. */
  __IO uint32_t RESET_PIO0_13;              /*!< (@ 0x4004409C) Configures pin RESET/PIO0_13. */
  __IO uint32_t PIO0_14;                    /*!< (@ 0x400440A0) Configures pin PIO0_14/SSP_CLK. */
  __IO uint32_t PIO0_15;                    /*!< (@ 0x400440A4) Configures pin PIO0_15/SSP_SSEL/CT16B1_0. */
  __IO uint32_t PIO0_16;                    /*!< (@ 0x400440A8) Configures pin PIO0_16/SSP_MISO/CT16B1_1. */
  __IO uint32_t PIO0_17;                    /*!< (@ 0x400440AC) Configures pin PIO0_17/SSP_MOSI. */
  __IO uint32_t PIO0_18;                    /*!< (@ 0x400440B0) Configures pin PIO0_18/SWCLK/CT32B0_0. */
  __IO uint32_t R_PIO0_30;                  /*!< (@ 0x400440B4) Configures pin R/PIO0_30/AD0. */
  __IO uint32_t R_PIO0_31;                  /*!< (@ 0x400440B8) Configures pin R/PIO0_31/AD1. */
  __IO uint32_t R_PIO1_0;                   /*!< (@ 0x400440BC) Configures pin R/PIO1_0/AD2. */
  __IO uint32_t R_PIO1_1;                   /*!< (@ 0x400440C0) Configures pin R/PIO1_1/AD3. */
  __IO uint32_t PIO1_2;                     /*!< (@ 0x400440C4) Configures pin PIO1_2/SWDIO/AD4. */
  __IO uint32_t PIO1_3;                     /*!< (@ 0x400440C8) Configures pin PIO1_3/AD5/WAKEUP. */
  __IO uint32_t PIO1_4;                     /*!< (@ 0x400440CC) Configures pin PIO1_4/AD6 */
  __IO uint32_t PIO1_5;                     /*!< (@ 0x400440D0) Configures pin PIO1_5/AD7/CT16B1_0. */
  __IO uint32_t PIO1_6;                     /*!< (@ 0x400440D4) Configures pin PIO1_6/CT16B1_1. */
  __I  uint32_t RESERVED2[2];
  __IO uint32_t PIO2_8;                     /*!< (@ 0x400440E0) Configures pin PIO2_8/CT32B1_0. */
  __IO uint32_t PIO2_9;                     /*!< (@ 0x400440E4) Configures pin PIO2_9/CT32B1_1. */
  __IO uint32_t PIO2_10;                    /*!< (@ 0x400440E8) Configures pin PIO2_10/CT32B1_2/TXD1. */
  __IO uint32_t PIO2_11;                    /*!< (@ 0x400440EC) Configures pin PIO2_11/CT32B1_3/RXD1. */
} LPC_IOCON_Type;


// ------------------------------------------------------------------------------------------------
// -----                                        SYSCON                                        -----
// ------------------------------------------------------------------------------------------------


/**
  * @brief Product name title=UM10441 Chapter title=LPC122x System control (SYSCON) Modification date=2/10/2011 Major revision=1 Minor revision=0  (SYSCON)
  */

typedef struct {                            /*!< (@ 0x40048000) SYSCON Structure       */
  __IO uint32_t SYSMEMREMAP;                /*!< (@ 0x40048000) System memory remap    */
  __IO uint32_t PRESETCTRL;                 /*!< (@ 0x40048004) Peripheral reset control and flash timing overwrite */
  __IO uint32_t SYSPLLCTRL;                 /*!< (@ 0x40048008) System PLL control     */
  __I  uint32_t SYSPLLSTAT;                 /*!< (@ 0x4004800C) System PLL status      */
  __I  uint32_t RESERVED0[4];
  __IO uint32_t SYSOSCCTRL;                 /*!< (@ 0x40048020) System oscillator control */
  __IO uint32_t WDTOSCCTRL;                 /*!< (@ 0x40048024) Watchdog oscillator control */
  __IO uint32_t IRCCTRL;                    /*!< (@ 0x40048028) IRC control            */
  __I  uint32_t RESERVED1[1];
  __IO uint32_t SYSRESSTAT;                 /*!< (@ 0x40048030) System reset status register */
  __I  uint32_t RESERVED2[3];
  __IO uint32_t SYSPLLCLKSEL;               /*!< (@ 0x40048040) System PLL clock source select */
  __IO uint32_t SYSPLLCLKUEN;               /*!< (@ 0x40048044) System PLL clock source update enable */
  __I  uint32_t RESERVED3[10];
  __IO uint32_t MAINCLKSEL;                 /*!< (@ 0x40048070) Main clock source select */
  __IO uint32_t MAINCLKUEN;                 /*!< (@ 0x40048074) Main clock source update enable */
  __IO uint32_t SYSAHBCLKDIV;               /*!< (@ 0x40048078) System AHB clock divider */
  __I  uint32_t RESERVED4[1];
  __IO uint32_t SYSAHBCLKCTRL;              /*!< (@ 0x40048080) System AHB clock control */
  __I  uint32_t RESERVED5[4];
  __IO uint32_t SSPCLKDIV;                  /*!< (@ 0x40048094) SSP clock divder       */
  __IO uint32_t UART0CLKDIV;                /*!< (@ 0x40048098) UART0 clock divider    */
  __IO uint32_t UART1CLKDIV;                /*!< (@ 0x4004809C) UART1 clock divider    */
  __IO uint32_t RTCCLKDIV;                  /*!< (@ 0x400480A0) RTC clock divider      */
  __I  uint32_t RESERVED6[15];
  __IO uint32_t CLKOUTCLKSEL;               /*!< (@ 0x400480E0) CLKOUT clock source select */
  __IO uint32_t CLKOUTUEN;                  /*!< (@ 0x400480E4) CLKOUT clock source update enable */
  __IO uint32_t CLKOUTDIV;                  /*!< (@ 0x400480E8) CLKOUT clock divider   */
  __I  uint32_t RESERVED7[5];
  __I  uint32_t PIOPORCAP0;                 /*!< (@ 0x40048100) POR captured PIO status 0 */
  __I  uint32_t PIOPORCAP1;                 /*!< (@ 0x40048104) POR captured PIO status 1 */
  __I  uint32_t RESERVED8[11];
  __IO uint32_t IOCONFIGCLKDIV6;            /*!< (@ 0x40048134) Peripheral clock 6 to the IOCONFIG block for programmable glitch filter */
  __IO uint32_t IOCONFIGCLKDIV5;            /*!< (@ 0x40048138) Peripheral clock 5 to the IOCONFIG block for programmable glitch filter */
  __IO uint32_t IOCONFIGCLKDIV4;            /*!< (@ 0x4004813C) Peripheral clock 4 to the IOCONFIG block for programmable glitch filter */
  __IO uint32_t IOCONFIGCLKDIV3;            /*!< (@ 0x40048140) Peripheral clock 3to the IOCONFIG block for programmable glitch filter */
  __IO uint32_t IOCONFIGCLKDIV2;            /*!< (@ 0x40048144) Peripheral clock 2 to the IOCONFIG block for programmable glitch filter */
  __IO uint32_t IOCONFIGCLKDIV1;            /*!< (@ 0x40048148) Peripheral clock 1 to the IOCONFIG block for programmable glitch filter */
  __IO uint32_t IOCONFIGCLKDIV0;            /*!< (@ 0x4004814C) Peripheral clock 0 to the IOCONFIG block for programmable glitch filter */
  __IO uint32_t BODCTRL;                    /*!< (@ 0x40048150) BOD control            */
  __IO uint32_t SYSTCKCAL;                  /*!< (@ 0x40048154) System tick counter calibration */
  __IO uint32_t AHBPRIO;                    /*!< (@ 0x40048158) AHB priority setting   */
  __I  uint32_t RESERVED9[5];
  __IO uint32_t IRQLATENCY;                 /*!< (@ 0x40048170) IQR delay. Allows trade-off between interrupt latency and determinism. */
  __IO uint32_t INTNMI;                     /*!< (@ 0x40048174) NMI interrupt source configuration control */
  __I  uint32_t RESERVED10[34];
  __IO uint32_t STARTAPRP0;                 /*!< (@ 0x40048200) Start logic edge control register 0 */
  __IO uint32_t STARTERP0;                  /*!< (@ 0x40048204) Start logic signal enable register 0 */
  __IO uint32_t STARTRSRP0CLR;              /*!< (@ 0x40048208) Start logic reset register 0 */
  __I  uint32_t STARTSRP0;                  /*!< (@ 0x4004820C) Start logic status register 0 */
  __IO uint32_t STARTAPRP1;                 /*!< (@ 0x40048210) Start logic edge control register 1; peripheral interrupts */
  __IO uint32_t STARTERP1;                  /*!< (@ 0x40048214) Start logic signal enable register 1; peripheral interrupts */
  __IO uint32_t STARTRSRP1CLR;              /*!< (@ 0x40048218) Start logic reset register 1; peripheral interrupts */
  __I  uint32_t STARTSRP1;                  /*!< (@ 0x4004821C) Start logic status register 1; peripheral interrupts */
  __I  uint32_t RESERVED11[4];
  __IO uint32_t PDSLEEPCFG;                 /*!< (@ 0x40048230) Power-down states in Deep-sleep mode */
  __IO uint32_t PDAWAKECFG;                 /*!< (@ 0x40048234) Power-down states after wake-up from Deep-sleep mode */
  __IO uint32_t PDRUNCFG;                   /*!< (@ 0x40048238) Power-down configuration register */
  __I  uint32_t RESERVED12[110];
  __I  uint32_t DEVICE_ID;                  /*!< (@ 0x400483F4) Device ID              */
} LPC_SYSCON_Type;


// ------------------------------------------------------------------------------------------------
// -----                                       MICRO_DMA                                      -----
// ------------------------------------------------------------------------------------------------


/**
  * @brief Product name title=UM10441 Chapter title=LPC122x General purpose micro DMA controller Modification date=2/10/2011 Major revision=0 Minor revision=17  (MICRO_DMA)
  */

typedef struct {                            /*!< (@ 0x4004C000) MICRO_DMA Structure    */
  __I  uint32_t DMA_STATUS;                 /*!< (@ 0x4004C000) DMA status register    */
  __IO uint32_t DMA_CFG;                    /*!< (@ 0x4004C004) DMA configuration register */
  __IO uint32_t CTRL_BASE_PTR;              /*!< (@ 0x4004C008) Channel control base pointer register */
  __I  uint32_t ATL_CTRL_BASE_PTR;          /*!< (@ 0x4004C00C) Channel alternate control base pointer register */
  __I  uint32_t DMA_WAITONREQ_STATUS;       /*!< (@ 0x4004C010) Channel wait on request status register */
  __IO uint32_t CHNL_SW_REQUEST;            /*!< (@ 0x4004C014) Channel software request register */
  __IO uint32_t CHNL_USEBURST_SET;          /*!< (@ 0x4004C018) Channel useburst set register */
  __IO uint32_t CHNL_USEBURST_CLR;          /*!< (@ 0x4004C01C) Channel useburst clear register */
  __IO uint32_t CHNL_REQ_MASK_SET;          /*!< (@ 0x4004C020) Channel request mask set register */
  __IO uint32_t CHNL_REQ_MASK_CLR;          /*!< (@ 0x4004C024) Channel request mask clear register */
  __IO uint32_t CHNL_ENABLE_SET;            /*!< (@ 0x4004C028) Channel enable set register */
  __IO uint32_t CHNL_ENABLE_CLR;            /*!< (@ 0x4004C02C) Channel enable clear register */
  __IO uint32_t CHNL_PRI_ALT_SET;           /*!< (@ 0x4004C030) Channel primary-alternate set register */
  __IO uint32_t CHNL_PRI_ALT_CLR;           /*!< (@ 0x4004C034) Channel primary-alternate clear register */
  __IO uint32_t CHNL_PRIORITY_SET;          /*!< (@ 0x4004C038) Channel priority set register */
  __IO uint32_t CHNL_PRIORITY_CLR;          /*!< (@ 0x4004C03C) Channel priority clear register */
  __I  uint32_t RESERVED0[3];
  __IO uint32_t ERR_CLR;                    /*!< (@ 0x4004C04C) Bus error clear register */
  __I  uint32_t RESERVED1[12];
  __IO uint32_t CHNL_IRQ_STATUS;            /*!< (@ 0x4004C080) Channel DMA interrupt status register */
  __IO uint32_t IRQ_ERR_ENABLE;             /*!< (@ 0x4004C084) DMA error interrupt enable register */
  __IO uint32_t CHNL_IRQ_ENABLE;            /*!< (@ 0x4004C088) Channel DMA interrupt enable register */
} LPC_MICRO_DMA_Type;


// ------------------------------------------------------------------------------------------------
// -----                                          RTC                                         -----
// ------------------------------------------------------------------------------------------------


/**
  * @brief Product name title=UM10441 Chapter title=LPC122x Real-Time Clock (RTC) Modification date=2/11/2011 Major revision=1 Minor revision=0  (RTC)
  */

typedef struct {                            /*!< (@ 0x40050000) RTC Structure          */
  __I  uint32_t DR;                         /*!< (@ 0x40050000) Data register          */
  __IO uint32_t MR;                         /*!< (@ 0x40050004) Match register         */
  __IO uint32_t LR;                         /*!< (@ 0x40050008) Load register          */
  __IO uint32_t CR;                         /*!< (@ 0x4005000C) Control register       */
  __IO uint32_t ICSC;                       /*!< (@ 0x40050010) Interrupt control set/clear register */
  __I  uint32_t RIS;                        /*!< (@ 0x40050014) Raw interrupt status register */
  __I  uint32_t MIS;                        /*!< (@ 0x40050018) Masked interrupt status register */
  __IO uint32_t ICR;                        /*!< (@ 0x4005001C) Interrupt clear register */
} LPC_RTC_Type;


// ------------------------------------------------------------------------------------------------
// -----                                         ACOMP                                        -----
// ------------------------------------------------------------------------------------------------


/**
  * @brief Product name title=UM10441 Chapter title=LPC122x Comparator Modification date=2/11/2011 Major revision=0 Minor revision=17  (ACOMP)
  */

typedef struct {                            /*!< (@ 0x40054000) ACOMP Structure        */
  __IO uint32_t CMP;                        /*!< (@ 0x40054000) Comparator control register */
  __IO uint32_t VLAD;                       /*!< (@ 0x40054004) Voltage ladder register */
} LPC_ACOMP_Type;


// ------------------------------------------------------------------------------------------------
// -----                                         GPIO0/1/2                                    -----
// ------------------------------------------------------------------------------------------------


/**
  * @brief Product name title=UM10441 Chapter title=LPC122x General Purpose I/O (GPIO) Modification date=2/11/2011 Major revision=0 Minor revision=17  (GPIO0)
  */

typedef struct {                            /*!< (@ 0x500X0000) GPIO0 Structure        */
  __IO uint32_t MASK;                       /*!< (@ 0x500X0000) Pin value mask register. Affects operations on PIN, OUT, SET, CLR, and NOT registers. */
  __I  uint32_t PIN;                        /*!< (@ 0x500X0004) Pin value register.    */
  __IO uint32_t OUT;                        /*!< (@ 0x500X0008) Pin output value register. */
  __IO uint32_t SET;                        /*!< (@ 0x500X000C) Pin output value set register. */
  __IO uint32_t CLR;                        /*!< (@ 0x500X0010) Pin output value clear register. */
  __IO uint32_t NOT;                        /*!< (@ 0x500X0014) Pin output value invert register. */
  __I  uint32_t RESERVED0[2];
  __IO uint32_t DIR;                        /*!< (@ 0x500X0020) Data direction register. */
  __IO uint32_t IS;                         /*!< (@ 0x500X0024) Interrupt sense register. */
  __IO uint32_t IBE;                        /*!< (@ 0x500X0028) Interrupt both edges register. */
  __IO uint32_t IEV;                        /*!< (@ 0x500X002C) Interrupt event register. */
  __IO uint32_t IE;                         /*!< (@ 0x500X0030) Interrupt mask register. */
  __I  uint32_t RIS;                        /*!< (@ 0x500X0034) Raw interrupt status register. */
  __I  uint32_t MIS;                        /*!< (@ 0x500X0038) Masked interrupt status register. */
  __IO uint32_t IC;                         /*!< (@ 0x5000003C) Interrupt clear register. */
} LPC_GPIO_Type;


// ------------------------------------------------------------------------------------------------
// -----                                       FLASHCTRL                                      -----
// ------------------------------------------------------------------------------------------------


/**
  * @brief Product name title=UM10441 Chapter title=LPC122x System control (SYSCON) Modification date=2/16/2011 Major revision=1 Minor revision=0  (FLASHCTRL)
  */

typedef struct {                            /*!< (@ 0x50060000) FLASHCTRL Structure    */
  __I  uint32_t RESERVED0[10];
  __IO uint32_t FLASHCFG;                   /*!< (@ 0x50060028) Flash read cycle configuration */
} LPC_FLASHCTRL_Type;


// ------------------------------------------------------------------------------------------------
// -----                                          CRC                                         -----
// ------------------------------------------------------------------------------------------------


/**
  * @brief Product name title=UM10441 Chapter title=LPC122x CRC engine Modification date=2/11/2011 Major revision=0 Minor revision=17  (CRC)
  */

typedef struct {                            /*!< (@ 0x50070000) CRC Structure          */
  __IO uint32_t MODE;                       /*!< (@ 0x50070000) CRC mode register      */
  __IO uint32_t SEED;                       /*!< (@ 0x50070004) CRC seed register      */
  
  union {
    union{
    	__O uint8_t WR_DATA_8;                  /*!< (@ 0x50070008) CRC 8-bit data register      */
    	__O uint16_t WR_DATA_16;                /*!< (@ 0x50070008) CRC 16-bit data register      */
    	__O uint32_t WR_DATA_32;                /*!< (@ 0x50070008) CRC 32-bit data register      */
    };
    __I  uint32_t SUM;                      /*!< (@ 0x50070008) CRC checksum register  */
  };
} LPC_CRC_Type;


#if defined ( __CC_ARM   )
  #pragma no_anon_unions
#endif


// ------------------------------------------------------------------------------------------------
// -----                                 Peripheral memory map                                -----
// ------------------------------------------------------------------------------------------------

#define LPC_I2C_BASE              (0x40000000)
#define LPC_WWDT_BASE             (0x40004000)
#define LPC_UART0_BASE            (0x40008000)
#define LPC_UART1_BASE            (0x4000C000)
#define LPC_CT16B0_BASE           (0x40010000)
#define LPC_CT16B1_BASE           (0x40014000)
#define LPC_CT32B0_BASE           (0x40018000)
#define LPC_CT32B1_BASE           (0x4001C000)
#define LPC_ADC_BASE              (0x40020000)
#define LPC_PMU_BASE              (0x40038000)
#define LPC_SSP_BASE              (0x40040000)
#define LPC_IOCON_BASE            (0x40044000)
#define LPC_SYSCON_BASE           (0x40048000)
#define LPC_MICRO_DMA_BASE        (0x4004C000)
#define LPC_RTC_BASE              (0x40050000)
#define LPC_ACOMP_BASE            (0x40054000)
#define LPC_GPIO0_BASE            (0x50000000)
#define LPC_GPIO1_BASE            (0x50010000)
#define LPC_GPIO2_BASE            (0x50020000)
#define LPC_FLASHCTRL_BASE        (0x50060000)
#define LPC_CRC_BASE              (0x50070000)


// ------------------------------------------------------------------------------------------------
// -----                                Peripheral declaration                                -----
// ------------------------------------------------------------------------------------------------

#define LPC_I2C                   ((LPC_I2C_Type            *) LPC_I2C_BASE)
#define LPC_WWDT                  ((LPC_WWDT_Type           *) LPC_WWDT_BASE)
#define LPC_UART0                 ((LPC_UART0_Type          *) LPC_UART0_BASE)
#define LPC_UART1                 ((LPC_UART1_Type          *) LPC_UART1_BASE)
#define LPC_CT16B0                ((LPC_CTxxBx_Type         *) LPC_CT16B0_BASE)
#define LPC_CT16B1                ((LPC_CTxxBx_Type         *) LPC_CT16B1_BASE)
#define LPC_CT32B0                ((LPC_CTxxBx_Type         *) LPC_CT32B0_BASE)
#define LPC_CT32B1                ((LPC_CTxxBx_Type         *) LPC_CT32B1_BASE)
#define LPC_ADC                   ((LPC_ADC_Type            *) LPC_ADC_BASE)
#define LPC_PMU                   ((LPC_PMU_Type            *) LPC_PMU_BASE)
#define LPC_SSP                   ((LPC_SSP_Type            *) LPC_SSP_BASE)
#define LPC_IOCON                 ((LPC_IOCON_Type          *) LPC_IOCON_BASE)
#define LPC_SYSCON                ((LPC_SYSCON_Type         *) LPC_SYSCON_BASE)
#define LPC_MICRO_DMA             ((LPC_MICRO_DMA_Type      *) LPC_MICRO_DMA_BASE)
#define LPC_RTC                   ((LPC_RTC_Type            *) LPC_RTC_BASE)
#define LPC_ACOMP                 ((LPC_ACOMP_Type          *) LPC_ACOMP_BASE)
#define LPC_GPIO0                 ((LPC_GPIO_Type          	*) LPC_GPIO0_BASE)
#define LPC_GPIO1                 ((LPC_GPIO_Type          	*) LPC_GPIO1_BASE)
#define LPC_GPIO2                 ((LPC_GPIO_Type          	*) LPC_GPIO2_BASE)
#define LPC_FLASHCTRL             ((LPC_FLASHCTRL_Type      *) LPC_FLASHCTRL_BASE)
#define LPC_CRC                   ((LPC_CRC_Type            *) LPC_CRC_BASE)


/** @} */ /* End of group Device_Peripheral_Registers */
/** @} */ /* End of group (null) */
/** @} */ /* End of group LPC122x */

#ifdef __cplusplus
}
#endif 


#endif  // __LPC122X_H__
