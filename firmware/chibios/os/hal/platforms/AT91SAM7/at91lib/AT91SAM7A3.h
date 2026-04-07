//  ----------------------------------------------------------------------------
//          ATMEL Microcontroller Software Support  -  ROUSSET  -
//  ----------------------------------------------------------------------------
//  Copyright (c) 2006, Atmel Corporation
//
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  - Redistributions of source code must retain the above copyright notice,
//  this list of conditions and the disclaimer below.
//
//  Atmel's name may not be used to endorse or promote products derived from
//  this software without specific prior written permission.
//
//  DISCLAIMER:  THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
//  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
//  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
//  DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
//  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
//  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
//  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  ----------------------------------------------------------------------------
// File Name           : AT91SAM7A3.h
// Object              : AT91SAM7A3 definitions
// Generated           : AT91 SW Application Group  07/07/2008 (16:10:41)
//
// CVS Reference       : /AT91SAM7A3.pl/1.30/Wed Aug 30 14:08:29 2006//
// CVS Reference       : /SYS_SAM7A3.pl/1.7/Thu Feb  3 17:24:14 2005//
// CVS Reference       : /MC_SAM7A3.pl/1.3/Fri Sep 23 12:47:15 2005//
// CVS Reference       : /PMC_SAM7A3.pl/1.2/Tue Feb  8 14:00:18 2005//
// CVS Reference       : /RSTC_SAM7A3.pl/1.2/Wed Jul 13 15:25:16 2005//
// CVS Reference       : /SHDWC_SAM7A3.pl/1.1/Thu Feb  3 17:23:24 2005//
// CVS Reference       : /UDP_6ept.pl/1.1/Wed Aug 30 10:56:49 2006//
// CVS Reference       : /PWM_SAM7A3.pl/1.1/Tue May 10 12:38:54 2005//
// CVS Reference       : /AIC_6075B.pl/1.3/Fri May 20 14:21:42 2005//
// CVS Reference       : /PIO_6057A.pl/1.2/Thu Feb  3 10:29:42 2005//
// CVS Reference       : /RTTC_6081A.pl/1.2/Thu Nov  4 13:57:22 2004//
// CVS Reference       : /PITC_6079A.pl/1.2/Thu Nov  4 13:56:22 2004//
// CVS Reference       : /WDTC_6080A.pl/1.3/Thu Nov  4 13:58:52 2004//
// CVS Reference       : /PDC_6074C.pl/1.2/Thu Feb  3 09:02:11 2005//
// CVS Reference       : /DBGU_6059D.pl/1.1/Mon Jan 31 13:54:41 2005//
// CVS Reference       : /SPI_6088D.pl/1.3/Fri May 20 14:23:02 2005//
// CVS Reference       : /US_6089C.pl/1.1/Mon Jan 31 13:56:02 2005//
// CVS Reference       : /SSC_6078A.pl/1.1/Tue Jul 13 07:10:41 2004//
// CVS Reference       : /TWI_6061A.pl/1.2/Fri Oct 27 11:40:48 2006//
// CVS Reference       : /TC_6082A.pl/1.7/Wed Mar  9 16:31:51 2005//
// CVS Reference       : /CAN_6019B.pl/1.1/Mon Jan 31 13:54:30 2005//
// CVS Reference       : /MCI_6101A.pl/1.1/Tue Jul 13 06:33:59 2004//
// CVS Reference       : /ADC_6051C.pl/1.1/Mon Jan 31 13:12:40 2005//
// CVS Reference       : /AES_6149A.pl/1.12/Wed Nov  2 14:17:53 2005//
// CVS Reference       : /DES3_6150A.pl/1.1/Mon Jan 17 13:30:33 2005//
//  ----------------------------------------------------------------------------

#ifndef AT91SAM7A3_H
#define AT91SAM7A3_H

#ifndef __ASSEMBLY__
typedef volatile unsigned int AT91_REG;// Hardware register definition
#define AT91_CAST(a) (a)
#else
#define AT91_CAST(a)
#endif

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR System Peripherals
// *****************************************************************************
#ifndef __ASSEMBLY__
typedef struct _AT91S_SYS {
	AT91_REG	 AIC_SMR[32]; 	// Source Mode Register
	AT91_REG	 AIC_SVR[32]; 	// Source Vector Register
	AT91_REG	 AIC_IVR; 	// IRQ Vector Register
	AT91_REG	 AIC_FVR; 	// FIQ Vector Register
	AT91_REG	 AIC_ISR; 	// Interrupt Status Register
	AT91_REG	 AIC_IPR; 	// Interrupt Pending Register
	AT91_REG	 AIC_IMR; 	// Interrupt Mask Register
	AT91_REG	 AIC_CISR; 	// Core Interrupt Status Register
	AT91_REG	 Reserved0[2]; 	//
	AT91_REG	 AIC_IECR; 	// Interrupt Enable Command Register
	AT91_REG	 AIC_IDCR; 	// Interrupt Disable Command Register
	AT91_REG	 AIC_ICCR; 	// Interrupt Clear Command Register
	AT91_REG	 AIC_ISCR; 	// Interrupt Set Command Register
	AT91_REG	 AIC_EOICR; 	// End of Interrupt Command Register
	AT91_REG	 AIC_SPU; 	// Spurious Vector Register
	AT91_REG	 AIC_DCR; 	// Debug Control Register (Protect)
	AT91_REG	 Reserved1[1]; 	//
	AT91_REG	 AIC_FFER; 	// Fast Forcing Enable Register
	AT91_REG	 AIC_FFDR; 	// Fast Forcing Disable Register
	AT91_REG	 AIC_FFSR; 	// Fast Forcing Status Register
	AT91_REG	 Reserved2[45]; 	//
	AT91_REG	 DBGU_CR; 	// Control Register
	AT91_REG	 DBGU_MR; 	// Mode Register
	AT91_REG	 DBGU_IER; 	// Interrupt Enable Register
	AT91_REG	 DBGU_IDR; 	// Interrupt Disable Register
	AT91_REG	 DBGU_IMR; 	// Interrupt Mask Register
	AT91_REG	 DBGU_CSR; 	// Channel Status Register
	AT91_REG	 DBGU_RHR; 	// Receiver Holding Register
	AT91_REG	 DBGU_THR; 	// Transmitter Holding Register
	AT91_REG	 DBGU_BRGR; 	// Baud Rate Generator Register
	AT91_REG	 Reserved3[7]; 	//
	AT91_REG	 DBGU_CIDR; 	// Chip ID Register
	AT91_REG	 DBGU_EXID; 	// Chip ID Extension Register
	AT91_REG	 DBGU_FNTR; 	// Force NTRST Register
	AT91_REG	 Reserved4[45]; 	//
	AT91_REG	 DBGU_RPR; 	// Receive Pointer Register
	AT91_REG	 DBGU_RCR; 	// Receive Counter Register
	AT91_REG	 DBGU_TPR; 	// Transmit Pointer Register
	AT91_REG	 DBGU_TCR; 	// Transmit Counter Register
	AT91_REG	 DBGU_RNPR; 	// Receive Next Pointer Register
	AT91_REG	 DBGU_RNCR; 	// Receive Next Counter Register
	AT91_REG	 DBGU_TNPR; 	// Transmit Next Pointer Register
	AT91_REG	 DBGU_TNCR; 	// Transmit Next Counter Register
	AT91_REG	 DBGU_PTCR; 	// PDC Transfer Control Register
	AT91_REG	 DBGU_PTSR; 	// PDC Transfer Status Register
	AT91_REG	 Reserved5[54]; 	//
	AT91_REG	 PIOA_PER; 	// PIO Enable Register
	AT91_REG	 PIOA_PDR; 	// PIO Disable Register
	AT91_REG	 PIOA_PSR; 	// PIO Status Register
	AT91_REG	 Reserved6[1]; 	//
	AT91_REG	 PIOA_OER; 	// Output Enable Register
	AT91_REG	 PIOA_ODR; 	// Output Disable Registerr
	AT91_REG	 PIOA_OSR; 	// Output Status Register
	AT91_REG	 Reserved7[1]; 	//
	AT91_REG	 PIOA_IFER; 	// Input Filter Enable Register
	AT91_REG	 PIOA_IFDR; 	// Input Filter Disable Register
	AT91_REG	 PIOA_IFSR; 	// Input Filter Status Register
	AT91_REG	 Reserved8[1]; 	//
	AT91_REG	 PIOA_SODR; 	// Set Output Data Register
	AT91_REG	 PIOA_CODR; 	// Clear Output Data Register
	AT91_REG	 PIOA_ODSR; 	// Output Data Status Register
	AT91_REG	 PIOA_PDSR; 	// Pin Data Status Register
	AT91_REG	 PIOA_IER; 	// Interrupt Enable Register
	AT91_REG	 PIOA_IDR; 	// Interrupt Disable Register
	AT91_REG	 PIOA_IMR; 	// Interrupt Mask Register
	AT91_REG	 PIOA_ISR; 	// Interrupt Status Register
	AT91_REG	 PIOA_MDER; 	// Multi-driver Enable Register
	AT91_REG	 PIOA_MDDR; 	// Multi-driver Disable Register
	AT91_REG	 PIOA_MDSR; 	// Multi-driver Status Register
	AT91_REG	 Reserved9[1]; 	//
	AT91_REG	 PIOA_PPUDR; 	// Pull-up Disable Register
	AT91_REG	 PIOA_PPUER; 	// Pull-up Enable Register
	AT91_REG	 PIOA_PPUSR; 	// Pull-up Status Register
	AT91_REG	 Reserved10[1]; 	//
	AT91_REG	 PIOA_ASR; 	// Select A Register
	AT91_REG	 PIOA_BSR; 	// Select B Register
	AT91_REG	 PIOA_ABSR; 	// AB Select Status Register
	AT91_REG	 Reserved11[9]; 	//
	AT91_REG	 PIOA_OWER; 	// Output Write Enable Register
	AT91_REG	 PIOA_OWDR; 	// Output Write Disable Register
	AT91_REG	 PIOA_OWSR; 	// Output Write Status Register
	AT91_REG	 Reserved12[85]; 	//
	AT91_REG	 PIOB_PER; 	// PIO Enable Register
	AT91_REG	 PIOB_PDR; 	// PIO Disable Register
	AT91_REG	 PIOB_PSR; 	// PIO Status Register
	AT91_REG	 Reserved13[1]; 	//
	AT91_REG	 PIOB_OER; 	// Output Enable Register
	AT91_REG	 PIOB_ODR; 	// Output Disable Registerr
	AT91_REG	 PIOB_OSR; 	// Output Status Register
	AT91_REG	 Reserved14[1]; 	//
	AT91_REG	 PIOB_IFER; 	// Input Filter Enable Register
	AT91_REG	 PIOB_IFDR; 	// Input Filter Disable Register
	AT91_REG	 PIOB_IFSR; 	// Input Filter Status Register
	AT91_REG	 Reserved15[1]; 	//
	AT91_REG	 PIOB_SODR; 	// Set Output Data Register
	AT91_REG	 PIOB_CODR; 	// Clear Output Data Register
	AT91_REG	 PIOB_ODSR; 	// Output Data Status Register
	AT91_REG	 PIOB_PDSR; 	// Pin Data Status Register
	AT91_REG	 PIOB_IER; 	// Interrupt Enable Register
	AT91_REG	 PIOB_IDR; 	// Interrupt Disable Register
	AT91_REG	 PIOB_IMR; 	// Interrupt Mask Register
	AT91_REG	 PIOB_ISR; 	// Interrupt Status Register
	AT91_REG	 PIOB_MDER; 	// Multi-driver Enable Register
	AT91_REG	 PIOB_MDDR; 	// Multi-driver Disable Register
	AT91_REG	 PIOB_MDSR; 	// Multi-driver Status Register
	AT91_REG	 Reserved16[1]; 	//
	AT91_REG	 PIOB_PPUDR; 	// Pull-up Disable Register
	AT91_REG	 PIOB_PPUER; 	// Pull-up Enable Register
	AT91_REG	 PIOB_PPUSR; 	// Pull-up Status Register
	AT91_REG	 Reserved17[1]; 	//
	AT91_REG	 PIOB_ASR; 	// Select A Register
	AT91_REG	 PIOB_BSR; 	// Select B Register
	AT91_REG	 PIOB_ABSR; 	// AB Select Status Register
	AT91_REG	 Reserved18[9]; 	//
	AT91_REG	 PIOB_OWER; 	// Output Write Enable Register
	AT91_REG	 PIOB_OWDR; 	// Output Write Disable Register
	AT91_REG	 PIOB_OWSR; 	// Output Write Status Register
	AT91_REG	 Reserved19[341]; 	//
	AT91_REG	 PMC_SCER; 	// System Clock Enable Register
	AT91_REG	 PMC_SCDR; 	// System Clock Disable Register
	AT91_REG	 PMC_SCSR; 	// System Clock Status Register
	AT91_REG	 Reserved20[1]; 	//
	AT91_REG	 PMC_PCER; 	// Peripheral Clock Enable Register
	AT91_REG	 PMC_PCDR; 	// Peripheral Clock Disable Register
	AT91_REG	 PMC_PCSR; 	// Peripheral Clock Status Register
	AT91_REG	 Reserved21[1]; 	//
	AT91_REG	 PMC_MOR; 	// Main Oscillator Register
	AT91_REG	 PMC_MCFR; 	// Main Clock  Frequency Register
	AT91_REG	 Reserved22[1]; 	//
	AT91_REG	 PMC_PLLR; 	// PLL Register
	AT91_REG	 PMC_MCKR; 	// Master Clock Register
	AT91_REG	 Reserved23[3]; 	//
	AT91_REG	 PMC_PCKR[4]; 	// Programmable Clock Register
	AT91_REG	 Reserved24[4]; 	//
	AT91_REG	 PMC_IER; 	// Interrupt Enable Register
	AT91_REG	 PMC_IDR; 	// Interrupt Disable Register
	AT91_REG	 PMC_SR; 	// Status Register
	AT91_REG	 PMC_IMR; 	// Interrupt Mask Register
	AT91_REG	 Reserved25[36]; 	//
	AT91_REG	 RSTC_RCR; 	// Reset Control Register
	AT91_REG	 RSTC_RSR; 	// Reset Status Register
	AT91_REG	 RSTC_RMR; 	// Reset Mode Register
	AT91_REG	 Reserved26[1]; 	//
	AT91_REG	 SHDWC_SHCR; 	// Shut Down Control Register
	AT91_REG	 SHDWC_SHMR; 	// Shut Down Mode Register
	AT91_REG	 SHDWC_SHSR; 	// Shut Down Status Register
	AT91_REG	 Reserved27[1]; 	//
	AT91_REG	 RTTC_RTMR; 	// Real-time Mode Register
	AT91_REG	 RTTC_RTAR; 	// Real-time Alarm Register
	AT91_REG	 RTTC_RTVR; 	// Real-time Value Register
	AT91_REG	 RTTC_RTSR; 	// Real-time Status Register
	AT91_REG	 PITC_PIMR; 	// Period Interval Mode Register
	AT91_REG	 PITC_PISR; 	// Period Interval Status Register
	AT91_REG	 PITC_PIVR; 	// Period Interval Value Register
	AT91_REG	 PITC_PIIR; 	// Period Interval Image Register
	AT91_REG	 WDTC_WDCR; 	// Watchdog Control Register
	AT91_REG	 WDTC_WDMR; 	// Watchdog Mode Register
	AT91_REG	 WDTC_WDSR; 	// Watchdog Status Register
	AT91_REG	 Reserved28[1]; 	//
	AT91_REG	 SYS_GPBR0; 	// General Purpose Register 0
	AT91_REG	 SYS_GPBR1; 	// General Purpose Register 1
	AT91_REG	 Reserved29[106]; 	//
	AT91_REG	 MC_RCR; 	// MC Remap Control Register
	AT91_REG	 MC_ASR; 	// MC Abort Status Register
	AT91_REG	 MC_AASR; 	// MC Abort Address Status Register
	AT91_REG	 Reserved30[1]; 	//
	AT91_REG	 MC_PUIA[16]; 	// MC Protection Unit Area
	AT91_REG	 MC_PUP; 	// MC Protection Unit Peripherals
	AT91_REG	 MC_PUER; 	// MC Protection Unit Enable Register
	AT91_REG	 Reserved31[2]; 	//
	AT91_REG	 MC_FMR; 	// MC Flash Mode Register
	AT91_REG	 MC_FCR; 	// MC Flash Command Register
	AT91_REG	 MC_FSR; 	// MC Flash Status Register
} AT91S_SYS, *AT91PS_SYS;
#else
#define SYS_GPBR0       (AT91_CAST(AT91_REG *) 	0x00000D50) // (SYS_GPBR0) General Purpose Register 0
#define SYS_GPBR1       (AT91_CAST(AT91_REG *) 	0x00000D54) // (SYS_GPBR1) General Purpose Register 1

#endif
// -------- GPBR : (SYS Offset: 0xd50) GPBR General Purpose Register --------
// -------- GPBR : (SYS Offset: 0xd54) GPBR General Purpose Register --------

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Advanced Interrupt Controller
// *****************************************************************************
#ifndef __ASSEMBLY__
typedef struct _AT91S_AIC {
	AT91_REG	 AIC_SMR[32]; 	// Source Mode Register
	AT91_REG	 AIC_SVR[32]; 	// Source Vector Register
	AT91_REG	 AIC_IVR; 	// IRQ Vector Register
	AT91_REG	 AIC_FVR; 	// FIQ Vector Register
	AT91_REG	 AIC_ISR; 	// Interrupt Status Register
	AT91_REG	 AIC_IPR; 	// Interrupt Pending Register
	AT91_REG	 AIC_IMR; 	// Interrupt Mask Register
	AT91_REG	 AIC_CISR; 	// Core Interrupt Status Register
	AT91_REG	 Reserved0[2]; 	//
	AT91_REG	 AIC_IECR; 	// Interrupt Enable Command Register
	AT91_REG	 AIC_IDCR; 	// Interrupt Disable Command Register
	AT91_REG	 AIC_ICCR; 	// Interrupt Clear Command Register
	AT91_REG	 AIC_ISCR; 	// Interrupt Set Command Register
	AT91_REG	 AIC_EOICR; 	// End of Interrupt Command Register
	AT91_REG	 AIC_SPU; 	// Spurious Vector Register
	AT91_REG	 AIC_DCR; 	// Debug Control Register (Protect)
	AT91_REG	 Reserved1[1]; 	//
	AT91_REG	 AIC_FFER; 	// Fast Forcing Enable Register
	AT91_REG	 AIC_FFDR; 	// Fast Forcing Disable Register
	AT91_REG	 AIC_FFSR; 	// Fast Forcing Status Register
} AT91S_AIC, *AT91PS_AIC;
#else
#define AIC_SMR         (AT91_CAST(AT91_REG *) 	0x00000000) // (AIC_SMR) Source Mode Register
#define AIC_SVR         (AT91_CAST(AT91_REG *) 	0x00000080) // (AIC_SVR) Source Vector Register
#define AIC_IVR         (AT91_CAST(AT91_REG *) 	0x00000100) // (AIC_IVR) IRQ Vector Register
#define AIC_FVR         (AT91_CAST(AT91_REG *) 	0x00000104) // (AIC_FVR) FIQ Vector Register
#define AIC_ISR         (AT91_CAST(AT91_REG *) 	0x00000108) // (AIC_ISR) Interrupt Status Register
#define AIC_IPR         (AT91_CAST(AT91_REG *) 	0x0000010C) // (AIC_IPR) Interrupt Pending Register
#define AIC_IMR         (AT91_CAST(AT91_REG *) 	0x00000110) // (AIC_IMR) Interrupt Mask Register
#define AIC_CISR        (AT91_CAST(AT91_REG *) 	0x00000114) // (AIC_CISR) Core Interrupt Status Register
#define AIC_IECR        (AT91_CAST(AT91_REG *) 	0x00000120) // (AIC_IECR) Interrupt Enable Command Register
#define AIC_IDCR        (AT91_CAST(AT91_REG *) 	0x00000124) // (AIC_IDCR) Interrupt Disable Command Register
#define AIC_ICCR        (AT91_CAST(AT91_REG *) 	0x00000128) // (AIC_ICCR) Interrupt Clear Command Register
#define AIC_ISCR        (AT91_CAST(AT91_REG *) 	0x0000012C) // (AIC_ISCR) Interrupt Set Command Register
#define AIC_EOICR       (AT91_CAST(AT91_REG *) 	0x00000130) // (AIC_EOICR) End of Interrupt Command Register
#define AIC_SPU         (AT91_CAST(AT91_REG *) 	0x00000134) // (AIC_SPU) Spurious Vector Register
#define AIC_DCR         (AT91_CAST(AT91_REG *) 	0x00000138) // (AIC_DCR) Debug Control Register (Protect)
#define AIC_FFER        (AT91_CAST(AT91_REG *) 	0x00000140) // (AIC_FFER) Fast Forcing Enable Register
#define AIC_FFDR        (AT91_CAST(AT91_REG *) 	0x00000144) // (AIC_FFDR) Fast Forcing Disable Register
#define AIC_FFSR        (AT91_CAST(AT91_REG *) 	0x00000148) // (AIC_FFSR) Fast Forcing Status Register

#endif
// -------- AIC_SMR : (AIC Offset: 0x0) Control Register --------
#define AT91C_AIC_PRIOR       (0x7 <<  0) // (AIC) Priority Level
#define 	AT91C_AIC_PRIOR_LOWEST               (0x0) // (AIC) Lowest priority level
#define 	AT91C_AIC_PRIOR_HIGHEST              (0x7) // (AIC) Highest priority level
#define AT91C_AIC_SRCTYPE     (0x3 <<  5) // (AIC) Interrupt Source Type
#define 	AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL       (0x0 <<  5) // (AIC) Internal Sources Code Label High-level Sensitive
#define 	AT91C_AIC_SRCTYPE_EXT_LOW_LEVEL        (0x0 <<  5) // (AIC) External Sources Code Label Low-level Sensitive
#define 	AT91C_AIC_SRCTYPE_INT_POSITIVE_EDGE    (0x1 <<  5) // (AIC) Internal Sources Code Label Positive Edge triggered
#define 	AT91C_AIC_SRCTYPE_EXT_NEGATIVE_EDGE    (0x1 <<  5) // (AIC) External Sources Code Label Negative Edge triggered
#define 	AT91C_AIC_SRCTYPE_HIGH_LEVEL           (0x2 <<  5) // (AIC) Internal Or External Sources Code Label High-level Sensitive
#define 	AT91C_AIC_SRCTYPE_POSITIVE_EDGE        (0x3 <<  5) // (AIC) Internal Or External Sources Code Label Positive Edge triggered
// -------- AIC_CISR : (AIC Offset: 0x114) AIC Core Interrupt Status Register --------
#define AT91C_AIC_NFIQ        (0x1 <<  0) // (AIC) NFIQ Status
#define AT91C_AIC_NIRQ        (0x1 <<  1) // (AIC) NIRQ Status
// -------- AIC_DCR : (AIC Offset: 0x138) AIC Debug Control Register (Protect) --------
#define AT91C_AIC_DCR_PROT    (0x1 <<  0) // (AIC) Protection Mode
#define AT91C_AIC_DCR_GMSK    (0x1 <<  1) // (AIC) General Mask

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Peripheral DMA Controller
// *****************************************************************************
#ifndef __ASSEMBLY__
typedef struct _AT91S_PDC {
	AT91_REG	 PDC_RPR; 	// Receive Pointer Register
	AT91_REG	 PDC_RCR; 	// Receive Counter Register
	AT91_REG	 PDC_TPR; 	// Transmit Pointer Register
	AT91_REG	 PDC_TCR; 	// Transmit Counter Register
	AT91_REG	 PDC_RNPR; 	// Receive Next Pointer Register
	AT91_REG	 PDC_RNCR; 	// Receive Next Counter Register
	AT91_REG	 PDC_TNPR; 	// Transmit Next Pointer Register
	AT91_REG	 PDC_TNCR; 	// Transmit Next Counter Register
	AT91_REG	 PDC_PTCR; 	// PDC Transfer Control Register
	AT91_REG	 PDC_PTSR; 	// PDC Transfer Status Register
} AT91S_PDC, *AT91PS_PDC;
#else
#define PDC_RPR         (AT91_CAST(AT91_REG *) 	0x00000000) // (PDC_RPR) Receive Pointer Register
#define PDC_RCR         (AT91_CAST(AT91_REG *) 	0x00000004) // (PDC_RCR) Receive Counter Register
#define PDC_TPR         (AT91_CAST(AT91_REG *) 	0x00000008) // (PDC_TPR) Transmit Pointer Register
#define PDC_TCR         (AT91_CAST(AT91_REG *) 	0x0000000C) // (PDC_TCR) Transmit Counter Register
#define PDC_RNPR        (AT91_CAST(AT91_REG *) 	0x00000010) // (PDC_RNPR) Receive Next Pointer Register
#define PDC_RNCR        (AT91_CAST(AT91_REG *) 	0x00000014) // (PDC_RNCR) Receive Next Counter Register
#define PDC_TNPR        (AT91_CAST(AT91_REG *) 	0x00000018) // (PDC_TNPR) Transmit Next Pointer Register
#define PDC_TNCR        (AT91_CAST(AT91_REG *) 	0x0000001C) // (PDC_TNCR) Transmit Next Counter Register
#define PDC_PTCR        (AT91_CAST(AT91_REG *) 	0x00000020) // (PDC_PTCR) PDC Transfer Control Register
#define PDC_PTSR        (AT91_CAST(AT91_REG *) 	0x00000024) // (PDC_PTSR) PDC Transfer Status Register

#endif
// -------- PDC_PTCR : (PDC Offset: 0x20) PDC Transfer Control Register --------
#define AT91C_PDC_RXTEN       (0x1 <<  0) // (PDC) Receiver Transfer Enable
#define AT91C_PDC_RXTDIS      (0x1 <<  1) // (PDC) Receiver Transfer Disable
#define AT91C_PDC_TXTEN       (0x1 <<  8) // (PDC) Transmitter Transfer Enable
#define AT91C_PDC_TXTDIS      (0x1 <<  9) // (PDC) Transmitter Transfer Disable
// -------- PDC_PTSR : (PDC Offset: 0x24) PDC Transfer Status Register --------

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Debug Unit
// *****************************************************************************
#ifndef __ASSEMBLY__
typedef struct _AT91S_DBGU {
	AT91_REG	 DBGU_CR; 	// Control Register
	AT91_REG	 DBGU_MR; 	// Mode Register
	AT91_REG	 DBGU_IER; 	// Interrupt Enable Register
	AT91_REG	 DBGU_IDR; 	// Interrupt Disable Register
	AT91_REG	 DBGU_IMR; 	// Interrupt Mask Register
	AT91_REG	 DBGU_CSR; 	// Channel Status Register
	AT91_REG	 DBGU_RHR; 	// Receiver Holding Register
	AT91_REG	 DBGU_THR; 	// Transmitter Holding Register
	AT91_REG	 DBGU_BRGR; 	// Baud Rate Generator Register
	AT91_REG	 Reserved0[7]; 	//
	AT91_REG	 DBGU_CIDR; 	// Chip ID Register
	AT91_REG	 DBGU_EXID; 	// Chip ID Extension Register
	AT91_REG	 DBGU_FNTR; 	// Force NTRST Register
	AT91_REG	 Reserved1[45]; 	//
	AT91_REG	 DBGU_RPR; 	// Receive Pointer Register
	AT91_REG	 DBGU_RCR; 	// Receive Counter Register
	AT91_REG	 DBGU_TPR; 	// Transmit Pointer Register
	AT91_REG	 DBGU_TCR; 	// Transmit Counter Register
	AT91_REG	 DBGU_RNPR; 	// Receive Next Pointer Register
	AT91_REG	 DBGU_RNCR; 	// Receive Next Counter Register
	AT91_REG	 DBGU_TNPR; 	// Transmit Next Pointer Register
	AT91_REG	 DBGU_TNCR; 	// Transmit Next Counter Register
	AT91_REG	 DBGU_PTCR; 	// PDC Transfer Control Register
	AT91_REG	 DBGU_PTSR; 	// PDC Transfer Status Register
} AT91S_DBGU, *AT91PS_DBGU;
#else
#define DBGU_CR         (AT91_CAST(AT91_REG *) 	0x00000000) // (DBGU_CR) Control Register
#define DBGU_MR         (AT91_CAST(AT91_REG *) 	0x00000004) // (DBGU_MR) Mode Register
#define DBGU_IER        (AT91_CAST(AT91_REG *) 	0x00000008) // (DBGU_IER) Interrupt Enable Register
#define DBGU_IDR        (AT91_CAST(AT91_REG *) 	0x0000000C) // (DBGU_IDR) Interrupt Disable Register
#define DBGU_IMR        (AT91_CAST(AT91_REG *) 	0x00000010) // (DBGU_IMR) Interrupt Mask Register
#define DBGU_CSR        (AT91_CAST(AT91_REG *) 	0x00000014) // (DBGU_CSR) Channel Status Register
#define DBGU_RHR        (AT91_CAST(AT91_REG *) 	0x00000018) // (DBGU_RHR) Receiver Holding Register
#define DBGU_THR        (AT91_CAST(AT91_REG *) 	0x0000001C) // (DBGU_THR) Transmitter Holding Register
#define DBGU_BRGR       (AT91_CAST(AT91_REG *) 	0x00000020) // (DBGU_BRGR) Baud Rate Generator Register
#define DBGU_CIDR       (AT91_CAST(AT91_REG *) 	0x00000040) // (DBGU_CIDR) Chip ID Register
#define DBGU_EXID       (AT91_CAST(AT91_REG *) 	0x00000044) // (DBGU_EXID) Chip ID Extension Register
#define DBGU_FNTR       (AT91_CAST(AT91_REG *) 	0x00000048) // (DBGU_FNTR) Force NTRST Register

#endif
// -------- DBGU_CR : (DBGU Offset: 0x0) Debug Unit Control Register --------
#define AT91C_US_RSTRX        (0x1 <<  2) // (DBGU) Reset Receiver
#define AT91C_US_RSTTX        (0x1 <<  3) // (DBGU) Reset Transmitter
#define AT91C_US_RXEN         (0x1 <<  4) // (DBGU) Receiver Enable
#define AT91C_US_RXDIS        (0x1 <<  5) // (DBGU) Receiver Disable
#define AT91C_US_TXEN         (0x1 <<  6) // (DBGU) Transmitter Enable
#define AT91C_US_TXDIS        (0x1 <<  7) // (DBGU) Transmitter Disable
#define AT91C_US_RSTSTA       (0x1 <<  8) // (DBGU) Reset Status Bits
// -------- DBGU_MR : (DBGU Offset: 0x4) Debug Unit Mode Register --------
#define AT91C_US_PAR          (0x7 <<  9) // (DBGU) Parity type
#define 	AT91C_US_PAR_EVEN                 (0x0 <<  9) // (DBGU) Even Parity
#define 	AT91C_US_PAR_ODD                  (0x1 <<  9) // (DBGU) Odd Parity
#define 	AT91C_US_PAR_SPACE                (0x2 <<  9) // (DBGU) Parity forced to 0 (Space)
#define 	AT91C_US_PAR_MARK                 (0x3 <<  9) // (DBGU) Parity forced to 1 (Mark)
#define 	AT91C_US_PAR_NONE                 (0x4 <<  9) // (DBGU) No Parity
#define 	AT91C_US_PAR_MULTI_DROP           (0x6 <<  9) // (DBGU) Multi-drop mode
#define AT91C_US_CHMODE       (0x3 << 14) // (DBGU) Channel Mode
#define 	AT91C_US_CHMODE_NORMAL               (0x0 << 14) // (DBGU) Normal Mode: The USART channel operates as an RX/TX USART.
#define 	AT91C_US_CHMODE_AUTO                 (0x1 << 14) // (DBGU) Automatic Echo: Receiver Data Input is connected to the TXD pin.
#define 	AT91C_US_CHMODE_LOCAL                (0x2 << 14) // (DBGU) Local Loopback: Transmitter Output Signal is connected to Receiver Input Signal.
#define 	AT91C_US_CHMODE_REMOTE               (0x3 << 14) // (DBGU) Remote Loopback: RXD pin is internally connected to TXD pin.
// -------- DBGU_IER : (DBGU Offset: 0x8) Debug Unit Interrupt Enable Register --------
#define AT91C_US_RXRDY        (0x1 <<  0) // (DBGU) RXRDY Interrupt
#define AT91C_US_TXRDY        (0x1 <<  1) // (DBGU) TXRDY Interrupt
#define AT91C_US_ENDRX        (0x1 <<  3) // (DBGU) End of Receive Transfer Interrupt
#define AT91C_US_ENDTX        (0x1 <<  4) // (DBGU) End of Transmit Interrupt
#define AT91C_US_OVRE         (0x1 <<  5) // (DBGU) Overrun Interrupt
#define AT91C_US_FRAME        (0x1 <<  6) // (DBGU) Framing Error Interrupt
#define AT91C_US_PARE         (0x1 <<  7) // (DBGU) Parity Error Interrupt
#define AT91C_US_TXEMPTY      (0x1 <<  9) // (DBGU) TXEMPTY Interrupt
#define AT91C_US_TXBUFE       (0x1 << 11) // (DBGU) TXBUFE Interrupt
#define AT91C_US_RXBUFF       (0x1 << 12) // (DBGU) RXBUFF Interrupt
#define AT91C_US_COMM_TX      (0x1 << 30) // (DBGU) COMM_TX Interrupt
#define AT91C_US_COMM_RX      (0x1 << 31) // (DBGU) COMM_RX Interrupt
// -------- DBGU_IDR : (DBGU Offset: 0xc) Debug Unit Interrupt Disable Register --------
// -------- DBGU_IMR : (DBGU Offset: 0x10) Debug Unit Interrupt Mask Register --------
// -------- DBGU_CSR : (DBGU Offset: 0x14) Debug Unit Channel Status Register --------
// -------- DBGU_FNTR : (DBGU Offset: 0x48) Debug Unit FORCE_NTRST Register --------
#define AT91C_US_FORCE_NTRST  (0x1 <<  0) // (DBGU) Force NTRST in JTAG

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Parallel Input Output Controler
// *****************************************************************************
#ifndef __ASSEMBLY__
typedef struct _AT91S_PIO {
	AT91_REG	 PIO_PER; 	// PIO Enable Register
	AT91_REG	 PIO_PDR; 	// PIO Disable Register
	AT91_REG	 PIO_PSR; 	// PIO Status Register
	AT91_REG	 Reserved0[1]; 	//
	AT91_REG	 PIO_OER; 	// Output Enable Register
	AT91_REG	 PIO_ODR; 	// Output Disable Registerr
	AT91_REG	 PIO_OSR; 	// Output Status Register
	AT91_REG	 Reserved1[1]; 	//
	AT91_REG	 PIO_IFER; 	// Input Filter Enable Register
	AT91_REG	 PIO_IFDR; 	// Input Filter Disable Register
	AT91_REG	 PIO_IFSR; 	// Input Filter Status Register
	AT91_REG	 Reserved2[1]; 	//
	AT91_REG	 PIO_SODR; 	// Set Output Data Register
	AT91_REG	 PIO_CODR; 	// Clear Output Data Register
	AT91_REG	 PIO_ODSR; 	// Output Data Status Register
	AT91_REG	 PIO_PDSR; 	// Pin Data Status Register
	AT91_REG	 PIO_IER; 	// Interrupt Enable Register
	AT91_REG	 PIO_IDR; 	// Interrupt Disable Register
	AT91_REG	 PIO_IMR; 	// Interrupt Mask Register
	AT91_REG	 PIO_ISR; 	// Interrupt Status Register
	AT91_REG	 PIO_MDER; 	// Multi-driver Enable Register
	AT91_REG	 PIO_MDDR; 	// Multi-driver Disable Register
	AT91_REG	 PIO_MDSR; 	// Multi-driver Status Register
	AT91_REG	 Reserved3[1]; 	//
	AT91_REG	 PIO_PPUDR; 	// Pull-up Disable Register
	AT91_REG	 PIO_PPUER; 	// Pull-up Enable Register
	AT91_REG	 PIO_PPUSR; 	// Pull-up Status Register
	AT91_REG	 Reserved4[1]; 	//
	AT91_REG	 PIO_ASR; 	// Select A Register
	AT91_REG	 PIO_BSR; 	// Select B Register
	AT91_REG	 PIO_ABSR; 	// AB Select Status Register
	AT91_REG	 Reserved5[9]; 	//
	AT91_REG	 PIO_OWER; 	// Output Write Enable Register
	AT91_REG	 PIO_OWDR; 	// Output Write Disable Register
	AT91_REG	 PIO_OWSR; 	// Output Write Status Register
} AT91S_PIO, *AT91PS_PIO;
#else
#define PIO_PER         (AT91_CAST(AT91_REG *) 	0x00000000) // (PIO_PER) PIO Enable Register
#define PIO_PDR         (AT91_CAST(AT91_REG *) 	0x00000004) // (PIO_PDR) PIO Disable Register
#define PIO_PSR         (AT91_CAST(AT91_REG *) 	0x00000008) // (PIO_PSR) PIO Status Register
#define PIO_OER         (AT91_CAST(AT91_REG *) 	0x00000010) // (PIO_OER) Output Enable Register
#define PIO_ODR         (AT91_CAST(AT91_REG *) 	0x00000014) // (PIO_ODR) Output Disable Registerr
#define PIO_OSR         (AT91_CAST(AT91_REG *) 	0x00000018) // (PIO_OSR) Output Status Register
#define PIO_IFER        (AT91_CAST(AT91_REG *) 	0x00000020) // (PIO_IFER) Input Filter Enable Register
#define PIO_IFDR        (AT91_CAST(AT91_REG *) 	0x00000024) // (PIO_IFDR) Input Filter Disable Register
#define PIO_IFSR        (AT91_CAST(AT91_REG *) 	0x00000028) // (PIO_IFSR) Input Filter Status Register
#define PIO_SODR        (AT91_CAST(AT91_REG *) 	0x00000030) // (PIO_SODR) Set Output Data Register
#define PIO_CODR        (AT91_CAST(AT91_REG *) 	0x00000034) // (PIO_CODR) Clear Output Data Register
#define PIO_ODSR        (AT91_CAST(AT91_REG *) 	0x00000038) // (PIO_ODSR) Output Data Status Register
#define PIO_PDSR        (AT91_CAST(AT91_REG *) 	0x0000003C) // (PIO_PDSR) Pin Data Status Register
#define PIO_IER         (AT91_CAST(AT91_REG *) 	0x00000040) // (PIO_IER) Interrupt Enable Register
#define PIO_IDR         (AT91_CAST(AT91_REG *) 	0x00000044) // (PIO_IDR) Interrupt Disable Register
#define PIO_IMR         (AT91_CAST(AT91_REG *) 	0x00000048) // (PIO_IMR) Interrupt Mask Register
#define PIO_ISR         (AT91_CAST(AT91_REG *) 	0x0000004C) // (PIO_ISR) Interrupt Status Register
#define PIO_MDER        (AT91_CAST(AT91_REG *) 	0x00000050) // (PIO_MDER) Multi-driver Enable Register
#define PIO_MDDR        (AT91_CAST(AT91_REG *) 	0x00000054) // (PIO_MDDR) Multi-driver Disable Register
#define PIO_MDSR        (AT91_CAST(AT91_REG *) 	0x00000058) // (PIO_MDSR) Multi-driver Status Register
#define PIO_PPUDR       (AT91_CAST(AT91_REG *) 	0x00000060) // (PIO_PPUDR) Pull-up Disable Register
#define PIO_PPUER       (AT91_CAST(AT91_REG *) 	0x00000064) // (PIO_PPUER) Pull-up Enable Register
#define PIO_PPUSR       (AT91_CAST(AT91_REG *) 	0x00000068) // (PIO_PPUSR) Pull-up Status Register
#define PIO_ASR         (AT91_CAST(AT91_REG *) 	0x00000070) // (PIO_ASR) Select A Register
#define PIO_BSR         (AT91_CAST(AT91_REG *) 	0x00000074) // (PIO_BSR) Select B Register
#define PIO_ABSR        (AT91_CAST(AT91_REG *) 	0x00000078) // (PIO_ABSR) AB Select Status Register
#define PIO_OWER        (AT91_CAST(AT91_REG *) 	0x000000A0) // (PIO_OWER) Output Write Enable Register
#define PIO_OWDR        (AT91_CAST(AT91_REG *) 	0x000000A4) // (PIO_OWDR) Output Write Disable Register
#define PIO_OWSR        (AT91_CAST(AT91_REG *) 	0x000000A8) // (PIO_OWSR) Output Write Status Register

#endif

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Clock Generator Controler
// *****************************************************************************
#ifndef __ASSEMBLY__
typedef struct _AT91S_CKGR {
	AT91_REG	 CKGR_MOR; 	// Main Oscillator Register
	AT91_REG	 CKGR_MCFR; 	// Main Clock  Frequency Register
	AT91_REG	 Reserved0[1]; 	//
	AT91_REG	 CKGR_PLLR; 	// PLL Register
} AT91S_CKGR, *AT91PS_CKGR;
#else
#define CKGR_MOR        (AT91_CAST(AT91_REG *) 	0x00000000) // (CKGR_MOR) Main Oscillator Register
#define CKGR_MCFR       (AT91_CAST(AT91_REG *) 	0x00000004) // (CKGR_MCFR) Main Clock  Frequency Register
#define CKGR_PLLR       (AT91_CAST(AT91_REG *) 	0x0000000C) // (CKGR_PLLR) PLL Register

#endif
// -------- CKGR_MOR : (CKGR Offset: 0x0) Main Oscillator Register --------
#define AT91C_CKGR_MOSCEN     (0x1 <<  0) // (CKGR) Main Oscillator Enable
#define AT91C_CKGR_OSCBYPASS  (0x1 <<  1) // (CKGR) Main Oscillator Bypass
#define AT91C_CKGR_OSCOUNT    (0xFF <<  8) // (CKGR) Main Oscillator Start-up Time
// -------- CKGR_MCFR : (CKGR Offset: 0x4) Main Clock Frequency Register --------
#define AT91C_CKGR_MAINF      (0xFFFF <<  0) // (CKGR) Main Clock Frequency
#define AT91C_CKGR_MAINRDY    (0x1 << 16) // (CKGR) Main Clock Ready
// -------- CKGR_PLLR : (CKGR Offset: 0xc) PLL B Register --------
#define AT91C_CKGR_DIV        (0xFF <<  0) // (CKGR) Divider Selected
#define 	AT91C_CKGR_DIV_0                    (0x0) // (CKGR) Divider output is 0
#define 	AT91C_CKGR_DIV_BYPASS               (0x1) // (CKGR) Divider is bypassed
#define AT91C_CKGR_PLLCOUNT   (0x3F <<  8) // (CKGR) PLL Counter
#define AT91C_CKGR_OUT        (0x3 << 14) // (CKGR) PLL Output Frequency Range
#define 	AT91C_CKGR_OUT_0                    (0x0 << 14) // (CKGR) Please refer to the PLL datasheet
#define 	AT91C_CKGR_OUT_1                    (0x1 << 14) // (CKGR) Please refer to the PLL datasheet
#define 	AT91C_CKGR_OUT_2                    (0x2 << 14) // (CKGR) Please refer to the PLL datasheet
#define 	AT91C_CKGR_OUT_3                    (0x3 << 14) // (CKGR) Please refer to the PLL datasheet
#define AT91C_CKGR_MUL        (0x7FF << 16) // (CKGR) PLL Multiplier
#define AT91C_CKGR_USBDIV     (0x3 << 28) // (CKGR) Divider for USB Clocks
#define 	AT91C_CKGR_USBDIV_0                    (0x0 << 28) // (CKGR) Divider output is PLL clock output
#define 	AT91C_CKGR_USBDIV_1                    (0x1 << 28) // (CKGR) Divider output is PLL clock output divided by 2
#define 	AT91C_CKGR_USBDIV_2                    (0x2 << 28) // (CKGR) Divider output is PLL clock output divided by 4

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Power Management Controler
// *****************************************************************************
#ifndef __ASSEMBLY__
typedef struct _AT91S_PMC {
	AT91_REG	 PMC_SCER; 	// System Clock Enable Register
	AT91_REG	 PMC_SCDR; 	// System Clock Disable Register
	AT91_REG	 PMC_SCSR; 	// System Clock Status Register
	AT91_REG	 Reserved0[1]; 	//
	AT91_REG	 PMC_PCER; 	// Peripheral Clock Enable Register
	AT91_REG	 PMC_PCDR; 	// Peripheral Clock Disable Register
	AT91_REG	 PMC_PCSR; 	// Peripheral Clock Status Register
	AT91_REG	 Reserved1[1]; 	//
	AT91_REG	 PMC_MOR; 	// Main Oscillator Register
	AT91_REG	 PMC_MCFR; 	// Main Clock  Frequency Register
	AT91_REG	 Reserved2[1]; 	//
	AT91_REG	 PMC_PLLR; 	// PLL Register
	AT91_REG	 PMC_MCKR; 	// Master Clock Register
	AT91_REG	 Reserved3[3]; 	//
	AT91_REG	 PMC_PCKR[4]; 	// Programmable Clock Register
	AT91_REG	 Reserved4[4]; 	//
	AT91_REG	 PMC_IER; 	// Interrupt Enable Register
	AT91_REG	 PMC_IDR; 	// Interrupt Disable Register
	AT91_REG	 PMC_SR; 	// Status Register
	AT91_REG	 PMC_IMR; 	// Interrupt Mask Register
} AT91S_PMC, *AT91PS_PMC;
#else
#define PMC_SCER        (AT91_CAST(AT91_REG *) 	0x00000000) // (PMC_SCER) System Clock Enable Register
#define PMC_SCDR        (AT91_CAST(AT91_REG *) 	0x00000004) // (PMC_SCDR) System Clock Disable Register
#define PMC_SCSR        (AT91_CAST(AT91_REG *) 	0x00000008) // (PMC_SCSR) System Clock Status Register
#define PMC_PCER        (AT91_CAST(AT91_REG *) 	0x00000010) // (PMC_PCER) Peripheral Clock Enable Register
#define PMC_PCDR        (AT91_CAST(AT91_REG *) 	0x00000014) // (PMC_PCDR) Peripheral Clock Disable Register
#define PMC_PCSR        (AT91_CAST(AT91_REG *) 	0x00000018) // (PMC_PCSR) Peripheral Clock Status Register
#define PMC_MCKR        (AT91_CAST(AT91_REG *) 	0x00000030) // (PMC_MCKR) Master Clock Register
#define PMC_PCKR        (AT91_CAST(AT91_REG *) 	0x00000040) // (PMC_PCKR) Programmable Clock Register
#define PMC_IER         (AT91_CAST(AT91_REG *) 	0x00000060) // (PMC_IER) Interrupt Enable Register
#define PMC_IDR         (AT91_CAST(AT91_REG *) 	0x00000064) // (PMC_IDR) Interrupt Disable Register
#define PMC_SR          (AT91_CAST(AT91_REG *) 	0x00000068) // (PMC_SR) Status Register
#define PMC_IMR         (AT91_CAST(AT91_REG *) 	0x0000006C) // (PMC_IMR) Interrupt Mask Register

#endif
// -------- PMC_SCER : (PMC Offset: 0x0) System Clock Enable Register --------
#define AT91C_PMC_PCK         (0x1 <<  0) // (PMC) Processor Clock
#define AT91C_PMC_UDP         (0x1 <<  7) // (PMC) USB Device Port Clock
#define AT91C_PMC_PCK0        (0x1 <<  8) // (PMC) Programmable Clock Output
#define AT91C_PMC_PCK1        (0x1 <<  9) // (PMC) Programmable Clock Output
#define AT91C_PMC_PCK2        (0x1 << 10) // (PMC) Programmable Clock Output
#define AT91C_PMC_PCK3        (0x1 << 11) // (PMC) Programmable Clock Output
// -------- PMC_SCDR : (PMC Offset: 0x4) System Clock Disable Register --------
// -------- PMC_SCSR : (PMC Offset: 0x8) System Clock Status Register --------
// -------- CKGR_MOR : (PMC Offset: 0x20) Main Oscillator Register --------
// -------- CKGR_MCFR : (PMC Offset: 0x24) Main Clock Frequency Register --------
// -------- CKGR_PLLR : (PMC Offset: 0x2c) PLL B Register --------
// -------- PMC_MCKR : (PMC Offset: 0x30) Master Clock Register --------
#define AT91C_PMC_CSS         (0x3 <<  0) // (PMC) Programmable Clock Selection
#define 	AT91C_PMC_CSS_SLOW_CLK             (0x0) // (PMC) Slow Clock is selected
#define 	AT91C_PMC_CSS_MAIN_CLK             (0x1) // (PMC) Main Clock is selected
#define 	AT91C_PMC_CSS_PLL_CLK              (0x3) // (PMC) Clock from PLL is selected
#define AT91C_PMC_PRES        (0x7 <<  2) // (PMC) Programmable Clock Prescaler
#define 	AT91C_PMC_PRES_CLK                  (0x0 <<  2) // (PMC) Selected clock
#define 	AT91C_PMC_PRES_CLK_2                (0x1 <<  2) // (PMC) Selected clock divided by 2
#define 	AT91C_PMC_PRES_CLK_4                (0x2 <<  2) // (PMC) Selected clock divided by 4
#define 	AT91C_PMC_PRES_CLK_8                (0x3 <<  2) // (PMC) Selected clock divided by 8
#define 	AT91C_PMC_PRES_CLK_16               (0x4 <<  2) // (PMC) Selected clock divided by 16
#define 	AT91C_PMC_PRES_CLK_32               (0x5 <<  2) // (PMC) Selected clock divided by 32
#define 	AT91C_PMC_PRES_CLK_64               (0x6 <<  2) // (PMC) Selected clock divided by 64
// -------- PMC_PCKR : (PMC Offset: 0x40) Programmable Clock Register --------
// -------- PMC_IER : (PMC Offset: 0x60) PMC Interrupt Enable Register --------
#define AT91C_PMC_MOSCS       (0x1 <<  0) // (PMC) MOSC Status/Enable/Disable/Mask
#define AT91C_PMC_LOCK        (0x1 <<  2) // (PMC) PLL Status/Enable/Disable/Mask
#define AT91C_PMC_MCKRDY      (0x1 <<  3) // (PMC) MCK_RDY Status/Enable/Disable/Mask
#define AT91C_PMC_PCK0RDY     (0x1 <<  8) // (PMC) PCK0_RDY Status/Enable/Disable/Mask
#define AT91C_PMC_PCK1RDY     (0x1 <<  9) // (PMC) PCK1_RDY Status/Enable/Disable/Mask
#define AT91C_PMC_PCK2RDY     (0x1 << 10) // (PMC) PCK2_RDY Status/Enable/Disable/Mask
#define AT91C_PMC_PCK3RDY     (0x1 << 11) // (PMC) PCK3_RDY Status/Enable/Disable/Mask
// -------- PMC_IDR : (PMC Offset: 0x64) PMC Interrupt Disable Register --------
// -------- PMC_SR : (PMC Offset: 0x68) PMC Status Register --------
// -------- PMC_IMR : (PMC Offset: 0x6c) PMC Interrupt Mask Register --------

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Reset Controller Interface
// *****************************************************************************
#ifndef __ASSEMBLY__
typedef struct _AT91S_RSTC {
	AT91_REG	 RSTC_RCR; 	// Reset Control Register
	AT91_REG	 RSTC_RSR; 	// Reset Status Register
	AT91_REG	 RSTC_RMR; 	// Reset Mode Register
} AT91S_RSTC, *AT91PS_RSTC;
#else
#define RSTC_RCR        (AT91_CAST(AT91_REG *) 	0x00000000) // (RSTC_RCR) Reset Control Register
#define RSTC_RSR        (AT91_CAST(AT91_REG *) 	0x00000004) // (RSTC_RSR) Reset Status Register
#define RSTC_RMR        (AT91_CAST(AT91_REG *) 	0x00000008) // (RSTC_RMR) Reset Mode Register

#endif
// -------- RSTC_RCR : (RSTC Offset: 0x0) Reset Control Register --------
#define AT91C_RSTC_PROCRST    (0x1 <<  0) // (RSTC) Processor Reset
#define AT91C_RSTC_PERRST     (0x1 <<  2) // (RSTC) Peripheral Reset
#define AT91C_RSTC_EXTRST     (0x1 <<  3) // (RSTC) External Reset
#define AT91C_RSTC_KEY        (0xFF << 24) // (RSTC) Password
// -------- RSTC_RSR : (RSTC Offset: 0x4) Reset Status Register --------
#define AT91C_RSTC_URSTS      (0x1 <<  0) // (RSTC) User Reset Status
#define AT91C_RSTC_RSTTYP     (0x7 <<  8) // (RSTC) Reset Type
#define 	AT91C_RSTC_RSTTYP_GENERAL              (0x0 <<  8) // (RSTC) General reset. Both VDDCORE and VDDBU rising.
#define 	AT91C_RSTC_RSTTYP_WAKEUP               (0x1 <<  8) // (RSTC) WakeUp Reset. VDDCORE rising.
#define 	AT91C_RSTC_RSTTYP_WATCHDOG             (0x2 <<  8) // (RSTC) Watchdog Reset. Watchdog overflow occured.
#define 	AT91C_RSTC_RSTTYP_SOFTWARE             (0x3 <<  8) // (RSTC) Software Reset. Processor reset required by the software.
#define 	AT91C_RSTC_RSTTYP_USER                 (0x4 <<  8) // (RSTC) User Reset. NRST pin detected low.
#define AT91C_RSTC_NRSTL      (0x1 << 16) // (RSTC) NRST pin level
#define AT91C_RSTC_SRCMP      (0x1 << 17) // (RSTC) Software Reset Command in Progress.
// -------- RSTC_RMR : (RSTC Offset: 0x8) Reset Mode Register --------
#define AT91C_RSTC_URSTEN     (0x1 <<  0) // (RSTC) User Reset Enable
#define AT91C_RSTC_URSTIEN    (0x1 <<  4) // (RSTC) User Reset Interrupt Enable
#define AT91C_RSTC_ERSTL      (0xF <<  8) // (RSTC) User Reset Length

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Shut Down Controller Interface
// *****************************************************************************
#ifndef __ASSEMBLY__
typedef struct _AT91S_SHDWC {
	AT91_REG	 SHDWC_SHCR; 	// Shut Down Control Register
	AT91_REG	 SHDWC_SHMR; 	// Shut Down Mode Register
	AT91_REG	 SHDWC_SHSR; 	// Shut Down Status Register
} AT91S_SHDWC, *AT91PS_SHDWC;
#else
#define SHDWC_SHCR      (AT91_CAST(AT91_REG *) 	0x00000000) // (SHDWC_SHCR) Shut Down Control Register
#define SHDWC_SHMR      (AT91_CAST(AT91_REG *) 	0x00000004) // (SHDWC_SHMR) Shut Down Mode Register
#define SHDWC_SHSR      (AT91_CAST(AT91_REG *) 	0x00000008) // (SHDWC_SHSR) Shut Down Status Register

#endif
// -------- SHDWC_SHCR : (SHDWC Offset: 0x0) Shut Down Control Register --------
#define AT91C_SHDWC_SHDW      (0x1 <<  0) // (SHDWC) Processor Reset
#define AT91C_SHDWC_KEY       (0xFF << 24) // (SHDWC) Shut down KEY Password
// -------- SHDWC_SHMR : (SHDWC Offset: 0x4) Shut Down Mode Register --------
#define AT91C_SHDWC_WKMODE0   (0x3 <<  0) // (SHDWC) Wake Up 0 Mode Selection
#define 	AT91C_SHDWC_WKMODE0_NONE                 (0x0) // (SHDWC) None. No detection is performed on the wake up input.
#define 	AT91C_SHDWC_WKMODE0_HIGH                 (0x1) // (SHDWC) High Level.
#define 	AT91C_SHDWC_WKMODE0_LOW                  (0x2) // (SHDWC) Low Level.
#define 	AT91C_SHDWC_WKMODE0_ANYLEVEL             (0x3) // (SHDWC) Any level change.
#define AT91C_SHDWC_CPTWK0    (0xF <<  4) // (SHDWC) Counter On Wake Up 0
#define AT91C_SHDWC_WKMODE1   (0x3 <<  8) // (SHDWC) Wake Up 1 Mode Selection
#define 	AT91C_SHDWC_WKMODE1_NONE                 (0x0 <<  8) // (SHDWC) None. No detection is performed on the wake up input.
#define 	AT91C_SHDWC_WKMODE1_HIGH                 (0x1 <<  8) // (SHDWC) High Level.
#define 	AT91C_SHDWC_WKMODE1_LOW                  (0x2 <<  8) // (SHDWC) Low Level.
#define 	AT91C_SHDWC_WKMODE1_ANYLEVEL             (0x3 <<  8) // (SHDWC) Any level change.
#define AT91C_SHDWC_CPTWK1    (0xF << 12) // (SHDWC) Counter On Wake Up 1
#define AT91C_SHDWC_RTTWKEN   (0x1 << 16) // (SHDWC) Real Time Timer Wake Up Enable
// -------- SHDWC_SHSR : (SHDWC Offset: 0x8) Shut Down Status Register --------
#define AT91C_SHDWC_WAKEUP0   (0x1 <<  0) // (SHDWC) Wake Up 0 Status
#define AT91C_SHDWC_WAKEUP1   (0x1 <<  1) // (SHDWC) Wake Up 1 Status
#define AT91C_SHDWC_FWKUP     (0x1 <<  2) // (SHDWC) Force Wake Up Status
#define AT91C_SHDWC_RTTWK     (0x1 << 16) // (SHDWC) Real Time Timer wake Up

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Real Time Timer Controller Interface
// *****************************************************************************
#ifndef __ASSEMBLY__
typedef struct _AT91S_RTTC {
	AT91_REG	 RTTC_RTMR; 	// Real-time Mode Register
	AT91_REG	 RTTC_RTAR; 	// Real-time Alarm Register
	AT91_REG	 RTTC_RTVR; 	// Real-time Value Register
	AT91_REG	 RTTC_RTSR; 	// Real-time Status Register
} AT91S_RTTC, *AT91PS_RTTC;
#else
#define RTTC_RTMR       (AT91_CAST(AT91_REG *) 	0x00000000) // (RTTC_RTMR) Real-time Mode Register
#define RTTC_RTAR       (AT91_CAST(AT91_REG *) 	0x00000004) // (RTTC_RTAR) Real-time Alarm Register
#define RTTC_RTVR       (AT91_CAST(AT91_REG *) 	0x00000008) // (RTTC_RTVR) Real-time Value Register
#define RTTC_RTSR       (AT91_CAST(AT91_REG *) 	0x0000000C) // (RTTC_RTSR) Real-time Status Register

#endif
// -------- RTTC_RTMR : (RTTC Offset: 0x0) Real-time Mode Register --------
#define AT91C_RTTC_RTPRES     (0xFFFF <<  0) // (RTTC) Real-time Timer Prescaler Value
#define AT91C_RTTC_ALMIEN     (0x1 << 16) // (RTTC) Alarm Interrupt Enable
#define AT91C_RTTC_RTTINCIEN  (0x1 << 17) // (RTTC) Real Time Timer Increment Interrupt Enable
#define AT91C_RTTC_RTTRST     (0x1 << 18) // (RTTC) Real Time Timer Restart
// -------- RTTC_RTAR : (RTTC Offset: 0x4) Real-time Alarm Register --------
#define AT91C_RTTC_ALMV       (0x0 <<  0) // (RTTC) Alarm Value
// -------- RTTC_RTVR : (RTTC Offset: 0x8) Current Real-time Value Register --------
#define AT91C_RTTC_CRTV       (0x0 <<  0) // (RTTC) Current Real-time Value
// -------- RTTC_RTSR : (RTTC Offset: 0xc) Real-time Status Register --------
#define AT91C_RTTC_ALMS       (0x1 <<  0) // (RTTC) Real-time Alarm Status
#define AT91C_RTTC_RTTINC     (0x1 <<  1) // (RTTC) Real-time Timer Increment

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Periodic Interval Timer Controller Interface
// *****************************************************************************
#ifndef __ASSEMBLY__
typedef struct _AT91S_PITC {
	AT91_REG	 PITC_PIMR; 	// Period Interval Mode Register
	AT91_REG	 PITC_PISR; 	// Period Interval Status Register
	AT91_REG	 PITC_PIVR; 	// Period Interval Value Register
	AT91_REG	 PITC_PIIR; 	// Period Interval Image Register
} AT91S_PITC, *AT91PS_PITC;
#else
#define PITC_PIMR       (AT91_CAST(AT91_REG *) 	0x00000000) // (PITC_PIMR) Period Interval Mode Register
#define PITC_PISR       (AT91_CAST(AT91_REG *) 	0x00000004) // (PITC_PISR) Period Interval Status Register
#define PITC_PIVR       (AT91_CAST(AT91_REG *) 	0x00000008) // (PITC_PIVR) Period Interval Value Register
#define PITC_PIIR       (AT91_CAST(AT91_REG *) 	0x0000000C) // (PITC_PIIR) Period Interval Image Register

#endif
// -------- PITC_PIMR : (PITC Offset: 0x0) Periodic Interval Mode Register --------
#define AT91C_PITC_PIV        (0xFFFFF <<  0) // (PITC) Periodic Interval Value
#define AT91C_PITC_PITEN      (0x1 << 24) // (PITC) Periodic Interval Timer Enabled
#define AT91C_PITC_PITIEN     (0x1 << 25) // (PITC) Periodic Interval Timer Interrupt Enable
// -------- PITC_PISR : (PITC Offset: 0x4) Periodic Interval Status Register --------
#define AT91C_PITC_PITS       (0x1 <<  0) // (PITC) Periodic Interval Timer Status
// -------- PITC_PIVR : (PITC Offset: 0x8) Periodic Interval Value Register --------
#define AT91C_PITC_CPIV       (0xFFFFF <<  0) // (PITC) Current Periodic Interval Value
#define AT91C_PITC_PICNT      (0xFFF << 20) // (PITC) Periodic Interval Counter
// -------- PITC_PIIR : (PITC Offset: 0xc) Periodic Interval Image Register --------

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Watchdog Timer Controller Interface
// *****************************************************************************
#ifndef __ASSEMBLY__
typedef struct _AT91S_WDTC {
	AT91_REG	 WDTC_WDCR; 	// Watchdog Control Register
	AT91_REG	 WDTC_WDMR; 	// Watchdog Mode Register
	AT91_REG	 WDTC_WDSR; 	// Watchdog Status Register
} AT91S_WDTC, *AT91PS_WDTC;
#else
#define WDTC_WDCR       (AT91_CAST(AT91_REG *) 	0x00000000) // (WDTC_WDCR) Watchdog Control Register
#define WDTC_WDMR       (AT91_CAST(AT91_REG *) 	0x00000004) // (WDTC_WDMR) Watchdog Mode Register
#define WDTC_WDSR       (AT91_CAST(AT91_REG *) 	0x00000008) // (WDTC_WDSR) Watchdog Status Register

#endif
// -------- WDTC_WDCR : (WDTC Offset: 0x0) Periodic Interval Image Register --------
#define AT91C_WDTC_WDRSTT     (0x1 <<  0) // (WDTC) Watchdog Restart
#define AT91C_WDTC_KEY        (0xFF << 24) // (WDTC) Watchdog KEY Password
// -------- WDTC_WDMR : (WDTC Offset: 0x4) Watchdog Mode Register --------
#define AT91C_WDTC_WDV        (0xFFF <<  0) // (WDTC) Watchdog Timer Restart
#define AT91C_WDTC_WDFIEN     (0x1 << 12) // (WDTC) Watchdog Fault Interrupt Enable
#define AT91C_WDTC_WDRSTEN    (0x1 << 13) // (WDTC) Watchdog Reset Enable
#define AT91C_WDTC_WDRPROC    (0x1 << 14) // (WDTC) Watchdog Timer Restart
#define AT91C_WDTC_WDDIS      (0x1 << 15) // (WDTC) Watchdog Disable
#define AT91C_WDTC_WDD        (0xFFF << 16) // (WDTC) Watchdog Delta Value
#define AT91C_WDTC_WDDBGHLT   (0x1 << 28) // (WDTC) Watchdog Debug Halt
#define AT91C_WDTC_WDIDLEHLT  (0x1 << 29) // (WDTC) Watchdog Idle Halt
// -------- WDTC_WDSR : (WDTC Offset: 0x8) Watchdog Status Register --------
#define AT91C_WDTC_WDUNF      (0x1 <<  0) // (WDTC) Watchdog Underflow
#define AT91C_WDTC_WDERR      (0x1 <<  1) // (WDTC) Watchdog Error

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Memory Controller Interface
// *****************************************************************************
#ifndef __ASSEMBLY__
typedef struct _AT91S_MC {
	AT91_REG	 MC_RCR; 	// MC Remap Control Register
	AT91_REG	 MC_ASR; 	// MC Abort Status Register
	AT91_REG	 MC_AASR; 	// MC Abort Address Status Register
	AT91_REG	 Reserved0[1]; 	//
	AT91_REG	 MC_PUIA[16]; 	// MC Protection Unit Area
	AT91_REG	 MC_PUP; 	// MC Protection Unit Peripherals
	AT91_REG	 MC_PUER; 	// MC Protection Unit Enable Register
	AT91_REG	 Reserved1[2]; 	//
	AT91_REG	 MC_FMR; 	// MC Flash Mode Register
	AT91_REG	 MC_FCR; 	// MC Flash Command Register
	AT91_REG	 MC_FSR; 	// MC Flash Status Register
} AT91S_MC, *AT91PS_MC;
#else
#define MC_RCR          (AT91_CAST(AT91_REG *) 	0x00000000) // (MC_RCR) MC Remap Control Register
#define MC_ASR          (AT91_CAST(AT91_REG *) 	0x00000004) // (MC_ASR) MC Abort Status Register
#define MC_AASR         (AT91_CAST(AT91_REG *) 	0x00000008) // (MC_AASR) MC Abort Address Status Register
#define MC_PUIA         (AT91_CAST(AT91_REG *) 	0x00000010) // (MC_PUIA) MC Protection Unit Area
#define MC_PUP          (AT91_CAST(AT91_REG *) 	0x00000050) // (MC_PUP) MC Protection Unit Peripherals
#define MC_PUER         (AT91_CAST(AT91_REG *) 	0x00000054) // (MC_PUER) MC Protection Unit Enable Register
#define MC_FMR          (AT91_CAST(AT91_REG *) 	0x00000060) // (MC_FMR) MC Flash Mode Register
#define MC_FCR          (AT91_CAST(AT91_REG *) 	0x00000064) // (MC_FCR) MC Flash Command Register
#define MC_FSR          (AT91_CAST(AT91_REG *) 	0x00000068) // (MC_FSR) MC Flash Status Register

#endif
// -------- MC_RCR : (MC Offset: 0x0) MC Remap Control Register --------
#define AT91C_MC_RCB          (0x1 <<  0) // (MC) Remap Command Bit
// -------- MC_ASR : (MC Offset: 0x4) MC Abort Status Register --------
#define AT91C_MC_UNDADD       (0x1 <<  0) // (MC) Undefined Addess Abort Status
#define AT91C_MC_MISADD       (0x1 <<  1) // (MC) Misaligned Addess Abort Status
#define AT91C_MC_MPU          (0x1 <<  2) // (MC) Memory protection Unit Abort Status
#define AT91C_MC_ABTSZ        (0x3 <<  8) // (MC) Abort Size Status
#define 	AT91C_MC_ABTSZ_BYTE                 (0x0 <<  8) // (MC) Byte
#define 	AT91C_MC_ABTSZ_HWORD                (0x1 <<  8) // (MC) Half-word
#define 	AT91C_MC_ABTSZ_WORD                 (0x2 <<  8) // (MC) Word
#define AT91C_MC_ABTTYP       (0x3 << 10) // (MC) Abort Type Status
#define 	AT91C_MC_ABTTYP_DATAR                (0x0 << 10) // (MC) Data Read
#define 	AT91C_MC_ABTTYP_DATAW                (0x1 << 10) // (MC) Data Write
#define 	AT91C_MC_ABTTYP_FETCH                (0x2 << 10) // (MC) Code Fetch
#define AT91C_MC_MST0         (0x1 << 16) // (MC) Master 0 Abort Source
#define AT91C_MC_MST1         (0x1 << 17) // (MC) Master 1 Abort Source
#define AT91C_MC_SVMST0       (0x1 << 24) // (MC) Saved Master 0 Abort Source
#define AT91C_MC_SVMST1       (0x1 << 25) // (MC) Saved Master 1 Abort Source
// -------- MC_PUIA : (MC Offset: 0x10) MC Protection Unit Area --------
#define AT91C_MC_PROT         (0x3 <<  0) // (MC) Protection
#define 	AT91C_MC_PROT_PNAUNA               (0x0) // (MC) Privilege: No Access, User: No Access
#define 	AT91C_MC_PROT_PRWUNA               (0x1) // (MC) Privilege: Read/Write, User: No Access
#define 	AT91C_MC_PROT_PRWURO               (0x2) // (MC) Privilege: Read/Write, User: Read Only
#define 	AT91C_MC_PROT_PRWURW               (0x3) // (MC) Privilege: Read/Write, User: Read/Write
#define AT91C_MC_SIZE         (0xF <<  4) // (MC) Internal Area Size
#define 	AT91C_MC_SIZE_1KB                  (0x0 <<  4) // (MC) Area size 1KByte
#define 	AT91C_MC_SIZE_2KB                  (0x1 <<  4) // (MC) Area size 2KByte
#define 	AT91C_MC_SIZE_4KB                  (0x2 <<  4) // (MC) Area size 4KByte
#define 	AT91C_MC_SIZE_8KB                  (0x3 <<  4) // (MC) Area size 8KByte
#define 	AT91C_MC_SIZE_16KB                 (0x4 <<  4) // (MC) Area size 16KByte
#define 	AT91C_MC_SIZE_32KB                 (0x5 <<  4) // (MC) Area size 32KByte
#define 	AT91C_MC_SIZE_64KB                 (0x6 <<  4) // (MC) Area size 64KByte
#define 	AT91C_MC_SIZE_128KB                (0x7 <<  4) // (MC) Area size 128KByte
#define 	AT91C_MC_SIZE_256KB                (0x8 <<  4) // (MC) Area size 256KByte
#define 	AT91C_MC_SIZE_512KB                (0x9 <<  4) // (MC) Area size 512KByte
#define 	AT91C_MC_SIZE_1MB                  (0xA <<  4) // (MC) Area size 1MByte
#define 	AT91C_MC_SIZE_2MB                  (0xB <<  4) // (MC) Area size 2MByte
#define 	AT91C_MC_SIZE_4MB                  (0xC <<  4) // (MC) Area size 4MByte
#define 	AT91C_MC_SIZE_8MB                  (0xD <<  4) // (MC) Area size 8MByte
#define 	AT91C_MC_SIZE_16MB                 (0xE <<  4) // (MC) Area size 16MByte
#define 	AT91C_MC_SIZE_64MB                 (0xF <<  4) // (MC) Area size 64MByte
#define AT91C_MC_BA           (0x3FFFF << 10) // (MC) Internal Area Base Address
// -------- MC_PUP : (MC Offset: 0x50) MC Protection Unit Peripheral --------
// -------- MC_PUER : (MC Offset: 0x54) MC Protection Unit Area --------
#define AT91C_MC_PUEB         (0x1 <<  0) // (MC) Protection Unit enable Bit
// -------- MC_FMR : (MC Offset: 0x60) MC Flash Mode Register --------
#define AT91C_MC_EOP          (0x1 <<  0) // (MC) End Of Programming Flag
#define AT91C_MC_EOL          (0x1 <<  1) // (MC) End Of Lock/Unlock Flag
#define AT91C_MC_LOCKE        (0x1 <<  2) // (MC) Lock Error Flag
#define AT91C_MC_PROGE        (0x1 <<  3) // (MC) Programming Error Flag
#define AT91C_MC_NEBP         (0x1 <<  7) // (MC) No Erase Before Programming
#define AT91C_MC_FWS          (0x3 <<  8) // (MC) Flash Wait State
#define 	AT91C_MC_FWS_0FWS                 (0x0 <<  8) // (MC) 1 cycle for Read, 2 for Write operations
#define 	AT91C_MC_FWS_1FWS                 (0x1 <<  8) // (MC) 2 cycles for Read, 3 for Write operations
#define 	AT91C_MC_FWS_2FWS                 (0x2 <<  8) // (MC) 3 cycles for Read, 4 for Write operations
#define 	AT91C_MC_FWS_3FWS                 (0x3 <<  8) // (MC) 4 cycles for Read, 4 for Write operations
#define AT91C_MC_FMCN         (0xFF << 16) // (MC) Flash Microsecond Cycle Number
// -------- MC_FCR : (MC Offset: 0x64) MC Flash Command Register --------
#define AT91C_MC_FCMD         (0xF <<  0) // (MC) Flash Command
#define 	AT91C_MC_FCMD_START_PROG           (0x1) // (MC) Starts the programming of th epage specified by PAGEN.
#define 	AT91C_MC_FCMD_LOCK                 (0x2) // (MC) Starts a lock sequence of the sector defined by the bits 4 to 7 of the field PAGEN.
#define 	AT91C_MC_FCMD_PROG_AND_LOCK        (0x3) // (MC) The lock sequence automatically happens after the programming sequence is completed.
#define 	AT91C_MC_FCMD_UNLOCK               (0x4) // (MC) Starts an unlock sequence of the sector defined by the bits 4 to 7 of the field PAGEN.
#define 	AT91C_MC_FCMD_ERASE_ALL            (0x8) // (MC) Starts the erase of the entire flash.If at least a page is locked, the command is cancelled.
#define AT91C_MC_PAGEN        (0x3FF <<  8) // (MC) Page Number
#define AT91C_MC_KEY          (0xFF << 24) // (MC) Writing Protect Key
// -------- MC_FSR : (MC Offset: 0x68) MC Flash Status Register --------
#define AT91C_MC_LOCKS0       (0x1 << 16) // (MC) Sector 0 Lock Status
#define AT91C_MC_LOCKS1       (0x1 << 17) // (MC) Sector 1 Lock Status
#define AT91C_MC_LOCKS2       (0x1 << 18) // (MC) Sector 2 Lock Status
#define AT91C_MC_LOCKS3       (0x1 << 19) // (MC) Sector 3 Lock Status
#define AT91C_MC_LOCKS4       (0x1 << 20) // (MC) Sector 4 Lock Status
#define AT91C_MC_LOCKS5       (0x1 << 21) // (MC) Sector 5 Lock Status
#define AT91C_MC_LOCKS6       (0x1 << 22) // (MC) Sector 6 Lock Status
#define AT91C_MC_LOCKS7       (0x1 << 23) // (MC) Sector 7 Lock Status
#define AT91C_MC_LOCKS8       (0x1 << 24) // (MC) Sector 8 Lock Status
#define AT91C_MC_LOCKS9       (0x1 << 25) // (MC) Sector 9 Lock Status
#define AT91C_MC_LOCKS10      (0x1 << 26) // (MC) Sector 10 Lock Status
#define AT91C_MC_LOCKS11      (0x1 << 27) // (MC) Sector 11 Lock Status
#define AT91C_MC_LOCKS12      (0x1 << 28) // (MC) Sector 12 Lock Status
#define AT91C_MC_LOCKS13      (0x1 << 29) // (MC) Sector 13 Lock Status
#define AT91C_MC_LOCKS14      (0x1 << 30) // (MC) Sector 14 Lock Status
#define AT91C_MC_LOCKS15      (0x1 << 31) // (MC) Sector 15 Lock Status

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Control Area Network MailBox Interface
// *****************************************************************************
#ifndef __ASSEMBLY__
typedef struct _AT91S_CAN_MB {
	AT91_REG	 CAN_MB_MMR; 	// MailBox Mode Register
	AT91_REG	 CAN_MB_MAM; 	// MailBox Acceptance Mask Register
	AT91_REG	 CAN_MB_MID; 	// MailBox ID Register
	AT91_REG	 CAN_MB_MFID; 	// MailBox Family ID Register
	AT91_REG	 CAN_MB_MSR; 	// MailBox Status Register
	AT91_REG	 CAN_MB_MDL; 	// MailBox Data Low Register
	AT91_REG	 CAN_MB_MDH; 	// MailBox Data High Register
	AT91_REG	 CAN_MB_MCR; 	// MailBox Control Register
} AT91S_CAN_MB, *AT91PS_CAN_MB;
#else
#define CAN_MMR         (AT91_CAST(AT91_REG *) 	0x00000000) // (CAN_MMR) MailBox Mode Register
#define CAN_MAM         (AT91_CAST(AT91_REG *) 	0x00000004) // (CAN_MAM) MailBox Acceptance Mask Register
#define CAN_MID         (AT91_CAST(AT91_REG *) 	0x00000008) // (CAN_MID) MailBox ID Register
#define CAN_MFID        (AT91_CAST(AT91_REG *) 	0x0000000C) // (CAN_MFID) MailBox Family ID Register
#define CAN_MSR         (AT91_CAST(AT91_REG *) 	0x00000010) // (CAN_MSR) MailBox Status Register
#define CAN_MDL         (AT91_CAST(AT91_REG *) 	0x00000014) // (CAN_MDL) MailBox Data Low Register
#define CAN_MDH         (AT91_CAST(AT91_REG *) 	0x00000018) // (CAN_MDH) MailBox Data High Register
#define CAN_MCR         (AT91_CAST(AT91_REG *) 	0x0000001C) // (CAN_MCR) MailBox Control Register

#endif
// -------- CAN_MMR : (CAN_MB Offset: 0x0) CAN Message Mode Register --------
#define AT91C_CAN_MTIMEMARK   (0xFFFF <<  0) // (CAN_MB) Mailbox Timemark
#define AT91C_CAN_PRIOR       (0xF << 16) // (CAN_MB) Mailbox Priority
#define AT91C_CAN_MOT         (0x7 << 24) // (CAN_MB) Mailbox Object Type
#define 	AT91C_CAN_MOT_DIS                  (0x0 << 24) // (CAN_MB)
#define 	AT91C_CAN_MOT_RX                   (0x1 << 24) // (CAN_MB)
#define 	AT91C_CAN_MOT_RXOVERWRITE          (0x2 << 24) // (CAN_MB)
#define 	AT91C_CAN_MOT_TX                   (0x3 << 24) // (CAN_MB)
#define 	AT91C_CAN_MOT_CONSUMER             (0x4 << 24) // (CAN_MB)
#define 	AT91C_CAN_MOT_PRODUCER             (0x5 << 24) // (CAN_MB)
// -------- CAN_MAM : (CAN_MB Offset: 0x4) CAN Message Acceptance Mask Register --------
#define AT91C_CAN_MIDvB       (0x3FFFF <<  0) // (CAN_MB) Complementary bits for identifier in extended mode
#define AT91C_CAN_MIDvA       (0x7FF << 18) // (CAN_MB) Identifier for standard frame mode
#define AT91C_CAN_MIDE        (0x1 << 29) // (CAN_MB) Identifier Version
// -------- CAN_MID : (CAN_MB Offset: 0x8) CAN Message ID Register --------
// -------- CAN_MFID : (CAN_MB Offset: 0xc) CAN Message Family ID Register --------
// -------- CAN_MSR : (CAN_MB Offset: 0x10) CAN Message Status Register --------
#define AT91C_CAN_MTIMESTAMP  (0xFFFF <<  0) // (CAN_MB) Timer Value
#define AT91C_CAN_MDLC        (0xF << 16) // (CAN_MB) Mailbox Data Length Code
#define AT91C_CAN_MRTR        (0x1 << 20) // (CAN_MB) Mailbox Remote Transmission Request
#define AT91C_CAN_MABT        (0x1 << 22) // (CAN_MB) Mailbox Message Abort
#define AT91C_CAN_MRDY        (0x1 << 23) // (CAN_MB) Mailbox Ready
#define AT91C_CAN_MMI         (0x1 << 24) // (CAN_MB) Mailbox Message Ignored
// -------- CAN_MDL : (CAN_MB Offset: 0x14) CAN Message Data Low Register --------
// -------- CAN_MDH : (CAN_MB Offset: 0x18) CAN Message Data High Register --------
// -------- CAN_MCR : (CAN_MB Offset: 0x1c) CAN Message Control Register --------
#define AT91C_CAN_MACR        (0x1 << 22) // (CAN_MB) Abort Request for Mailbox
#define AT91C_CAN_MTCR        (0x1 << 23) // (CAN_MB) Mailbox Transfer Command

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Control Area Network Interface
// *****************************************************************************
#ifndef __ASSEMBLY__
typedef struct _AT91S_CAN {
	AT91_REG	 CAN_MR; 	// Mode Register
	AT91_REG	 CAN_IER; 	// Interrupt Enable Register
	AT91_REG	 CAN_IDR; 	// Interrupt Disable Register
	AT91_REG	 CAN_IMR; 	// Interrupt Mask Register
	AT91_REG	 CAN_SR; 	// Status Register
	AT91_REG	 CAN_BR; 	// Baudrate Register
	AT91_REG	 CAN_TIM; 	// Timer Register
	AT91_REG	 CAN_TIMESTP; 	// Time Stamp Register
	AT91_REG	 CAN_ECR; 	// Error Counter Register
	AT91_REG	 CAN_TCR; 	// Transfer Command Register
	AT91_REG	 CAN_ACR; 	// Abort Command Register
	AT91_REG	 Reserved0[52]; 	//
	AT91_REG	 CAN_VR; 	// Version Register
	AT91_REG	 Reserved1[64]; 	//
	AT91S_CAN_MB	 CAN_MB0; 	// CAN Mailbox 0
	AT91S_CAN_MB	 CAN_MB1; 	// CAN Mailbox 1
	AT91S_CAN_MB	 CAN_MB2; 	// CAN Mailbox 2
	AT91S_CAN_MB	 CAN_MB3; 	// CAN Mailbox 3
	AT91S_CAN_MB	 CAN_MB4; 	// CAN Mailbox 4
	AT91S_CAN_MB	 CAN_MB5; 	// CAN Mailbox 5
	AT91S_CAN_MB	 CAN_MB6; 	// CAN Mailbox 6
	AT91S_CAN_MB	 CAN_MB7; 	// CAN Mailbox 7
	AT91S_CAN_MB	 CAN_MB8; 	// CAN Mailbox 8
	AT91S_CAN_MB	 CAN_MB9; 	// CAN Mailbox 9
	AT91S_CAN_MB	 CAN_MB10; 	// CAN Mailbox 10
	AT91S_CAN_MB	 CAN_MB11; 	// CAN Mailbox 11
	AT91S_CAN_MB	 CAN_MB12; 	// CAN Mailbox 12
	AT91S_CAN_MB	 CAN_MB13; 	// CAN Mailbox 13
	AT91S_CAN_MB	 CAN_MB14; 	// CAN Mailbox 14
	AT91S_CAN_MB	 CAN_MB15; 	// CAN Mailbox 15
} AT91S_CAN, *AT91PS_CAN;
#else
#define CAN_MR          (AT91_CAST(AT91_REG *) 	0x00000000) // (CAN_MR) Mode Register
#define CAN_IER         (AT91_CAST(AT91_REG *) 	0x00000004) // (CAN_IER) Interrupt Enable Register
#define CAN_IDR         (AT91_CAST(AT91_REG *) 	0x00000008) // (CAN_IDR) Interrupt Disable Register
#define CAN_IMR         (AT91_CAST(AT91_REG *) 	0x0000000C) // (CAN_IMR) Interrupt Mask Register
#define CAN_SR          (AT91_CAST(AT91_REG *) 	0x00000010) // (CAN_SR) Status Register
#define CAN_BR          (AT91_CAST(AT91_REG *) 	0x00000014) // (CAN_BR) Baudrate Register
#define CAN_TIM         (AT91_CAST(AT91_REG *) 	0x00000018) // (CAN_TIM) Timer Register
#define CAN_TIMESTP     (AT91_CAST(AT91_REG *) 	0x0000001C) // (CAN_TIMESTP) Time Stamp Register
#define CAN_ECR         (AT91_CAST(AT91_REG *) 	0x00000020) // (CAN_ECR) Error Counter Register
#define CAN_TCR         (AT91_CAST(AT91_REG *) 	0x00000024) // (CAN_TCR) Transfer Command Register
#define CAN_ACR         (AT91_CAST(AT91_REG *) 	0x00000028) // (CAN_ACR) Abort Command Register
#define CAN_VR          (AT91_CAST(AT91_REG *) 	0x000000FC) // (CAN_VR) Version Register

#endif
// -------- CAN_MR : (CAN Offset: 0x0) CAN Mode Register --------
#define AT91C_CAN_CANEN       (0x1 <<  0) // (CAN) CAN Controller Enable
#define AT91C_CAN_LPM         (0x1 <<  1) // (CAN) Disable/Enable Low Power Mode
#define AT91C_CAN_ABM         (0x1 <<  2) // (CAN) Disable/Enable Autobaud/Listen Mode
#define AT91C_CAN_OVL         (0x1 <<  3) // (CAN) Disable/Enable Overload Frame
#define AT91C_CAN_TEOF        (0x1 <<  4) // (CAN) Time Stamp messages at each end of Frame
#define AT91C_CAN_TTM         (0x1 <<  5) // (CAN) Disable/Enable Time Trigger Mode
#define AT91C_CAN_TIMFRZ      (0x1 <<  6) // (CAN) Enable Timer Freeze
#define AT91C_CAN_DRPT        (0x1 <<  7) // (CAN) Disable Repeat
// -------- CAN_IER : (CAN Offset: 0x4) CAN Interrupt Enable Register --------
#define AT91C_CAN_MB0         (0x1 <<  0) // (CAN) Mailbox 0 Flag
#define AT91C_CAN_MB1         (0x1 <<  1) // (CAN) Mailbox 1 Flag
#define AT91C_CAN_MB2         (0x1 <<  2) // (CAN) Mailbox 2 Flag
#define AT91C_CAN_MB3         (0x1 <<  3) // (CAN) Mailbox 3 Flag
#define AT91C_CAN_MB4         (0x1 <<  4) // (CAN) Mailbox 4 Flag
#define AT91C_CAN_MB5         (0x1 <<  5) // (CAN) Mailbox 5 Flag
#define AT91C_CAN_MB6         (0x1 <<  6) // (CAN) Mailbox 6 Flag
#define AT91C_CAN_MB7         (0x1 <<  7) // (CAN) Mailbox 7 Flag
#define AT91C_CAN_MB8         (0x1 <<  8) // (CAN) Mailbox 8 Flag
#define AT91C_CAN_MB9         (0x1 <<  9) // (CAN) Mailbox 9 Flag
#define AT91C_CAN_MB10        (0x1 << 10) // (CAN) Mailbox 10 Flag
#define AT91C_CAN_MB11        (0x1 << 11) // (CAN) Mailbox 11 Flag
#define AT91C_CAN_MB12        (0x1 << 12) // (CAN) Mailbox 12 Flag
#define AT91C_CAN_MB13        (0x1 << 13) // (CAN) Mailbox 13 Flag
#define AT91C_CAN_MB14        (0x1 << 14) // (CAN) Mailbox 14 Flag
#define AT91C_CAN_MB15        (0x1 << 15) // (CAN) Mailbox 15 Flag
#define AT91C_CAN_ERRA        (0x1 << 16) // (CAN) Error Active Mode Flag
#define AT91C_CAN_WARN        (0x1 << 17) // (CAN) Warning Limit Flag
#define AT91C_CAN_ERRP        (0x1 << 18) // (CAN) Error Passive Mode Flag
#define AT91C_CAN_BOFF        (0x1 << 19) // (CAN) Bus Off Mode Flag
#define AT91C_CAN_SLEEP       (0x1 << 20) // (CAN) Sleep Flag
#define AT91C_CAN_WAKEUP      (0x1 << 21) // (CAN) Wakeup Flag
#define AT91C_CAN_TOVF        (0x1 << 22) // (CAN) Timer Overflow Flag
#define AT91C_CAN_TSTP        (0x1 << 23) // (CAN) Timestamp Flag
#define AT91C_CAN_CERR        (0x1 << 24) // (CAN) CRC Error
#define AT91C_CAN_SERR        (0x1 << 25) // (CAN) Stuffing Error
#define AT91C_CAN_AERR        (0x1 << 26) // (CAN) Acknowledgment Error
#define AT91C_CAN_FERR        (0x1 << 27) // (CAN) Form Error
#define AT91C_CAN_BERR        (0x1 << 28) // (CAN) Bit Error
// -------- CAN_IDR : (CAN Offset: 0x8) CAN Interrupt Disable Register --------
// -------- CAN_IMR : (CAN Offset: 0xc) CAN Interrupt Mask Register --------
// -------- CAN_SR : (CAN Offset: 0x10) CAN Status Register --------
#define AT91C_CAN_RBSY        (0x1 << 29) // (CAN) Receiver Busy
#define AT91C_CAN_TBSY        (0x1 << 30) // (CAN) Transmitter Busy
#define AT91C_CAN_OVLY        (0x1 << 31) // (CAN) Overload Busy
// -------- CAN_BR : (CAN Offset: 0x14) CAN Baudrate Register --------
#define AT91C_CAN_PHASE2      (0x7 <<  0) // (CAN) Phase 2 segment
#define AT91C_CAN_PHASE1      (0x7 <<  4) // (CAN) Phase 1 segment
#define AT91C_CAN_PROPAG      (0x7 <<  8) // (CAN) Programmation time segment
#define AT91C_CAN_SYNC        (0x3 << 12) // (CAN) Re-synchronization jump width segment
#define AT91C_CAN_BRP         (0x7F << 16) // (CAN) Baudrate Prescaler
#define AT91C_CAN_SMP         (0x1 << 24) // (CAN) Sampling mode
// -------- CAN_TIM : (CAN Offset: 0x18) CAN Timer Register --------
#define AT91C_CAN_TIMER       (0xFFFF <<  0) // (CAN) Timer field
// -------- CAN_TIMESTP : (CAN Offset: 0x1c) CAN Timestamp Register --------
// -------- CAN_ECR : (CAN Offset: 0x20) CAN Error Counter Register --------
#define AT91C_CAN_REC         (0xFF <<  0) // (CAN) Receive Error Counter
#define AT91C_CAN_TEC         (0xFF << 16) // (CAN) Transmit Error Counter
// -------- CAN_TCR : (CAN Offset: 0x24) CAN Transfer Command Register --------
#define AT91C_CAN_TIMRST      (0x1 << 31) // (CAN) Timer Reset Field
// -------- CAN_ACR : (CAN Offset: 0x28) CAN Abort Command Register --------

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Timer Counter Channel Interface
// *****************************************************************************
#ifndef __ASSEMBLY__
typedef struct _AT91S_TC {
	AT91_REG	 TC_CCR; 	// Channel Control Register
	AT91_REG	 TC_CMR; 	// Channel Mode Register (Capture Mode / Waveform Mode)
	AT91_REG	 Reserved0[2]; 	//
	AT91_REG	 TC_CV; 	// Counter Value
	AT91_REG	 TC_RA; 	// Register A
	AT91_REG	 TC_RB; 	// Register B
	AT91_REG	 TC_RC; 	// Register C
	AT91_REG	 TC_SR; 	// Status Register
	AT91_REG	 TC_IER; 	// Interrupt Enable Register
	AT91_REG	 TC_IDR; 	// Interrupt Disable Register
	AT91_REG	 TC_IMR; 	// Interrupt Mask Register
} AT91S_TC, *AT91PS_TC;
#else
#define TC_CCR          (AT91_CAST(AT91_REG *) 	0x00000000) // (TC_CCR) Channel Control Register
#define TC_CMR          (AT91_CAST(AT91_REG *) 	0x00000004) // (TC_CMR) Channel Mode Register (Capture Mode / Waveform Mode)
#define TC_CV           (AT91_CAST(AT91_REG *) 	0x00000010) // (TC_CV) Counter Value
#define TC_RA           (AT91_CAST(AT91_REG *) 	0x00000014) // (TC_RA) Register A
#define TC_RB           (AT91_CAST(AT91_REG *) 	0x00000018) // (TC_RB) Register B
#define TC_RC           (AT91_CAST(AT91_REG *) 	0x0000001C) // (TC_RC) Register C
#define TC_SR           (AT91_CAST(AT91_REG *) 	0x00000020) // (TC_SR) Status Register
#define TC_IER          (AT91_CAST(AT91_REG *) 	0x00000024) // (TC_IER) Interrupt Enable Register
#define TC_IDR          (AT91_CAST(AT91_REG *) 	0x00000028) // (TC_IDR) Interrupt Disable Register
#define TC_IMR          (AT91_CAST(AT91_REG *) 	0x0000002C) // (TC_IMR) Interrupt Mask Register

#endif
// -------- TC_CCR : (TC Offset: 0x0) TC Channel Control Register --------
#define AT91C_TC_CLKEN        (0x1 <<  0) // (TC) Counter Clock Enable Command
#define AT91C_TC_CLKDIS       (0x1 <<  1) // (TC) Counter Clock Disable Command
#define AT91C_TC_SWTRG        (0x1 <<  2) // (TC) Software Trigger Command
// -------- TC_CMR : (TC Offset: 0x4) TC Channel Mode Register: Capture Mode / Waveform Mode --------
#define AT91C_TC_CLKS         (0x7 <<  0) // (TC) Clock Selection
#define 	AT91C_TC_CLKS_TIMER_DIV1_CLOCK     (0x0) // (TC) Clock selected: TIMER_DIV1_CLOCK
#define 	AT91C_TC_CLKS_TIMER_DIV2_CLOCK     (0x1) // (TC) Clock selected: TIMER_DIV2_CLOCK
#define 	AT91C_TC_CLKS_TIMER_DIV3_CLOCK     (0x2) // (TC) Clock selected: TIMER_DIV3_CLOCK
#define 	AT91C_TC_CLKS_TIMER_DIV4_CLOCK     (0x3) // (TC) Clock selected: TIMER_DIV4_CLOCK
#define 	AT91C_TC_CLKS_TIMER_DIV5_CLOCK     (0x4) // (TC) Clock selected: TIMER_DIV5_CLOCK
#define 	AT91C_TC_CLKS_XC0                  (0x5) // (TC) Clock selected: XC0
#define 	AT91C_TC_CLKS_XC1                  (0x6) // (TC) Clock selected: XC1
#define 	AT91C_TC_CLKS_XC2                  (0x7) // (TC) Clock selected: XC2
#define AT91C_TC_CLKI         (0x1 <<  3) // (TC) Clock Invert
#define AT91C_TC_BURST        (0x3 <<  4) // (TC) Burst Signal Selection
#define 	AT91C_TC_BURST_NONE                 (0x0 <<  4) // (TC) The clock is not gated by an external signal
#define 	AT91C_TC_BURST_XC0                  (0x1 <<  4) // (TC) XC0 is ANDed with the selected clock
#define 	AT91C_TC_BURST_XC1                  (0x2 <<  4) // (TC) XC1 is ANDed with the selected clock
#define 	AT91C_TC_BURST_XC2                  (0x3 <<  4) // (TC) XC2 is ANDed with the selected clock
#define AT91C_TC_CPCSTOP      (0x1 <<  6) // (TC) Counter Clock Stopped with RC Compare
#define AT91C_TC_LDBSTOP      (0x1 <<  6) // (TC) Counter Clock Stopped with RB Loading
#define AT91C_TC_CPCDIS       (0x1 <<  7) // (TC) Counter Clock Disable with RC Compare
#define AT91C_TC_LDBDIS       (0x1 <<  7) // (TC) Counter Clock Disabled with RB Loading
#define AT91C_TC_ETRGEDG      (0x3 <<  8) // (TC) External Trigger Edge Selection
#define 	AT91C_TC_ETRGEDG_NONE                 (0x0 <<  8) // (TC) Edge: None
#define 	AT91C_TC_ETRGEDG_RISING               (0x1 <<  8) // (TC) Edge: rising edge
#define 	AT91C_TC_ETRGEDG_FALLING              (0x2 <<  8) // (TC) Edge: falling edge
#define 	AT91C_TC_ETRGEDG_BOTH                 (0x3 <<  8) // (TC) Edge: each edge
#define AT91C_TC_EEVTEDG      (0x3 <<  8) // (TC) External Event Edge Selection
#define 	AT91C_TC_EEVTEDG_NONE                 (0x0 <<  8) // (TC) Edge: None
#define 	AT91C_TC_EEVTEDG_RISING               (0x1 <<  8) // (TC) Edge: rising edge
#define 	AT91C_TC_EEVTEDG_FALLING              (0x2 <<  8) // (TC) Edge: falling edge
#define 	AT91C_TC_EEVTEDG_BOTH                 (0x3 <<  8) // (TC) Edge: each edge
#define AT91C_TC_EEVT         (0x3 << 10) // (TC) External Event  Selection
#define 	AT91C_TC_EEVT_TIOB                 (0x0 << 10) // (TC) Signal selected as external event: TIOB TIOB direction: input
#define 	AT91C_TC_EEVT_XC0                  (0x1 << 10) // (TC) Signal selected as external event: XC0 TIOB direction: output
#define 	AT91C_TC_EEVT_XC1                  (0x2 << 10) // (TC) Signal selected as external event: XC1 TIOB direction: output
#define 	AT91C_TC_EEVT_XC2                  (0x3 << 10) // (TC) Signal selected as external event: XC2 TIOB direction: output
#define AT91C_TC_ABETRG       (0x1 << 10) // (TC) TIOA or TIOB External Trigger Selection
#define AT91C_TC_ENETRG       (0x1 << 12) // (TC) External Event Trigger enable
#define AT91C_TC_WAVESEL      (0x3 << 13) // (TC) Waveform  Selection
#define 	AT91C_TC_WAVESEL_UP                   (0x0 << 13) // (TC) UP mode without atomatic trigger on RC Compare
#define 	AT91C_TC_WAVESEL_UPDOWN               (0x1 << 13) // (TC) UPDOWN mode without automatic trigger on RC Compare
#define 	AT91C_TC_WAVESEL_UP_AUTO              (0x2 << 13) // (TC) UP mode with automatic trigger on RC Compare
#define 	AT91C_TC_WAVESEL_UPDOWN_AUTO          (0x3 << 13) // (TC) UPDOWN mode with automatic trigger on RC Compare
#define AT91C_TC_CPCTRG       (0x1 << 14) // (TC) RC Compare Trigger Enable
#define AT91C_TC_WAVE         (0x1 << 15) // (TC)
#define AT91C_TC_ACPA         (0x3 << 16) // (TC) RA Compare Effect on TIOA
#define 	AT91C_TC_ACPA_NONE                 (0x0 << 16) // (TC) Effect: none
#define 	AT91C_TC_ACPA_SET                  (0x1 << 16) // (TC) Effect: set
#define 	AT91C_TC_ACPA_CLEAR                (0x2 << 16) // (TC) Effect: clear
#define 	AT91C_TC_ACPA_TOGGLE               (0x3 << 16) // (TC) Effect: toggle
#define AT91C_TC_LDRA         (0x3 << 16) // (TC) RA Loading Selection
#define 	AT91C_TC_LDRA_NONE                 (0x0 << 16) // (TC) Edge: None
#define 	AT91C_TC_LDRA_RISING               (0x1 << 16) // (TC) Edge: rising edge of TIOA
#define 	AT91C_TC_LDRA_FALLING              (0x2 << 16) // (TC) Edge: falling edge of TIOA
#define 	AT91C_TC_LDRA_BOTH                 (0x3 << 16) // (TC) Edge: each edge of TIOA
#define AT91C_TC_ACPC         (0x3 << 18) // (TC) RC Compare Effect on TIOA
#define 	AT91C_TC_ACPC_NONE                 (0x0 << 18) // (TC) Effect: none
#define 	AT91C_TC_ACPC_SET                  (0x1 << 18) // (TC) Effect: set
#define 	AT91C_TC_ACPC_CLEAR                (0x2 << 18) // (TC) Effect: clear
#define 	AT91C_TC_ACPC_TOGGLE               (0x3 << 18) // (TC) Effect: toggle
#define AT91C_TC_LDRB         (0x3 << 18) // (TC) RB Loading Selection
#define 	AT91C_TC_LDRB_NONE                 (0x0 << 18) // (TC) Edge: None
#define 	AT91C_TC_LDRB_RISING               (0x1 << 18) // (TC) Edge: rising edge of TIOA
#define 	AT91C_TC_LDRB_FALLING              (0x2 << 18) // (TC) Edge: falling edge of TIOA
#define 	AT91C_TC_LDRB_BOTH                 (0x3 << 18) // (TC) Edge: each edge of TIOA
#define AT91C_TC_AEEVT        (0x3 << 20) // (TC) External Event Effect on TIOA
#define 	AT91C_TC_AEEVT_NONE                 (0x0 << 20) // (TC) Effect: none
#define 	AT91C_TC_AEEVT_SET                  (0x1 << 20) // (TC) Effect: set
#define 	AT91C_TC_AEEVT_CLEAR                (0x2 << 20) // (TC) Effect: clear
#define 	AT91C_TC_AEEVT_TOGGLE               (0x3 << 20) // (TC) Effect: toggle
#define AT91C_TC_ASWTRG       (0x3 << 22) // (TC) Software Trigger Effect on TIOA
#define 	AT91C_TC_ASWTRG_NONE                 (0x0 << 22) // (TC) Effect: none
#define 	AT91C_TC_ASWTRG_SET                  (0x1 << 22) // (TC) Effect: set
#define 	AT91C_TC_ASWTRG_CLEAR                (0x2 << 22) // (TC) Effect: clear
#define 	AT91C_TC_ASWTRG_TOGGLE               (0x3 << 22) // (TC) Effect: toggle
#define AT91C_TC_BCPB         (0x3 << 24) // (TC) RB Compare Effect on TIOB
#define 	AT91C_TC_BCPB_NONE                 (0x0 << 24) // (TC) Effect: none
#define 	AT91C_TC_BCPB_SET                  (0x1 << 24) // (TC) Effect: set
#define 	AT91C_TC_BCPB_CLEAR                (0x2 << 24) // (TC) Effect: clear
#define 	AT91C_TC_BCPB_TOGGLE               (0x3 << 24) // (TC) Effect: toggle
#define AT91C_TC_BCPC         (0x3 << 26) // (TC) RC Compare Effect on TIOB
#define 	AT91C_TC_BCPC_NONE                 (0x0 << 26) // (TC) Effect: none
#define 	AT91C_TC_BCPC_SET                  (0x1 << 26) // (TC) Effect: set
#define 	AT91C_TC_BCPC_CLEAR                (0x2 << 26) // (TC) Effect: clear
#define 	AT91C_TC_BCPC_TOGGLE               (0x3 << 26) // (TC) Effect: toggle
#define AT91C_TC_BEEVT        (0x3 << 28) // (TC) External Event Effect on TIOB
#define 	AT91C_TC_BEEVT_NONE                 (0x0 << 28) // (TC) Effect: none
#define 	AT91C_TC_BEEVT_SET                  (0x1 << 28) // (TC) Effect: set
#define 	AT91C_TC_BEEVT_CLEAR                (0x2 << 28) // (TC) Effect: clear
#define 	AT91C_TC_BEEVT_TOGGLE               (0x3 << 28) // (TC) Effect: toggle
#define AT91C_TC_BSWTRG       (0x3 << 30) // (TC) Software Trigger Effect on TIOB
#define 	AT91C_TC_BSWTRG_NONE                 (0x0 << 30) // (TC) Effect: none
#define 	AT91C_TC_BSWTRG_SET                  (0x1 << 30) // (TC) Effect: set
#define 	AT91C_TC_BSWTRG_CLEAR                (0x2 << 30) // (TC) Effect: clear
#define 	AT91C_TC_BSWTRG_TOGGLE               (0x3 << 30) // (TC) Effect: toggle
// -------- TC_SR : (TC Offset: 0x20) TC Channel Status Register --------
#define AT91C_TC_COVFS        (0x1 <<  0) // (TC) Counter Overflow
#define AT91C_TC_LOVRS        (0x1 <<  1) // (TC) Load Overrun
#define AT91C_TC_CPAS         (0x1 <<  2) // (TC) RA Compare
#define AT91C_TC_CPBS         (0x1 <<  3) // (TC) RB Compare
#define AT91C_TC_CPCS         (0x1 <<  4) // (TC) RC Compare
#define AT91C_TC_LDRAS        (0x1 <<  5) // (TC) RA Loading
#define AT91C_TC_LDRBS        (0x1 <<  6) // (TC) RB Loading
#define AT91C_TC_ETRGS        (0x1 <<  7) // (TC) External Trigger
#define AT91C_TC_CLKSTA       (0x1 << 16) // (TC) Clock Enabling
#define AT91C_TC_MTIOA        (0x1 << 17) // (TC) TIOA Mirror
#define AT91C_TC_MTIOB        (0x1 << 18) // (TC) TIOA Mirror
// -------- TC_IER : (TC Offset: 0x24) TC Channel Interrupt Enable Register --------
// -------- TC_IDR : (TC Offset: 0x28) TC Channel Interrupt Disable Register --------
// -------- TC_IMR : (TC Offset: 0x2c) TC Channel Interrupt Mask Register --------

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Timer Counter Interface
// *****************************************************************************
#ifndef __ASSEMBLY__
typedef struct _AT91S_TCB {
	AT91S_TC	 TCB_TC0; 	// TC Channel 0
	AT91_REG	 Reserved0[4]; 	//
	AT91S_TC	 TCB_TC1; 	// TC Channel 1
	AT91_REG	 Reserved1[4]; 	//
	AT91S_TC	 TCB_TC2; 	// TC Channel 2
	AT91_REG	 Reserved2[4]; 	//
	AT91_REG	 TCB_BCR; 	// TC Block Control Register
	AT91_REG	 TCB_BMR; 	// TC Block Mode Register
} AT91S_TCB, *AT91PS_TCB;
#else
#define TCB_BCR         (AT91_CAST(AT91_REG *) 	0x000000C0) // (TCB_BCR) TC Block Control Register
#define TCB_BMR         (AT91_CAST(AT91_REG *) 	0x000000C4) // (TCB_BMR) TC Block Mode Register

#endif
// -------- TCB_BCR : (TCB Offset: 0xc0) TC Block Control Register --------
#define AT91C_TCB_SYNC        (0x1 <<  0) // (TCB) Synchro Command
// -------- TCB_BMR : (TCB Offset: 0xc4) TC Block Mode Register --------
#define AT91C_TCB_TC0XC0S     (0x3 <<  0) // (TCB) External Clock Signal 0 Selection
#define 	AT91C_TCB_TC0XC0S_TCLK0                (0x0) // (TCB) TCLK0 connected to XC0
#define 	AT91C_TCB_TC0XC0S_NONE                 (0x1) // (TCB) None signal connected to XC0
#define 	AT91C_TCB_TC0XC0S_TIOA1                (0x2) // (TCB) TIOA1 connected to XC0
#define 	AT91C_TCB_TC0XC0S_TIOA2                (0x3) // (TCB) TIOA2 connected to XC0
#define AT91C_TCB_TC1XC1S     (0x3 <<  2) // (TCB) External Clock Signal 1 Selection
#define 	AT91C_TCB_TC1XC1S_TCLK1                (0x0 <<  2) // (TCB) TCLK1 connected to XC1
#define 	AT91C_TCB_TC1XC1S_NONE                 (0x1 <<  2) // (TCB) None signal connected to XC1
#define 	AT91C_TCB_TC1XC1S_TIOA0                (0x2 <<  2) // (TCB) TIOA0 connected to XC1
#define 	AT91C_TCB_TC1XC1S_TIOA2                (0x3 <<  2) // (TCB) TIOA2 connected to XC1
#define AT91C_TCB_TC2XC2S     (0x3 <<  4) // (TCB) External Clock Signal 2 Selection
#define 	AT91C_TCB_TC2XC2S_TCLK2                (0x0 <<  4) // (TCB) TCLK2 connected to XC2
#define 	AT91C_TCB_TC2XC2S_NONE                 (0x1 <<  4) // (TCB) None signal connected to XC2
#define 	AT91C_TCB_TC2XC2S_TIOA0                (0x2 <<  4) // (TCB) TIOA0 connected to XC2
#define 	AT91C_TCB_TC2XC2S_TIOA1                (0x3 <<  4) // (TCB) TIOA2 connected to XC2

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Multimedia Card Interface
// *****************************************************************************
#ifndef __ASSEMBLY__
typedef struct _AT91S_MCI {
	AT91_REG	 MCI_CR; 	// MCI Control Register
	AT91_REG	 MCI_MR; 	// MCI Mode Register
	AT91_REG	 MCI_DTOR; 	// MCI Data Timeout Register
	AT91_REG	 MCI_SDCR; 	// MCI SD Card Register
	AT91_REG	 MCI_ARGR; 	// MCI Argument Register
	AT91_REG	 MCI_CMDR; 	// MCI Command Register
	AT91_REG	 Reserved0[2]; 	//
	AT91_REG	 MCI_RSPR[4]; 	// MCI Response Register
	AT91_REG	 MCI_RDR; 	// MCI Receive Data Register
	AT91_REG	 MCI_TDR; 	// MCI Transmit Data Register
	AT91_REG	 Reserved1[2]; 	//
	AT91_REG	 MCI_SR; 	// MCI Status Register
	AT91_REG	 MCI_IER; 	// MCI Interrupt Enable Register
	AT91_REG	 MCI_IDR; 	// MCI Interrupt Disable Register
	AT91_REG	 MCI_IMR; 	// MCI Interrupt Mask Register
	AT91_REG	 Reserved2[44]; 	//
	AT91_REG	 MCI_RPR; 	// Receive Pointer Register
	AT91_REG	 MCI_RCR; 	// Receive Counter Register
	AT91_REG	 MCI_TPR; 	// Transmit Pointer Register
	AT91_REG	 MCI_TCR; 	// Transmit Counter Register
	AT91_REG	 MCI_RNPR; 	// Receive Next Pointer Register
	AT91_REG	 MCI_RNCR; 	// Receive Next Counter Register
	AT91_REG	 MCI_TNPR; 	// Transmit Next Pointer Register
	AT91_REG	 MCI_TNCR; 	// Transmit Next Counter Register
	AT91_REG	 MCI_PTCR; 	// PDC Transfer Control Register
	AT91_REG	 MCI_PTSR; 	// PDC Transfer Status Register
} AT91S_MCI, *AT91PS_MCI;
#else
#define MCI_CR          (AT91_CAST(AT91_REG *) 	0x00000000) // (MCI_CR) MCI Control Register
#define MCI_MR          (AT91_CAST(AT91_REG *) 	0x00000004) // (MCI_MR) MCI Mode Register
#define MCI_DTOR        (AT91_CAST(AT91_REG *) 	0x00000008) // (MCI_DTOR) MCI Data Timeout Register
#define MCI_SDCR        (AT91_CAST(AT91_REG *) 	0x0000000C) // (MCI_SDCR) MCI SD Card Register
#define MCI_ARGR        (AT91_CAST(AT91_REG *) 	0x00000010) // (MCI_ARGR) MCI Argument Register
#define MCI_CMDR        (AT91_CAST(AT91_REG *) 	0x00000014) // (MCI_CMDR) MCI Command Register
#define MCI_RSPR        (AT91_CAST(AT91_REG *) 	0x00000020) // (MCI_RSPR) MCI Response Register
#define MCI_RDR         (AT91_CAST(AT91_REG *) 	0x00000030) // (MCI_RDR) MCI Receive Data Register
#define MCI_TDR         (AT91_CAST(AT91_REG *) 	0x00000034) // (MCI_TDR) MCI Transmit Data Register
#define MCI_SR          (AT91_CAST(AT91_REG *) 	0x00000040) // (MCI_SR) MCI Status Register
#define MCI_IER         (AT91_CAST(AT91_REG *) 	0x00000044) // (MCI_IER) MCI Interrupt Enable Register
#define MCI_IDR         (AT91_CAST(AT91_REG *) 	0x00000048) // (MCI_IDR) MCI Interrupt Disable Register
#define MCI_IMR         (AT91_CAST(AT91_REG *) 	0x0000004C) // (MCI_IMR) MCI Interrupt Mask Register

#endif
// -------- MCI_CR : (MCI Offset: 0x0) MCI Control Register --------
#define AT91C_MCI_MCIEN       (0x1 <<  0) // (MCI) Multimedia Interface Enable
#define AT91C_MCI_MCIDIS      (0x1 <<  1) // (MCI) Multimedia Interface Disable
#define AT91C_MCI_PWSEN       (0x1 <<  2) // (MCI) Power Save Mode Enable
#define AT91C_MCI_PWSDIS      (0x1 <<  3) // (MCI) Power Save Mode Disable
#define AT91C_MCI_SWRST       (0x1 <<  7) // (MCI) MCI Software reset
// -------- MCI_MR : (MCI Offset: 0x4) MCI Mode Register --------
#define AT91C_MCI_CLKDIV      (0xFF <<  0) // (MCI) Clock Divider
#define AT91C_MCI_PWSDIV      (0x7 <<  8) // (MCI) Power Saving Divider
#define AT91C_MCI_PDCPADV     (0x1 << 14) // (MCI) PDC Padding Value
#define AT91C_MCI_PDCMODE     (0x1 << 15) // (MCI) PDC Oriented Mode
#define AT91C_MCI_BLKLEN      (0xFFF << 18) // (MCI) Data Block Length
// -------- MCI_DTOR : (MCI Offset: 0x8) MCI Data Timeout Register --------
#define AT91C_MCI_DTOCYC      (0xF <<  0) // (MCI) Data Timeout Cycle Number
#define AT91C_MCI_DTOMUL      (0x7 <<  4) // (MCI) Data Timeout Multiplier
#define 	AT91C_MCI_DTOMUL_1                    (0x0 <<  4) // (MCI) DTOCYC x 1
#define 	AT91C_MCI_DTOMUL_16                   (0x1 <<  4) // (MCI) DTOCYC x 16
#define 	AT91C_MCI_DTOMUL_128                  (0x2 <<  4) // (MCI) DTOCYC x 128
#define 	AT91C_MCI_DTOMUL_256                  (0x3 <<  4) // (MCI) DTOCYC x 256
#define 	AT91C_MCI_DTOMUL_1024                 (0x4 <<  4) // (MCI) DTOCYC x 1024
#define 	AT91C_MCI_DTOMUL_4096                 (0x5 <<  4) // (MCI) DTOCYC x 4096
#define 	AT91C_MCI_DTOMUL_65536                (0x6 <<  4) // (MCI) DTOCYC x 65536
#define 	AT91C_MCI_DTOMUL_1048576              (0x7 <<  4) // (MCI) DTOCYC x 1048576
// -------- MCI_SDCR : (MCI Offset: 0xc) MCI SD Card Register --------
#define AT91C_MCI_SCDSEL      (0xF <<  0) // (MCI) SD Card Selector
#define AT91C_MCI_SCDBUS      (0x1 <<  7) // (MCI) SD Card Bus Width
// -------- MCI_CMDR : (MCI Offset: 0x14) MCI Command Register --------
#define AT91C_MCI_CMDNB       (0x3F <<  0) // (MCI) Command Number
#define AT91C_MCI_RSPTYP      (0x3 <<  6) // (MCI) Response Type
#define 	AT91C_MCI_RSPTYP_NO                   (0x0 <<  6) // (MCI) No response
#define 	AT91C_MCI_RSPTYP_48                   (0x1 <<  6) // (MCI) 48-bit response
#define 	AT91C_MCI_RSPTYP_136                  (0x2 <<  6) // (MCI) 136-bit response
#define AT91C_MCI_SPCMD       (0x7 <<  8) // (MCI) Special CMD
#define 	AT91C_MCI_SPCMD_NONE                 (0x0 <<  8) // (MCI) Not a special CMD
#define 	AT91C_MCI_SPCMD_INIT                 (0x1 <<  8) // (MCI) Initialization CMD
#define 	AT91C_MCI_SPCMD_SYNC                 (0x2 <<  8) // (MCI) Synchronized CMD
#define 	AT91C_MCI_SPCMD_IT_CMD               (0x4 <<  8) // (MCI) Interrupt command
#define 	AT91C_MCI_SPCMD_IT_REP               (0x5 <<  8) // (MCI) Interrupt response
#define AT91C_MCI_OPDCMD      (0x1 << 11) // (MCI) Open Drain Command
#define AT91C_MCI_MAXLAT      (0x1 << 12) // (MCI) Maximum Latency for Command to respond
#define AT91C_MCI_TRCMD       (0x3 << 16) // (MCI) Transfer CMD
#define 	AT91C_MCI_TRCMD_NO                   (0x0 << 16) // (MCI) No transfer
#define 	AT91C_MCI_TRCMD_START                (0x1 << 16) // (MCI) Start transfer
#define 	AT91C_MCI_TRCMD_STOP                 (0x2 << 16) // (MCI) Stop transfer
#define AT91C_MCI_TRDIR       (0x1 << 18) // (MCI) Transfer Direction
#define AT91C_MCI_TRTYP       (0x3 << 19) // (MCI) Transfer Type
#define 	AT91C_MCI_TRTYP_BLOCK                (0x0 << 19) // (MCI) Block Transfer type
#define 	AT91C_MCI_TRTYP_MULTIPLE             (0x1 << 19) // (MCI) Multiple Block transfer type
#define 	AT91C_MCI_TRTYP_STREAM               (0x2 << 19) // (MCI) Stream transfer type
// -------- MCI_SR : (MCI Offset: 0x40) MCI Status Register --------
#define AT91C_MCI_CMDRDY      (0x1 <<  0) // (MCI) Command Ready flag
#define AT91C_MCI_RXRDY       (0x1 <<  1) // (MCI) RX Ready flag
#define AT91C_MCI_TXRDY       (0x1 <<  2) // (MCI) TX Ready flag
#define AT91C_MCI_BLKE        (0x1 <<  3) // (MCI) Data Block Transfer Ended flag
#define AT91C_MCI_DTIP        (0x1 <<  4) // (MCI) Data Transfer in Progress flag
#define AT91C_MCI_NOTBUSY     (0x1 <<  5) // (MCI) Data Line Not Busy flag
#define AT91C_MCI_ENDRX       (0x1 <<  6) // (MCI) End of RX Buffer flag
#define AT91C_MCI_ENDTX       (0x1 <<  7) // (MCI) End of TX Buffer flag
#define AT91C_MCI_RXBUFF      (0x1 << 14) // (MCI) RX Buffer Full flag
#define AT91C_MCI_TXBUFE      (0x1 << 15) // (MCI) TX Buffer Empty flag
#define AT91C_MCI_RINDE       (0x1 << 16) // (MCI) Response Index Error flag
#define AT91C_MCI_RDIRE       (0x1 << 17) // (MCI) Response Direction Error flag
#define AT91C_MCI_RCRCE       (0x1 << 18) // (MCI) Response CRC Error flag
#define AT91C_MCI_RENDE       (0x1 << 19) // (MCI) Response End Bit Error flag
#define AT91C_MCI_RTOE        (0x1 << 20) // (MCI) Response Time-out Error flag
#define AT91C_MCI_DCRCE       (0x1 << 21) // (MCI) data CRC Error flag
#define AT91C_MCI_DTOE        (0x1 << 22) // (MCI) Data timeout Error flag
#define AT91C_MCI_OVRE        (0x1 << 30) // (MCI) Overrun flag
#define AT91C_MCI_UNRE        (0x1 << 31) // (MCI) Underrun flag
// -------- MCI_IER : (MCI Offset: 0x44) MCI Interrupt Enable Register --------
// -------- MCI_IDR : (MCI Offset: 0x48) MCI Interrupt Disable Register --------
// -------- MCI_IMR : (MCI Offset: 0x4c) MCI Interrupt Mask Register --------

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR USB Device Interface
// *****************************************************************************
#ifndef __ASSEMBLY__
typedef struct _AT91S_UDP {
	AT91_REG	 UDP_NUM; 	// Frame Number Register
	AT91_REG	 UDP_GLBSTATE; 	// Global State Register
	AT91_REG	 UDP_FADDR; 	// Function Address Register
	AT91_REG	 Reserved0[1]; 	//
	AT91_REG	 UDP_IER; 	// Interrupt Enable Register
	AT91_REG	 UDP_IDR; 	// Interrupt Disable Register
	AT91_REG	 UDP_IMR; 	// Interrupt Mask Register
	AT91_REG	 UDP_ISR; 	// Interrupt Status Register
	AT91_REG	 UDP_ICR; 	// Interrupt Clear Register
	AT91_REG	 Reserved1[1]; 	//
	AT91_REG	 UDP_RSTEP; 	// Reset Endpoint Register
	AT91_REG	 Reserved2[1]; 	//
	AT91_REG	 UDP_CSR[6]; 	// Endpoint Control and Status Register
	AT91_REG	 Reserved3[2]; 	//
	AT91_REG	 UDP_FDR[6]; 	// Endpoint FIFO Data Register
	AT91_REG	 Reserved4[3]; 	//
	AT91_REG	 UDP_TXVC; 	// Transceiver Control Register
} AT91S_UDP, *AT91PS_UDP;
#else
#define UDP_FRM_NUM     (AT91_CAST(AT91_REG *) 	0x00000000) // (UDP_FRM_NUM) Frame Number Register
#define UDP_GLBSTATE    (AT91_CAST(AT91_REG *) 	0x00000004) // (UDP_GLBSTATE) Global State Register
#define UDP_FADDR       (AT91_CAST(AT91_REG *) 	0x00000008) // (UDP_FADDR) Function Address Register
#define UDP_IER         (AT91_CAST(AT91_REG *) 	0x00000010) // (UDP_IER) Interrupt Enable Register
#define UDP_IDR         (AT91_CAST(AT91_REG *) 	0x00000014) // (UDP_IDR) Interrupt Disable Register
#define UDP_IMR         (AT91_CAST(AT91_REG *) 	0x00000018) // (UDP_IMR) Interrupt Mask Register
#define UDP_ISR         (AT91_CAST(AT91_REG *) 	0x0000001C) // (UDP_ISR) Interrupt Status Register
#define UDP_ICR         (AT91_CAST(AT91_REG *) 	0x00000020) // (UDP_ICR) Interrupt Clear Register
#define UDP_RSTEP       (AT91_CAST(AT91_REG *) 	0x00000028) // (UDP_RSTEP) Reset Endpoint Register
#define UDP_CSR         (AT91_CAST(AT91_REG *) 	0x00000030) // (UDP_CSR) Endpoint Control and Status Register
#define UDP_FDR         (AT91_CAST(AT91_REG *) 	0x00000050) // (UDP_FDR) Endpoint FIFO Data Register
#define UDP_TXVC        (AT91_CAST(AT91_REG *) 	0x00000074) // (UDP_TXVC) Transceiver Control Register

#endif
// -------- UDP_FRM_NUM : (UDP Offset: 0x0) USB Frame Number Register --------
#define AT91C_UDP_FRM_NUM     (0x7FF <<  0) // (UDP) Frame Number as Defined in the Packet Field Formats
#define AT91C_UDP_FRM_ERR     (0x1 << 16) // (UDP) Frame Error
#define AT91C_UDP_FRM_OK      (0x1 << 17) // (UDP) Frame OK
// -------- UDP_GLB_STATE : (UDP Offset: 0x4) USB Global State Register --------
#define AT91C_UDP_FADDEN      (0x1 <<  0) // (UDP) Function Address Enable
#define AT91C_UDP_CONFG       (0x1 <<  1) // (UDP) Configured
#define AT91C_UDP_ESR         (0x1 <<  2) // (UDP) Enable Send Resume
#define AT91C_UDP_RSMINPR     (0x1 <<  3) // (UDP) A Resume Has Been Sent to the Host
#define AT91C_UDP_RMWUPE      (0x1 <<  4) // (UDP) Remote Wake Up Enable
// -------- UDP_FADDR : (UDP Offset: 0x8) USB Function Address Register --------
#define AT91C_UDP_FADD        (0xFF <<  0) // (UDP) Function Address Value
#define AT91C_UDP_FEN         (0x1 <<  8) // (UDP) Function Enable
// -------- UDP_IER : (UDP Offset: 0x10) USB Interrupt Enable Register --------
#define AT91C_UDP_EPINT0      (0x1 <<  0) // (UDP) Endpoint 0 Interrupt
#define AT91C_UDP_EPINT1      (0x1 <<  1) // (UDP) Endpoint 0 Interrupt
#define AT91C_UDP_EPINT2      (0x1 <<  2) // (UDP) Endpoint 2 Interrupt
#define AT91C_UDP_EPINT3      (0x1 <<  3) // (UDP) Endpoint 3 Interrupt
#define AT91C_UDP_EPINT4      (0x1 <<  4) // (UDP) Endpoint 4 Interrupt
#define AT91C_UDP_EPINT5      (0x1 <<  5) // (UDP) Endpoint 5 Interrupt
#define AT91C_UDP_RXSUSP      (0x1 <<  8) // (UDP) USB Suspend Interrupt
#define AT91C_UDP_RXRSM       (0x1 <<  9) // (UDP) USB Resume Interrupt
#define AT91C_UDP_EXTRSM      (0x1 << 10) // (UDP) USB External Resume Interrupt
#define AT91C_UDP_SOFINT      (0x1 << 11) // (UDP) USB Start Of frame Interrupt
#define AT91C_UDP_WAKEUP      (0x1 << 13) // (UDP) USB Resume Interrupt
// -------- UDP_IDR : (UDP Offset: 0x14) USB Interrupt Disable Register --------
// -------- UDP_IMR : (UDP Offset: 0x18) USB Interrupt Mask Register --------
// -------- UDP_ISR : (UDP Offset: 0x1c) USB Interrupt Status Register --------
#define AT91C_UDP_ENDBUSRES   (0x1 << 12) // (UDP) USB End Of Bus Reset Interrupt
// -------- UDP_ICR : (UDP Offset: 0x20) USB Interrupt Clear Register --------
// -------- UDP_RST_EP : (UDP Offset: 0x28) USB Reset Endpoint Register --------
#define AT91C_UDP_EP0         (0x1 <<  0) // (UDP) Reset Endpoint 0
#define AT91C_UDP_EP1         (0x1 <<  1) // (UDP) Reset Endpoint 1
#define AT91C_UDP_EP2         (0x1 <<  2) // (UDP) Reset Endpoint 2
#define AT91C_UDP_EP3         (0x1 <<  3) // (UDP) Reset Endpoint 3
#define AT91C_UDP_EP4         (0x1 <<  4) // (UDP) Reset Endpoint 4
#define AT91C_UDP_EP5         (0x1 <<  5) // (UDP) Reset Endpoint 5
// -------- UDP_CSR : (UDP Offset: 0x30) USB Endpoint Control and Status Register --------
#define AT91C_UDP_TXCOMP      (0x1 <<  0) // (UDP) Generates an IN packet with data previously written in the DPR
#define AT91C_UDP_RX_DATA_BK0 (0x1 <<  1) // (UDP) Receive Data Bank 0
#define AT91C_UDP_RXSETUP     (0x1 <<  2) // (UDP) Sends STALL to the Host (Control endpoints)
#define AT91C_UDP_ISOERROR    (0x1 <<  3) // (UDP) Isochronous error (Isochronous endpoints)
#define AT91C_UDP_STALLSENT   (0x1 <<  3) // (UDP) Stall sent (Control, bulk, interrupt endpoints)
#define AT91C_UDP_TXPKTRDY    (0x1 <<  4) // (UDP) Transmit Packet Ready
#define AT91C_UDP_FORCESTALL  (0x1 <<  5) // (UDP) Force Stall (used by Control, Bulk and Isochronous endpoints).
#define AT91C_UDP_RX_DATA_BK1 (0x1 <<  6) // (UDP) Receive Data Bank 1 (only used by endpoints with ping-pong attributes).
#define AT91C_UDP_DIR         (0x1 <<  7) // (UDP) Transfer Direction
#define AT91C_UDP_EPTYPE      (0x7 <<  8) // (UDP) Endpoint type
#define 	AT91C_UDP_EPTYPE_CTRL                 (0x0 <<  8) // (UDP) Control
#define 	AT91C_UDP_EPTYPE_ISO_OUT              (0x1 <<  8) // (UDP) Isochronous OUT
#define 	AT91C_UDP_EPTYPE_BULK_OUT             (0x2 <<  8) // (UDP) Bulk OUT
#define 	AT91C_UDP_EPTYPE_INT_OUT              (0x3 <<  8) // (UDP) Interrupt OUT
#define 	AT91C_UDP_EPTYPE_ISO_IN               (0x5 <<  8) // (UDP) Isochronous IN
#define 	AT91C_UDP_EPTYPE_BULK_IN              (0x6 <<  8) // (UDP) Bulk IN
#define 	AT91C_UDP_EPTYPE_INT_IN               (0x7 <<  8) // (UDP) Interrupt IN
#define AT91C_UDP_DTGLE       (0x1 << 11) // (UDP) Data Toggle
#define AT91C_UDP_EPEDS       (0x1 << 15) // (UDP) Endpoint Enable Disable
#define AT91C_UDP_RXBYTECNT   (0x7FF << 16) // (UDP) Number Of Bytes Available in the FIFO
// -------- UDP_TXVC : (UDP Offset: 0x74) Transceiver Control Register --------
#define AT91C_UDP_TXVDIS      (0x1 <<  8) // (UDP)

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Two-wire Interface
// *****************************************************************************
#ifndef __ASSEMBLY__
typedef struct _AT91S_TWI {
	AT91_REG	 TWI_CR; 	// Control Register
	AT91_REG	 TWI_MMR; 	// Master Mode Register
	AT91_REG	 Reserved0[1]; 	//
	AT91_REG	 TWI_IADR; 	// Internal Address Register
	AT91_REG	 TWI_CWGR; 	// Clock Waveform Generator Register
	AT91_REG	 Reserved1[3]; 	//
	AT91_REG	 TWI_SR; 	// Status Register
	AT91_REG	 TWI_IER; 	// Interrupt Enable Register
	AT91_REG	 TWI_IDR; 	// Interrupt Disable Register
	AT91_REG	 TWI_IMR; 	// Interrupt Mask Register
	AT91_REG	 TWI_RHR; 	// Receive Holding Register
	AT91_REG	 TWI_THR; 	// Transmit Holding Register
	AT91_REG	 Reserved2[50]; 	//
	AT91_REG	 TWI_RPR; 	// Receive Pointer Register
	AT91_REG	 TWI_RCR; 	// Receive Counter Register
	AT91_REG	 TWI_TPR; 	// Transmit Pointer Register
	AT91_REG	 TWI_TCR; 	// Transmit Counter Register
	AT91_REG	 TWI_RNPR; 	// Receive Next Pointer Register
	AT91_REG	 TWI_RNCR; 	// Receive Next Counter Register
	AT91_REG	 TWI_TNPR; 	// Transmit Next Pointer Register
	AT91_REG	 TWI_TNCR; 	// Transmit Next Counter Register
	AT91_REG	 TWI_PTCR; 	// PDC Transfer Control Register
	AT91_REG	 TWI_PTSR; 	// PDC Transfer Status Register
} AT91S_TWI, *AT91PS_TWI;
#else
#define TWI_CR          (AT91_CAST(AT91_REG *) 	0x00000000) // (TWI_CR) Control Register
#define TWI_MMR         (AT91_CAST(AT91_REG *) 	0x00000004) // (TWI_MMR) Master Mode Register
#define TWI_IADR        (AT91_CAST(AT91_REG *) 	0x0000000C) // (TWI_IADR) Internal Address Register
#define TWI_CWGR        (AT91_CAST(AT91_REG *) 	0x00000010) // (TWI_CWGR) Clock Waveform Generator Register
#define TWI_SR          (AT91_CAST(AT91_REG *) 	0x00000020) // (TWI_SR) Status Register
#define TWI_IER         (AT91_CAST(AT91_REG *) 	0x00000024) // (TWI_IER) Interrupt Enable Register
#define TWI_IDR         (AT91_CAST(AT91_REG *) 	0x00000028) // (TWI_IDR) Interrupt Disable Register
#define TWI_IMR         (AT91_CAST(AT91_REG *) 	0x0000002C) // (TWI_IMR) Interrupt Mask Register
#define TWI_RHR         (AT91_CAST(AT91_REG *) 	0x00000030) // (TWI_RHR) Receive Holding Register
#define TWI_THR         (AT91_CAST(AT91_REG *) 	0x00000034) // (TWI_THR) Transmit Holding Register

#endif
// -------- TWI_CR : (TWI Offset: 0x0) TWI Control Register --------
#define AT91C_TWI_START       (0x1 <<  0) // (TWI) Send a START Condition
#define AT91C_TWI_STOP        (0x1 <<  1) // (TWI) Send a STOP Condition
#define AT91C_TWI_MSEN        (0x1 <<  2) // (TWI) TWI Master Transfer Enabled
#define AT91C_TWI_MSDIS       (0x1 <<  3) // (TWI) TWI Master Transfer Disabled
#define AT91C_TWI_SWRST       (0x1 <<  7) // (TWI) Software Reset
// -------- TWI_MMR : (TWI Offset: 0x4) TWI Master Mode Register --------
#define AT91C_TWI_IADRSZ      (0x3 <<  8) // (TWI) Internal Device Address Size
#define 	AT91C_TWI_IADRSZ_NO                   (0x0 <<  8) // (TWI) No internal device address
#define 	AT91C_TWI_IADRSZ_1_BYTE               (0x1 <<  8) // (TWI) One-byte internal device address
#define 	AT91C_TWI_IADRSZ_2_BYTE               (0x2 <<  8) // (TWI) Two-byte internal device address
#define 	AT91C_TWI_IADRSZ_3_BYTE               (0x3 <<  8) // (TWI) Three-byte internal device address
#define AT91C_TWI_MREAD       (0x1 << 12) // (TWI) Master Read Direction
#define AT91C_TWI_DADR        (0x7F << 16) // (TWI) Device Address
// -------- TWI_CWGR : (TWI Offset: 0x10) TWI Clock Waveform Generator Register --------
#define AT91C_TWI_CLDIV       (0xFF <<  0) // (TWI) Clock Low Divider
#define AT91C_TWI_CHDIV       (0xFF <<  8) // (TWI) Clock High Divider
#define AT91C_TWI_CKDIV       (0x7 << 16) // (TWI) Clock Divider
// -------- TWI_SR : (TWI Offset: 0x20) TWI Status Register --------
#define AT91C_TWI_TXCOMP      (0x1 <<  0) // (TWI) Transmission Completed
#define AT91C_TWI_RXRDY       (0x1 <<  1) // (TWI) Receive holding register ReaDY
#define AT91C_TWI_TXRDY       (0x1 <<  2) // (TWI) Transmit holding register ReaDY
#define AT91C_TWI_OVRE        (0x1 <<  6) // (TWI) Overrun Error
#define AT91C_TWI_UNRE        (0x1 <<  7) // (TWI) Underrun Error
#define AT91C_TWI_NACK        (0x1 <<  8) // (TWI) Not Acknowledged
#define AT91C_TWI_ENDRX       (0x1 << 12) // (TWI)
#define AT91C_TWI_ENDTX       (0x1 << 13) // (TWI)
#define AT91C_TWI_RXBUFF      (0x1 << 14) // (TWI)
#define AT91C_TWI_TXBUFE      (0x1 << 15) // (TWI)
// -------- TWI_IER : (TWI Offset: 0x24) TWI Interrupt Enable Register --------
// -------- TWI_IDR : (TWI Offset: 0x28) TWI Interrupt Disable Register --------
// -------- TWI_IMR : (TWI Offset: 0x2c) TWI Interrupt Mask Register --------

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Usart
// *****************************************************************************
#ifndef __ASSEMBLY__
typedef struct _AT91S_USART {
	AT91_REG	 US_CR; 	// Control Register
	AT91_REG	 US_MR; 	// Mode Register
	AT91_REG	 US_IER; 	// Interrupt Enable Register
	AT91_REG	 US_IDR; 	// Interrupt Disable Register
	AT91_REG	 US_IMR; 	// Interrupt Mask Register
	AT91_REG	 US_CSR; 	// Channel Status Register
	AT91_REG	 US_RHR; 	// Receiver Holding Register
	AT91_REG	 US_THR; 	// Transmitter Holding Register
	AT91_REG	 US_BRGR; 	// Baud Rate Generator Register
	AT91_REG	 US_RTOR; 	// Receiver Time-out Register
	AT91_REG	 US_TTGR; 	// Transmitter Time-guard Register
	AT91_REG	 Reserved0[5]; 	//
	AT91_REG	 US_FIDI; 	// FI_DI_Ratio Register
	AT91_REG	 US_NER; 	// Nb Errors Register
	AT91_REG	 Reserved1[1]; 	//
	AT91_REG	 US_IF; 	// IRDA_FILTER Register
	AT91_REG	 Reserved2[44]; 	//
	AT91_REG	 US_RPR; 	// Receive Pointer Register
	AT91_REG	 US_RCR; 	// Receive Counter Register
	AT91_REG	 US_TPR; 	// Transmit Pointer Register
	AT91_REG	 US_TCR; 	// Transmit Counter Register
	AT91_REG	 US_RNPR; 	// Receive Next Pointer Register
	AT91_REG	 US_RNCR; 	// Receive Next Counter Register
	AT91_REG	 US_TNPR; 	// Transmit Next Pointer Register
	AT91_REG	 US_TNCR; 	// Transmit Next Counter Register
	AT91_REG	 US_PTCR; 	// PDC Transfer Control Register
	AT91_REG	 US_PTSR; 	// PDC Transfer Status Register
} AT91S_USART, *AT91PS_USART;
#else
#define US_CR           (AT91_CAST(AT91_REG *) 	0x00000000) // (US_CR) Control Register
#define US_MR           (AT91_CAST(AT91_REG *) 	0x00000004) // (US_MR) Mode Register
#define US_IER          (AT91_CAST(AT91_REG *) 	0x00000008) // (US_IER) Interrupt Enable Register
#define US_IDR          (AT91_CAST(AT91_REG *) 	0x0000000C) // (US_IDR) Interrupt Disable Register
#define US_IMR          (AT91_CAST(AT91_REG *) 	0x00000010) // (US_IMR) Interrupt Mask Register
#define US_CSR          (AT91_CAST(AT91_REG *) 	0x00000014) // (US_CSR) Channel Status Register
#define US_RHR          (AT91_CAST(AT91_REG *) 	0x00000018) // (US_RHR) Receiver Holding Register
#define US_THR          (AT91_CAST(AT91_REG *) 	0x0000001C) // (US_THR) Transmitter Holding Register
#define US_BRGR         (AT91_CAST(AT91_REG *) 	0x00000020) // (US_BRGR) Baud Rate Generator Register
#define US_RTOR         (AT91_CAST(AT91_REG *) 	0x00000024) // (US_RTOR) Receiver Time-out Register
#define US_TTGR         (AT91_CAST(AT91_REG *) 	0x00000028) // (US_TTGR) Transmitter Time-guard Register
#define US_FIDI         (AT91_CAST(AT91_REG *) 	0x00000040) // (US_FIDI) FI_DI_Ratio Register
#define US_NER          (AT91_CAST(AT91_REG *) 	0x00000044) // (US_NER) Nb Errors Register
#define US_IF           (AT91_CAST(AT91_REG *) 	0x0000004C) // (US_IF) IRDA_FILTER Register

#endif
// -------- US_CR : (USART Offset: 0x0) Debug Unit Control Register --------
#define AT91C_US_STTBRK       (0x1 <<  9) // (USART) Start Break
#define AT91C_US_STPBRK       (0x1 << 10) // (USART) Stop Break
#define AT91C_US_STTTO        (0x1 << 11) // (USART) Start Time-out
#define AT91C_US_SENDA        (0x1 << 12) // (USART) Send Address
#define AT91C_US_RSTIT        (0x1 << 13) // (USART) Reset Iterations
#define AT91C_US_RSTNACK      (0x1 << 14) // (USART) Reset Non Acknowledge
#define AT91C_US_RETTO        (0x1 << 15) // (USART) Rearm Time-out
#define AT91C_US_DTREN        (0x1 << 16) // (USART) Data Terminal ready Enable
#define AT91C_US_DTRDIS       (0x1 << 17) // (USART) Data Terminal ready Disable
#define AT91C_US_RTSEN        (0x1 << 18) // (USART) Request to Send enable
#define AT91C_US_RTSDIS       (0x1 << 19) // (USART) Request to Send Disable
// -------- US_MR : (USART Offset: 0x4) Debug Unit Mode Register --------
#define AT91C_US_USMODE       (0xF <<  0) // (USART) Usart mode
#define 	AT91C_US_USMODE_NORMAL               (0x0) // (USART) Normal
#define 	AT91C_US_USMODE_RS485                (0x1) // (USART) RS485
#define 	AT91C_US_USMODE_HWHSH                (0x2) // (USART) Hardware Handshaking
#define 	AT91C_US_USMODE_MODEM                (0x3) // (USART) Modem
#define 	AT91C_US_USMODE_ISO7816_0            (0x4) // (USART) ISO7816 protocol: T = 0
#define 	AT91C_US_USMODE_ISO7816_1            (0x6) // (USART) ISO7816 protocol: T = 1
#define 	AT91C_US_USMODE_IRDA                 (0x8) // (USART) IrDA
#define 	AT91C_US_USMODE_SWHSH                (0xC) // (USART) Software Handshaking
#define AT91C_US_CLKS         (0x3 <<  4) // (USART) Clock Selection (Baud Rate generator Input Clock
#define 	AT91C_US_CLKS_CLOCK                (0x0 <<  4) // (USART) Clock
#define 	AT91C_US_CLKS_FDIV1                (0x1 <<  4) // (USART) fdiv1
#define 	AT91C_US_CLKS_SLOW                 (0x2 <<  4) // (USART) slow_clock (ARM)
#define 	AT91C_US_CLKS_EXT                  (0x3 <<  4) // (USART) External (SCK)
#define AT91C_US_CHRL         (0x3 <<  6) // (USART) Clock Selection (Baud Rate generator Input Clock
#define 	AT91C_US_CHRL_5_BITS               (0x0 <<  6) // (USART) Character Length: 5 bits
#define 	AT91C_US_CHRL_6_BITS               (0x1 <<  6) // (USART) Character Length: 6 bits
#define 	AT91C_US_CHRL_7_BITS               (0x2 <<  6) // (USART) Character Length: 7 bits
#define 	AT91C_US_CHRL_8_BITS               (0x3 <<  6) // (USART) Character Length: 8 bits
#define AT91C_US_SYNC         (0x1 <<  8) // (USART) Synchronous Mode Select
#define AT91C_US_NBSTOP       (0x3 << 12) // (USART) Number of Stop bits
#define 	AT91C_US_NBSTOP_1_BIT                (0x0 << 12) // (USART) 1 stop bit
#define 	AT91C_US_NBSTOP_15_BIT               (0x1 << 12) // (USART) Asynchronous (SYNC=0) 2 stop bits Synchronous (SYNC=1) 2 stop bits
#define 	AT91C_US_NBSTOP_2_BIT                (0x2 << 12) // (USART) 2 stop bits
#define AT91C_US_MSBF         (0x1 << 16) // (USART) Bit Order
#define AT91C_US_MODE9        (0x1 << 17) // (USART) 9-bit Character length
#define AT91C_US_CKLO         (0x1 << 18) // (USART) Clock Output Select
#define AT91C_US_OVER         (0x1 << 19) // (USART) Over Sampling Mode
#define AT91C_US_INACK        (0x1 << 20) // (USART) Inhibit Non Acknowledge
#define AT91C_US_DSNACK       (0x1 << 21) // (USART) Disable Successive NACK
#define AT91C_US_MAX_ITER     (0x1 << 24) // (USART) Number of Repetitions
#define AT91C_US_FILTER       (0x1 << 28) // (USART) Receive Line Filter
// -------- US_IER : (USART Offset: 0x8) Debug Unit Interrupt Enable Register --------
#define AT91C_US_RXBRK        (0x1 <<  2) // (USART) Break Received/End of Break
#define AT91C_US_TIMEOUT      (0x1 <<  8) // (USART) Receiver Time-out
#define AT91C_US_ITERATION    (0x1 << 10) // (USART) Max number of Repetitions Reached
#define AT91C_US_NACK         (0x1 << 13) // (USART) Non Acknowledge
#define AT91C_US_RIIC         (0x1 << 16) // (USART) Ring INdicator Input Change Flag
#define AT91C_US_DSRIC        (0x1 << 17) // (USART) Data Set Ready Input Change Flag
#define AT91C_US_DCDIC        (0x1 << 18) // (USART) Data Carrier Flag
#define AT91C_US_CTSIC        (0x1 << 19) // (USART) Clear To Send Input Change Flag
// -------- US_IDR : (USART Offset: 0xc) Debug Unit Interrupt Disable Register --------
// -------- US_IMR : (USART Offset: 0x10) Debug Unit Interrupt Mask Register --------
// -------- US_CSR : (USART Offset: 0x14) Debug Unit Channel Status Register --------
#define AT91C_US_RI           (0x1 << 20) // (USART) Image of RI Input
#define AT91C_US_DSR          (0x1 << 21) // (USART) Image of DSR Input
#define AT91C_US_DCD          (0x1 << 22) // (USART) Image of DCD Input
#define AT91C_US_CTS          (0x1 << 23) // (USART) Image of CTS Input

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR PWMC Channel Interface
// *****************************************************************************
#ifndef __ASSEMBLY__
typedef struct _AT91S_PWMC_CH {
	AT91_REG	 PWMC_CMR; 	// Channel Mode Register
	AT91_REG	 PWMC_CDTYR; 	// Channel Duty Cycle Register
	AT91_REG	 PWMC_CPRDR; 	// Channel Period Register
	AT91_REG	 PWMC_CCNTR; 	// Channel Counter Register
	AT91_REG	 PWMC_CUPDR; 	// Channel Update Register
	AT91_REG	 PWMC_Reserved[3]; 	// Reserved
} AT91S_PWMC_CH, *AT91PS_PWMC_CH;
#else
#define PWMC_CMR        (AT91_CAST(AT91_REG *) 	0x00000000) // (PWMC_CMR) Channel Mode Register
#define PWMC_CDTYR      (AT91_CAST(AT91_REG *) 	0x00000004) // (PWMC_CDTYR) Channel Duty Cycle Register
#define PWMC_CPRDR      (AT91_CAST(AT91_REG *) 	0x00000008) // (PWMC_CPRDR) Channel Period Register
#define PWMC_CCNTR      (AT91_CAST(AT91_REG *) 	0x0000000C) // (PWMC_CCNTR) Channel Counter Register
#define PWMC_CUPDR      (AT91_CAST(AT91_REG *) 	0x00000010) // (PWMC_CUPDR) Channel Update Register
#define Reserved        (AT91_CAST(AT91_REG *) 	0x00000014) // (Reserved) Reserved

#endif
// -------- PWMC_CMR : (PWMC_CH Offset: 0x0) PWMC Channel Mode Register --------
#define AT91C_PWMC_CPRE       (0xF <<  0) // (PWMC_CH) Channel Pre-scaler : PWMC_CLKx
#define 	AT91C_PWMC_CPRE_MCK                  (0x0) // (PWMC_CH)
#define 	AT91C_PWMC_CPRE_MCKA                 (0xB) // (PWMC_CH)
#define 	AT91C_PWMC_CPRE_MCKB                 (0xC) // (PWMC_CH)
#define AT91C_PWMC_CALG       (0x1 <<  8) // (PWMC_CH) Channel Alignment
#define AT91C_PWMC_CPOL       (0x1 <<  9) // (PWMC_CH) Channel Polarity
#define AT91C_PWMC_CPD        (0x1 << 10) // (PWMC_CH) Channel Update Period
// -------- PWMC_CDTYR : (PWMC_CH Offset: 0x4) PWMC Channel Duty Cycle Register --------
#define AT91C_PWMC_CDTY       (0x0 <<  0) // (PWMC_CH) Channel Duty Cycle
// -------- PWMC_CPRDR : (PWMC_CH Offset: 0x8) PWMC Channel Period Register --------
#define AT91C_PWMC_CPRD       (0x0 <<  0) // (PWMC_CH) Channel Period
// -------- PWMC_CCNTR : (PWMC_CH Offset: 0xc) PWMC Channel Counter Register --------
#define AT91C_PWMC_CCNT       (0x0 <<  0) // (PWMC_CH) Channel Counter
// -------- PWMC_CUPDR : (PWMC_CH Offset: 0x10) PWMC Channel Update Register --------
#define AT91C_PWMC_CUPD       (0x0 <<  0) // (PWMC_CH) Channel Update

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Pulse Width Modulation Controller Interface
// *****************************************************************************
#ifndef __ASSEMBLY__
typedef struct _AT91S_PWMC {
	AT91_REG	 PWMC_MR; 	// PWMC Mode Register
	AT91_REG	 PWMC_ENA; 	// PWMC Enable Register
	AT91_REG	 PWMC_DIS; 	// PWMC Disable Register
	AT91_REG	 PWMC_SR; 	// PWMC Status Register
	AT91_REG	 PWMC_IER; 	// PWMC Interrupt Enable Register
	AT91_REG	 PWMC_IDR; 	// PWMC Interrupt Disable Register
	AT91_REG	 PWMC_IMR; 	// PWMC Interrupt Mask Register
	AT91_REG	 PWMC_ISR; 	// PWMC Interrupt Status Register
	AT91_REG	 Reserved0[55]; 	//
	AT91_REG	 PWMC_VR; 	// PWMC Version Register
	AT91_REG	 Reserved1[64]; 	//
	AT91S_PWMC_CH	 PWMC_CH[8]; 	// PWMC Channel
} AT91S_PWMC, *AT91PS_PWMC;
#else
#define PWMC_MR         (AT91_CAST(AT91_REG *) 	0x00000000) // (PWMC_MR) PWMC Mode Register
#define PWMC_ENA        (AT91_CAST(AT91_REG *) 	0x00000004) // (PWMC_ENA) PWMC Enable Register
#define PWMC_DIS        (AT91_CAST(AT91_REG *) 	0x00000008) // (PWMC_DIS) PWMC Disable Register
#define PWMC_SR         (AT91_CAST(AT91_REG *) 	0x0000000C) // (PWMC_SR) PWMC Status Register
#define PWMC_IER        (AT91_CAST(AT91_REG *) 	0x00000010) // (PWMC_IER) PWMC Interrupt Enable Register
#define PWMC_IDR        (AT91_CAST(AT91_REG *) 	0x00000014) // (PWMC_IDR) PWMC Interrupt Disable Register
#define PWMC_IMR        (AT91_CAST(AT91_REG *) 	0x00000018) // (PWMC_IMR) PWMC Interrupt Mask Register
#define PWMC_ISR        (AT91_CAST(AT91_REG *) 	0x0000001C) // (PWMC_ISR) PWMC Interrupt Status Register
#define PWMC_VR         (AT91_CAST(AT91_REG *) 	0x000000FC) // (PWMC_VR) PWMC Version Register

#endif
// -------- PWMC_MR : (PWMC Offset: 0x0) PWMC Mode Register --------
#define AT91C_PWMC_DIVA       (0xFF <<  0) // (PWMC) CLKA divide factor.
#define AT91C_PWMC_PREA       (0xF <<  8) // (PWMC) Divider Input Clock Prescaler A
#define 	AT91C_PWMC_PREA_MCK                  (0x0 <<  8) // (PWMC)
#define AT91C_PWMC_DIVB       (0xFF << 16) // (PWMC) CLKB divide factor.
#define AT91C_PWMC_PREB       (0xF << 24) // (PWMC) Divider Input Clock Prescaler B
#define 	AT91C_PWMC_PREB_MCK                  (0x0 << 24) // (PWMC)
// -------- PWMC_ENA : (PWMC Offset: 0x4) PWMC Enable Register --------
#define AT91C_PWMC_CHID0      (0x1 <<  0) // (PWMC) Channel ID 0
#define AT91C_PWMC_CHID1      (0x1 <<  1) // (PWMC) Channel ID 1
#define AT91C_PWMC_CHID2      (0x1 <<  2) // (PWMC) Channel ID 2
#define AT91C_PWMC_CHID3      (0x1 <<  3) // (PWMC) Channel ID 3
#define AT91C_PWMC_CHID4      (0x1 <<  4) // (PWMC) Channel ID 4
#define AT91C_PWMC_CHID5      (0x1 <<  5) // (PWMC) Channel ID 5
#define AT91C_PWMC_CHID6      (0x1 <<  6) // (PWMC) Channel ID 6
#define AT91C_PWMC_CHID7      (0x1 <<  7) // (PWMC) Channel ID 7
// -------- PWMC_DIS : (PWMC Offset: 0x8) PWMC Disable Register --------
// -------- PWMC_SR : (PWMC Offset: 0xc) PWMC Status Register --------
// -------- PWMC_IER : (PWMC Offset: 0x10) PWMC Interrupt Enable Register --------
// -------- PWMC_IDR : (PWMC Offset: 0x14) PWMC Interrupt Disable Register --------
// -------- PWMC_IMR : (PWMC Offset: 0x18) PWMC Interrupt Mask Register --------
// -------- PWMC_ISR : (PWMC Offset: 0x1c) PWMC Interrupt Status Register --------

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Synchronous Serial Controller Interface
// *****************************************************************************
#ifndef __ASSEMBLY__
typedef struct _AT91S_SSC {
	AT91_REG	 SSC_CR; 	// Control Register
	AT91_REG	 SSC_CMR; 	// Clock Mode Register
	AT91_REG	 Reserved0[2]; 	//
	AT91_REG	 SSC_RCMR; 	// Receive Clock ModeRegister
	AT91_REG	 SSC_RFMR; 	// Receive Frame Mode Register
	AT91_REG	 SSC_TCMR; 	// Transmit Clock Mode Register
	AT91_REG	 SSC_TFMR; 	// Transmit Frame Mode Register
	AT91_REG	 SSC_RHR; 	// Receive Holding Register
	AT91_REG	 SSC_THR; 	// Transmit Holding Register
	AT91_REG	 Reserved1[2]; 	//
	AT91_REG	 SSC_RSHR; 	// Receive Sync Holding Register
	AT91_REG	 SSC_TSHR; 	// Transmit Sync Holding Register
	AT91_REG	 Reserved2[2]; 	//
	AT91_REG	 SSC_SR; 	// Status Register
	AT91_REG	 SSC_IER; 	// Interrupt Enable Register
	AT91_REG	 SSC_IDR; 	// Interrupt Disable Register
	AT91_REG	 SSC_IMR; 	// Interrupt Mask Register
	AT91_REG	 Reserved3[44]; 	//
	AT91_REG	 SSC_RPR; 	// Receive Pointer Register
	AT91_REG	 SSC_RCR; 	// Receive Counter Register
	AT91_REG	 SSC_TPR; 	// Transmit Pointer Register
	AT91_REG	 SSC_TCR; 	// Transmit Counter Register
	AT91_REG	 SSC_RNPR; 	// Receive Next Pointer Register
	AT91_REG	 SSC_RNCR; 	// Receive Next Counter Register
	AT91_REG	 SSC_TNPR; 	// Transmit Next Pointer Register
	AT91_REG	 SSC_TNCR; 	// Transmit Next Counter Register
	AT91_REG	 SSC_PTCR; 	// PDC Transfer Control Register
	AT91_REG	 SSC_PTSR; 	// PDC Transfer Status Register
} AT91S_SSC, *AT91PS_SSC;
#else
#define SSC_CR          (AT91_CAST(AT91_REG *) 	0x00000000) // (SSC_CR) Control Register
#define SSC_CMR         (AT91_CAST(AT91_REG *) 	0x00000004) // (SSC_CMR) Clock Mode Register
#define SSC_RCMR        (AT91_CAST(AT91_REG *) 	0x00000010) // (SSC_RCMR) Receive Clock ModeRegister
#define SSC_RFMR        (AT91_CAST(AT91_REG *) 	0x00000014) // (SSC_RFMR) Receive Frame Mode Register
#define SSC_TCMR        (AT91_CAST(AT91_REG *) 	0x00000018) // (SSC_TCMR) Transmit Clock Mode Register
#define SSC_TFMR        (AT91_CAST(AT91_REG *) 	0x0000001C) // (SSC_TFMR) Transmit Frame Mode Register
#define SSC_RHR         (AT91_CAST(AT91_REG *) 	0x00000020) // (SSC_RHR) Receive Holding Register
#define SSC_THR         (AT91_CAST(AT91_REG *) 	0x00000024) // (SSC_THR) Transmit Holding Register
#define SSC_RSHR        (AT91_CAST(AT91_REG *) 	0x00000030) // (SSC_RSHR) Receive Sync Holding Register
#define SSC_TSHR        (AT91_CAST(AT91_REG *) 	0x00000034) // (SSC_TSHR) Transmit Sync Holding Register
#define SSC_SR          (AT91_CAST(AT91_REG *) 	0x00000040) // (SSC_SR) Status Register
#define SSC_IER         (AT91_CAST(AT91_REG *) 	0x00000044) // (SSC_IER) Interrupt Enable Register
#define SSC_IDR         (AT91_CAST(AT91_REG *) 	0x00000048) // (SSC_IDR) Interrupt Disable Register
#define SSC_IMR         (AT91_CAST(AT91_REG *) 	0x0000004C) // (SSC_IMR) Interrupt Mask Register

#endif
// -------- SSC_CR : (SSC Offset: 0x0) SSC Control Register --------
#define AT91C_SSC_RXEN        (0x1 <<  0) // (SSC) Receive Enable
#define AT91C_SSC_RXDIS       (0x1 <<  1) // (SSC) Receive Disable
#define AT91C_SSC_TXEN        (0x1 <<  8) // (SSC) Transmit Enable
#define AT91C_SSC_TXDIS       (0x1 <<  9) // (SSC) Transmit Disable
#define AT91C_SSC_SWRST       (0x1 << 15) // (SSC) Software Reset
// -------- SSC_RCMR : (SSC Offset: 0x10) SSC Receive Clock Mode Register --------
#define AT91C_SSC_CKS         (0x3 <<  0) // (SSC) Receive/Transmit Clock Selection
#define 	AT91C_SSC_CKS_DIV                  (0x0) // (SSC) Divided Clock
#define 	AT91C_SSC_CKS_TK                   (0x1) // (SSC) TK Clock signal
#define 	AT91C_SSC_CKS_RK                   (0x2) // (SSC) RK pin
#define AT91C_SSC_CKO         (0x7 <<  2) // (SSC) Receive/Transmit Clock Output Mode Selection
#define 	AT91C_SSC_CKO_NONE                 (0x0 <<  2) // (SSC) Receive/Transmit Clock Output Mode: None RK pin: Input-only
#define 	AT91C_SSC_CKO_CONTINOUS            (0x1 <<  2) // (SSC) Continuous Receive/Transmit Clock RK pin: Output
#define 	AT91C_SSC_CKO_DATA_TX              (0x2 <<  2) // (SSC) Receive/Transmit Clock only during data transfers RK pin: Output
#define AT91C_SSC_CKI         (0x1 <<  5) // (SSC) Receive/Transmit Clock Inversion
#define AT91C_SSC_START       (0xF <<  8) // (SSC) Receive/Transmit Start Selection
#define 	AT91C_SSC_START_CONTINOUS            (0x0 <<  8) // (SSC) Continuous, as soon as the receiver is enabled, and immediately after the end of transfer of the previous data.
#define 	AT91C_SSC_START_TX                   (0x1 <<  8) // (SSC) Transmit/Receive start
#define 	AT91C_SSC_START_LOW_RF               (0x2 <<  8) // (SSC) Detection of a low level on RF input
#define 	AT91C_SSC_START_HIGH_RF              (0x3 <<  8) // (SSC) Detection of a high level on RF input
#define 	AT91C_SSC_START_FALL_RF              (0x4 <<  8) // (SSC) Detection of a falling edge on RF input
#define 	AT91C_SSC_START_RISE_RF              (0x5 <<  8) // (SSC) Detection of a rising edge on RF input
#define 	AT91C_SSC_START_LEVEL_RF             (0x6 <<  8) // (SSC) Detection of any level change on RF input
#define 	AT91C_SSC_START_EDGE_RF              (0x7 <<  8) // (SSC) Detection of any edge on RF input
#define 	AT91C_SSC_START_0                    (0x8 <<  8) // (SSC) Compare 0
#define AT91C_SSC_STTDLY      (0xFF << 16) // (SSC) Receive/Transmit Start Delay
#define AT91C_SSC_PERIOD      (0xFF << 24) // (SSC) Receive/Transmit Period Divider Selection
// -------- SSC_RFMR : (SSC Offset: 0x14) SSC Receive Frame Mode Register --------
#define AT91C_SSC_DATLEN      (0x1F <<  0) // (SSC) Data Length
#define AT91C_SSC_LOOP        (0x1 <<  5) // (SSC) Loop Mode
#define AT91C_SSC_MSBF        (0x1 <<  7) // (SSC) Most Significant Bit First
#define AT91C_SSC_DATNB       (0xF <<  8) // (SSC) Data Number per Frame
#define AT91C_SSC_FSLEN       (0xF << 16) // (SSC) Receive/Transmit Frame Sync length
#define AT91C_SSC_FSOS        (0x7 << 20) // (SSC) Receive/Transmit Frame Sync Output Selection
#define 	AT91C_SSC_FSOS_NONE                 (0x0 << 20) // (SSC) Selected Receive/Transmit Frame Sync Signal: None RK pin Input-only
#define 	AT91C_SSC_FSOS_NEGATIVE             (0x1 << 20) // (SSC) Selected Receive/Transmit Frame Sync Signal: Negative Pulse
#define 	AT91C_SSC_FSOS_POSITIVE             (0x2 << 20) // (SSC) Selected Receive/Transmit Frame Sync Signal: Positive Pulse
#define 	AT91C_SSC_FSOS_LOW                  (0x3 << 20) // (SSC) Selected Receive/Transmit Frame Sync Signal: Driver Low during data transfer
#define 	AT91C_SSC_FSOS_HIGH                 (0x4 << 20) // (SSC) Selected Receive/Transmit Frame Sync Signal: Driver High during data transfer
#define 	AT91C_SSC_FSOS_TOGGLE               (0x5 << 20) // (SSC) Selected Receive/Transmit Frame Sync Signal: Toggling at each start of data transfer
#define AT91C_SSC_FSEDGE      (0x1 << 24) // (SSC) Frame Sync Edge Detection
// -------- SSC_TCMR : (SSC Offset: 0x18) SSC Transmit Clock Mode Register --------
// -------- SSC_TFMR : (SSC Offset: 0x1c) SSC Transmit Frame Mode Register --------
#define AT91C_SSC_DATDEF      (0x1 <<  5) // (SSC) Data Default Value
#define AT91C_SSC_FSDEN       (0x1 << 23) // (SSC) Frame Sync Data Enable
// -------- SSC_SR : (SSC Offset: 0x40) SSC Status Register --------
#define AT91C_SSC_TXRDY       (0x1 <<  0) // (SSC) Transmit Ready
#define AT91C_SSC_TXEMPTY     (0x1 <<  1) // (SSC) Transmit Empty
#define AT91C_SSC_ENDTX       (0x1 <<  2) // (SSC) End Of Transmission
#define AT91C_SSC_TXBUFE      (0x1 <<  3) // (SSC) Transmit Buffer Empty
#define AT91C_SSC_RXRDY       (0x1 <<  4) // (SSC) Receive Ready
#define AT91C_SSC_OVRUN       (0x1 <<  5) // (SSC) Receive Overrun
#define AT91C_SSC_ENDRX       (0x1 <<  6) // (SSC) End of Reception
#define AT91C_SSC_RXBUFF      (0x1 <<  7) // (SSC) Receive Buffer Full
#define AT91C_SSC_TXSYN       (0x1 << 10) // (SSC) Transmit Sync
#define AT91C_SSC_RXSYN       (0x1 << 11) // (SSC) Receive Sync
#define AT91C_SSC_TXENA       (0x1 << 16) // (SSC) Transmit Enable
#define AT91C_SSC_RXENA       (0x1 << 17) // (SSC) Receive Enable
// -------- SSC_IER : (SSC Offset: 0x44) SSC Interrupt Enable Register --------
// -------- SSC_IDR : (SSC Offset: 0x48) SSC Interrupt Disable Register --------
// -------- SSC_IMR : (SSC Offset: 0x4c) SSC Interrupt Mask Register --------

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Analog to Digital Convertor
// *****************************************************************************
#ifndef __ASSEMBLY__
typedef struct _AT91S_ADC {
	AT91_REG	 ADC_CR; 	// ADC Control Register
	AT91_REG	 ADC_MR; 	// ADC Mode Register
	AT91_REG	 Reserved0[2]; 	//
	AT91_REG	 ADC_CHER; 	// ADC Channel Enable Register
	AT91_REG	 ADC_CHDR; 	// ADC Channel Disable Register
	AT91_REG	 ADC_CHSR; 	// ADC Channel Status Register
	AT91_REG	 ADC_SR; 	// ADC Status Register
	AT91_REG	 ADC_LCDR; 	// ADC Last Converted Data Register
	AT91_REG	 ADC_IER; 	// ADC Interrupt Enable Register
	AT91_REG	 ADC_IDR; 	// ADC Interrupt Disable Register
	AT91_REG	 ADC_IMR; 	// ADC Interrupt Mask Register
	AT91_REG	 ADC_CDR0; 	// ADC Channel Data Register 0
	AT91_REG	 ADC_CDR1; 	// ADC Channel Data Register 1
	AT91_REG	 ADC_CDR2; 	// ADC Channel Data Register 2
	AT91_REG	 ADC_CDR3; 	// ADC Channel Data Register 3
	AT91_REG	 ADC_CDR4; 	// ADC Channel Data Register 4
	AT91_REG	 ADC_CDR5; 	// ADC Channel Data Register 5
	AT91_REG	 ADC_CDR6; 	// ADC Channel Data Register 6
	AT91_REG	 ADC_CDR7; 	// ADC Channel Data Register 7
	AT91_REG	 Reserved1[44]; 	//
	AT91_REG	 ADC_RPR; 	// Receive Pointer Register
	AT91_REG	 ADC_RCR; 	// Receive Counter Register
	AT91_REG	 ADC_TPR; 	// Transmit Pointer Register
	AT91_REG	 ADC_TCR; 	// Transmit Counter Register
	AT91_REG	 ADC_RNPR; 	// Receive Next Pointer Register
	AT91_REG	 ADC_RNCR; 	// Receive Next Counter Register
	AT91_REG	 ADC_TNPR; 	// Transmit Next Pointer Register
	AT91_REG	 ADC_TNCR; 	// Transmit Next Counter Register
	AT91_REG	 ADC_PTCR; 	// PDC Transfer Control Register
	AT91_REG	 ADC_PTSR; 	// PDC Transfer Status Register
} AT91S_ADC, *AT91PS_ADC;
#else
#define ADC_CR          (AT91_CAST(AT91_REG *) 	0x00000000) // (ADC_CR) ADC Control Register
#define ADC_MR          (AT91_CAST(AT91_REG *) 	0x00000004) // (ADC_MR) ADC Mode Register
#define ADC_CHER        (AT91_CAST(AT91_REG *) 	0x00000010) // (ADC_CHER) ADC Channel Enable Register
#define ADC_CHDR        (AT91_CAST(AT91_REG *) 	0x00000014) // (ADC_CHDR) ADC Channel Disable Register
#define ADC_CHSR        (AT91_CAST(AT91_REG *) 	0x00000018) // (ADC_CHSR) ADC Channel Status Register
#define ADC_SR          (AT91_CAST(AT91_REG *) 	0x0000001C) // (ADC_SR) ADC Status Register
#define ADC_LCDR        (AT91_CAST(AT91_REG *) 	0x00000020) // (ADC_LCDR) ADC Last Converted Data Register
#define ADC_IER         (AT91_CAST(AT91_REG *) 	0x00000024) // (ADC_IER) ADC Interrupt Enable Register
#define ADC_IDR         (AT91_CAST(AT91_REG *) 	0x00000028) // (ADC_IDR) ADC Interrupt Disable Register
#define ADC_IMR         (AT91_CAST(AT91_REG *) 	0x0000002C) // (ADC_IMR) ADC Interrupt Mask Register
#define ADC_CDR0        (AT91_CAST(AT91_REG *) 	0x00000030) // (ADC_CDR0) ADC Channel Data Register 0
#define ADC_CDR1        (AT91_CAST(AT91_REG *) 	0x00000034) // (ADC_CDR1) ADC Channel Data Register 1
#define ADC_CDR2        (AT91_CAST(AT91_REG *) 	0x00000038) // (ADC_CDR2) ADC Channel Data Register 2
#define ADC_CDR3        (AT91_CAST(AT91_REG *) 	0x0000003C) // (ADC_CDR3) ADC Channel Data Register 3
#define ADC_CDR4        (AT91_CAST(AT91_REG *) 	0x00000040) // (ADC_CDR4) ADC Channel Data Register 4
#define ADC_CDR5        (AT91_CAST(AT91_REG *) 	0x00000044) // (ADC_CDR5) ADC Channel Data Register 5
#define ADC_CDR6        (AT91_CAST(AT91_REG *) 	0x00000048) // (ADC_CDR6) ADC Channel Data Register 6
#define ADC_CDR7        (AT91_CAST(AT91_REG *) 	0x0000004C) // (ADC_CDR7) ADC Channel Data Register 7

#endif
// -------- ADC_CR : (ADC Offset: 0x0) ADC Control Register --------
#define AT91C_ADC_SWRST       (0x1 <<  0) // (ADC) Software Reset
#define AT91C_ADC_START       (0x1 <<  1) // (ADC) Start Conversion
// -------- ADC_MR : (ADC Offset: 0x4) ADC Mode Register --------
#define AT91C_ADC_TRGEN       (0x1 <<  0) // (ADC) Trigger Enable
#define 	AT91C_ADC_TRGEN_DIS                  (0x0) // (ADC) Hradware triggers are disabled. Starting a conversion is only possible by software
#define 	AT91C_ADC_TRGEN_EN                   (0x1) // (ADC) Hardware trigger selected by TRGSEL field is enabled.
#define AT91C_ADC_TRGSEL      (0x7 <<  1) // (ADC) Trigger Selection
#define 	AT91C_ADC_TRGSEL_TIOA0                (0x0 <<  1) // (ADC) Selected TRGSEL = TIAO0
#define 	AT91C_ADC_TRGSEL_TIOA1                (0x1 <<  1) // (ADC) Selected TRGSEL = TIAO1
#define 	AT91C_ADC_TRGSEL_TIOA2                (0x2 <<  1) // (ADC) Selected TRGSEL = TIAO2
#define 	AT91C_ADC_TRGSEL_TIOA3                (0x3 <<  1) // (ADC) Selected TRGSEL = TIAO3
#define 	AT91C_ADC_TRGSEL_TIOA4                (0x4 <<  1) // (ADC) Selected TRGSEL = TIAO4
#define 	AT91C_ADC_TRGSEL_TIOA5                (0x5 <<  1) // (ADC) Selected TRGSEL = TIAO5
#define 	AT91C_ADC_TRGSEL_EXT                  (0x6 <<  1) // (ADC) Selected TRGSEL = External Trigger
#define AT91C_ADC_LOWRES      (0x1 <<  4) // (ADC) Resolution.
#define 	AT91C_ADC_LOWRES_10_BIT               (0x0 <<  4) // (ADC) 10-bit resolution
#define 	AT91C_ADC_LOWRES_8_BIT                (0x1 <<  4) // (ADC) 8-bit resolution
#define AT91C_ADC_SLEEP       (0x1 <<  5) // (ADC) Sleep Mode
#define 	AT91C_ADC_SLEEP_NORMAL_MODE          (0x0 <<  5) // (ADC) Normal Mode
#define 	AT91C_ADC_SLEEP_MODE                 (0x1 <<  5) // (ADC) Sleep Mode
#define AT91C_ADC_PRESCAL     (0x3F <<  8) // (ADC) Prescaler rate selection
#define AT91C_ADC_STARTUP     (0x1F << 16) // (ADC) Startup Time
#define AT91C_ADC_SHTIM       (0xF << 24) // (ADC) Sample & Hold Time
// -------- 	ADC_CHER : (ADC Offset: 0x10) ADC Channel Enable Register --------
#define AT91C_ADC_CH0         (0x1 <<  0) // (ADC) Channel 0
#define AT91C_ADC_CH1         (0x1 <<  1) // (ADC) Channel 1
#define AT91C_ADC_CH2         (0x1 <<  2) // (ADC) Channel 2
#define AT91C_ADC_CH3         (0x1 <<  3) // (ADC) Channel 3
#define AT91C_ADC_CH4         (0x1 <<  4) // (ADC) Channel 4
#define AT91C_ADC_CH5         (0x1 <<  5) // (ADC) Channel 5
#define AT91C_ADC_CH6         (0x1 <<  6) // (ADC) Channel 6
#define AT91C_ADC_CH7         (0x1 <<  7) // (ADC) Channel 7
// -------- 	ADC_CHDR : (ADC Offset: 0x14) ADC Channel Disable Register --------
// -------- 	ADC_CHSR : (ADC Offset: 0x18) ADC Channel Status Register --------
// -------- ADC_SR : (ADC Offset: 0x1c) ADC Status Register --------
#define AT91C_ADC_EOC0        (0x1 <<  0) // (ADC) End of Conversion
#define AT91C_ADC_EOC1        (0x1 <<  1) // (ADC) End of Conversion
#define AT91C_ADC_EOC2        (0x1 <<  2) // (ADC) End of Conversion
#define AT91C_ADC_EOC3        (0x1 <<  3) // (ADC) End of Conversion
#define AT91C_ADC_EOC4        (0x1 <<  4) // (ADC) End of Conversion
#define AT91C_ADC_EOC5        (0x1 <<  5) // (ADC) End of Conversion
#define AT91C_ADC_EOC6        (0x1 <<  6) // (ADC) End of Conversion
#define AT91C_ADC_EOC7        (0x1 <<  7) // (ADC) End of Conversion
#define AT91C_ADC_OVRE0       (0x1 <<  8) // (ADC) Overrun Error
#define AT91C_ADC_OVRE1       (0x1 <<  9) // (ADC) Overrun Error
#define AT91C_ADC_OVRE2       (0x1 << 10) // (ADC) Overrun Error
#define AT91C_ADC_OVRE3       (0x1 << 11) // (ADC) Overrun Error
#define AT91C_ADC_OVRE4       (0x1 << 12) // (ADC) Overrun Error
#define AT91C_ADC_OVRE5       (0x1 << 13) // (ADC) Overrun Error
#define AT91C_ADC_OVRE6       (0x1 << 14) // (ADC) Overrun Error
#define AT91C_ADC_OVRE7       (0x1 << 15) // (ADC) Overrun Error
#define AT91C_ADC_DRDY        (0x1 << 16) // (ADC) Data Ready
#define AT91C_ADC_GOVRE       (0x1 << 17) // (ADC) General Overrun
#define AT91C_ADC_ENDRX       (0x1 << 18) // (ADC) End of Receiver Transfer
#define AT91C_ADC_RXBUFF      (0x1 << 19) // (ADC) RXBUFF Interrupt
// -------- ADC_LCDR : (ADC Offset: 0x20) ADC Last Converted Data Register --------
#define AT91C_ADC_LDATA       (0x3FF <<  0) // (ADC) Last Data Converted
// -------- ADC_IER : (ADC Offset: 0x24) ADC Interrupt Enable Register --------
// -------- ADC_IDR : (ADC Offset: 0x28) ADC Interrupt Disable Register --------
// -------- ADC_IMR : (ADC Offset: 0x2c) ADC Interrupt Mask Register --------
// -------- ADC_CDR0 : (ADC Offset: 0x30) ADC Channel Data Register 0 --------
#define AT91C_ADC_DATA        (0x3FF <<  0) // (ADC) Converted Data
// -------- ADC_CDR1 : (ADC Offset: 0x34) ADC Channel Data Register 1 --------
// -------- ADC_CDR2 : (ADC Offset: 0x38) ADC Channel Data Register 2 --------
// -------- ADC_CDR3 : (ADC Offset: 0x3c) ADC Channel Data Register 3 --------
// -------- ADC_CDR4 : (ADC Offset: 0x40) ADC Channel Data Register 4 --------
// -------- ADC_CDR5 : (ADC Offset: 0x44) ADC Channel Data Register 5 --------
// -------- ADC_CDR6 : (ADC Offset: 0x48) ADC Channel Data Register 6 --------
// -------- ADC_CDR7 : (ADC Offset: 0x4c) ADC Channel Data Register 7 --------

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Serial Parallel Interface
// *****************************************************************************
#ifndef __ASSEMBLY__
typedef struct _AT91S_SPI {
	AT91_REG	 SPI_CR; 	// Control Register
	AT91_REG	 SPI_MR; 	// Mode Register
	AT91_REG	 SPI_RDR; 	// Receive Data Register
	AT91_REG	 SPI_TDR; 	// Transmit Data Register
	AT91_REG	 SPI_SR; 	// Status Register
	AT91_REG	 SPI_IER; 	// Interrupt Enable Register
	AT91_REG	 SPI_IDR; 	// Interrupt Disable Register
	AT91_REG	 SPI_IMR; 	// Interrupt Mask Register
	AT91_REG	 Reserved0[4]; 	//
	AT91_REG	 SPI_CSR[4]; 	// Chip Select Register
	AT91_REG	 Reserved1[48]; 	//
	AT91_REG	 SPI_RPR; 	// Receive Pointer Register
	AT91_REG	 SPI_RCR; 	// Receive Counter Register
	AT91_REG	 SPI_TPR; 	// Transmit Pointer Register
	AT91_REG	 SPI_TCR; 	// Transmit Counter Register
	AT91_REG	 SPI_RNPR; 	// Receive Next Pointer Register
	AT91_REG	 SPI_RNCR; 	// Receive Next Counter Register
	AT91_REG	 SPI_TNPR; 	// Transmit Next Pointer Register
	AT91_REG	 SPI_TNCR; 	// Transmit Next Counter Register
	AT91_REG	 SPI_PTCR; 	// PDC Transfer Control Register
	AT91_REG	 SPI_PTSR; 	// PDC Transfer Status Register
} AT91S_SPI, *AT91PS_SPI;
#else
#define SPI_CR          (AT91_CAST(AT91_REG *) 	0x00000000) // (SPI_CR) Control Register
#define SPI_MR          (AT91_CAST(AT91_REG *) 	0x00000004) // (SPI_MR) Mode Register
#define SPI_RDR         (AT91_CAST(AT91_REG *) 	0x00000008) // (SPI_RDR) Receive Data Register
#define SPI_TDR         (AT91_CAST(AT91_REG *) 	0x0000000C) // (SPI_TDR) Transmit Data Register
#define SPI_SR          (AT91_CAST(AT91_REG *) 	0x00000010) // (SPI_SR) Status Register
#define SPI_IER         (AT91_CAST(AT91_REG *) 	0x00000014) // (SPI_IER) Interrupt Enable Register
#define SPI_IDR         (AT91_CAST(AT91_REG *) 	0x00000018) // (SPI_IDR) Interrupt Disable Register
#define SPI_IMR         (AT91_CAST(AT91_REG *) 	0x0000001C) // (SPI_IMR) Interrupt Mask Register
#define SPI_CSR         (AT91_CAST(AT91_REG *) 	0x00000030) // (SPI_CSR) Chip Select Register

#endif
// -------- SPI_CR : (SPI Offset: 0x0) SPI Control Register --------
#define AT91C_SPI_SPIEN       (0x1 <<  0) // (SPI) SPI Enable
#define AT91C_SPI_SPIDIS      (0x1 <<  1) // (SPI) SPI Disable
#define AT91C_SPI_SWRST       (0x1 <<  7) // (SPI) SPI Software reset
#define AT91C_SPI_LASTXFER    (0x1 << 24) // (SPI) SPI Last Transfer
// -------- SPI_MR : (SPI Offset: 0x4) SPI Mode Register --------
#define AT91C_SPI_MSTR        (0x1 <<  0) // (SPI) Master/Slave Mode
#define AT91C_SPI_PS          (0x1 <<  1) // (SPI) Peripheral Select
#define 	AT91C_SPI_PS_FIXED                (0x0 <<  1) // (SPI) Fixed Peripheral Select
#define 	AT91C_SPI_PS_VARIABLE             (0x1 <<  1) // (SPI) Variable Peripheral Select
#define AT91C_SPI_PCSDEC      (0x1 <<  2) // (SPI) Chip Select Decode
#define AT91C_SPI_FDIV        (0x1 <<  3) // (SPI) Clock Selection
#define AT91C_SPI_MODFDIS     (0x1 <<  4) // (SPI) Mode Fault Detection
#define AT91C_SPI_LLB         (0x1 <<  7) // (SPI) Clock Selection
#define AT91C_SPI_PCS         (0xF << 16) // (SPI) Peripheral Chip Select
#define AT91C_SPI_DLYBCS      (0xFF << 24) // (SPI) Delay Between Chip Selects
// -------- SPI_RDR : (SPI Offset: 0x8) Receive Data Register --------
#define AT91C_SPI_RD          (0xFFFF <<  0) // (SPI) Receive Data
#define AT91C_SPI_RPCS        (0xF << 16) // (SPI) Peripheral Chip Select Status
// -------- SPI_TDR : (SPI Offset: 0xc) Transmit Data Register --------
#define AT91C_SPI_TD          (0xFFFF <<  0) // (SPI) Transmit Data
#define AT91C_SPI_TPCS        (0xF << 16) // (SPI) Peripheral Chip Select Status
// -------- SPI_SR : (SPI Offset: 0x10) Status Register --------
#define AT91C_SPI_RDRF        (0x1 <<  0) // (SPI) Receive Data Register Full
#define AT91C_SPI_TDRE        (0x1 <<  1) // (SPI) Transmit Data Register Empty
#define AT91C_SPI_MODF        (0x1 <<  2) // (SPI) Mode Fault Error
#define AT91C_SPI_OVRES       (0x1 <<  3) // (SPI) Overrun Error Status
#define AT91C_SPI_ENDRX       (0x1 <<  4) // (SPI) End of Receiver Transfer
#define AT91C_SPI_ENDTX       (0x1 <<  5) // (SPI) End of Receiver Transfer
#define AT91C_SPI_RXBUFF      (0x1 <<  6) // (SPI) RXBUFF Interrupt
#define AT91C_SPI_TXBUFE      (0x1 <<  7) // (SPI) TXBUFE Interrupt
#define AT91C_SPI_NSSR        (0x1 <<  8) // (SPI) NSSR Interrupt
#define AT91C_SPI_TXEMPTY     (0x1 <<  9) // (SPI) TXEMPTY Interrupt
#define AT91C_SPI_SPIENS      (0x1 << 16) // (SPI) Enable Status
// -------- SPI_IER : (SPI Offset: 0x14) Interrupt Enable Register --------
// -------- SPI_IDR : (SPI Offset: 0x18) Interrupt Disable Register --------
// -------- SPI_IMR : (SPI Offset: 0x1c) Interrupt Mask Register --------
// -------- SPI_CSR : (SPI Offset: 0x30) Chip Select Register --------
#define AT91C_SPI_CPOL        (0x1 <<  0) // (SPI) Clock Polarity
#define AT91C_SPI_NCPHA       (0x1 <<  1) // (SPI) Clock Phase
#define AT91C_SPI_CSAAT       (0x1 <<  3) // (SPI) Chip Select Active After Transfer
#define AT91C_SPI_BITS        (0xF <<  4) // (SPI) Bits Per Transfer
#define 	AT91C_SPI_BITS_8                    (0x0 <<  4) // (SPI) 8 Bits Per transfer
#define 	AT91C_SPI_BITS_9                    (0x1 <<  4) // (SPI) 9 Bits Per transfer
#define 	AT91C_SPI_BITS_10                   (0x2 <<  4) // (SPI) 10 Bits Per transfer
#define 	AT91C_SPI_BITS_11                   (0x3 <<  4) // (SPI) 11 Bits Per transfer
#define 	AT91C_SPI_BITS_12                   (0x4 <<  4) // (SPI) 12 Bits Per transfer
#define 	AT91C_SPI_BITS_13                   (0x5 <<  4) // (SPI) 13 Bits Per transfer
#define 	AT91C_SPI_BITS_14                   (0x6 <<  4) // (SPI) 14 Bits Per transfer
#define 	AT91C_SPI_BITS_15                   (0x7 <<  4) // (SPI) 15 Bits Per transfer
#define 	AT91C_SPI_BITS_16                   (0x8 <<  4) // (SPI) 16 Bits Per transfer
#define AT91C_SPI_SCBR        (0xFF <<  8) // (SPI) Serial Clock Baud Rate
#define AT91C_SPI_DLYBS       (0xFF << 16) // (SPI) Delay Before SPCK
#define AT91C_SPI_DLYBCT      (0xFF << 24) // (SPI) Delay Between Consecutive Transfers

// *****************************************************************************
//               REGISTER ADDRESS DEFINITION FOR AT91SAM7A3
// *****************************************************************************
// ========== Register definition for SYS peripheral ==========
#define AT91C_SYS_GPBR1 (AT91_CAST(AT91_REG *) 	0xFFFFFD54) // (SYS) General Purpose Register 1
#define AT91C_SYS_GPBR0 (AT91_CAST(AT91_REG *) 	0xFFFFFD50) // (SYS) General Purpose Register 0
// ========== Register definition for AIC peripheral ==========
#define AT91C_AIC_IVR   (AT91_CAST(AT91_REG *) 	0xFFFFF100) // (AIC) IRQ Vector Register
#define AT91C_AIC_SMR   (AT91_CAST(AT91_REG *) 	0xFFFFF000) // (AIC) Source Mode Register
#define AT91C_AIC_FVR   (AT91_CAST(AT91_REG *) 	0xFFFFF104) // (AIC) FIQ Vector Register
#define AT91C_AIC_DCR   (AT91_CAST(AT91_REG *) 	0xFFFFF138) // (AIC) Debug Control Register (Protect)
#define AT91C_AIC_EOICR (AT91_CAST(AT91_REG *) 	0xFFFFF130) // (AIC) End of Interrupt Command Register
#define AT91C_AIC_SVR   (AT91_CAST(AT91_REG *) 	0xFFFFF080) // (AIC) Source Vector Register
#define AT91C_AIC_FFSR  (AT91_CAST(AT91_REG *) 	0xFFFFF148) // (AIC) Fast Forcing Status Register
#define AT91C_AIC_ICCR  (AT91_CAST(AT91_REG *) 	0xFFFFF128) // (AIC) Interrupt Clear Command Register
#define AT91C_AIC_ISR   (AT91_CAST(AT91_REG *) 	0xFFFFF108) // (AIC) Interrupt Status Register
#define AT91C_AIC_IMR   (AT91_CAST(AT91_REG *) 	0xFFFFF110) // (AIC) Interrupt Mask Register
#define AT91C_AIC_IPR   (AT91_CAST(AT91_REG *) 	0xFFFFF10C) // (AIC) Interrupt Pending Register
#define AT91C_AIC_FFER  (AT91_CAST(AT91_REG *) 	0xFFFFF140) // (AIC) Fast Forcing Enable Register
#define AT91C_AIC_IECR  (AT91_CAST(AT91_REG *) 	0xFFFFF120) // (AIC) Interrupt Enable Command Register
#define AT91C_AIC_ISCR  (AT91_CAST(AT91_REG *) 	0xFFFFF12C) // (AIC) Interrupt Set Command Register
#define AT91C_AIC_FFDR  (AT91_CAST(AT91_REG *) 	0xFFFFF144) // (AIC) Fast Forcing Disable Register
#define AT91C_AIC_CISR  (AT91_CAST(AT91_REG *) 	0xFFFFF114) // (AIC) Core Interrupt Status Register
#define AT91C_AIC_IDCR  (AT91_CAST(AT91_REG *) 	0xFFFFF124) // (AIC) Interrupt Disable Command Register
#define AT91C_AIC_SPU   (AT91_CAST(AT91_REG *) 	0xFFFFF134) // (AIC) Spurious Vector Register
// ========== Register definition for PDC_DBGU peripheral ==========
#define AT91C_DBGU_TCR  (AT91_CAST(AT91_REG *) 	0xFFFFF30C) // (PDC_DBGU) Transmit Counter Register
#define AT91C_DBGU_RNPR (AT91_CAST(AT91_REG *) 	0xFFFFF310) // (PDC_DBGU) Receive Next Pointer Register
#define AT91C_DBGU_TNPR (AT91_CAST(AT91_REG *) 	0xFFFFF318) // (PDC_DBGU) Transmit Next Pointer Register
#define AT91C_DBGU_TPR  (AT91_CAST(AT91_REG *) 	0xFFFFF308) // (PDC_DBGU) Transmit Pointer Register
#define AT91C_DBGU_RPR  (AT91_CAST(AT91_REG *) 	0xFFFFF300) // (PDC_DBGU) Receive Pointer Register
#define AT91C_DBGU_RCR  (AT91_CAST(AT91_REG *) 	0xFFFFF304) // (PDC_DBGU) Receive Counter Register
#define AT91C_DBGU_RNCR (AT91_CAST(AT91_REG *) 	0xFFFFF314) // (PDC_DBGU) Receive Next Counter Register
#define AT91C_DBGU_PTCR (AT91_CAST(AT91_REG *) 	0xFFFFF320) // (PDC_DBGU) PDC Transfer Control Register
#define AT91C_DBGU_PTSR (AT91_CAST(AT91_REG *) 	0xFFFFF324) // (PDC_DBGU) PDC Transfer Status Register
#define AT91C_DBGU_TNCR (AT91_CAST(AT91_REG *) 	0xFFFFF31C) // (PDC_DBGU) Transmit Next Counter Register
// ========== Register definition for DBGU peripheral ==========
#define AT91C_DBGU_EXID (AT91_CAST(AT91_REG *) 	0xFFFFF244) // (DBGU) Chip ID Extension Register
#define AT91C_DBGU_BRGR (AT91_CAST(AT91_REG *) 	0xFFFFF220) // (DBGU) Baud Rate Generator Register
#define AT91C_DBGU_IDR  (AT91_CAST(AT91_REG *) 	0xFFFFF20C) // (DBGU) Interrupt Disable Register
#define AT91C_DBGU_CSR  (AT91_CAST(AT91_REG *) 	0xFFFFF214) // (DBGU) Channel Status Register
#define AT91C_DBGU_CIDR (AT91_CAST(AT91_REG *) 	0xFFFFF240) // (DBGU) Chip ID Register
#define AT91C_DBGU_MR   (AT91_CAST(AT91_REG *) 	0xFFFFF204) // (DBGU) Mode Register
#define AT91C_DBGU_IMR  (AT91_CAST(AT91_REG *) 	0xFFFFF210) // (DBGU) Interrupt Mask Register
#define AT91C_DBGU_CR   (AT91_CAST(AT91_REG *) 	0xFFFFF200) // (DBGU) Control Register
#define AT91C_DBGU_FNTR (AT91_CAST(AT91_REG *) 	0xFFFFF248) // (DBGU) Force NTRST Register
#define AT91C_DBGU_THR  (AT91_CAST(AT91_REG *) 	0xFFFFF21C) // (DBGU) Transmitter Holding Register
#define AT91C_DBGU_RHR  (AT91_CAST(AT91_REG *) 	0xFFFFF218) // (DBGU) Receiver Holding Register
#define AT91C_DBGU_IER  (AT91_CAST(AT91_REG *) 	0xFFFFF208) // (DBGU) Interrupt Enable Register
// ========== Register definition for PIOA peripheral ==========
#define AT91C_PIOA_ODR  (AT91_CAST(AT91_REG *) 	0xFFFFF414) // (PIOA) Output Disable Registerr
#define AT91C_PIOA_SODR (AT91_CAST(AT91_REG *) 	0xFFFFF430) // (PIOA) Set Output Data Register
#define AT91C_PIOA_ISR  (AT91_CAST(AT91_REG *) 	0xFFFFF44C) // (PIOA) Interrupt Status Register
#define AT91C_PIOA_ABSR (AT91_CAST(AT91_REG *) 	0xFFFFF478) // (PIOA) AB Select Status Register
#define AT91C_PIOA_IER  (AT91_CAST(AT91_REG *) 	0xFFFFF440) // (PIOA) Interrupt Enable Register
#define AT91C_PIOA_PPUDR (AT91_CAST(AT91_REG *) 	0xFFFFF460) // (PIOA) Pull-up Disable Register
#define AT91C_PIOA_IMR  (AT91_CAST(AT91_REG *) 	0xFFFFF448) // (PIOA) Interrupt Mask Register
#define AT91C_PIOA_PER  (AT91_CAST(AT91_REG *) 	0xFFFFF400) // (PIOA) PIO Enable Register
#define AT91C_PIOA_IFDR (AT91_CAST(AT91_REG *) 	0xFFFFF424) // (PIOA) Input Filter Disable Register
#define AT91C_PIOA_OWDR (AT91_CAST(AT91_REG *) 	0xFFFFF4A4) // (PIOA) Output Write Disable Register
#define AT91C_PIOA_MDSR (AT91_CAST(AT91_REG *) 	0xFFFFF458) // (PIOA) Multi-driver Status Register
#define AT91C_PIOA_IDR  (AT91_CAST(AT91_REG *) 	0xFFFFF444) // (PIOA) Interrupt Disable Register
#define AT91C_PIOA_ODSR (AT91_CAST(AT91_REG *) 	0xFFFFF438) // (PIOA) Output Data Status Register
#define AT91C_PIOA_PPUSR (AT91_CAST(AT91_REG *) 	0xFFFFF468) // (PIOA) Pull-up Status Register
#define AT91C_PIOA_OWSR (AT91_CAST(AT91_REG *) 	0xFFFFF4A8) // (PIOA) Output Write Status Register
#define AT91C_PIOA_BSR  (AT91_CAST(AT91_REG *) 	0xFFFFF474) // (PIOA) Select B Register
#define AT91C_PIOA_OWER (AT91_CAST(AT91_REG *) 	0xFFFFF4A0) // (PIOA) Output Write Enable Register
#define AT91C_PIOA_IFER (AT91_CAST(AT91_REG *) 	0xFFFFF420) // (PIOA) Input Filter Enable Register
#define AT91C_PIOA_PDSR (AT91_CAST(AT91_REG *) 	0xFFFFF43C) // (PIOA) Pin Data Status Register
#define AT91C_PIOA_PPUER (AT91_CAST(AT91_REG *) 	0xFFFFF464) // (PIOA) Pull-up Enable Register
#define AT91C_PIOA_OSR  (AT91_CAST(AT91_REG *) 	0xFFFFF418) // (PIOA) Output Status Register
#define AT91C_PIOA_ASR  (AT91_CAST(AT91_REG *) 	0xFFFFF470) // (PIOA) Select A Register
#define AT91C_PIOA_MDDR (AT91_CAST(AT91_REG *) 	0xFFFFF454) // (PIOA) Multi-driver Disable Register
#define AT91C_PIOA_CODR (AT91_CAST(AT91_REG *) 	0xFFFFF434) // (PIOA) Clear Output Data Register
#define AT91C_PIOA_MDER (AT91_CAST(AT91_REG *) 	0xFFFFF450) // (PIOA) Multi-driver Enable Register
#define AT91C_PIOA_PDR  (AT91_CAST(AT91_REG *) 	0xFFFFF404) // (PIOA) PIO Disable Register
#define AT91C_PIOA_IFSR (AT91_CAST(AT91_REG *) 	0xFFFFF428) // (PIOA) Input Filter Status Register
#define AT91C_PIOA_OER  (AT91_CAST(AT91_REG *) 	0xFFFFF410) // (PIOA) Output Enable Register
#define AT91C_PIOA_PSR  (AT91_CAST(AT91_REG *) 	0xFFFFF408) // (PIOA) PIO Status Register
// ========== Register definition for PIOB peripheral ==========
#define AT91C_PIOB_OWDR (AT91_CAST(AT91_REG *) 	0xFFFFF6A4) // (PIOB) Output Write Disable Register
#define AT91C_PIOB_MDER (AT91_CAST(AT91_REG *) 	0xFFFFF650) // (PIOB) Multi-driver Enable Register
#define AT91C_PIOB_PPUSR (AT91_CAST(AT91_REG *) 	0xFFFFF668) // (PIOB) Pull-up Status Register
#define AT91C_PIOB_IMR  (AT91_CAST(AT91_REG *) 	0xFFFFF648) // (PIOB) Interrupt Mask Register
#define AT91C_PIOB_ASR  (AT91_CAST(AT91_REG *) 	0xFFFFF670) // (PIOB) Select A Register
#define AT91C_PIOB_PPUDR (AT91_CAST(AT91_REG *) 	0xFFFFF660) // (PIOB) Pull-up Disable Register
#define AT91C_PIOB_PSR  (AT91_CAST(AT91_REG *) 	0xFFFFF608) // (PIOB) PIO Status Register
#define AT91C_PIOB_IER  (AT91_CAST(AT91_REG *) 	0xFFFFF640) // (PIOB) Interrupt Enable Register
#define AT91C_PIOB_CODR (AT91_CAST(AT91_REG *) 	0xFFFFF634) // (PIOB) Clear Output Data Register
#define AT91C_PIOB_OWER (AT91_CAST(AT91_REG *) 	0xFFFFF6A0) // (PIOB) Output Write Enable Register
#define AT91C_PIOB_ABSR (AT91_CAST(AT91_REG *) 	0xFFFFF678) // (PIOB) AB Select Status Register
#define AT91C_PIOB_IFDR (AT91_CAST(AT91_REG *) 	0xFFFFF624) // (PIOB) Input Filter Disable Register
#define AT91C_PIOB_PDSR (AT91_CAST(AT91_REG *) 	0xFFFFF63C) // (PIOB) Pin Data Status Register
#define AT91C_PIOB_IDR  (AT91_CAST(AT91_REG *) 	0xFFFFF644) // (PIOB) Interrupt Disable Register
#define AT91C_PIOB_OWSR (AT91_CAST(AT91_REG *) 	0xFFFFF6A8) // (PIOB) Output Write Status Register
#define AT91C_PIOB_PDR  (AT91_CAST(AT91_REG *) 	0xFFFFF604) // (PIOB) PIO Disable Register
#define AT91C_PIOB_ODR  (AT91_CAST(AT91_REG *) 	0xFFFFF614) // (PIOB) Output Disable Registerr
#define AT91C_PIOB_IFSR (AT91_CAST(AT91_REG *) 	0xFFFFF628) // (PIOB) Input Filter Status Register
#define AT91C_PIOB_PPUER (AT91_CAST(AT91_REG *) 	0xFFFFF664) // (PIOB) Pull-up Enable Register
#define AT91C_PIOB_SODR (AT91_CAST(AT91_REG *) 	0xFFFFF630) // (PIOB) Set Output Data Register
#define AT91C_PIOB_ISR  (AT91_CAST(AT91_REG *) 	0xFFFFF64C) // (PIOB) Interrupt Status Register
#define AT91C_PIOB_ODSR (AT91_CAST(AT91_REG *) 	0xFFFFF638) // (PIOB) Output Data Status Register
#define AT91C_PIOB_OSR  (AT91_CAST(AT91_REG *) 	0xFFFFF618) // (PIOB) Output Status Register
#define AT91C_PIOB_MDSR (AT91_CAST(AT91_REG *) 	0xFFFFF658) // (PIOB) Multi-driver Status Register
#define AT91C_PIOB_IFER (AT91_CAST(AT91_REG *) 	0xFFFFF620) // (PIOB) Input Filter Enable Register
#define AT91C_PIOB_BSR  (AT91_CAST(AT91_REG *) 	0xFFFFF674) // (PIOB) Select B Register
#define AT91C_PIOB_MDDR (AT91_CAST(AT91_REG *) 	0xFFFFF654) // (PIOB) Multi-driver Disable Register
#define AT91C_PIOB_OER  (AT91_CAST(AT91_REG *) 	0xFFFFF610) // (PIOB) Output Enable Register
#define AT91C_PIOB_PER  (AT91_CAST(AT91_REG *) 	0xFFFFF600) // (PIOB) PIO Enable Register
// ========== Register definition for CKGR peripheral ==========
#define AT91C_CKGR_MOR  (AT91_CAST(AT91_REG *) 	0xFFFFFC20) // (CKGR) Main Oscillator Register
#define AT91C_CKGR_PLLR (AT91_CAST(AT91_REG *) 	0xFFFFFC2C) // (CKGR) PLL Register
#define AT91C_CKGR_MCFR (AT91_CAST(AT91_REG *) 	0xFFFFFC24) // (CKGR) Main Clock  Frequency Register
// ========== Register definition for PMC peripheral ==========
#define AT91C_PMC_IDR   (AT91_CAST(AT91_REG *) 	0xFFFFFC64) // (PMC) Interrupt Disable Register
#define AT91C_PMC_MOR   (AT91_CAST(AT91_REG *) 	0xFFFFFC20) // (PMC) Main Oscillator Register
#define AT91C_PMC_PLLR  (AT91_CAST(AT91_REG *) 	0xFFFFFC2C) // (PMC) PLL Register
#define AT91C_PMC_PCER  (AT91_CAST(AT91_REG *) 	0xFFFFFC10) // (PMC) Peripheral Clock Enable Register
#define AT91C_PMC_PCKR  (AT91_CAST(AT91_REG *) 	0xFFFFFC40) // (PMC) Programmable Clock Register
#define AT91C_PMC_MCKR  (AT91_CAST(AT91_REG *) 	0xFFFFFC30) // (PMC) Master Clock Register
#define AT91C_PMC_SCDR  (AT91_CAST(AT91_REG *) 	0xFFFFFC04) // (PMC) System Clock Disable Register
#define AT91C_PMC_PCDR  (AT91_CAST(AT91_REG *) 	0xFFFFFC14) // (PMC) Peripheral Clock Disable Register
#define AT91C_PMC_SCSR  (AT91_CAST(AT91_REG *) 	0xFFFFFC08) // (PMC) System Clock Status Register
#define AT91C_PMC_PCSR  (AT91_CAST(AT91_REG *) 	0xFFFFFC18) // (PMC) Peripheral Clock Status Register
#define AT91C_PMC_MCFR  (AT91_CAST(AT91_REG *) 	0xFFFFFC24) // (PMC) Main Clock  Frequency Register
#define AT91C_PMC_SCER  (AT91_CAST(AT91_REG *) 	0xFFFFFC00) // (PMC) System Clock Enable Register
#define AT91C_PMC_IMR   (AT91_CAST(AT91_REG *) 	0xFFFFFC6C) // (PMC) Interrupt Mask Register
#define AT91C_PMC_IER   (AT91_CAST(AT91_REG *) 	0xFFFFFC60) // (PMC) Interrupt Enable Register
#define AT91C_PMC_SR    (AT91_CAST(AT91_REG *) 	0xFFFFFC68) // (PMC) Status Register
// ========== Register definition for RSTC peripheral ==========
#define AT91C_RSTC_RCR  (AT91_CAST(AT91_REG *) 	0xFFFFFD00) // (RSTC) Reset Control Register
#define AT91C_RSTC_RMR  (AT91_CAST(AT91_REG *) 	0xFFFFFD08) // (RSTC) Reset Mode Register
#define AT91C_RSTC_RSR  (AT91_CAST(AT91_REG *) 	0xFFFFFD04) // (RSTC) Reset Status Register
// ========== Register definition for SHDWC peripheral ==========
#define AT91C_SHDWC_SHSR (AT91_CAST(AT91_REG *) 	0xFFFFFD18) // (SHDWC) Shut Down Status Register
#define AT91C_SHDWC_SHMR (AT91_CAST(AT91_REG *) 	0xFFFFFD14) // (SHDWC) Shut Down Mode Register
#define AT91C_SHDWC_SHCR (AT91_CAST(AT91_REG *) 	0xFFFFFD10) // (SHDWC) Shut Down Control Register
// ========== Register definition for RTTC peripheral ==========
#define AT91C_RTTC_RTSR (AT91_CAST(AT91_REG *) 	0xFFFFFD2C) // (RTTC) Real-time Status Register
#define AT91C_RTTC_RTMR (AT91_CAST(AT91_REG *) 	0xFFFFFD20) // (RTTC) Real-time Mode Register
#define AT91C_RTTC_RTVR (AT91_CAST(AT91_REG *) 	0xFFFFFD28) // (RTTC) Real-time Value Register
#define AT91C_RTTC_RTAR (AT91_CAST(AT91_REG *) 	0xFFFFFD24) // (RTTC) Real-time Alarm Register
// ========== Register definition for PITC peripheral ==========
#define AT91C_PITC_PIVR (AT91_CAST(AT91_REG *) 	0xFFFFFD38) // (PITC) Period Interval Value Register
#define AT91C_PITC_PISR (AT91_CAST(AT91_REG *) 	0xFFFFFD34) // (PITC) Period Interval Status Register
#define AT91C_PITC_PIIR (AT91_CAST(AT91_REG *) 	0xFFFFFD3C) // (PITC) Period Interval Image Register
#define AT91C_PITC_PIMR (AT91_CAST(AT91_REG *) 	0xFFFFFD30) // (PITC) Period Interval Mode Register
// ========== Register definition for WDTC peripheral ==========
#define AT91C_WDTC_WDCR (AT91_CAST(AT91_REG *) 	0xFFFFFD40) // (WDTC) Watchdog Control Register
#define AT91C_WDTC_WDSR (AT91_CAST(AT91_REG *) 	0xFFFFFD48) // (WDTC) Watchdog Status Register
#define AT91C_WDTC_WDMR (AT91_CAST(AT91_REG *) 	0xFFFFFD44) // (WDTC) Watchdog Mode Register
// ========== Register definition for MC peripheral ==========
#define AT91C_MC_PUIA   (AT91_CAST(AT91_REG *) 	0xFFFFFF10) // (MC) MC Protection Unit Area
#define AT91C_MC_FMR    (AT91_CAST(AT91_REG *) 	0xFFFFFF60) // (MC) MC Flash Mode Register
#define AT91C_MC_RCR    (AT91_CAST(AT91_REG *) 	0xFFFFFF00) // (MC) MC Remap Control Register
#define AT91C_MC_ASR    (AT91_CAST(AT91_REG *) 	0xFFFFFF04) // (MC) MC Abort Status Register
#define AT91C_MC_PUP    (AT91_CAST(AT91_REG *) 	0xFFFFFF50) // (MC) MC Protection Unit Peripherals
#define AT91C_MC_AASR   (AT91_CAST(AT91_REG *) 	0xFFFFFF08) // (MC) MC Abort Address Status Register
#define AT91C_MC_FCR    (AT91_CAST(AT91_REG *) 	0xFFFFFF64) // (MC) MC Flash Command Register
#define AT91C_MC_FSR    (AT91_CAST(AT91_REG *) 	0xFFFFFF68) // (MC) MC Flash Status Register
#define AT91C_MC_PUER   (AT91_CAST(AT91_REG *) 	0xFFFFFF54) // (MC) MC Protection Unit Enable Register
// ========== Register definition for CAN0_MB0 peripheral ==========
#define AT91C_CAN0_MB0_MDH (AT91_CAST(AT91_REG *) 	0xFFF80218) // (CAN0_MB0) MailBox Data High Register
#define AT91C_CAN0_MB0_MDL (AT91_CAST(AT91_REG *) 	0xFFF80214) // (CAN0_MB0) MailBox Data Low Register
#define AT91C_CAN0_MB0_MAM (AT91_CAST(AT91_REG *) 	0xFFF80204) // (CAN0_MB0) MailBox Acceptance Mask Register
#define AT91C_CAN0_MB0_MCR (AT91_CAST(AT91_REG *) 	0xFFF8021C) // (CAN0_MB0) MailBox Control Register
#define AT91C_CAN0_MB0_MMR (AT91_CAST(AT91_REG *) 	0xFFF80200) // (CAN0_MB0) MailBox Mode Register
#define AT91C_CAN0_MB0_MID (AT91_CAST(AT91_REG *) 	0xFFF80208) // (CAN0_MB0) MailBox ID Register
#define AT91C_CAN0_MB0_MFID (AT91_CAST(AT91_REG *) 	0xFFF8020C) // (CAN0_MB0) MailBox Family ID Register
#define AT91C_CAN0_MB0_MSR (AT91_CAST(AT91_REG *) 	0xFFF80210) // (CAN0_MB0) MailBox Status Register
// ========== Register definition for CAN0_MB1 peripheral ==========
#define AT91C_CAN0_MB1_MSR (AT91_CAST(AT91_REG *) 	0xFFF80230) // (CAN0_MB1) MailBox Status Register
#define AT91C_CAN0_MB1_MDH (AT91_CAST(AT91_REG *) 	0xFFF80238) // (CAN0_MB1) MailBox Data High Register
#define AT91C_CAN0_MB1_MFID (AT91_CAST(AT91_REG *) 	0xFFF8022C) // (CAN0_MB1) MailBox Family ID Register
#define AT91C_CAN0_MB1_MAM (AT91_CAST(AT91_REG *) 	0xFFF80224) // (CAN0_MB1) MailBox Acceptance Mask Register
#define AT91C_CAN0_MB1_MDL (AT91_CAST(AT91_REG *) 	0xFFF80234) // (CAN0_MB1) MailBox Data Low Register
#define AT91C_CAN0_MB1_MMR (AT91_CAST(AT91_REG *) 	0xFFF80220) // (CAN0_MB1) MailBox Mode Register
#define AT91C_CAN0_MB1_MID (AT91_CAST(AT91_REG *) 	0xFFF80228) // (CAN0_MB1) MailBox ID Register
#define AT91C_CAN0_MB1_MCR (AT91_CAST(AT91_REG *) 	0xFFF8023C) // (CAN0_MB1) MailBox Control Register
// ========== Register definition for CAN0_MB2 peripheral ==========
#define AT91C_CAN0_MB2_MCR (AT91_CAST(AT91_REG *) 	0xFFF8025C) // (CAN0_MB2) MailBox Control Register
#define AT91C_CAN0_MB2_MFID (AT91_CAST(AT91_REG *) 	0xFFF8024C) // (CAN0_MB2) MailBox Family ID Register
#define AT91C_CAN0_MB2_MID (AT91_CAST(AT91_REG *) 	0xFFF80248) // (CAN0_MB2) MailBox ID Register
#define AT91C_CAN0_MB2_MMR (AT91_CAST(AT91_REG *) 	0xFFF80240) // (CAN0_MB2) MailBox Mode Register
#define AT91C_CAN0_MB2_MAM (AT91_CAST(AT91_REG *) 	0xFFF80244) // (CAN0_MB2) MailBox Acceptance Mask Register
#define AT91C_CAN0_MB2_MDH (AT91_CAST(AT91_REG *) 	0xFFF80258) // (CAN0_MB2) MailBox Data High Register
#define AT91C_CAN0_MB2_MSR (AT91_CAST(AT91_REG *) 	0xFFF80250) // (CAN0_MB2) MailBox Status Register
#define AT91C_CAN0_MB2_MDL (AT91_CAST(AT91_REG *) 	0xFFF80254) // (CAN0_MB2) MailBox Data Low Register
// ========== Register definition for CAN0_MB3 peripheral ==========
#define AT91C_CAN0_MB3_MMR (AT91_CAST(AT91_REG *) 	0xFFF80260) // (CAN0_MB3) MailBox Mode Register
#define AT91C_CAN0_MB3_MCR (AT91_CAST(AT91_REG *) 	0xFFF8027C) // (CAN0_MB3) MailBox Control Register
#define AT91C_CAN0_MB3_MDH (AT91_CAST(AT91_REG *) 	0xFFF80278) // (CAN0_MB3) MailBox Data High Register
#define AT91C_CAN0_MB3_MDL (AT91_CAST(AT91_REG *) 	0xFFF80274) // (CAN0_MB3) MailBox Data Low Register
#define AT91C_CAN0_MB3_MAM (AT91_CAST(AT91_REG *) 	0xFFF80264) // (CAN0_MB3) MailBox Acceptance Mask Register
#define AT91C_CAN0_MB3_MSR (AT91_CAST(AT91_REG *) 	0xFFF80270) // (CAN0_MB3) MailBox Status Register
#define AT91C_CAN0_MB3_MFID (AT91_CAST(AT91_REG *) 	0xFFF8026C) // (CAN0_MB3) MailBox Family ID Register
#define AT91C_CAN0_MB3_MID (AT91_CAST(AT91_REG *) 	0xFFF80268) // (CAN0_MB3) MailBox ID Register
// ========== Register definition for CAN0_MB4 peripheral ==========
#define AT91C_CAN0_MB4_MFID (AT91_CAST(AT91_REG *) 	0xFFF8028C) // (CAN0_MB4) MailBox Family ID Register
#define AT91C_CAN0_MB4_MID (AT91_CAST(AT91_REG *) 	0xFFF80288) // (CAN0_MB4) MailBox ID Register
#define AT91C_CAN0_MB4_MDH (AT91_CAST(AT91_REG *) 	0xFFF80298) // (CAN0_MB4) MailBox Data High Register
#define AT91C_CAN0_MB4_MDL (AT91_CAST(AT91_REG *) 	0xFFF80294) // (CAN0_MB4) MailBox Data Low Register
#define AT91C_CAN0_MB4_MAM (AT91_CAST(AT91_REG *) 	0xFFF80284) // (CAN0_MB4) MailBox Acceptance Mask Register
#define AT91C_CAN0_MB4_MCR (AT91_CAST(AT91_REG *) 	0xFFF8029C) // (CAN0_MB4) MailBox Control Register
#define AT91C_CAN0_MB4_MSR (AT91_CAST(AT91_REG *) 	0xFFF80290) // (CAN0_MB4) MailBox Status Register
#define AT91C_CAN0_MB4_MMR (AT91_CAST(AT91_REG *) 	0xFFF80280) // (CAN0_MB4) MailBox Mode Register
// ========== Register definition for CAN0_MB5 peripheral ==========
#define AT91C_CAN0_MB5_MSR (AT91_CAST(AT91_REG *) 	0xFFF802B0) // (CAN0_MB5) MailBox Status Register
#define AT91C_CAN0_MB5_MFID (AT91_CAST(AT91_REG *) 	0xFFF802AC) // (CAN0_MB5) MailBox Family ID Register
#define AT91C_CAN0_MB5_MAM (AT91_CAST(AT91_REG *) 	0xFFF802A4) // (CAN0_MB5) MailBox Acceptance Mask Register
#define AT91C_CAN0_MB5_MDL (AT91_CAST(AT91_REG *) 	0xFFF802B4) // (CAN0_MB5) MailBox Data Low Register
#define AT91C_CAN0_MB5_MDH (AT91_CAST(AT91_REG *) 	0xFFF802B8) // (CAN0_MB5) MailBox Data High Register
#define AT91C_CAN0_MB5_MID (AT91_CAST(AT91_REG *) 	0xFFF802A8) // (CAN0_MB5) MailBox ID Register
#define AT91C_CAN0_MB5_MMR (AT91_CAST(AT91_REG *) 	0xFFF802A0) // (CAN0_MB5) MailBox Mode Register
#define AT91C_CAN0_MB5_MCR (AT91_CAST(AT91_REG *) 	0xFFF802BC) // (CAN0_MB5) MailBox Control Register
// ========== Register definition for CAN0_MB6 peripheral ==========
#define AT91C_CAN0_MB6_MAM (AT91_CAST(AT91_REG *) 	0xFFF802C4) // (CAN0_MB6) MailBox Acceptance Mask Register
#define AT91C_CAN0_MB6_MID (AT91_CAST(AT91_REG *) 	0xFFF802C8) // (CAN0_MB6) MailBox ID Register
#define AT91C_CAN0_MB6_MDL (AT91_CAST(AT91_REG *) 	0xFFF802D4) // (CAN0_MB6) MailBox Data Low Register
#define AT91C_CAN0_MB6_MDH (AT91_CAST(AT91_REG *) 	0xFFF802D8) // (CAN0_MB6) MailBox Data High Register
#define AT91C_CAN0_MB6_MCR (AT91_CAST(AT91_REG *) 	0xFFF802DC) // (CAN0_MB6) MailBox Control Register
#define AT91C_CAN0_MB6_MMR (AT91_CAST(AT91_REG *) 	0xFFF802C0) // (CAN0_MB6) MailBox Mode Register
#define AT91C_CAN0_MB6_MFID (AT91_CAST(AT91_REG *) 	0xFFF802CC) // (CAN0_MB6) MailBox Family ID Register
#define AT91C_CAN0_MB6_MSR (AT91_CAST(AT91_REG *) 	0xFFF802D0) // (CAN0_MB6) MailBox Status Register
// ========== Register definition for CAN0_MB7 peripheral ==========
#define AT91C_CAN0_MB7_MSR (AT91_CAST(AT91_REG *) 	0xFFF802F0) // (CAN0_MB7) MailBox Status Register
#define AT91C_CAN0_MB7_MMR (AT91_CAST(AT91_REG *) 	0xFFF802E0) // (CAN0_MB7) MailBox Mode Register
#define AT91C_CAN0_MB7_MCR (AT91_CAST(AT91_REG *) 	0xFFF802FC) // (CAN0_MB7) MailBox Control Register
#define AT91C_CAN0_MB7_MDL (AT91_CAST(AT91_REG *) 	0xFFF802F4) // (CAN0_MB7) MailBox Data Low Register
#define AT91C_CAN0_MB7_MID (AT91_CAST(AT91_REG *) 	0xFFF802E8) // (CAN0_MB7) MailBox ID Register
#define AT91C_CAN0_MB7_MDH (AT91_CAST(AT91_REG *) 	0xFFF802F8) // (CAN0_MB7) MailBox Data High Register
#define AT91C_CAN0_MB7_MFID (AT91_CAST(AT91_REG *) 	0xFFF802EC) // (CAN0_MB7) MailBox Family ID Register
#define AT91C_CAN0_MB7_MAM (AT91_CAST(AT91_REG *) 	0xFFF802E4) // (CAN0_MB7) MailBox Acceptance Mask Register
// ========== Register definition for CAN0_MB8 peripheral ==========
#define AT91C_CAN0_MB8_MAM (AT91_CAST(AT91_REG *) 	0xFFF80304) // (CAN0_MB8) MailBox Acceptance Mask Register
#define AT91C_CAN0_MB8_MCR (AT91_CAST(AT91_REG *) 	0xFFF8031C) // (CAN0_MB8) MailBox Control Register
#define AT91C_CAN0_MB8_MSR (AT91_CAST(AT91_REG *) 	0xFFF80310) // (CAN0_MB8) MailBox Status Register
#define AT91C_CAN0_MB8_MID (AT91_CAST(AT91_REG *) 	0xFFF80308) // (CAN0_MB8) MailBox ID Register
#define AT91C_CAN0_MB8_MDH (AT91_CAST(AT91_REG *) 	0xFFF80318) // (CAN0_MB8) MailBox Data High Register
#define AT91C_CAN0_MB8_MFID (AT91_CAST(AT91_REG *) 	0xFFF8030C) // (CAN0_MB8) MailBox Family ID Register
#define AT91C_CAN0_MB8_MMR (AT91_CAST(AT91_REG *) 	0xFFF80300) // (CAN0_MB8) MailBox Mode Register
#define AT91C_CAN0_MB8_MDL (AT91_CAST(AT91_REG *) 	0xFFF80314) // (CAN0_MB8) MailBox Data Low Register
// ========== Register definition for CAN0_MB9 peripheral ==========
#define AT91C_CAN0_MB9_MMR (AT91_CAST(AT91_REG *) 	0xFFF80320) // (CAN0_MB9) MailBox Mode Register
#define AT91C_CAN0_MB9_MDH (AT91_CAST(AT91_REG *) 	0xFFF80338) // (CAN0_MB9) MailBox Data High Register
#define AT91C_CAN0_MB9_MSR (AT91_CAST(AT91_REG *) 	0xFFF80330) // (CAN0_MB9) MailBox Status Register
#define AT91C_CAN0_MB9_MDL (AT91_CAST(AT91_REG *) 	0xFFF80334) // (CAN0_MB9) MailBox Data Low Register
#define AT91C_CAN0_MB9_MID (AT91_CAST(AT91_REG *) 	0xFFF80328) // (CAN0_MB9) MailBox ID Register
#define AT91C_CAN0_MB9_MFID (AT91_CAST(AT91_REG *) 	0xFFF8032C) // (CAN0_MB9) MailBox Family ID Register
#define AT91C_CAN0_MB9_MCR (AT91_CAST(AT91_REG *) 	0xFFF8033C) // (CAN0_MB9) MailBox Control Register
#define AT91C_CAN0_MB9_MAM (AT91_CAST(AT91_REG *) 	0xFFF80324) // (CAN0_MB9) MailBox Acceptance Mask Register
// ========== Register definition for CAN0_MB10 peripheral ==========
#define AT91C_CAN0_MB10_MCR (AT91_CAST(AT91_REG *) 	0xFFF8035C) // (CAN0_MB10) MailBox Control Register
#define AT91C_CAN0_MB10_MID (AT91_CAST(AT91_REG *) 	0xFFF80348) // (CAN0_MB10) MailBox ID Register
#define AT91C_CAN0_MB10_MAM (AT91_CAST(AT91_REG *) 	0xFFF80344) // (CAN0_MB10) MailBox Acceptance Mask Register
#define AT91C_CAN0_MB10_MFID (AT91_CAST(AT91_REG *) 	0xFFF8034C) // (CAN0_MB10) MailBox Family ID Register
#define AT91C_CAN0_MB10_MDL (AT91_CAST(AT91_REG *) 	0xFFF80354) // (CAN0_MB10) MailBox Data Low Register
#define AT91C_CAN0_MB10_MMR (AT91_CAST(AT91_REG *) 	0xFFF80340) // (CAN0_MB10) MailBox Mode Register
#define AT91C_CAN0_MB10_MDH (AT91_CAST(AT91_REG *) 	0xFFF80358) // (CAN0_MB10) MailBox Data High Register
#define AT91C_CAN0_MB10_MSR (AT91_CAST(AT91_REG *) 	0xFFF80350) // (CAN0_MB10) MailBox Status Register
// ========== Register definition for CAN0_MB11 peripheral ==========
#define AT91C_CAN0_MB11_MCR (AT91_CAST(AT91_REG *) 	0xFFF8037C) // (CAN0_MB11) MailBox Control Register
#define AT91C_CAN0_MB11_MFID (AT91_CAST(AT91_REG *) 	0xFFF8036C) // (CAN0_MB11) MailBox Family ID Register
#define AT91C_CAN0_MB11_MDH (AT91_CAST(AT91_REG *) 	0xFFF80378) // (CAN0_MB11) MailBox Data High Register
#define AT91C_CAN0_MB11_MAM (AT91_CAST(AT91_REG *) 	0xFFF80364) // (CAN0_MB11) MailBox Acceptance Mask Register
#define AT91C_CAN0_MB11_MID (AT91_CAST(AT91_REG *) 	0xFFF80368) // (CAN0_MB11) MailBox ID Register
#define AT91C_CAN0_MB11_MMR (AT91_CAST(AT91_REG *) 	0xFFF80360) // (CAN0_MB11) MailBox Mode Register
#define AT91C_CAN0_MB11_MSR (AT91_CAST(AT91_REG *) 	0xFFF80370) // (CAN0_MB11) MailBox Status Register
#define AT91C_CAN0_MB11_MDL (AT91_CAST(AT91_REG *) 	0xFFF80374) // (CAN0_MB11) MailBox Data Low Register
// ========== Register definition for CAN0_MB12 peripheral ==========
#define AT91C_CAN0_MB12_MCR (AT91_CAST(AT91_REG *) 	0xFFF8039C) // (CAN0_MB12) MailBox Control Register
#define AT91C_CAN0_MB12_MID (AT91_CAST(AT91_REG *) 	0xFFF80388) // (CAN0_MB12) MailBox ID Register
#define AT91C_CAN0_MB12_MDH (AT91_CAST(AT91_REG *) 	0xFFF80398) // (CAN0_MB12) MailBox Data High Register
#define AT91C_CAN0_MB12_MAM (AT91_CAST(AT91_REG *) 	0xFFF80384) // (CAN0_MB12) MailBox Acceptance Mask Register
#define AT91C_CAN0_MB12_MFID (AT91_CAST(AT91_REG *) 	0xFFF8038C) // (CAN0_MB12) MailBox Family ID Register
#define AT91C_CAN0_MB12_MDL (AT91_CAST(AT91_REG *) 	0xFFF80394) // (CAN0_MB12) MailBox Data Low Register
#define AT91C_CAN0_MB12_MMR (AT91_CAST(AT91_REG *) 	0xFFF80380) // (CAN0_MB12) MailBox Mode Register
#define AT91C_CAN0_MB12_MSR (AT91_CAST(AT91_REG *) 	0xFFF80390) // (CAN0_MB12) MailBox Status Register
// ========== Register definition for CAN0_MB13 peripheral ==========
#define AT91C_CAN0_MB13_MCR (AT91_CAST(AT91_REG *) 	0xFFF803BC) // (CAN0_MB13) MailBox Control Register
#define AT91C_CAN0_MB13_MDL (AT91_CAST(AT91_REG *) 	0xFFF803B4) // (CAN0_MB13) MailBox Data Low Register
#define AT91C_CAN0_MB13_MAM (AT91_CAST(AT91_REG *) 	0xFFF803A4) // (CAN0_MB13) MailBox Acceptance Mask Register
#define AT91C_CAN0_MB13_MFID (AT91_CAST(AT91_REG *) 	0xFFF803AC) // (CAN0_MB13) MailBox Family ID Register
#define AT91C_CAN0_MB13_MDH (AT91_CAST(AT91_REG *) 	0xFFF803B8) // (CAN0_MB13) MailBox Data High Register
#define AT91C_CAN0_MB13_MID (AT91_CAST(AT91_REG *) 	0xFFF803A8) // (CAN0_MB13) MailBox ID Register
#define AT91C_CAN0_MB13_MSR (AT91_CAST(AT91_REG *) 	0xFFF803B0) // (CAN0_MB13) MailBox Status Register
#define AT91C_CAN0_MB13_MMR (AT91_CAST(AT91_REG *) 	0xFFF803A0) // (CAN0_MB13) MailBox Mode Register
// ========== Register definition for CAN0_MB14 peripheral ==========
#define AT91C_CAN0_MB14_MSR (AT91_CAST(AT91_REG *) 	0xFFF803D0) // (CAN0_MB14) MailBox Status Register
#define AT91C_CAN0_MB14_MMR (AT91_CAST(AT91_REG *) 	0xFFF803C0) // (CAN0_MB14) MailBox Mode Register
#define AT91C_CAN0_MB14_MDL (AT91_CAST(AT91_REG *) 	0xFFF803D4) // (CAN0_MB14) MailBox Data Low Register
#define AT91C_CAN0_MB14_MDH (AT91_CAST(AT91_REG *) 	0xFFF803D8) // (CAN0_MB14) MailBox Data High Register
#define AT91C_CAN0_MB14_MID (AT91_CAST(AT91_REG *) 	0xFFF803C8) // (CAN0_MB14) MailBox ID Register
#define AT91C_CAN0_MB14_MCR (AT91_CAST(AT91_REG *) 	0xFFF803DC) // (CAN0_MB14) MailBox Control Register
#define AT91C_CAN0_MB14_MFID (AT91_CAST(AT91_REG *) 	0xFFF803CC) // (CAN0_MB14) MailBox Family ID Register
#define AT91C_CAN0_MB14_MAM (AT91_CAST(AT91_REG *) 	0xFFF803C4) // (CAN0_MB14) MailBox Acceptance Mask Register
// ========== Register definition for CAN0_MB15 peripheral ==========
#define AT91C_CAN0_MB15_MDH (AT91_CAST(AT91_REG *) 	0xFFF803F8) // (CAN0_MB15) MailBox Data High Register
#define AT91C_CAN0_MB15_MMR (AT91_CAST(AT91_REG *) 	0xFFF803E0) // (CAN0_MB15) MailBox Mode Register
#define AT91C_CAN0_MB15_MCR (AT91_CAST(AT91_REG *) 	0xFFF803FC) // (CAN0_MB15) MailBox Control Register
#define AT91C_CAN0_MB15_MAM (AT91_CAST(AT91_REG *) 	0xFFF803E4) // (CAN0_MB15) MailBox Acceptance Mask Register
#define AT91C_CAN0_MB15_MID (AT91_CAST(AT91_REG *) 	0xFFF803E8) // (CAN0_MB15) MailBox ID Register
#define AT91C_CAN0_MB15_MFID (AT91_CAST(AT91_REG *) 	0xFFF803EC) // (CAN0_MB15) MailBox Family ID Register
#define AT91C_CAN0_MB15_MSR (AT91_CAST(AT91_REG *) 	0xFFF803F0) // (CAN0_MB15) MailBox Status Register
#define AT91C_CAN0_MB15_MDL (AT91_CAST(AT91_REG *) 	0xFFF803F4) // (CAN0_MB15) MailBox Data Low Register
// ========== Register definition for CAN0 peripheral ==========
#define AT91C_CAN0_BR   (AT91_CAST(AT91_REG *) 	0xFFF80014) // (CAN0) Baudrate Register
#define AT91C_CAN0_TIMESTP (AT91_CAST(AT91_REG *) 	0xFFF8001C) // (CAN0) Time Stamp Register
#define AT91C_CAN0_IER  (AT91_CAST(AT91_REG *) 	0xFFF80004) // (CAN0) Interrupt Enable Register
#define AT91C_CAN0_MR   (AT91_CAST(AT91_REG *) 	0xFFF80000) // (CAN0) Mode Register
#define AT91C_CAN0_TCR  (AT91_CAST(AT91_REG *) 	0xFFF80024) // (CAN0) Transfer Command Register
#define AT91C_CAN0_ACR  (AT91_CAST(AT91_REG *) 	0xFFF80028) // (CAN0) Abort Command Register
#define AT91C_CAN0_IDR  (AT91_CAST(AT91_REG *) 	0xFFF80008) // (CAN0) Interrupt Disable Register
#define AT91C_CAN0_IMR  (AT91_CAST(AT91_REG *) 	0xFFF8000C) // (CAN0) Interrupt Mask Register
#define AT91C_CAN0_TIM  (AT91_CAST(AT91_REG *) 	0xFFF80018) // (CAN0) Timer Register
#define AT91C_CAN0_VR   (AT91_CAST(AT91_REG *) 	0xFFF800FC) // (CAN0) Version Register
#define AT91C_CAN0_ECR  (AT91_CAST(AT91_REG *) 	0xFFF80020) // (CAN0) Error Counter Register
#define AT91C_CAN0_SR   (AT91_CAST(AT91_REG *) 	0xFFF80010) // (CAN0) Status Register
// ========== Register definition for CAN1_MB0 peripheral ==========
#define AT91C_CAN1_MB0_MFID (AT91_CAST(AT91_REG *) 	0xFFF8420C) // (CAN1_MB0) MailBox Family ID Register
#define AT91C_CAN1_MB0_MDL (AT91_CAST(AT91_REG *) 	0xFFF84214) // (CAN1_MB0) MailBox Data Low Register
#define AT91C_CAN1_MB0_MID (AT91_CAST(AT91_REG *) 	0xFFF84208) // (CAN1_MB0) MailBox ID Register
#define AT91C_CAN1_MB0_MMR (AT91_CAST(AT91_REG *) 	0xFFF84200) // (CAN1_MB0) MailBox Mode Register
#define AT91C_CAN1_MB0_MAM (AT91_CAST(AT91_REG *) 	0xFFF84204) // (CAN1_MB0) MailBox Acceptance Mask Register
#define AT91C_CAN1_MB0_MSR (AT91_CAST(AT91_REG *) 	0xFFF84210) // (CAN1_MB0) MailBox Status Register
#define AT91C_CAN1_MB0_MDH (AT91_CAST(AT91_REG *) 	0xFFF84218) // (CAN1_MB0) MailBox Data High Register
#define AT91C_CAN1_MB0_MCR (AT91_CAST(AT91_REG *) 	0xFFF8421C) // (CAN1_MB0) MailBox Control Register
// ========== Register definition for CAN1_MB1 peripheral ==========
#define AT91C_CAN1_MB1_MFID (AT91_CAST(AT91_REG *) 	0xFFF8422C) // (CAN1_MB1) MailBox Family ID Register
#define AT91C_CAN1_MB1_MSR (AT91_CAST(AT91_REG *) 	0xFFF84230) // (CAN1_MB1) MailBox Status Register
#define AT91C_CAN1_MB1_MMR (AT91_CAST(AT91_REG *) 	0xFFF84220) // (CAN1_MB1) MailBox Mode Register
#define AT91C_CAN1_MB1_MDH (AT91_CAST(AT91_REG *) 	0xFFF84238) // (CAN1_MB1) MailBox Data High Register
#define AT91C_CAN1_MB1_MID (AT91_CAST(AT91_REG *) 	0xFFF84228) // (CAN1_MB1) MailBox ID Register
#define AT91C_CAN1_MB1_MDL (AT91_CAST(AT91_REG *) 	0xFFF84234) // (CAN1_MB1) MailBox Data Low Register
#define AT91C_CAN1_MB1_MCR (AT91_CAST(AT91_REG *) 	0xFFF8423C) // (CAN1_MB1) MailBox Control Register
#define AT91C_CAN1_MB1_MAM (AT91_CAST(AT91_REG *) 	0xFFF84224) // (CAN1_MB1) MailBox Acceptance Mask Register
// ========== Register definition for CAN1_MB2 peripheral ==========
#define AT91C_CAN1_MB2_MSR (AT91_CAST(AT91_REG *) 	0xFFF84250) // (CAN1_MB2) MailBox Status Register
#define AT91C_CAN1_MB2_MMR (AT91_CAST(AT91_REG *) 	0xFFF84240) // (CAN1_MB2) MailBox Mode Register
#define AT91C_CAN1_MB2_MAM (AT91_CAST(AT91_REG *) 	0xFFF84244) // (CAN1_MB2) MailBox Acceptance Mask Register
#define AT91C_CAN1_MB2_MID (AT91_CAST(AT91_REG *) 	0xFFF84248) // (CAN1_MB2) MailBox ID Register
#define AT91C_CAN1_MB2_MFID (AT91_CAST(AT91_REG *) 	0xFFF8424C) // (CAN1_MB2) MailBox Family ID Register
#define AT91C_CAN1_MB2_MDL (AT91_CAST(AT91_REG *) 	0xFFF84254) // (CAN1_MB2) MailBox Data Low Register
#define AT91C_CAN1_MB2_MDH (AT91_CAST(AT91_REG *) 	0xFFF84258) // (CAN1_MB2) MailBox Data High Register
#define AT91C_CAN1_MB2_MCR (AT91_CAST(AT91_REG *) 	0xFFF8425C) // (CAN1_MB2) MailBox Control Register
// ========== Register definition for CAN1_MB3 peripheral ==========
#define AT91C_CAN1_MB3_MCR (AT91_CAST(AT91_REG *) 	0xFFF8427C) // (CAN1_MB3) MailBox Control Register
#define AT91C_CAN1_MB3_MDH (AT91_CAST(AT91_REG *) 	0xFFF84278) // (CAN1_MB3) MailBox Data High Register
#define AT91C_CAN1_MB3_MAM (AT91_CAST(AT91_REG *) 	0xFFF84264) // (CAN1_MB3) MailBox Acceptance Mask Register
#define AT91C_CAN1_MB3_MDL (AT91_CAST(AT91_REG *) 	0xFFF84274) // (CAN1_MB3) MailBox Data Low Register
#define AT91C_CAN1_MB3_MMR (AT91_CAST(AT91_REG *) 	0xFFF84260) // (CAN1_MB3) MailBox Mode Register
#define AT91C_CAN1_MB3_MSR (AT91_CAST(AT91_REG *) 	0xFFF84270) // (CAN1_MB3) MailBox Status Register
#define AT91C_CAN1_MB3_MFID (AT91_CAST(AT91_REG *) 	0xFFF8426C) // (CAN1_MB3) MailBox Family ID Register
#define AT91C_CAN1_MB3_MID (AT91_CAST(AT91_REG *) 	0xFFF84268) // (CAN1_MB3) MailBox ID Register
// ========== Register definition for CAN1_MB4 peripheral ==========
#define AT91C_CAN1_MB4_MCR (AT91_CAST(AT91_REG *) 	0xFFF8429C) // (CAN1_MB4) MailBox Control Register
#define AT91C_CAN1_MB4_MDH (AT91_CAST(AT91_REG *) 	0xFFF84298) // (CAN1_MB4) MailBox Data High Register
#define AT91C_CAN1_MB4_MID (AT91_CAST(AT91_REG *) 	0xFFF84288) // (CAN1_MB4) MailBox ID Register
#define AT91C_CAN1_MB4_MDL (AT91_CAST(AT91_REG *) 	0xFFF84294) // (CAN1_MB4) MailBox Data Low Register
#define AT91C_CAN1_MB4_MFID (AT91_CAST(AT91_REG *) 	0xFFF8428C) // (CAN1_MB4) MailBox Family ID Register
#define AT91C_CAN1_MB4_MAM (AT91_CAST(AT91_REG *) 	0xFFF84284) // (CAN1_MB4) MailBox Acceptance Mask Register
#define AT91C_CAN1_MB4_MSR (AT91_CAST(AT91_REG *) 	0xFFF84290) // (CAN1_MB4) MailBox Status Register
#define AT91C_CAN1_MB4_MMR (AT91_CAST(AT91_REG *) 	0xFFF84280) // (CAN1_MB4) MailBox Mode Register
// ========== Register definition for CAN1_MB5 peripheral ==========
#define AT91C_CAN1_MB5_MCR (AT91_CAST(AT91_REG *) 	0xFFF842BC) // (CAN1_MB5) MailBox Control Register
#define AT91C_CAN1_MB5_MSR (AT91_CAST(AT91_REG *) 	0xFFF842B0) // (CAN1_MB5) MailBox Status Register
#define AT91C_CAN1_MB5_MMR (AT91_CAST(AT91_REG *) 	0xFFF842A0) // (CAN1_MB5) MailBox Mode Register
#define AT91C_CAN1_MB5_MDL (AT91_CAST(AT91_REG *) 	0xFFF842B4) // (CAN1_MB5) MailBox Data Low Register
#define AT91C_CAN1_MB5_MAM (AT91_CAST(AT91_REG *) 	0xFFF842A4) // (CAN1_MB5) MailBox Acceptance Mask Register
#define AT91C_CAN1_MB5_MID (AT91_CAST(AT91_REG *) 	0xFFF842A8) // (CAN1_MB5) MailBox ID Register
#define AT91C_CAN1_MB5_MDH (AT91_CAST(AT91_REG *) 	0xFFF842B8) // (CAN1_MB5) MailBox Data High Register
#define AT91C_CAN1_MB5_MFID (AT91_CAST(AT91_REG *) 	0xFFF842AC) // (CAN1_MB5) MailBox Family ID Register
// ========== Register definition for CAN1_MB6 peripheral ==========
#define AT91C_CAN1_MB6_MDH (AT91_CAST(AT91_REG *) 	0xFFF842D8) // (CAN1_MB6) MailBox Data High Register
#define AT91C_CAN1_MB6_MDL (AT91_CAST(AT91_REG *) 	0xFFF842D4) // (CAN1_MB6) MailBox Data Low Register
#define AT91C_CAN1_MB6_MAM (AT91_CAST(AT91_REG *) 	0xFFF842C4) // (CAN1_MB6) MailBox Acceptance Mask Register
#define AT91C_CAN1_MB6_MCR (AT91_CAST(AT91_REG *) 	0xFFF842DC) // (CAN1_MB6) MailBox Control Register
#define AT91C_CAN1_MB6_MMR (AT91_CAST(AT91_REG *) 	0xFFF842C0) // (CAN1_MB6) MailBox Mode Register
#define AT91C_CAN1_MB6_MID (AT91_CAST(AT91_REG *) 	0xFFF842C8) // (CAN1_MB6) MailBox ID Register
#define AT91C_CAN1_MB6_MSR (AT91_CAST(AT91_REG *) 	0xFFF842D0) // (CAN1_MB6) MailBox Status Register
#define AT91C_CAN1_MB6_MFID (AT91_CAST(AT91_REG *) 	0xFFF842CC) // (CAN1_MB6) MailBox Family ID Register
// ========== Register definition for CAN1_MB7 peripheral ==========
#define AT91C_CAN1_MB7_MDH (AT91_CAST(AT91_REG *) 	0xFFF842F8) // (CAN1_MB7) MailBox Data High Register
#define AT91C_CAN1_MB7_MDL (AT91_CAST(AT91_REG *) 	0xFFF842F4) // (CAN1_MB7) MailBox Data Low Register
#define AT91C_CAN1_MB7_MID (AT91_CAST(AT91_REG *) 	0xFFF842E8) // (CAN1_MB7) MailBox ID Register
#define AT91C_CAN1_MB7_MSR (AT91_CAST(AT91_REG *) 	0xFFF842F0) // (CAN1_MB7) MailBox Status Register
#define AT91C_CAN1_MB7_MFID (AT91_CAST(AT91_REG *) 	0xFFF842EC) // (CAN1_MB7) MailBox Family ID Register
#define AT91C_CAN1_MB7_MAM (AT91_CAST(AT91_REG *) 	0xFFF842E4) // (CAN1_MB7) MailBox Acceptance Mask Register
#define AT91C_CAN1_MB7_MMR (AT91_CAST(AT91_REG *) 	0xFFF842E0) // (CAN1_MB7) MailBox Mode Register
#define AT91C_CAN1_MB7_MCR (AT91_CAST(AT91_REG *) 	0xFFF842FC) // (CAN1_MB7) MailBox Control Register
// ========== Register definition for CAN1_MB8 peripheral ==========
#define AT91C_CAN1_MB8_MCR (AT91_CAST(AT91_REG *) 	0xFFF8431C) // (CAN1_MB8) MailBox Control Register
#define AT91C_CAN1_MB8_MFID (AT91_CAST(AT91_REG *) 	0xFFF8430C) // (CAN1_MB8) MailBox Family ID Register
#define AT91C_CAN1_MB8_MSR (AT91_CAST(AT91_REG *) 	0xFFF84310) // (CAN1_MB8) MailBox Status Register
#define AT91C_CAN1_MB8_MAM (AT91_CAST(AT91_REG *) 	0xFFF84304) // (CAN1_MB8) MailBox Acceptance Mask Register
#define AT91C_CAN1_MB8_MDL (AT91_CAST(AT91_REG *) 	0xFFF84314) // (CAN1_MB8) MailBox Data Low Register
#define AT91C_CAN1_MB8_MID (AT91_CAST(AT91_REG *) 	0xFFF84308) // (CAN1_MB8) MailBox ID Register
#define AT91C_CAN1_MB8_MDH (AT91_CAST(AT91_REG *) 	0xFFF84318) // (CAN1_MB8) MailBox Data High Register
#define AT91C_CAN1_MB8_MMR (AT91_CAST(AT91_REG *) 	0xFFF84300) // (CAN1_MB8) MailBox Mode Register
// ========== Register definition for CAN1_MB9 peripheral ==========
#define AT91C_CAN1_MB9_MDH (AT91_CAST(AT91_REG *) 	0xFFF84338) // (CAN1_MB9) MailBox Data High Register
#define AT91C_CAN1_MB9_MDL (AT91_CAST(AT91_REG *) 	0xFFF84334) // (CAN1_MB9) MailBox Data Low Register
#define AT91C_CAN1_MB9_MFID (AT91_CAST(AT91_REG *) 	0xFFF8432C) // (CAN1_MB9) MailBox Family ID Register
#define AT91C_CAN1_MB9_MMR (AT91_CAST(AT91_REG *) 	0xFFF84320) // (CAN1_MB9) MailBox Mode Register
#define AT91C_CAN1_MB9_MAM (AT91_CAST(AT91_REG *) 	0xFFF84324) // (CAN1_MB9) MailBox Acceptance Mask Register
#define AT91C_CAN1_MB9_MID (AT91_CAST(AT91_REG *) 	0xFFF84328) // (CAN1_MB9) MailBox ID Register
#define AT91C_CAN1_MB9_MCR (AT91_CAST(AT91_REG *) 	0xFFF8433C) // (CAN1_MB9) MailBox Control Register
#define AT91C_CAN1_MB9_MSR (AT91_CAST(AT91_REG *) 	0xFFF84330) // (CAN1_MB9) MailBox Status Register
// ========== Register definition for CAN1_MB10 peripheral ==========
#define AT91C_CAN1_MB10_MFID (AT91_CAST(AT91_REG *) 	0xFFF8434C) // (CAN1_MB10) MailBox Family ID Register
#define AT91C_CAN1_MB10_MSR (AT91_CAST(AT91_REG *) 	0xFFF84350) // (CAN1_MB10) MailBox Status Register
#define AT91C_CAN1_MB10_MDL (AT91_CAST(AT91_REG *) 	0xFFF84354) // (CAN1_MB10) MailBox Data Low Register
#define AT91C_CAN1_MB10_MMR (AT91_CAST(AT91_REG *) 	0xFFF84340) // (CAN1_MB10) MailBox Mode Register
#define AT91C_CAN1_MB10_MCR (AT91_CAST(AT91_REG *) 	0xFFF8435C) // (CAN1_MB10) MailBox Control Register
#define AT91C_CAN1_MB10_MAM (AT91_CAST(AT91_REG *) 	0xFFF84344) // (CAN1_MB10) MailBox Acceptance Mask Register
#define AT91C_CAN1_MB10_MID (AT91_CAST(AT91_REG *) 	0xFFF84348) // (CAN1_MB10) MailBox ID Register
#define AT91C_CAN1_MB10_MDH (AT91_CAST(AT91_REG *) 	0xFFF84358) // (CAN1_MB10) MailBox Data High Register
// ========== Register definition for CAN1_MB11 peripheral ==========
#define AT91C_CAN1_MB11_MMR (AT91_CAST(AT91_REG *) 	0xFFF84360) // (CAN1_MB11) MailBox Mode Register
#define AT91C_CAN1_MB11_MDL (AT91_CAST(AT91_REG *) 	0xFFF84374) // (CAN1_MB11) MailBox Data Low Register
#define AT91C_CAN1_MB11_MAM (AT91_CAST(AT91_REG *) 	0xFFF84364) // (CAN1_MB11) MailBox Acceptance Mask Register
#define AT91C_CAN1_MB11_MID (AT91_CAST(AT91_REG *) 	0xFFF84368) // (CAN1_MB11) MailBox ID Register
#define AT91C_CAN1_MB11_MCR (AT91_CAST(AT91_REG *) 	0xFFF8437C) // (CAN1_MB11) MailBox Control Register
#define AT91C_CAN1_MB11_MDH (AT91_CAST(AT91_REG *) 	0xFFF84378) // (CAN1_MB11) MailBox Data High Register
#define AT91C_CAN1_MB11_MSR (AT91_CAST(AT91_REG *) 	0xFFF84370) // (CAN1_MB11) MailBox Status Register
#define AT91C_CAN1_MB11_MFID (AT91_CAST(AT91_REG *) 	0xFFF8436C) // (CAN1_MB11) MailBox Family ID Register
// ========== Register definition for CAN1_MB12 peripheral ==========
#define AT91C_CAN1_MB12_MFID (AT91_CAST(AT91_REG *) 	0xFFF8438C) // (CAN1_MB12) MailBox Family ID Register
#define AT91C_CAN1_MB12_MAM (AT91_CAST(AT91_REG *) 	0xFFF84384) // (CAN1_MB12) MailBox Acceptance Mask Register
#define AT91C_CAN1_MB12_MDH (AT91_CAST(AT91_REG *) 	0xFFF84398) // (CAN1_MB12) MailBox Data High Register
#define AT91C_CAN1_MB12_MMR (AT91_CAST(AT91_REG *) 	0xFFF84380) // (CAN1_MB12) MailBox Mode Register
#define AT91C_CAN1_MB12_MID (AT91_CAST(AT91_REG *) 	0xFFF84388) // (CAN1_MB12) MailBox ID Register
#define AT91C_CAN1_MB12_MCR (AT91_CAST(AT91_REG *) 	0xFFF8439C) // (CAN1_MB12) MailBox Control Register
#define AT91C_CAN1_MB12_MDL (AT91_CAST(AT91_REG *) 	0xFFF84394) // (CAN1_MB12) MailBox Data Low Register
#define AT91C_CAN1_MB12_MSR (AT91_CAST(AT91_REG *) 	0xFFF84390) // (CAN1_MB12) MailBox Status Register
// ========== Register definition for CAN1_MB13 peripheral ==========
#define AT91C_CAN1_MB13_MDL (AT91_CAST(AT91_REG *) 	0xFFF843B4) // (CAN1_MB13) MailBox Data Low Register
#define AT91C_CAN1_MB13_MSR (AT91_CAST(AT91_REG *) 	0xFFF843B0) // (CAN1_MB13) MailBox Status Register
#define AT91C_CAN1_MB13_MFID (AT91_CAST(AT91_REG *) 	0xFFF843AC) // (CAN1_MB13) MailBox Family ID Register
#define AT91C_CAN1_MB13_MAM (AT91_CAST(AT91_REG *) 	0xFFF843A4) // (CAN1_MB13) MailBox Acceptance Mask Register
#define AT91C_CAN1_MB13_MMR (AT91_CAST(AT91_REG *) 	0xFFF843A0) // (CAN1_MB13) MailBox Mode Register
#define AT91C_CAN1_MB13_MCR (AT91_CAST(AT91_REG *) 	0xFFF843BC) // (CAN1_MB13) MailBox Control Register
#define AT91C_CAN1_MB13_MDH (AT91_CAST(AT91_REG *) 	0xFFF843B8) // (CAN1_MB13) MailBox Data High Register
#define AT91C_CAN1_MB13_MID (AT91_CAST(AT91_REG *) 	0xFFF843A8) // (CAN1_MB13) MailBox ID Register
// ========== Register definition for CAN1_MB14 peripheral ==========
#define AT91C_CAN1_MB14_MCR (AT91_CAST(AT91_REG *) 	0xFFF843DC) // (CAN1_MB14) MailBox Control Register
#define AT91C_CAN1_MB14_MID (AT91_CAST(AT91_REG *) 	0xFFF843C8) // (CAN1_MB14) MailBox ID Register
#define AT91C_CAN1_MB14_MMR (AT91_CAST(AT91_REG *) 	0xFFF843C0) // (CAN1_MB14) MailBox Mode Register
#define AT91C_CAN1_MB14_MDH (AT91_CAST(AT91_REG *) 	0xFFF843D8) // (CAN1_MB14) MailBox Data High Register
#define AT91C_CAN1_MB14_MSR (AT91_CAST(AT91_REG *) 	0xFFF843D0) // (CAN1_MB14) MailBox Status Register
#define AT91C_CAN1_MB14_MFID (AT91_CAST(AT91_REG *) 	0xFFF843CC) // (CAN1_MB14) MailBox Family ID Register
#define AT91C_CAN1_MB14_MDL (AT91_CAST(AT91_REG *) 	0xFFF843D4) // (CAN1_MB14) MailBox Data Low Register
#define AT91C_CAN1_MB14_MAM (AT91_CAST(AT91_REG *) 	0xFFF843C4) // (CAN1_MB14) MailBox Acceptance Mask Register
// ========== Register definition for CAN1_MB15 peripheral ==========
#define AT91C_CAN1_MB15_MSR (AT91_CAST(AT91_REG *) 	0xFFF843F0) // (CAN1_MB15) MailBox Status Register
#define AT91C_CAN1_MB15_MDL (AT91_CAST(AT91_REG *) 	0xFFF843F4) // (CAN1_MB15) MailBox Data Low Register
#define AT91C_CAN1_MB15_MDH (AT91_CAST(AT91_REG *) 	0xFFF843F8) // (CAN1_MB15) MailBox Data High Register
#define AT91C_CAN1_MB15_MMR (AT91_CAST(AT91_REG *) 	0xFFF843E0) // (CAN1_MB15) MailBox Mode Register
#define AT91C_CAN1_MB15_MAM (AT91_CAST(AT91_REG *) 	0xFFF843E4) // (CAN1_MB15) MailBox Acceptance Mask Register
#define AT91C_CAN1_MB15_MFID (AT91_CAST(AT91_REG *) 	0xFFF843EC) // (CAN1_MB15) MailBox Family ID Register
#define AT91C_CAN1_MB15_MCR (AT91_CAST(AT91_REG *) 	0xFFF843FC) // (CAN1_MB15) MailBox Control Register
#define AT91C_CAN1_MB15_MID (AT91_CAST(AT91_REG *) 	0xFFF843E8) // (CAN1_MB15) MailBox ID Register
// ========== Register definition for CAN1 peripheral ==========
#define AT91C_CAN1_ECR  (AT91_CAST(AT91_REG *) 	0xFFF84020) // (CAN1) Error Counter Register
#define AT91C_CAN1_BR   (AT91_CAST(AT91_REG *) 	0xFFF84014) // (CAN1) Baudrate Register
#define AT91C_CAN1_IDR  (AT91_CAST(AT91_REG *) 	0xFFF84008) // (CAN1) Interrupt Disable Register
#define AT91C_CAN1_ACR  (AT91_CAST(AT91_REG *) 	0xFFF84028) // (CAN1) Abort Command Register
#define AT91C_CAN1_IMR  (AT91_CAST(AT91_REG *) 	0xFFF8400C) // (CAN1) Interrupt Mask Register
#define AT91C_CAN1_TCR  (AT91_CAST(AT91_REG *) 	0xFFF84024) // (CAN1) Transfer Command Register
#define AT91C_CAN1_SR   (AT91_CAST(AT91_REG *) 	0xFFF84010) // (CAN1) Status Register
#define AT91C_CAN1_TIM  (AT91_CAST(AT91_REG *) 	0xFFF84018) // (CAN1) Timer Register
#define AT91C_CAN1_VR   (AT91_CAST(AT91_REG *) 	0xFFF840FC) // (CAN1) Version Register
#define AT91C_CAN1_MR   (AT91_CAST(AT91_REG *) 	0xFFF84000) // (CAN1) Mode Register
#define AT91C_CAN1_IER  (AT91_CAST(AT91_REG *) 	0xFFF84004) // (CAN1) Interrupt Enable Register
#define AT91C_CAN1_TIMESTP (AT91_CAST(AT91_REG *) 	0xFFF8401C) // (CAN1) Time Stamp Register
// ========== Register definition for TC0 peripheral ==========
#define AT91C_TC0_SR    (AT91_CAST(AT91_REG *) 	0xFFFA0020) // (TC0) Status Register
#define AT91C_TC0_RC    (AT91_CAST(AT91_REG *) 	0xFFFA001C) // (TC0) Register C
#define AT91C_TC0_RB    (AT91_CAST(AT91_REG *) 	0xFFFA0018) // (TC0) Register B
#define AT91C_TC0_CCR   (AT91_CAST(AT91_REG *) 	0xFFFA0000) // (TC0) Channel Control Register
#define AT91C_TC0_CMR   (AT91_CAST(AT91_REG *) 	0xFFFA0004) // (TC0) Channel Mode Register (Capture Mode / Waveform Mode)
#define AT91C_TC0_IER   (AT91_CAST(AT91_REG *) 	0xFFFA0024) // (TC0) Interrupt Enable Register
#define AT91C_TC0_RA    (AT91_CAST(AT91_REG *) 	0xFFFA0014) // (TC0) Register A
#define AT91C_TC0_IDR   (AT91_CAST(AT91_REG *) 	0xFFFA0028) // (TC0) Interrupt Disable Register
#define AT91C_TC0_CV    (AT91_CAST(AT91_REG *) 	0xFFFA0010) // (TC0) Counter Value
#define AT91C_TC0_IMR   (AT91_CAST(AT91_REG *) 	0xFFFA002C) // (TC0) Interrupt Mask Register
// ========== Register definition for TC1 peripheral ==========
#define AT91C_TC1_RB    (AT91_CAST(AT91_REG *) 	0xFFFA0058) // (TC1) Register B
#define AT91C_TC1_CCR   (AT91_CAST(AT91_REG *) 	0xFFFA0040) // (TC1) Channel Control Register
#define AT91C_TC1_IER   (AT91_CAST(AT91_REG *) 	0xFFFA0064) // (TC1) Interrupt Enable Register
#define AT91C_TC1_IDR   (AT91_CAST(AT91_REG *) 	0xFFFA0068) // (TC1) Interrupt Disable Register
#define AT91C_TC1_SR    (AT91_CAST(AT91_REG *) 	0xFFFA0060) // (TC1) Status Register
#define AT91C_TC1_CMR   (AT91_CAST(AT91_REG *) 	0xFFFA0044) // (TC1) Channel Mode Register (Capture Mode / Waveform Mode)
#define AT91C_TC1_RA    (AT91_CAST(AT91_REG *) 	0xFFFA0054) // (TC1) Register A
#define AT91C_TC1_RC    (AT91_CAST(AT91_REG *) 	0xFFFA005C) // (TC1) Register C
#define AT91C_TC1_IMR   (AT91_CAST(AT91_REG *) 	0xFFFA006C) // (TC1) Interrupt Mask Register
#define AT91C_TC1_CV    (AT91_CAST(AT91_REG *) 	0xFFFA0050) // (TC1) Counter Value
// ========== Register definition for TC2 peripheral ==========
#define AT91C_TC2_CMR   (AT91_CAST(AT91_REG *) 	0xFFFA0084) // (TC2) Channel Mode Register (Capture Mode / Waveform Mode)
#define AT91C_TC2_CCR   (AT91_CAST(AT91_REG *) 	0xFFFA0080) // (TC2) Channel Control Register
#define AT91C_TC2_CV    (AT91_CAST(AT91_REG *) 	0xFFFA0090) // (TC2) Counter Value
#define AT91C_TC2_RA    (AT91_CAST(AT91_REG *) 	0xFFFA0094) // (TC2) Register A
#define AT91C_TC2_RB    (AT91_CAST(AT91_REG *) 	0xFFFA0098) // (TC2) Register B
#define AT91C_TC2_IDR   (AT91_CAST(AT91_REG *) 	0xFFFA00A8) // (TC2) Interrupt Disable Register
#define AT91C_TC2_IMR   (AT91_CAST(AT91_REG *) 	0xFFFA00AC) // (TC2) Interrupt Mask Register
#define AT91C_TC2_RC    (AT91_CAST(AT91_REG *) 	0xFFFA009C) // (TC2) Register C
#define AT91C_TC2_IER   (AT91_CAST(AT91_REG *) 	0xFFFA00A4) // (TC2) Interrupt Enable Register
#define AT91C_TC2_SR    (AT91_CAST(AT91_REG *) 	0xFFFA00A0) // (TC2) Status Register
// ========== Register definition for TCB0 peripheral ==========
#define AT91C_TCB0_BMR  (AT91_CAST(AT91_REG *) 	0xFFFA00C4) // (TCB0) TC Block Mode Register
#define AT91C_TCB0_BCR  (AT91_CAST(AT91_REG *) 	0xFFFA00C0) // (TCB0) TC Block Control Register
// ========== Register definition for TC3 peripheral ==========
#define AT91C_TC3_IMR   (AT91_CAST(AT91_REG *) 	0xFFFA402C) // (TC3) Interrupt Mask Register
#define AT91C_TC3_CMR   (AT91_CAST(AT91_REG *) 	0xFFFA4004) // (TC3) Channel Mode Register (Capture Mode / Waveform Mode)
#define AT91C_TC3_IDR   (AT91_CAST(AT91_REG *) 	0xFFFA4028) // (TC3) Interrupt Disable Register
#define AT91C_TC3_CCR   (AT91_CAST(AT91_REG *) 	0xFFFA4000) // (TC3) Channel Control Register
#define AT91C_TC3_RA    (AT91_CAST(AT91_REG *) 	0xFFFA4014) // (TC3) Register A
#define AT91C_TC3_RB    (AT91_CAST(AT91_REG *) 	0xFFFA4018) // (TC3) Register B
#define AT91C_TC3_CV    (AT91_CAST(AT91_REG *) 	0xFFFA4010) // (TC3) Counter Value
#define AT91C_TC3_SR    (AT91_CAST(AT91_REG *) 	0xFFFA4020) // (TC3) Status Register
#define AT91C_TC3_IER   (AT91_CAST(AT91_REG *) 	0xFFFA4024) // (TC3) Interrupt Enable Register
#define AT91C_TC3_RC    (AT91_CAST(AT91_REG *) 	0xFFFA401C) // (TC3) Register C
// ========== Register definition for TC4 peripheral ==========
#define AT91C_TC4_SR    (AT91_CAST(AT91_REG *) 	0xFFFA4060) // (TC4) Status Register
#define AT91C_TC4_RA    (AT91_CAST(AT91_REG *) 	0xFFFA4054) // (TC4) Register A
#define AT91C_TC4_CV    (AT91_CAST(AT91_REG *) 	0xFFFA4050) // (TC4) Counter Value
#define AT91C_TC4_CMR   (AT91_CAST(AT91_REG *) 	0xFFFA4044) // (TC4) Channel Mode Register (Capture Mode / Waveform Mode)
#define AT91C_TC4_RB    (AT91_CAST(AT91_REG *) 	0xFFFA4058) // (TC4) Register B
#define AT91C_TC4_CCR   (AT91_CAST(AT91_REG *) 	0xFFFA4040) // (TC4) Channel Control Register
#define AT91C_TC4_IER   (AT91_CAST(AT91_REG *) 	0xFFFA4064) // (TC4) Interrupt Enable Register
#define AT91C_TC4_IMR   (AT91_CAST(AT91_REG *) 	0xFFFA406C) // (TC4) Interrupt Mask Register
#define AT91C_TC4_RC    (AT91_CAST(AT91_REG *) 	0xFFFA405C) // (TC4) Register C
#define AT91C_TC4_IDR   (AT91_CAST(AT91_REG *) 	0xFFFA4068) // (TC4) Interrupt Disable Register
// ========== Register definition for TC5 peripheral ==========
#define AT91C_TC5_CMR   (AT91_CAST(AT91_REG *) 	0xFFFA4084) // (TC5) Channel Mode Register (Capture Mode / Waveform Mode)
#define AT91C_TC5_IDR   (AT91_CAST(AT91_REG *) 	0xFFFA40A8) // (TC5) Interrupt Disable Register
#define AT91C_TC5_CCR   (AT91_CAST(AT91_REG *) 	0xFFFA4080) // (TC5) Channel Control Register
#define AT91C_TC5_RB    (AT91_CAST(AT91_REG *) 	0xFFFA4098) // (TC5) Register B
#define AT91C_TC5_IMR   (AT91_CAST(AT91_REG *) 	0xFFFA40AC) // (TC5) Interrupt Mask Register
#define AT91C_TC5_CV    (AT91_CAST(AT91_REG *) 	0xFFFA4090) // (TC5) Counter Value
#define AT91C_TC5_RC    (AT91_CAST(AT91_REG *) 	0xFFFA409C) // (TC5) Register C
#define AT91C_TC5_SR    (AT91_CAST(AT91_REG *) 	0xFFFA40A0) // (TC5) Status Register
#define AT91C_TC5_IER   (AT91_CAST(AT91_REG *) 	0xFFFA40A4) // (TC5) Interrupt Enable Register
#define AT91C_TC5_RA    (AT91_CAST(AT91_REG *) 	0xFFFA4094) // (TC5) Register A
// ========== Register definition for TCB1 peripheral ==========
#define AT91C_TCB1_BMR  (AT91_CAST(AT91_REG *) 	0xFFFA40C4) // (TCB1) TC Block Mode Register
#define AT91C_TCB1_BCR  (AT91_CAST(AT91_REG *) 	0xFFFA40C0) // (TCB1) TC Block Control Register
// ========== Register definition for TC6 peripheral ==========
#define AT91C_TC6_IDR   (AT91_CAST(AT91_REG *) 	0xFFFA8028) // (TC6) Interrupt Disable Register
#define AT91C_TC6_RA    (AT91_CAST(AT91_REG *) 	0xFFFA8014) // (TC6) Register A
#define AT91C_TC6_IER   (AT91_CAST(AT91_REG *) 	0xFFFA8024) // (TC6) Interrupt Enable Register
#define AT91C_TC6_RB    (AT91_CAST(AT91_REG *) 	0xFFFA8018) // (TC6) Register B
#define AT91C_TC6_CMR   (AT91_CAST(AT91_REG *) 	0xFFFA8004) // (TC6) Channel Mode Register (Capture Mode / Waveform Mode)
#define AT91C_TC6_CCR   (AT91_CAST(AT91_REG *) 	0xFFFA8000) // (TC6) Channel Control Register
#define AT91C_TC6_CV    (AT91_CAST(AT91_REG *) 	0xFFFA8010) // (TC6) Counter Value
#define AT91C_TC6_RC    (AT91_CAST(AT91_REG *) 	0xFFFA801C) // (TC6) Register C
#define AT91C_TC6_IMR   (AT91_CAST(AT91_REG *) 	0xFFFA802C) // (TC6) Interrupt Mask Register
#define AT91C_TC6_SR    (AT91_CAST(AT91_REG *) 	0xFFFA8020) // (TC6) Status Register
// ========== Register definition for TC7 peripheral ==========
#define AT91C_TC7_IMR   (AT91_CAST(AT91_REG *) 	0xFFFA806C) // (TC7) Interrupt Mask Register
#define AT91C_TC7_SR    (AT91_CAST(AT91_REG *) 	0xFFFA8060) // (TC7) Status Register
#define AT91C_TC7_IDR   (AT91_CAST(AT91_REG *) 	0xFFFA8068) // (TC7) Interrupt Disable Register
#define AT91C_TC7_CMR   (AT91_CAST(AT91_REG *) 	0xFFFA8044) // (TC7) Channel Mode Register (Capture Mode / Waveform Mode)
#define AT91C_TC7_CV    (AT91_CAST(AT91_REG *) 	0xFFFA8050) // (TC7) Counter Value
#define AT91C_TC7_RA    (AT91_CAST(AT91_REG *) 	0xFFFA8054) // (TC7) Register A
#define AT91C_TC7_RB    (AT91_CAST(AT91_REG *) 	0xFFFA8058) // (TC7) Register B
#define AT91C_TC7_RC    (AT91_CAST(AT91_REG *) 	0xFFFA805C) // (TC7) Register C
#define AT91C_TC7_CCR   (AT91_CAST(AT91_REG *) 	0xFFFA8040) // (TC7) Channel Control Register
#define AT91C_TC7_IER   (AT91_CAST(AT91_REG *) 	0xFFFA8064) // (TC7) Interrupt Enable Register
// ========== Register definition for TC8 peripheral ==========
#define AT91C_TC8_RA    (AT91_CAST(AT91_REG *) 	0xFFFA8094) // (TC8) Register A
#define AT91C_TC8_IDR   (AT91_CAST(AT91_REG *) 	0xFFFA80A8) // (TC8) Interrupt Disable Register
#define AT91C_TC8_RC    (AT91_CAST(AT91_REG *) 	0xFFFA809C) // (TC8) Register C
#define AT91C_TC8_CCR   (AT91_CAST(AT91_REG *) 	0xFFFA8080) // (TC8) Channel Control Register
#define AT91C_TC8_SR    (AT91_CAST(AT91_REG *) 	0xFFFA80A0) // (TC8) Status Register
#define AT91C_TC8_RB    (AT91_CAST(AT91_REG *) 	0xFFFA8098) // (TC8) Register B
#define AT91C_TC8_IMR   (AT91_CAST(AT91_REG *) 	0xFFFA80AC) // (TC8) Interrupt Mask Register
#define AT91C_TC8_CMR   (AT91_CAST(AT91_REG *) 	0xFFFA8084) // (TC8) Channel Mode Register (Capture Mode / Waveform Mode)
#define AT91C_TC8_IER   (AT91_CAST(AT91_REG *) 	0xFFFA80A4) // (TC8) Interrupt Enable Register
#define AT91C_TC8_CV    (AT91_CAST(AT91_REG *) 	0xFFFA8090) // (TC8) Counter Value
// ========== Register definition for TCB2 peripheral ==========
#define AT91C_TCB2_BMR  (AT91_CAST(AT91_REG *) 	0xFFFA80C4) // (TCB2) TC Block Mode Register
#define AT91C_TCB2_BCR  (AT91_CAST(AT91_REG *) 	0xFFFA80C0) // (TCB2) TC Block Control Register
// ========== Register definition for PDC_MCI peripheral ==========
#define AT91C_MCI_PTSR  (AT91_CAST(AT91_REG *) 	0xFFFAC124) // (PDC_MCI) PDC Transfer Status Register
#define AT91C_MCI_RPR   (AT91_CAST(AT91_REG *) 	0xFFFAC100) // (PDC_MCI) Receive Pointer Register
#define AT91C_MCI_RNCR  (AT91_CAST(AT91_REG *) 	0xFFFAC114) // (PDC_MCI) Receive Next Counter Register
#define AT91C_MCI_RCR   (AT91_CAST(AT91_REG *) 	0xFFFAC104) // (PDC_MCI) Receive Counter Register
#define AT91C_MCI_PTCR  (AT91_CAST(AT91_REG *) 	0xFFFAC120) // (PDC_MCI) PDC Transfer Control Register
#define AT91C_MCI_TPR   (AT91_CAST(AT91_REG *) 	0xFFFAC108) // (PDC_MCI) Transmit Pointer Register
#define AT91C_MCI_RNPR  (AT91_CAST(AT91_REG *) 	0xFFFAC110) // (PDC_MCI) Receive Next Pointer Register
#define AT91C_MCI_TNPR  (AT91_CAST(AT91_REG *) 	0xFFFAC118) // (PDC_MCI) Transmit Next Pointer Register
#define AT91C_MCI_TCR   (AT91_CAST(AT91_REG *) 	0xFFFAC10C) // (PDC_MCI) Transmit Counter Register
#define AT91C_MCI_TNCR  (AT91_CAST(AT91_REG *) 	0xFFFAC11C) // (PDC_MCI) Transmit Next Counter Register
// ========== Register definition for MCI peripheral ==========
#define AT91C_MCI_TDR   (AT91_CAST(AT91_REG *) 	0xFFFAC034) // (MCI) MCI Transmit Data Register
#define AT91C_MCI_IDR   (AT91_CAST(AT91_REG *) 	0xFFFAC048) // (MCI) MCI Interrupt Disable Register
#define AT91C_MCI_SR    (AT91_CAST(AT91_REG *) 	0xFFFAC040) // (MCI) MCI Status Register
#define AT91C_MCI_CMDR  (AT91_CAST(AT91_REG *) 	0xFFFAC014) // (MCI) MCI Command Register
#define AT91C_MCI_DTOR  (AT91_CAST(AT91_REG *) 	0xFFFAC008) // (MCI) MCI Data Timeout Register
#define AT91C_MCI_IER   (AT91_CAST(AT91_REG *) 	0xFFFAC044) // (MCI) MCI Interrupt Enable Register
#define AT91C_MCI_ARGR  (AT91_CAST(AT91_REG *) 	0xFFFAC010) // (MCI) MCI Argument Register
#define AT91C_MCI_SDCR  (AT91_CAST(AT91_REG *) 	0xFFFAC00C) // (MCI) MCI SD Card Register
#define AT91C_MCI_RDR   (AT91_CAST(AT91_REG *) 	0xFFFAC030) // (MCI) MCI Receive Data Register
#define AT91C_MCI_IMR   (AT91_CAST(AT91_REG *) 	0xFFFAC04C) // (MCI) MCI Interrupt Mask Register
#define AT91C_MCI_MR    (AT91_CAST(AT91_REG *) 	0xFFFAC004) // (MCI) MCI Mode Register
#define AT91C_MCI_RSPR  (AT91_CAST(AT91_REG *) 	0xFFFAC020) // (MCI) MCI Response Register
#define AT91C_MCI_CR    (AT91_CAST(AT91_REG *) 	0xFFFAC000) // (MCI) MCI Control Register
// ========== Register definition for UDP peripheral ==========
#define AT91C_UDP_IMR   (AT91_CAST(AT91_REG *) 	0xFFFB0018) // (UDP) Interrupt Mask Register
#define AT91C_UDP_FADDR (AT91_CAST(AT91_REG *) 	0xFFFB0008) // (UDP) Function Address Register
#define AT91C_UDP_NUM   (AT91_CAST(AT91_REG *) 	0xFFFB0000) // (UDP) Frame Number Register
#define AT91C_UDP_FDR   (AT91_CAST(AT91_REG *) 	0xFFFB0050) // (UDP) Endpoint FIFO Data Register
#define AT91C_UDP_ISR   (AT91_CAST(AT91_REG *) 	0xFFFB001C) // (UDP) Interrupt Status Register
#define AT91C_UDP_CSR   (AT91_CAST(AT91_REG *) 	0xFFFB0030) // (UDP) Endpoint Control and Status Register
#define AT91C_UDP_IDR   (AT91_CAST(AT91_REG *) 	0xFFFB0014) // (UDP) Interrupt Disable Register
#define AT91C_UDP_ICR   (AT91_CAST(AT91_REG *) 	0xFFFB0020) // (UDP) Interrupt Clear Register
#define AT91C_UDP_RSTEP (AT91_CAST(AT91_REG *) 	0xFFFB0028) // (UDP) Reset Endpoint Register
#define AT91C_UDP_TXVC  (AT91_CAST(AT91_REG *) 	0xFFFB0074) // (UDP) Transceiver Control Register
#define AT91C_UDP_GLBSTATE (AT91_CAST(AT91_REG *) 	0xFFFB0004) // (UDP) Global State Register
#define AT91C_UDP_IER   (AT91_CAST(AT91_REG *) 	0xFFFB0010) // (UDP) Interrupt Enable Register
// ========== Register definition for TWI peripheral ==========
#define AT91C_TWI_IER   (AT91_CAST(AT91_REG *) 	0xFFFB8024) // (TWI) Interrupt Enable Register
#define AT91C_TWI_CR    (AT91_CAST(AT91_REG *) 	0xFFFB8000) // (TWI) Control Register
#define AT91C_TWI_SR    (AT91_CAST(AT91_REG *) 	0xFFFB8020) // (TWI) Status Register
#define AT91C_TWI_IMR   (AT91_CAST(AT91_REG *) 	0xFFFB802C) // (TWI) Interrupt Mask Register
#define AT91C_TWI_THR   (AT91_CAST(AT91_REG *) 	0xFFFB8034) // (TWI) Transmit Holding Register
#define AT91C_TWI_IDR   (AT91_CAST(AT91_REG *) 	0xFFFB8028) // (TWI) Interrupt Disable Register
#define AT91C_TWI_IADR  (AT91_CAST(AT91_REG *) 	0xFFFB800C) // (TWI) Internal Address Register
#define AT91C_TWI_MMR   (AT91_CAST(AT91_REG *) 	0xFFFB8004) // (TWI) Master Mode Register
#define AT91C_TWI_CWGR  (AT91_CAST(AT91_REG *) 	0xFFFB8010) // (TWI) Clock Waveform Generator Register
#define AT91C_TWI_RHR   (AT91_CAST(AT91_REG *) 	0xFFFB8030) // (TWI) Receive Holding Register
// ========== Register definition for PDC_US0 peripheral ==========
#define AT91C_US0_TNPR  (AT91_CAST(AT91_REG *) 	0xFFFC0118) // (PDC_US0) Transmit Next Pointer Register
#define AT91C_US0_RNPR  (AT91_CAST(AT91_REG *) 	0xFFFC0110) // (PDC_US0) Receive Next Pointer Register
#define AT91C_US0_TCR   (AT91_CAST(AT91_REG *) 	0xFFFC010C) // (PDC_US0) Transmit Counter Register
#define AT91C_US0_PTCR  (AT91_CAST(AT91_REG *) 	0xFFFC0120) // (PDC_US0) PDC Transfer Control Register
#define AT91C_US0_PTSR  (AT91_CAST(AT91_REG *) 	0xFFFC0124) // (PDC_US0) PDC Transfer Status Register
#define AT91C_US0_TNCR  (AT91_CAST(AT91_REG *) 	0xFFFC011C) // (PDC_US0) Transmit Next Counter Register
#define AT91C_US0_TPR   (AT91_CAST(AT91_REG *) 	0xFFFC0108) // (PDC_US0) Transmit Pointer Register
#define AT91C_US0_RCR   (AT91_CAST(AT91_REG *) 	0xFFFC0104) // (PDC_US0) Receive Counter Register
#define AT91C_US0_RPR   (AT91_CAST(AT91_REG *) 	0xFFFC0100) // (PDC_US0) Receive Pointer Register
#define AT91C_US0_RNCR  (AT91_CAST(AT91_REG *) 	0xFFFC0114) // (PDC_US0) Receive Next Counter Register
// ========== Register definition for US0 peripheral ==========
#define AT91C_US0_BRGR  (AT91_CAST(AT91_REG *) 	0xFFFC0020) // (US0) Baud Rate Generator Register
#define AT91C_US0_NER   (AT91_CAST(AT91_REG *) 	0xFFFC0044) // (US0) Nb Errors Register
#define AT91C_US0_CR    (AT91_CAST(AT91_REG *) 	0xFFFC0000) // (US0) Control Register
#define AT91C_US0_IMR   (AT91_CAST(AT91_REG *) 	0xFFFC0010) // (US0) Interrupt Mask Register
#define AT91C_US0_FIDI  (AT91_CAST(AT91_REG *) 	0xFFFC0040) // (US0) FI_DI_Ratio Register
#define AT91C_US0_TTGR  (AT91_CAST(AT91_REG *) 	0xFFFC0028) // (US0) Transmitter Time-guard Register
#define AT91C_US0_MR    (AT91_CAST(AT91_REG *) 	0xFFFC0004) // (US0) Mode Register
#define AT91C_US0_RTOR  (AT91_CAST(AT91_REG *) 	0xFFFC0024) // (US0) Receiver Time-out Register
#define AT91C_US0_CSR   (AT91_CAST(AT91_REG *) 	0xFFFC0014) // (US0) Channel Status Register
#define AT91C_US0_RHR   (AT91_CAST(AT91_REG *) 	0xFFFC0018) // (US0) Receiver Holding Register
#define AT91C_US0_IDR   (AT91_CAST(AT91_REG *) 	0xFFFC000C) // (US0) Interrupt Disable Register
#define AT91C_US0_THR   (AT91_CAST(AT91_REG *) 	0xFFFC001C) // (US0) Transmitter Holding Register
#define AT91C_US0_IF    (AT91_CAST(AT91_REG *) 	0xFFFC004C) // (US0) IRDA_FILTER Register
#define AT91C_US0_IER   (AT91_CAST(AT91_REG *) 	0xFFFC0008) // (US0) Interrupt Enable Register
// ========== Register definition for PDC_US1 peripheral ==========
#define AT91C_US1_RNCR  (AT91_CAST(AT91_REG *) 	0xFFFC4114) // (PDC_US1) Receive Next Counter Register
#define AT91C_US1_PTCR  (AT91_CAST(AT91_REG *) 	0xFFFC4120) // (PDC_US1) PDC Transfer Control Register
#define AT91C_US1_TCR   (AT91_CAST(AT91_REG *) 	0xFFFC410C) // (PDC_US1) Transmit Counter Register
#define AT91C_US1_PTSR  (AT91_CAST(AT91_REG *) 	0xFFFC4124) // (PDC_US1) PDC Transfer Status Register
#define AT91C_US1_TNPR  (AT91_CAST(AT91_REG *) 	0xFFFC4118) // (PDC_US1) Transmit Next Pointer Register
#define AT91C_US1_RCR   (AT91_CAST(AT91_REG *) 	0xFFFC4104) // (PDC_US1) Receive Counter Register
#define AT91C_US1_RNPR  (AT91_CAST(AT91_REG *) 	0xFFFC4110) // (PDC_US1) Receive Next Pointer Register
#define AT91C_US1_RPR   (AT91_CAST(AT91_REG *) 	0xFFFC4100) // (PDC_US1) Receive Pointer Register
#define AT91C_US1_TNCR  (AT91_CAST(AT91_REG *) 	0xFFFC411C) // (PDC_US1) Transmit Next Counter Register
#define AT91C_US1_TPR   (AT91_CAST(AT91_REG *) 	0xFFFC4108) // (PDC_US1) Transmit Pointer Register
// ========== Register definition for US1 peripheral ==========
#define AT91C_US1_IF    (AT91_CAST(AT91_REG *) 	0xFFFC404C) // (US1) IRDA_FILTER Register
#define AT91C_US1_NER   (AT91_CAST(AT91_REG *) 	0xFFFC4044) // (US1) Nb Errors Register
#define AT91C_US1_RTOR  (AT91_CAST(AT91_REG *) 	0xFFFC4024) // (US1) Receiver Time-out Register
#define AT91C_US1_CSR   (AT91_CAST(AT91_REG *) 	0xFFFC4014) // (US1) Channel Status Register
#define AT91C_US1_IDR   (AT91_CAST(AT91_REG *) 	0xFFFC400C) // (US1) Interrupt Disable Register
#define AT91C_US1_IER   (AT91_CAST(AT91_REG *) 	0xFFFC4008) // (US1) Interrupt Enable Register
#define AT91C_US1_THR   (AT91_CAST(AT91_REG *) 	0xFFFC401C) // (US1) Transmitter Holding Register
#define AT91C_US1_TTGR  (AT91_CAST(AT91_REG *) 	0xFFFC4028) // (US1) Transmitter Time-guard Register
#define AT91C_US1_RHR   (AT91_CAST(AT91_REG *) 	0xFFFC4018) // (US1) Receiver Holding Register
#define AT91C_US1_BRGR  (AT91_CAST(AT91_REG *) 	0xFFFC4020) // (US1) Baud Rate Generator Register
#define AT91C_US1_IMR   (AT91_CAST(AT91_REG *) 	0xFFFC4010) // (US1) Interrupt Mask Register
#define AT91C_US1_FIDI  (AT91_CAST(AT91_REG *) 	0xFFFC4040) // (US1) FI_DI_Ratio Register
#define AT91C_US1_CR    (AT91_CAST(AT91_REG *) 	0xFFFC4000) // (US1) Control Register
#define AT91C_US1_MR    (AT91_CAST(AT91_REG *) 	0xFFFC4004) // (US1) Mode Register
// ========== Register definition for PDC_US2 peripheral ==========
#define AT91C_US2_PTCR  (AT91_CAST(AT91_REG *) 	0xFFFC8120) // (PDC_US2) PDC Transfer Control Register
#define AT91C_US2_TCR   (AT91_CAST(AT91_REG *) 	0xFFFC810C) // (PDC_US2) Transmit Counter Register
#define AT91C_US2_RPR   (AT91_CAST(AT91_REG *) 	0xFFFC8100) // (PDC_US2) Receive Pointer Register
#define AT91C_US2_TPR   (AT91_CAST(AT91_REG *) 	0xFFFC8108) // (PDC_US2) Transmit Pointer Register
#define AT91C_US2_PTSR  (AT91_CAST(AT91_REG *) 	0xFFFC8124) // (PDC_US2) PDC Transfer Status Register
#define AT91C_US2_RNCR  (AT91_CAST(AT91_REG *) 	0xFFFC8114) // (PDC_US2) Receive Next Counter Register
#define AT91C_US2_TNPR  (AT91_CAST(AT91_REG *) 	0xFFFC8118) // (PDC_US2) Transmit Next Pointer Register
#define AT91C_US2_RCR   (AT91_CAST(AT91_REG *) 	0xFFFC8104) // (PDC_US2) Receive Counter Register
#define AT91C_US2_RNPR  (AT91_CAST(AT91_REG *) 	0xFFFC8110) // (PDC_US2) Receive Next Pointer Register
#define AT91C_US2_TNCR  (AT91_CAST(AT91_REG *) 	0xFFFC811C) // (PDC_US2) Transmit Next Counter Register
// ========== Register definition for US2 peripheral ==========
#define AT91C_US2_RHR   (AT91_CAST(AT91_REG *) 	0xFFFC8018) // (US2) Receiver Holding Register
#define AT91C_US2_BRGR  (AT91_CAST(AT91_REG *) 	0xFFFC8020) // (US2) Baud Rate Generator Register
#define AT91C_US2_IF    (AT91_CAST(AT91_REG *) 	0xFFFC804C) // (US2) IRDA_FILTER Register
#define AT91C_US2_IDR   (AT91_CAST(AT91_REG *) 	0xFFFC800C) // (US2) Interrupt Disable Register
#define AT91C_US2_IMR   (AT91_CAST(AT91_REG *) 	0xFFFC8010) // (US2) Interrupt Mask Register
#define AT91C_US2_CR    (AT91_CAST(AT91_REG *) 	0xFFFC8000) // (US2) Control Register
#define AT91C_US2_IER   (AT91_CAST(AT91_REG *) 	0xFFFC8008) // (US2) Interrupt Enable Register
#define AT91C_US2_NER   (AT91_CAST(AT91_REG *) 	0xFFFC8044) // (US2) Nb Errors Register
#define AT91C_US2_RTOR  (AT91_CAST(AT91_REG *) 	0xFFFC8024) // (US2) Receiver Time-out Register
#define AT91C_US2_TTGR  (AT91_CAST(AT91_REG *) 	0xFFFC8028) // (US2) Transmitter Time-guard Register
#define AT91C_US2_MR    (AT91_CAST(AT91_REG *) 	0xFFFC8004) // (US2) Mode Register
#define AT91C_US2_CSR   (AT91_CAST(AT91_REG *) 	0xFFFC8014) // (US2) Channel Status Register
#define AT91C_US2_THR   (AT91_CAST(AT91_REG *) 	0xFFFC801C) // (US2) Transmitter Holding Register
#define AT91C_US2_FIDI  (AT91_CAST(AT91_REG *) 	0xFFFC8040) // (US2) FI_DI_Ratio Register
// ========== Register definition for PWMC_CH0 peripheral ==========
#define AT91C_PWMC_CH0_Reserved (AT91_CAST(AT91_REG *) 	0xFFFCC214) // (PWMC_CH0) Reserved
#define AT91C_PWMC_CH0_CPRDR (AT91_CAST(AT91_REG *) 	0xFFFCC208) // (PWMC_CH0) Channel Period Register
#define AT91C_PWMC_CH0_CDTYR (AT91_CAST(AT91_REG *) 	0xFFFCC204) // (PWMC_CH0) Channel Duty Cycle Register
#define AT91C_PWMC_CH0_CMR (AT91_CAST(AT91_REG *) 	0xFFFCC200) // (PWMC_CH0) Channel Mode Register
#define AT91C_PWMC_CH0_CUPDR (AT91_CAST(AT91_REG *) 	0xFFFCC210) // (PWMC_CH0) Channel Update Register
#define AT91C_PWMC_CH0_CCNTR (AT91_CAST(AT91_REG *) 	0xFFFCC20C) // (PWMC_CH0) Channel Counter Register
// ========== Register definition for PWMC_CH1 peripheral ==========
#define AT91C_PWMC_CH1_Reserved (AT91_CAST(AT91_REG *) 	0xFFFCC234) // (PWMC_CH1) Reserved
#define AT91C_PWMC_CH1_CUPDR (AT91_CAST(AT91_REG *) 	0xFFFCC230) // (PWMC_CH1) Channel Update Register
#define AT91C_PWMC_CH1_CPRDR (AT91_CAST(AT91_REG *) 	0xFFFCC228) // (PWMC_CH1) Channel Period Register
#define AT91C_PWMC_CH1_CCNTR (AT91_CAST(AT91_REG *) 	0xFFFCC22C) // (PWMC_CH1) Channel Counter Register
#define AT91C_PWMC_CH1_CDTYR (AT91_CAST(AT91_REG *) 	0xFFFCC224) // (PWMC_CH1) Channel Duty Cycle Register
#define AT91C_PWMC_CH1_CMR (AT91_CAST(AT91_REG *) 	0xFFFCC220) // (PWMC_CH1) Channel Mode Register
// ========== Register definition for PWMC_CH2 peripheral ==========
#define AT91C_PWMC_CH2_Reserved (AT91_CAST(AT91_REG *) 	0xFFFCC254) // (PWMC_CH2) Reserved
#define AT91C_PWMC_CH2_CMR (AT91_CAST(AT91_REG *) 	0xFFFCC240) // (PWMC_CH2) Channel Mode Register
#define AT91C_PWMC_CH2_CCNTR (AT91_CAST(AT91_REG *) 	0xFFFCC24C) // (PWMC_CH2) Channel Counter Register
#define AT91C_PWMC_CH2_CPRDR (AT91_CAST(AT91_REG *) 	0xFFFCC248) // (PWMC_CH2) Channel Period Register
#define AT91C_PWMC_CH2_CUPDR (AT91_CAST(AT91_REG *) 	0xFFFCC250) // (PWMC_CH2) Channel Update Register
#define AT91C_PWMC_CH2_CDTYR (AT91_CAST(AT91_REG *) 	0xFFFCC244) // (PWMC_CH2) Channel Duty Cycle Register
// ========== Register definition for PWMC_CH3 peripheral ==========
#define AT91C_PWMC_CH3_CUPDR (AT91_CAST(AT91_REG *) 	0xFFFCC270) // (PWMC_CH3) Channel Update Register
#define AT91C_PWMC_CH3_Reserved (AT91_CAST(AT91_REG *) 	0xFFFCC274) // (PWMC_CH3) Reserved
#define AT91C_PWMC_CH3_CPRDR (AT91_CAST(AT91_REG *) 	0xFFFCC268) // (PWMC_CH3) Channel Period Register
#define AT91C_PWMC_CH3_CDTYR (AT91_CAST(AT91_REG *) 	0xFFFCC264) // (PWMC_CH3) Channel Duty Cycle Register
#define AT91C_PWMC_CH3_CCNTR (AT91_CAST(AT91_REG *) 	0xFFFCC26C) // (PWMC_CH3) Channel Counter Register
#define AT91C_PWMC_CH3_CMR (AT91_CAST(AT91_REG *) 	0xFFFCC260) // (PWMC_CH3) Channel Mode Register
// ========== Register definition for PWMC_CH4 peripheral ==========
#define AT91C_PWMC_CH4_Reserved (AT91_CAST(AT91_REG *) 	0xFFFCC294) // (PWMC_CH4) Reserved
#define AT91C_PWMC_CH4_CDTYR (AT91_CAST(AT91_REG *) 	0xFFFCC284) // (PWMC_CH4) Channel Duty Cycle Register
#define AT91C_PWMC_CH4_CUPDR (AT91_CAST(AT91_REG *) 	0xFFFCC290) // (PWMC_CH4) Channel Update Register
#define AT91C_PWMC_CH4_CCNTR (AT91_CAST(AT91_REG *) 	0xFFFCC28C) // (PWMC_CH4) Channel Counter Register
#define AT91C_PWMC_CH4_CMR (AT91_CAST(AT91_REG *) 	0xFFFCC280) // (PWMC_CH4) Channel Mode Register
#define AT91C_PWMC_CH4_CPRDR (AT91_CAST(AT91_REG *) 	0xFFFCC288) // (PWMC_CH4) Channel Period Register
// ========== Register definition for PWMC_CH5 peripheral ==========
#define AT91C_PWMC_CH5_Reserved (AT91_CAST(AT91_REG *) 	0xFFFCC2B4) // (PWMC_CH5) Reserved
#define AT91C_PWMC_CH5_CCNTR (AT91_CAST(AT91_REG *) 	0xFFFCC2AC) // (PWMC_CH5) Channel Counter Register
#define AT91C_PWMC_CH5_CDTYR (AT91_CAST(AT91_REG *) 	0xFFFCC2A4) // (PWMC_CH5) Channel Duty Cycle Register
#define AT91C_PWMC_CH5_CPRDR (AT91_CAST(AT91_REG *) 	0xFFFCC2A8) // (PWMC_CH5) Channel Period Register
#define AT91C_PWMC_CH5_CMR (AT91_CAST(AT91_REG *) 	0xFFFCC2A0) // (PWMC_CH5) Channel Mode Register
#define AT91C_PWMC_CH5_CUPDR (AT91_CAST(AT91_REG *) 	0xFFFCC2B0) // (PWMC_CH5) Channel Update Register
// ========== Register definition for PWMC_CH6 peripheral ==========
#define AT91C_PWMC_CH6_CCNTR (AT91_CAST(AT91_REG *) 	0xFFFCC2CC) // (PWMC_CH6) Channel Counter Register
#define AT91C_PWMC_CH6_CMR (AT91_CAST(AT91_REG *) 	0xFFFCC2C0) // (PWMC_CH6) Channel Mode Register
#define AT91C_PWMC_CH6_CDTYR (AT91_CAST(AT91_REG *) 	0xFFFCC2C4) // (PWMC_CH6) Channel Duty Cycle Register
#define AT91C_PWMC_CH6_Reserved (AT91_CAST(AT91_REG *) 	0xFFFCC2D4) // (PWMC_CH6) Reserved
#define AT91C_PWMC_CH6_CPRDR (AT91_CAST(AT91_REG *) 	0xFFFCC2C8) // (PWMC_CH6) Channel Period Register
#define AT91C_PWMC_CH6_CUPDR (AT91_CAST(AT91_REG *) 	0xFFFCC2D0) // (PWMC_CH6) Channel Update Register
// ========== Register definition for PWMC_CH7 peripheral ==========
#define AT91C_PWMC_CH7_CCNTR (AT91_CAST(AT91_REG *) 	0xFFFCC2EC) // (PWMC_CH7) Channel Counter Register
#define AT91C_PWMC_CH7_CDTYR (AT91_CAST(AT91_REG *) 	0xFFFCC2E4) // (PWMC_CH7) Channel Duty Cycle Register
#define AT91C_PWMC_CH7_CPRDR (AT91_CAST(AT91_REG *) 	0xFFFCC2E8) // (PWMC_CH7) Channel Period Register
#define AT91C_PWMC_CH7_Reserved (AT91_CAST(AT91_REG *) 	0xFFFCC2F4) // (PWMC_CH7) Reserved
#define AT91C_PWMC_CH7_CUPDR (AT91_CAST(AT91_REG *) 	0xFFFCC2F0) // (PWMC_CH7) Channel Update Register
#define AT91C_PWMC_CH7_CMR (AT91_CAST(AT91_REG *) 	0xFFFCC2E0) // (PWMC_CH7) Channel Mode Register
// ========== Register definition for PWMC peripheral ==========
#define AT91C_PWMC_IDR  (AT91_CAST(AT91_REG *) 	0xFFFCC014) // (PWMC) PWMC Interrupt Disable Register
#define AT91C_PWMC_DIS  (AT91_CAST(AT91_REG *) 	0xFFFCC008) // (PWMC) PWMC Disable Register
#define AT91C_PWMC_IER  (AT91_CAST(AT91_REG *) 	0xFFFCC010) // (PWMC) PWMC Interrupt Enable Register
#define AT91C_PWMC_VR   (AT91_CAST(AT91_REG *) 	0xFFFCC0FC) // (PWMC) PWMC Version Register
#define AT91C_PWMC_ISR  (AT91_CAST(AT91_REG *) 	0xFFFCC01C) // (PWMC) PWMC Interrupt Status Register
#define AT91C_PWMC_SR   (AT91_CAST(AT91_REG *) 	0xFFFCC00C) // (PWMC) PWMC Status Register
#define AT91C_PWMC_IMR  (AT91_CAST(AT91_REG *) 	0xFFFCC018) // (PWMC) PWMC Interrupt Mask Register
#define AT91C_PWMC_MR   (AT91_CAST(AT91_REG *) 	0xFFFCC000) // (PWMC) PWMC Mode Register
#define AT91C_PWMC_ENA  (AT91_CAST(AT91_REG *) 	0xFFFCC004) // (PWMC) PWMC Enable Register
// ========== Register definition for PDC_SSC0 peripheral ==========
#define AT91C_SSC0_RNPR (AT91_CAST(AT91_REG *) 	0xFFFD0110) // (PDC_SSC0) Receive Next Pointer Register
#define AT91C_SSC0_RNCR (AT91_CAST(AT91_REG *) 	0xFFFD0114) // (PDC_SSC0) Receive Next Counter Register
#define AT91C_SSC0_PTSR (AT91_CAST(AT91_REG *) 	0xFFFD0124) // (PDC_SSC0) PDC Transfer Status Register
#define AT91C_SSC0_PTCR (AT91_CAST(AT91_REG *) 	0xFFFD0120) // (PDC_SSC0) PDC Transfer Control Register
#define AT91C_SSC0_TCR  (AT91_CAST(AT91_REG *) 	0xFFFD010C) // (PDC_SSC0) Transmit Counter Register
#define AT91C_SSC0_TNPR (AT91_CAST(AT91_REG *) 	0xFFFD0118) // (PDC_SSC0) Transmit Next Pointer Register
#define AT91C_SSC0_RCR  (AT91_CAST(AT91_REG *) 	0xFFFD0104) // (PDC_SSC0) Receive Counter Register
#define AT91C_SSC0_TPR  (AT91_CAST(AT91_REG *) 	0xFFFD0108) // (PDC_SSC0) Transmit Pointer Register
#define AT91C_SSC0_TNCR (AT91_CAST(AT91_REG *) 	0xFFFD011C) // (PDC_SSC0) Transmit Next Counter Register
#define AT91C_SSC0_RPR  (AT91_CAST(AT91_REG *) 	0xFFFD0100) // (PDC_SSC0) Receive Pointer Register
// ========== Register definition for SSC0 peripheral ==========
#define AT91C_SSC0_IER  (AT91_CAST(AT91_REG *) 	0xFFFD0044) // (SSC0) Interrupt Enable Register
#define AT91C_SSC0_THR  (AT91_CAST(AT91_REG *) 	0xFFFD0024) // (SSC0) Transmit Holding Register
#define AT91C_SSC0_IDR  (AT91_CAST(AT91_REG *) 	0xFFFD0048) // (SSC0) Interrupt Disable Register
#define AT91C_SSC0_CMR  (AT91_CAST(AT91_REG *) 	0xFFFD0004) // (SSC0) Clock Mode Register
#define AT91C_SSC0_SR   (AT91_CAST(AT91_REG *) 	0xFFFD0040) // (SSC0) Status Register
#define AT91C_SSC0_RHR  (AT91_CAST(AT91_REG *) 	0xFFFD0020) // (SSC0) Receive Holding Register
#define AT91C_SSC0_TFMR (AT91_CAST(AT91_REG *) 	0xFFFD001C) // (SSC0) Transmit Frame Mode Register
#define AT91C_SSC0_CR   (AT91_CAST(AT91_REG *) 	0xFFFD0000) // (SSC0) Control Register
#define AT91C_SSC0_IMR  (AT91_CAST(AT91_REG *) 	0xFFFD004C) // (SSC0) Interrupt Mask Register
#define AT91C_SSC0_TCMR (AT91_CAST(AT91_REG *) 	0xFFFD0018) // (SSC0) Transmit Clock Mode Register
#define AT91C_SSC0_RCMR (AT91_CAST(AT91_REG *) 	0xFFFD0010) // (SSC0) Receive Clock ModeRegister
#define AT91C_SSC0_RSHR (AT91_CAST(AT91_REG *) 	0xFFFD0030) // (SSC0) Receive Sync Holding Register
#define AT91C_SSC0_TSHR (AT91_CAST(AT91_REG *) 	0xFFFD0034) // (SSC0) Transmit Sync Holding Register
#define AT91C_SSC0_RFMR (AT91_CAST(AT91_REG *) 	0xFFFD0014) // (SSC0) Receive Frame Mode Register
// ========== Register definition for PDC_SSC1 peripheral ==========
#define AT91C_SSC1_TNCR (AT91_CAST(AT91_REG *) 	0xFFFD411C) // (PDC_SSC1) Transmit Next Counter Register
#define AT91C_SSC1_RPR  (AT91_CAST(AT91_REG *) 	0xFFFD4100) // (PDC_SSC1) Receive Pointer Register
#define AT91C_SSC1_RNCR (AT91_CAST(AT91_REG *) 	0xFFFD4114) // (PDC_SSC1) Receive Next Counter Register
#define AT91C_SSC1_TPR  (AT91_CAST(AT91_REG *) 	0xFFFD4108) // (PDC_SSC1) Transmit Pointer Register
#define AT91C_SSC1_PTCR (AT91_CAST(AT91_REG *) 	0xFFFD4120) // (PDC_SSC1) PDC Transfer Control Register
#define AT91C_SSC1_TCR  (AT91_CAST(AT91_REG *) 	0xFFFD410C) // (PDC_SSC1) Transmit Counter Register
#define AT91C_SSC1_RCR  (AT91_CAST(AT91_REG *) 	0xFFFD4104) // (PDC_SSC1) Receive Counter Register
#define AT91C_SSC1_RNPR (AT91_CAST(AT91_REG *) 	0xFFFD4110) // (PDC_SSC1) Receive Next Pointer Register
#define AT91C_SSC1_TNPR (AT91_CAST(AT91_REG *) 	0xFFFD4118) // (PDC_SSC1) Transmit Next Pointer Register
#define AT91C_SSC1_PTSR (AT91_CAST(AT91_REG *) 	0xFFFD4124) // (PDC_SSC1) PDC Transfer Status Register
// ========== Register definition for SSC1 peripheral ==========
#define AT91C_SSC1_RHR  (AT91_CAST(AT91_REG *) 	0xFFFD4020) // (SSC1) Receive Holding Register
#define AT91C_SSC1_RSHR (AT91_CAST(AT91_REG *) 	0xFFFD4030) // (SSC1) Receive Sync Holding Register
#define AT91C_SSC1_TFMR (AT91_CAST(AT91_REG *) 	0xFFFD401C) // (SSC1) Transmit Frame Mode Register
#define AT91C_SSC1_IDR  (AT91_CAST(AT91_REG *) 	0xFFFD4048) // (SSC1) Interrupt Disable Register
#define AT91C_SSC1_THR  (AT91_CAST(AT91_REG *) 	0xFFFD4024) // (SSC1) Transmit Holding Register
#define AT91C_SSC1_RCMR (AT91_CAST(AT91_REG *) 	0xFFFD4010) // (SSC1) Receive Clock ModeRegister
#define AT91C_SSC1_IER  (AT91_CAST(AT91_REG *) 	0xFFFD4044) // (SSC1) Interrupt Enable Register
#define AT91C_SSC1_TSHR (AT91_CAST(AT91_REG *) 	0xFFFD4034) // (SSC1) Transmit Sync Holding Register
#define AT91C_SSC1_SR   (AT91_CAST(AT91_REG *) 	0xFFFD4040) // (SSC1) Status Register
#define AT91C_SSC1_CMR  (AT91_CAST(AT91_REG *) 	0xFFFD4004) // (SSC1) Clock Mode Register
#define AT91C_SSC1_TCMR (AT91_CAST(AT91_REG *) 	0xFFFD4018) // (SSC1) Transmit Clock Mode Register
#define AT91C_SSC1_CR   (AT91_CAST(AT91_REG *) 	0xFFFD4000) // (SSC1) Control Register
#define AT91C_SSC1_IMR  (AT91_CAST(AT91_REG *) 	0xFFFD404C) // (SSC1) Interrupt Mask Register
#define AT91C_SSC1_RFMR (AT91_CAST(AT91_REG *) 	0xFFFD4014) // (SSC1) Receive Frame Mode Register
// ========== Register definition for PDC_ADC0 peripheral ==========
#define AT91C_ADC0_PTSR (AT91_CAST(AT91_REG *) 	0xFFFD8124) // (PDC_ADC0) PDC Transfer Status Register
#define AT91C_ADC0_PTCR (AT91_CAST(AT91_REG *) 	0xFFFD8120) // (PDC_ADC0) PDC Transfer Control Register
#define AT91C_ADC0_TNPR (AT91_CAST(AT91_REG *) 	0xFFFD8118) // (PDC_ADC0) Transmit Next Pointer Register
#define AT91C_ADC0_TNCR (AT91_CAST(AT91_REG *) 	0xFFFD811C) // (PDC_ADC0) Transmit Next Counter Register
#define AT91C_ADC0_RNPR (AT91_CAST(AT91_REG *) 	0xFFFD8110) // (PDC_ADC0) Receive Next Pointer Register
#define AT91C_ADC0_RNCR (AT91_CAST(AT91_REG *) 	0xFFFD8114) // (PDC_ADC0) Receive Next Counter Register
#define AT91C_ADC0_RPR  (AT91_CAST(AT91_REG *) 	0xFFFD8100) // (PDC_ADC0) Receive Pointer Register
#define AT91C_ADC0_TCR  (AT91_CAST(AT91_REG *) 	0xFFFD810C) // (PDC_ADC0) Transmit Counter Register
#define AT91C_ADC0_TPR  (AT91_CAST(AT91_REG *) 	0xFFFD8108) // (PDC_ADC0) Transmit Pointer Register
#define AT91C_ADC0_RCR  (AT91_CAST(AT91_REG *) 	0xFFFD8104) // (PDC_ADC0) Receive Counter Register
// ========== Register definition for ADC0 peripheral ==========
#define AT91C_ADC0_CDR2 (AT91_CAST(AT91_REG *) 	0xFFFD8038) // (ADC0) ADC Channel Data Register 2
#define AT91C_ADC0_CDR3 (AT91_CAST(AT91_REG *) 	0xFFFD803C) // (ADC0) ADC Channel Data Register 3
#define AT91C_ADC0_CDR0 (AT91_CAST(AT91_REG *) 	0xFFFD8030) // (ADC0) ADC Channel Data Register 0
#define AT91C_ADC0_CDR5 (AT91_CAST(AT91_REG *) 	0xFFFD8044) // (ADC0) ADC Channel Data Register 5
#define AT91C_ADC0_CHDR (AT91_CAST(AT91_REG *) 	0xFFFD8014) // (ADC0) ADC Channel Disable Register
#define AT91C_ADC0_SR   (AT91_CAST(AT91_REG *) 	0xFFFD801C) // (ADC0) ADC Status Register
#define AT91C_ADC0_CDR4 (AT91_CAST(AT91_REG *) 	0xFFFD8040) // (ADC0) ADC Channel Data Register 4
#define AT91C_ADC0_CDR1 (AT91_CAST(AT91_REG *) 	0xFFFD8034) // (ADC0) ADC Channel Data Register 1
#define AT91C_ADC0_LCDR (AT91_CAST(AT91_REG *) 	0xFFFD8020) // (ADC0) ADC Last Converted Data Register
#define AT91C_ADC0_IDR  (AT91_CAST(AT91_REG *) 	0xFFFD8028) // (ADC0) ADC Interrupt Disable Register
#define AT91C_ADC0_CR   (AT91_CAST(AT91_REG *) 	0xFFFD8000) // (ADC0) ADC Control Register
#define AT91C_ADC0_CDR7 (AT91_CAST(AT91_REG *) 	0xFFFD804C) // (ADC0) ADC Channel Data Register 7
#define AT91C_ADC0_CDR6 (AT91_CAST(AT91_REG *) 	0xFFFD8048) // (ADC0) ADC Channel Data Register 6
#define AT91C_ADC0_IER  (AT91_CAST(AT91_REG *) 	0xFFFD8024) // (ADC0) ADC Interrupt Enable Register
#define AT91C_ADC0_CHER (AT91_CAST(AT91_REG *) 	0xFFFD8010) // (ADC0) ADC Channel Enable Register
#define AT91C_ADC0_CHSR (AT91_CAST(AT91_REG *) 	0xFFFD8018) // (ADC0) ADC Channel Status Register
#define AT91C_ADC0_MR   (AT91_CAST(AT91_REG *) 	0xFFFD8004) // (ADC0) ADC Mode Register
#define AT91C_ADC0_IMR  (AT91_CAST(AT91_REG *) 	0xFFFD802C) // (ADC0) ADC Interrupt Mask Register
// ========== Register definition for PDC_ADC1 peripheral ==========
#define AT91C_ADC1_RCR  (AT91_CAST(AT91_REG *) 	0xFFFDC104) // (PDC_ADC1) Receive Counter Register
#define AT91C_ADC1_TPR  (AT91_CAST(AT91_REG *) 	0xFFFDC108) // (PDC_ADC1) Transmit Pointer Register
#define AT91C_ADC1_TNPR (AT91_CAST(AT91_REG *) 	0xFFFDC118) // (PDC_ADC1) Transmit Next Pointer Register
#define AT91C_ADC1_RNCR (AT91_CAST(AT91_REG *) 	0xFFFDC114) // (PDC_ADC1) Receive Next Counter Register
#define AT91C_ADC1_PTSR (AT91_CAST(AT91_REG *) 	0xFFFDC124) // (PDC_ADC1) PDC Transfer Status Register
#define AT91C_ADC1_PTCR (AT91_CAST(AT91_REG *) 	0xFFFDC120) // (PDC_ADC1) PDC Transfer Control Register
#define AT91C_ADC1_RNPR (AT91_CAST(AT91_REG *) 	0xFFFDC110) // (PDC_ADC1) Receive Next Pointer Register
#define AT91C_ADC1_TNCR (AT91_CAST(AT91_REG *) 	0xFFFDC11C) // (PDC_ADC1) Transmit Next Counter Register
#define AT91C_ADC1_TCR  (AT91_CAST(AT91_REG *) 	0xFFFDC10C) // (PDC_ADC1) Transmit Counter Register
#define AT91C_ADC1_RPR  (AT91_CAST(AT91_REG *) 	0xFFFDC100) // (PDC_ADC1) Receive Pointer Register
// ========== Register definition for ADC1 peripheral ==========
#define AT91C_ADC1_IER  (AT91_CAST(AT91_REG *) 	0xFFFDC024) // (ADC1) ADC Interrupt Enable Register
#define AT91C_ADC1_CHSR (AT91_CAST(AT91_REG *) 	0xFFFDC018) // (ADC1) ADC Channel Status Register
#define AT91C_ADC1_MR   (AT91_CAST(AT91_REG *) 	0xFFFDC004) // (ADC1) ADC Mode Register
#define AT91C_ADC1_CR   (AT91_CAST(AT91_REG *) 	0xFFFDC000) // (ADC1) ADC Control Register
#define AT91C_ADC1_LCDR (AT91_CAST(AT91_REG *) 	0xFFFDC020) // (ADC1) ADC Last Converted Data Register
#define AT91C_ADC1_CHER (AT91_CAST(AT91_REG *) 	0xFFFDC010) // (ADC1) ADC Channel Enable Register
#define AT91C_ADC1_CHDR (AT91_CAST(AT91_REG *) 	0xFFFDC014) // (ADC1) ADC Channel Disable Register
#define AT91C_ADC1_IMR  (AT91_CAST(AT91_REG *) 	0xFFFDC02C) // (ADC1) ADC Interrupt Mask Register
#define AT91C_ADC1_CDR1 (AT91_CAST(AT91_REG *) 	0xFFFDC034) // (ADC1) ADC Channel Data Register 1
#define AT91C_ADC1_CDR4 (AT91_CAST(AT91_REG *) 	0xFFFDC040) // (ADC1) ADC Channel Data Register 4
#define AT91C_ADC1_CDR0 (AT91_CAST(AT91_REG *) 	0xFFFDC030) // (ADC1) ADC Channel Data Register 0
#define AT91C_ADC1_CDR5 (AT91_CAST(AT91_REG *) 	0xFFFDC044) // (ADC1) ADC Channel Data Register 5
#define AT91C_ADC1_CDR3 (AT91_CAST(AT91_REG *) 	0xFFFDC03C) // (ADC1) ADC Channel Data Register 3
#define AT91C_ADC1_CDR6 (AT91_CAST(AT91_REG *) 	0xFFFDC048) // (ADC1) ADC Channel Data Register 6
#define AT91C_ADC1_SR   (AT91_CAST(AT91_REG *) 	0xFFFDC01C) // (ADC1) ADC Status Register
#define AT91C_ADC1_CDR2 (AT91_CAST(AT91_REG *) 	0xFFFDC038) // (ADC1) ADC Channel Data Register 2
#define AT91C_ADC1_CDR7 (AT91_CAST(AT91_REG *) 	0xFFFDC04C) // (ADC1) ADC Channel Data Register 7
#define AT91C_ADC1_IDR  (AT91_CAST(AT91_REG *) 	0xFFFDC028) // (ADC1) ADC Interrupt Disable Register
// ========== Register definition for PDC_SPI0 peripheral ==========
#define AT91C_SPI0_PTCR (AT91_CAST(AT91_REG *) 	0xFFFE0120) // (PDC_SPI0) PDC Transfer Control Register
#define AT91C_SPI0_TPR  (AT91_CAST(AT91_REG *) 	0xFFFE0108) // (PDC_SPI0) Transmit Pointer Register
#define AT91C_SPI0_TCR  (AT91_CAST(AT91_REG *) 	0xFFFE010C) // (PDC_SPI0) Transmit Counter Register
#define AT91C_SPI0_RCR  (AT91_CAST(AT91_REG *) 	0xFFFE0104) // (PDC_SPI0) Receive Counter Register
#define AT91C_SPI0_PTSR (AT91_CAST(AT91_REG *) 	0xFFFE0124) // (PDC_SPI0) PDC Transfer Status Register
#define AT91C_SPI0_RNPR (AT91_CAST(AT91_REG *) 	0xFFFE0110) // (PDC_SPI0) Receive Next Pointer Register
#define AT91C_SPI0_RPR  (AT91_CAST(AT91_REG *) 	0xFFFE0100) // (PDC_SPI0) Receive Pointer Register
#define AT91C_SPI0_TNCR (AT91_CAST(AT91_REG *) 	0xFFFE011C) // (PDC_SPI0) Transmit Next Counter Register
#define AT91C_SPI0_RNCR (AT91_CAST(AT91_REG *) 	0xFFFE0114) // (PDC_SPI0) Receive Next Counter Register
#define AT91C_SPI0_TNPR (AT91_CAST(AT91_REG *) 	0xFFFE0118) // (PDC_SPI0) Transmit Next Pointer Register
// ========== Register definition for SPI0 peripheral ==========
#define AT91C_SPI0_IER  (AT91_CAST(AT91_REG *) 	0xFFFE0014) // (SPI0) Interrupt Enable Register
#define AT91C_SPI0_SR   (AT91_CAST(AT91_REG *) 	0xFFFE0010) // (SPI0) Status Register
#define AT91C_SPI0_IDR  (AT91_CAST(AT91_REG *) 	0xFFFE0018) // (SPI0) Interrupt Disable Register
#define AT91C_SPI0_CR   (AT91_CAST(AT91_REG *) 	0xFFFE0000) // (SPI0) Control Register
#define AT91C_SPI0_MR   (AT91_CAST(AT91_REG *) 	0xFFFE0004) // (SPI0) Mode Register
#define AT91C_SPI0_IMR  (AT91_CAST(AT91_REG *) 	0xFFFE001C) // (SPI0) Interrupt Mask Register
#define AT91C_SPI0_TDR  (AT91_CAST(AT91_REG *) 	0xFFFE000C) // (SPI0) Transmit Data Register
#define AT91C_SPI0_RDR  (AT91_CAST(AT91_REG *) 	0xFFFE0008) // (SPI0) Receive Data Register
#define AT91C_SPI0_CSR  (AT91_CAST(AT91_REG *) 	0xFFFE0030) // (SPI0) Chip Select Register
// ========== Register definition for PDC_SPI1 peripheral ==========
#define AT91C_SPI1_PTCR (AT91_CAST(AT91_REG *) 	0xFFFE4120) // (PDC_SPI1) PDC Transfer Control Register
#define AT91C_SPI1_RPR  (AT91_CAST(AT91_REG *) 	0xFFFE4100) // (PDC_SPI1) Receive Pointer Register
#define AT91C_SPI1_TNCR (AT91_CAST(AT91_REG *) 	0xFFFE411C) // (PDC_SPI1) Transmit Next Counter Register
#define AT91C_SPI1_TPR  (AT91_CAST(AT91_REG *) 	0xFFFE4108) // (PDC_SPI1) Transmit Pointer Register
#define AT91C_SPI1_TNPR (AT91_CAST(AT91_REG *) 	0xFFFE4118) // (PDC_SPI1) Transmit Next Pointer Register
#define AT91C_SPI1_TCR  (AT91_CAST(AT91_REG *) 	0xFFFE410C) // (PDC_SPI1) Transmit Counter Register
#define AT91C_SPI1_RCR  (AT91_CAST(AT91_REG *) 	0xFFFE4104) // (PDC_SPI1) Receive Counter Register
#define AT91C_SPI1_RNPR (AT91_CAST(AT91_REG *) 	0xFFFE4110) // (PDC_SPI1) Receive Next Pointer Register
#define AT91C_SPI1_RNCR (AT91_CAST(AT91_REG *) 	0xFFFE4114) // (PDC_SPI1) Receive Next Counter Register
#define AT91C_SPI1_PTSR (AT91_CAST(AT91_REG *) 	0xFFFE4124) // (PDC_SPI1) PDC Transfer Status Register
// ========== Register definition for SPI1 peripheral ==========
#define AT91C_SPI1_IMR  (AT91_CAST(AT91_REG *) 	0xFFFE401C) // (SPI1) Interrupt Mask Register
#define AT91C_SPI1_IER  (AT91_CAST(AT91_REG *) 	0xFFFE4014) // (SPI1) Interrupt Enable Register
#define AT91C_SPI1_MR   (AT91_CAST(AT91_REG *) 	0xFFFE4004) // (SPI1) Mode Register
#define AT91C_SPI1_RDR  (AT91_CAST(AT91_REG *) 	0xFFFE4008) // (SPI1) Receive Data Register
#define AT91C_SPI1_IDR  (AT91_CAST(AT91_REG *) 	0xFFFE4018) // (SPI1) Interrupt Disable Register
#define AT91C_SPI1_SR   (AT91_CAST(AT91_REG *) 	0xFFFE4010) // (SPI1) Status Register
#define AT91C_SPI1_TDR  (AT91_CAST(AT91_REG *) 	0xFFFE400C) // (SPI1) Transmit Data Register
#define AT91C_SPI1_CR   (AT91_CAST(AT91_REG *) 	0xFFFE4000) // (SPI1) Control Register
#define AT91C_SPI1_CSR  (AT91_CAST(AT91_REG *) 	0xFFFE4030) // (SPI1) Chip Select Register

// *****************************************************************************
//               PIO DEFINITIONS FOR AT91SAM7A3
// *****************************************************************************
#define AT91C_PIO_PA0        (1 <<  0) // Pin Controlled by PA0
#define AT91C_PA0_TWD      (AT91C_PIO_PA0) //  TWI Two-wire Serial Data
#define AT91C_PA0_ADTRG0   (AT91C_PIO_PA0) //  ADC0 External Trigger
#define AT91C_PIO_PA1        (1 <<  1) // Pin Controlled by PA1
#define AT91C_PA1_TWCK     (AT91C_PIO_PA1) //  TWI Two-wire Serial Clock
#define AT91C_PA1_ADTRG1   (AT91C_PIO_PA1) //  ADC1 External Trigger
#define AT91C_PIO_PA10       (1 << 10) // Pin Controlled by PA10
#define AT91C_PA10_TXD2     (AT91C_PIO_PA10) //  USART 2 Transmit Data
#define AT91C_PA10_SPI1_SPCK (AT91C_PIO_PA10) //  SPI1 Serial Clock
#define AT91C_PIO_PA11       (1 << 11) // Pin Controlled by PA11
#define AT91C_PA11_SPI0_NPCS0 (AT91C_PIO_PA11) //  SPI0 Peripheral Chip Select 0
#define AT91C_PIO_PA12       (1 << 12) // Pin Controlled by PA12
#define AT91C_PA12_SPI0_NPCS1 (AT91C_PIO_PA12) //  SPI0 Peripheral Chip Select 1
#define AT91C_PA12_MCDA1    (AT91C_PIO_PA12) //  Multimedia Card A Data 1
#define AT91C_PIO_PA13       (1 << 13) // Pin Controlled by PA13
#define AT91C_PA13_SPI0_NPCS2 (AT91C_PIO_PA13) //  SPI0 Peripheral Chip Select 2
#define AT91C_PA13_MCDA2    (AT91C_PIO_PA13) //  Multimedia Card A Data 2
#define AT91C_PIO_PA14       (1 << 14) // Pin Controlled by PA14
#define AT91C_PA14_SPI0_NPCS3 (AT91C_PIO_PA14) //  SPI0 Peripheral Chip Select 3
#define AT91C_PA14_MCDA3    (AT91C_PIO_PA14) //  Multimedia Card A Data 3
#define AT91C_PIO_PA15       (1 << 15) // Pin Controlled by PA15
#define AT91C_PA15_SPI0_MISO (AT91C_PIO_PA15) //  SPI0 Master In Slave
#define AT91C_PA15_MCDA0    (AT91C_PIO_PA15) //  Multimedia Card A Data 0
#define AT91C_PIO_PA16       (1 << 16) // Pin Controlled by PA16
#define AT91C_PA16_SPI0_MOSI (AT91C_PIO_PA16) //  SPI0 Master Out Slave
#define AT91C_PA16_MCCDA    (AT91C_PIO_PA16) //  Multimedia Card A Command
#define AT91C_PIO_PA17       (1 << 17) // Pin Controlled by PA17
#define AT91C_PA17_SPI0_SPCK (AT91C_PIO_PA17) //  SPI0 Serial Clock
#define AT91C_PA17_MCCK     (AT91C_PIO_PA17) //  Multimedia Card Clock
#define AT91C_PIO_PA18       (1 << 18) // Pin Controlled by PA18
#define AT91C_PA18_PWM0     (AT91C_PIO_PA18) //  PWMC Channel 0
#define AT91C_PA18_PCK0     (AT91C_PIO_PA18) //  PMC Programmable Clock Output 0
#define AT91C_PIO_PA19       (1 << 19) // Pin Controlled by PA19
#define AT91C_PA19_PWM1     (AT91C_PIO_PA19) //  PWMC Channel 1
#define AT91C_PA19_PCK1     (AT91C_PIO_PA19) //  PMC Programmable Clock Output 1
#define AT91C_PIO_PA2        (1 <<  2) // Pin Controlled by PA2
#define AT91C_PA2_RXD0     (AT91C_PIO_PA2) //  USART 0 Receive Data
#define AT91C_PIO_PA20       (1 << 20) // Pin Controlled by PA20
#define AT91C_PA20_PWM2     (AT91C_PIO_PA20) //  PWMC Channel 2
#define AT91C_PA20_PCK2     (AT91C_PIO_PA20) //  PMC Programmable Clock Output 2
#define AT91C_PIO_PA21       (1 << 21) // Pin Controlled by PA21
#define AT91C_PA21_PWM3     (AT91C_PIO_PA21) //  PWMC Channel 3
#define AT91C_PA21_PCK3     (AT91C_PIO_PA21) //  PMC Programmable Clock Output 3
#define AT91C_PIO_PA22       (1 << 22) // Pin Controlled by PA22
#define AT91C_PA22_PWM4     (AT91C_PIO_PA22) //  PWMC Channel 4
#define AT91C_PA22_IRQ0     (AT91C_PIO_PA22) //  Interrupt input 0
#define AT91C_PIO_PA23       (1 << 23) // Pin Controlled by PA23
#define AT91C_PA23_PWM5     (AT91C_PIO_PA23) //  PWMC Channel 5
#define AT91C_PA23_IRQ1     (AT91C_PIO_PA23) //  Interrupt input 1
#define AT91C_PIO_PA24       (1 << 24) // Pin Controlled by PA24
#define AT91C_PA24_PWM6     (AT91C_PIO_PA24) //  PWMC Channel 6
#define AT91C_PA24_TCLK4    (AT91C_PIO_PA24) //  Timer Counter 4 external Clock Input
#define AT91C_PIO_PA25       (1 << 25) // Pin Controlled by PA25
#define AT91C_PA25_PWM7     (AT91C_PIO_PA25) //  PWMC Channel 7
#define AT91C_PA25_TCLK5    (AT91C_PIO_PA25) //  Timer Counter 5 external Clock Input
#define AT91C_PIO_PA26       (1 << 26) // Pin Controlled by PA26
#define AT91C_PA26_CANRX0   (AT91C_PIO_PA26) //  CAN Receive 0
#define AT91C_PIO_PA27       (1 << 27) // Pin Controlled by PA27
#define AT91C_PA27_CANTX0   (AT91C_PIO_PA27) //  CAN Transmit 0
#define AT91C_PIO_PA28       (1 << 28) // Pin Controlled by PA28
#define AT91C_PA28_CANRX1   (AT91C_PIO_PA28) //  CAN Receive 1
#define AT91C_PA28_TCLK3    (AT91C_PIO_PA28) //  Timer Counter 3 external Clock Input
#define AT91C_PIO_PA29       (1 << 29) // Pin Controlled by PA29
#define AT91C_PA29_CANTX1   (AT91C_PIO_PA29) //  CAN Transmit 1
#define AT91C_PA29_TCLK6    (AT91C_PIO_PA29) //  Timer Counter 6 external clock input
#define AT91C_PIO_PA3        (1 <<  3) // Pin Controlled by PA3
#define AT91C_PA3_TXD0     (AT91C_PIO_PA3) //  USART 0 Transmit Data
#define AT91C_PIO_PA30       (1 << 30) // Pin Controlled by PA30
#define AT91C_PA30_DRXD     (AT91C_PIO_PA30) //  DBGU Debug Receive Data
#define AT91C_PA30_TCLK7    (AT91C_PIO_PA30) //  Timer Counter 7 external clock input
#define AT91C_PIO_PA31       (1 << 31) // Pin Controlled by PA31
#define AT91C_PA31_DTXD     (AT91C_PIO_PA31) //  DBGU Debug Transmit Data
#define AT91C_PA31_TCLK8    (AT91C_PIO_PA31) //  Timer Counter 8 external clock input
#define AT91C_PIO_PA4        (1 <<  4) // Pin Controlled by PA4
#define AT91C_PA4_SCK0     (AT91C_PIO_PA4) //  USART 0 Serial Clock
#define AT91C_PA4_SPI1_NPCS0 (AT91C_PIO_PA4) //  SPI1 Peripheral Chip Select 0
#define AT91C_PIO_PA5        (1 <<  5) // Pin Controlled by PA5
#define AT91C_PA5_RTS0     (AT91C_PIO_PA5) //  USART 0 Ready To Send
#define AT91C_PA5_SPI1_NPCS1 (AT91C_PIO_PA5) //  SPI1 Peripheral Chip Select 1
#define AT91C_PIO_PA6        (1 <<  6) // Pin Controlled by PA6
#define AT91C_PA6_CTS0     (AT91C_PIO_PA6) //  USART 0 Clear To Send
#define AT91C_PA6_SPI1_NPCS2 (AT91C_PIO_PA6) //  SPI1 Peripheral Chip Select 2
#define AT91C_PIO_PA7        (1 <<  7) // Pin Controlled by PA7
#define AT91C_PA7_RXD1     (AT91C_PIO_PA7) //  USART 1 Receive Data
#define AT91C_PA7_SPI1_NPCS3 (AT91C_PIO_PA7) //  SPI1 Peripheral Chip Select 3
#define AT91C_PIO_PA8        (1 <<  8) // Pin Controlled by PA8
#define AT91C_PA8_TXD1     (AT91C_PIO_PA8) //  USART 1 Transmit Data
#define AT91C_PA8_SPI1_MISO (AT91C_PIO_PA8) //  SPI1 Master In Slave
#define AT91C_PIO_PA9        (1 <<  9) // Pin Controlled by PA9
#define AT91C_PA9_RXD2     (AT91C_PIO_PA9) //  USART 2 Receive Data
#define AT91C_PA9_SPI1_MOSI (AT91C_PIO_PA9) //  SPI1 Master Out Slave
#define AT91C_PIO_PB0        (1 <<  0) // Pin Controlled by PB0
#define AT91C_PB0_IRQ2     (AT91C_PIO_PB0) //  Interrupt input 2
#define AT91C_PB0_PWM5     (AT91C_PIO_PB0) //  PWMC Channel 5
#define AT91C_PIO_PB1        (1 <<  1) // Pin Controlled by PB1
#define AT91C_PB1_IRQ3     (AT91C_PIO_PB1) //  Interrupt input 3
#define AT91C_PB1_PWM6     (AT91C_PIO_PB1) //  PWMC Channel 6
#define AT91C_PIO_PB10       (1 << 10) // Pin Controlled by PB10
#define AT91C_PB10_TCLK1    (AT91C_PIO_PB10) //  Timer Counter 1 external clock input
#define AT91C_PB10_RK1      (AT91C_PIO_PB10) //  SSC Receive Clock 1
#define AT91C_PIO_PB11       (1 << 11) // Pin Controlled by PB11
#define AT91C_PB11_TCLK2    (AT91C_PIO_PB11) //  Timer Counter 2 external clock input
#define AT91C_PB11_RF1      (AT91C_PIO_PB11) //  SSC Receive Frame Sync 1
#define AT91C_PIO_PB12       (1 << 12) // Pin Controlled by PB12
#define AT91C_PB12_TIOA0    (AT91C_PIO_PB12) //  Timer Counter 0 Multipurpose Timer I/O Pin A
#define AT91C_PB12_TD1      (AT91C_PIO_PB12) //  SSC Transmit Data 1
#define AT91C_PIO_PB13       (1 << 13) // Pin Controlled by PB13
#define AT91C_PB13_TIOB0    (AT91C_PIO_PB13) //  Timer Counter 0 Multipurpose Timer I/O Pin B
#define AT91C_PB13_RD1      (AT91C_PIO_PB13) //  SSC Receive Data 1
#define AT91C_PIO_PB14       (1 << 14) // Pin Controlled by PB14
#define AT91C_PB14_TIOA1    (AT91C_PIO_PB14) //  Timer Counter 1 Multipurpose Timer I/O Pin A
#define AT91C_PB14_PWM0     (AT91C_PIO_PB14) //  PWMC Channel 0
#define AT91C_PIO_PB15       (1 << 15) // Pin Controlled by PB15
#define AT91C_PB15_TIOB1    (AT91C_PIO_PB15) //  Timer Counter 1 Multipurpose Timer I/O Pin B
#define AT91C_PB15_PWM1     (AT91C_PIO_PB15) //  PWMC Channel 1
#define AT91C_PIO_PB16       (1 << 16) // Pin Controlled by PB16
#define AT91C_PB16_TIOA2    (AT91C_PIO_PB16) //  Timer Counter 2 Multipurpose Timer I/O Pin A
#define AT91C_PB16_PWM2     (AT91C_PIO_PB16) //  PWMC Channel 2
#define AT91C_PIO_PB17       (1 << 17) // Pin Controlled by PB17
#define AT91C_PB17_TIOB2    (AT91C_PIO_PB17) //  Timer Counter 2 Multipurpose Timer I/O Pin B
#define AT91C_PB17_PWM3     (AT91C_PIO_PB17) //  PWMC Channel 3
#define AT91C_PIO_PB18       (1 << 18) // Pin Controlled by PB18
#define AT91C_PB18_TIOA3    (AT91C_PIO_PB18) //  Timer Counter 3 Multipurpose Timer I/O Pin A
#define AT91C_PB18_PWM4     (AT91C_PIO_PB18) //  PWMC Channel 4
#define AT91C_PIO_PB19       (1 << 19) // Pin Controlled by PB19
#define AT91C_PB19_TIOB3    (AT91C_PIO_PB19) //  Timer Counter 3 Multipurpose Timer I/O Pin B
#define AT91C_PB19_SPI1_NPCS1 (AT91C_PIO_PB19) //  SPI1 Peripheral Chip Select 1
#define AT91C_PIO_PB2        (1 <<  2) // Pin Controlled by PB2
#define AT91C_PB2_TF0      (AT91C_PIO_PB2) //  SSC Transmit Frame Sync 0
#define AT91C_PB2_PWM7     (AT91C_PIO_PB2) //  PWMC Channel 7
#define AT91C_PIO_PB20       (1 << 20) // Pin Controlled by PB20
#define AT91C_PB20_TIOA4    (AT91C_PIO_PB20) //  Timer Counter 4 Multipurpose Timer I/O Pin A
#define AT91C_PB20_SPI1_NPCS2 (AT91C_PIO_PB20) //  SPI1 Peripheral Chip Select 2
#define AT91C_PIO_PB21       (1 << 21) // Pin Controlled by PB21
#define AT91C_PB21_TIOB4    (AT91C_PIO_PB21) //  Timer Counter 4 Multipurpose Timer I/O Pin B
#define AT91C_PB21_SPI1_NPCS3 (AT91C_PIO_PB21) //  SPI1 Peripheral Chip Select 3
#define AT91C_PIO_PB22       (1 << 22) // Pin Controlled by PB22
#define AT91C_PB22_TIOA5    (AT91C_PIO_PB22) //  Timer Counter 5 Multipurpose Timer I/O Pin A
#define AT91C_PIO_PB23       (1 << 23) // Pin Controlled by PB23
#define AT91C_PB23_TIOB5    (AT91C_PIO_PB23) //  Timer Counter 5 Multipurpose Timer I/O Pin B
#define AT91C_PIO_PB24       (1 << 24) // Pin Controlled by PB24
#define AT91C_PB24_TIOA6    (AT91C_PIO_PB24) //  Timer Counter 6 Multipurpose Timer I/O Pin A
#define AT91C_PB24_RTS1     (AT91C_PIO_PB24) //  USART 1 Ready To Send
#define AT91C_PIO_PB25       (1 << 25) // Pin Controlled by PB25
#define AT91C_PB25_TIOB6    (AT91C_PIO_PB25) //  Timer Counter 6 Multipurpose Timer I/O Pin B
#define AT91C_PB25_CTS1     (AT91C_PIO_PB25) //  USART 1 Clear To Send
#define AT91C_PIO_PB26       (1 << 26) // Pin Controlled by PB26
#define AT91C_PB26_TIOA7    (AT91C_PIO_PB26) //  Timer Counter 7 Multipurpose Timer I/O Pin A
#define AT91C_PB26_SCK1     (AT91C_PIO_PB26) //  USART 1 Serial Clock
#define AT91C_PIO_PB27       (1 << 27) // Pin Controlled by PB27
#define AT91C_PB27_TIOB7    (AT91C_PIO_PB27) //  Timer Counter 7 Multipurpose Timer I/O Pin B
#define AT91C_PB27_RTS2     (AT91C_PIO_PB27) //  USART 2 Ready To Send
#define AT91C_PIO_PB28       (1 << 28) // Pin Controlled by PB28
#define AT91C_PB28_TIOA8    (AT91C_PIO_PB28) //  Timer Counter 8 Multipurpose Timer I/O Pin A
#define AT91C_PB28_CTS2     (AT91C_PIO_PB28) //  USART 2 Clear To Send
#define AT91C_PIO_PB29       (1 << 29) // Pin Controlled by PB29
#define AT91C_PB29_TIOB8    (AT91C_PIO_PB29) //  Timer Counter 8 Multipurpose Timer I/O Pin B
#define AT91C_PB29_SCK2     (AT91C_PIO_PB29) //  USART 2 Serial Clock
#define AT91C_PIO_PB3        (1 <<  3) // Pin Controlled by PB3
#define AT91C_PB3_TK0      (AT91C_PIO_PB3) //  SSC Transmit Clock 0
#define AT91C_PB3_PCK0     (AT91C_PIO_PB3) //  PMC Programmable Clock Output 0
#define AT91C_PIO_PB4        (1 <<  4) // Pin Controlled by PB4
#define AT91C_PB4_TD0      (AT91C_PIO_PB4) //  SSC Transmit data
#define AT91C_PB4_PCK1     (AT91C_PIO_PB4) //  PMC Programmable Clock Output 1
#define AT91C_PIO_PB5        (1 <<  5) // Pin Controlled by PB5
#define AT91C_PB5_RD0      (AT91C_PIO_PB5) //  SSC Receive Data
#define AT91C_PB5_PCK2     (AT91C_PIO_PB5) //  PMC Programmable Clock Output 2
#define AT91C_PIO_PB6        (1 <<  6) // Pin Controlled by PB6
#define AT91C_PB6_RK0      (AT91C_PIO_PB6) //  SSC Receive Clock
#define AT91C_PB6_PCK3     (AT91C_PIO_PB6) //  PMC Programmable Clock Output 3
#define AT91C_PIO_PB7        (1 <<  7) // Pin Controlled by PB7
#define AT91C_PB7_RF0      (AT91C_PIO_PB7) //  SSC Receive Frame Sync 0
#define AT91C_PB7_CANTX1   (AT91C_PIO_PB7) //  CAN Transmit 1
#define AT91C_PIO_PB8        (1 <<  8) // Pin Controlled by PB8
#define AT91C_PB8_FIQ      (AT91C_PIO_PB8) //  AIC Fast Interrupt Input
#define AT91C_PB8_TF1      (AT91C_PIO_PB8) //  SSC Transmit Frame Sync 1
#define AT91C_PIO_PB9        (1 <<  9) // Pin Controlled by PB9
#define AT91C_PB9_TCLK0    (AT91C_PIO_PB9) //  Timer Counter 0 external clock input
#define AT91C_PB9_TK1      (AT91C_PIO_PB9) //  SSC Transmit Clock 1

// *****************************************************************************
//               PERIPHERAL ID DEFINITIONS FOR AT91SAM7A3
// *****************************************************************************
#define AT91C_ID_FIQ    ( 0) // Advanced Interrupt Controller (FIQ)
#define AT91C_ID_SYS    ( 1) // System Peripheral
#define AT91C_ID_PIOA   ( 2) // Parallel IO Controller A
#define AT91C_ID_PIOB   ( 3) // Parallel IO Controller B
#define AT91C_ID_CAN0   ( 4) // Control Area Network Controller 0
#define AT91C_ID_CAN1   ( 5) // Control Area Network Controller 1
#define AT91C_ID_US0    ( 6) // USART 0
#define AT91C_ID_US1    ( 7) // USART 1
#define AT91C_ID_US2    ( 8) // USART 2
#define AT91C_ID_MCI    ( 9) // Multimedia Card Interface
#define AT91C_ID_TWI    (10) // Two-Wire Interface
#define AT91C_ID_SPI0   (11) // Serial Peripheral Interface 0
#define AT91C_ID_SPI1   (12) // Serial Peripheral Interface 1
#define AT91C_ID_SSC0   (13) // Serial Synchronous Controller 0
#define AT91C_ID_SSC1   (14) // Serial Synchronous Controller 1
#define AT91C_ID_TC0    (15) // Timer Counter 0
#define AT91C_ID_TC1    (16) // Timer Counter 1
#define AT91C_ID_TC2    (17) // Timer Counter 2
#define AT91C_ID_TC3    (18) // Timer Counter 3
#define AT91C_ID_TC4    (19) // Timer Counter 4
#define AT91C_ID_TC5    (20) // Timer Counter 5
#define AT91C_ID_TC6    (21) // Timer Counter 6
#define AT91C_ID_TC7    (22) // Timer Counter 7
#define AT91C_ID_TC8    (23) // Timer Counter 8
#define AT91C_ID_ADC0   (24) // Analog To Digital Converter 0
#define AT91C_ID_ADC1   (25) // Analog To Digital Converter 1
#define AT91C_ID_PWMC   (26) // Pulse Width Modulation Controller
#define AT91C_ID_UDP    (27) // USB Device Port
#define AT91C_ID_IRQ0   (28) // Advanced Interrupt Controller (IRQ0)
#define AT91C_ID_IRQ1   (29) // Advanced Interrupt Controller (IRQ1)
#define AT91C_ID_IRQ2   (30) // Advanced Interrupt Controller (IRQ2)
#define AT91C_ID_IRQ3   (31) // Advanced Interrupt Controller (IRQ3)
#define AT91C_ALL_INT   (0xFFFFFFFF) // ALL VALID INTERRUPTS

// *****************************************************************************
//               BASE ADDRESS DEFINITIONS FOR AT91SAM7A3
// *****************************************************************************
#define AT91C_BASE_SYS       (AT91_CAST(AT91PS_SYS) 	0xFFFFF000) // (SYS) Base Address
#define AT91C_BASE_AIC       (AT91_CAST(AT91PS_AIC) 	0xFFFFF000) // (AIC) Base Address
#define AT91C_BASE_PDC_DBGU  (AT91_CAST(AT91PS_PDC) 	0xFFFFF300) // (PDC_DBGU) Base Address
#define AT91C_BASE_DBGU      (AT91_CAST(AT91PS_DBGU) 	0xFFFFF200) // (DBGU) Base Address
#define AT91C_BASE_PIOA      (AT91_CAST(AT91PS_PIO) 	0xFFFFF400) // (PIOA) Base Address
#define AT91C_BASE_PIOB      (AT91_CAST(AT91PS_PIO) 	0xFFFFF600) // (PIOB) Base Address
#define AT91C_BASE_CKGR      (AT91_CAST(AT91PS_CKGR) 	0xFFFFFC20) // (CKGR) Base Address
#define AT91C_BASE_PMC       (AT91_CAST(AT91PS_PMC) 	0xFFFFFC00) // (PMC) Base Address
#define AT91C_BASE_RSTC      (AT91_CAST(AT91PS_RSTC) 	0xFFFFFD00) // (RSTC) Base Address
#define AT91C_BASE_SHDWC     (AT91_CAST(AT91PS_SHDWC) 	0xFFFFFD10) // (SHDWC) Base Address
#define AT91C_BASE_RTTC      (AT91_CAST(AT91PS_RTTC) 	0xFFFFFD20) // (RTTC) Base Address
#define AT91C_BASE_PITC      (AT91_CAST(AT91PS_PITC) 	0xFFFFFD30) // (PITC) Base Address
#define AT91C_BASE_WDTC      (AT91_CAST(AT91PS_WDTC) 	0xFFFFFD40) // (WDTC) Base Address
#define AT91C_BASE_MC        (AT91_CAST(AT91PS_MC) 	0xFFFFFF00) // (MC) Base Address
#define AT91C_BASE_CAN0_MB0  (AT91_CAST(AT91PS_CAN_MB) 	0xFFF80200) // (CAN0_MB0) Base Address
#define AT91C_BASE_CAN0_MB1  (AT91_CAST(AT91PS_CAN_MB) 	0xFFF80220) // (CAN0_MB1) Base Address
#define AT91C_BASE_CAN0_MB2  (AT91_CAST(AT91PS_CAN_MB) 	0xFFF80240) // (CAN0_MB2) Base Address
#define AT91C_BASE_CAN0_MB3  (AT91_CAST(AT91PS_CAN_MB) 	0xFFF80260) // (CAN0_MB3) Base Address
#define AT91C_BASE_CAN0_MB4  (AT91_CAST(AT91PS_CAN_MB) 	0xFFF80280) // (CAN0_MB4) Base Address
#define AT91C_BASE_CAN0_MB5  (AT91_CAST(AT91PS_CAN_MB) 	0xFFF802A0) // (CAN0_MB5) Base Address
#define AT91C_BASE_CAN0_MB6  (AT91_CAST(AT91PS_CAN_MB) 	0xFFF802C0) // (CAN0_MB6) Base Address
#define AT91C_BASE_CAN0_MB7  (AT91_CAST(AT91PS_CAN_MB) 	0xFFF802E0) // (CAN0_MB7) Base Address
#define AT91C_BASE_CAN0_MB8  (AT91_CAST(AT91PS_CAN_MB) 	0xFFF80300) // (CAN0_MB8) Base Address
#define AT91C_BASE_CAN0_MB9  (AT91_CAST(AT91PS_CAN_MB) 	0xFFF80320) // (CAN0_MB9) Base Address
#define AT91C_BASE_CAN0_MB10 (AT91_CAST(AT91PS_CAN_MB) 	0xFFF80340) // (CAN0_MB10) Base Address
#define AT91C_BASE_CAN0_MB11 (AT91_CAST(AT91PS_CAN_MB) 	0xFFF80360) // (CAN0_MB11) Base Address
#define AT91C_BASE_CAN0_MB12 (AT91_CAST(AT91PS_CAN_MB) 	0xFFF80380) // (CAN0_MB12) Base Address
#define AT91C_BASE_CAN0_MB13 (AT91_CAST(AT91PS_CAN_MB) 	0xFFF803A0) // (CAN0_MB13) Base Address
#define AT91C_BASE_CAN0_MB14 (AT91_CAST(AT91PS_CAN_MB) 	0xFFF803C0) // (CAN0_MB14) Base Address
#define AT91C_BASE_CAN0_MB15 (AT91_CAST(AT91PS_CAN_MB) 	0xFFF803E0) // (CAN0_MB15) Base Address
#define AT91C_BASE_CAN0      (AT91_CAST(AT91PS_CAN) 	0xFFF80000) // (CAN0) Base Address
#define AT91C_BASE_CAN1_MB0  (AT91_CAST(AT91PS_CAN_MB) 	0xFFF84200) // (CAN1_MB0) Base Address
#define AT91C_BASE_CAN1_MB1  (AT91_CAST(AT91PS_CAN_MB) 	0xFFF84220) // (CAN1_MB1) Base Address
#define AT91C_BASE_CAN1_MB2  (AT91_CAST(AT91PS_CAN_MB) 	0xFFF84240) // (CAN1_MB2) Base Address
#define AT91C_BASE_CAN1_MB3  (AT91_CAST(AT91PS_CAN_MB) 	0xFFF84260) // (CAN1_MB3) Base Address
#define AT91C_BASE_CAN1_MB4  (AT91_CAST(AT91PS_CAN_MB) 	0xFFF84280) // (CAN1_MB4) Base Address
#define AT91C_BASE_CAN1_MB5  (AT91_CAST(AT91PS_CAN_MB) 	0xFFF842A0) // (CAN1_MB5) Base Address
#define AT91C_BASE_CAN1_MB6  (AT91_CAST(AT91PS_CAN_MB) 	0xFFF842C0) // (CAN1_MB6) Base Address
#define AT91C_BASE_CAN1_MB7  (AT91_CAST(AT91PS_CAN_MB) 	0xFFF842E0) // (CAN1_MB7) Base Address
#define AT91C_BASE_CAN1_MB8  (AT91_CAST(AT91PS_CAN_MB) 	0xFFF84300) // (CAN1_MB8) Base Address
#define AT91C_BASE_CAN1_MB9  (AT91_CAST(AT91PS_CAN_MB) 	0xFFF84320) // (CAN1_MB9) Base Address
#define AT91C_BASE_CAN1_MB10 (AT91_CAST(AT91PS_CAN_MB) 	0xFFF84340) // (CAN1_MB10) Base Address
#define AT91C_BASE_CAN1_MB11 (AT91_CAST(AT91PS_CAN_MB) 	0xFFF84360) // (CAN1_MB11) Base Address
#define AT91C_BASE_CAN1_MB12 (AT91_CAST(AT91PS_CAN_MB) 	0xFFF84380) // (CAN1_MB12) Base Address
#define AT91C_BASE_CAN1_MB13 (AT91_CAST(AT91PS_CAN_MB) 	0xFFF843A0) // (CAN1_MB13) Base Address
#define AT91C_BASE_CAN1_MB14 (AT91_CAST(AT91PS_CAN_MB) 	0xFFF843C0) // (CAN1_MB14) Base Address
#define AT91C_BASE_CAN1_MB15 (AT91_CAST(AT91PS_CAN_MB) 	0xFFF843E0) // (CAN1_MB15) Base Address
#define AT91C_BASE_CAN1      (AT91_CAST(AT91PS_CAN) 	0xFFF84000) // (CAN1) Base Address
#define AT91C_BASE_TC0       (AT91_CAST(AT91PS_TC) 	0xFFFA0000) // (TC0) Base Address
#define AT91C_BASE_TC1       (AT91_CAST(AT91PS_TC) 	0xFFFA0040) // (TC1) Base Address
#define AT91C_BASE_TC2       (AT91_CAST(AT91PS_TC) 	0xFFFA0080) // (TC2) Base Address
#define AT91C_BASE_TCB0      (AT91_CAST(AT91PS_TCB) 	0xFFFA0000) // (TCB0) Base Address
#define AT91C_BASE_TC3       (AT91_CAST(AT91PS_TC) 	0xFFFA4000) // (TC3) Base Address
#define AT91C_BASE_TC4       (AT91_CAST(AT91PS_TC) 	0xFFFA4040) // (TC4) Base Address
#define AT91C_BASE_TC5       (AT91_CAST(AT91PS_TC) 	0xFFFA4080) // (TC5) Base Address
#define AT91C_BASE_TCB1      (AT91_CAST(AT91PS_TCB) 	0xFFFA4000) // (TCB1) Base Address
#define AT91C_BASE_TC6       (AT91_CAST(AT91PS_TC) 	0xFFFA8000) // (TC6) Base Address
#define AT91C_BASE_TC7       (AT91_CAST(AT91PS_TC) 	0xFFFA8040) // (TC7) Base Address
#define AT91C_BASE_TC8       (AT91_CAST(AT91PS_TC) 	0xFFFA8080) // (TC8) Base Address
#define AT91C_BASE_TCB2      (AT91_CAST(AT91PS_TCB) 	0xFFFA8000) // (TCB2) Base Address
#define AT91C_BASE_PDC_MCI   (AT91_CAST(AT91PS_PDC) 	0xFFFAC100) // (PDC_MCI) Base Address
#define AT91C_BASE_MCI       (AT91_CAST(AT91PS_MCI) 	0xFFFAC000) // (MCI) Base Address
#define AT91C_BASE_UDP       (AT91_CAST(AT91PS_UDP) 	0xFFFB0000) // (UDP) Base Address
#define AT91C_BASE_TWI       (AT91_CAST(AT91PS_TWI) 	0xFFFB8000) // (TWI) Base Address
#define AT91C_BASE_PDC_US0   (AT91_CAST(AT91PS_PDC) 	0xFFFC0100) // (PDC_US0) Base Address
#define AT91C_BASE_US0       (AT91_CAST(AT91PS_USART) 	0xFFFC0000) // (US0) Base Address
#define AT91C_BASE_PDC_US1   (AT91_CAST(AT91PS_PDC) 	0xFFFC4100) // (PDC_US1) Base Address
#define AT91C_BASE_US1       (AT91_CAST(AT91PS_USART) 	0xFFFC4000) // (US1) Base Address
#define AT91C_BASE_PDC_US2   (AT91_CAST(AT91PS_PDC) 	0xFFFC8100) // (PDC_US2) Base Address
#define AT91C_BASE_US2       (AT91_CAST(AT91PS_USART) 	0xFFFC8000) // (US2) Base Address
#define AT91C_BASE_PWMC_CH0  (AT91_CAST(AT91PS_PWMC_CH) 	0xFFFCC200) // (PWMC_CH0) Base Address
#define AT91C_BASE_PWMC_CH1  (AT91_CAST(AT91PS_PWMC_CH) 	0xFFFCC220) // (PWMC_CH1) Base Address
#define AT91C_BASE_PWMC_CH2  (AT91_CAST(AT91PS_PWMC_CH) 	0xFFFCC240) // (PWMC_CH2) Base Address
#define AT91C_BASE_PWMC_CH3  (AT91_CAST(AT91PS_PWMC_CH) 	0xFFFCC260) // (PWMC_CH3) Base Address
#define AT91C_BASE_PWMC_CH4  (AT91_CAST(AT91PS_PWMC_CH) 	0xFFFCC280) // (PWMC_CH4) Base Address
#define AT91C_BASE_PWMC_CH5  (AT91_CAST(AT91PS_PWMC_CH) 	0xFFFCC2A0) // (PWMC_CH5) Base Address
#define AT91C_BASE_PWMC_CH6  (AT91_CAST(AT91PS_PWMC_CH) 	0xFFFCC2C0) // (PWMC_CH6) Base Address
#define AT91C_BASE_PWMC_CH7  (AT91_CAST(AT91PS_PWMC_CH) 	0xFFFCC2E0) // (PWMC_CH7) Base Address
#define AT91C_BASE_PWMC      (AT91_CAST(AT91PS_PWMC) 	0xFFFCC000) // (PWMC) Base Address
#define AT91C_BASE_PDC_SSC0  (AT91_CAST(AT91PS_PDC) 	0xFFFD0100) // (PDC_SSC0) Base Address
#define AT91C_BASE_SSC0      (AT91_CAST(AT91PS_SSC) 	0xFFFD0000) // (SSC0) Base Address
#define AT91C_BASE_PDC_SSC1  (AT91_CAST(AT91PS_PDC) 	0xFFFD4100) // (PDC_SSC1) Base Address
#define AT91C_BASE_SSC1      (AT91_CAST(AT91PS_SSC) 	0xFFFD4000) // (SSC1) Base Address
#define AT91C_BASE_PDC_ADC0  (AT91_CAST(AT91PS_PDC) 	0xFFFD8100) // (PDC_ADC0) Base Address
#define AT91C_BASE_ADC0      (AT91_CAST(AT91PS_ADC) 	0xFFFD8000) // (ADC0) Base Address
#define AT91C_BASE_PDC_ADC1  (AT91_CAST(AT91PS_PDC) 	0xFFFDC100) // (PDC_ADC1) Base Address
#define AT91C_BASE_ADC1      (AT91_CAST(AT91PS_ADC) 	0xFFFDC000) // (ADC1) Base Address
#define AT91C_BASE_PDC_SPI0  (AT91_CAST(AT91PS_PDC) 	0xFFFE0100) // (PDC_SPI0) Base Address
#define AT91C_BASE_SPI0      (AT91_CAST(AT91PS_SPI) 	0xFFFE0000) // (SPI0) Base Address
#define AT91C_BASE_PDC_SPI1  (AT91_CAST(AT91PS_PDC) 	0xFFFE4100) // (PDC_SPI1) Base Address
#define AT91C_BASE_SPI1      (AT91_CAST(AT91PS_SPI) 	0xFFFE4000) // (SPI1) Base Address

// *****************************************************************************
//               MEMORY MAPPING DEFINITIONS FOR AT91SAM7A3
// *****************************************************************************
// ISRAM
#define AT91C_ISRAM	 (0x00200000) // Internal SRAM base address
#define AT91C_ISRAM_SIZE	 (0x00008000) // Internal SRAM size in byte (32 Kbytes)
// IFLASH
#define AT91C_IFLASH	 (0x00100000) // Internal FLASH base address
#define AT91C_IFLASH_SIZE	 (0x00040000) // Internal FLASH size in byte (256 Kbytes)
#define AT91C_IFLASH_PAGE_SIZE	 (256) // Internal FLASH Page Size: 256 bytes
#define AT91C_IFLASH_LOCK_REGION_SIZE	 (4096) // Internal FLASH Lock Region Size: 4 Kbytes
#define AT91C_IFLASH_NB_OF_PAGES	 (1024) // Internal FLASH Number of Pages: 1024 bytes
#define AT91C_IFLASH_NB_OF_LOCK_BITS	 (16) // Internal FLASH Number of Lock Bits: 16 bytes

#endif
