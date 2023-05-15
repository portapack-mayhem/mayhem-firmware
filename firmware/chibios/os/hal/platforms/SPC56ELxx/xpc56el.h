/****************************************************************************\
 * PROJECT     : MPC5643L
 * FILE        : mpc5643l.h
 *
 * DESCRIPTION : This is the header file describing the register
 *               set for the named projects.
 *
 * COPYRIGHT   : (c) 2009, Freescale Semiconductor & ST Microelectronics
 *
 * VERSION     : 1.01
 * DATE        : Thu Oct  8 13:53:51 CEST 2009
 * AUTHOR      : generated from IP-XACT database
 * HISTORY     : Preliminary release.
\****************************************************************************/

/*   >>>>  NOTE! this file is auto-generated please do not edit it!  <<<<   */

/****************************************************************************\
 * Example instantiation and use:
 *
 *  <MODULE>.<REGISTER>.B.<BIT> = 1;
 *  <MODULE>.<REGISTER>.R       = 0x10000000;
 *
\****************************************************************************/

#ifndef _leopard_H_ /* prevents multiple inclusions of this file */
#define _leopard_H_

#include "typedefs.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __MWERKS__
#pragma push
#pragma ANSI_strict off
#endif

/* #define USE_FIELD_ALIASES_CFLASH */
/* #define USE_FIELD_ALIASES_SIUL */
/* #define USE_FIELD_ALIASES_SSCM */
/* #define USE_FIELD_ALIASES_ME */
/* #define USE_FIELD_ALIASES_RGM */
/* #define USE_FIELD_ALIASES_ADC */
/* #define USE_FIELD_ALIASES_CTU */
/* #define USE_FIELD_ALIASES_mcTIMER */
/* #define USE_FIELD_ALIASES_mcPWM */
/* #define USE_FIELD_ALIASES_LINFLEX */
/* #define USE_FIELD_ALIASES_SPP_MCM */
/* #define USE_FIELD_ALIASES_INTC */
/* #define USE_FIELD_ALIASES_DSPI */
/* #define USE_FIELD_ALIASES_FLEXCAN */
/* #define USE_FIELD_ALIASES_FR */

/****************************************************************/
/*                                                              */
/* Global definitions and aliases */
/*                                                              */
/****************************************************************/

/*
	 Platform blocks that are only accessible by the second core (core 1) when
		the device is in DPM mode. The block definition is equivalent to the one
		for the first core (core 0) and reuses the related block structure.

	 NOTE: the <block_name>_1 defines are the preferred method for programming
 */
#define AIPS_1 (*(volatile struct AIPS_tag*)0x8FF00000UL)
#define MAX_1 (*(volatile struct MAX_tag*)0x8FF04000UL)
#define MPU_1 (*(volatile struct MPU_tag*)0x8FF10000UL)
#define SEMA4_1 (*(volatile struct SEMA4_tag*)0x8FF24000UL)
#define SWT_1 (*(volatile struct SWT_tag*)0x8FF38000UL)
#define STM_1 (*(volatile struct STM_tag*)0x8FF3C000UL)
#define SPP_MCM_1 (*(volatile struct SPP_MCM_tag*)0x8FF40000UL)
#define SPP_DMA2_1 (*(volatile struct SPP_DMA2_tag*)0x8FF44000UL)
#define INTC_1 (*(volatile struct INTC_tag*)0x8FF48000UL)

/*
	 Platform blocks that are only accessible by the second core (core 1) when
		the device is in DPM mode. The block definition is equivalent to the one
		for the first core (core 0) and reuses the related block structure.

	 NOTE: the <block_name>_DPM defines are deprecated, use <block_name>_1 for
				 programming the corresponding blocks for new code instead.
 */
#define AIPS_DPM AIPS_1
#define MAX_DPM MAX_1
#define MPU_DPM MPU_1
#define SEMA4_DPM SEMA4_1
#define SWT_DPM SWT_1
#define STM_DPM STM_1
#define SPP_MCM_DPM SPP_MCM_1
#define SPP_DMA2_DPM SPP_DMA2_1
#define INTC_DPM INTC_1

/* Aliases for Pictus Module names */
#define CAN_0 FLEXCAN_A
#define CAN_1 FLEXCAN_B
#define CTU_0 CTU
#define DFLASH CRC
#define DMAMUX DMA_CH_MUX
#define DSPI_0 DSPI_A
#define DSPI_1 DSPI_B
#define DSPI_2 DSPI_C
#define EDMA SPP_DMA2
#define ETIMER_0 mcTIMER0
#define ETIMER_1 mcTIMER1
#define FLEXPWM_0 mcPWM_A
#define FLEXPWM_1 mcPWM_B
#define LINFLEX_0 LINFLEX0
#define LINFLEX_1 LINFLEX1
#define MCM_ SPP_MCM
#define PIT PIT_RTI
#define SIU SIUL
#define WKUP WKPU
/****************************************************************/
/*                                                              */
/* Module: CFLASH_SHADOW  */
/*                                                              */
/****************************************************************/

/* Register layout for all registers NVPWD... */

typedef union { /* NVPWD0-1 - Non Volatile Private Censorship PassWorD Register
								 */
	vuint32_t R;
	struct {
		vuint32_t PWD : 32; /* PassWorD */
	} B;
} CFLASH_SHADOW_NVPWD_32B_tag;

/* Register layout for all registers NVSCI... */

typedef union { /* NVSCI - Non Volatile System Censoring Information Register */
	vuint32_t R;
	struct {
		vuint32_t SC : 16; /* Serial Censorship Control Word */
		vuint32_t CW : 16; /* Censorship Control Word */
	} B;
} CFLASH_SHADOW_NVSCI_32B_tag;

typedef union { /* Non Volatile LML Default Value */
	vuint32_t R;
} CFLASH_SHADOW_NVLML_32B_tag;

typedef union { /* Non Volatile HBL Default Value */
	vuint32_t R;
} CFLASH_SHADOW_NVHBL_32B_tag;

typedef union { /* Non Volatile SLL Default Value */
	vuint32_t R;
} CFLASH_SHADOW_NVSLL_32B_tag;

/* Register layout for all registers NVBIU... */

typedef union { /* Non Volatile Bus Interface Unit Register */
	vuint32_t R;
	struct {
		vuint32_t BI : 32; /* Bus interface Unit */
	} B;
} CFLASH_SHADOW_NVBIU_32B_tag;

typedef union { /* NVUSRO - Non Volatile USeR Options Register */
	vuint32_t R;
	struct {
		vuint32_t UO : 32; /* User Options */
	} B;
} CFLASH_SHADOW_NVUSRO_32B_tag;

typedef struct CFLASH_SHADOW_BIU_DEFAULTS_struct_tag {
	/* Non Volatile Bus Interface Unit Register */
	CFLASH_SHADOW_NVBIU_32B_tag NVBIU; /* relative offset: 0x0000 */
	int8_t CFLASH_SHADOW_BIU_DEFAULTS_reserved_0004[4];

} CFLASH_SHADOW_BIU_DEFAULTS_tag;

typedef struct CFLASH_SHADOW_struct_tag { /* start of CFLASH_SHADOW_tag */
	int8_t CFLASH_SHADOW_reserved_0000_C[15832];
	union {
		/* NVPWD0-1 - Non Volatile Private Censorship PassWorD Register */
		CFLASH_SHADOW_NVPWD_32B_tag NVPWD[2]; /* offset: 0x3DD8  (0x0004 x 2) */

		struct {
			/* NVPWD0-1 - Non Volatile Private Censorship PassWorD Register */
			CFLASH_SHADOW_NVPWD_32B_tag NVPWD0; /* offset: 0x3DD8 size: 32 bit */
			CFLASH_SHADOW_NVPWD_32B_tag NVPWD1; /* offset: 0x3DDC size: 32 bit */
		};
	};
	union {
		/* NVSCI - Non Volatile System Censoring Information Register */
		CFLASH_SHADOW_NVSCI_32B_tag NVSCI[2]; /* offset: 0x3DE0  (0x0004 x 2) */

		struct {
			/* NVSCI - Non Volatile System Censoring Information Register */
			CFLASH_SHADOW_NVSCI_32B_tag NVSCI0; /* offset: 0x3DE0 size: 32 bit */
			CFLASH_SHADOW_NVSCI_32B_tag NVSCI1; /* offset: 0x3DE4 size: 32 bit */
		};
	};
	/* Non Volatile LML Default Value */
	CFLASH_SHADOW_NVLML_32B_tag NVLML; /* offset: 0x3DE8 size: 32 bit */
	int8_t CFLASH_SHADOW_reserved_3DEC[4];
	/* Non Volatile HBL Default Value */
	CFLASH_SHADOW_NVHBL_32B_tag NVHBL; /* offset: 0x3DF0 size: 32 bit */
	int8_t CFLASH_SHADOW_reserved_3DF4[4];
	/* Non Volatile SLL Default Value */
	CFLASH_SHADOW_NVSLL_32B_tag NVSLL; /* offset: 0x3DF8 size: 32 bit */
	int8_t CFLASH_SHADOW_reserved_3DFC_C[4];
	union {
		/*  Register set BIU_DEFAULTS */
		CFLASH_SHADOW_BIU_DEFAULTS_tag
				BIU_DEFAULTS[3]; /* offset: 0x3E00  (0x0008 x 3) */

		struct {
			/* Non Volatile Bus Interface Unit Register */
			CFLASH_SHADOW_NVBIU_32B_tag NVBIU2; /* offset: 0x3E00 size: 32 bit */
			int8_t CFLASH_SHADOW_reserved_3E04_I1[4];
			CFLASH_SHADOW_NVBIU_32B_tag NVBIU3; /* offset: 0x3E08 size: 32 bit */
			int8_t CFLASH_SHADOW_reserved_3E0C_I1[4];
			CFLASH_SHADOW_NVBIU_32B_tag NVBIU4; /* offset: 0x3E10 size: 32 bit */
			int8_t CFLASH_SHADOW_reserved_3E14_E1[4];
		};
	};
	/* NVUSRO - Non Volatile USeR Options Register */
	CFLASH_SHADOW_NVUSRO_32B_tag NVUSRO; /* offset: 0x3E18 size: 32 bit */
} CFLASH_SHADOW_tag;

#define CFLASH_SHADOW (*(volatile CFLASH_SHADOW_tag*)0x00F00000UL)

/****************************************************************/
/*                                                              */
/* Module: CFLASH  */
/*                                                              */
/****************************************************************/

typedef union { /* MCR - Module Configuration Register */
	vuint32_t R;
	struct {
		vuint32_t : 5;
		vuint32_t SIZE : 3; /* Array Space Size */
		vuint32_t : 1;
		vuint32_t LAS : 3; /* Low Address Space */
		vuint32_t : 3;
		vuint32_t MAS : 1; /* Mid Address Space Configuration */
		vuint32_t EER : 1; /* ECC Event Error */
		vuint32_t RWE : 1; /* Read-while-Write Event Error */
		vuint32_t SBC : 1; /* Single Bit Correction */
		vuint32_t : 1;
		vuint32_t PEAS : 1; /* Program/Erase Access Space */
		vuint32_t DONE : 1; /* modify operation DONE */
		vuint32_t PEG : 1;	/* Program/Erase Good */
		vuint32_t : 4;
		vuint32_t PGM : 1;	/* Program Bit */
		vuint32_t PSUS : 1; /* Program Suspend */
		vuint32_t ERS : 1;	/* Erase Bit */
		vuint32_t ESUS : 1; /* Erase Suspend */
		vuint32_t EHV : 1;	/* Enable High Voltage */
	} B;
} CFLASH_MCR_32B_tag;

typedef union { /* LML - Low/Mid Address Space Block Locking Register */
	vuint32_t R;
	struct {
		vuint32_t LME : 1; /* Low/Mid Address Space Block Enable */
		vuint32_t : 10;
#ifndef USE_FIELD_ALIASES_CFLASH
		vuint32_t SLOCK : 1; /* Shadow Address Space Block Lock */
#else
		vuint32_t TSLK : 1;				/* deprecated name - please avoid */
#endif
		vuint32_t : 2;
#ifndef USE_FIELD_ALIASES_CFLASH
		vuint32_t MLOCK : 2; /* Mid Address Space Block Lock */
#else
		vuint32_t MLK : 2;				/* deprecated name - please avoid */
#endif
		vuint32_t : 6;
		vuint32_t LLOCK : 10; /* Low Address Space Block Lock */
	} B;
} CFLASH_LML_32B_tag;

typedef union { /* HBL - High Address Space Block Locking Register */
	vuint32_t R;
	struct {
		vuint32_t HBE : 1; /* High Address Space Block Enable */
		vuint32_t : 25;
		vuint32_t HLOCK : 6; /* High Address Space Block Lock */
	} B;
} CFLASH_HBL_32B_tag;

typedef union { /* SLL - Secondary Low/Mid Address Space Block Locking Register
								 */
	vuint32_t R;
	struct {
		vuint32_t SLE : 1; /* Secondary Low/Mid Address Space Block Enable */
		vuint32_t : 10;
#ifndef USE_FIELD_ALIASES_CFLASH
		vuint32_t SSLOCK : 1; /* Secondary Shadow Address Space Block Lock */
#else
		vuint32_t STSLK : 1;			/* deprecated name - please avoid */
#endif
		vuint32_t : 2;
#ifndef USE_FIELD_ALIASES_CFLASH
		vuint32_t SMLOCK : 2; /* Secondary Mid Address Space Block Lock */
#else
		vuint32_t SMK : 2;				/* deprecated name - please avoid */
#endif
		vuint32_t : 6;
		vuint32_t SLLOCK : 10; /* Secondary Low Address Space Block Lock */
	} B;
} CFLASH_SLL_32B_tag;

typedef union { /* LMS - Low/Mid Address Space Block Select Register */
	vuint32_t R;
	struct {
		vuint32_t : 14;
		vuint32_t MSL : 2; /* Mid Address Space Block Select */
		vuint32_t : 6;
		vuint32_t LSL : 10; /* Low Address Space Block Select */
	} B;
} CFLASH_LMS_32B_tag;

typedef union { /* HBS - High Address Space Block Select Register */
	vuint32_t R;
	struct {
		vuint32_t : 26;
		vuint32_t HSL : 6; /* High Address Space Block Select */
	} B;
} CFLASH_HBS_32B_tag;

typedef union { /* ADR - Address Register */
	vuint32_t R;
	struct {
		vuint32_t SAD : 1; /* Shadow Address */
		vuint32_t : 10;
		vuint32_t ADDR : 18; /* Address */
		vuint32_t : 3;
	} B;
} CFLASH_ADR_32B_tag;

typedef union { /* PFLASH2P_LCA_PFCR0 - Platform Flash Configuration Register 0
								 */
	vuint32_t R;
	struct {
#ifndef USE_FIELD_ALIASES_CFLASH
		vuint32_t B02_APC : 5; /* Bank0+2 Address Pipelining Control */
#else
		vuint32_t BK0_APC : 5;		/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_CFLASH
		vuint32_t B02_WWSC : 5; /* Bank0+2 Write Wait State Control */
#else
		vuint32_t BK0_WWSC : 5;		/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_CFLASH
		vuint32_t B02_RWSC : 5; /* Bank0+2 Read Wait State Control */
#else
		vuint32_t BK0_RWSC : 5;		/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_CFLASH
		vuint32_t B02_RWWC2 : 1; /* Bank 0+2 Read While Write Control, bit 2 */
#else
		vuint32_t BK0_RWWC2 : 1;	/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_CFLASH
		vuint32_t B02_RWWC1 : 1; /* Bank 0+2 Read While Write Control, bit 1 */
#else
		vuint32_t BK0_RWWC1 : 1;	/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_CFLASH
		vuint32_t B02_P1_BCFG : 2; /* Bank0+2 Port 1 Page Buffer Configuration */
#else
		vuint32_t B0_P1_BCFG : 2; /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_CFLASH
		vuint32_t B02_P1_DPFE : 1; /* Bank0+2 Port 1 Data Prefetch Enable */
#else
		vuint32_t B0_P1_DPFE : 1; /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_CFLASH
		vuint32_t B02_P1_IPFE : 1; /* Bank0+2 Port 1 Inst Prefetch Enable */
#else
		vuint32_t B0_P1_IPFE : 1; /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_CFLASH
		vuint32_t B02_P1_PFLM : 2; /* Bank0+2 Port 1 Prefetch Limit */
#else
		vuint32_t B0_P1_PFLM : 2; /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_CFLASH
		vuint32_t B02_P1_BFE : 1; /* Bank0+2 Port 1 Buffer Enable */
#else
		vuint32_t B0_P1_BFE : 1;	/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_CFLASH
		vuint32_t B02_RWWC0 : 1; /* Bank 0+2 Read While Write Control, bit 0 */
#else
		vuint32_t BK0_RWWC0 : 1;	/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_CFLASH
		vuint32_t B02_P0_BCFG : 2; /* Bank0+2 Port 0 Page Buffer Configuration */
#else
		vuint32_t B0_P0_BCFG : 2; /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_CFLASH
		vuint32_t B02_P0_DPFE : 1; /* Bank0+2 Port 0 Data Prefetch Enable */
#else
		vuint32_t B0_P0_DPFE : 1; /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_CFLASH
		vuint32_t B02_P0_IPFE : 1; /* Bank0+2 Port 0 Inst Prefetch Enable */
#else
		vuint32_t B0_P0_IPFE : 1; /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_CFLASH
		vuint32_t B02_P0_PFLM : 2; /* Bank0+2 Port 0 Prefetch Limit */
#else
		vuint32_t B0_P0_PFLM : 2; /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_CFLASH
		vuint32_t B02_P0_BFE : 1; /* Bank0+2 Port 0 Buffer Enable */
#else
		vuint32_t B0_P0_BFE : 1;	/* deprecated name - please avoid */
#endif
	} B;
} CFLASH_PFCR0_32B_tag;

/* Register layout for all registers BIU... */

typedef union { /* Bus Interface Unit Register */
	vuint32_t R;
} CFLASH_BIU_32B_tag;

typedef union { /* PFLASH2P_LCA_PFCR1 - Platform Flash Configuration Register 1
								 */
	vuint32_t R;
	struct {
#ifndef USE_FIELD_ALIASES_CFLASH
		vuint32_t B1_APC : 5;		/* Bank 1 Address Pipelining Control */
		vuint32_t B1_WWSC : 5;	/* Bank 1 Write Wait State Control */
		vuint32_t B1_RWSC : 5;	/* Bank 1 Read Wait State Control */
		vuint32_t B1_RWWC2 : 1; /* Bank1 Read While Write Control, bit 2 */
		vuint32_t B1_RWWC1 : 1; /* Bank1 Read While Write Control, bit 1 */
		vuint32_t : 6;
		vuint32_t B1_P1_BFE : 1; /* Bank 1 Port 1 Buffer Enable */
		vuint32_t B1_RWWC0 : 1;	 /* Bank1 Read While Write Control, bit 0 */
		vuint32_t : 6;
		vuint32_t B1_P0_BFE : 1; /* Bank 1 Port 0 Buffer Enable */
#else
		vuint32_t BK1_APC : 5;
		vuint32_t BK1_WWSC : 5;
		vuint32_t BK1_RWSC : 5;
		vuint32_t BK1_RWWC2 : 1;
		vuint32_t BK1_RWWC1 : 1;
		vuint32_t : 6;
		vuint32_t B0_P1_BFE : 1;
		vuint32_t BK1_RWWC0 : 1;
		vuint32_t : 6;
		vuint32_t B1_P0_BFE : 1;
#endif
	} B;
} CFLASH_PFCR1_32B_tag;

typedef union { /* PFLASH2P_LCA_PFAPR - Platform Flash Access Protection
									 Register */
	vuint32_t R;
	struct {
		vuint32_t : 6;
		vuint32_t ARBM : 2;	 /* Arbitration Mode */
		vuint32_t M7PFD : 1; /* Master x Prefetch Disable */
		vuint32_t M6PFD : 1; /* Master x Prefetch Disable */
		vuint32_t M5PFD : 1; /* Master x Prefetch Disable */
		vuint32_t M4PFD : 1; /* Master x Prefetch Disable */
		vuint32_t M3PFD : 1; /* Master x Prefetch Disable */
		vuint32_t M2PFD : 1; /* Master x Prefetch Disable */
		vuint32_t M1PFD : 1; /* Master x Prefetch Disable */
		vuint32_t M0PFD : 1; /* Master x Prefetch Disable */
		vuint32_t M7AP : 2;	 /* Master 7 Access Protection */
		vuint32_t M6AP : 2;	 /* Master 6 Access Protection */
		vuint32_t M5AP : 2;	 /* Master 5 Access Protection */
		vuint32_t M4AP : 2;	 /* Master 4 Access Protection */
		vuint32_t M3AP : 2;	 /* Master 3 Access Protection */
		vuint32_t M2AP : 2;	 /* Master 2 Access Protection */
		vuint32_t M1AP : 2;	 /* Master 1 Access Protection */
		vuint32_t M0AP : 2;	 /* Master 0 Access Protection */
	} B;
} CFLASH_PFAPR_32B_tag;

typedef union { /* UT0 - User Test Register */
	vuint32_t R;
	struct {
		vuint32_t UTE : 1;	/* User Test Enable */
		vuint32_t SBCE : 1; /* Single Bit Correction Enable */
		vuint32_t : 6;
		vuint32_t DSI : 8; /* Data Syndrome Input */
		vuint32_t : 10;
		vuint32_t MRE : 1; /* Margin Read Enable */
		vuint32_t MRV : 1; /* Margin Read Value */
		vuint32_t EIE : 1; /* ECC Data Input Enable */
		vuint32_t AIS : 1; /* Array Integrity Sequence */
		vuint32_t AIE : 1; /* Array Integrity Enable */
		vuint32_t AID : 1; /* Array Integrity Done */
	} B;
} CFLASH_UT0_32B_tag;

typedef union { /* UT1 - User Test Register */
	vuint32_t R;
} CFLASH_UT1_32B_tag;

typedef union { /* UT2 - User Test Register */
	vuint32_t R;
} CFLASH_UT2_32B_tag;

/* Register layout for all registers UM... */

typedef union { /* UM - User Multiple Input Signature Register */
	vuint32_t R;
	struct {
#ifndef USE_FIELD_ALIASES_CFLASH
		vuint32_t MISR : 32; /* Multiple Input Signature */
#else
		vuint32_t MS : 32;					 /* deprecated - please avoid */
#endif
	} B;
} CFLASH_UM_32B_tag;

/* Register layout for generated register(s) UT... */

typedef union { /*  */
	vuint32_t R;
} CFLASH_UT_32B_tag;

/* Register layout for generated register(s) PFCR... */

typedef union { /*  */
	vuint32_t R;
} CFLASH_PFCR_32B_tag;

typedef struct CFLASH_struct_tag { /* start of CFLASH_tag */
																	 /* MCR - Module Configuration Register */
	CFLASH_MCR_32B_tag MCR;					 /* offset: 0x0000 size: 32 bit */
	/* LML - Low/Mid Address Space Block Locking Register */
	CFLASH_LML_32B_tag LML; /* offset: 0x0004 size: 32 bit */
													/* HBL - High Address Space Block Locking Register */
	CFLASH_HBL_32B_tag HBL; /* offset: 0x0008 size: 32 bit */
	/* SLL - Secondary Low/Mid Address Space Block Locking Register */
	CFLASH_SLL_32B_tag SLL; /* offset: 0x000C size: 32 bit */
	/* LMS - Low/Mid Address Space Block Select Register */
	CFLASH_LMS_32B_tag LMS; /* offset: 0x0010 size: 32 bit */
													/* HBS - High Address Space Block Select Register */
	CFLASH_HBS_32B_tag HBS; /* offset: 0x0014 size: 32 bit */
													/* ADR - Address Register */
	CFLASH_ADR_32B_tag ADR; /* offset: 0x0018 size: 32 bit */
	union {
		struct {
			/*  */
			CFLASH_PFCR_32B_tag PFCR[2]; /* offset: 0x001C  (0x0004 x 2) */
			int8_t CFLASH_reserved_0024_E0[12];
		};

		/* Bus Interface Unit Register */
		CFLASH_BIU_32B_tag BIU[5]; /* offset: 0x001C  (0x0004 x 5) */

		struct {
			/* Bus Interface Unit Register */
			CFLASH_BIU_32B_tag BIU0; /* offset: 0x001C size: 32 bit */
			CFLASH_BIU_32B_tag BIU1; /* offset: 0x0020 size: 32 bit */
			CFLASH_BIU_32B_tag BIU2; /* offset: 0x0024 size: 32 bit */
			CFLASH_BIU_32B_tag BIU3; /* offset: 0x0028 size: 32 bit */
			CFLASH_BIU_32B_tag BIU4; /* offset: 0x002C size: 32 bit */
		};

		struct {
			int8_t CFLASH_reserved_001C_I3[8];
			CFLASH_PFAPR_32B_tag FAPR; /* deprecated - please avoid */
			int8_t CFLASH_reserved_0028_E3[8];
		};

		struct {
			/* PFLASH2P_LCA_PFCR0 - Platform Flash Configuration Register 0 */
			CFLASH_PFCR0_32B_tag PFCR0; /* offset: 0x001C size: 32 bit */
			/* PFLASH2P_LCA_PFCR1 - Platform Flash Configuration Register 1 */
			CFLASH_PFCR1_32B_tag PFCR1; /* offset: 0x0020 size: 32 bit */
			/* PFLASH2P_LCA_PFAPR - Platform Flash Access Protection Register */
			CFLASH_PFAPR_32B_tag PFAPR; /* offset: 0x0024 size: 32 bit */
			int8_t CFLASH_reserved_0028_E4[8];
		};
	};
	int8_t CFLASH_reserved_0030_C[12];
	union {
		CFLASH_UT_32B_tag UT[3]; /* offset: 0x003C  (0x0004 x 3) */

		struct {
			/* UT0 - User Test Register */
			CFLASH_UT0_32B_tag UT0; /* offset: 0x003C size: 32 bit */
															/* UT1 - User Test Register */
			CFLASH_UT1_32B_tag UT1; /* offset: 0x0040 size: 32 bit */
															/* UT2 - User Test Register */
			CFLASH_UT2_32B_tag UT2; /* offset: 0x0044 size: 32 bit */
		};
	};
	union {
		CFLASH_UM_32B_tag UMISR[5]; /* offset: 0x0048  (0x0004 x 5) */

		/* UM - User Multiple Input Signature Register */
		CFLASH_UM_32B_tag UM[5]; /* offset: 0x0048  (0x0004 x 5) */

		struct {
			/* UM - User Multiple Input Signature Register */
			CFLASH_UM_32B_tag UM0; /* offset: 0x0048 size: 32 bit */
			CFLASH_UM_32B_tag UM1; /* offset: 0x004C size: 32 bit */
			CFLASH_UM_32B_tag UM2; /* offset: 0x0050 size: 32 bit */
			CFLASH_UM_32B_tag UM3; /* offset: 0x0054 size: 32 bit */
			CFLASH_UM_32B_tag UM4; /* offset: 0x0058 size: 32 bit */
		};
	};
} CFLASH_tag;

#define CFLASH (*(volatile CFLASH_tag*)0xC3F88000UL)

/****************************************************************/
/*                                                              */
/* Module: SIUL  */
/*                                                              */
/****************************************************************/

typedef union { /* MIDR1 - MCU ID Register #1 */
	vuint32_t R;
	struct {
		vuint32_t PARTNUM : 16; /* MCU Part Number */
		vuint32_t CSP : 1;			/* CSP Package */
		vuint32_t PKG : 5;			/* Package Settings */
		vuint32_t : 2;
#ifndef USE_FIELD_ALIASES_SIUL
		vuint32_t MAJOR_MASK : 4; /* Major Mask Revision */
#else
		vuint32_t MAJORMASK : 4;		 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_SIUL
		vuint32_t MINOR_MASK : 4; /* Minor Mask Revision */
#else
		vuint32_t MINORMASK : 4;		 /* deprecated name - please avoid */
#endif
	} B;
} SIUL_MIDR1_32B_tag;

typedef union { /* MIDR2 - MCU ID Register #2 */
	vuint32_t R;
	struct {
		vuint32_t SF : 1;						/* Manufacturer */
		vuint32_t FLASH_SIZE_1 : 4; /* Coarse Flash Memory Size */
		vuint32_t FLASH_SIZE_2 : 4; /* Fine Flash Memory Size */
		vuint32_t : 7;
#ifndef USE_FIELD_ALIASES_SIUL
		vuint32_t PARTNUM2 : 8; /* MCU Part Number */
#else
		vuint32_t PARTNUM : 8;			 /* deprecated name - please avoid */
#endif
		vuint32_t TBD : 1; /* Optional Bit */
		vuint32_t : 2;
		vuint32_t EE : 1; /* Data Flash Present */
		vuint32_t : 3;
		vuint32_t FR : 1; /* Flexray Present */
	} B;
} SIUL_MIDR2_32B_tag;

typedef union { /* ISR - Interrupt Status Flag Register */
	vuint32_t R;
	struct {
		vuint32_t EIF31 : 1; /* External Interrupt Status Flag */
		vuint32_t EIF30 : 1; /* External Interrupt Status Flag */
		vuint32_t EIF29 : 1; /* External Interrupt Status Flag */
		vuint32_t EIF28 : 1; /* External Interrupt Status Flag */
		vuint32_t EIF27 : 1; /* External Interrupt Status Flag */
		vuint32_t EIF26 : 1; /* External Interrupt Status Flag */
		vuint32_t EIF25 : 1; /* External Interrupt Status Flag */
		vuint32_t EIF24 : 1; /* External Interrupt Status Flag */
		vuint32_t EIF23 : 1; /* External Interrupt Status Flag */
		vuint32_t EIF22 : 1; /* External Interrupt Status Flag */
		vuint32_t EIF21 : 1; /* External Interrupt Status Flag */
		vuint32_t EIF20 : 1; /* External Interrupt Status Flag */
		vuint32_t EIF19 : 1; /* External Interrupt Status Flag */
		vuint32_t EIF18 : 1; /* External Interrupt Status Flag */
		vuint32_t EIF17 : 1; /* External Interrupt Status Flag */
		vuint32_t EIF16 : 1; /* External Interrupt Status Flag */
		vuint32_t EIF15 : 1; /* External Interrupt Status Flag */
		vuint32_t EIF14 : 1; /* External Interrupt Status Flag */
		vuint32_t EIF13 : 1; /* External Interrupt Status Flag */
		vuint32_t EIF12 : 1; /* External Interrupt Status Flag */
		vuint32_t EIF11 : 1; /* External Interrupt Status Flag */
		vuint32_t EIF10 : 1; /* External Interrupt Status Flag */
		vuint32_t EIF9 : 1;	 /* External Interrupt Status Flag */
		vuint32_t EIF8 : 1;	 /* External Interrupt Status Flag */
		vuint32_t EIF7 : 1;	 /* External Interrupt Status Flag */
		vuint32_t EIF6 : 1;	 /* External Interrupt Status Flag */
		vuint32_t EIF5 : 1;	 /* External Interrupt Status Flag */
		vuint32_t EIF4 : 1;	 /* External Interrupt Status Flag */
		vuint32_t EIF3 : 1;	 /* External Interrupt Status Flag */
		vuint32_t EIF2 : 1;	 /* External Interrupt Status Flag */
		vuint32_t EIF1 : 1;	 /* External Interrupt Status Flag */
		vuint32_t EIF0 : 1;	 /* External Interrupt Status Flag */
	} B;
} SIUL_ISR_32B_tag;

typedef union { /* IRER - Interrupt Request Enable Register */
	vuint32_t R;
	struct {
		vuint32_t EIRE31 : 1; /* Enable External Interrupt Requests */
		vuint32_t EIRE30 : 1; /* Enable External Interrupt Requests */
		vuint32_t EIRE29 : 1; /* Enable External Interrupt Requests */
		vuint32_t EIRE28 : 1; /* Enable External Interrupt Requests */
		vuint32_t EIRE27 : 1; /* Enable External Interrupt Requests */
		vuint32_t EIRE26 : 1; /* Enable External Interrupt Requests */
		vuint32_t EIRE25 : 1; /* Enable External Interrupt Requests */
		vuint32_t EIRE24 : 1; /* Enable External Interrupt Requests */
		vuint32_t EIRE23 : 1; /* Enable External Interrupt Requests */
		vuint32_t EIRE22 : 1; /* Enable External Interrupt Requests */
		vuint32_t EIRE21 : 1; /* Enable External Interrupt Requests */
		vuint32_t EIRE20 : 1; /* Enable External Interrupt Requests */
		vuint32_t EIRE19 : 1; /* Enable External Interrupt Requests */
		vuint32_t EIRE18 : 1; /* Enable External Interrupt Requests */
		vuint32_t EIRE17 : 1; /* Enable External Interrupt Requests */
		vuint32_t EIRE16 : 1; /* Enable External Interrupt Requests */
		vuint32_t EIRE15 : 1; /* Enable External Interrupt Requests */
		vuint32_t EIRE14 : 1; /* Enable External Interrupt Requests */
		vuint32_t EIRE13 : 1; /* Enable External Interrupt Requests */
		vuint32_t EIRE12 : 1; /* Enable External Interrupt Requests */
		vuint32_t EIRE11 : 1; /* Enable External Interrupt Requests */
		vuint32_t EIRE10 : 1; /* Enable External Interrupt Requests */
		vuint32_t EIRE9 : 1;	/* Enable External Interrupt Requests */
		vuint32_t EIRE8 : 1;	/* Enable External Interrupt Requests */
		vuint32_t EIRE7 : 1;	/* Enable External Interrupt Requests */
		vuint32_t EIRE6 : 1;	/* Enable External Interrupt Requests */
		vuint32_t EIRE5 : 1;	/* Enable External Interrupt Requests */
		vuint32_t EIRE4 : 1;	/* Enable External Interrupt Requests */
		vuint32_t EIRE3 : 1;	/* Enable External Interrupt Requests */
		vuint32_t EIRE2 : 1;	/* Enable External Interrupt Requests */
		vuint32_t EIRE1 : 1;	/* Enable External Interrupt Requests */
		vuint32_t EIRE0 : 1;	/* Enable External Interrupt Requests */
	} B;
} SIUL_IRER_32B_tag;

typedef union { /* IREER - Interrupt Rising Edge Event Enable */
	vuint32_t R;
	struct {
		vuint32_t IREE31 : 1; /* Enable rising-edge events */
		vuint32_t IREE30 : 1; /* Enable rising-edge events */
		vuint32_t IREE29 : 1; /* Enable rising-edge events */
		vuint32_t IREE28 : 1; /* Enable rising-edge events */
		vuint32_t IREE27 : 1; /* Enable rising-edge events */
		vuint32_t IREE26 : 1; /* Enable rising-edge events */
		vuint32_t IREE25 : 1; /* Enable rising-edge events */
		vuint32_t IREE24 : 1; /* Enable rising-edge events */
		vuint32_t IREE23 : 1; /* Enable rising-edge events */
		vuint32_t IREE22 : 1; /* Enable rising-edge events */
		vuint32_t IREE21 : 1; /* Enable rising-edge events */
		vuint32_t IREE20 : 1; /* Enable rising-edge events */
		vuint32_t IREE19 : 1; /* Enable rising-edge events */
		vuint32_t IREE18 : 1; /* Enable rising-edge events */
		vuint32_t IREE17 : 1; /* Enable rising-edge events */
		vuint32_t IREE16 : 1; /* Enable rising-edge events */
		vuint32_t IREE15 : 1; /* Enable rising-edge events */
		vuint32_t IREE14 : 1; /* Enable rising-edge events */
		vuint32_t IREE13 : 1; /* Enable rising-edge events */
		vuint32_t IREE12 : 1; /* Enable rising-edge events */
		vuint32_t IREE11 : 1; /* Enable rising-edge events */
		vuint32_t IREE10 : 1; /* Enable rising-edge events */
		vuint32_t IREE9 : 1;	/* Enable rising-edge events */
		vuint32_t IREE8 : 1;	/* Enable rising-edge events */
		vuint32_t IREE7 : 1;	/* Enable rising-edge events */
		vuint32_t IREE6 : 1;	/* Enable rising-edge events */
		vuint32_t IREE5 : 1;	/* Enable rising-edge events */
		vuint32_t IREE4 : 1;	/* Enable rising-edge events */
		vuint32_t IREE3 : 1;	/* Enable rising-edge events */
		vuint32_t IREE2 : 1;	/* Enable rising-edge events */
		vuint32_t IREE1 : 1;	/* Enable rising-edge events */
		vuint32_t IREE0 : 1;	/* Enable rising-edge events */
	} B;
} SIUL_IREER_32B_tag;

typedef union { /* IFEER - Interrupt Falling-Edge Event Enable */
	vuint32_t R;
	struct {
		vuint32_t IFEE31 : 1; /* Enable Falling Edge Events */
		vuint32_t IFEE30 : 1; /* Enable Falling Edge Events */
		vuint32_t IFEE29 : 1; /* Enable Falling Edge Events */
		vuint32_t IFEE28 : 1; /* Enable Falling Edge Events */
		vuint32_t IFEE27 : 1; /* Enable Falling Edge Events */
		vuint32_t IFEE26 : 1; /* Enable Falling Edge Events */
		vuint32_t IFEE25 : 1; /* Enable Falling Edge Events */
		vuint32_t IFEE24 : 1; /* Enable Falling Edge Events */
		vuint32_t IFEE23 : 1; /* Enable Falling Edge Events */
		vuint32_t IFEE22 : 1; /* Enable Falling Edge Events */
		vuint32_t IFEE21 : 1; /* Enable Falling Edge Events */
		vuint32_t IFEE20 : 1; /* Enable Falling Edge Events */
		vuint32_t IFEE19 : 1; /* Enable Falling Edge Events */
		vuint32_t IFEE18 : 1; /* Enable Falling Edge Events */
		vuint32_t IFEE17 : 1; /* Enable Falling Edge Events */
		vuint32_t IFEE16 : 1; /* Enable Falling Edge Events */
		vuint32_t IFEE15 : 1; /* Enable Falling Edge Events */
		vuint32_t IFEE14 : 1; /* Enable Falling Edge Events */
		vuint32_t IFEE13 : 1; /* Enable Falling Edge Events */
		vuint32_t IFEE12 : 1; /* Enable Falling Edge Events */
		vuint32_t IFEE11 : 1; /* Enable Falling Edge Events */
		vuint32_t IFEE10 : 1; /* Enable Falling Edge Events */
		vuint32_t IFEE9 : 1;	/* Enable Falling Edge Events */
		vuint32_t IFEE8 : 1;	/* Enable Falling Edge Events */
		vuint32_t IFEE7 : 1;	/* Enable Falling Edge Events */
		vuint32_t IFEE6 : 1;	/* Enable Falling Edge Events */
		vuint32_t IFEE5 : 1;	/* Enable Falling Edge Events */
		vuint32_t IFEE4 : 1;	/* Enable Falling Edge Events */
		vuint32_t IFEE3 : 1;	/* Enable Falling Edge Events */
		vuint32_t IFEE2 : 1;	/* Enable Falling Edge Events */
		vuint32_t IFEE1 : 1;	/* Enable Falling Edge Events */
		vuint32_t IFEE0 : 1;	/* Enable Falling Edge Events */
	} B;
} SIUL_IFEER_32B_tag;

typedef union { /* IFER Interrupt Filter Enable Register */
	vuint32_t R;
	struct {
		vuint32_t IFE31 : 1; /* Enable Digital Glitch Filter */
		vuint32_t IFE30 : 1; /* Enable Digital Glitch Filter */
		vuint32_t IFE29 : 1; /* Enable Digital Glitch Filter */
		vuint32_t IFE28 : 1; /* Enable Digital Glitch Filter */
		vuint32_t IFE27 : 1; /* Enable Digital Glitch Filter */
		vuint32_t IFE26 : 1; /* Enable Digital Glitch Filter */
		vuint32_t IFE25 : 1; /* Enable Digital Glitch Filter */
		vuint32_t IFE24 : 1; /* Enable Digital Glitch Filter */
		vuint32_t IFE23 : 1; /* Enable Digital Glitch Filter */
		vuint32_t IFE22 : 1; /* Enable Digital Glitch Filter */
		vuint32_t IFE21 : 1; /* Enable Digital Glitch Filter */
		vuint32_t IFE20 : 1; /* Enable Digital Glitch Filter */
		vuint32_t IFE19 : 1; /* Enable Digital Glitch Filter */
		vuint32_t IFE18 : 1; /* Enable Digital Glitch Filter */
		vuint32_t IFE17 : 1; /* Enable Digital Glitch Filter */
		vuint32_t IFE16 : 1; /* Enable Digital Glitch Filter */
		vuint32_t IFE15 : 1; /* Enable Digital Glitch Filter */
		vuint32_t IFE14 : 1; /* Enable Digital Glitch Filter */
		vuint32_t IFE13 : 1; /* Enable Digital Glitch Filter */
		vuint32_t IFE12 : 1; /* Enable Digital Glitch Filter */
		vuint32_t IFE11 : 1; /* Enable Digital Glitch Filter */
		vuint32_t IFE10 : 1; /* Enable Digital Glitch Filter */
		vuint32_t IFE9 : 1;	 /* Enable Digital Glitch Filter */
		vuint32_t IFE8 : 1;	 /* Enable Digital Glitch Filter */
		vuint32_t IFE7 : 1;	 /* Enable Digital Glitch Filter */
		vuint32_t IFE6 : 1;	 /* Enable Digital Glitch Filter */
		vuint32_t IFE5 : 1;	 /* Enable Digital Glitch Filter */
		vuint32_t IFE4 : 1;	 /* Enable Digital Glitch Filter */
		vuint32_t IFE3 : 1;	 /* Enable Digital Glitch Filter */
		vuint32_t IFE2 : 1;	 /* Enable Digital Glitch Filter */
		vuint32_t IFE1 : 1;	 /* Enable Digital Glitch Filter */
		vuint32_t IFE0 : 1;	 /* Enable Digital Glitch Filter */
	} B;
} SIUL_IFER_32B_tag;

/* Register layout for all registers PCR... */

typedef union { /* PCR - Pad Configuration Register */
	vuint16_t R;
	struct {
		vuint16_t : 1;
#ifndef USE_FIELD_ALIASES_SIUL
		vuint16_t SMC : 1; /* Safe Mode Control */
#else
		vuint16_t SME : 1;					 /* deprecated name - please avoid */
#endif
		vuint16_t APC : 1; /* Analog Pad Control */
		vuint16_t : 1;
		vuint16_t PA : 2;	 /* Pad Output Assignment */
		vuint16_t OBE : 1; /* Output Buffer Enable */
		vuint16_t IBE : 1; /* Input Buffer Enable */
#ifndef USE_FIELD_ALIASES_SIUL
		vuint16_t DSC : 2; /* Drive Strength Control */
#else
		vuint16_t DCS : 2;					 /* deprecated name - please avoid */
#endif
		vuint16_t ODE : 1; /* Open Drain Output Enable */
		vuint16_t HYS : 1; /* Input Hysteresis */
		vuint16_t SRC : 2; /* Slew Rate Control */
		vuint16_t WPE : 1; /* Weak Pull Up/Down Enable */
		vuint16_t WPS : 1; /* Weak Pull Up/Down Select */
	} B;
} SIUL_PCR_16B_tag;

/* Register layout for all registers PSMI... */

typedef union { /* PSMI - Pad Selection for Multiplexed Inputs */
	vuint8_t R;
	struct {
		vuint8_t : 4;
		vuint8_t PADSEL : 4; /* Pad selection for pin */
	} B;
} SIUL_PSMI_8B_tag;

/* Register layout for all registers PSMI... */

typedef union { /* PSMI - Pad Selection for Multiplexed Inputs */
	vuint32_t R;
	struct {
		vuint32_t : 4;
		vuint32_t PADSEL0 : 4; /* Pad selection for pin */
		vuint32_t : 4;
		vuint32_t PADSEL1 : 4; /* Pad selection for pin */
		vuint32_t : 4;
		vuint32_t PADSEL2 : 4; /* Pad selection for pin */
		vuint32_t : 4;
		vuint32_t PADSEL3 : 4; /* Pad selection for pin */
	} B;
} SIUL_PSMI_32B_tag;

/* Register layout for all registers GPDO... */

typedef union { /* GPDO - GPIO Pad Data Output Register */
	vuint8_t R;
	struct {
		vuint8_t : 7;
		vuint8_t PDO : 1; /* Pad Data Out */
	} B;
} SIUL_GPDO_8B_tag;

/* Register layout for all registers GPDO... */

typedef union { /* GPDO - GPIO Pad Data Output Register */
	vuint32_t R;
	struct {
		vuint32_t : 7;
		vuint32_t PDO0 : 1; /* Pad Data Out */
		vuint32_t : 7;
		vuint32_t PDO1 : 1; /* Pad Data Out */
		vuint32_t : 7;
		vuint32_t PDO2 : 1; /* Pad Data Out */
		vuint32_t : 7;
		vuint32_t PDO3 : 1; /* Pad Data Out */
	} B;
} SIUL_GPDO_32B_tag;

/* Register layout for all registers GPDI... */

typedef union { /* GPDI - GPIO Pad Data Input Register */
	vuint8_t R;
	struct {
		vuint8_t : 7;
		vuint8_t PDI : 1; /* Pad Data In */
	} B;
} SIUL_GPDI_8B_tag;

/* Register layout for all registers GPDI... */

typedef union { /* GPDI - GPIO Pad Data Input Register */
	vuint32_t R;
	struct {
		vuint32_t : 7;
		vuint32_t PDI0 : 1; /* Pad Data In */
		vuint32_t : 7;
		vuint32_t PDI1 : 1; /* Pad Data In */
		vuint32_t : 7;
		vuint32_t PDI2 : 1; /* Pad Data In */
		vuint32_t : 7;
		vuint32_t PDI3 : 1; /* Pad Data In */
	} B;
} SIUL_GPDI_32B_tag;

/* Register layout for all registers PGPDO... */

typedef union { /* PGPDO - Parallel GPIO Pad Data Out Register */
	vuint16_t R;
} SIUL_PGPDO_16B_tag;

/* Register layout for all registers PGPDI... */

typedef union { /* PGPDI - Parallel GPIO Pad Data In Register */
	vuint16_t R;
} SIUL_PGPDI_16B_tag;

/* Register layout for all registers MPGPDO... */

typedef union { /* MPGPDO - Masked Parallel GPIO Pad Data Out Register */
	vuint32_t R;
	struct {
		vuint32_t MASK : 16;	/* Mask Field */
		vuint32_t MPPDO : 16; /* Masked Parallel Pad Data Out */
	} B;
} SIUL_MPGPDO_32B_tag;

/* Register layout for all registers IFMC... */

typedef union { /* IFMC - Interrupt Filter Maximum Counter Register */
	vuint32_t R;
	struct {
		vuint32_t : 28;
		vuint32_t MAXCNT : 4; /* Maximum Interrupt Filter Counter Setting */
	} B;
} SIUL_IFMC_32B_tag;

typedef union { /* IFCPR - Inerrupt Filter Clock Prescaler Register */
	vuint32_t R;
	struct {
		vuint32_t : 28;
		vuint32_t IFCP : 4; /* Interrupt Filter Clock Prescaler Setting */
	} B;
} SIUL_IFCPR_32B_tag;

typedef struct SIUL_struct_tag { /* start of SIUL_tag */
	int8_t SIUL_reserved_0000_C[4];
	union {
		SIUL_MIDR1_32B_tag MIDR; /* deprecated - please avoid */

		/* MIDR1 - MCU ID Register #1 */
		SIUL_MIDR1_32B_tag MIDR1; /* offset: 0x0004 size: 32 bit */
	};
	/* MIDR2 - MCU ID Register #2 */
	SIUL_MIDR2_32B_tag MIDR2; /* offset: 0x0008 size: 32 bit */
	int8_t SIUL_reserved_000C[8];
	/* ISR - Interrupt Status Flag Register */
	SIUL_ISR_32B_tag ISR;		/* offset: 0x0014 size: 32 bit */
													/* IRER - Interrupt Request Enable Register */
	SIUL_IRER_32B_tag IRER; /* offset: 0x0018 size: 32 bit */
	int8_t SIUL_reserved_001C[12];
	/* IREER - Interrupt Rising Edge Event Enable */
	SIUL_IREER_32B_tag IREER; /* offset: 0x0028 size: 32 bit */
														/* IFEER - Interrupt Falling-Edge Event Enable */
	SIUL_IFEER_32B_tag IFEER; /* offset: 0x002C size: 32 bit */
														/* IFER Interrupt Filter Enable Register */
	SIUL_IFER_32B_tag IFER;		/* offset: 0x0030 size: 32 bit */
	int8_t SIUL_reserved_0034_C[12];
	union {
		/* PCR - Pad Configuration Register */
		SIUL_PCR_16B_tag PCR[512]; /* offset: 0x0040  (0x0002 x 512) */

		struct {
			/* PCR - Pad Configuration Register */
			SIUL_PCR_16B_tag PCR0;	 /* offset: 0x0040 size: 16 bit */
			SIUL_PCR_16B_tag PCR1;	 /* offset: 0x0042 size: 16 bit */
			SIUL_PCR_16B_tag PCR2;	 /* offset: 0x0044 size: 16 bit */
			SIUL_PCR_16B_tag PCR3;	 /* offset: 0x0046 size: 16 bit */
			SIUL_PCR_16B_tag PCR4;	 /* offset: 0x0048 size: 16 bit */
			SIUL_PCR_16B_tag PCR5;	 /* offset: 0x004A size: 16 bit */
			SIUL_PCR_16B_tag PCR6;	 /* offset: 0x004C size: 16 bit */
			SIUL_PCR_16B_tag PCR7;	 /* offset: 0x004E size: 16 bit */
			SIUL_PCR_16B_tag PCR8;	 /* offset: 0x0050 size: 16 bit */
			SIUL_PCR_16B_tag PCR9;	 /* offset: 0x0052 size: 16 bit */
			SIUL_PCR_16B_tag PCR10;	 /* offset: 0x0054 size: 16 bit */
			SIUL_PCR_16B_tag PCR11;	 /* offset: 0x0056 size: 16 bit */
			SIUL_PCR_16B_tag PCR12;	 /* offset: 0x0058 size: 16 bit */
			SIUL_PCR_16B_tag PCR13;	 /* offset: 0x005A size: 16 bit */
			SIUL_PCR_16B_tag PCR14;	 /* offset: 0x005C size: 16 bit */
			SIUL_PCR_16B_tag PCR15;	 /* offset: 0x005E size: 16 bit */
			SIUL_PCR_16B_tag PCR16;	 /* offset: 0x0060 size: 16 bit */
			SIUL_PCR_16B_tag PCR17;	 /* offset: 0x0062 size: 16 bit */
			SIUL_PCR_16B_tag PCR18;	 /* offset: 0x0064 size: 16 bit */
			SIUL_PCR_16B_tag PCR19;	 /* offset: 0x0066 size: 16 bit */
			SIUL_PCR_16B_tag PCR20;	 /* offset: 0x0068 size: 16 bit */
			SIUL_PCR_16B_tag PCR21;	 /* offset: 0x006A size: 16 bit */
			SIUL_PCR_16B_tag PCR22;	 /* offset: 0x006C size: 16 bit */
			SIUL_PCR_16B_tag PCR23;	 /* offset: 0x006E size: 16 bit */
			SIUL_PCR_16B_tag PCR24;	 /* offset: 0x0070 size: 16 bit */
			SIUL_PCR_16B_tag PCR25;	 /* offset: 0x0072 size: 16 bit */
			SIUL_PCR_16B_tag PCR26;	 /* offset: 0x0074 size: 16 bit */
			SIUL_PCR_16B_tag PCR27;	 /* offset: 0x0076 size: 16 bit */
			SIUL_PCR_16B_tag PCR28;	 /* offset: 0x0078 size: 16 bit */
			SIUL_PCR_16B_tag PCR29;	 /* offset: 0x007A size: 16 bit */
			SIUL_PCR_16B_tag PCR30;	 /* offset: 0x007C size: 16 bit */
			SIUL_PCR_16B_tag PCR31;	 /* offset: 0x007E size: 16 bit */
			SIUL_PCR_16B_tag PCR32;	 /* offset: 0x0080 size: 16 bit */
			SIUL_PCR_16B_tag PCR33;	 /* offset: 0x0082 size: 16 bit */
			SIUL_PCR_16B_tag PCR34;	 /* offset: 0x0084 size: 16 bit */
			SIUL_PCR_16B_tag PCR35;	 /* offset: 0x0086 size: 16 bit */
			SIUL_PCR_16B_tag PCR36;	 /* offset: 0x0088 size: 16 bit */
			SIUL_PCR_16B_tag PCR37;	 /* offset: 0x008A size: 16 bit */
			SIUL_PCR_16B_tag PCR38;	 /* offset: 0x008C size: 16 bit */
			SIUL_PCR_16B_tag PCR39;	 /* offset: 0x008E size: 16 bit */
			SIUL_PCR_16B_tag PCR40;	 /* offset: 0x0090 size: 16 bit */
			SIUL_PCR_16B_tag PCR41;	 /* offset: 0x0092 size: 16 bit */
			SIUL_PCR_16B_tag PCR42;	 /* offset: 0x0094 size: 16 bit */
			SIUL_PCR_16B_tag PCR43;	 /* offset: 0x0096 size: 16 bit */
			SIUL_PCR_16B_tag PCR44;	 /* offset: 0x0098 size: 16 bit */
			SIUL_PCR_16B_tag PCR45;	 /* offset: 0x009A size: 16 bit */
			SIUL_PCR_16B_tag PCR46;	 /* offset: 0x009C size: 16 bit */
			SIUL_PCR_16B_tag PCR47;	 /* offset: 0x009E size: 16 bit */
			SIUL_PCR_16B_tag PCR48;	 /* offset: 0x00A0 size: 16 bit */
			SIUL_PCR_16B_tag PCR49;	 /* offset: 0x00A2 size: 16 bit */
			SIUL_PCR_16B_tag PCR50;	 /* offset: 0x00A4 size: 16 bit */
			SIUL_PCR_16B_tag PCR51;	 /* offset: 0x00A6 size: 16 bit */
			SIUL_PCR_16B_tag PCR52;	 /* offset: 0x00A8 size: 16 bit */
			SIUL_PCR_16B_tag PCR53;	 /* offset: 0x00AA size: 16 bit */
			SIUL_PCR_16B_tag PCR54;	 /* offset: 0x00AC size: 16 bit */
			SIUL_PCR_16B_tag PCR55;	 /* offset: 0x00AE size: 16 bit */
			SIUL_PCR_16B_tag PCR56;	 /* offset: 0x00B0 size: 16 bit */
			SIUL_PCR_16B_tag PCR57;	 /* offset: 0x00B2 size: 16 bit */
			SIUL_PCR_16B_tag PCR58;	 /* offset: 0x00B4 size: 16 bit */
			SIUL_PCR_16B_tag PCR59;	 /* offset: 0x00B6 size: 16 bit */
			SIUL_PCR_16B_tag PCR60;	 /* offset: 0x00B8 size: 16 bit */
			SIUL_PCR_16B_tag PCR61;	 /* offset: 0x00BA size: 16 bit */
			SIUL_PCR_16B_tag PCR62;	 /* offset: 0x00BC size: 16 bit */
			SIUL_PCR_16B_tag PCR63;	 /* offset: 0x00BE size: 16 bit */
			SIUL_PCR_16B_tag PCR64;	 /* offset: 0x00C0 size: 16 bit */
			SIUL_PCR_16B_tag PCR65;	 /* offset: 0x00C2 size: 16 bit */
			SIUL_PCR_16B_tag PCR66;	 /* offset: 0x00C4 size: 16 bit */
			SIUL_PCR_16B_tag PCR67;	 /* offset: 0x00C6 size: 16 bit */
			SIUL_PCR_16B_tag PCR68;	 /* offset: 0x00C8 size: 16 bit */
			SIUL_PCR_16B_tag PCR69;	 /* offset: 0x00CA size: 16 bit */
			SIUL_PCR_16B_tag PCR70;	 /* offset: 0x00CC size: 16 bit */
			SIUL_PCR_16B_tag PCR71;	 /* offset: 0x00CE size: 16 bit */
			SIUL_PCR_16B_tag PCR72;	 /* offset: 0x00D0 size: 16 bit */
			SIUL_PCR_16B_tag PCR73;	 /* offset: 0x00D2 size: 16 bit */
			SIUL_PCR_16B_tag PCR74;	 /* offset: 0x00D4 size: 16 bit */
			SIUL_PCR_16B_tag PCR75;	 /* offset: 0x00D6 size: 16 bit */
			SIUL_PCR_16B_tag PCR76;	 /* offset: 0x00D8 size: 16 bit */
			SIUL_PCR_16B_tag PCR77;	 /* offset: 0x00DA size: 16 bit */
			SIUL_PCR_16B_tag PCR78;	 /* offset: 0x00DC size: 16 bit */
			SIUL_PCR_16B_tag PCR79;	 /* offset: 0x00DE size: 16 bit */
			SIUL_PCR_16B_tag PCR80;	 /* offset: 0x00E0 size: 16 bit */
			SIUL_PCR_16B_tag PCR81;	 /* offset: 0x00E2 size: 16 bit */
			SIUL_PCR_16B_tag PCR82;	 /* offset: 0x00E4 size: 16 bit */
			SIUL_PCR_16B_tag PCR83;	 /* offset: 0x00E6 size: 16 bit */
			SIUL_PCR_16B_tag PCR84;	 /* offset: 0x00E8 size: 16 bit */
			SIUL_PCR_16B_tag PCR85;	 /* offset: 0x00EA size: 16 bit */
			SIUL_PCR_16B_tag PCR86;	 /* offset: 0x00EC size: 16 bit */
			SIUL_PCR_16B_tag PCR87;	 /* offset: 0x00EE size: 16 bit */
			SIUL_PCR_16B_tag PCR88;	 /* offset: 0x00F0 size: 16 bit */
			SIUL_PCR_16B_tag PCR89;	 /* offset: 0x00F2 size: 16 bit */
			SIUL_PCR_16B_tag PCR90;	 /* offset: 0x00F4 size: 16 bit */
			SIUL_PCR_16B_tag PCR91;	 /* offset: 0x00F6 size: 16 bit */
			SIUL_PCR_16B_tag PCR92;	 /* offset: 0x00F8 size: 16 bit */
			SIUL_PCR_16B_tag PCR93;	 /* offset: 0x00FA size: 16 bit */
			SIUL_PCR_16B_tag PCR94;	 /* offset: 0x00FC size: 16 bit */
			SIUL_PCR_16B_tag PCR95;	 /* offset: 0x00FE size: 16 bit */
			SIUL_PCR_16B_tag PCR96;	 /* offset: 0x0100 size: 16 bit */
			SIUL_PCR_16B_tag PCR97;	 /* offset: 0x0102 size: 16 bit */
			SIUL_PCR_16B_tag PCR98;	 /* offset: 0x0104 size: 16 bit */
			SIUL_PCR_16B_tag PCR99;	 /* offset: 0x0106 size: 16 bit */
			SIUL_PCR_16B_tag PCR100; /* offset: 0x0108 size: 16 bit */
			SIUL_PCR_16B_tag PCR101; /* offset: 0x010A size: 16 bit */
			SIUL_PCR_16B_tag PCR102; /* offset: 0x010C size: 16 bit */
			SIUL_PCR_16B_tag PCR103; /* offset: 0x010E size: 16 bit */
			SIUL_PCR_16B_tag PCR104; /* offset: 0x0110 size: 16 bit */
			SIUL_PCR_16B_tag PCR105; /* offset: 0x0112 size: 16 bit */
			SIUL_PCR_16B_tag PCR106; /* offset: 0x0114 size: 16 bit */
			SIUL_PCR_16B_tag PCR107; /* offset: 0x0116 size: 16 bit */
			SIUL_PCR_16B_tag PCR108; /* offset: 0x0118 size: 16 bit */
			SIUL_PCR_16B_tag PCR109; /* offset: 0x011A size: 16 bit */
			SIUL_PCR_16B_tag PCR110; /* offset: 0x011C size: 16 bit */
			SIUL_PCR_16B_tag PCR111; /* offset: 0x011E size: 16 bit */
			SIUL_PCR_16B_tag PCR112; /* offset: 0x0120 size: 16 bit */
			SIUL_PCR_16B_tag PCR113; /* offset: 0x0122 size: 16 bit */
			SIUL_PCR_16B_tag PCR114; /* offset: 0x0124 size: 16 bit */
			SIUL_PCR_16B_tag PCR115; /* offset: 0x0126 size: 16 bit */
			SIUL_PCR_16B_tag PCR116; /* offset: 0x0128 size: 16 bit */
			SIUL_PCR_16B_tag PCR117; /* offset: 0x012A size: 16 bit */
			SIUL_PCR_16B_tag PCR118; /* offset: 0x012C size: 16 bit */
			SIUL_PCR_16B_tag PCR119; /* offset: 0x012E size: 16 bit */
			SIUL_PCR_16B_tag PCR120; /* offset: 0x0130 size: 16 bit */
			SIUL_PCR_16B_tag PCR121; /* offset: 0x0132 size: 16 bit */
			SIUL_PCR_16B_tag PCR122; /* offset: 0x0134 size: 16 bit */
			SIUL_PCR_16B_tag PCR123; /* offset: 0x0136 size: 16 bit */
			SIUL_PCR_16B_tag PCR124; /* offset: 0x0138 size: 16 bit */
			SIUL_PCR_16B_tag PCR125; /* offset: 0x013A size: 16 bit */
			SIUL_PCR_16B_tag PCR126; /* offset: 0x013C size: 16 bit */
			SIUL_PCR_16B_tag PCR127; /* offset: 0x013E size: 16 bit */
			SIUL_PCR_16B_tag PCR128; /* offset: 0x0140 size: 16 bit */
			SIUL_PCR_16B_tag PCR129; /* offset: 0x0142 size: 16 bit */
			SIUL_PCR_16B_tag PCR130; /* offset: 0x0144 size: 16 bit */
			SIUL_PCR_16B_tag PCR131; /* offset: 0x0146 size: 16 bit */
			SIUL_PCR_16B_tag PCR132; /* offset: 0x0148 size: 16 bit */
			SIUL_PCR_16B_tag PCR133; /* offset: 0x014A size: 16 bit */
			SIUL_PCR_16B_tag PCR134; /* offset: 0x014C size: 16 bit */
			SIUL_PCR_16B_tag PCR135; /* offset: 0x014E size: 16 bit */
			SIUL_PCR_16B_tag PCR136; /* offset: 0x0150 size: 16 bit */
			SIUL_PCR_16B_tag PCR137; /* offset: 0x0152 size: 16 bit */
			SIUL_PCR_16B_tag PCR138; /* offset: 0x0154 size: 16 bit */
			SIUL_PCR_16B_tag PCR139; /* offset: 0x0156 size: 16 bit */
			SIUL_PCR_16B_tag PCR140; /* offset: 0x0158 size: 16 bit */
			SIUL_PCR_16B_tag PCR141; /* offset: 0x015A size: 16 bit */
			SIUL_PCR_16B_tag PCR142; /* offset: 0x015C size: 16 bit */
			SIUL_PCR_16B_tag PCR143; /* offset: 0x015E size: 16 bit */
			SIUL_PCR_16B_tag PCR144; /* offset: 0x0160 size: 16 bit */
			SIUL_PCR_16B_tag PCR145; /* offset: 0x0162 size: 16 bit */
			SIUL_PCR_16B_tag PCR146; /* offset: 0x0164 size: 16 bit */
			SIUL_PCR_16B_tag PCR147; /* offset: 0x0166 size: 16 bit */
			SIUL_PCR_16B_tag PCR148; /* offset: 0x0168 size: 16 bit */
			SIUL_PCR_16B_tag PCR149; /* offset: 0x016A size: 16 bit */
			SIUL_PCR_16B_tag PCR150; /* offset: 0x016C size: 16 bit */
			SIUL_PCR_16B_tag PCR151; /* offset: 0x016E size: 16 bit */
			SIUL_PCR_16B_tag PCR152; /* offset: 0x0170 size: 16 bit */
			SIUL_PCR_16B_tag PCR153; /* offset: 0x0172 size: 16 bit */
			SIUL_PCR_16B_tag PCR154; /* offset: 0x0174 size: 16 bit */
			SIUL_PCR_16B_tag PCR155; /* offset: 0x0176 size: 16 bit */
			SIUL_PCR_16B_tag PCR156; /* offset: 0x0178 size: 16 bit */
			SIUL_PCR_16B_tag PCR157; /* offset: 0x017A size: 16 bit */
			SIUL_PCR_16B_tag PCR158; /* offset: 0x017C size: 16 bit */
			SIUL_PCR_16B_tag PCR159; /* offset: 0x017E size: 16 bit */
			SIUL_PCR_16B_tag PCR160; /* offset: 0x0180 size: 16 bit */
			SIUL_PCR_16B_tag PCR161; /* offset: 0x0182 size: 16 bit */
			SIUL_PCR_16B_tag PCR162; /* offset: 0x0184 size: 16 bit */
			SIUL_PCR_16B_tag PCR163; /* offset: 0x0186 size: 16 bit */
			SIUL_PCR_16B_tag PCR164; /* offset: 0x0188 size: 16 bit */
			SIUL_PCR_16B_tag PCR165; /* offset: 0x018A size: 16 bit */
			SIUL_PCR_16B_tag PCR166; /* offset: 0x018C size: 16 bit */
			SIUL_PCR_16B_tag PCR167; /* offset: 0x018E size: 16 bit */
			SIUL_PCR_16B_tag PCR168; /* offset: 0x0190 size: 16 bit */
			SIUL_PCR_16B_tag PCR169; /* offset: 0x0192 size: 16 bit */
			SIUL_PCR_16B_tag PCR170; /* offset: 0x0194 size: 16 bit */
			SIUL_PCR_16B_tag PCR171; /* offset: 0x0196 size: 16 bit */
			SIUL_PCR_16B_tag PCR172; /* offset: 0x0198 size: 16 bit */
			SIUL_PCR_16B_tag PCR173; /* offset: 0x019A size: 16 bit */
			SIUL_PCR_16B_tag PCR174; /* offset: 0x019C size: 16 bit */
			SIUL_PCR_16B_tag PCR175; /* offset: 0x019E size: 16 bit */
			SIUL_PCR_16B_tag PCR176; /* offset: 0x01A0 size: 16 bit */
			SIUL_PCR_16B_tag PCR177; /* offset: 0x01A2 size: 16 bit */
			SIUL_PCR_16B_tag PCR178; /* offset: 0x01A4 size: 16 bit */
			SIUL_PCR_16B_tag PCR179; /* offset: 0x01A6 size: 16 bit */
			SIUL_PCR_16B_tag PCR180; /* offset: 0x01A8 size: 16 bit */
			SIUL_PCR_16B_tag PCR181; /* offset: 0x01AA size: 16 bit */
			SIUL_PCR_16B_tag PCR182; /* offset: 0x01AC size: 16 bit */
			SIUL_PCR_16B_tag PCR183; /* offset: 0x01AE size: 16 bit */
			SIUL_PCR_16B_tag PCR184; /* offset: 0x01B0 size: 16 bit */
			SIUL_PCR_16B_tag PCR185; /* offset: 0x01B2 size: 16 bit */
			SIUL_PCR_16B_tag PCR186; /* offset: 0x01B4 size: 16 bit */
			SIUL_PCR_16B_tag PCR187; /* offset: 0x01B6 size: 16 bit */
			SIUL_PCR_16B_tag PCR188; /* offset: 0x01B8 size: 16 bit */
			SIUL_PCR_16B_tag PCR189; /* offset: 0x01BA size: 16 bit */
			SIUL_PCR_16B_tag PCR190; /* offset: 0x01BC size: 16 bit */
			SIUL_PCR_16B_tag PCR191; /* offset: 0x01BE size: 16 bit */
			SIUL_PCR_16B_tag PCR192; /* offset: 0x01C0 size: 16 bit */
			SIUL_PCR_16B_tag PCR193; /* offset: 0x01C2 size: 16 bit */
			SIUL_PCR_16B_tag PCR194; /* offset: 0x01C4 size: 16 bit */
			SIUL_PCR_16B_tag PCR195; /* offset: 0x01C6 size: 16 bit */
			SIUL_PCR_16B_tag PCR196; /* offset: 0x01C8 size: 16 bit */
			SIUL_PCR_16B_tag PCR197; /* offset: 0x01CA size: 16 bit */
			SIUL_PCR_16B_tag PCR198; /* offset: 0x01CC size: 16 bit */
			SIUL_PCR_16B_tag PCR199; /* offset: 0x01CE size: 16 bit */
			SIUL_PCR_16B_tag PCR200; /* offset: 0x01D0 size: 16 bit */
			SIUL_PCR_16B_tag PCR201; /* offset: 0x01D2 size: 16 bit */
			SIUL_PCR_16B_tag PCR202; /* offset: 0x01D4 size: 16 bit */
			SIUL_PCR_16B_tag PCR203; /* offset: 0x01D6 size: 16 bit */
			SIUL_PCR_16B_tag PCR204; /* offset: 0x01D8 size: 16 bit */
			SIUL_PCR_16B_tag PCR205; /* offset: 0x01DA size: 16 bit */
			SIUL_PCR_16B_tag PCR206; /* offset: 0x01DC size: 16 bit */
			SIUL_PCR_16B_tag PCR207; /* offset: 0x01DE size: 16 bit */
			SIUL_PCR_16B_tag PCR208; /* offset: 0x01E0 size: 16 bit */
			SIUL_PCR_16B_tag PCR209; /* offset: 0x01E2 size: 16 bit */
			SIUL_PCR_16B_tag PCR210; /* offset: 0x01E4 size: 16 bit */
			SIUL_PCR_16B_tag PCR211; /* offset: 0x01E6 size: 16 bit */
			SIUL_PCR_16B_tag PCR212; /* offset: 0x01E8 size: 16 bit */
			SIUL_PCR_16B_tag PCR213; /* offset: 0x01EA size: 16 bit */
			SIUL_PCR_16B_tag PCR214; /* offset: 0x01EC size: 16 bit */
			SIUL_PCR_16B_tag PCR215; /* offset: 0x01EE size: 16 bit */
			SIUL_PCR_16B_tag PCR216; /* offset: 0x01F0 size: 16 bit */
			SIUL_PCR_16B_tag PCR217; /* offset: 0x01F2 size: 16 bit */
			SIUL_PCR_16B_tag PCR218; /* offset: 0x01F4 size: 16 bit */
			SIUL_PCR_16B_tag PCR219; /* offset: 0x01F6 size: 16 bit */
			SIUL_PCR_16B_tag PCR220; /* offset: 0x01F8 size: 16 bit */
			SIUL_PCR_16B_tag PCR221; /* offset: 0x01FA size: 16 bit */
			SIUL_PCR_16B_tag PCR222; /* offset: 0x01FC size: 16 bit */
			SIUL_PCR_16B_tag PCR223; /* offset: 0x01FE size: 16 bit */
			SIUL_PCR_16B_tag PCR224; /* offset: 0x0200 size: 16 bit */
			SIUL_PCR_16B_tag PCR225; /* offset: 0x0202 size: 16 bit */
			SIUL_PCR_16B_tag PCR226; /* offset: 0x0204 size: 16 bit */
			SIUL_PCR_16B_tag PCR227; /* offset: 0x0206 size: 16 bit */
			SIUL_PCR_16B_tag PCR228; /* offset: 0x0208 size: 16 bit */
			SIUL_PCR_16B_tag PCR229; /* offset: 0x020A size: 16 bit */
			SIUL_PCR_16B_tag PCR230; /* offset: 0x020C size: 16 bit */
			SIUL_PCR_16B_tag PCR231; /* offset: 0x020E size: 16 bit */
			SIUL_PCR_16B_tag PCR232; /* offset: 0x0210 size: 16 bit */
			SIUL_PCR_16B_tag PCR233; /* offset: 0x0212 size: 16 bit */
			SIUL_PCR_16B_tag PCR234; /* offset: 0x0214 size: 16 bit */
			SIUL_PCR_16B_tag PCR235; /* offset: 0x0216 size: 16 bit */
			SIUL_PCR_16B_tag PCR236; /* offset: 0x0218 size: 16 bit */
			SIUL_PCR_16B_tag PCR237; /* offset: 0x021A size: 16 bit */
			SIUL_PCR_16B_tag PCR238; /* offset: 0x021C size: 16 bit */
			SIUL_PCR_16B_tag PCR239; /* offset: 0x021E size: 16 bit */
			SIUL_PCR_16B_tag PCR240; /* offset: 0x0220 size: 16 bit */
			SIUL_PCR_16B_tag PCR241; /* offset: 0x0222 size: 16 bit */
			SIUL_PCR_16B_tag PCR242; /* offset: 0x0224 size: 16 bit */
			SIUL_PCR_16B_tag PCR243; /* offset: 0x0226 size: 16 bit */
			SIUL_PCR_16B_tag PCR244; /* offset: 0x0228 size: 16 bit */
			SIUL_PCR_16B_tag PCR245; /* offset: 0x022A size: 16 bit */
			SIUL_PCR_16B_tag PCR246; /* offset: 0x022C size: 16 bit */
			SIUL_PCR_16B_tag PCR247; /* offset: 0x022E size: 16 bit */
			SIUL_PCR_16B_tag PCR248; /* offset: 0x0230 size: 16 bit */
			SIUL_PCR_16B_tag PCR249; /* offset: 0x0232 size: 16 bit */
			SIUL_PCR_16B_tag PCR250; /* offset: 0x0234 size: 16 bit */
			SIUL_PCR_16B_tag PCR251; /* offset: 0x0236 size: 16 bit */
			SIUL_PCR_16B_tag PCR252; /* offset: 0x0238 size: 16 bit */
			SIUL_PCR_16B_tag PCR253; /* offset: 0x023A size: 16 bit */
			SIUL_PCR_16B_tag PCR254; /* offset: 0x023C size: 16 bit */
			SIUL_PCR_16B_tag PCR255; /* offset: 0x023E size: 16 bit */
			SIUL_PCR_16B_tag PCR256; /* offset: 0x0240 size: 16 bit */
			SIUL_PCR_16B_tag PCR257; /* offset: 0x0242 size: 16 bit */
			SIUL_PCR_16B_tag PCR258; /* offset: 0x0244 size: 16 bit */
			SIUL_PCR_16B_tag PCR259; /* offset: 0x0246 size: 16 bit */
			SIUL_PCR_16B_tag PCR260; /* offset: 0x0248 size: 16 bit */
			SIUL_PCR_16B_tag PCR261; /* offset: 0x024A size: 16 bit */
			SIUL_PCR_16B_tag PCR262; /* offset: 0x024C size: 16 bit */
			SIUL_PCR_16B_tag PCR263; /* offset: 0x024E size: 16 bit */
			SIUL_PCR_16B_tag PCR264; /* offset: 0x0250 size: 16 bit */
			SIUL_PCR_16B_tag PCR265; /* offset: 0x0252 size: 16 bit */
			SIUL_PCR_16B_tag PCR266; /* offset: 0x0254 size: 16 bit */
			SIUL_PCR_16B_tag PCR267; /* offset: 0x0256 size: 16 bit */
			SIUL_PCR_16B_tag PCR268; /* offset: 0x0258 size: 16 bit */
			SIUL_PCR_16B_tag PCR269; /* offset: 0x025A size: 16 bit */
			SIUL_PCR_16B_tag PCR270; /* offset: 0x025C size: 16 bit */
			SIUL_PCR_16B_tag PCR271; /* offset: 0x025E size: 16 bit */
			SIUL_PCR_16B_tag PCR272; /* offset: 0x0260 size: 16 bit */
			SIUL_PCR_16B_tag PCR273; /* offset: 0x0262 size: 16 bit */
			SIUL_PCR_16B_tag PCR274; /* offset: 0x0264 size: 16 bit */
			SIUL_PCR_16B_tag PCR275; /* offset: 0x0266 size: 16 bit */
			SIUL_PCR_16B_tag PCR276; /* offset: 0x0268 size: 16 bit */
			SIUL_PCR_16B_tag PCR277; /* offset: 0x026A size: 16 bit */
			SIUL_PCR_16B_tag PCR278; /* offset: 0x026C size: 16 bit */
			SIUL_PCR_16B_tag PCR279; /* offset: 0x026E size: 16 bit */
			SIUL_PCR_16B_tag PCR280; /* offset: 0x0270 size: 16 bit */
			SIUL_PCR_16B_tag PCR281; /* offset: 0x0272 size: 16 bit */
			SIUL_PCR_16B_tag PCR282; /* offset: 0x0274 size: 16 bit */
			SIUL_PCR_16B_tag PCR283; /* offset: 0x0276 size: 16 bit */
			SIUL_PCR_16B_tag PCR284; /* offset: 0x0278 size: 16 bit */
			SIUL_PCR_16B_tag PCR285; /* offset: 0x027A size: 16 bit */
			SIUL_PCR_16B_tag PCR286; /* offset: 0x027C size: 16 bit */
			SIUL_PCR_16B_tag PCR287; /* offset: 0x027E size: 16 bit */
			SIUL_PCR_16B_tag PCR288; /* offset: 0x0280 size: 16 bit */
			SIUL_PCR_16B_tag PCR289; /* offset: 0x0282 size: 16 bit */
			SIUL_PCR_16B_tag PCR290; /* offset: 0x0284 size: 16 bit */
			SIUL_PCR_16B_tag PCR291; /* offset: 0x0286 size: 16 bit */
			SIUL_PCR_16B_tag PCR292; /* offset: 0x0288 size: 16 bit */
			SIUL_PCR_16B_tag PCR293; /* offset: 0x028A size: 16 bit */
			SIUL_PCR_16B_tag PCR294; /* offset: 0x028C size: 16 bit */
			SIUL_PCR_16B_tag PCR295; /* offset: 0x028E size: 16 bit */
			SIUL_PCR_16B_tag PCR296; /* offset: 0x0290 size: 16 bit */
			SIUL_PCR_16B_tag PCR297; /* offset: 0x0292 size: 16 bit */
			SIUL_PCR_16B_tag PCR298; /* offset: 0x0294 size: 16 bit */
			SIUL_PCR_16B_tag PCR299; /* offset: 0x0296 size: 16 bit */
			SIUL_PCR_16B_tag PCR300; /* offset: 0x0298 size: 16 bit */
			SIUL_PCR_16B_tag PCR301; /* offset: 0x029A size: 16 bit */
			SIUL_PCR_16B_tag PCR302; /* offset: 0x029C size: 16 bit */
			SIUL_PCR_16B_tag PCR303; /* offset: 0x029E size: 16 bit */
			SIUL_PCR_16B_tag PCR304; /* offset: 0x02A0 size: 16 bit */
			SIUL_PCR_16B_tag PCR305; /* offset: 0x02A2 size: 16 bit */
			SIUL_PCR_16B_tag PCR306; /* offset: 0x02A4 size: 16 bit */
			SIUL_PCR_16B_tag PCR307; /* offset: 0x02A6 size: 16 bit */
			SIUL_PCR_16B_tag PCR308; /* offset: 0x02A8 size: 16 bit */
			SIUL_PCR_16B_tag PCR309; /* offset: 0x02AA size: 16 bit */
			SIUL_PCR_16B_tag PCR310; /* offset: 0x02AC size: 16 bit */
			SIUL_PCR_16B_tag PCR311; /* offset: 0x02AE size: 16 bit */
			SIUL_PCR_16B_tag PCR312; /* offset: 0x02B0 size: 16 bit */
			SIUL_PCR_16B_tag PCR313; /* offset: 0x02B2 size: 16 bit */
			SIUL_PCR_16B_tag PCR314; /* offset: 0x02B4 size: 16 bit */
			SIUL_PCR_16B_tag PCR315; /* offset: 0x02B6 size: 16 bit */
			SIUL_PCR_16B_tag PCR316; /* offset: 0x02B8 size: 16 bit */
			SIUL_PCR_16B_tag PCR317; /* offset: 0x02BA size: 16 bit */
			SIUL_PCR_16B_tag PCR318; /* offset: 0x02BC size: 16 bit */
			SIUL_PCR_16B_tag PCR319; /* offset: 0x02BE size: 16 bit */
			SIUL_PCR_16B_tag PCR320; /* offset: 0x02C0 size: 16 bit */
			SIUL_PCR_16B_tag PCR321; /* offset: 0x02C2 size: 16 bit */
			SIUL_PCR_16B_tag PCR322; /* offset: 0x02C4 size: 16 bit */
			SIUL_PCR_16B_tag PCR323; /* offset: 0x02C6 size: 16 bit */
			SIUL_PCR_16B_tag PCR324; /* offset: 0x02C8 size: 16 bit */
			SIUL_PCR_16B_tag PCR325; /* offset: 0x02CA size: 16 bit */
			SIUL_PCR_16B_tag PCR326; /* offset: 0x02CC size: 16 bit */
			SIUL_PCR_16B_tag PCR327; /* offset: 0x02CE size: 16 bit */
			SIUL_PCR_16B_tag PCR328; /* offset: 0x02D0 size: 16 bit */
			SIUL_PCR_16B_tag PCR329; /* offset: 0x02D2 size: 16 bit */
			SIUL_PCR_16B_tag PCR330; /* offset: 0x02D4 size: 16 bit */
			SIUL_PCR_16B_tag PCR331; /* offset: 0x02D6 size: 16 bit */
			SIUL_PCR_16B_tag PCR332; /* offset: 0x02D8 size: 16 bit */
			SIUL_PCR_16B_tag PCR333; /* offset: 0x02DA size: 16 bit */
			SIUL_PCR_16B_tag PCR334; /* offset: 0x02DC size: 16 bit */
			SIUL_PCR_16B_tag PCR335; /* offset: 0x02DE size: 16 bit */
			SIUL_PCR_16B_tag PCR336; /* offset: 0x02E0 size: 16 bit */
			SIUL_PCR_16B_tag PCR337; /* offset: 0x02E2 size: 16 bit */
			SIUL_PCR_16B_tag PCR338; /* offset: 0x02E4 size: 16 bit */
			SIUL_PCR_16B_tag PCR339; /* offset: 0x02E6 size: 16 bit */
			SIUL_PCR_16B_tag PCR340; /* offset: 0x02E8 size: 16 bit */
			SIUL_PCR_16B_tag PCR341; /* offset: 0x02EA size: 16 bit */
			SIUL_PCR_16B_tag PCR342; /* offset: 0x02EC size: 16 bit */
			SIUL_PCR_16B_tag PCR343; /* offset: 0x02EE size: 16 bit */
			SIUL_PCR_16B_tag PCR344; /* offset: 0x02F0 size: 16 bit */
			SIUL_PCR_16B_tag PCR345; /* offset: 0x02F2 size: 16 bit */
			SIUL_PCR_16B_tag PCR346; /* offset: 0x02F4 size: 16 bit */
			SIUL_PCR_16B_tag PCR347; /* offset: 0x02F6 size: 16 bit */
			SIUL_PCR_16B_tag PCR348; /* offset: 0x02F8 size: 16 bit */
			SIUL_PCR_16B_tag PCR349; /* offset: 0x02FA size: 16 bit */
			SIUL_PCR_16B_tag PCR350; /* offset: 0x02FC size: 16 bit */
			SIUL_PCR_16B_tag PCR351; /* offset: 0x02FE size: 16 bit */
			SIUL_PCR_16B_tag PCR352; /* offset: 0x0300 size: 16 bit */
			SIUL_PCR_16B_tag PCR353; /* offset: 0x0302 size: 16 bit */
			SIUL_PCR_16B_tag PCR354; /* offset: 0x0304 size: 16 bit */
			SIUL_PCR_16B_tag PCR355; /* offset: 0x0306 size: 16 bit */
			SIUL_PCR_16B_tag PCR356; /* offset: 0x0308 size: 16 bit */
			SIUL_PCR_16B_tag PCR357; /* offset: 0x030A size: 16 bit */
			SIUL_PCR_16B_tag PCR358; /* offset: 0x030C size: 16 bit */
			SIUL_PCR_16B_tag PCR359; /* offset: 0x030E size: 16 bit */
			SIUL_PCR_16B_tag PCR360; /* offset: 0x0310 size: 16 bit */
			SIUL_PCR_16B_tag PCR361; /* offset: 0x0312 size: 16 bit */
			SIUL_PCR_16B_tag PCR362; /* offset: 0x0314 size: 16 bit */
			SIUL_PCR_16B_tag PCR363; /* offset: 0x0316 size: 16 bit */
			SIUL_PCR_16B_tag PCR364; /* offset: 0x0318 size: 16 bit */
			SIUL_PCR_16B_tag PCR365; /* offset: 0x031A size: 16 bit */
			SIUL_PCR_16B_tag PCR366; /* offset: 0x031C size: 16 bit */
			SIUL_PCR_16B_tag PCR367; /* offset: 0x031E size: 16 bit */
			SIUL_PCR_16B_tag PCR368; /* offset: 0x0320 size: 16 bit */
			SIUL_PCR_16B_tag PCR369; /* offset: 0x0322 size: 16 bit */
			SIUL_PCR_16B_tag PCR370; /* offset: 0x0324 size: 16 bit */
			SIUL_PCR_16B_tag PCR371; /* offset: 0x0326 size: 16 bit */
			SIUL_PCR_16B_tag PCR372; /* offset: 0x0328 size: 16 bit */
			SIUL_PCR_16B_tag PCR373; /* offset: 0x032A size: 16 bit */
			SIUL_PCR_16B_tag PCR374; /* offset: 0x032C size: 16 bit */
			SIUL_PCR_16B_tag PCR375; /* offset: 0x032E size: 16 bit */
			SIUL_PCR_16B_tag PCR376; /* offset: 0x0330 size: 16 bit */
			SIUL_PCR_16B_tag PCR377; /* offset: 0x0332 size: 16 bit */
			SIUL_PCR_16B_tag PCR378; /* offset: 0x0334 size: 16 bit */
			SIUL_PCR_16B_tag PCR379; /* offset: 0x0336 size: 16 bit */
			SIUL_PCR_16B_tag PCR380; /* offset: 0x0338 size: 16 bit */
			SIUL_PCR_16B_tag PCR381; /* offset: 0x033A size: 16 bit */
			SIUL_PCR_16B_tag PCR382; /* offset: 0x033C size: 16 bit */
			SIUL_PCR_16B_tag PCR383; /* offset: 0x033E size: 16 bit */
			SIUL_PCR_16B_tag PCR384; /* offset: 0x0340 size: 16 bit */
			SIUL_PCR_16B_tag PCR385; /* offset: 0x0342 size: 16 bit */
			SIUL_PCR_16B_tag PCR386; /* offset: 0x0344 size: 16 bit */
			SIUL_PCR_16B_tag PCR387; /* offset: 0x0346 size: 16 bit */
			SIUL_PCR_16B_tag PCR388; /* offset: 0x0348 size: 16 bit */
			SIUL_PCR_16B_tag PCR389; /* offset: 0x034A size: 16 bit */
			SIUL_PCR_16B_tag PCR390; /* offset: 0x034C size: 16 bit */
			SIUL_PCR_16B_tag PCR391; /* offset: 0x034E size: 16 bit */
			SIUL_PCR_16B_tag PCR392; /* offset: 0x0350 size: 16 bit */
			SIUL_PCR_16B_tag PCR393; /* offset: 0x0352 size: 16 bit */
			SIUL_PCR_16B_tag PCR394; /* offset: 0x0354 size: 16 bit */
			SIUL_PCR_16B_tag PCR395; /* offset: 0x0356 size: 16 bit */
			SIUL_PCR_16B_tag PCR396; /* offset: 0x0358 size: 16 bit */
			SIUL_PCR_16B_tag PCR397; /* offset: 0x035A size: 16 bit */
			SIUL_PCR_16B_tag PCR398; /* offset: 0x035C size: 16 bit */
			SIUL_PCR_16B_tag PCR399; /* offset: 0x035E size: 16 bit */
			SIUL_PCR_16B_tag PCR400; /* offset: 0x0360 size: 16 bit */
			SIUL_PCR_16B_tag PCR401; /* offset: 0x0362 size: 16 bit */
			SIUL_PCR_16B_tag PCR402; /* offset: 0x0364 size: 16 bit */
			SIUL_PCR_16B_tag PCR403; /* offset: 0x0366 size: 16 bit */
			SIUL_PCR_16B_tag PCR404; /* offset: 0x0368 size: 16 bit */
			SIUL_PCR_16B_tag PCR405; /* offset: 0x036A size: 16 bit */
			SIUL_PCR_16B_tag PCR406; /* offset: 0x036C size: 16 bit */
			SIUL_PCR_16B_tag PCR407; /* offset: 0x036E size: 16 bit */
			SIUL_PCR_16B_tag PCR408; /* offset: 0x0370 size: 16 bit */
			SIUL_PCR_16B_tag PCR409; /* offset: 0x0372 size: 16 bit */
			SIUL_PCR_16B_tag PCR410; /* offset: 0x0374 size: 16 bit */
			SIUL_PCR_16B_tag PCR411; /* offset: 0x0376 size: 16 bit */
			SIUL_PCR_16B_tag PCR412; /* offset: 0x0378 size: 16 bit */
			SIUL_PCR_16B_tag PCR413; /* offset: 0x037A size: 16 bit */
			SIUL_PCR_16B_tag PCR414; /* offset: 0x037C size: 16 bit */
			SIUL_PCR_16B_tag PCR415; /* offset: 0x037E size: 16 bit */
			SIUL_PCR_16B_tag PCR416; /* offset: 0x0380 size: 16 bit */
			SIUL_PCR_16B_tag PCR417; /* offset: 0x0382 size: 16 bit */
			SIUL_PCR_16B_tag PCR418; /* offset: 0x0384 size: 16 bit */
			SIUL_PCR_16B_tag PCR419; /* offset: 0x0386 size: 16 bit */
			SIUL_PCR_16B_tag PCR420; /* offset: 0x0388 size: 16 bit */
			SIUL_PCR_16B_tag PCR421; /* offset: 0x038A size: 16 bit */
			SIUL_PCR_16B_tag PCR422; /* offset: 0x038C size: 16 bit */
			SIUL_PCR_16B_tag PCR423; /* offset: 0x038E size: 16 bit */
			SIUL_PCR_16B_tag PCR424; /* offset: 0x0390 size: 16 bit */
			SIUL_PCR_16B_tag PCR425; /* offset: 0x0392 size: 16 bit */
			SIUL_PCR_16B_tag PCR426; /* offset: 0x0394 size: 16 bit */
			SIUL_PCR_16B_tag PCR427; /* offset: 0x0396 size: 16 bit */
			SIUL_PCR_16B_tag PCR428; /* offset: 0x0398 size: 16 bit */
			SIUL_PCR_16B_tag PCR429; /* offset: 0x039A size: 16 bit */
			SIUL_PCR_16B_tag PCR430; /* offset: 0x039C size: 16 bit */
			SIUL_PCR_16B_tag PCR431; /* offset: 0x039E size: 16 bit */
			SIUL_PCR_16B_tag PCR432; /* offset: 0x03A0 size: 16 bit */
			SIUL_PCR_16B_tag PCR433; /* offset: 0x03A2 size: 16 bit */
			SIUL_PCR_16B_tag PCR434; /* offset: 0x03A4 size: 16 bit */
			SIUL_PCR_16B_tag PCR435; /* offset: 0x03A6 size: 16 bit */
			SIUL_PCR_16B_tag PCR436; /* offset: 0x03A8 size: 16 bit */
			SIUL_PCR_16B_tag PCR437; /* offset: 0x03AA size: 16 bit */
			SIUL_PCR_16B_tag PCR438; /* offset: 0x03AC size: 16 bit */
			SIUL_PCR_16B_tag PCR439; /* offset: 0x03AE size: 16 bit */
			SIUL_PCR_16B_tag PCR440; /* offset: 0x03B0 size: 16 bit */
			SIUL_PCR_16B_tag PCR441; /* offset: 0x03B2 size: 16 bit */
			SIUL_PCR_16B_tag PCR442; /* offset: 0x03B4 size: 16 bit */
			SIUL_PCR_16B_tag PCR443; /* offset: 0x03B6 size: 16 bit */
			SIUL_PCR_16B_tag PCR444; /* offset: 0x03B8 size: 16 bit */
			SIUL_PCR_16B_tag PCR445; /* offset: 0x03BA size: 16 bit */
			SIUL_PCR_16B_tag PCR446; /* offset: 0x03BC size: 16 bit */
			SIUL_PCR_16B_tag PCR447; /* offset: 0x03BE size: 16 bit */
			SIUL_PCR_16B_tag PCR448; /* offset: 0x03C0 size: 16 bit */
			SIUL_PCR_16B_tag PCR449; /* offset: 0x03C2 size: 16 bit */
			SIUL_PCR_16B_tag PCR450; /* offset: 0x03C4 size: 16 bit */
			SIUL_PCR_16B_tag PCR451; /* offset: 0x03C6 size: 16 bit */
			SIUL_PCR_16B_tag PCR452; /* offset: 0x03C8 size: 16 bit */
			SIUL_PCR_16B_tag PCR453; /* offset: 0x03CA size: 16 bit */
			SIUL_PCR_16B_tag PCR454; /* offset: 0x03CC size: 16 bit */
			SIUL_PCR_16B_tag PCR455; /* offset: 0x03CE size: 16 bit */
			SIUL_PCR_16B_tag PCR456; /* offset: 0x03D0 size: 16 bit */
			SIUL_PCR_16B_tag PCR457; /* offset: 0x03D2 size: 16 bit */
			SIUL_PCR_16B_tag PCR458; /* offset: 0x03D4 size: 16 bit */
			SIUL_PCR_16B_tag PCR459; /* offset: 0x03D6 size: 16 bit */
			SIUL_PCR_16B_tag PCR460; /* offset: 0x03D8 size: 16 bit */
			SIUL_PCR_16B_tag PCR461; /* offset: 0x03DA size: 16 bit */
			SIUL_PCR_16B_tag PCR462; /* offset: 0x03DC size: 16 bit */
			SIUL_PCR_16B_tag PCR463; /* offset: 0x03DE size: 16 bit */
			SIUL_PCR_16B_tag PCR464; /* offset: 0x03E0 size: 16 bit */
			SIUL_PCR_16B_tag PCR465; /* offset: 0x03E2 size: 16 bit */
			SIUL_PCR_16B_tag PCR466; /* offset: 0x03E4 size: 16 bit */
			SIUL_PCR_16B_tag PCR467; /* offset: 0x03E6 size: 16 bit */
			SIUL_PCR_16B_tag PCR468; /* offset: 0x03E8 size: 16 bit */
			SIUL_PCR_16B_tag PCR469; /* offset: 0x03EA size: 16 bit */
			SIUL_PCR_16B_tag PCR470; /* offset: 0x03EC size: 16 bit */
			SIUL_PCR_16B_tag PCR471; /* offset: 0x03EE size: 16 bit */
			SIUL_PCR_16B_tag PCR472; /* offset: 0x03F0 size: 16 bit */
			SIUL_PCR_16B_tag PCR473; /* offset: 0x03F2 size: 16 bit */
			SIUL_PCR_16B_tag PCR474; /* offset: 0x03F4 size: 16 bit */
			SIUL_PCR_16B_tag PCR475; /* offset: 0x03F6 size: 16 bit */
			SIUL_PCR_16B_tag PCR476; /* offset: 0x03F8 size: 16 bit */
			SIUL_PCR_16B_tag PCR477; /* offset: 0x03FA size: 16 bit */
			SIUL_PCR_16B_tag PCR478; /* offset: 0x03FC size: 16 bit */
			SIUL_PCR_16B_tag PCR479; /* offset: 0x03FE size: 16 bit */
			SIUL_PCR_16B_tag PCR480; /* offset: 0x0400 size: 16 bit */
			SIUL_PCR_16B_tag PCR481; /* offset: 0x0402 size: 16 bit */
			SIUL_PCR_16B_tag PCR482; /* offset: 0x0404 size: 16 bit */
			SIUL_PCR_16B_tag PCR483; /* offset: 0x0406 size: 16 bit */
			SIUL_PCR_16B_tag PCR484; /* offset: 0x0408 size: 16 bit */
			SIUL_PCR_16B_tag PCR485; /* offset: 0x040A size: 16 bit */
			SIUL_PCR_16B_tag PCR486; /* offset: 0x040C size: 16 bit */
			SIUL_PCR_16B_tag PCR487; /* offset: 0x040E size: 16 bit */
			SIUL_PCR_16B_tag PCR488; /* offset: 0x0410 size: 16 bit */
			SIUL_PCR_16B_tag PCR489; /* offset: 0x0412 size: 16 bit */
			SIUL_PCR_16B_tag PCR490; /* offset: 0x0414 size: 16 bit */
			SIUL_PCR_16B_tag PCR491; /* offset: 0x0416 size: 16 bit */
			SIUL_PCR_16B_tag PCR492; /* offset: 0x0418 size: 16 bit */
			SIUL_PCR_16B_tag PCR493; /* offset: 0x041A size: 16 bit */
			SIUL_PCR_16B_tag PCR494; /* offset: 0x041C size: 16 bit */
			SIUL_PCR_16B_tag PCR495; /* offset: 0x041E size: 16 bit */
			SIUL_PCR_16B_tag PCR496; /* offset: 0x0420 size: 16 bit */
			SIUL_PCR_16B_tag PCR497; /* offset: 0x0422 size: 16 bit */
			SIUL_PCR_16B_tag PCR498; /* offset: 0x0424 size: 16 bit */
			SIUL_PCR_16B_tag PCR499; /* offset: 0x0426 size: 16 bit */
			SIUL_PCR_16B_tag PCR500; /* offset: 0x0428 size: 16 bit */
			SIUL_PCR_16B_tag PCR501; /* offset: 0x042A size: 16 bit */
			SIUL_PCR_16B_tag PCR502; /* offset: 0x042C size: 16 bit */
			SIUL_PCR_16B_tag PCR503; /* offset: 0x042E size: 16 bit */
			SIUL_PCR_16B_tag PCR504; /* offset: 0x0430 size: 16 bit */
			SIUL_PCR_16B_tag PCR505; /* offset: 0x0432 size: 16 bit */
			SIUL_PCR_16B_tag PCR506; /* offset: 0x0434 size: 16 bit */
			SIUL_PCR_16B_tag PCR507; /* offset: 0x0436 size: 16 bit */
			SIUL_PCR_16B_tag PCR508; /* offset: 0x0438 size: 16 bit */
			SIUL_PCR_16B_tag PCR509; /* offset: 0x043A size: 16 bit */
			SIUL_PCR_16B_tag PCR510; /* offset: 0x043C size: 16 bit */
			SIUL_PCR_16B_tag PCR511; /* offset: 0x043E size: 16 bit */
		};
	};
	int8_t SIUL_reserved_0440_C[192];
	union {
		/* PSMI - Pad Selection for Multiplexed Inputs */
		SIUL_PSMI_32B_tag PSMI_32B[64]; /* offset: 0x0500  (0x0004 x 64) */

		/* PSMI - Pad Selection for Multiplexed Inputs */
		SIUL_PSMI_8B_tag PSMI[256]; /* offset: 0x0500  (0x0001 x 256) */

		struct {
			/* PSMI - Pad Selection for Multiplexed Inputs */
			SIUL_PSMI_32B_tag PSMI0_3;		 /* offset: 0x0500 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI4_7;		 /* offset: 0x0504 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI8_11;		 /* offset: 0x0508 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI12_15;	 /* offset: 0x050C size: 32 bit */
			SIUL_PSMI_32B_tag PSMI16_19;	 /* offset: 0x0510 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI20_23;	 /* offset: 0x0514 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI24_27;	 /* offset: 0x0518 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI28_31;	 /* offset: 0x051C size: 32 bit */
			SIUL_PSMI_32B_tag PSMI32_35;	 /* offset: 0x0520 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI36_39;	 /* offset: 0x0524 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI40_43;	 /* offset: 0x0528 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI44_47;	 /* offset: 0x052C size: 32 bit */
			SIUL_PSMI_32B_tag PSMI48_51;	 /* offset: 0x0530 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI52_55;	 /* offset: 0x0534 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI56_59;	 /* offset: 0x0538 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI60_63;	 /* offset: 0x053C size: 32 bit */
			SIUL_PSMI_32B_tag PSMI64_67;	 /* offset: 0x0540 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI68_71;	 /* offset: 0x0544 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI72_75;	 /* offset: 0x0548 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI76_79;	 /* offset: 0x054C size: 32 bit */
			SIUL_PSMI_32B_tag PSMI80_83;	 /* offset: 0x0550 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI84_87;	 /* offset: 0x0554 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI88_91;	 /* offset: 0x0558 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI92_95;	 /* offset: 0x055C size: 32 bit */
			SIUL_PSMI_32B_tag PSMI96_99;	 /* offset: 0x0560 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI100_103; /* offset: 0x0564 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI104_107; /* offset: 0x0568 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI108_111; /* offset: 0x056C size: 32 bit */
			SIUL_PSMI_32B_tag PSMI112_115; /* offset: 0x0570 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI116_119; /* offset: 0x0574 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI120_123; /* offset: 0x0578 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI124_127; /* offset: 0x057C size: 32 bit */
			SIUL_PSMI_32B_tag PSMI128_131; /* offset: 0x0580 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI132_135; /* offset: 0x0584 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI136_139; /* offset: 0x0588 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI140_143; /* offset: 0x058C size: 32 bit */
			SIUL_PSMI_32B_tag PSMI144_147; /* offset: 0x0590 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI148_151; /* offset: 0x0594 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI152_155; /* offset: 0x0598 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI156_159; /* offset: 0x059C size: 32 bit */
			SIUL_PSMI_32B_tag PSMI160_163; /* offset: 0x05A0 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI164_167; /* offset: 0x05A4 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI168_171; /* offset: 0x05A8 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI172_175; /* offset: 0x05AC size: 32 bit */
			SIUL_PSMI_32B_tag PSMI176_179; /* offset: 0x05B0 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI180_183; /* offset: 0x05B4 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI184_187; /* offset: 0x05B8 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI188_191; /* offset: 0x05BC size: 32 bit */
			SIUL_PSMI_32B_tag PSMI192_195; /* offset: 0x05C0 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI196_199; /* offset: 0x05C4 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI200_203; /* offset: 0x05C8 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI204_207; /* offset: 0x05CC size: 32 bit */
			SIUL_PSMI_32B_tag PSMI208_211; /* offset: 0x05D0 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI212_215; /* offset: 0x05D4 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI216_219; /* offset: 0x05D8 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI220_223; /* offset: 0x05DC size: 32 bit */
			SIUL_PSMI_32B_tag PSMI224_227; /* offset: 0x05E0 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI228_231; /* offset: 0x05E4 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI232_235; /* offset: 0x05E8 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI236_239; /* offset: 0x05EC size: 32 bit */
			SIUL_PSMI_32B_tag PSMI240_243; /* offset: 0x05F0 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI244_247; /* offset: 0x05F4 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI248_251; /* offset: 0x05F8 size: 32 bit */
			SIUL_PSMI_32B_tag PSMI252_255; /* offset: 0x05FC size: 32 bit */
		};

		struct {
			/* PSMI - Pad Selection for Multiplexed Inputs */
			SIUL_PSMI_8B_tag PSMI0;		/* offset: 0x0500 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI1;		/* offset: 0x0501 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI2;		/* offset: 0x0502 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI3;		/* offset: 0x0503 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI4;		/* offset: 0x0504 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI5;		/* offset: 0x0505 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI6;		/* offset: 0x0506 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI7;		/* offset: 0x0507 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI8;		/* offset: 0x0508 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI9;		/* offset: 0x0509 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI10;	/* offset: 0x050A size: 8 bit */
			SIUL_PSMI_8B_tag PSMI11;	/* offset: 0x050B size: 8 bit */
			SIUL_PSMI_8B_tag PSMI12;	/* offset: 0x050C size: 8 bit */
			SIUL_PSMI_8B_tag PSMI13;	/* offset: 0x050D size: 8 bit */
			SIUL_PSMI_8B_tag PSMI14;	/* offset: 0x050E size: 8 bit */
			SIUL_PSMI_8B_tag PSMI15;	/* offset: 0x050F size: 8 bit */
			SIUL_PSMI_8B_tag PSMI16;	/* offset: 0x0510 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI17;	/* offset: 0x0511 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI18;	/* offset: 0x0512 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI19;	/* offset: 0x0513 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI20;	/* offset: 0x0514 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI21;	/* offset: 0x0515 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI22;	/* offset: 0x0516 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI23;	/* offset: 0x0517 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI24;	/* offset: 0x0518 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI25;	/* offset: 0x0519 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI26;	/* offset: 0x051A size: 8 bit */
			SIUL_PSMI_8B_tag PSMI27;	/* offset: 0x051B size: 8 bit */
			SIUL_PSMI_8B_tag PSMI28;	/* offset: 0x051C size: 8 bit */
			SIUL_PSMI_8B_tag PSMI29;	/* offset: 0x051D size: 8 bit */
			SIUL_PSMI_8B_tag PSMI30;	/* offset: 0x051E size: 8 bit */
			SIUL_PSMI_8B_tag PSMI31;	/* offset: 0x051F size: 8 bit */
			SIUL_PSMI_8B_tag PSMI32;	/* offset: 0x0520 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI33;	/* offset: 0x0521 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI34;	/* offset: 0x0522 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI35;	/* offset: 0x0523 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI36;	/* offset: 0x0524 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI37;	/* offset: 0x0525 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI38;	/* offset: 0x0526 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI39;	/* offset: 0x0527 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI40;	/* offset: 0x0528 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI41;	/* offset: 0x0529 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI42;	/* offset: 0x052A size: 8 bit */
			SIUL_PSMI_8B_tag PSMI43;	/* offset: 0x052B size: 8 bit */
			SIUL_PSMI_8B_tag PSMI44;	/* offset: 0x052C size: 8 bit */
			SIUL_PSMI_8B_tag PSMI45;	/* offset: 0x052D size: 8 bit */
			SIUL_PSMI_8B_tag PSMI46;	/* offset: 0x052E size: 8 bit */
			SIUL_PSMI_8B_tag PSMI47;	/* offset: 0x052F size: 8 bit */
			SIUL_PSMI_8B_tag PSMI48;	/* offset: 0x0530 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI49;	/* offset: 0x0531 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI50;	/* offset: 0x0532 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI51;	/* offset: 0x0533 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI52;	/* offset: 0x0534 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI53;	/* offset: 0x0535 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI54;	/* offset: 0x0536 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI55;	/* offset: 0x0537 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI56;	/* offset: 0x0538 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI57;	/* offset: 0x0539 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI58;	/* offset: 0x053A size: 8 bit */
			SIUL_PSMI_8B_tag PSMI59;	/* offset: 0x053B size: 8 bit */
			SIUL_PSMI_8B_tag PSMI60;	/* offset: 0x053C size: 8 bit */
			SIUL_PSMI_8B_tag PSMI61;	/* offset: 0x053D size: 8 bit */
			SIUL_PSMI_8B_tag PSMI62;	/* offset: 0x053E size: 8 bit */
			SIUL_PSMI_8B_tag PSMI63;	/* offset: 0x053F size: 8 bit */
			SIUL_PSMI_8B_tag PSMI64;	/* offset: 0x0540 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI65;	/* offset: 0x0541 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI66;	/* offset: 0x0542 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI67;	/* offset: 0x0543 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI68;	/* offset: 0x0544 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI69;	/* offset: 0x0545 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI70;	/* offset: 0x0546 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI71;	/* offset: 0x0547 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI72;	/* offset: 0x0548 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI73;	/* offset: 0x0549 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI74;	/* offset: 0x054A size: 8 bit */
			SIUL_PSMI_8B_tag PSMI75;	/* offset: 0x054B size: 8 bit */
			SIUL_PSMI_8B_tag PSMI76;	/* offset: 0x054C size: 8 bit */
			SIUL_PSMI_8B_tag PSMI77;	/* offset: 0x054D size: 8 bit */
			SIUL_PSMI_8B_tag PSMI78;	/* offset: 0x054E size: 8 bit */
			SIUL_PSMI_8B_tag PSMI79;	/* offset: 0x054F size: 8 bit */
			SIUL_PSMI_8B_tag PSMI80;	/* offset: 0x0550 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI81;	/* offset: 0x0551 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI82;	/* offset: 0x0552 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI83;	/* offset: 0x0553 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI84;	/* offset: 0x0554 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI85;	/* offset: 0x0555 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI86;	/* offset: 0x0556 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI87;	/* offset: 0x0557 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI88;	/* offset: 0x0558 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI89;	/* offset: 0x0559 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI90;	/* offset: 0x055A size: 8 bit */
			SIUL_PSMI_8B_tag PSMI91;	/* offset: 0x055B size: 8 bit */
			SIUL_PSMI_8B_tag PSMI92;	/* offset: 0x055C size: 8 bit */
			SIUL_PSMI_8B_tag PSMI93;	/* offset: 0x055D size: 8 bit */
			SIUL_PSMI_8B_tag PSMI94;	/* offset: 0x055E size: 8 bit */
			SIUL_PSMI_8B_tag PSMI95;	/* offset: 0x055F size: 8 bit */
			SIUL_PSMI_8B_tag PSMI96;	/* offset: 0x0560 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI97;	/* offset: 0x0561 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI98;	/* offset: 0x0562 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI99;	/* offset: 0x0563 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI100; /* offset: 0x0564 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI101; /* offset: 0x0565 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI102; /* offset: 0x0566 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI103; /* offset: 0x0567 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI104; /* offset: 0x0568 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI105; /* offset: 0x0569 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI106; /* offset: 0x056A size: 8 bit */
			SIUL_PSMI_8B_tag PSMI107; /* offset: 0x056B size: 8 bit */
			SIUL_PSMI_8B_tag PSMI108; /* offset: 0x056C size: 8 bit */
			SIUL_PSMI_8B_tag PSMI109; /* offset: 0x056D size: 8 bit */
			SIUL_PSMI_8B_tag PSMI110; /* offset: 0x056E size: 8 bit */
			SIUL_PSMI_8B_tag PSMI111; /* offset: 0x056F size: 8 bit */
			SIUL_PSMI_8B_tag PSMI112; /* offset: 0x0570 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI113; /* offset: 0x0571 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI114; /* offset: 0x0572 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI115; /* offset: 0x0573 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI116; /* offset: 0x0574 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI117; /* offset: 0x0575 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI118; /* offset: 0x0576 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI119; /* offset: 0x0577 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI120; /* offset: 0x0578 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI121; /* offset: 0x0579 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI122; /* offset: 0x057A size: 8 bit */
			SIUL_PSMI_8B_tag PSMI123; /* offset: 0x057B size: 8 bit */
			SIUL_PSMI_8B_tag PSMI124; /* offset: 0x057C size: 8 bit */
			SIUL_PSMI_8B_tag PSMI125; /* offset: 0x057D size: 8 bit */
			SIUL_PSMI_8B_tag PSMI126; /* offset: 0x057E size: 8 bit */
			SIUL_PSMI_8B_tag PSMI127; /* offset: 0x057F size: 8 bit */
			SIUL_PSMI_8B_tag PSMI128; /* offset: 0x0580 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI129; /* offset: 0x0581 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI130; /* offset: 0x0582 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI131; /* offset: 0x0583 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI132; /* offset: 0x0584 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI133; /* offset: 0x0585 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI134; /* offset: 0x0586 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI135; /* offset: 0x0587 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI136; /* offset: 0x0588 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI137; /* offset: 0x0589 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI138; /* offset: 0x058A size: 8 bit */
			SIUL_PSMI_8B_tag PSMI139; /* offset: 0x058B size: 8 bit */
			SIUL_PSMI_8B_tag PSMI140; /* offset: 0x058C size: 8 bit */
			SIUL_PSMI_8B_tag PSMI141; /* offset: 0x058D size: 8 bit */
			SIUL_PSMI_8B_tag PSMI142; /* offset: 0x058E size: 8 bit */
			SIUL_PSMI_8B_tag PSMI143; /* offset: 0x058F size: 8 bit */
			SIUL_PSMI_8B_tag PSMI144; /* offset: 0x0590 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI145; /* offset: 0x0591 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI146; /* offset: 0x0592 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI147; /* offset: 0x0593 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI148; /* offset: 0x0594 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI149; /* offset: 0x0595 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI150; /* offset: 0x0596 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI151; /* offset: 0x0597 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI152; /* offset: 0x0598 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI153; /* offset: 0x0599 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI154; /* offset: 0x059A size: 8 bit */
			SIUL_PSMI_8B_tag PSMI155; /* offset: 0x059B size: 8 bit */
			SIUL_PSMI_8B_tag PSMI156; /* offset: 0x059C size: 8 bit */
			SIUL_PSMI_8B_tag PSMI157; /* offset: 0x059D size: 8 bit */
			SIUL_PSMI_8B_tag PSMI158; /* offset: 0x059E size: 8 bit */
			SIUL_PSMI_8B_tag PSMI159; /* offset: 0x059F size: 8 bit */
			SIUL_PSMI_8B_tag PSMI160; /* offset: 0x05A0 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI161; /* offset: 0x05A1 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI162; /* offset: 0x05A2 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI163; /* offset: 0x05A3 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI164; /* offset: 0x05A4 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI165; /* offset: 0x05A5 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI166; /* offset: 0x05A6 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI167; /* offset: 0x05A7 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI168; /* offset: 0x05A8 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI169; /* offset: 0x05A9 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI170; /* offset: 0x05AA size: 8 bit */
			SIUL_PSMI_8B_tag PSMI171; /* offset: 0x05AB size: 8 bit */
			SIUL_PSMI_8B_tag PSMI172; /* offset: 0x05AC size: 8 bit */
			SIUL_PSMI_8B_tag PSMI173; /* offset: 0x05AD size: 8 bit */
			SIUL_PSMI_8B_tag PSMI174; /* offset: 0x05AE size: 8 bit */
			SIUL_PSMI_8B_tag PSMI175; /* offset: 0x05AF size: 8 bit */
			SIUL_PSMI_8B_tag PSMI176; /* offset: 0x05B0 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI177; /* offset: 0x05B1 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI178; /* offset: 0x05B2 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI179; /* offset: 0x05B3 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI180; /* offset: 0x05B4 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI181; /* offset: 0x05B5 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI182; /* offset: 0x05B6 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI183; /* offset: 0x05B7 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI184; /* offset: 0x05B8 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI185; /* offset: 0x05B9 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI186; /* offset: 0x05BA size: 8 bit */
			SIUL_PSMI_8B_tag PSMI187; /* offset: 0x05BB size: 8 bit */
			SIUL_PSMI_8B_tag PSMI188; /* offset: 0x05BC size: 8 bit */
			SIUL_PSMI_8B_tag PSMI189; /* offset: 0x05BD size: 8 bit */
			SIUL_PSMI_8B_tag PSMI190; /* offset: 0x05BE size: 8 bit */
			SIUL_PSMI_8B_tag PSMI191; /* offset: 0x05BF size: 8 bit */
			SIUL_PSMI_8B_tag PSMI192; /* offset: 0x05C0 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI193; /* offset: 0x05C1 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI194; /* offset: 0x05C2 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI195; /* offset: 0x05C3 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI196; /* offset: 0x05C4 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI197; /* offset: 0x05C5 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI198; /* offset: 0x05C6 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI199; /* offset: 0x05C7 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI200; /* offset: 0x05C8 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI201; /* offset: 0x05C9 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI202; /* offset: 0x05CA size: 8 bit */
			SIUL_PSMI_8B_tag PSMI203; /* offset: 0x05CB size: 8 bit */
			SIUL_PSMI_8B_tag PSMI204; /* offset: 0x05CC size: 8 bit */
			SIUL_PSMI_8B_tag PSMI205; /* offset: 0x05CD size: 8 bit */
			SIUL_PSMI_8B_tag PSMI206; /* offset: 0x05CE size: 8 bit */
			SIUL_PSMI_8B_tag PSMI207; /* offset: 0x05CF size: 8 bit */
			SIUL_PSMI_8B_tag PSMI208; /* offset: 0x05D0 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI209; /* offset: 0x05D1 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI210; /* offset: 0x05D2 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI211; /* offset: 0x05D3 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI212; /* offset: 0x05D4 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI213; /* offset: 0x05D5 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI214; /* offset: 0x05D6 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI215; /* offset: 0x05D7 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI216; /* offset: 0x05D8 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI217; /* offset: 0x05D9 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI218; /* offset: 0x05DA size: 8 bit */
			SIUL_PSMI_8B_tag PSMI219; /* offset: 0x05DB size: 8 bit */
			SIUL_PSMI_8B_tag PSMI220; /* offset: 0x05DC size: 8 bit */
			SIUL_PSMI_8B_tag PSMI221; /* offset: 0x05DD size: 8 bit */
			SIUL_PSMI_8B_tag PSMI222; /* offset: 0x05DE size: 8 bit */
			SIUL_PSMI_8B_tag PSMI223; /* offset: 0x05DF size: 8 bit */
			SIUL_PSMI_8B_tag PSMI224; /* offset: 0x05E0 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI225; /* offset: 0x05E1 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI226; /* offset: 0x05E2 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI227; /* offset: 0x05E3 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI228; /* offset: 0x05E4 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI229; /* offset: 0x05E5 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI230; /* offset: 0x05E6 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI231; /* offset: 0x05E7 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI232; /* offset: 0x05E8 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI233; /* offset: 0x05E9 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI234; /* offset: 0x05EA size: 8 bit */
			SIUL_PSMI_8B_tag PSMI235; /* offset: 0x05EB size: 8 bit */
			SIUL_PSMI_8B_tag PSMI236; /* offset: 0x05EC size: 8 bit */
			SIUL_PSMI_8B_tag PSMI237; /* offset: 0x05ED size: 8 bit */
			SIUL_PSMI_8B_tag PSMI238; /* offset: 0x05EE size: 8 bit */
			SIUL_PSMI_8B_tag PSMI239; /* offset: 0x05EF size: 8 bit */
			SIUL_PSMI_8B_tag PSMI240; /* offset: 0x05F0 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI241; /* offset: 0x05F1 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI242; /* offset: 0x05F2 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI243; /* offset: 0x05F3 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI244; /* offset: 0x05F4 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI245; /* offset: 0x05F5 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI246; /* offset: 0x05F6 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI247; /* offset: 0x05F7 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI248; /* offset: 0x05F8 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI249; /* offset: 0x05F9 size: 8 bit */
			SIUL_PSMI_8B_tag PSMI250; /* offset: 0x05FA size: 8 bit */
			SIUL_PSMI_8B_tag PSMI251; /* offset: 0x05FB size: 8 bit */
			SIUL_PSMI_8B_tag PSMI252; /* offset: 0x05FC size: 8 bit */
			SIUL_PSMI_8B_tag PSMI253; /* offset: 0x05FD size: 8 bit */
			SIUL_PSMI_8B_tag PSMI254; /* offset: 0x05FE size: 8 bit */
			SIUL_PSMI_8B_tag PSMI255; /* offset: 0x05FF size: 8 bit */
		};
	};
	union {
		/* GPDO - GPIO Pad Data Output Register */
		SIUL_GPDO_32B_tag GPDO_32B[128]; /* offset: 0x0600  (0x0004 x 128) */

		/* GPDO - GPIO Pad Data Output Register */
		SIUL_GPDO_8B_tag GPDO[512]; /* offset: 0x0600  (0x0001 x 512) */

		struct {
			/* GPDO - GPIO Pad Data Output Register */
			SIUL_GPDO_32B_tag GPDO0_3;		 /* offset: 0x0600 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO4_7;		 /* offset: 0x0604 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO8_11;		 /* offset: 0x0608 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO12_15;	 /* offset: 0x060C size: 32 bit */
			SIUL_GPDO_32B_tag GPDO16_19;	 /* offset: 0x0610 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO20_23;	 /* offset: 0x0614 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO24_27;	 /* offset: 0x0618 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO28_31;	 /* offset: 0x061C size: 32 bit */
			SIUL_GPDO_32B_tag GPDO32_35;	 /* offset: 0x0620 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO36_39;	 /* offset: 0x0624 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO40_43;	 /* offset: 0x0628 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO44_47;	 /* offset: 0x062C size: 32 bit */
			SIUL_GPDO_32B_tag GPDO48_51;	 /* offset: 0x0630 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO52_55;	 /* offset: 0x0634 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO56_59;	 /* offset: 0x0638 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO60_63;	 /* offset: 0x063C size: 32 bit */
			SIUL_GPDO_32B_tag GPDO64_67;	 /* offset: 0x0640 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO68_71;	 /* offset: 0x0644 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO72_75;	 /* offset: 0x0648 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO76_79;	 /* offset: 0x064C size: 32 bit */
			SIUL_GPDO_32B_tag GPDO80_83;	 /* offset: 0x0650 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO84_87;	 /* offset: 0x0654 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO88_91;	 /* offset: 0x0658 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO92_95;	 /* offset: 0x065C size: 32 bit */
			SIUL_GPDO_32B_tag GPDO96_99;	 /* offset: 0x0660 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO100_103; /* offset: 0x0664 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO104_107; /* offset: 0x0668 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO108_111; /* offset: 0x066C size: 32 bit */
			SIUL_GPDO_32B_tag GPDO112_115; /* offset: 0x0670 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO116_119; /* offset: 0x0674 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO120_123; /* offset: 0x0678 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO124_127; /* offset: 0x067C size: 32 bit */
			SIUL_GPDO_32B_tag GPDO128_131; /* offset: 0x0680 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO132_135; /* offset: 0x0684 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO136_139; /* offset: 0x0688 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO140_143; /* offset: 0x068C size: 32 bit */
			SIUL_GPDO_32B_tag GPDO144_147; /* offset: 0x0690 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO148_151; /* offset: 0x0694 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO152_155; /* offset: 0x0698 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO156_159; /* offset: 0x069C size: 32 bit */
			SIUL_GPDO_32B_tag GPDO160_163; /* offset: 0x06A0 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO164_167; /* offset: 0x06A4 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO168_171; /* offset: 0x06A8 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO172_175; /* offset: 0x06AC size: 32 bit */
			SIUL_GPDO_32B_tag GPDO176_179; /* offset: 0x06B0 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO180_183; /* offset: 0x06B4 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO184_187; /* offset: 0x06B8 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO188_191; /* offset: 0x06BC size: 32 bit */
			SIUL_GPDO_32B_tag GPDO192_195; /* offset: 0x06C0 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO196_199; /* offset: 0x06C4 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO200_203; /* offset: 0x06C8 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO204_207; /* offset: 0x06CC size: 32 bit */
			SIUL_GPDO_32B_tag GPDO208_211; /* offset: 0x06D0 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO212_215; /* offset: 0x06D4 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO216_219; /* offset: 0x06D8 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO220_223; /* offset: 0x06DC size: 32 bit */
			SIUL_GPDO_32B_tag GPDO224_227; /* offset: 0x06E0 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO228_231; /* offset: 0x06E4 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO232_235; /* offset: 0x06E8 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO236_239; /* offset: 0x06EC size: 32 bit */
			SIUL_GPDO_32B_tag GPDO240_243; /* offset: 0x06F0 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO244_247; /* offset: 0x06F4 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO248_251; /* offset: 0x06F8 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO252_255; /* offset: 0x06FC size: 32 bit */
			SIUL_GPDO_32B_tag GPDO256_259; /* offset: 0x0700 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO260_263; /* offset: 0x0704 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO264_267; /* offset: 0x0708 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO268_271; /* offset: 0x070C size: 32 bit */
			SIUL_GPDO_32B_tag GPDO272_275; /* offset: 0x0710 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO276_279; /* offset: 0x0714 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO280_283; /* offset: 0x0718 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO284_287; /* offset: 0x071C size: 32 bit */
			SIUL_GPDO_32B_tag GPDO288_291; /* offset: 0x0720 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO292_295; /* offset: 0x0724 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO296_299; /* offset: 0x0728 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO300_303; /* offset: 0x072C size: 32 bit */
			SIUL_GPDO_32B_tag GPDO304_307; /* offset: 0x0730 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO308_311; /* offset: 0x0734 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO312_315; /* offset: 0x0738 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO316_319; /* offset: 0x073C size: 32 bit */
			SIUL_GPDO_32B_tag GPDO320_323; /* offset: 0x0740 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO324_327; /* offset: 0x0744 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO328_331; /* offset: 0x0748 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO332_335; /* offset: 0x074C size: 32 bit */
			SIUL_GPDO_32B_tag GPDO336_339; /* offset: 0x0750 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO340_343; /* offset: 0x0754 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO344_347; /* offset: 0x0758 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO348_351; /* offset: 0x075C size: 32 bit */
			SIUL_GPDO_32B_tag GPDO352_355; /* offset: 0x0760 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO356_359; /* offset: 0x0764 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO360_363; /* offset: 0x0768 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO364_367; /* offset: 0x076C size: 32 bit */
			SIUL_GPDO_32B_tag GPDO368_371; /* offset: 0x0770 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO372_375; /* offset: 0x0774 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO376_379; /* offset: 0x0778 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO380_383; /* offset: 0x077C size: 32 bit */
			SIUL_GPDO_32B_tag GPDO384_387; /* offset: 0x0780 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO388_391; /* offset: 0x0784 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO392_395; /* offset: 0x0788 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO396_399; /* offset: 0x078C size: 32 bit */
			SIUL_GPDO_32B_tag GPDO400_403; /* offset: 0x0790 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO404_407; /* offset: 0x0794 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO408_411; /* offset: 0x0798 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO412_415; /* offset: 0x079C size: 32 bit */
			SIUL_GPDO_32B_tag GPDO416_419; /* offset: 0x07A0 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO420_423; /* offset: 0x07A4 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO424_427; /* offset: 0x07A8 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO428_431; /* offset: 0x07AC size: 32 bit */
			SIUL_GPDO_32B_tag GPDO432_435; /* offset: 0x07B0 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO436_439; /* offset: 0x07B4 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO440_443; /* offset: 0x07B8 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO444_447; /* offset: 0x07BC size: 32 bit */
			SIUL_GPDO_32B_tag GPDO448_451; /* offset: 0x07C0 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO452_455; /* offset: 0x07C4 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO456_459; /* offset: 0x07C8 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO460_463; /* offset: 0x07CC size: 32 bit */
			SIUL_GPDO_32B_tag GPDO464_467; /* offset: 0x07D0 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO468_471; /* offset: 0x07D4 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO472_475; /* offset: 0x07D8 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO476_479; /* offset: 0x07DC size: 32 bit */
			SIUL_GPDO_32B_tag GPDO480_483; /* offset: 0x07E0 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO484_487; /* offset: 0x07E4 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO488_491; /* offset: 0x07E8 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO492_495; /* offset: 0x07EC size: 32 bit */
			SIUL_GPDO_32B_tag GPDO496_499; /* offset: 0x07F0 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO500_503; /* offset: 0x07F4 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO504_507; /* offset: 0x07F8 size: 32 bit */
			SIUL_GPDO_32B_tag GPDO508_511; /* offset: 0x07FC size: 32 bit */
		};

		struct {
			/* GPDO - GPIO Pad Data Output Register */
			SIUL_GPDO_8B_tag GPDO0;		/* offset: 0x0600 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO1;		/* offset: 0x0601 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO2;		/* offset: 0x0602 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO3;		/* offset: 0x0603 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO4;		/* offset: 0x0604 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO5;		/* offset: 0x0605 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO6;		/* offset: 0x0606 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO7;		/* offset: 0x0607 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO8;		/* offset: 0x0608 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO9;		/* offset: 0x0609 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO10;	/* offset: 0x060A size: 8 bit */
			SIUL_GPDO_8B_tag GPDO11;	/* offset: 0x060B size: 8 bit */
			SIUL_GPDO_8B_tag GPDO12;	/* offset: 0x060C size: 8 bit */
			SIUL_GPDO_8B_tag GPDO13;	/* offset: 0x060D size: 8 bit */
			SIUL_GPDO_8B_tag GPDO14;	/* offset: 0x060E size: 8 bit */
			SIUL_GPDO_8B_tag GPDO15;	/* offset: 0x060F size: 8 bit */
			SIUL_GPDO_8B_tag GPDO16;	/* offset: 0x0610 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO17;	/* offset: 0x0611 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO18;	/* offset: 0x0612 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO19;	/* offset: 0x0613 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO20;	/* offset: 0x0614 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO21;	/* offset: 0x0615 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO22;	/* offset: 0x0616 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO23;	/* offset: 0x0617 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO24;	/* offset: 0x0618 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO25;	/* offset: 0x0619 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO26;	/* offset: 0x061A size: 8 bit */
			SIUL_GPDO_8B_tag GPDO27;	/* offset: 0x061B size: 8 bit */
			SIUL_GPDO_8B_tag GPDO28;	/* offset: 0x061C size: 8 bit */
			SIUL_GPDO_8B_tag GPDO29;	/* offset: 0x061D size: 8 bit */
			SIUL_GPDO_8B_tag GPDO30;	/* offset: 0x061E size: 8 bit */
			SIUL_GPDO_8B_tag GPDO31;	/* offset: 0x061F size: 8 bit */
			SIUL_GPDO_8B_tag GPDO32;	/* offset: 0x0620 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO33;	/* offset: 0x0621 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO34;	/* offset: 0x0622 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO35;	/* offset: 0x0623 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO36;	/* offset: 0x0624 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO37;	/* offset: 0x0625 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO38;	/* offset: 0x0626 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO39;	/* offset: 0x0627 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO40;	/* offset: 0x0628 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO41;	/* offset: 0x0629 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO42;	/* offset: 0x062A size: 8 bit */
			SIUL_GPDO_8B_tag GPDO43;	/* offset: 0x062B size: 8 bit */
			SIUL_GPDO_8B_tag GPDO44;	/* offset: 0x062C size: 8 bit */
			SIUL_GPDO_8B_tag GPDO45;	/* offset: 0x062D size: 8 bit */
			SIUL_GPDO_8B_tag GPDO46;	/* offset: 0x062E size: 8 bit */
			SIUL_GPDO_8B_tag GPDO47;	/* offset: 0x062F size: 8 bit */
			SIUL_GPDO_8B_tag GPDO48;	/* offset: 0x0630 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO49;	/* offset: 0x0631 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO50;	/* offset: 0x0632 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO51;	/* offset: 0x0633 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO52;	/* offset: 0x0634 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO53;	/* offset: 0x0635 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO54;	/* offset: 0x0636 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO55;	/* offset: 0x0637 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO56;	/* offset: 0x0638 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO57;	/* offset: 0x0639 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO58;	/* offset: 0x063A size: 8 bit */
			SIUL_GPDO_8B_tag GPDO59;	/* offset: 0x063B size: 8 bit */
			SIUL_GPDO_8B_tag GPDO60;	/* offset: 0x063C size: 8 bit */
			SIUL_GPDO_8B_tag GPDO61;	/* offset: 0x063D size: 8 bit */
			SIUL_GPDO_8B_tag GPDO62;	/* offset: 0x063E size: 8 bit */
			SIUL_GPDO_8B_tag GPDO63;	/* offset: 0x063F size: 8 bit */
			SIUL_GPDO_8B_tag GPDO64;	/* offset: 0x0640 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO65;	/* offset: 0x0641 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO66;	/* offset: 0x0642 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO67;	/* offset: 0x0643 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO68;	/* offset: 0x0644 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO69;	/* offset: 0x0645 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO70;	/* offset: 0x0646 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO71;	/* offset: 0x0647 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO72;	/* offset: 0x0648 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO73;	/* offset: 0x0649 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO74;	/* offset: 0x064A size: 8 bit */
			SIUL_GPDO_8B_tag GPDO75;	/* offset: 0x064B size: 8 bit */
			SIUL_GPDO_8B_tag GPDO76;	/* offset: 0x064C size: 8 bit */
			SIUL_GPDO_8B_tag GPDO77;	/* offset: 0x064D size: 8 bit */
			SIUL_GPDO_8B_tag GPDO78;	/* offset: 0x064E size: 8 bit */
			SIUL_GPDO_8B_tag GPDO79;	/* offset: 0x064F size: 8 bit */
			SIUL_GPDO_8B_tag GPDO80;	/* offset: 0x0650 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO81;	/* offset: 0x0651 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO82;	/* offset: 0x0652 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO83;	/* offset: 0x0653 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO84;	/* offset: 0x0654 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO85;	/* offset: 0x0655 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO86;	/* offset: 0x0656 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO87;	/* offset: 0x0657 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO88;	/* offset: 0x0658 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO89;	/* offset: 0x0659 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO90;	/* offset: 0x065A size: 8 bit */
			SIUL_GPDO_8B_tag GPDO91;	/* offset: 0x065B size: 8 bit */
			SIUL_GPDO_8B_tag GPDO92;	/* offset: 0x065C size: 8 bit */
			SIUL_GPDO_8B_tag GPDO93;	/* offset: 0x065D size: 8 bit */
			SIUL_GPDO_8B_tag GPDO94;	/* offset: 0x065E size: 8 bit */
			SIUL_GPDO_8B_tag GPDO95;	/* offset: 0x065F size: 8 bit */
			SIUL_GPDO_8B_tag GPDO96;	/* offset: 0x0660 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO97;	/* offset: 0x0661 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO98;	/* offset: 0x0662 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO99;	/* offset: 0x0663 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO100; /* offset: 0x0664 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO101; /* offset: 0x0665 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO102; /* offset: 0x0666 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO103; /* offset: 0x0667 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO104; /* offset: 0x0668 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO105; /* offset: 0x0669 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO106; /* offset: 0x066A size: 8 bit */
			SIUL_GPDO_8B_tag GPDO107; /* offset: 0x066B size: 8 bit */
			SIUL_GPDO_8B_tag GPDO108; /* offset: 0x066C size: 8 bit */
			SIUL_GPDO_8B_tag GPDO109; /* offset: 0x066D size: 8 bit */
			SIUL_GPDO_8B_tag GPDO110; /* offset: 0x066E size: 8 bit */
			SIUL_GPDO_8B_tag GPDO111; /* offset: 0x066F size: 8 bit */
			SIUL_GPDO_8B_tag GPDO112; /* offset: 0x0670 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO113; /* offset: 0x0671 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO114; /* offset: 0x0672 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO115; /* offset: 0x0673 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO116; /* offset: 0x0674 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO117; /* offset: 0x0675 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO118; /* offset: 0x0676 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO119; /* offset: 0x0677 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO120; /* offset: 0x0678 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO121; /* offset: 0x0679 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO122; /* offset: 0x067A size: 8 bit */
			SIUL_GPDO_8B_tag GPDO123; /* offset: 0x067B size: 8 bit */
			SIUL_GPDO_8B_tag GPDO124; /* offset: 0x067C size: 8 bit */
			SIUL_GPDO_8B_tag GPDO125; /* offset: 0x067D size: 8 bit */
			SIUL_GPDO_8B_tag GPDO126; /* offset: 0x067E size: 8 bit */
			SIUL_GPDO_8B_tag GPDO127; /* offset: 0x067F size: 8 bit */
			SIUL_GPDO_8B_tag GPDO128; /* offset: 0x0680 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO129; /* offset: 0x0681 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO130; /* offset: 0x0682 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO131; /* offset: 0x0683 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO132; /* offset: 0x0684 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO133; /* offset: 0x0685 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO134; /* offset: 0x0686 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO135; /* offset: 0x0687 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO136; /* offset: 0x0688 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO137; /* offset: 0x0689 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO138; /* offset: 0x068A size: 8 bit */
			SIUL_GPDO_8B_tag GPDO139; /* offset: 0x068B size: 8 bit */
			SIUL_GPDO_8B_tag GPDO140; /* offset: 0x068C size: 8 bit */
			SIUL_GPDO_8B_tag GPDO141; /* offset: 0x068D size: 8 bit */
			SIUL_GPDO_8B_tag GPDO142; /* offset: 0x068E size: 8 bit */
			SIUL_GPDO_8B_tag GPDO143; /* offset: 0x068F size: 8 bit */
			SIUL_GPDO_8B_tag GPDO144; /* offset: 0x0690 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO145; /* offset: 0x0691 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO146; /* offset: 0x0692 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO147; /* offset: 0x0693 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO148; /* offset: 0x0694 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO149; /* offset: 0x0695 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO150; /* offset: 0x0696 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO151; /* offset: 0x0697 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO152; /* offset: 0x0698 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO153; /* offset: 0x0699 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO154; /* offset: 0x069A size: 8 bit */
			SIUL_GPDO_8B_tag GPDO155; /* offset: 0x069B size: 8 bit */
			SIUL_GPDO_8B_tag GPDO156; /* offset: 0x069C size: 8 bit */
			SIUL_GPDO_8B_tag GPDO157; /* offset: 0x069D size: 8 bit */
			SIUL_GPDO_8B_tag GPDO158; /* offset: 0x069E size: 8 bit */
			SIUL_GPDO_8B_tag GPDO159; /* offset: 0x069F size: 8 bit */
			SIUL_GPDO_8B_tag GPDO160; /* offset: 0x06A0 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO161; /* offset: 0x06A1 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO162; /* offset: 0x06A2 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO163; /* offset: 0x06A3 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO164; /* offset: 0x06A4 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO165; /* offset: 0x06A5 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO166; /* offset: 0x06A6 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO167; /* offset: 0x06A7 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO168; /* offset: 0x06A8 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO169; /* offset: 0x06A9 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO170; /* offset: 0x06AA size: 8 bit */
			SIUL_GPDO_8B_tag GPDO171; /* offset: 0x06AB size: 8 bit */
			SIUL_GPDO_8B_tag GPDO172; /* offset: 0x06AC size: 8 bit */
			SIUL_GPDO_8B_tag GPDO173; /* offset: 0x06AD size: 8 bit */
			SIUL_GPDO_8B_tag GPDO174; /* offset: 0x06AE size: 8 bit */
			SIUL_GPDO_8B_tag GPDO175; /* offset: 0x06AF size: 8 bit */
			SIUL_GPDO_8B_tag GPDO176; /* offset: 0x06B0 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO177; /* offset: 0x06B1 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO178; /* offset: 0x06B2 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO179; /* offset: 0x06B3 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO180; /* offset: 0x06B4 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO181; /* offset: 0x06B5 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO182; /* offset: 0x06B6 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO183; /* offset: 0x06B7 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO184; /* offset: 0x06B8 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO185; /* offset: 0x06B9 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO186; /* offset: 0x06BA size: 8 bit */
			SIUL_GPDO_8B_tag GPDO187; /* offset: 0x06BB size: 8 bit */
			SIUL_GPDO_8B_tag GPDO188; /* offset: 0x06BC size: 8 bit */
			SIUL_GPDO_8B_tag GPDO189; /* offset: 0x06BD size: 8 bit */
			SIUL_GPDO_8B_tag GPDO190; /* offset: 0x06BE size: 8 bit */
			SIUL_GPDO_8B_tag GPDO191; /* offset: 0x06BF size: 8 bit */
			SIUL_GPDO_8B_tag GPDO192; /* offset: 0x06C0 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO193; /* offset: 0x06C1 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO194; /* offset: 0x06C2 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO195; /* offset: 0x06C3 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO196; /* offset: 0x06C4 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO197; /* offset: 0x06C5 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO198; /* offset: 0x06C6 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO199; /* offset: 0x06C7 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO200; /* offset: 0x06C8 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO201; /* offset: 0x06C9 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO202; /* offset: 0x06CA size: 8 bit */
			SIUL_GPDO_8B_tag GPDO203; /* offset: 0x06CB size: 8 bit */
			SIUL_GPDO_8B_tag GPDO204; /* offset: 0x06CC size: 8 bit */
			SIUL_GPDO_8B_tag GPDO205; /* offset: 0x06CD size: 8 bit */
			SIUL_GPDO_8B_tag GPDO206; /* offset: 0x06CE size: 8 bit */
			SIUL_GPDO_8B_tag GPDO207; /* offset: 0x06CF size: 8 bit */
			SIUL_GPDO_8B_tag GPDO208; /* offset: 0x06D0 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO209; /* offset: 0x06D1 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO210; /* offset: 0x06D2 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO211; /* offset: 0x06D3 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO212; /* offset: 0x06D4 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO213; /* offset: 0x06D5 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO214; /* offset: 0x06D6 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO215; /* offset: 0x06D7 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO216; /* offset: 0x06D8 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO217; /* offset: 0x06D9 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO218; /* offset: 0x06DA size: 8 bit */
			SIUL_GPDO_8B_tag GPDO219; /* offset: 0x06DB size: 8 bit */
			SIUL_GPDO_8B_tag GPDO220; /* offset: 0x06DC size: 8 bit */
			SIUL_GPDO_8B_tag GPDO221; /* offset: 0x06DD size: 8 bit */
			SIUL_GPDO_8B_tag GPDO222; /* offset: 0x06DE size: 8 bit */
			SIUL_GPDO_8B_tag GPDO223; /* offset: 0x06DF size: 8 bit */
			SIUL_GPDO_8B_tag GPDO224; /* offset: 0x06E0 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO225; /* offset: 0x06E1 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO226; /* offset: 0x06E2 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO227; /* offset: 0x06E3 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO228; /* offset: 0x06E4 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO229; /* offset: 0x06E5 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO230; /* offset: 0x06E6 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO231; /* offset: 0x06E7 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO232; /* offset: 0x06E8 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO233; /* offset: 0x06E9 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO234; /* offset: 0x06EA size: 8 bit */
			SIUL_GPDO_8B_tag GPDO235; /* offset: 0x06EB size: 8 bit */
			SIUL_GPDO_8B_tag GPDO236; /* offset: 0x06EC size: 8 bit */
			SIUL_GPDO_8B_tag GPDO237; /* offset: 0x06ED size: 8 bit */
			SIUL_GPDO_8B_tag GPDO238; /* offset: 0x06EE size: 8 bit */
			SIUL_GPDO_8B_tag GPDO239; /* offset: 0x06EF size: 8 bit */
			SIUL_GPDO_8B_tag GPDO240; /* offset: 0x06F0 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO241; /* offset: 0x06F1 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO242; /* offset: 0x06F2 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO243; /* offset: 0x06F3 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO244; /* offset: 0x06F4 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO245; /* offset: 0x06F5 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO246; /* offset: 0x06F6 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO247; /* offset: 0x06F7 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO248; /* offset: 0x06F8 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO249; /* offset: 0x06F9 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO250; /* offset: 0x06FA size: 8 bit */
			SIUL_GPDO_8B_tag GPDO251; /* offset: 0x06FB size: 8 bit */
			SIUL_GPDO_8B_tag GPDO252; /* offset: 0x06FC size: 8 bit */
			SIUL_GPDO_8B_tag GPDO253; /* offset: 0x06FD size: 8 bit */
			SIUL_GPDO_8B_tag GPDO254; /* offset: 0x06FE size: 8 bit */
			SIUL_GPDO_8B_tag GPDO255; /* offset: 0x06FF size: 8 bit */
			SIUL_GPDO_8B_tag GPDO256; /* offset: 0x0700 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO257; /* offset: 0x0701 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO258; /* offset: 0x0702 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO259; /* offset: 0x0703 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO260; /* offset: 0x0704 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO261; /* offset: 0x0705 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO262; /* offset: 0x0706 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO263; /* offset: 0x0707 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO264; /* offset: 0x0708 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO265; /* offset: 0x0709 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO266; /* offset: 0x070A size: 8 bit */
			SIUL_GPDO_8B_tag GPDO267; /* offset: 0x070B size: 8 bit */
			SIUL_GPDO_8B_tag GPDO268; /* offset: 0x070C size: 8 bit */
			SIUL_GPDO_8B_tag GPDO269; /* offset: 0x070D size: 8 bit */
			SIUL_GPDO_8B_tag GPDO270; /* offset: 0x070E size: 8 bit */
			SIUL_GPDO_8B_tag GPDO271; /* offset: 0x070F size: 8 bit */
			SIUL_GPDO_8B_tag GPDO272; /* offset: 0x0710 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO273; /* offset: 0x0711 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO274; /* offset: 0x0712 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO275; /* offset: 0x0713 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO276; /* offset: 0x0714 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO277; /* offset: 0x0715 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO278; /* offset: 0x0716 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO279; /* offset: 0x0717 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO280; /* offset: 0x0718 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO281; /* offset: 0x0719 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO282; /* offset: 0x071A size: 8 bit */
			SIUL_GPDO_8B_tag GPDO283; /* offset: 0x071B size: 8 bit */
			SIUL_GPDO_8B_tag GPDO284; /* offset: 0x071C size: 8 bit */
			SIUL_GPDO_8B_tag GPDO285; /* offset: 0x071D size: 8 bit */
			SIUL_GPDO_8B_tag GPDO286; /* offset: 0x071E size: 8 bit */
			SIUL_GPDO_8B_tag GPDO287; /* offset: 0x071F size: 8 bit */
			SIUL_GPDO_8B_tag GPDO288; /* offset: 0x0720 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO289; /* offset: 0x0721 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO290; /* offset: 0x0722 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO291; /* offset: 0x0723 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO292; /* offset: 0x0724 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO293; /* offset: 0x0725 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO294; /* offset: 0x0726 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO295; /* offset: 0x0727 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO296; /* offset: 0x0728 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO297; /* offset: 0x0729 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO298; /* offset: 0x072A size: 8 bit */
			SIUL_GPDO_8B_tag GPDO299; /* offset: 0x072B size: 8 bit */
			SIUL_GPDO_8B_tag GPDO300; /* offset: 0x072C size: 8 bit */
			SIUL_GPDO_8B_tag GPDO301; /* offset: 0x072D size: 8 bit */
			SIUL_GPDO_8B_tag GPDO302; /* offset: 0x072E size: 8 bit */
			SIUL_GPDO_8B_tag GPDO303; /* offset: 0x072F size: 8 bit */
			SIUL_GPDO_8B_tag GPDO304; /* offset: 0x0730 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO305; /* offset: 0x0731 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO306; /* offset: 0x0732 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO307; /* offset: 0x0733 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO308; /* offset: 0x0734 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO309; /* offset: 0x0735 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO310; /* offset: 0x0736 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO311; /* offset: 0x0737 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO312; /* offset: 0x0738 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO313; /* offset: 0x0739 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO314; /* offset: 0x073A size: 8 bit */
			SIUL_GPDO_8B_tag GPDO315; /* offset: 0x073B size: 8 bit */
			SIUL_GPDO_8B_tag GPDO316; /* offset: 0x073C size: 8 bit */
			SIUL_GPDO_8B_tag GPDO317; /* offset: 0x073D size: 8 bit */
			SIUL_GPDO_8B_tag GPDO318; /* offset: 0x073E size: 8 bit */
			SIUL_GPDO_8B_tag GPDO319; /* offset: 0x073F size: 8 bit */
			SIUL_GPDO_8B_tag GPDO320; /* offset: 0x0740 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO321; /* offset: 0x0741 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO322; /* offset: 0x0742 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO323; /* offset: 0x0743 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO324; /* offset: 0x0744 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO325; /* offset: 0x0745 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO326; /* offset: 0x0746 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO327; /* offset: 0x0747 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO328; /* offset: 0x0748 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO329; /* offset: 0x0749 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO330; /* offset: 0x074A size: 8 bit */
			SIUL_GPDO_8B_tag GPDO331; /* offset: 0x074B size: 8 bit */
			SIUL_GPDO_8B_tag GPDO332; /* offset: 0x074C size: 8 bit */
			SIUL_GPDO_8B_tag GPDO333; /* offset: 0x074D size: 8 bit */
			SIUL_GPDO_8B_tag GPDO334; /* offset: 0x074E size: 8 bit */
			SIUL_GPDO_8B_tag GPDO335; /* offset: 0x074F size: 8 bit */
			SIUL_GPDO_8B_tag GPDO336; /* offset: 0x0750 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO337; /* offset: 0x0751 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO338; /* offset: 0x0752 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO339; /* offset: 0x0753 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO340; /* offset: 0x0754 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO341; /* offset: 0x0755 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO342; /* offset: 0x0756 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO343; /* offset: 0x0757 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO344; /* offset: 0x0758 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO345; /* offset: 0x0759 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO346; /* offset: 0x075A size: 8 bit */
			SIUL_GPDO_8B_tag GPDO347; /* offset: 0x075B size: 8 bit */
			SIUL_GPDO_8B_tag GPDO348; /* offset: 0x075C size: 8 bit */
			SIUL_GPDO_8B_tag GPDO349; /* offset: 0x075D size: 8 bit */
			SIUL_GPDO_8B_tag GPDO350; /* offset: 0x075E size: 8 bit */
			SIUL_GPDO_8B_tag GPDO351; /* offset: 0x075F size: 8 bit */
			SIUL_GPDO_8B_tag GPDO352; /* offset: 0x0760 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO353; /* offset: 0x0761 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO354; /* offset: 0x0762 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO355; /* offset: 0x0763 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO356; /* offset: 0x0764 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO357; /* offset: 0x0765 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO358; /* offset: 0x0766 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO359; /* offset: 0x0767 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO360; /* offset: 0x0768 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO361; /* offset: 0x0769 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO362; /* offset: 0x076A size: 8 bit */
			SIUL_GPDO_8B_tag GPDO363; /* offset: 0x076B size: 8 bit */
			SIUL_GPDO_8B_tag GPDO364; /* offset: 0x076C size: 8 bit */
			SIUL_GPDO_8B_tag GPDO365; /* offset: 0x076D size: 8 bit */
			SIUL_GPDO_8B_tag GPDO366; /* offset: 0x076E size: 8 bit */
			SIUL_GPDO_8B_tag GPDO367; /* offset: 0x076F size: 8 bit */
			SIUL_GPDO_8B_tag GPDO368; /* offset: 0x0770 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO369; /* offset: 0x0771 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO370; /* offset: 0x0772 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO371; /* offset: 0x0773 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO372; /* offset: 0x0774 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO373; /* offset: 0x0775 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO374; /* offset: 0x0776 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO375; /* offset: 0x0777 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO376; /* offset: 0x0778 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO377; /* offset: 0x0779 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO378; /* offset: 0x077A size: 8 bit */
			SIUL_GPDO_8B_tag GPDO379; /* offset: 0x077B size: 8 bit */
			SIUL_GPDO_8B_tag GPDO380; /* offset: 0x077C size: 8 bit */
			SIUL_GPDO_8B_tag GPDO381; /* offset: 0x077D size: 8 bit */
			SIUL_GPDO_8B_tag GPDO382; /* offset: 0x077E size: 8 bit */
			SIUL_GPDO_8B_tag GPDO383; /* offset: 0x077F size: 8 bit */
			SIUL_GPDO_8B_tag GPDO384; /* offset: 0x0780 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO385; /* offset: 0x0781 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO386; /* offset: 0x0782 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO387; /* offset: 0x0783 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO388; /* offset: 0x0784 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO389; /* offset: 0x0785 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO390; /* offset: 0x0786 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO391; /* offset: 0x0787 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO392; /* offset: 0x0788 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO393; /* offset: 0x0789 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO394; /* offset: 0x078A size: 8 bit */
			SIUL_GPDO_8B_tag GPDO395; /* offset: 0x078B size: 8 bit */
			SIUL_GPDO_8B_tag GPDO396; /* offset: 0x078C size: 8 bit */
			SIUL_GPDO_8B_tag GPDO397; /* offset: 0x078D size: 8 bit */
			SIUL_GPDO_8B_tag GPDO398; /* offset: 0x078E size: 8 bit */
			SIUL_GPDO_8B_tag GPDO399; /* offset: 0x078F size: 8 bit */
			SIUL_GPDO_8B_tag GPDO400; /* offset: 0x0790 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO401; /* offset: 0x0791 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO402; /* offset: 0x0792 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO403; /* offset: 0x0793 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO404; /* offset: 0x0794 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO405; /* offset: 0x0795 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO406; /* offset: 0x0796 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO407; /* offset: 0x0797 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO408; /* offset: 0x0798 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO409; /* offset: 0x0799 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO410; /* offset: 0x079A size: 8 bit */
			SIUL_GPDO_8B_tag GPDO411; /* offset: 0x079B size: 8 bit */
			SIUL_GPDO_8B_tag GPDO412; /* offset: 0x079C size: 8 bit */
			SIUL_GPDO_8B_tag GPDO413; /* offset: 0x079D size: 8 bit */
			SIUL_GPDO_8B_tag GPDO414; /* offset: 0x079E size: 8 bit */
			SIUL_GPDO_8B_tag GPDO415; /* offset: 0x079F size: 8 bit */
			SIUL_GPDO_8B_tag GPDO416; /* offset: 0x07A0 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO417; /* offset: 0x07A1 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO418; /* offset: 0x07A2 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO419; /* offset: 0x07A3 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO420; /* offset: 0x07A4 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO421; /* offset: 0x07A5 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO422; /* offset: 0x07A6 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO423; /* offset: 0x07A7 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO424; /* offset: 0x07A8 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO425; /* offset: 0x07A9 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO426; /* offset: 0x07AA size: 8 bit */
			SIUL_GPDO_8B_tag GPDO427; /* offset: 0x07AB size: 8 bit */
			SIUL_GPDO_8B_tag GPDO428; /* offset: 0x07AC size: 8 bit */
			SIUL_GPDO_8B_tag GPDO429; /* offset: 0x07AD size: 8 bit */
			SIUL_GPDO_8B_tag GPDO430; /* offset: 0x07AE size: 8 bit */
			SIUL_GPDO_8B_tag GPDO431; /* offset: 0x07AF size: 8 bit */
			SIUL_GPDO_8B_tag GPDO432; /* offset: 0x07B0 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO433; /* offset: 0x07B1 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO434; /* offset: 0x07B2 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO435; /* offset: 0x07B3 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO436; /* offset: 0x07B4 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO437; /* offset: 0x07B5 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO438; /* offset: 0x07B6 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO439; /* offset: 0x07B7 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO440; /* offset: 0x07B8 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO441; /* offset: 0x07B9 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO442; /* offset: 0x07BA size: 8 bit */
			SIUL_GPDO_8B_tag GPDO443; /* offset: 0x07BB size: 8 bit */
			SIUL_GPDO_8B_tag GPDO444; /* offset: 0x07BC size: 8 bit */
			SIUL_GPDO_8B_tag GPDO445; /* offset: 0x07BD size: 8 bit */
			SIUL_GPDO_8B_tag GPDO446; /* offset: 0x07BE size: 8 bit */
			SIUL_GPDO_8B_tag GPDO447; /* offset: 0x07BF size: 8 bit */
			SIUL_GPDO_8B_tag GPDO448; /* offset: 0x07C0 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO449; /* offset: 0x07C1 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO450; /* offset: 0x07C2 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO451; /* offset: 0x07C3 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO452; /* offset: 0x07C4 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO453; /* offset: 0x07C5 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO454; /* offset: 0x07C6 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO455; /* offset: 0x07C7 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO456; /* offset: 0x07C8 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO457; /* offset: 0x07C9 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO458; /* offset: 0x07CA size: 8 bit */
			SIUL_GPDO_8B_tag GPDO459; /* offset: 0x07CB size: 8 bit */
			SIUL_GPDO_8B_tag GPDO460; /* offset: 0x07CC size: 8 bit */
			SIUL_GPDO_8B_tag GPDO461; /* offset: 0x07CD size: 8 bit */
			SIUL_GPDO_8B_tag GPDO462; /* offset: 0x07CE size: 8 bit */
			SIUL_GPDO_8B_tag GPDO463; /* offset: 0x07CF size: 8 bit */
			SIUL_GPDO_8B_tag GPDO464; /* offset: 0x07D0 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO465; /* offset: 0x07D1 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO466; /* offset: 0x07D2 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO467; /* offset: 0x07D3 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO468; /* offset: 0x07D4 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO469; /* offset: 0x07D5 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO470; /* offset: 0x07D6 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO471; /* offset: 0x07D7 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO472; /* offset: 0x07D8 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO473; /* offset: 0x07D9 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO474; /* offset: 0x07DA size: 8 bit */
			SIUL_GPDO_8B_tag GPDO475; /* offset: 0x07DB size: 8 bit */
			SIUL_GPDO_8B_tag GPDO476; /* offset: 0x07DC size: 8 bit */
			SIUL_GPDO_8B_tag GPDO477; /* offset: 0x07DD size: 8 bit */
			SIUL_GPDO_8B_tag GPDO478; /* offset: 0x07DE size: 8 bit */
			SIUL_GPDO_8B_tag GPDO479; /* offset: 0x07DF size: 8 bit */
			SIUL_GPDO_8B_tag GPDO480; /* offset: 0x07E0 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO481; /* offset: 0x07E1 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO482; /* offset: 0x07E2 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO483; /* offset: 0x07E3 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO484; /* offset: 0x07E4 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO485; /* offset: 0x07E5 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO486; /* offset: 0x07E6 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO487; /* offset: 0x07E7 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO488; /* offset: 0x07E8 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO489; /* offset: 0x07E9 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO490; /* offset: 0x07EA size: 8 bit */
			SIUL_GPDO_8B_tag GPDO491; /* offset: 0x07EB size: 8 bit */
			SIUL_GPDO_8B_tag GPDO492; /* offset: 0x07EC size: 8 bit */
			SIUL_GPDO_8B_tag GPDO493; /* offset: 0x07ED size: 8 bit */
			SIUL_GPDO_8B_tag GPDO494; /* offset: 0x07EE size: 8 bit */
			SIUL_GPDO_8B_tag GPDO495; /* offset: 0x07EF size: 8 bit */
			SIUL_GPDO_8B_tag GPDO496; /* offset: 0x07F0 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO497; /* offset: 0x07F1 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO498; /* offset: 0x07F2 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO499; /* offset: 0x07F3 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO500; /* offset: 0x07F4 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO501; /* offset: 0x07F5 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO502; /* offset: 0x07F6 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO503; /* offset: 0x07F7 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO504; /* offset: 0x07F8 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO505; /* offset: 0x07F9 size: 8 bit */
			SIUL_GPDO_8B_tag GPDO506; /* offset: 0x07FA size: 8 bit */
			SIUL_GPDO_8B_tag GPDO507; /* offset: 0x07FB size: 8 bit */
			SIUL_GPDO_8B_tag GPDO508; /* offset: 0x07FC size: 8 bit */
			SIUL_GPDO_8B_tag GPDO509; /* offset: 0x07FD size: 8 bit */
			SIUL_GPDO_8B_tag GPDO510; /* offset: 0x07FE size: 8 bit */
			SIUL_GPDO_8B_tag GPDO511; /* offset: 0x07FF size: 8 bit */
		};
	};
	union {
		/* GPDI - GPIO Pad Data Input Register */
		SIUL_GPDI_32B_tag GPDI_32B[128]; /* offset: 0x0800  (0x0004 x 128) */

		/* GPDI - GPIO Pad Data Input Register */
		SIUL_GPDI_8B_tag GPDI[512]; /* offset: 0x0800  (0x0001 x 512) */

		struct {
			/* GPDI - GPIO Pad Data Input Register */
			SIUL_GPDI_32B_tag GPDI0_3;		 /* offset: 0x0800 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI4_7;		 /* offset: 0x0804 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI8_11;		 /* offset: 0x0808 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI12_15;	 /* offset: 0x080C size: 32 bit */
			SIUL_GPDI_32B_tag GPDI16_19;	 /* offset: 0x0810 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI20_23;	 /* offset: 0x0814 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI24_27;	 /* offset: 0x0818 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI28_31;	 /* offset: 0x081C size: 32 bit */
			SIUL_GPDI_32B_tag GPDI32_35;	 /* offset: 0x0820 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI36_39;	 /* offset: 0x0824 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI40_43;	 /* offset: 0x0828 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI44_47;	 /* offset: 0x082C size: 32 bit */
			SIUL_GPDI_32B_tag GPDI48_51;	 /* offset: 0x0830 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI52_55;	 /* offset: 0x0834 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI56_59;	 /* offset: 0x0838 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI60_63;	 /* offset: 0x083C size: 32 bit */
			SIUL_GPDI_32B_tag GPDI64_67;	 /* offset: 0x0840 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI68_71;	 /* offset: 0x0844 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI72_75;	 /* offset: 0x0848 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI76_79;	 /* offset: 0x084C size: 32 bit */
			SIUL_GPDI_32B_tag GPDI80_83;	 /* offset: 0x0850 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI84_87;	 /* offset: 0x0854 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI88_91;	 /* offset: 0x0858 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI92_95;	 /* offset: 0x085C size: 32 bit */
			SIUL_GPDI_32B_tag GPDI96_99;	 /* offset: 0x0860 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI100_103; /* offset: 0x0864 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI104_107; /* offset: 0x0868 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI108_111; /* offset: 0x086C size: 32 bit */
			SIUL_GPDI_32B_tag GPDI112_115; /* offset: 0x0870 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI116_119; /* offset: 0x0874 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI120_123; /* offset: 0x0878 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI124_127; /* offset: 0x087C size: 32 bit */
			SIUL_GPDI_32B_tag GPDI128_131; /* offset: 0x0880 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI132_135; /* offset: 0x0884 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI136_139; /* offset: 0x0888 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI140_143; /* offset: 0x088C size: 32 bit */
			SIUL_GPDI_32B_tag GPDI144_147; /* offset: 0x0890 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI148_151; /* offset: 0x0894 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI152_155; /* offset: 0x0898 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI156_159; /* offset: 0x089C size: 32 bit */
			SIUL_GPDI_32B_tag GPDI160_163; /* offset: 0x08A0 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI164_167; /* offset: 0x08A4 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI168_171; /* offset: 0x08A8 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI172_175; /* offset: 0x08AC size: 32 bit */
			SIUL_GPDI_32B_tag GPDI176_179; /* offset: 0x08B0 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI180_183; /* offset: 0x08B4 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI184_187; /* offset: 0x08B8 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI188_191; /* offset: 0x08BC size: 32 bit */
			SIUL_GPDI_32B_tag GPDI192_195; /* offset: 0x08C0 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI196_199; /* offset: 0x08C4 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI200_203; /* offset: 0x08C8 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI204_207; /* offset: 0x08CC size: 32 bit */
			SIUL_GPDI_32B_tag GPDI208_211; /* offset: 0x08D0 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI212_215; /* offset: 0x08D4 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI216_219; /* offset: 0x08D8 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI220_223; /* offset: 0x08DC size: 32 bit */
			SIUL_GPDI_32B_tag GPDI224_227; /* offset: 0x08E0 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI228_231; /* offset: 0x08E4 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI232_235; /* offset: 0x08E8 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI236_239; /* offset: 0x08EC size: 32 bit */
			SIUL_GPDI_32B_tag GPDI240_243; /* offset: 0x08F0 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI244_247; /* offset: 0x08F4 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI248_251; /* offset: 0x08F8 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI252_255; /* offset: 0x08FC size: 32 bit */
			SIUL_GPDI_32B_tag GPDI256_259; /* offset: 0x0900 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI260_263; /* offset: 0x0904 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI264_267; /* offset: 0x0908 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI268_271; /* offset: 0x090C size: 32 bit */
			SIUL_GPDI_32B_tag GPDI272_275; /* offset: 0x0910 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI276_279; /* offset: 0x0914 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI280_283; /* offset: 0x0918 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI284_287; /* offset: 0x091C size: 32 bit */
			SIUL_GPDI_32B_tag GPDI288_291; /* offset: 0x0920 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI292_295; /* offset: 0x0924 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI296_299; /* offset: 0x0928 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI300_303; /* offset: 0x092C size: 32 bit */
			SIUL_GPDI_32B_tag GPDI304_307; /* offset: 0x0930 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI308_311; /* offset: 0x0934 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI312_315; /* offset: 0x0938 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI316_319; /* offset: 0x093C size: 32 bit */
			SIUL_GPDI_32B_tag GPDI320_323; /* offset: 0x0940 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI324_327; /* offset: 0x0944 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI328_331; /* offset: 0x0948 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI332_335; /* offset: 0x094C size: 32 bit */
			SIUL_GPDI_32B_tag GPDI336_339; /* offset: 0x0950 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI340_343; /* offset: 0x0954 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI344_347; /* offset: 0x0958 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI348_351; /* offset: 0x095C size: 32 bit */
			SIUL_GPDI_32B_tag GPDI352_355; /* offset: 0x0960 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI356_359; /* offset: 0x0964 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI360_363; /* offset: 0x0968 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI364_367; /* offset: 0x096C size: 32 bit */
			SIUL_GPDI_32B_tag GPDI368_371; /* offset: 0x0970 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI372_375; /* offset: 0x0974 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI376_379; /* offset: 0x0978 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI380_383; /* offset: 0x097C size: 32 bit */
			SIUL_GPDI_32B_tag GPDI384_387; /* offset: 0x0980 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI388_391; /* offset: 0x0984 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI392_395; /* offset: 0x0988 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI396_399; /* offset: 0x098C size: 32 bit */
			SIUL_GPDI_32B_tag GPDI400_403; /* offset: 0x0990 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI404_407; /* offset: 0x0994 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI408_411; /* offset: 0x0998 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI412_415; /* offset: 0x099C size: 32 bit */
			SIUL_GPDI_32B_tag GPDI416_419; /* offset: 0x09A0 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI420_423; /* offset: 0x09A4 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI424_427; /* offset: 0x09A8 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI428_431; /* offset: 0x09AC size: 32 bit */
			SIUL_GPDI_32B_tag GPDI432_435; /* offset: 0x09B0 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI436_439; /* offset: 0x09B4 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI440_443; /* offset: 0x09B8 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI444_447; /* offset: 0x09BC size: 32 bit */
			SIUL_GPDI_32B_tag GPDI448_451; /* offset: 0x09C0 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI452_455; /* offset: 0x09C4 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI456_459; /* offset: 0x09C8 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI460_463; /* offset: 0x09CC size: 32 bit */
			SIUL_GPDI_32B_tag GPDI464_467; /* offset: 0x09D0 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI468_471; /* offset: 0x09D4 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI472_475; /* offset: 0x09D8 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI476_479; /* offset: 0x09DC size: 32 bit */
			SIUL_GPDI_32B_tag GPDI480_483; /* offset: 0x09E0 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI484_487; /* offset: 0x09E4 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI488_491; /* offset: 0x09E8 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI492_495; /* offset: 0x09EC size: 32 bit */
			SIUL_GPDI_32B_tag GPDI496_499; /* offset: 0x09F0 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI500_503; /* offset: 0x09F4 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI504_507; /* offset: 0x09F8 size: 32 bit */
			SIUL_GPDI_32B_tag GPDI508_511; /* offset: 0x09FC size: 32 bit */
		};

		struct {
			/* GPDI - GPIO Pad Data Input Register */
			SIUL_GPDI_8B_tag GPDI0;		/* offset: 0x0800 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI1;		/* offset: 0x0801 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI2;		/* offset: 0x0802 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI3;		/* offset: 0x0803 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI4;		/* offset: 0x0804 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI5;		/* offset: 0x0805 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI6;		/* offset: 0x0806 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI7;		/* offset: 0x0807 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI8;		/* offset: 0x0808 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI9;		/* offset: 0x0809 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI10;	/* offset: 0x080A size: 8 bit */
			SIUL_GPDI_8B_tag GPDI11;	/* offset: 0x080B size: 8 bit */
			SIUL_GPDI_8B_tag GPDI12;	/* offset: 0x080C size: 8 bit */
			SIUL_GPDI_8B_tag GPDI13;	/* offset: 0x080D size: 8 bit */
			SIUL_GPDI_8B_tag GPDI14;	/* offset: 0x080E size: 8 bit */
			SIUL_GPDI_8B_tag GPDI15;	/* offset: 0x080F size: 8 bit */
			SIUL_GPDI_8B_tag GPDI16;	/* offset: 0x0810 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI17;	/* offset: 0x0811 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI18;	/* offset: 0x0812 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI19;	/* offset: 0x0813 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI20;	/* offset: 0x0814 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI21;	/* offset: 0x0815 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI22;	/* offset: 0x0816 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI23;	/* offset: 0x0817 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI24;	/* offset: 0x0818 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI25;	/* offset: 0x0819 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI26;	/* offset: 0x081A size: 8 bit */
			SIUL_GPDI_8B_tag GPDI27;	/* offset: 0x081B size: 8 bit */
			SIUL_GPDI_8B_tag GPDI28;	/* offset: 0x081C size: 8 bit */
			SIUL_GPDI_8B_tag GPDI29;	/* offset: 0x081D size: 8 bit */
			SIUL_GPDI_8B_tag GPDI30;	/* offset: 0x081E size: 8 bit */
			SIUL_GPDI_8B_tag GPDI31;	/* offset: 0x081F size: 8 bit */
			SIUL_GPDI_8B_tag GPDI32;	/* offset: 0x0820 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI33;	/* offset: 0x0821 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI34;	/* offset: 0x0822 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI35;	/* offset: 0x0823 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI36;	/* offset: 0x0824 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI37;	/* offset: 0x0825 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI38;	/* offset: 0x0826 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI39;	/* offset: 0x0827 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI40;	/* offset: 0x0828 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI41;	/* offset: 0x0829 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI42;	/* offset: 0x082A size: 8 bit */
			SIUL_GPDI_8B_tag GPDI43;	/* offset: 0x082B size: 8 bit */
			SIUL_GPDI_8B_tag GPDI44;	/* offset: 0x082C size: 8 bit */
			SIUL_GPDI_8B_tag GPDI45;	/* offset: 0x082D size: 8 bit */
			SIUL_GPDI_8B_tag GPDI46;	/* offset: 0x082E size: 8 bit */
			SIUL_GPDI_8B_tag GPDI47;	/* offset: 0x082F size: 8 bit */
			SIUL_GPDI_8B_tag GPDI48;	/* offset: 0x0830 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI49;	/* offset: 0x0831 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI50;	/* offset: 0x0832 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI51;	/* offset: 0x0833 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI52;	/* offset: 0x0834 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI53;	/* offset: 0x0835 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI54;	/* offset: 0x0836 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI55;	/* offset: 0x0837 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI56;	/* offset: 0x0838 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI57;	/* offset: 0x0839 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI58;	/* offset: 0x083A size: 8 bit */
			SIUL_GPDI_8B_tag GPDI59;	/* offset: 0x083B size: 8 bit */
			SIUL_GPDI_8B_tag GPDI60;	/* offset: 0x083C size: 8 bit */
			SIUL_GPDI_8B_tag GPDI61;	/* offset: 0x083D size: 8 bit */
			SIUL_GPDI_8B_tag GPDI62;	/* offset: 0x083E size: 8 bit */
			SIUL_GPDI_8B_tag GPDI63;	/* offset: 0x083F size: 8 bit */
			SIUL_GPDI_8B_tag GPDI64;	/* offset: 0x0840 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI65;	/* offset: 0x0841 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI66;	/* offset: 0x0842 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI67;	/* offset: 0x0843 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI68;	/* offset: 0x0844 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI69;	/* offset: 0x0845 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI70;	/* offset: 0x0846 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI71;	/* offset: 0x0847 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI72;	/* offset: 0x0848 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI73;	/* offset: 0x0849 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI74;	/* offset: 0x084A size: 8 bit */
			SIUL_GPDI_8B_tag GPDI75;	/* offset: 0x084B size: 8 bit */
			SIUL_GPDI_8B_tag GPDI76;	/* offset: 0x084C size: 8 bit */
			SIUL_GPDI_8B_tag GPDI77;	/* offset: 0x084D size: 8 bit */
			SIUL_GPDI_8B_tag GPDI78;	/* offset: 0x084E size: 8 bit */
			SIUL_GPDI_8B_tag GPDI79;	/* offset: 0x084F size: 8 bit */
			SIUL_GPDI_8B_tag GPDI80;	/* offset: 0x0850 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI81;	/* offset: 0x0851 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI82;	/* offset: 0x0852 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI83;	/* offset: 0x0853 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI84;	/* offset: 0x0854 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI85;	/* offset: 0x0855 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI86;	/* offset: 0x0856 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI87;	/* offset: 0x0857 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI88;	/* offset: 0x0858 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI89;	/* offset: 0x0859 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI90;	/* offset: 0x085A size: 8 bit */
			SIUL_GPDI_8B_tag GPDI91;	/* offset: 0x085B size: 8 bit */
			SIUL_GPDI_8B_tag GPDI92;	/* offset: 0x085C size: 8 bit */
			SIUL_GPDI_8B_tag GPDI93;	/* offset: 0x085D size: 8 bit */
			SIUL_GPDI_8B_tag GPDI94;	/* offset: 0x085E size: 8 bit */
			SIUL_GPDI_8B_tag GPDI95;	/* offset: 0x085F size: 8 bit */
			SIUL_GPDI_8B_tag GPDI96;	/* offset: 0x0860 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI97;	/* offset: 0x0861 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI98;	/* offset: 0x0862 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI99;	/* offset: 0x0863 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI100; /* offset: 0x0864 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI101; /* offset: 0x0865 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI102; /* offset: 0x0866 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI103; /* offset: 0x0867 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI104; /* offset: 0x0868 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI105; /* offset: 0x0869 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI106; /* offset: 0x086A size: 8 bit */
			SIUL_GPDI_8B_tag GPDI107; /* offset: 0x086B size: 8 bit */
			SIUL_GPDI_8B_tag GPDI108; /* offset: 0x086C size: 8 bit */
			SIUL_GPDI_8B_tag GPDI109; /* offset: 0x086D size: 8 bit */
			SIUL_GPDI_8B_tag GPDI110; /* offset: 0x086E size: 8 bit */
			SIUL_GPDI_8B_tag GPDI111; /* offset: 0x086F size: 8 bit */
			SIUL_GPDI_8B_tag GPDI112; /* offset: 0x0870 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI113; /* offset: 0x0871 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI114; /* offset: 0x0872 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI115; /* offset: 0x0873 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI116; /* offset: 0x0874 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI117; /* offset: 0x0875 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI118; /* offset: 0x0876 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI119; /* offset: 0x0877 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI120; /* offset: 0x0878 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI121; /* offset: 0x0879 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI122; /* offset: 0x087A size: 8 bit */
			SIUL_GPDI_8B_tag GPDI123; /* offset: 0x087B size: 8 bit */
			SIUL_GPDI_8B_tag GPDI124; /* offset: 0x087C size: 8 bit */
			SIUL_GPDI_8B_tag GPDI125; /* offset: 0x087D size: 8 bit */
			SIUL_GPDI_8B_tag GPDI126; /* offset: 0x087E size: 8 bit */
			SIUL_GPDI_8B_tag GPDI127; /* offset: 0x087F size: 8 bit */
			SIUL_GPDI_8B_tag GPDI128; /* offset: 0x0880 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI129; /* offset: 0x0881 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI130; /* offset: 0x0882 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI131; /* offset: 0x0883 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI132; /* offset: 0x0884 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI133; /* offset: 0x0885 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI134; /* offset: 0x0886 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI135; /* offset: 0x0887 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI136; /* offset: 0x0888 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI137; /* offset: 0x0889 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI138; /* offset: 0x088A size: 8 bit */
			SIUL_GPDI_8B_tag GPDI139; /* offset: 0x088B size: 8 bit */
			SIUL_GPDI_8B_tag GPDI140; /* offset: 0x088C size: 8 bit */
			SIUL_GPDI_8B_tag GPDI141; /* offset: 0x088D size: 8 bit */
			SIUL_GPDI_8B_tag GPDI142; /* offset: 0x088E size: 8 bit */
			SIUL_GPDI_8B_tag GPDI143; /* offset: 0x088F size: 8 bit */
			SIUL_GPDI_8B_tag GPDI144; /* offset: 0x0890 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI145; /* offset: 0x0891 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI146; /* offset: 0x0892 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI147; /* offset: 0x0893 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI148; /* offset: 0x0894 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI149; /* offset: 0x0895 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI150; /* offset: 0x0896 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI151; /* offset: 0x0897 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI152; /* offset: 0x0898 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI153; /* offset: 0x0899 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI154; /* offset: 0x089A size: 8 bit */
			SIUL_GPDI_8B_tag GPDI155; /* offset: 0x089B size: 8 bit */
			SIUL_GPDI_8B_tag GPDI156; /* offset: 0x089C size: 8 bit */
			SIUL_GPDI_8B_tag GPDI157; /* offset: 0x089D size: 8 bit */
			SIUL_GPDI_8B_tag GPDI158; /* offset: 0x089E size: 8 bit */
			SIUL_GPDI_8B_tag GPDI159; /* offset: 0x089F size: 8 bit */
			SIUL_GPDI_8B_tag GPDI160; /* offset: 0x08A0 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI161; /* offset: 0x08A1 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI162; /* offset: 0x08A2 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI163; /* offset: 0x08A3 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI164; /* offset: 0x08A4 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI165; /* offset: 0x08A5 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI166; /* offset: 0x08A6 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI167; /* offset: 0x08A7 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI168; /* offset: 0x08A8 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI169; /* offset: 0x08A9 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI170; /* offset: 0x08AA size: 8 bit */
			SIUL_GPDI_8B_tag GPDI171; /* offset: 0x08AB size: 8 bit */
			SIUL_GPDI_8B_tag GPDI172; /* offset: 0x08AC size: 8 bit */
			SIUL_GPDI_8B_tag GPDI173; /* offset: 0x08AD size: 8 bit */
			SIUL_GPDI_8B_tag GPDI174; /* offset: 0x08AE size: 8 bit */
			SIUL_GPDI_8B_tag GPDI175; /* offset: 0x08AF size: 8 bit */
			SIUL_GPDI_8B_tag GPDI176; /* offset: 0x08B0 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI177; /* offset: 0x08B1 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI178; /* offset: 0x08B2 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI179; /* offset: 0x08B3 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI180; /* offset: 0x08B4 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI181; /* offset: 0x08B5 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI182; /* offset: 0x08B6 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI183; /* offset: 0x08B7 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI184; /* offset: 0x08B8 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI185; /* offset: 0x08B9 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI186; /* offset: 0x08BA size: 8 bit */
			SIUL_GPDI_8B_tag GPDI187; /* offset: 0x08BB size: 8 bit */
			SIUL_GPDI_8B_tag GPDI188; /* offset: 0x08BC size: 8 bit */
			SIUL_GPDI_8B_tag GPDI189; /* offset: 0x08BD size: 8 bit */
			SIUL_GPDI_8B_tag GPDI190; /* offset: 0x08BE size: 8 bit */
			SIUL_GPDI_8B_tag GPDI191; /* offset: 0x08BF size: 8 bit */
			SIUL_GPDI_8B_tag GPDI192; /* offset: 0x08C0 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI193; /* offset: 0x08C1 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI194; /* offset: 0x08C2 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI195; /* offset: 0x08C3 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI196; /* offset: 0x08C4 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI197; /* offset: 0x08C5 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI198; /* offset: 0x08C6 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI199; /* offset: 0x08C7 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI200; /* offset: 0x08C8 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI201; /* offset: 0x08C9 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI202; /* offset: 0x08CA size: 8 bit */
			SIUL_GPDI_8B_tag GPDI203; /* offset: 0x08CB size: 8 bit */
			SIUL_GPDI_8B_tag GPDI204; /* offset: 0x08CC size: 8 bit */
			SIUL_GPDI_8B_tag GPDI205; /* offset: 0x08CD size: 8 bit */
			SIUL_GPDI_8B_tag GPDI206; /* offset: 0x08CE size: 8 bit */
			SIUL_GPDI_8B_tag GPDI207; /* offset: 0x08CF size: 8 bit */
			SIUL_GPDI_8B_tag GPDI208; /* offset: 0x08D0 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI209; /* offset: 0x08D1 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI210; /* offset: 0x08D2 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI211; /* offset: 0x08D3 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI212; /* offset: 0x08D4 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI213; /* offset: 0x08D5 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI214; /* offset: 0x08D6 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI215; /* offset: 0x08D7 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI216; /* offset: 0x08D8 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI217; /* offset: 0x08D9 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI218; /* offset: 0x08DA size: 8 bit */
			SIUL_GPDI_8B_tag GPDI219; /* offset: 0x08DB size: 8 bit */
			SIUL_GPDI_8B_tag GPDI220; /* offset: 0x08DC size: 8 bit */
			SIUL_GPDI_8B_tag GPDI221; /* offset: 0x08DD size: 8 bit */
			SIUL_GPDI_8B_tag GPDI222; /* offset: 0x08DE size: 8 bit */
			SIUL_GPDI_8B_tag GPDI223; /* offset: 0x08DF size: 8 bit */
			SIUL_GPDI_8B_tag GPDI224; /* offset: 0x08E0 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI225; /* offset: 0x08E1 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI226; /* offset: 0x08E2 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI227; /* offset: 0x08E3 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI228; /* offset: 0x08E4 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI229; /* offset: 0x08E5 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI230; /* offset: 0x08E6 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI231; /* offset: 0x08E7 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI232; /* offset: 0x08E8 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI233; /* offset: 0x08E9 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI234; /* offset: 0x08EA size: 8 bit */
			SIUL_GPDI_8B_tag GPDI235; /* offset: 0x08EB size: 8 bit */
			SIUL_GPDI_8B_tag GPDI236; /* offset: 0x08EC size: 8 bit */
			SIUL_GPDI_8B_tag GPDI237; /* offset: 0x08ED size: 8 bit */
			SIUL_GPDI_8B_tag GPDI238; /* offset: 0x08EE size: 8 bit */
			SIUL_GPDI_8B_tag GPDI239; /* offset: 0x08EF size: 8 bit */
			SIUL_GPDI_8B_tag GPDI240; /* offset: 0x08F0 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI241; /* offset: 0x08F1 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI242; /* offset: 0x08F2 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI243; /* offset: 0x08F3 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI244; /* offset: 0x08F4 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI245; /* offset: 0x08F5 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI246; /* offset: 0x08F6 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI247; /* offset: 0x08F7 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI248; /* offset: 0x08F8 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI249; /* offset: 0x08F9 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI250; /* offset: 0x08FA size: 8 bit */
			SIUL_GPDI_8B_tag GPDI251; /* offset: 0x08FB size: 8 bit */
			SIUL_GPDI_8B_tag GPDI252; /* offset: 0x08FC size: 8 bit */
			SIUL_GPDI_8B_tag GPDI253; /* offset: 0x08FD size: 8 bit */
			SIUL_GPDI_8B_tag GPDI254; /* offset: 0x08FE size: 8 bit */
			SIUL_GPDI_8B_tag GPDI255; /* offset: 0x08FF size: 8 bit */
			SIUL_GPDI_8B_tag GPDI256; /* offset: 0x0900 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI257; /* offset: 0x0901 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI258; /* offset: 0x0902 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI259; /* offset: 0x0903 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI260; /* offset: 0x0904 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI261; /* offset: 0x0905 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI262; /* offset: 0x0906 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI263; /* offset: 0x0907 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI264; /* offset: 0x0908 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI265; /* offset: 0x0909 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI266; /* offset: 0x090A size: 8 bit */
			SIUL_GPDI_8B_tag GPDI267; /* offset: 0x090B size: 8 bit */
			SIUL_GPDI_8B_tag GPDI268; /* offset: 0x090C size: 8 bit */
			SIUL_GPDI_8B_tag GPDI269; /* offset: 0x090D size: 8 bit */
			SIUL_GPDI_8B_tag GPDI270; /* offset: 0x090E size: 8 bit */
			SIUL_GPDI_8B_tag GPDI271; /* offset: 0x090F size: 8 bit */
			SIUL_GPDI_8B_tag GPDI272; /* offset: 0x0910 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI273; /* offset: 0x0911 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI274; /* offset: 0x0912 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI275; /* offset: 0x0913 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI276; /* offset: 0x0914 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI277; /* offset: 0x0915 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI278; /* offset: 0x0916 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI279; /* offset: 0x0917 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI280; /* offset: 0x0918 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI281; /* offset: 0x0919 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI282; /* offset: 0x091A size: 8 bit */
			SIUL_GPDI_8B_tag GPDI283; /* offset: 0x091B size: 8 bit */
			SIUL_GPDI_8B_tag GPDI284; /* offset: 0x091C size: 8 bit */
			SIUL_GPDI_8B_tag GPDI285; /* offset: 0x091D size: 8 bit */
			SIUL_GPDI_8B_tag GPDI286; /* offset: 0x091E size: 8 bit */
			SIUL_GPDI_8B_tag GPDI287; /* offset: 0x091F size: 8 bit */
			SIUL_GPDI_8B_tag GPDI288; /* offset: 0x0920 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI289; /* offset: 0x0921 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI290; /* offset: 0x0922 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI291; /* offset: 0x0923 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI292; /* offset: 0x0924 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI293; /* offset: 0x0925 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI294; /* offset: 0x0926 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI295; /* offset: 0x0927 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI296; /* offset: 0x0928 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI297; /* offset: 0x0929 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI298; /* offset: 0x092A size: 8 bit */
			SIUL_GPDI_8B_tag GPDI299; /* offset: 0x092B size: 8 bit */
			SIUL_GPDI_8B_tag GPDI300; /* offset: 0x092C size: 8 bit */
			SIUL_GPDI_8B_tag GPDI301; /* offset: 0x092D size: 8 bit */
			SIUL_GPDI_8B_tag GPDI302; /* offset: 0x092E size: 8 bit */
			SIUL_GPDI_8B_tag GPDI303; /* offset: 0x092F size: 8 bit */
			SIUL_GPDI_8B_tag GPDI304; /* offset: 0x0930 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI305; /* offset: 0x0931 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI306; /* offset: 0x0932 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI307; /* offset: 0x0933 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI308; /* offset: 0x0934 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI309; /* offset: 0x0935 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI310; /* offset: 0x0936 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI311; /* offset: 0x0937 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI312; /* offset: 0x0938 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI313; /* offset: 0x0939 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI314; /* offset: 0x093A size: 8 bit */
			SIUL_GPDI_8B_tag GPDI315; /* offset: 0x093B size: 8 bit */
			SIUL_GPDI_8B_tag GPDI316; /* offset: 0x093C size: 8 bit */
			SIUL_GPDI_8B_tag GPDI317; /* offset: 0x093D size: 8 bit */
			SIUL_GPDI_8B_tag GPDI318; /* offset: 0x093E size: 8 bit */
			SIUL_GPDI_8B_tag GPDI319; /* offset: 0x093F size: 8 bit */
			SIUL_GPDI_8B_tag GPDI320; /* offset: 0x0940 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI321; /* offset: 0x0941 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI322; /* offset: 0x0942 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI323; /* offset: 0x0943 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI324; /* offset: 0x0944 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI325; /* offset: 0x0945 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI326; /* offset: 0x0946 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI327; /* offset: 0x0947 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI328; /* offset: 0x0948 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI329; /* offset: 0x0949 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI330; /* offset: 0x094A size: 8 bit */
			SIUL_GPDI_8B_tag GPDI331; /* offset: 0x094B size: 8 bit */
			SIUL_GPDI_8B_tag GPDI332; /* offset: 0x094C size: 8 bit */
			SIUL_GPDI_8B_tag GPDI333; /* offset: 0x094D size: 8 bit */
			SIUL_GPDI_8B_tag GPDI334; /* offset: 0x094E size: 8 bit */
			SIUL_GPDI_8B_tag GPDI335; /* offset: 0x094F size: 8 bit */
			SIUL_GPDI_8B_tag GPDI336; /* offset: 0x0950 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI337; /* offset: 0x0951 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI338; /* offset: 0x0952 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI339; /* offset: 0x0953 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI340; /* offset: 0x0954 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI341; /* offset: 0x0955 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI342; /* offset: 0x0956 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI343; /* offset: 0x0957 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI344; /* offset: 0x0958 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI345; /* offset: 0x0959 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI346; /* offset: 0x095A size: 8 bit */
			SIUL_GPDI_8B_tag GPDI347; /* offset: 0x095B size: 8 bit */
			SIUL_GPDI_8B_tag GPDI348; /* offset: 0x095C size: 8 bit */
			SIUL_GPDI_8B_tag GPDI349; /* offset: 0x095D size: 8 bit */
			SIUL_GPDI_8B_tag GPDI350; /* offset: 0x095E size: 8 bit */
			SIUL_GPDI_8B_tag GPDI351; /* offset: 0x095F size: 8 bit */
			SIUL_GPDI_8B_tag GPDI352; /* offset: 0x0960 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI353; /* offset: 0x0961 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI354; /* offset: 0x0962 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI355; /* offset: 0x0963 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI356; /* offset: 0x0964 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI357; /* offset: 0x0965 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI358; /* offset: 0x0966 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI359; /* offset: 0x0967 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI360; /* offset: 0x0968 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI361; /* offset: 0x0969 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI362; /* offset: 0x096A size: 8 bit */
			SIUL_GPDI_8B_tag GPDI363; /* offset: 0x096B size: 8 bit */
			SIUL_GPDI_8B_tag GPDI364; /* offset: 0x096C size: 8 bit */
			SIUL_GPDI_8B_tag GPDI365; /* offset: 0x096D size: 8 bit */
			SIUL_GPDI_8B_tag GPDI366; /* offset: 0x096E size: 8 bit */
			SIUL_GPDI_8B_tag GPDI367; /* offset: 0x096F size: 8 bit */
			SIUL_GPDI_8B_tag GPDI368; /* offset: 0x0970 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI369; /* offset: 0x0971 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI370; /* offset: 0x0972 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI371; /* offset: 0x0973 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI372; /* offset: 0x0974 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI373; /* offset: 0x0975 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI374; /* offset: 0x0976 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI375; /* offset: 0x0977 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI376; /* offset: 0x0978 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI377; /* offset: 0x0979 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI378; /* offset: 0x097A size: 8 bit */
			SIUL_GPDI_8B_tag GPDI379; /* offset: 0x097B size: 8 bit */
			SIUL_GPDI_8B_tag GPDI380; /* offset: 0x097C size: 8 bit */
			SIUL_GPDI_8B_tag GPDI381; /* offset: 0x097D size: 8 bit */
			SIUL_GPDI_8B_tag GPDI382; /* offset: 0x097E size: 8 bit */
			SIUL_GPDI_8B_tag GPDI383; /* offset: 0x097F size: 8 bit */
			SIUL_GPDI_8B_tag GPDI384; /* offset: 0x0980 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI385; /* offset: 0x0981 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI386; /* offset: 0x0982 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI387; /* offset: 0x0983 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI388; /* offset: 0x0984 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI389; /* offset: 0x0985 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI390; /* offset: 0x0986 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI391; /* offset: 0x0987 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI392; /* offset: 0x0988 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI393; /* offset: 0x0989 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI394; /* offset: 0x098A size: 8 bit */
			SIUL_GPDI_8B_tag GPDI395; /* offset: 0x098B size: 8 bit */
			SIUL_GPDI_8B_tag GPDI396; /* offset: 0x098C size: 8 bit */
			SIUL_GPDI_8B_tag GPDI397; /* offset: 0x098D size: 8 bit */
			SIUL_GPDI_8B_tag GPDI398; /* offset: 0x098E size: 8 bit */
			SIUL_GPDI_8B_tag GPDI399; /* offset: 0x098F size: 8 bit */
			SIUL_GPDI_8B_tag GPDI400; /* offset: 0x0990 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI401; /* offset: 0x0991 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI402; /* offset: 0x0992 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI403; /* offset: 0x0993 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI404; /* offset: 0x0994 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI405; /* offset: 0x0995 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI406; /* offset: 0x0996 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI407; /* offset: 0x0997 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI408; /* offset: 0x0998 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI409; /* offset: 0x0999 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI410; /* offset: 0x099A size: 8 bit */
			SIUL_GPDI_8B_tag GPDI411; /* offset: 0x099B size: 8 bit */
			SIUL_GPDI_8B_tag GPDI412; /* offset: 0x099C size: 8 bit */
			SIUL_GPDI_8B_tag GPDI413; /* offset: 0x099D size: 8 bit */
			SIUL_GPDI_8B_tag GPDI414; /* offset: 0x099E size: 8 bit */
			SIUL_GPDI_8B_tag GPDI415; /* offset: 0x099F size: 8 bit */
			SIUL_GPDI_8B_tag GPDI416; /* offset: 0x09A0 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI417; /* offset: 0x09A1 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI418; /* offset: 0x09A2 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI419; /* offset: 0x09A3 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI420; /* offset: 0x09A4 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI421; /* offset: 0x09A5 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI422; /* offset: 0x09A6 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI423; /* offset: 0x09A7 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI424; /* offset: 0x09A8 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI425; /* offset: 0x09A9 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI426; /* offset: 0x09AA size: 8 bit */
			SIUL_GPDI_8B_tag GPDI427; /* offset: 0x09AB size: 8 bit */
			SIUL_GPDI_8B_tag GPDI428; /* offset: 0x09AC size: 8 bit */
			SIUL_GPDI_8B_tag GPDI429; /* offset: 0x09AD size: 8 bit */
			SIUL_GPDI_8B_tag GPDI430; /* offset: 0x09AE size: 8 bit */
			SIUL_GPDI_8B_tag GPDI431; /* offset: 0x09AF size: 8 bit */
			SIUL_GPDI_8B_tag GPDI432; /* offset: 0x09B0 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI433; /* offset: 0x09B1 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI434; /* offset: 0x09B2 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI435; /* offset: 0x09B3 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI436; /* offset: 0x09B4 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI437; /* offset: 0x09B5 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI438; /* offset: 0x09B6 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI439; /* offset: 0x09B7 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI440; /* offset: 0x09B8 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI441; /* offset: 0x09B9 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI442; /* offset: 0x09BA size: 8 bit */
			SIUL_GPDI_8B_tag GPDI443; /* offset: 0x09BB size: 8 bit */
			SIUL_GPDI_8B_tag GPDI444; /* offset: 0x09BC size: 8 bit */
			SIUL_GPDI_8B_tag GPDI445; /* offset: 0x09BD size: 8 bit */
			SIUL_GPDI_8B_tag GPDI446; /* offset: 0x09BE size: 8 bit */
			SIUL_GPDI_8B_tag GPDI447; /* offset: 0x09BF size: 8 bit */
			SIUL_GPDI_8B_tag GPDI448; /* offset: 0x09C0 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI449; /* offset: 0x09C1 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI450; /* offset: 0x09C2 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI451; /* offset: 0x09C3 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI452; /* offset: 0x09C4 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI453; /* offset: 0x09C5 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI454; /* offset: 0x09C6 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI455; /* offset: 0x09C7 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI456; /* offset: 0x09C8 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI457; /* offset: 0x09C9 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI458; /* offset: 0x09CA size: 8 bit */
			SIUL_GPDI_8B_tag GPDI459; /* offset: 0x09CB size: 8 bit */
			SIUL_GPDI_8B_tag GPDI460; /* offset: 0x09CC size: 8 bit */
			SIUL_GPDI_8B_tag GPDI461; /* offset: 0x09CD size: 8 bit */
			SIUL_GPDI_8B_tag GPDI462; /* offset: 0x09CE size: 8 bit */
			SIUL_GPDI_8B_tag GPDI463; /* offset: 0x09CF size: 8 bit */
			SIUL_GPDI_8B_tag GPDI464; /* offset: 0x09D0 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI465; /* offset: 0x09D1 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI466; /* offset: 0x09D2 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI467; /* offset: 0x09D3 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI468; /* offset: 0x09D4 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI469; /* offset: 0x09D5 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI470; /* offset: 0x09D6 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI471; /* offset: 0x09D7 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI472; /* offset: 0x09D8 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI473; /* offset: 0x09D9 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI474; /* offset: 0x09DA size: 8 bit */
			SIUL_GPDI_8B_tag GPDI475; /* offset: 0x09DB size: 8 bit */
			SIUL_GPDI_8B_tag GPDI476; /* offset: 0x09DC size: 8 bit */
			SIUL_GPDI_8B_tag GPDI477; /* offset: 0x09DD size: 8 bit */
			SIUL_GPDI_8B_tag GPDI478; /* offset: 0x09DE size: 8 bit */
			SIUL_GPDI_8B_tag GPDI479; /* offset: 0x09DF size: 8 bit */
			SIUL_GPDI_8B_tag GPDI480; /* offset: 0x09E0 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI481; /* offset: 0x09E1 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI482; /* offset: 0x09E2 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI483; /* offset: 0x09E3 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI484; /* offset: 0x09E4 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI485; /* offset: 0x09E5 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI486; /* offset: 0x09E6 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI487; /* offset: 0x09E7 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI488; /* offset: 0x09E8 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI489; /* offset: 0x09E9 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI490; /* offset: 0x09EA size: 8 bit */
			SIUL_GPDI_8B_tag GPDI491; /* offset: 0x09EB size: 8 bit */
			SIUL_GPDI_8B_tag GPDI492; /* offset: 0x09EC size: 8 bit */
			SIUL_GPDI_8B_tag GPDI493; /* offset: 0x09ED size: 8 bit */
			SIUL_GPDI_8B_tag GPDI494; /* offset: 0x09EE size: 8 bit */
			SIUL_GPDI_8B_tag GPDI495; /* offset: 0x09EF size: 8 bit */
			SIUL_GPDI_8B_tag GPDI496; /* offset: 0x09F0 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI497; /* offset: 0x09F1 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI498; /* offset: 0x09F2 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI499; /* offset: 0x09F3 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI500; /* offset: 0x09F4 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI501; /* offset: 0x09F5 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI502; /* offset: 0x09F6 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI503; /* offset: 0x09F7 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI504; /* offset: 0x09F8 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI505; /* offset: 0x09F9 size: 8 bit */
			SIUL_GPDI_8B_tag GPDI506; /* offset: 0x09FA size: 8 bit */
			SIUL_GPDI_8B_tag GPDI507; /* offset: 0x09FB size: 8 bit */
			SIUL_GPDI_8B_tag GPDI508; /* offset: 0x09FC size: 8 bit */
			SIUL_GPDI_8B_tag GPDI509; /* offset: 0x09FD size: 8 bit */
			SIUL_GPDI_8B_tag GPDI510; /* offset: 0x09FE size: 8 bit */
			SIUL_GPDI_8B_tag GPDI511; /* offset: 0x09FF size: 8 bit */
		};
	};
	int8_t SIUL_reserved_0A00_C[512];
	union {
		/* PGPDO - Parallel GPIO Pad Data Out Register */
		SIUL_PGPDO_16B_tag PGPDO[32]; /* offset: 0x0C00  (0x0002 x 32) */

		struct {
			/* PGPDO - Parallel GPIO Pad Data Out Register */
			SIUL_PGPDO_16B_tag PGPDO0;	/* offset: 0x0C00 size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO1;	/* offset: 0x0C02 size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO2;	/* offset: 0x0C04 size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO3;	/* offset: 0x0C06 size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO4;	/* offset: 0x0C08 size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO5;	/* offset: 0x0C0A size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO6;	/* offset: 0x0C0C size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO7;	/* offset: 0x0C0E size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO8;	/* offset: 0x0C10 size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO9;	/* offset: 0x0C12 size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO10; /* offset: 0x0C14 size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO11; /* offset: 0x0C16 size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO12; /* offset: 0x0C18 size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO13; /* offset: 0x0C1A size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO14; /* offset: 0x0C1C size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO15; /* offset: 0x0C1E size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO16; /* offset: 0x0C20 size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO17; /* offset: 0x0C22 size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO18; /* offset: 0x0C24 size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO19; /* offset: 0x0C26 size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO20; /* offset: 0x0C28 size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO21; /* offset: 0x0C2A size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO22; /* offset: 0x0C2C size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO23; /* offset: 0x0C2E size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO24; /* offset: 0x0C30 size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO25; /* offset: 0x0C32 size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO26; /* offset: 0x0C34 size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO27; /* offset: 0x0C36 size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO28; /* offset: 0x0C38 size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO29; /* offset: 0x0C3A size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO30; /* offset: 0x0C3C size: 16 bit */
			SIUL_PGPDO_16B_tag PGPDO31; /* offset: 0x0C3E size: 16 bit */
		};
	};
	union {
		/* PGPDI - Parallel GPIO Pad Data In Register */
		SIUL_PGPDI_16B_tag PGPDI[32]; /* offset: 0x0C40  (0x0002 x 32) */

		struct {
			/* PGPDI - Parallel GPIO Pad Data In Register */
			SIUL_PGPDI_16B_tag PGPDI0;	/* offset: 0x0C40 size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI1;	/* offset: 0x0C42 size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI2;	/* offset: 0x0C44 size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI3;	/* offset: 0x0C46 size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI4;	/* offset: 0x0C48 size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI5;	/* offset: 0x0C4A size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI6;	/* offset: 0x0C4C size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI7;	/* offset: 0x0C4E size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI8;	/* offset: 0x0C50 size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI9;	/* offset: 0x0C52 size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI10; /* offset: 0x0C54 size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI11; /* offset: 0x0C56 size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI12; /* offset: 0x0C58 size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI13; /* offset: 0x0C5A size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI14; /* offset: 0x0C5C size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI15; /* offset: 0x0C5E size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI16; /* offset: 0x0C60 size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI17; /* offset: 0x0C62 size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI18; /* offset: 0x0C64 size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI19; /* offset: 0x0C66 size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI20; /* offset: 0x0C68 size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI21; /* offset: 0x0C6A size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI22; /* offset: 0x0C6C size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI23; /* offset: 0x0C6E size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI24; /* offset: 0x0C70 size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI25; /* offset: 0x0C72 size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI26; /* offset: 0x0C74 size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI27; /* offset: 0x0C76 size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI28; /* offset: 0x0C78 size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI29; /* offset: 0x0C7A size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI30; /* offset: 0x0C7C size: 16 bit */
			SIUL_PGPDI_16B_tag PGPDI31; /* offset: 0x0C7E size: 16 bit */
		};
	};
	union {
		/* MPGPDO - Masked Parallel GPIO Pad Data Out Register */
		SIUL_MPGPDO_32B_tag MPGPDO[32]; /* offset: 0x0C80  (0x0004 x 32) */

		struct {
			/* MPGPDO - Masked Parallel GPIO Pad Data Out Register */
			SIUL_MPGPDO_32B_tag MPGPDO0;	/* offset: 0x0C80 size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO1;	/* offset: 0x0C84 size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO2;	/* offset: 0x0C88 size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO3;	/* offset: 0x0C8C size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO4;	/* offset: 0x0C90 size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO5;	/* offset: 0x0C94 size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO6;	/* offset: 0x0C98 size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO7;	/* offset: 0x0C9C size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO8;	/* offset: 0x0CA0 size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO9;	/* offset: 0x0CA4 size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO10; /* offset: 0x0CA8 size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO11; /* offset: 0x0CAC size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO12; /* offset: 0x0CB0 size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO13; /* offset: 0x0CB4 size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO14; /* offset: 0x0CB8 size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO15; /* offset: 0x0CBC size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO16; /* offset: 0x0CC0 size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO17; /* offset: 0x0CC4 size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO18; /* offset: 0x0CC8 size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO19; /* offset: 0x0CCC size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO20; /* offset: 0x0CD0 size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO21; /* offset: 0x0CD4 size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO22; /* offset: 0x0CD8 size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO23; /* offset: 0x0CDC size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO24; /* offset: 0x0CE0 size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO25; /* offset: 0x0CE4 size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO26; /* offset: 0x0CE8 size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO27; /* offset: 0x0CEC size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO28; /* offset: 0x0CF0 size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO29; /* offset: 0x0CF4 size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO30; /* offset: 0x0CF8 size: 32 bit */
			SIUL_MPGPDO_32B_tag MPGPDO31; /* offset: 0x0CFC size: 32 bit */
		};
	};
	int8_t SIUL_reserved_0D00_C[768];
	union {
		/* IFMC - Interrupt Filter Maximum Counter Register */
		SIUL_IFMC_32B_tag IFMC[32]; /* offset: 0x1000  (0x0004 x 32) */

		struct {
			/* IFMC - Interrupt Filter Maximum Counter Register */
			SIUL_IFMC_32B_tag IFMC0;	/* offset: 0x1000 size: 32 bit */
			SIUL_IFMC_32B_tag IFMC1;	/* offset: 0x1004 size: 32 bit */
			SIUL_IFMC_32B_tag IFMC2;	/* offset: 0x1008 size: 32 bit */
			SIUL_IFMC_32B_tag IFMC3;	/* offset: 0x100C size: 32 bit */
			SIUL_IFMC_32B_tag IFMC4;	/* offset: 0x1010 size: 32 bit */
			SIUL_IFMC_32B_tag IFMC5;	/* offset: 0x1014 size: 32 bit */
			SIUL_IFMC_32B_tag IFMC6;	/* offset: 0x1018 size: 32 bit */
			SIUL_IFMC_32B_tag IFMC7;	/* offset: 0x101C size: 32 bit */
			SIUL_IFMC_32B_tag IFMC8;	/* offset: 0x1020 size: 32 bit */
			SIUL_IFMC_32B_tag IFMC9;	/* offset: 0x1024 size: 32 bit */
			SIUL_IFMC_32B_tag IFMC10; /* offset: 0x1028 size: 32 bit */
			SIUL_IFMC_32B_tag IFMC11; /* offset: 0x102C size: 32 bit */
			SIUL_IFMC_32B_tag IFMC12; /* offset: 0x1030 size: 32 bit */
			SIUL_IFMC_32B_tag IFMC13; /* offset: 0x1034 size: 32 bit */
			SIUL_IFMC_32B_tag IFMC14; /* offset: 0x1038 size: 32 bit */
			SIUL_IFMC_32B_tag IFMC15; /* offset: 0x103C size: 32 bit */
			SIUL_IFMC_32B_tag IFMC16; /* offset: 0x1040 size: 32 bit */
			SIUL_IFMC_32B_tag IFMC17; /* offset: 0x1044 size: 32 bit */
			SIUL_IFMC_32B_tag IFMC18; /* offset: 0x1048 size: 32 bit */
			SIUL_IFMC_32B_tag IFMC19; /* offset: 0x104C size: 32 bit */
			SIUL_IFMC_32B_tag IFMC20; /* offset: 0x1050 size: 32 bit */
			SIUL_IFMC_32B_tag IFMC21; /* offset: 0x1054 size: 32 bit */
			SIUL_IFMC_32B_tag IFMC22; /* offset: 0x1058 size: 32 bit */
			SIUL_IFMC_32B_tag IFMC23; /* offset: 0x105C size: 32 bit */
			SIUL_IFMC_32B_tag IFMC24; /* offset: 0x1060 size: 32 bit */
			SIUL_IFMC_32B_tag IFMC25; /* offset: 0x1064 size: 32 bit */
			SIUL_IFMC_32B_tag IFMC26; /* offset: 0x1068 size: 32 bit */
			SIUL_IFMC_32B_tag IFMC27; /* offset: 0x106C size: 32 bit */
			SIUL_IFMC_32B_tag IFMC28; /* offset: 0x1070 size: 32 bit */
			SIUL_IFMC_32B_tag IFMC29; /* offset: 0x1074 size: 32 bit */
			SIUL_IFMC_32B_tag IFMC30; /* offset: 0x1078 size: 32 bit */
			SIUL_IFMC_32B_tag IFMC31; /* offset: 0x107C size: 32 bit */
		};
	};
	/* IFCPR - Inerrupt Filter Clock Prescaler Register */
	SIUL_IFCPR_32B_tag IFCPR; /* offset: 0x1080 size: 32 bit */
} SIUL_tag;

#define SIUL (*(volatile SIUL_tag*)0xC3F90000UL)

/****************************************************************/
/*                                                              */
/* Module: WKPU  */
/*                                                              */
/****************************************************************/

typedef union { /* WKPU_NSR - NMI Status Flag Register */
	vuint32_t R;
	struct {
		vuint32_t NIF0 : 1;	 /* NMI Status Flag 0 */
		vuint32_t NOVF0 : 1; /* NMI Overrun Status Flag 0 */
		vuint32_t : 6;
		vuint32_t NIF1 : 1;	 /* NMI Status Flag 1 */
		vuint32_t NOVF1 : 1; /* NMI Overrun Status Flag 1 */
		vuint32_t : 6;
		vuint32_t NIF2 : 1;	 /* NMI Status Flag 2 */
		vuint32_t NOVF2 : 1; /* NMI Overrun Status Flag 2 */
		vuint32_t : 6;
		vuint32_t NIF3 : 1;	 /* NMI Status Flag 3 */
		vuint32_t NOVF3 : 1; /* NMI Overrun Status Flag 3 */
		vuint32_t : 6;
	} B;
} WKPU_NSR_32B_tag;

typedef union { /* WKPU_NCR - NMI Configuration Register */
	vuint32_t R;
	struct {
		vuint32_t NLOCK0 : 1; /* NMI Configuration Lock Register 0 */
		vuint32_t NDSS0 : 2;	/* NMI Desination Source Select 0 */
		vuint32_t NWRE0 : 1;	/* NMI Wakeup Request Enable 0 */
		vuint32_t : 1;
		vuint32_t NREE0 : 1;	/* NMI Rising Edge Events Enable 0 */
		vuint32_t NFEE0 : 1;	/* NMI Falling Edge Events Enable 0 */
		vuint32_t NFE0 : 1;		/* NMI Filter Enable 0 */
		vuint32_t NLOCK1 : 1; /* NMI Configuration Lock Register 1 */
		vuint32_t NDSS1 : 2;	/* NMI Desination Source Select 1 */
		vuint32_t NWRE1 : 1;	/* NMI Wakeup Request Enable 1 */
		vuint32_t : 1;
		vuint32_t NREE1 : 1;	/* NMI Rising Edge Events Enable 1 */
		vuint32_t NFEE1 : 1;	/* NMI Falling Edge Events Enable 1 */
		vuint32_t NFE1 : 1;		/* NMI Filter Enable 1 */
		vuint32_t NLOCK2 : 1; /* NMI Configuration Lock Register 2 */
		vuint32_t NDSS2 : 2;	/* NMI Desination Source Select 2 */
		vuint32_t NWRE2 : 1;	/* NMI Wakeup Request Enable 2 */
		vuint32_t : 1;
		vuint32_t NREE2 : 1;	/* NMI Rising Edge Events Enable 2 */
		vuint32_t NFEE2 : 1;	/* NMI Falling Edge Events Enable 2 */
		vuint32_t NFE2 : 1;		/* NMI Filter Enable 2 */
		vuint32_t NLOCK3 : 1; /* NMI Configuration Lock Register 3 */
		vuint32_t NDSS3 : 2;	/* NMI Desination Source Select 3 */
		vuint32_t NWRE3 : 1;	/* NMI Wakeup Request Enable 3 */
		vuint32_t : 1;
		vuint32_t NREE3 : 1; /* NMI Rising Edge Events Enable 3 */
		vuint32_t NFEE3 : 1; /* NMI Falling Edge Events Enable 3 */
		vuint32_t NFE3 : 1;	 /* NMI Filter Enable 3 */
	} B;
} WKPU_NCR_32B_tag;

typedef union { /* WKPU_WISR - Wakeup/Interrupt Status Flag Register */
	vuint32_t R;
	struct {
		vuint32_t EIF : 32; /* External Wakeup/Interrupt Status Flag */
	} B;
} WKPU_WISR_32B_tag;

typedef union { /* WKPU_IRER - Interrupt Request Enable Register */
	vuint32_t R;
	struct {
		vuint32_t EIRE : 32; /* Enable External Interrupt Requests */
	} B;
} WKPU_IRER_32B_tag;

typedef union { /* WKPU_WRER - Wakeup Request Enable Register */
	vuint32_t R;
	struct {
		vuint32_t WRE : 32; /* Enable Wakeup requests to the mode entry module */
	} B;
} WKPU_WRER_32B_tag;

typedef union { /* WKPU_WIREER - Wakeup/Interrupt Rising-Edge Event Enable
									 Register */
	vuint32_t R;
	struct {
		vuint32_t
				IREE : 32; /* Enable rising-edge events to cause EIF[x] to be set */
	} B;
} WKPU_WIREER_32B_tag;

typedef union { /* WKPU_WIFEER - Wakeup/Interrupt Falling-Edge Event Enable
									 Register */
	vuint32_t R;
	struct {
		vuint32_t
				IFEE : 32; /* Enable Falling-edge events to cause EIF[x] to be set */
	} B;
} WKPU_WIFEER_32B_tag;

typedef union { /* WKPU_WIFER - Wakeup/Interrupt Filter Enable Register */
	vuint32_t R;
	struct {
		vuint32_t
				IFE : 32; /* Enable Digital glitch filter on the interrupt pad input */
	} B;
} WKPU_WIFER_32B_tag;

typedef union { /* WKPU_WIPUER - Wakeup/Interrupt Pullup Enable Register */
	vuint32_t R;
	struct {
		vuint32_t IPUE : 32; /* Enable a pullup on the interrupt pad input */
	} B;
} WKPU_WIPUER_32B_tag;

typedef struct WKPU_struct_tag { /* start of WKPU_tag */
																 /* WKPU_NSR - NMI Status Flag Register */
	WKPU_NSR_32B_tag NSR;					 /* offset: 0x0000 size: 32 bit */
	int8_t WKPU_reserved_0004[4];
	/* WKPU_NCR - NMI Configuration Register */
	WKPU_NCR_32B_tag NCR; /* offset: 0x0008 size: 32 bit */
	int8_t WKPU_reserved_000C[8];
	/* WKPU_WISR - Wakeup/Interrupt Status Flag Register */
	WKPU_WISR_32B_tag WISR; /* offset: 0x0014 size: 32 bit */
													/* WKPU_IRER - Interrupt Request Enable Register */
	WKPU_IRER_32B_tag IRER; /* offset: 0x0018 size: 32 bit */
													/* WKPU_WRER - Wakeup Request Enable Register */
	WKPU_WRER_32B_tag WRER; /* offset: 0x001C size: 32 bit */
	int8_t WKPU_reserved_0020[8];
	/* WKPU_WIREER - Wakeup/Interrupt Rising-Edge Event Enable Register */
	WKPU_WIREER_32B_tag WIREER; /* offset: 0x0028 size: 32 bit */
	/* WKPU_WIFEER - Wakeup/Interrupt Falling-Edge Event Enable Register */
	WKPU_WIFEER_32B_tag WIFEER; /* offset: 0x002C size: 32 bit */
	/* WKPU_WIFER - Wakeup/Interrupt Filter Enable Register */
	WKPU_WIFER_32B_tag WIFER; /* offset: 0x0030 size: 32 bit */
	/* WKPU_WIPUER - Wakeup/Interrupt Pullup Enable Register */
	WKPU_WIPUER_32B_tag WIPUER; /* offset: 0x0034 size: 32 bit */
} WKPU_tag;

#define WKPU (*(volatile WKPU_tag*)0xC3F94000UL)

/****************************************************************/
/*                                                              */
/* Module: SSCM  */
/*                                                              */
/****************************************************************/

typedef union { /* SSCM_STATUS - System Status Register */
	vuint16_t R;
	struct {
		vuint16_t LSM : 1; /* Lock Step Mode */
		vuint16_t : 2;
		vuint16_t NXEN1 : 1; /* Processor 1 Nexus enabled */
		vuint16_t NXEN : 1;	 /* Processor 0 Nexus enabled */
		vuint16_t PUB : 1;	 /* Public Serial Access Status */
		vuint16_t SEC : 1;	 /* Security Status */
		vuint16_t : 1;
		vuint16_t BMODE : 3; /* Device Boot Mode */
#ifndef USE_FIELD_ALIASES_SSCM
		vuint16_t VLE : 1; /* Variable Length Instruction Mode */
#else
		vuint16_t DMID : 1;					 /* deprecated name - please avoid */
#endif
		vuint16_t ABD : 1; /* Autobaud detection */
		vuint16_t : 3;
	} B;
} SSCM_STATUS_16B_tag;

typedef union { /* SSCM_MEMCONFIG - System Memory Configuration Register */
	vuint16_t R;
	struct {
		vuint16_t JPIN : 10; /* JTAG Part ID Number */
		vuint16_t IVLD : 1;	 /* Instruction Flash Valid */
		vuint16_t MREV : 4;	 /* Minor Mask Revision */
		vuint16_t DVLD : 1;	 /* Data Flash Valid */
	} B;
} SSCM_MEMCONFIG_16B_tag;

typedef union { /* SSCM_ERROR - Error Configuration */
	vuint16_t R;
	struct {
		vuint16_t : 14;
		vuint16_t PAE : 1; /* Peripheral Bus Abort Enable */
		vuint16_t RAE : 1; /* Register Bus Abort Enable */
	} B;
} SSCM_ERROR_16B_tag;

typedef union { /* SSCM_DEBUGPORT - Debug Status Port Register */
	vuint16_t R;
	struct {
		vuint16_t : 13;
		vuint16_t DEBUG_MODE : 3; /* Debug Status Port Mode */
	} B;
} SSCM_DEBUGPORT_16B_tag;

typedef union { /* SSCM_PWCMPH - Password Comparison Register High */
	vuint32_t R;
	struct {
		vuint32_t PWD_HI : 32; /* Password High */
	} B;
} SSCM_PWCMPH_32B_tag;

typedef union { /* SSCM_PWCMPL - Password Comparison Register Low */
	vuint32_t R;
	struct {
		vuint32_t PWD_LO : 32; /* Password Low */
	} B;
} SSCM_PWCMPL_32B_tag;

typedef union { /* SSCM_DPMBOOT - Decoupled Parallel Boot Register */
	vuint32_t R;
	struct {
		vuint32_t P2BOOT : 30; /* boot location 2nd processor */
		vuint32_t DVLE : 1;		 /* VLE mode for 2nd processor */
		vuint32_t : 1;
	} B;
} SSCM_DPMBOOT_32B_tag;

typedef union { /* SSCM_DPMKEY - Boot Key Register */
	vuint32_t R;
	struct {
		vuint32_t KEY : 32; /* Boot Control Key */
	} B;
} SSCM_DPMKEY_32B_tag;

typedef union { /* SSCM_UOPS - User Option Status Register */
	vuint32_t R;
	struct {
		vuint32_t UOPT : 32; /* User Option Bits */
	} B;
} SSCM_UOPS_32B_tag;

typedef union { /* SSCM_SCTR - SSCM Control Register */
	vuint32_t R;
	struct {
		vuint32_t : 29;
		vuint32_t TFE : 1; /* Test Flash Enable */
		vuint32_t DSL : 1; /* Disable Software-Controlled MBIST */
		vuint32_t DSM : 1; /* Disable Software-Controlled LBIST */
	} B;
} SSCM_SCTR_32B_tag;

typedef union { /* SSCM_TF_INFO0 - TestFlash Information Register 0 */
	vuint32_t R;
	struct {
		vuint32_t TINFO0 : 32; /* General purpose TestFlash word 0 */
	} B;
} SSCM_TF_INFO0_32B_tag;

typedef union { /* SSCM_TF_INFO1 - TestFlash Information Register 1 */
	vuint32_t R;
	struct {
		vuint32_t TINFO1 : 32; /* General purpose TestFlash word 1 */
	} B;
} SSCM_TF_INFO1_32B_tag;

typedef union { /* SSCM_TF_INFO2 - TestFlash Information Register 2 */
	vuint32_t R;
	struct {
		vuint32_t TINFO2 : 32; /* General purpose TestFlash word 2 */
	} B;
} SSCM_TF_INFO2_32B_tag;

typedef union { /* SSCM_TF_INFO3 - TestFlash Information Register 3 */
	vuint32_t R;
	struct {
		vuint32_t TINFO3 : 32; /* General purpose TestFlash word */
	} B;
} SSCM_TF_INFO3_32B_tag;

typedef struct SSCM_struct_tag { /* start of SSCM_tag */
																 /* SSCM_STATUS - System Status Register */
	SSCM_STATUS_16B_tag STATUS;		 /* offset: 0x0000 size: 16 bit */
	/* SSCM_MEMCONFIG - System Memory Configuration Register */
	SSCM_MEMCONFIG_16B_tag MEMCONFIG; /* offset: 0x0002 size: 16 bit */
	int8_t SSCM_reserved_0004[2];
	/* SSCM_ERROR - Error Configuration */
	SSCM_ERROR_16B_tag ERROR; /* offset: 0x0006 size: 16 bit */
														/* SSCM_DEBUGPORT - Debug Status Port Register */
	SSCM_DEBUGPORT_16B_tag DEBUGPORT; /* offset: 0x0008 size: 16 bit */
	int8_t SSCM_reserved_000A[2];
	/* SSCM_PWCMPH - Password Comparison Register High */
	SSCM_PWCMPH_32B_tag PWCMPH; /* offset: 0x000C size: 32 bit */
	/* SSCM_PWCMPL - Password Comparison Register Low */
	SSCM_PWCMPL_32B_tag PWCMPL; /* offset: 0x0010 size: 32 bit */
	int8_t SSCM_reserved_0014[4];
	/* SSCM_DPMBOOT - Decoupled Parallel Boot Register */
	SSCM_DPMBOOT_32B_tag DPMBOOT; /* offset: 0x0018 size: 32 bit */
																/* SSCM_DPMKEY - Boot Key Register */
	SSCM_DPMKEY_32B_tag DPMKEY;		/* offset: 0x001C size: 32 bit */
																/* SSCM_UOPS - User Option Status Register */
	SSCM_UOPS_32B_tag UOPS;				/* offset: 0x0020 size: 32 bit */
																/* SSCM_SCTR - SSCM Control Register */
	SSCM_SCTR_32B_tag SCTR;				/* offset: 0x0024 size: 32 bit */
	/* SSCM_TF_INFO0 - TestFlash Information Register 0 */
	SSCM_TF_INFO0_32B_tag TF_INFO0; /* offset: 0x0028 size: 32 bit */
	/* SSCM_TF_INFO1 - TestFlash Information Register 1 */
	SSCM_TF_INFO1_32B_tag TF_INFO1; /* offset: 0x002C size: 32 bit */
	/* SSCM_TF_INFO2 - TestFlash Information Register 2 */
	SSCM_TF_INFO2_32B_tag TF_INFO2; /* offset: 0x0030 size: 32 bit */
	/* SSCM_TF_INFO3 - TestFlash Information Register 3 */
	SSCM_TF_INFO3_32B_tag TF_INFO3; /* offset: 0x0034 size: 32 bit */
} SSCM_tag;

#define SSCM (*(volatile SSCM_tag*)0xC3FD8000UL)

/****************************************************************/
/*                                                              */
/* Module: ME  */
/*                                                              */
/****************************************************************/

typedef union { /* ME_GS - Global Status Register */
	vuint32_t R;
	struct {
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t S_CURRENT_MODE : 4; /* Current device mode status */
#else
		vuint32_t S_CURRENTMODE : 4; /* deprecated name - please avoid */
#endif
		vuint32_t S_MTRANS : 1; /* Mode transition status */
		vuint32_t : 3;
		vuint32_t S_PDO : 1; /* Output power-down status */
		vuint32_t : 2;
		vuint32_t S_MVR : 1; /* Main voltage regulator status */
		vuint32_t : 2;
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t S_FLA : 2; /* Flash availability status */
#else
		vuint32_t S_CFLA : 2;				 /* deprecated name - please avoid */
#endif
		vuint32_t : 8;
		vuint32_t S_PLL1 : 1; /* Secondary PLL status */
		vuint32_t S_PLL0 : 1; /* System PLL status */
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t S_XOSC : 1; /* System crystal oscillator status */
#else
		vuint32_t S_OSC : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t S_IRCOSC : 1; /* System RC oscillator status */
#else
		vuint32_t S_RC : 1;					 /* deprecated name - please avoid */
#endif
		vuint32_t S_SYSCLK : 4; /* System clock switch status */
	} B;
} ME_GS_32B_tag;

typedef union { /* ME_MCTL - Mode Control Register */
	vuint32_t R;
	struct {
		vuint32_t TARGET_MODE : 4; /* Target device mode */
		vuint32_t : 12;
		vuint32_t KEY : 16; /* Control key */
	} B;
} ME_MCTL_32B_tag;

typedef union { /* ME_MEN - Mode Enable Register */
	vuint32_t R;
	struct {
		vuint32_t : 21;
		vuint32_t STOP0 : 1; /* STOP0 mode enable */
		vuint32_t : 1;
		vuint32_t HALT0 : 1; /* HALT0 mode enable */
		vuint32_t RUN3 : 1;	 /* RUN3 mode enable */
		vuint32_t RUN2 : 1;	 /* RUN2 mode enable */
		vuint32_t RUN1 : 1;	 /* RUN1 mode enable */
		vuint32_t RUN0 : 1;	 /* RUN0 mode enable */
		vuint32_t DRUN : 1;	 /* DRUN mode enable */
		vuint32_t SAFE : 1;	 /* SAFE mode enable */
		vuint32_t : 1;
		vuint32_t RESET : 1; /* RESET mode enable */
	} B;
} ME_MEN_32B_tag;

typedef union { /* ME_IS - Interrupt Status Register */
	vuint32_t R;
	struct {
		vuint32_t : 28;
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t I_ICONF : 1; /* Invalid mode config interrupt */
#else
		vuint32_t I_CONF : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t I_IMODE : 1; /* Invalid mode interrupt */
#else
		vuint32_t I_MODE : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t I_SAFE : 1; /* SAFE mode interrupt */
#else
		vuint32_t I_AFE : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t I_MTC : 1; /* Mode transition complete interrupt */
#else
		vuint32_t I_TC : 1;					 /* deprecated name - please avoid */
#endif
	} B;
} ME_IS_32B_tag;

typedef union { /* ME_IM - Interrupt Mask Register */
	vuint32_t R;
	struct {
		vuint32_t : 28;
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t M_ICONF : 1; /* Invalid mode config interrupt mask */
#else
		vuint32_t M_CONF : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t M_IMODE : 1; /* Invalid mode interrupt mask */
#else
		vuint32_t M_MODE : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t M_SAFE : 1; /* SAFE mode interrupt mask */
#else
		vuint32_t M_AFE : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t M_MTC : 1; /* Mode transition complete interrupt mask */
#else
		vuint32_t M_TC : 1;					 /* deprecated name - please avoid */
#endif
	} B;
} ME_IM_32B_tag;

typedef union { /* ME_IMTS - Invalid Mode Transition Status Register */
	vuint32_t R;
	struct {
		vuint32_t : 27;
		vuint32_t S_MTI : 1; /* Mode Transition Illegal status */
		vuint32_t S_MRI : 1; /* Mode Request Illegal status */
		vuint32_t S_DMA : 1; /* Disabled Mode Access status */
		vuint32_t S_NMA : 1; /* Non-existing Mode Access status */
		vuint32_t S_SEA : 1; /* Safe Event Active status */
	} B;
} ME_IMTS_32B_tag;

typedef union { /* ME_DMTS - Debug Mode Transition Status Register */
	vuint32_t R;
	struct {
		vuint32_t PREVIOUS_MODE : 4; /* Previous Device Mode */
		vuint32_t : 4;
		vuint32_t MPH_BUSY : 1; /* MC_ME/MC_PCU Handshake Busy Indicator */
		vuint32_t : 2;
		vuint32_t PMC_PROG : 1; /* MC_PCU Mode Change in Process Indicator */
		vuint32_t CORE_DBG : 1; /* Processor is in Debug Mode Indicator */
		vuint32_t : 2;
		vuint32_t SMR : 1; /* SAFE Mode Request */
		vuint32_t : 1;
		vuint32_t
				VREG_CSRC_SC : 1; /* Main VREG Clock Source State Change Indicator */
		vuint32_t CSRC_CSRC_SC : 1; /* Other Clock Source State Change Indicator */
		vuint32_t IRCOSC_SC : 1;		/* IRCOSC State Change Indicator */
		vuint32_t SCSRC_SC : 1;			/* Secondary System Clock Sources State Change
																	 Indicator */
		vuint32_t
				SYSCLK_SW : 1; /* System Clock Switching pending Status Indicator */
		vuint32_t : 1;
		vuint32_t FLASH_SC : 1;				/* FLASH State Change Indicator */
		vuint32_t CDP_PRPH_0_143 : 1; /* Clock Disable Process Pending Status for
																		 Periph. 0-143 */
		vuint32_t : 4;
		vuint32_t CDP_PRPH_64_95 : 1; /* Clock Disable Process Pending Status for
																		 Periph. 64-95 */
		vuint32_t CDP_PRPH_32_63 : 1; /* Clock Disable Process Pending Status for
																		 Periph. 32-63 */
		vuint32_t CDP_PRPH_0_31 : 1;	/* Clock Disable Process Pending Status for
																		 Periph. 0-31 */
	} B;
} ME_DMTS_32B_tag;

typedef union { /* ME_RESET_MC - RESET Mode Configuration Register */
	vuint32_t R;
	struct {
		vuint32_t : 8;
		vuint32_t PDO : 1; /* IOs output power-down control */
		vuint32_t : 2;
		vuint32_t MVRON : 1; /* Main voltage regulator control */
		vuint32_t : 2;
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t FLAON : 2; /* Code flash power-down control */
#else
		vuint32_t CFLAON : 2;				 /* deprecated name - please avoid */
#endif
		vuint32_t : 8;
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t PLL1ON : 1; /* Secondary system clock source [8..0] control */
#else
		vuint32_t PLL2ON : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t PLL0ON : 1; /* System PLL control */
#else
		vuint32_t PLL1ON : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t XOSCON : 1; /* System crystal oscillator control */
#else
		vuint32_t XOSC0ON : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t IRCOSCON : 1; /* System RC oscillator control */
#else
		vuint32_t IRCON : 1;				 /* deprecated name - please avoid */
#endif
		vuint32_t SYSCLK : 4; /* System clock switch control */
	} B;
} ME_RESET_MC_32B_tag;

typedef union { /* ME_SAFE_MC - Mode Configuration Register */
	vuint32_t R;
	struct {
		vuint32_t : 8;
		vuint32_t PDO : 1; /* IOs output power-down control */
		vuint32_t : 2;
		vuint32_t MVRON : 1; /* Main voltage regulator control */
		vuint32_t : 2;
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t FLAON : 2; /* Code flash power-down control */
#else
		vuint32_t CFLAON : 2;				 /* deprecated name - please avoid */
#endif
		vuint32_t : 8;
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t PLL1ON : 1; /* Secondary system clock source [8..0] control */
#else
		vuint32_t PLL2ON : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t PLL0ON : 1; /* System PLL control */
#else
		vuint32_t PLL1ON : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t XOSCON : 1; /* System crystal oscillator control */
#else
		vuint32_t XOSC0ON : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t IRCOSCON : 1; /* System RC oscillator control */
#else
		vuint32_t IRCON : 1;				 /* deprecated name - please avoid */
#endif
		vuint32_t SYSCLK : 4; /* System clock switch control */
	} B;
} ME_SAFE_MC_32B_tag;

typedef union { /* ME_DRUN_MC - DRUN Mode Configuration Register */
	vuint32_t R;
	struct {
		vuint32_t : 8;
		vuint32_t PDO : 1; /* IOs output power-down control */
		vuint32_t : 2;
		vuint32_t MVRON : 1; /* Main voltage regulator control */
		vuint32_t : 2;
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t FLAON : 2; /* Code flash power-down control */
#else
		vuint32_t CFLAON : 2;				 /* deprecated name - please avoid */
#endif
		vuint32_t : 8;
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t PLL1ON : 1; /* Secondary system clock source [8..0] control */
#else
		vuint32_t PLL2ON : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t PLL0ON : 1; /* System PLL control */
#else
		vuint32_t PLL1ON : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t XOSCON : 1; /* System crystal oscillator control */
#else
		vuint32_t XOSC0ON : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t IRCOSCON : 1; /* System RC oscillator control */
#else
		vuint32_t IRCON : 1;				 /* deprecated name - please avoid */
#endif
		vuint32_t SYSCLK : 4; /* System clock switch control */
	} B;
} ME_DRUN_MC_32B_tag;

/* Register layout for all registers RUN_MC... */

typedef union { /* ME_RUN[0..3]_MC - RUN[0..3] Mode Configuration Registers */
	vuint32_t R;
	struct {
		vuint32_t : 8;
		vuint32_t PDO : 1; /* IOs output power-down control */
		vuint32_t : 2;
		vuint32_t MVRON : 1; /* Main voltage regulator control */
		vuint32_t : 2;
		vuint32_t FLAON : 2; /* Code flash power-down control */
		vuint32_t : 8;
		vuint32_t PLL1ON : 1;		/* Secondary system clock source [8..0] control */
		vuint32_t PLL0ON : 1;		/* System PLL control */
		vuint32_t XOSCON : 1;		/* System crystal oscillator control */
		vuint32_t IRCOSCON : 1; /* System RC oscillator control */
		vuint32_t SYSCLK : 4;		/* System clock switch control */
	} B;
} ME_RUN_MC_32B_tag;

typedef union { /* ME_HALT0_MC - HALT0 Mode Configuration Register */
	vuint32_t R;
	struct {
		vuint32_t : 8;
		vuint32_t PDO : 1; /* IOs output power-down control */
		vuint32_t : 2;
		vuint32_t MVRON : 1; /* Main voltage regulator control */
		vuint32_t : 2;
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t FLAON : 2; /* Code flash power-down control */
#else
		vuint32_t CFLAON : 2;				 /* deprecated name - please avoid */
#endif
		vuint32_t : 8;
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t PLL1ON : 1; /* Secondary system clock source [8..0] control */
#else
		vuint32_t PLL2ON : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t PLL0ON : 1; /* System PLL control */
#else
		vuint32_t PLL1ON : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t XOSCON : 1; /* System crystal oscillator control */
#else
		vuint32_t XOSC0ON : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t IRCOSCON : 1; /* System RC oscillator control */
#else
		vuint32_t IRCON : 1;				 /* deprecated name - please avoid */
#endif
		vuint32_t SYSCLK : 4; /* System clock switch control */
	} B;
} ME_HALT0_MC_32B_tag;

typedef union { /* ME_STOP0_MC - STOP0 Mode Configration Register */
	vuint32_t R;
	struct {
		vuint32_t : 8;
		vuint32_t PDO : 1; /* IOs output power-down control */
		vuint32_t : 2;
		vuint32_t MVRON : 1; /* Main voltage regulator control */
		vuint32_t : 2;
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t FLAON : 2; /* Code flash power-down control */
#else
		vuint32_t CFLAON : 2;				 /* deprecated name - please avoid */
#endif
		vuint32_t : 8;
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t PLL1ON : 1; /* Secondary system clock source [8..0] control */
#else
		vuint32_t PLL2ON : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t PLL0ON : 1; /* System PLL control */
#else
		vuint32_t PLL1ON : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t XOSCON : 1; /* System crystal oscillator control */
#else
		vuint32_t XOSC0ON : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t IRCOSCON : 1; /* System RC oscillator control */
#else
		vuint32_t IRCON : 1;				 /* deprecated name - please avoid */
#endif
		vuint32_t SYSCLK : 4; /* System clock switch control */
	} B;
} ME_STOP0_MC_32B_tag;

typedef union { /* ME_STANDBY0_MC - STANDBY0 Mode Configration Register */
	vuint32_t R;
	struct {
		vuint32_t : 8;
		vuint32_t PDO : 1; /* IOs output power-down control */
		vuint32_t : 2;
		vuint32_t MVRON : 1; /* Main voltage regulator control */
		vuint32_t : 2;
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t FLAON : 2; /* Code flash power-down control */
#else
		vuint32_t CFLAON : 2;				 /* deprecated name - please avoid */
#endif
		vuint32_t : 8;
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t PLL1ON : 1; /* Secondary system clock source [8..0] control */
#else
		vuint32_t PLL2ON : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t PLL0ON : 1; /* System PLL control */
#else
		vuint32_t PLL1ON : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t XOSCON : 1; /* System crystal oscillator control */
#else
		vuint32_t XOSC0ON : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ME
		vuint32_t IRCOSCON : 1; /* System RC oscillator control */
#else
		vuint32_t IRCON : 1;				 /* deprecated name - please avoid */
#endif
		vuint32_t SYSCLK : 4; /* System clock switch control */
	} B;
} ME_STANDBY0_MC_32B_tag;

typedef union { /* ME_PS0 - Peripheral Status Register 0 */
	vuint32_t R;
	struct {
		vuint32_t : 7;
		vuint32_t S_FLEXRAY : 1; /* FlexRay status */
		vuint32_t : 6;
		vuint32_t S_FLEXCAN1 : 1; /* FlexCAN1 status */
		vuint32_t S_FLEXCAN0 : 1; /* FlexCAN0 status */
		vuint32_t : 9;
		vuint32_t S_DSPI2 : 1; /* DSPI2 status */
		vuint32_t S_DSPI1 : 1; /* DSPI1 status */
		vuint32_t S_DSPI0 : 1; /* DSPI0 status */
		vuint32_t : 4;
	} B;
} ME_PS0_32B_tag;

typedef union { /* ME_PS1 - Peripheral Status Register 1 */
	vuint32_t R;
	struct {
		vuint32_t : 1;
		vuint32_t S_SWG : 1; /* SWG status */
		vuint32_t : 3;
		vuint32_t S_CRC : 1; /* CRC status */
		vuint32_t : 8;
		vuint32_t S_LIN_FLEX1 : 1; /* LinFlex1 status */
		vuint32_t S_LIN_FLEX0 : 1; /* LinFlex0 status */
		vuint32_t : 5;
		vuint32_t S_FLEXPWM1 : 1; /* FlexPWM1 status */
		vuint32_t S_FLEXPWM0 : 1; /* FlexPWM0 status */
		vuint32_t S_ETIMER2 : 1;	/* eTimer2 status */
		vuint32_t S_ETIMER1 : 1;	/* eTimer1 status */
		vuint32_t S_ETIMER0 : 1;	/* eTimer0 status */
		vuint32_t : 2;
		vuint32_t S_CTU : 1; /* CTU status */
		vuint32_t : 1;
		vuint32_t S_ADC1 : 1; /* ADC1 status */
		vuint32_t S_ADC0 : 1; /* ADC0 status */
	} B;
} ME_PS1_32B_tag;

typedef union { /* ME_PS2 - Peripheral Status Register 2 */
	vuint32_t R;
	struct {
		vuint32_t : 3;
		vuint32_t S_PIT : 1; /* PIT status */
		vuint32_t : 28;
	} B;
} ME_PS2_32B_tag;

/* Register layout for all registers RUN_PC... */

typedef union { /* ME_RUN_PC[0...7] - RUN Peripheral Configuration Registers */
	vuint32_t R;
	struct {
		vuint32_t : 24;
		vuint32_t RUN3 : 1;	 /* Peripheral control during RUN3 */
		vuint32_t RUN2 : 1;	 /* Peripheral control during RUN2 */
		vuint32_t RUN1 : 1;	 /* Peripheral control during RUN1 */
		vuint32_t RUN0 : 1;	 /* Peripheral control during RUN0 */
		vuint32_t DRUN : 1;	 /* Peripheral control during DRUN */
		vuint32_t SAFE : 1;	 /* Peripheral control during SAFE */
		vuint32_t TEST : 1;	 /* Peripheral control during TEST */
		vuint32_t RESET : 1; /* Peripheral control during RESET */
	} B;
} ME_RUN_PC_32B_tag;

/* Register layout for all registers LP_PC... */

typedef union { /* ME_LP_PC[0...7] - Low Power Peripheral Configuration
									 Registers */
	vuint32_t R;
	struct {
		vuint32_t : 21;
		vuint32_t STOP0 : 1; /* Peripheral control during STOP0 */
		vuint32_t : 1;
		vuint32_t HALT0 : 1; /* Peripheral control during HALT0 */
		vuint32_t : 8;
	} B;
} ME_LP_PC_32B_tag;

/* Register layout for all registers PCTL... */

typedef union { /* ME_PCTL[0...143] - Peripheral Control Registers */
	vuint8_t R;
	struct {
		vuint8_t : 1;
		vuint8_t DBG_F : 1;	 /* Peripheral control in debug mode */
		vuint8_t LP_CFG : 3; /* Peripheral configuration select for non-RUN modes */
		vuint8_t RUN_CFG : 3; /* Peripheral configuration select for RUN modes */
	} B;
} ME_PCTL_8B_tag;

/* Register layout for generated register(s) PS... */

typedef union { /*  */
	vuint32_t R;
} ME_PS_32B_tag;

typedef struct ME_struct_tag { /* start of ME_tag */
															 /* ME_GS - Global Status Register */
	ME_GS_32B_tag GS;						 /* offset: 0x0000 size: 32 bit */
															 /* ME_MCTL - Mode Control Register */
	ME_MCTL_32B_tag MCTL;				 /* offset: 0x0004 size: 32 bit */
	union {
		ME_MEN_32B_tag MER; /* deprecated - please avoid */

		/* ME_MEN - Mode Enable Register */
		ME_MEN_32B_tag MEN; /* offset: 0x0008 size: 32 bit */
	};
	/* ME_IS - Interrupt Status Register */
	ME_IS_32B_tag IS;			/* offset: 0x000C size: 32 bit */
												/* ME_IM - Interrupt Mask Register */
	ME_IM_32B_tag IM;			/* offset: 0x0010 size: 32 bit */
												/* ME_IMTS - Invalid Mode Transition Status Register */
	ME_IMTS_32B_tag IMTS; /* offset: 0x0014 size: 32 bit */
												/* ME_DMTS - Debug Mode Transition Status Register */
	ME_DMTS_32B_tag DMTS; /* offset: 0x0018 size: 32 bit */
	int8_t ME_reserved_001C_C[4];
	union {
		/* ME_RESET_MC - RESET Mode Configuration Register */
		ME_RESET_MC_32B_tag RESET_MC; /* offset: 0x0020 size: 32 bit */

		ME_RESET_MC_32B_tag RESET; /* deprecated - please avoid */
	};
	int8_t ME_reserved_0024_C[4];
	union {
		/* ME_SAFE_MC - Mode Configuration Register */
		ME_SAFE_MC_32B_tag SAFE_MC; /* offset: 0x0028 size: 32 bit */

		ME_SAFE_MC_32B_tag SAFE; /* deprecated - please avoid */
	};
	union {
		/* ME_DRUN_MC - DRUN Mode Configuration Register */
		ME_DRUN_MC_32B_tag DRUN_MC; /* offset: 0x002C size: 32 bit */

		ME_DRUN_MC_32B_tag DRUN; /* deprecated - please avoid */
	};
	union {
		/* ME_RUN[0..3]_MC - RUN[0..3] Mode Configuration Registers */
		ME_RUN_MC_32B_tag RUN_MC[4]; /* offset: 0x0030  (0x0004 x 4) */

		ME_RUN_MC_32B_tag RUN[4];
				/* offset: 0x0030  (0x0004 x 4) */ /* deprecated - please avoid */

		struct {
			/* ME_RUN[0..3]_MC - RUN[0..3] Mode Configuration Registers */
			ME_RUN_MC_32B_tag RUN0_MC; /* offset: 0x0030 size: 32 bit */
			ME_RUN_MC_32B_tag RUN1_MC; /* offset: 0x0034 size: 32 bit */
			ME_RUN_MC_32B_tag RUN2_MC; /* offset: 0x0038 size: 32 bit */
			ME_RUN_MC_32B_tag RUN3_MC; /* offset: 0x003C size: 32 bit */
		};
	};
	union {
		/* ME_HALT0_MC - HALT0 Mode Configuration Register */
		ME_HALT0_MC_32B_tag HALT0_MC; /* offset: 0x0040 size: 32 bit */

		ME_HALT0_MC_32B_tag HALT0; /* deprecated - please avoid */
	};
	int8_t ME_reserved_0044_C[4];
	union {
		/* ME_STOP0_MC - STOP0 Mode Configration Register */
		ME_STOP0_MC_32B_tag STOP0_MC; /* offset: 0x0048 size: 32 bit */

		ME_STOP0_MC_32B_tag STOP0; /* deprecated - please avoid */
	};
	int8_t ME_reserved_004C_C[8];
	union {
		/* ME_STANDBY0_MC - STANDBY0 Mode Configration Register */
		ME_STANDBY0_MC_32B_tag STANDBY0_MC; /* offset: 0x0054 size: 32 bit */

		ME_STANDBY0_MC_32B_tag STANDBY0; /* deprecated - please avoid */
	};
	int8_t ME_reserved_0058_C[8];
	union {
		ME_PS_32B_tag PS[3]; /* offset: 0x0060  (0x0004 x 3) */

		struct {
			/* ME_PS0 - Peripheral Status Register 0 */
			ME_PS0_32B_tag PS0; /* offset: 0x0060 size: 32 bit */
													/* ME_PS1 - Peripheral Status Register 1 */
			ME_PS1_32B_tag PS1; /* offset: 0x0064 size: 32 bit */
													/* ME_PS2 - Peripheral Status Register 2 */
			ME_PS2_32B_tag PS2; /* offset: 0x0068 size: 32 bit */
		};
	};
	int8_t ME_reserved_006C_C[20];
	union {
		ME_RUN_PC_32B_tag RUNPC[8]; /* offset: 0x0080  (0x0004 x 8) */

		/* ME_RUN_PC[0...7] - RUN Peripheral Configuration Registers */
		ME_RUN_PC_32B_tag RUN_PC[8]; /* offset: 0x0080  (0x0004 x 8) */

		struct {
			/* ME_RUN_PC[0...7] - RUN Peripheral Configuration Registers */
			ME_RUN_PC_32B_tag RUN_PC0; /* offset: 0x0080 size: 32 bit */
			ME_RUN_PC_32B_tag RUN_PC1; /* offset: 0x0084 size: 32 bit */
			ME_RUN_PC_32B_tag RUN_PC2; /* offset: 0x0088 size: 32 bit */
			ME_RUN_PC_32B_tag RUN_PC3; /* offset: 0x008C size: 32 bit */
			ME_RUN_PC_32B_tag RUN_PC4; /* offset: 0x0090 size: 32 bit */
			ME_RUN_PC_32B_tag RUN_PC5; /* offset: 0x0094 size: 32 bit */
			ME_RUN_PC_32B_tag RUN_PC6; /* offset: 0x0098 size: 32 bit */
			ME_RUN_PC_32B_tag RUN_PC7; /* offset: 0x009C size: 32 bit */
		};
	};
	union {
		ME_LP_PC_32B_tag LPPC[8]; /* offset: 0x00A0  (0x0004 x 8) */

		/* ME_LP_PC[0...7] - Low Power Peripheral Configuration Registers */
		ME_LP_PC_32B_tag LP_PC[8]; /* offset: 0x00A0  (0x0004 x 8) */

		struct {
			/* ME_LP_PC[0...7] - Low Power Peripheral Configuration Registers */
			ME_LP_PC_32B_tag LP_PC0; /* offset: 0x00A0 size: 32 bit */
			ME_LP_PC_32B_tag LP_PC1; /* offset: 0x00A4 size: 32 bit */
			ME_LP_PC_32B_tag LP_PC2; /* offset: 0x00A8 size: 32 bit */
			ME_LP_PC_32B_tag LP_PC3; /* offset: 0x00AC size: 32 bit */
			ME_LP_PC_32B_tag LP_PC4; /* offset: 0x00B0 size: 32 bit */
			ME_LP_PC_32B_tag LP_PC5; /* offset: 0x00B4 size: 32 bit */
			ME_LP_PC_32B_tag LP_PC6; /* offset: 0x00B8 size: 32 bit */
			ME_LP_PC_32B_tag LP_PC7; /* offset: 0x00BC size: 32 bit */
		};
	};
	union {
		/* ME_PCTL[0...143] - Peripheral Control Registers */
		ME_PCTL_8B_tag PCTL[144]; /* offset: 0x00C0  (0x0001 x 144) */

		struct {
			/* ME_PCTL[0...143] - Peripheral Control Registers */
			ME_PCTL_8B_tag PCTL0;		/* offset: 0x00C0 size: 8 bit */
			ME_PCTL_8B_tag PCTL1;		/* offset: 0x00C1 size: 8 bit */
			ME_PCTL_8B_tag PCTL2;		/* offset: 0x00C2 size: 8 bit */
			ME_PCTL_8B_tag PCTL3;		/* offset: 0x00C3 size: 8 bit */
			ME_PCTL_8B_tag PCTL4;		/* offset: 0x00C4 size: 8 bit */
			ME_PCTL_8B_tag PCTL5;		/* offset: 0x00C5 size: 8 bit */
			ME_PCTL_8B_tag PCTL6;		/* offset: 0x00C6 size: 8 bit */
			ME_PCTL_8B_tag PCTL7;		/* offset: 0x00C7 size: 8 bit */
			ME_PCTL_8B_tag PCTL8;		/* offset: 0x00C8 size: 8 bit */
			ME_PCTL_8B_tag PCTL9;		/* offset: 0x00C9 size: 8 bit */
			ME_PCTL_8B_tag PCTL10;	/* offset: 0x00CA size: 8 bit */
			ME_PCTL_8B_tag PCTL11;	/* offset: 0x00CB size: 8 bit */
			ME_PCTL_8B_tag PCTL12;	/* offset: 0x00CC size: 8 bit */
			ME_PCTL_8B_tag PCTL13;	/* offset: 0x00CD size: 8 bit */
			ME_PCTL_8B_tag PCTL14;	/* offset: 0x00CE size: 8 bit */
			ME_PCTL_8B_tag PCTL15;	/* offset: 0x00CF size: 8 bit */
			ME_PCTL_8B_tag PCTL16;	/* offset: 0x00D0 size: 8 bit */
			ME_PCTL_8B_tag PCTL17;	/* offset: 0x00D1 size: 8 bit */
			ME_PCTL_8B_tag PCTL18;	/* offset: 0x00D2 size: 8 bit */
			ME_PCTL_8B_tag PCTL19;	/* offset: 0x00D3 size: 8 bit */
			ME_PCTL_8B_tag PCTL20;	/* offset: 0x00D4 size: 8 bit */
			ME_PCTL_8B_tag PCTL21;	/* offset: 0x00D5 size: 8 bit */
			ME_PCTL_8B_tag PCTL22;	/* offset: 0x00D6 size: 8 bit */
			ME_PCTL_8B_tag PCTL23;	/* offset: 0x00D7 size: 8 bit */
			ME_PCTL_8B_tag PCTL24;	/* offset: 0x00D8 size: 8 bit */
			ME_PCTL_8B_tag PCTL25;	/* offset: 0x00D9 size: 8 bit */
			ME_PCTL_8B_tag PCTL26;	/* offset: 0x00DA size: 8 bit */
			ME_PCTL_8B_tag PCTL27;	/* offset: 0x00DB size: 8 bit */
			ME_PCTL_8B_tag PCTL28;	/* offset: 0x00DC size: 8 bit */
			ME_PCTL_8B_tag PCTL29;	/* offset: 0x00DD size: 8 bit */
			ME_PCTL_8B_tag PCTL30;	/* offset: 0x00DE size: 8 bit */
			ME_PCTL_8B_tag PCTL31;	/* offset: 0x00DF size: 8 bit */
			ME_PCTL_8B_tag PCTL32;	/* offset: 0x00E0 size: 8 bit */
			ME_PCTL_8B_tag PCTL33;	/* offset: 0x00E1 size: 8 bit */
			ME_PCTL_8B_tag PCTL34;	/* offset: 0x00E2 size: 8 bit */
			ME_PCTL_8B_tag PCTL35;	/* offset: 0x00E3 size: 8 bit */
			ME_PCTL_8B_tag PCTL36;	/* offset: 0x00E4 size: 8 bit */
			ME_PCTL_8B_tag PCTL37;	/* offset: 0x00E5 size: 8 bit */
			ME_PCTL_8B_tag PCTL38;	/* offset: 0x00E6 size: 8 bit */
			ME_PCTL_8B_tag PCTL39;	/* offset: 0x00E7 size: 8 bit */
			ME_PCTL_8B_tag PCTL40;	/* offset: 0x00E8 size: 8 bit */
			ME_PCTL_8B_tag PCTL41;	/* offset: 0x00E9 size: 8 bit */
			ME_PCTL_8B_tag PCTL42;	/* offset: 0x00EA size: 8 bit */
			ME_PCTL_8B_tag PCTL43;	/* offset: 0x00EB size: 8 bit */
			ME_PCTL_8B_tag PCTL44;	/* offset: 0x00EC size: 8 bit */
			ME_PCTL_8B_tag PCTL45;	/* offset: 0x00ED size: 8 bit */
			ME_PCTL_8B_tag PCTL46;	/* offset: 0x00EE size: 8 bit */
			ME_PCTL_8B_tag PCTL47;	/* offset: 0x00EF size: 8 bit */
			ME_PCTL_8B_tag PCTL48;	/* offset: 0x00F0 size: 8 bit */
			ME_PCTL_8B_tag PCTL49;	/* offset: 0x00F1 size: 8 bit */
			ME_PCTL_8B_tag PCTL50;	/* offset: 0x00F2 size: 8 bit */
			ME_PCTL_8B_tag PCTL51;	/* offset: 0x00F3 size: 8 bit */
			ME_PCTL_8B_tag PCTL52;	/* offset: 0x00F4 size: 8 bit */
			ME_PCTL_8B_tag PCTL53;	/* offset: 0x00F5 size: 8 bit */
			ME_PCTL_8B_tag PCTL54;	/* offset: 0x00F6 size: 8 bit */
			ME_PCTL_8B_tag PCTL55;	/* offset: 0x00F7 size: 8 bit */
			ME_PCTL_8B_tag PCTL56;	/* offset: 0x00F8 size: 8 bit */
			ME_PCTL_8B_tag PCTL57;	/* offset: 0x00F9 size: 8 bit */
			ME_PCTL_8B_tag PCTL58;	/* offset: 0x00FA size: 8 bit */
			ME_PCTL_8B_tag PCTL59;	/* offset: 0x00FB size: 8 bit */
			ME_PCTL_8B_tag PCTL60;	/* offset: 0x00FC size: 8 bit */
			ME_PCTL_8B_tag PCTL61;	/* offset: 0x00FD size: 8 bit */
			ME_PCTL_8B_tag PCTL62;	/* offset: 0x00FE size: 8 bit */
			ME_PCTL_8B_tag PCTL63;	/* offset: 0x00FF size: 8 bit */
			ME_PCTL_8B_tag PCTL64;	/* offset: 0x0100 size: 8 bit */
			ME_PCTL_8B_tag PCTL65;	/* offset: 0x0101 size: 8 bit */
			ME_PCTL_8B_tag PCTL66;	/* offset: 0x0102 size: 8 bit */
			ME_PCTL_8B_tag PCTL67;	/* offset: 0x0103 size: 8 bit */
			ME_PCTL_8B_tag PCTL68;	/* offset: 0x0104 size: 8 bit */
			ME_PCTL_8B_tag PCTL69;	/* offset: 0x0105 size: 8 bit */
			ME_PCTL_8B_tag PCTL70;	/* offset: 0x0106 size: 8 bit */
			ME_PCTL_8B_tag PCTL71;	/* offset: 0x0107 size: 8 bit */
			ME_PCTL_8B_tag PCTL72;	/* offset: 0x0108 size: 8 bit */
			ME_PCTL_8B_tag PCTL73;	/* offset: 0x0109 size: 8 bit */
			ME_PCTL_8B_tag PCTL74;	/* offset: 0x010A size: 8 bit */
			ME_PCTL_8B_tag PCTL75;	/* offset: 0x010B size: 8 bit */
			ME_PCTL_8B_tag PCTL76;	/* offset: 0x010C size: 8 bit */
			ME_PCTL_8B_tag PCTL77;	/* offset: 0x010D size: 8 bit */
			ME_PCTL_8B_tag PCTL78;	/* offset: 0x010E size: 8 bit */
			ME_PCTL_8B_tag PCTL79;	/* offset: 0x010F size: 8 bit */
			ME_PCTL_8B_tag PCTL80;	/* offset: 0x0110 size: 8 bit */
			ME_PCTL_8B_tag PCTL81;	/* offset: 0x0111 size: 8 bit */
			ME_PCTL_8B_tag PCTL82;	/* offset: 0x0112 size: 8 bit */
			ME_PCTL_8B_tag PCTL83;	/* offset: 0x0113 size: 8 bit */
			ME_PCTL_8B_tag PCTL84;	/* offset: 0x0114 size: 8 bit */
			ME_PCTL_8B_tag PCTL85;	/* offset: 0x0115 size: 8 bit */
			ME_PCTL_8B_tag PCTL86;	/* offset: 0x0116 size: 8 bit */
			ME_PCTL_8B_tag PCTL87;	/* offset: 0x0117 size: 8 bit */
			ME_PCTL_8B_tag PCTL88;	/* offset: 0x0118 size: 8 bit */
			ME_PCTL_8B_tag PCTL89;	/* offset: 0x0119 size: 8 bit */
			ME_PCTL_8B_tag PCTL90;	/* offset: 0x011A size: 8 bit */
			ME_PCTL_8B_tag PCTL91;	/* offset: 0x011B size: 8 bit */
			ME_PCTL_8B_tag PCTL92;	/* offset: 0x011C size: 8 bit */
			ME_PCTL_8B_tag PCTL93;	/* offset: 0x011D size: 8 bit */
			ME_PCTL_8B_tag PCTL94;	/* offset: 0x011E size: 8 bit */
			ME_PCTL_8B_tag PCTL95;	/* offset: 0x011F size: 8 bit */
			ME_PCTL_8B_tag PCTL96;	/* offset: 0x0120 size: 8 bit */
			ME_PCTL_8B_tag PCTL97;	/* offset: 0x0121 size: 8 bit */
			ME_PCTL_8B_tag PCTL98;	/* offset: 0x0122 size: 8 bit */
			ME_PCTL_8B_tag PCTL99;	/* offset: 0x0123 size: 8 bit */
			ME_PCTL_8B_tag PCTL100; /* offset: 0x0124 size: 8 bit */
			ME_PCTL_8B_tag PCTL101; /* offset: 0x0125 size: 8 bit */
			ME_PCTL_8B_tag PCTL102; /* offset: 0x0126 size: 8 bit */
			ME_PCTL_8B_tag PCTL103; /* offset: 0x0127 size: 8 bit */
			ME_PCTL_8B_tag PCTL104; /* offset: 0x0128 size: 8 bit */
			ME_PCTL_8B_tag PCTL105; /* offset: 0x0129 size: 8 bit */
			ME_PCTL_8B_tag PCTL106; /* offset: 0x012A size: 8 bit */
			ME_PCTL_8B_tag PCTL107; /* offset: 0x012B size: 8 bit */
			ME_PCTL_8B_tag PCTL108; /* offset: 0x012C size: 8 bit */
			ME_PCTL_8B_tag PCTL109; /* offset: 0x012D size: 8 bit */
			ME_PCTL_8B_tag PCTL110; /* offset: 0x012E size: 8 bit */
			ME_PCTL_8B_tag PCTL111; /* offset: 0x012F size: 8 bit */
			ME_PCTL_8B_tag PCTL112; /* offset: 0x0130 size: 8 bit */
			ME_PCTL_8B_tag PCTL113; /* offset: 0x0131 size: 8 bit */
			ME_PCTL_8B_tag PCTL114; /* offset: 0x0132 size: 8 bit */
			ME_PCTL_8B_tag PCTL115; /* offset: 0x0133 size: 8 bit */
			ME_PCTL_8B_tag PCTL116; /* offset: 0x0134 size: 8 bit */
			ME_PCTL_8B_tag PCTL117; /* offset: 0x0135 size: 8 bit */
			ME_PCTL_8B_tag PCTL118; /* offset: 0x0136 size: 8 bit */
			ME_PCTL_8B_tag PCTL119; /* offset: 0x0137 size: 8 bit */
			ME_PCTL_8B_tag PCTL120; /* offset: 0x0138 size: 8 bit */
			ME_PCTL_8B_tag PCTL121; /* offset: 0x0139 size: 8 bit */
			ME_PCTL_8B_tag PCTL122; /* offset: 0x013A size: 8 bit */
			ME_PCTL_8B_tag PCTL123; /* offset: 0x013B size: 8 bit */
			ME_PCTL_8B_tag PCTL124; /* offset: 0x013C size: 8 bit */
			ME_PCTL_8B_tag PCTL125; /* offset: 0x013D size: 8 bit */
			ME_PCTL_8B_tag PCTL126; /* offset: 0x013E size: 8 bit */
			ME_PCTL_8B_tag PCTL127; /* offset: 0x013F size: 8 bit */
			ME_PCTL_8B_tag PCTL128; /* offset: 0x0140 size: 8 bit */
			ME_PCTL_8B_tag PCTL129; /* offset: 0x0141 size: 8 bit */
			ME_PCTL_8B_tag PCTL130; /* offset: 0x0142 size: 8 bit */
			ME_PCTL_8B_tag PCTL131; /* offset: 0x0143 size: 8 bit */
			ME_PCTL_8B_tag PCTL132; /* offset: 0x0144 size: 8 bit */
			ME_PCTL_8B_tag PCTL133; /* offset: 0x0145 size: 8 bit */
			ME_PCTL_8B_tag PCTL134; /* offset: 0x0146 size: 8 bit */
			ME_PCTL_8B_tag PCTL135; /* offset: 0x0147 size: 8 bit */
			ME_PCTL_8B_tag PCTL136; /* offset: 0x0148 size: 8 bit */
			ME_PCTL_8B_tag PCTL137; /* offset: 0x0149 size: 8 bit */
			ME_PCTL_8B_tag PCTL138; /* offset: 0x014A size: 8 bit */
			ME_PCTL_8B_tag PCTL139; /* offset: 0x014B size: 8 bit */
			ME_PCTL_8B_tag PCTL140; /* offset: 0x014C size: 8 bit */
			ME_PCTL_8B_tag PCTL141; /* offset: 0x014D size: 8 bit */
			ME_PCTL_8B_tag PCTL142; /* offset: 0x014E size: 8 bit */
			ME_PCTL_8B_tag PCTL143; /* offset: 0x014F size: 8 bit */
		};
	};
} ME_tag;

#define ME (*(volatile ME_tag*)0xC3FDC000UL)

/****************************************************************/
/*                                                              */
/* Module: OSC  */
/*                                                              */
/****************************************************************/

typedef union { /* OSC_CTL - Control Register */
	vuint32_t R;
	struct {
		vuint32_t OSCBYP : 1; /* High Frequency Oscillator Bypass */
		vuint32_t : 7;
		vuint32_t EOCV : 8;	 /* End of Count Value */
		vuint32_t M_OSC : 1; /* High Frequency Oscillator Clock Interrupt Mask */
		vuint32_t : 2;
		vuint32_t OSCDIV : 5; /* High Frequency Oscillator Division Factor */
		vuint32_t I_OSC : 1;	/* High Frequency Oscillator Clock Interrupt */
		vuint32_t : 5;
		vuint32_t S_OSC : 1;
		vuint32_t OSCON : 1;
	} B;
} OSC_CTL_32B_tag;

typedef struct OSC_struct_tag { /* start of OSC_tag */
																/* OSC_CTL - Control Register */
	OSC_CTL_32B_tag CTL;					/* offset: 0x0000 size: 32 bit */
} OSC_tag;

#define OSC (*(volatile OSC_tag*)0xC3FE0000UL)

/****************************************************************/
/*                                                              */
/* Module: RC  */
/*                                                              */
/****************************************************************/

typedef union { /* RC_CTL - Control Register */
	vuint32_t R;
	struct {
		vuint32_t : 10;
		vuint32_t RCTRIM : 6; /* Main RC Trimming Bits */
		vuint32_t : 3;
		vuint32_t RCDIV : 5; /* Main RC Clock Division Factor */
		vuint32_t : 2;
		vuint32_t S_RC_STDBY : 1; /* MRC Oscillator Powerdown Status */
		vuint32_t : 5;
	} B;
} RC_CTL_32B_tag;

typedef struct RC_struct_tag { /* start of RC_tag */
															 /* RC_CTL - Control Register */
	RC_CTL_32B_tag CTL;					 /* offset: 0x0000 size: 32 bit */
} RC_tag;

#define RC (*(volatile RC_tag*)0xC3FE0060UL)

/****************************************************************/
/*                                                              */
/* Module: PLLD  */
/*                                                              */
/****************************************************************/

typedef union { /* PLLD_CR - Control Register */
	vuint32_t R;
	struct {
		vuint32_t : 2;
		vuint32_t IDF : 4; /* PLL Input Division Factor */
		vuint32_t ODF : 2; /* PLL Output Division Factor */
		vuint32_t : 1;
		vuint32_t NDIV : 7; /* PLL Loop Division Factor */
		vuint32_t : 7;
		vuint32_t EN_PLL_SW : 1;		 /* Enable Progressive Clock Switching */
		vuint32_t MODE : 1;					 /* Activate 1:1 Mode */
		vuint32_t UNLOCK_ONCE : 1;	 /* PLL Loss of Lock */
		vuint32_t M_LOCK : 1;				 /* Mask for the i_lock Output Interrupt */
		vuint32_t I_LOCK : 1;				 /* PLL Lock Signal Toggle Indicator */
		vuint32_t S_LOCK : 1;				 /* PLL has Aquired Lock */
		vuint32_t PLL_FAIL_MASK : 1; /* PLL Fail Mask */
		vuint32_t PLL_FAIL_FLAG : 1; /* PLL Fail Flag */
		vuint32_t PLL_ON : 1;				 /* PLL ON Bit */
	} B;
} PLLD_CR_32B_tag;

typedef union { /* PLLD_MR - PLLD Modulation Register */
	vuint32_t R;
	struct {
		vuint32_t STRB_BYPASS : 1; /* Strobe Bypass */
		vuint32_t : 1;
		vuint32_t SPRD_SEL : 1;		 /* Spread Type Selection */
		vuint32_t MOD_PERIOD : 13; /* Modulation Period */
		vuint32_t SSCG_EN : 1;		 /* Spread Spectrum Clock Generation Enable */
		vuint32_t INC_STEP : 15;	 /* Increment Step */
	} B;
} PLLD_MR_32B_tag;

typedef struct PLLD_struct_tag { /* start of PLLD_tag */
																 /* PLLD_CR - Control Register */
	PLLD_CR_32B_tag CR;						 /* offset: 0x0000 size: 32 bit */
																 /* PLLD_MR - PLLD Modulation Register */
	PLLD_MR_32B_tag MR;						 /* offset: 0x0004 size: 32 bit */

	vuint32_t plld_reserved[6];
} PLLD_tag;

#define PLLD0 (*(volatile PLLD_tag*)0xC3FE00A0UL)
#define PLLD1 (*(volatile PLLD_tag*)0xC3FE00C0UL)

/****************************************************************/
/*                                                              */
/* Module: CMU  */
/*                                                              */
/****************************************************************/

typedef union { /* CMU_CSR - Control Status Register */
	vuint32_t R;
	struct {
		vuint32_t : 8;
		vuint32_t SFM : 1; /* Start Frequency Measure */
		vuint32_t : 13;
		vuint32_t CKSEL1 : 2; /* RC Oscillator(s) Selection Bit */
		vuint32_t : 5;
		vuint32_t RCDIV : 2; /* RCfast Clock Division Factor */
		vuint32_t CME_A : 1; /* PLL_A Clock Monitor Enable */
	} B;
} CMU_CSR_32B_tag;

typedef union { /* CMU_FDR - Frequency Display Register */
	vuint32_t R;
	struct {
		vuint32_t : 12;
		vuint32_t FD : 20; /* Measured Frequency Bits */
	} B;
} CMU_FDR_32B_tag;

typedef union { /* CMU_HFREFR_A - High Frequency Reference Register */
	vuint32_t R;
	struct {
		vuint32_t : 20;
		vuint32_t HFREF_A : 12; /* High Frequency Reference Value */
	} B;
} CMU_HFREFR_A_32B_tag;

typedef union { /* CMU_LFREFR_A - Low Frequency Reference Register */
	vuint32_t R;
	struct {
		vuint32_t : 20;
		vuint32_t LFREF_A : 12; /* Low Frequency Reference Value */
	} B;
} CMU_LFREFR_A_32B_tag;

typedef union { /* CMU_ISR - Interrupt Status Register */
	vuint32_t R;
	struct {
		vuint32_t : 28;
		vuint32_t FLCI_A : 1; /* PLL_A Clock Frequency less than Reference Clock
														 Interrupt */
		vuint32_t FHH_AI : 1; /* PLL_A Clock Frequency higher than high Reference
														 Interrupt */
		vuint32_t FLLI_A : 1; /* PLL_A Clock Frequency less than low Reference
														 Interrupt */
		vuint32_t
				OLRI : 1; /* Oscillator Frequency less than RC Frequency Interrupt */
	} B;
} CMU_ISR_32B_tag;

typedef union { /* CMU_IMR - Interrupt Mask Register */
	vuint32_t R;
} CMU_IMR_32B_tag;

typedef union { /* CMU_MDR - Measurement Duration Register */
	vuint32_t R;
	struct {
		vuint32_t : 12;
		vuint32_t MD : 20; /* Measurment Duration Bits */
	} B;
} CMU_MDR_32B_tag;

typedef struct CMU_struct_tag { /* start of CMU_tag */
																/* CMU_CSR - Control Status Register */
	CMU_CSR_32B_tag CSR;					/* offset: 0x0000 size: 32 bit */
																/* CMU_FDR - Frequency Display Register */
	CMU_FDR_32B_tag FDR;					/* offset: 0x0004 size: 32 bit */
	/* CMU_HFREFR_A - High Frequency Reference Register */
	CMU_HFREFR_A_32B_tag HFREFR_A; /* offset: 0x0008 size: 32 bit */
	/* CMU_LFREFR_A - Low Frequency Reference Register */
	CMU_LFREFR_A_32B_tag LFREFR_A; /* offset: 0x000C size: 32 bit */
																 /* CMU_ISR - Interrupt Status Register */
	CMU_ISR_32B_tag ISR;					 /* offset: 0x0010 size: 32 bit */
																 /* CMU_IMR - Interrupt Mask Register */
	CMU_IMR_32B_tag IMR;					 /* offset: 0x0014 size: 32 bit */
																 /* CMU_MDR - Measurement Duration Register */
	CMU_MDR_32B_tag MDR;					 /* offset: 0x0018 size: 32 bit */
} CMU_tag;

#define CMU0 (*(volatile CMU_tag*)0xC3FE0100UL)
#define CMU1 (*(volatile CMU_tag*)0xC3FE0120UL)
#define CMU2 (*(volatile CMU_tag*)0xC3FE0140UL)

/****************************************************************/
/*                                                              */
/* Module: CGM  */
/*                                                              */
/****************************************************************/

typedef union { /* Output Clock Enable Register */
	vuint32_t R;
	vuint8_t BYTE[4];	 /* individual bytes can be accessed */
	vuint16_t HALF[2]; /* individual halfwords can be accessed */
	vuint32_t WORD;		 /* individual words can be accessed */
	struct {
		vuint32_t : 31;
		vuint32_t EN : 1; /* Clock Enable Bit */
	} B;
} CGM_OC_EN_32B_tag;

typedef union { /* Output Clock Division Select Register */
	vuint32_t R;
	vuint8_t BYTE[4];	 /* individual bytes can be accessed */
	vuint16_t HALF[2]; /* individual halfwords can be accessed */
	vuint32_t WORD;		 /* individual words can be accessed */
	struct {
		vuint32_t : 2;
		vuint32_t SELDIV : 2; /* Output Clock Division Select */
		vuint32_t SELCTL : 4; /* Output Clock Source Selection Control */
		vuint32_t : 24;
	} B;
} CGM_OCDS_SC_32B_tag;

typedef union { /* System Clock Select Status Register */
	vuint32_t R;
	vuint8_t BYTE[4];	 /* individual bytes can be accessed */
	vuint16_t HALF[2]; /* individual halfwords can be accessed */
	vuint32_t WORD;		 /* individual words can be accessed */
	struct {
		vuint32_t : 4;
		vuint32_t SELSTAT : 4; /* System Clock Source Selection Status */
		vuint32_t : 24;
	} B;
} CGM_SC_SS_32B_tag;

typedef union { /* System Clock Divider Configuration Register */
	vuint32_t R;
	vuint8_t BYTE[4];	 /* individual bytes can be accessed */
	vuint16_t HALF[2]; /* individual halfwords can be accessed */
	vuint32_t WORD;		 /* individual words can be accessed */
	struct {
		vuint32_t DE0 : 1; /* Divider 0 Enable */
		vuint32_t : 3;
		vuint32_t DIV0 : 4; /* Divider 0 Value */
		vuint32_t : 24;
	} B;
} CGM_SC_DC0_3_32B_tag;

/* Register layout for all registers SC_DC... */

typedef union { /* System Clock Divider Configuration Register */
	vuint8_t R;
	struct {
		vuint8_t DE : 1; /* Divider Enable */
		vuint8_t : 3;
		vuint8_t DIV : 4; /* Divider Division Value */
	} B;
} CGM_SC_DC_8B_tag;

/* Register layout for all registers AC_SC... */

typedef union { /* Auxiliary Clock Select Control Registers */
	vuint32_t R;
	vuint8_t BYTE[4];	 /* individual bytes can be accessed */
	vuint16_t HALF[2]; /* individual halfwords can be accessed */
	vuint32_t WORD;		 /* individual words can be accessed */
	struct {
		vuint32_t : 4;
		vuint32_t SELCTL : 4; /* Auxliary Clock Source Selection Control */
		vuint32_t : 24;
	} B;
} CGM_AC_SC_32B_tag;

/* Register layout for all registers AC_DC0_3... */

typedef union { /* Auxiliary Clock Divider Configuration Registers */
	vuint32_t R;
	struct {
		vuint32_t DE0 : 1; /* Divider 0 Enable */
		vuint32_t : 3;
		vuint32_t DIV0 : 4; /* Divider 0 Value */
		vuint32_t DE1 : 1;	/* Divider 1 Enable */
		vuint32_t : 3;
		vuint32_t DIV1 : 4; /* Divider 1 Value */
		vuint32_t : 16;
	} B;
} CGM_AC_DC0_3_32B_tag;

typedef struct CGM_AUXCLK_struct_tag {
	/* Auxiliary Clock Select Control Registers */
	CGM_AC_SC_32B_tag AC_SC; /* relative offset: 0x0000 */
													 /* Auxiliary Clock Divider Configuration Registers */
	CGM_AC_DC0_3_32B_tag AC_DC0_3; /* relative offset: 0x0004 */

} CGM_AUXCLK_tag;

typedef struct CGM_struct_tag { /* start of CGM_tag */
	OSC_CTL_32B_tag OSC_CTL;			/* offset: 0x0000 size: 32 bit */
	int8_t CGM_reserved_0004[92];
	RC_CTL_32B_tag RC_CTL; /* offset: 0x0060 size: 32 bit */
	int8_t CGM_reserved_0064[60];
	PLLD_tag FMPLL[2]; /* offset: 0x00A0  (0x0020 x 2) */
	int8_t CGM_reserved_00E0[32];
	CMU_CSR_32B_tag CMU_0_CSR;					 /* offset: 0x0100 size: 32 bit */
	CMU_FDR_32B_tag CMU_0_FDR;					 /* offset: 0x0104 size: 32 bit */
	CMU_HFREFR_A_32B_tag CMU_0_HFREFR_A; /* offset: 0x0108 size: 32 bit */
	CMU_LFREFR_A_32B_tag CMU_0_LFREFR_A; /* offset: 0x010C size: 32 bit */
	CMU_ISR_32B_tag CMU_0_ISR;					 /* offset: 0x0110 size: 32 bit */
	CMU_IMR_32B_tag CMU_0_IMR;					 /* offset: 0x0114 size: 32 bit */
	CMU_MDR_32B_tag CMU_0_MDR;					 /* offset: 0x0118 size: 32 bit */
	int8_t CGM_reserved_011C[4];
	CMU_CSR_32B_tag CMU_1_CSR; /* offset: 0x0120 size: 32 bit */
	int8_t CGM_reserved_0124[4];
	CMU_HFREFR_A_32B_tag CMU_1_HFREFR_A; /* offset: 0x0128 size: 32 bit */
	CMU_LFREFR_A_32B_tag CMU_1_LFREFR_A; /* offset: 0x012C size: 32 bit */
	CMU_ISR_32B_tag CMU_1_ISR;					 /* offset: 0x0130 size: 32 bit */
	int8_t CGM_reserved_0134[572];
	/* Output Clock Enable Register */
	CGM_OC_EN_32B_tag OC_EN;		 /* offset: 0x0370 size: 32 bit */
															 /* Output Clock Division Select Register */
	CGM_OCDS_SC_32B_tag OCDS_SC; /* offset: 0x0374 size: 32 bit */
															 /* System Clock Select Status Register */
	CGM_SC_SS_32B_tag SC_SS;		 /* offset: 0x0378 size: 32 bit */
	union {
		struct {
			/* System Clock Divider Configuration Register */
			CGM_SC_DC_8B_tag SC_DC[2]; /* offset: 0x037C  (0x0001 x 2) */
			int8_t CGM_reserved_037E_E0[2];
		};

		struct {
			/* System Clock Divider Configuration Register */
			CGM_SC_DC_8B_tag SC_DC0; /* offset: 0x037C size: 8 bit */
			CGM_SC_DC_8B_tag SC_DC1; /* offset: 0x037D size: 8 bit */
			int8_t CGM_reserved_037E_E1[2];
		};

		/* System Clock Divider Configuration Register */
		CGM_SC_DC0_3_32B_tag SC_DC0_3; /* offset: 0x037C size: 32 bit */
	};
	union {
		/*  Register set AUXCLK */
		CGM_AUXCLK_tag AUXCLK[6]; /* offset: 0x0380  (0x0008 x 6) */

		struct {
			/* Auxiliary Clock Select Control Registers */
			CGM_AC_SC_32B_tag AC0_SC; /* offset: 0x0380 size: 32 bit */
			/* Auxiliary Clock Divider Configuration Registers */
			CGM_AC_DC0_3_32B_tag AC0_DC0_3; /* offset: 0x0384 size: 32 bit */
			/* Auxiliary Clock Select Control Registers */
			CGM_AC_SC_32B_tag AC1_SC; /* offset: 0x0388 size: 32 bit */
			/* Auxiliary Clock Divider Configuration Registers */
			CGM_AC_DC0_3_32B_tag AC1_DC0_3; /* offset: 0x038C size: 32 bit */
			/* Auxiliary Clock Select Control Registers */
			CGM_AC_SC_32B_tag AC2_SC; /* offset: 0x0390 size: 32 bit */
			/* Auxiliary Clock Divider Configuration Registers */
			CGM_AC_DC0_3_32B_tag AC2_DC0_3; /* offset: 0x0394 size: 32 bit */
			/* Auxiliary Clock Select Control Registers */
			CGM_AC_SC_32B_tag AC3_SC; /* offset: 0x0398 size: 32 bit */
			/* Auxiliary Clock Divider Configuration Registers */
			CGM_AC_DC0_3_32B_tag AC3_DC0_3; /* offset: 0x039C size: 32 bit */
			/* Auxiliary Clock Select Control Registers */
			CGM_AC_SC_32B_tag AC4_SC; /* offset: 0x03A0 size: 32 bit */
			/* Auxiliary Clock Divider Configuration Registers */
			CGM_AC_DC0_3_32B_tag AC4_DC0_3; /* offset: 0x03A4 size: 32 bit */
			/* Auxiliary Clock Select Control Registers */
			CGM_AC_SC_32B_tag AC5_SC; /* offset: 0x03A8 size: 32 bit */
			/* Auxiliary Clock Divider Configuration Registers */
			CGM_AC_DC0_3_32B_tag AC5_DC0_3; /* offset: 0x03AC size: 32 bit */
		};
	};
} CGM_tag;

#define CGM (*(volatile CGM_tag*)0xC3FE0000UL)

/****************************************************************/
/*                                                              */
/* Module: RGM  */
/*                                                              */
/****************************************************************/

typedef union { /* Functional Event Status Register */
	vuint16_t R;
	struct {
		vuint16_t F_EXR : 1;			 /* Flag for External Reset */
		vuint16_t F_FCCU_HARD : 1; /* Flag for FCCU hard reaction request */
		vuint16_t F_FCCU_SOFT : 1; /* Flag for FCCU soft reaction request */
		vuint16_t F_ST_DONE : 1;	 /* Flag for self-test completed */
#ifndef USE_FIELD_ALIASES_RGM
		vuint16_t F_CMU12_FHL : 1; /* Flag for CMU 1/2 clock freq. too high/low */
#else
		vuint16_t F_CMU1_FHL : 1;		 /* deprecated name - please avoid */
#endif
		vuint16_t F_FL_ECC_RCC : 1; /* Flag for Flash, ECC, or lock-step error */
		vuint16_t F_PLL1 : 1;				/* Flag for PLL1 fail */
		vuint16_t F_SWT : 1;				/* Flag for Software Watchdog Timer */
		vuint16_t F_FCCU_SAFE : 1;	/* Flag for FCCU SAFE mode request */
		vuint16_t F_CMU0_FHL : 1;		/* Flag for CMU 0 clock freq. too high/low */
		vuint16_t F_CMU0_OLR : 1;		/* Flag for oscillator freq. too low */
		vuint16_t F_PLL0 : 1;				/* Flag for PLL0 fail */
		vuint16_t F_CWD : 1;				/* Flag for Core Watchdog Reset */
		vuint16_t F_SOFT : 1;				/* Flag for software reset */
		vuint16_t F_CORE : 1;				/* Flag for core reset */
		vuint16_t F_JTAG : 1;				/* Flag for JTAG initiated reset */
	} B;
} RGM_FES_16B_tag;

typedef union { /* Destructive Event Status Register */
	vuint16_t R;
	struct {
#ifndef USE_FIELD_ALIASES_RGM
		vuint16_t F_POR : 1; /* Flag for Power on Reset */
#else
		vuint16_t POR : 1;					 /* deprecated name - please avoid */
#endif
		vuint16_t : 7;
		vuint16_t F_COMP : 1;			/* Flag for comparator error */
		vuint16_t F_LVD27_IO : 1; /* Flag for 2.7V low-voltage detected (I/O) */
		vuint16_t
				F_LVD27_FLASH : 1;			/* Flag for 2.7V low-voltage detected (Flash) */
		vuint16_t F_LVD27_VREG : 1; /* Flag for 2.7V low-voltage detected (VREG) */
		vuint16_t : 2;
		vuint16_t F_HVD12 : 1; /* Flag for 1.2V high-voltage detected */
#ifndef USE_FIELD_ALIASES_RGM
		vuint16_t F_LVD12 : 1; /* Flag for 1.2V low-voltage detected */
#else
		vuint16_t F_LVD12_PD0 : 1;	 /* deprecated name - please avoid */
#endif
	} B;
} RGM_DES_16B_tag;

typedef union { /* Functional Event Reset Disable Register */
	vuint16_t R;
	struct {
		vuint16_t D_EXR : 1;			 /* Disable External Pad Event Reset */
		vuint16_t D_FCCU_HARD : 1; /* Disable FCCU hard reaction request */
		vuint16_t D_FCCU_SOFT : 1; /* Disable FCCU soft reaction request */
		vuint16_t D_ST_DONE : 1;	 /* Disable self-test completed */
#ifndef USE_FIELD_ALIASES_RGM
		vuint16_t D_CMU12_FHL : 1; /* Disable CMU 1/2 clock freq. too high/low */
#else
		vuint16_t D_CMU1_FHL : 1;		 /* deprecated name - please avoid */
#endif
		vuint16_t D_FL_ECC_RCC : 1; /* Disable Flash, ECC, or lock-step error */
		vuint16_t D_PLL1 : 1;				/* Disable PLL1 fail */
		vuint16_t D_SWT : 1;				/* Disable Software Watchdog Timer */
		vuint16_t D_FCCU_SAFE : 1;	/* Disable FCCU SAFE mode request */
		vuint16_t D_CMU0_FHL : 1;		/* Disable CMU 0 clock freq. too high/low */
		vuint16_t D_CMU0_OLR : 1;		/* Disable oscillator freq. too low */
		vuint16_t D_PLL0 : 1;				/* Disable PLL0 fail */
		vuint16_t D_CWD : 1;				/* Disable Core Watchdog Reset */
		vuint16_t D_SOFT : 1;				/* Disable software reset */
		vuint16_t D_CORE : 1;				/* Disable core reset */
		vuint16_t D_JTAG : 1;				/* Disable JTAG initiated reset */
	} B;
} RGM_FERD_16B_tag;

typedef union { /* Destructive Event Reset Disable Register */
	vuint16_t R;
	struct {
		vuint16_t : 8;
		vuint16_t D_COMP : 1;				 /* Disable comparator error */
		vuint16_t D_LVD27_IO : 1;		 /* Disable 2.7V low-voltage detected (I/O) */
		vuint16_t D_LVD27_FLASH : 1; /* Disable 2.7V low-voltage detected (Flash) */
		vuint16_t D_LVD27_VREG : 1;	 /* Disable 2.7V low-voltage detected (VREG) */
		vuint16_t : 2;
		vuint16_t D_HVD12 : 1; /* Disable 1.2V high-voltage detected */
#ifndef USE_FIELD_ALIASES_RGM
		vuint16_t D_LVD12 : 1; /* Disable 1.2V low-voltage detected */
#else
		vuint16_t D_LVD12_PD0 : 1;	 /* deprecated name - please avoid */
#endif
	} B;
} RGM_DERD_16B_tag;

typedef union { /* Functional Event Alternate Request Register */
	vuint16_t R;
	struct {
		vuint16_t : 4;
#ifndef USE_FIELD_ALIASES_RGM
		vuint16_t AR_CMU12_FHL : 1; /* Alternate Request for CMU1/2 clock freq. too
																	 high/low */
#else
		vuint16_t AR_CMU1_FHL : 1;	 /* deprecated name - please avoid */
#endif
		vuint16_t : 1;
		vuint16_t AR_PLL1 : 1; /* Alternate Request for PLL1 fail */
		vuint16_t : 1;
		vuint16_t
				AR_FCCU_SAVE : 1; /* Alternate Request for FCCU SAFE mode request */
		vuint16_t AR_CMU0_FHL : 1; /* Alternate Request for CMU0 clock freq.
too high/low */
		vuint16_t
				AR_CMU0_OLR : 1;	 /* Alternate Request for oscillator freq. too low */
		vuint16_t AR_PLL0 : 1; /* Alternate Request for PLL0 fail */
		vuint16_t AR_CWD : 1;	 /* Alternate Request for core watchdog reset */
		vuint16_t : 3;
	} B;
} RGM_FEAR_16B_tag;

typedef union { /* Functional Event Short Sequence Register */
	vuint16_t R;
	struct {
		vuint16_t SS_EXR : 1; /* Short Sequence for External Reset */
		vuint16_t
				SS_FCCU_HARD : 1; /* Short Sequence for FCCU hard reaction request */
		vuint16_t
				SS_FCCU_SOFT : 1; /* Short Sequence for FCCU soft reaction request */
		vuint16_t SS_ST_DONE : 1; /* Short Sequence for self-test completed */
#ifndef USE_FIELD_ALIASES_RGM
		vuint16_t SS_CMU12_FHL : 1; /* Short Sequence for CMU 1/2 clock freq. too
																	 high/low */
#else
		vuint16_t SS_CMU1_FHL : 1;	 /* deprecated name - please avoid */
#endif
		vuint16_t SS_FL_ECC_RCC : 1; /* Short Sequence for Flash, ECC, or lock-step
																		error */
		vuint16_t SS_PLL1 : 1;			 /* Short Sequence for PLL1 fail */
		vuint16_t SS_SWT : 1; /* Short Sequence for Software Watchdog Timer */
		vuint16_t : 1;
		vuint16_t
				SS_CMU0_FHL : 1; /* Short Sequence for CMU 0 clock freq. too high/low */
		vuint16_t SS_CMU0_OLR : 1; /* Short Sequence for oscillator freq. too low */
		vuint16_t SS_PLL0 : 1;		 /* Short Sequence for PLL0 fail */
		vuint16_t SS_CWD : 1;			 /* Short Sequence for Core Watchdog Reset */
		vuint16_t SS_SOFT : 1;		 /* Short Sequence for software reset */
		vuint16_t SS_CORE : 1;		 /* Short Sequence for core reset */
		vuint16_t SS_JTAG : 1;		 /* Short Sequence for JTAG initiated reset */
	} B;
} RGM_FESS_16B_tag;

typedef union { /* Functional Bidirectional Reset Enable Register */
	vuint16_t R;
	struct {
		vuint16_t BE_EXR : 1; /* Bidirectional Reset Enable for External Reset */
		vuint16_t BE_FCCU_HARD : 1; /* Bidirectional Reset Enable for FCCU hard
																	 reaction request */
		vuint16_t BE_FCCU_SOFT : 1; /* Bidirectional Reset Enable for FCCU soft
																	 reaction request */
		vuint16_t
				BE_ST_DONE : 1; /* Bidirectional Reset Enable for self-test completed */
#ifndef USE_FIELD_ALIASES_RGM
		vuint16_t BE_CMU12_FHL : 1; /* Bidirectional Reset Enable for CMU 1/2 clock
																	 freq. too high/low */
#else
		vuint16_t BE_CMU1_FHL : 1;	 /* deprecated name - please avoid */
#endif
		vuint16_t BE_FL_ECC_RCC : 1; /* Bidirectional Reset Enable for Flash, ECC,
																		or lock-step error */
		vuint16_t BE_PLL1 : 1;			 /* Bidirectional Reset Enable for PLL1 fail */
		vuint16_t
				BE_SWT : 1; /* Bidirectional Reset Enable for Software Watchdog Timer */
		vuint16_t : 1;
		vuint16_t BE_CMU0_FHL : 1; /* Bidirectional Reset Enable for CMU 0 clock
																	freq. too high/low */
		vuint16_t BE_CMU0_OLR : 1; /* Bidirectional Reset Enable for oscillator
																	freq. too low */
		vuint16_t BE_PLL0 : 1;		 /* Bidirectional Reset Enable for PLL0 fail */
		vuint16_t
				BE_CWD : 1; /* Bidirectional Reset Enable for Core Watchdog Reset */
		vuint16_t BE_SOFT : 1; /* Bidirectional Reset Enable for software reset */
		vuint16_t BE_CORE : 1; /* Bidirectional Reset Enable for core reset */
		vuint16_t
				BE_JTAG : 1; /* Bidirectional Reset Enable for JTAG initiated reset */
	} B;
} RGM_FBRE_16B_tag;

typedef struct RGM_struct_tag { /* start of RGM_tag */
																/* Functional Event Status Register */
	RGM_FES_16B_tag FES;					/* offset: 0x0000 size: 16 bit */
																/* Destructive Event Status Register */
	RGM_DES_16B_tag DES;					/* offset: 0x0002 size: 16 bit */
																/* Functional Event Reset Disable Register */
	RGM_FERD_16B_tag FERD;				/* offset: 0x0004 size: 16 bit */
																/* Destructive Event Reset Disable Register */
	RGM_DERD_16B_tag DERD;				/* offset: 0x0006 size: 16 bit */
	int8_t RGM_reserved_0008[8];
	/* Functional Event Alternate Request Register */
	RGM_FEAR_16B_tag FEAR; /* offset: 0x0010 size: 16 bit */
	int8_t RGM_reserved_0012[6];
	/* Functional Event Short Sequence Register */
	RGM_FESS_16B_tag FESS; /* offset: 0x0018 size: 16 bit */
	int8_t RGM_reserved_001A[2];
	/* Functional Bidirectional Reset Enable Register */
	RGM_FBRE_16B_tag FBRE; /* offset: 0x001C size: 16 bit */
} RGM_tag;

#define RGM (*(volatile RGM_tag*)0xC3FE4000UL)

/****************************************************************/
/*                                                              */
/* Module: PCU  */
/*                                                              */
/****************************************************************/

/* Register layout for all registers PCONF... */

typedef union { /* PCU_PCONF[0..15] -  Power Domain #0..#15 Configuration
									 Register */
	vuint32_t R;
	struct {
		vuint32_t : 18;
		vuint32_t STBY0 : 1; /* Power domain control during STBY0 */
		vuint32_t : 2;
		vuint32_t STOP0 : 1; /* Power domain control during STOP0 */
		vuint32_t : 1;
		vuint32_t HALT0 : 1; /* Power domain control during HALT0 */
		vuint32_t RUN3 : 1;	 /* Power domain control during RUN3 */
		vuint32_t RUN2 : 1;	 /* Power domain control during RUN2 */
		vuint32_t RUN1 : 1;	 /* Power domain control during RUN1 */
		vuint32_t RUN0 : 1;	 /* Power domain control during RUN0 */
		vuint32_t DRUN : 1;	 /* Power domain control during DRUN */
		vuint32_t SAFE : 1;	 /* Power domain control during SAFE */
		vuint32_t TEST : 1;	 /* Power domain control during TEST */
		vuint32_t RST : 1;	 /* Power domain control during RST */
	} B;
} PCU_PCONF_32B_tag;

typedef union { /* PCU_PSTAT - Power Domain Status Register */
	vuint32_t R;
	struct {
		vuint32_t : 16;
		vuint32_t PD15 : 1; /* Power Status for Power Domain 15 */
		vuint32_t PD14 : 1; /* Power Status for Power Domain 14 */
		vuint32_t PD13 : 1; /* Power Status for Power Domain 13 */
		vuint32_t PD12 : 1; /* Power Status for Power Domain 12 */
		vuint32_t PD11 : 1; /* Power Status for Power Domain 11 */
		vuint32_t PD10 : 1; /* Power Status for Power Domain 10 */
		vuint32_t PD9 : 1;	/* Power Status for Power Domain 9 */
		vuint32_t PD8 : 1;	/* Power Status for Power Domain 8 */
		vuint32_t PD7 : 1;	/* Power Status for Power Domain 7 */
		vuint32_t PD6 : 1;	/* Power Status for Power Domain 6 */
		vuint32_t PD5 : 1;	/* Power Status for Power Domain 5 */
		vuint32_t PD4 : 1;	/* Power Status for Power Domain 4 */
		vuint32_t PD3 : 1;	/* Power Status for Power Domain 3 */
		vuint32_t PD2 : 1;	/* Power Status for Power Domain 2 */
		vuint32_t PD1 : 1;	/* Power Status for Power Domain 1 */
		vuint32_t PD0 : 1;	/* Power Status for Power Domain 0 */
	} B;
} PCU_PSTAT_32B_tag;

typedef struct PCU_struct_tag { /* start of PCU_tag */
	union {
		/* PCU_PCONF[0..15] -  Power Domain #0..#15 Configuration Register */
		PCU_PCONF_32B_tag PCONF[16]; /* offset: 0x0000  (0x0004 x 16) */

		struct {
			/* PCU_PCONF[0..15] -  Power Domain #0..#15 Configuration Register */
			PCU_PCONF_32B_tag PCONF0;	 /* offset: 0x0000 size: 32 bit */
			PCU_PCONF_32B_tag PCONF1;	 /* offset: 0x0004 size: 32 bit */
			PCU_PCONF_32B_tag PCONF2;	 /* offset: 0x0008 size: 32 bit */
			PCU_PCONF_32B_tag PCONF3;	 /* offset: 0x000C size: 32 bit */
			PCU_PCONF_32B_tag PCONF4;	 /* offset: 0x0010 size: 32 bit */
			PCU_PCONF_32B_tag PCONF5;	 /* offset: 0x0014 size: 32 bit */
			PCU_PCONF_32B_tag PCONF6;	 /* offset: 0x0018 size: 32 bit */
			PCU_PCONF_32B_tag PCONF7;	 /* offset: 0x001C size: 32 bit */
			PCU_PCONF_32B_tag PCONF8;	 /* offset: 0x0020 size: 32 bit */
			PCU_PCONF_32B_tag PCONF9;	 /* offset: 0x0024 size: 32 bit */
			PCU_PCONF_32B_tag PCONF10; /* offset: 0x0028 size: 32 bit */
			PCU_PCONF_32B_tag PCONF11; /* offset: 0x002C size: 32 bit */
			PCU_PCONF_32B_tag PCONF12; /* offset: 0x0030 size: 32 bit */
			PCU_PCONF_32B_tag PCONF13; /* offset: 0x0034 size: 32 bit */
			PCU_PCONF_32B_tag PCONF14; /* offset: 0x0038 size: 32 bit */
			PCU_PCONF_32B_tag PCONF15; /* offset: 0x003C size: 32 bit */
		};
	};
	/* PCU_PSTAT - Power Domain Status Register */
	PCU_PSTAT_32B_tag PSTAT; /* offset: 0x0040 size: 32 bit */
} PCU_tag;

#define PCU (*(volatile PCU_tag*)0xC3FE8000UL)

/****************************************************************/
/*                                                              */
/* Module: PMUCTRL  */
/*                                                              */
/****************************************************************/

typedef union { /* PMUCTRL_STATHVD - PMU Status Register HVD */
	vuint32_t R;
	struct {
		vuint32_t : 11;
		vuint32_t HVDT_LPB : 5; /* High Voltage Detector trimming bits LPB bus */
		vuint32_t : 6;
		vuint32_t HVD_M : 1; /* High Voltage Detector Main */
		vuint32_t HVD_B : 1; /* High Voltage Detector Backup */
		vuint32_t : 4;
		vuint32_t HVD_LP : 4; /* High Voltage Detector trimming bits LP bus */
	} B;
} PMUCTRL_STATHVD_32B_tag;

typedef union { /* PMUCTRL_STATLVD - PMU Status Register LVD */
	vuint32_t R;
	struct {
		vuint32_t : 11;
		vuint32_t LVDT_LPB : 5; /* Ligh Voltage Detector trimming bits LPB bus */
		vuint32_t : 6;
		vuint32_t LVD_M : 1; /* Ligh Voltage Detector Main */
		vuint32_t LVD_B : 1; /* Ligh Voltage Detector Backup */
		vuint32_t : 4;
		vuint32_t LVD_LP : 4; /* Ligh Voltage Detector trimming bits LP bus */
	} B;
} PMUCTRL_STATLVD_32B_tag;

typedef union { /* PMUCTRL_STATIREG - PMU Status Register IREG */
	vuint32_t R;
	struct {
		vuint32_t : 28;
		vuint32_t
				IIREG_HP : 4; /* Internal ballast REGulator hpreg1 trimming bits */
	} B;
} PMUCTRL_STATIREG_32B_tag;

typedef union { /* PMUCTRL_STATEREG - PMU Status Register EREG */
	vuint32_t R;
	struct {
		vuint32_t : 28;
		vuint32_t
				EEREG_HP : 4; /* Internal ballast REGulator hpreg1 trimming bits */
	} B;
} PMUCTRL_STATEREG_32B_tag;

typedef union { /* PMUCTRL_STATUS - PMU Status Register STATUS */
	vuint32_t R;
	struct {
		vuint32_t EBMM : 1; /* External Ballast Management Mode */
		vuint32_t AEBD : 1; /* Automatic External Ballast Detection */
		vuint32_t ENPN : 1; /* External NPN status flag */
		vuint32_t : 13;
		vuint32_t CTB : 2; /* Configuration Trace Bits */
		vuint32_t : 6;
		vuint32_t CBS : 4;	/* Current BIST Status */
		vuint32_t CPCS : 4; /* Current Pmu Configuration Status */
	} B;
} PMUCTRL_STATUS_32B_tag;

typedef union { /* PMUCTRL_CTRL - PMU Control Register */
	vuint32_t R;
	struct {
		vuint32_t : 30;
		vuint32_t SILHT : 2; /* Start Idle or LVD or HVD BIST Test */
	} B;
} PMUCTRL_CTRL_32B_tag;

typedef union { /* PMUCTRL_MASKF - PMU Mask Fault Register */
	vuint32_t R;
	struct {
		vuint32_t MF_BB : 4; /* Mask Fault Bypass Balast */
		vuint32_t : 28;
	} B;
} PMUCTRL_MASKF_32B_tag;

typedef union { /* PMUCTRL_FAULT - PMU Fault Monitor Register */
	vuint32_t R;
	struct {
		vuint32_t BB_LV : 4; /* Bypass Ballast Low Voltage */
		vuint32_t : 9;
		vuint32_t FLNCF : 1; /* FLash voltage monitor Non Critical Fault */
		vuint32_t IONCF : 1; /* IO voltage monitor Non Critical Fault */
		vuint32_t RENCF : 1; /* REgulator voltage monitor Non Critical Fault */
		vuint32_t : 13;
		vuint32_t LHCF : 1; /* Low High voltage detector Critical Fault */
		vuint32_t LNCF : 1; /* Low  voltage detector Non Critical Fault */
		vuint32_t HNCF : 1; /* High voltage detector Non Critical Fault */
	} B;
} PMUCTRL_FAULT_32B_tag;

typedef union { /* PMUCTRL_IRQS - PMU Interrupt Request Status Register */
	vuint32_t R;
	struct {
		vuint32_t : 10;
		vuint32_t
				MFVMP : 1; /* Main   Flash     Voltage Monitor interrupt Pending */
		vuint32_t
				BFVMP : 1; /* Backup Flash     Voltage Monitor interrupt Pending */
		vuint32_t
				MIVMP : 1; /* MAin   IO        Voltage Monitor interrupt Pending */
		vuint32_t
				BIVMP : 1; /* Backup IO        Voltage Monitor interrupt Pending */
		vuint32_t
				MRVMP : 1; /* Main   Regulator Voltage Monitor interrupt Pending */
		vuint32_t
				BRVMP : 1; /* Backup Regulator Voltage Monitor interrupt Pending */
		vuint32_t : 12;
		vuint32_t
				MLVDP : 1; /* Main   Low  Voltage Detector error interrupt Pending */
		vuint32_t
				BLVDP : 1; /* Backup Low  Voltage Detector error interrupt Pending */
		vuint32_t
				MHVDP : 1; /* Main   High Voltage Detector error interrupt Pending */
		vuint32_t
				BHVDP : 1; /* Backup High Voltage Detector error interrupt Pending */
	} B;
} PMUCTRL_IRQS_32B_tag;

typedef union { /* PMUCTRL_IRQE - PMU Interrupt Request Enable Register */
	vuint32_t R;
	struct {
		vuint32_t : 10;
		vuint32_t MFVME : 1; /* Main   Flash     Voltage Monitor interrupt Enable */
		vuint32_t BFVME : 1; /* Backup Flash     Voltage Monitor interrupt Enable */
		vuint32_t MIVME : 1; /* MAin   IO        Voltage Monitor interrupt Enable */
		vuint32_t BIVME : 1; /* Backup IO        Voltage Monitor interrupt Enable */
		vuint32_t MRVME : 1; /* Main   Regulator Voltage Monitor interrupt Enable */
		vuint32_t BRVME : 1; /* Backup Regulator Voltage Monitor interrupt Enable */
		vuint32_t : 12;
		vuint32_t
				MLVDE : 1; /* Main   Low  Voltage Detector error interrupt Enable */
		vuint32_t
				BLVDE : 1; /* Backup Low  Voltage Detector error interrupt Enable */
		vuint32_t
				MHVDE : 1; /* Main   High Voltage Detector error interrupt Enable */
		vuint32_t
				BHVDE : 1; /* Backup High Voltage Detector error interrupt Enable */
	} B;
} PMUCTRL_IRQE_32B_tag;

typedef struct PMUCTRL_struct_tag { /* start of PMUCTRL_tag */
	int8_t PMUCTRL_reserved_0000[4];
	/* PMUCTRL_STATHVD - PMU Status Register HVD */
	PMUCTRL_STATHVD_32B_tag STATHVD; /* offset: 0x0004 size: 32 bit */
	/* PMUCTRL_STATLVD - PMU Status Register LVD */
	PMUCTRL_STATLVD_32B_tag STATLVD; /* offset: 0x0008 size: 32 bit */
	int8_t PMUCTRL_reserved_000C[20];
	/* PMUCTRL_STATIREG - PMU Status Register IREG */
	PMUCTRL_STATIREG_32B_tag STATIREG; /* offset: 0x0020 size: 32 bit */
	/* PMUCTRL_STATEREG - PMU Status Register EREG */
	PMUCTRL_STATEREG_32B_tag STATEREG; /* offset: 0x0024 size: 32 bit */
	int8_t PMUCTRL_reserved_0028[24];
	/* PMUCTRL_STATUS - PMU Status Register STATUS */
	PMUCTRL_STATUS_32B_tag STATUS; /* offset: 0x0040 size: 32 bit */
																 /* PMUCTRL_CTRL - PMU Control Register */
	PMUCTRL_CTRL_32B_tag CTRL;		 /* offset: 0x0044 size: 32 bit */
	int8_t PMUCTRL_reserved_0048[40];
	/* PMUCTRL_MASKF - PMU Mask Fault Register */
	PMUCTRL_MASKF_32B_tag MASKF; /* offset: 0x0070 size: 32 bit */
															 /* PMUCTRL_FAULT - PMU Fault Monitor Register */
	PMUCTRL_FAULT_32B_tag FAULT; /* offset: 0x0074 size: 32 bit */
	/* PMUCTRL_IRQS - PMU Interrupt Request Status Register */
	PMUCTRL_IRQS_32B_tag IRQS; /* offset: 0x0078 size: 32 bit */
	/* PMUCTRL_IRQE - PMU Interrupt Request Enable Register */
	PMUCTRL_IRQE_32B_tag IRQE; /* offset: 0x007C size: 32 bit */
} PMUCTRL_tag;

#define PMUCTRL (*(volatile PMUCTRL_tag*)0xC3FE8080UL)

/****************************************************************/
/*                                                              */
/* Module: PIT_RTI  */
/*                                                              */
/****************************************************************/

typedef union { /* PIT_RTI_PITMCR - PIT Module Control Register */
	vuint32_t R;
	struct {
		vuint32_t : 30;
		vuint32_t MDIS : 1; /* Module Disable. Disable the module clock */
		vuint32_t FRZ : 1; /* Freeze. Allows the timers to be stoppedwhen the device
													enters debug mode */
	} B;
} PIT_RTI_PITMCR_32B_tag;

/* Register layout for all registers LDVAL... */

typedef union { /* PIT_RTI_LDVAL - Timer Load Value Register */
	vuint32_t R;
	struct {
		vuint32_t TSV : 32; /* Time Start Value Bits */
	} B;
} PIT_RTI_LDVAL_32B_tag;

/* Register layout for all registers CVAL... */

typedef union { /* PIT_RTI_CVAL - Current Timer Value Register */
	vuint32_t R;
	struct {
		vuint32_t TVL : 32; /* Current Timer Value Bits */
	} B;
} PIT_RTI_CVAL_32B_tag;

/* Register layout for all registers TCTRL... */

typedef union { /* PIT_RTI_TCTRL - Timer Control Register */
	vuint32_t R;
	struct {
		vuint32_t : 30;
		vuint32_t TIE : 1; /* Timer Interrupt Enable Bit */
		vuint32_t TEN : 1; /* Timer Enable Bit */
	} B;
} PIT_RTI_TCTRL_32B_tag;

/* Register layout for all registers TFLG... */

typedef union { /* PIT_RTI_TFLG - Timer Flag Register */
	vuint32_t R;
	struct {
		vuint32_t : 31;
		vuint32_t TIF : 1; /* Timer Interrupt Flag Bit */
	} B;
} PIT_RTI_TFLG_32B_tag;

typedef struct PIT_RTI_CHANNEL_struct_tag {
	/* PIT_RTI_LDVAL - Timer Load Value Register */
	PIT_RTI_LDVAL_32B_tag LDVAL; /* relative offset: 0x0000 */
															 /* PIT_RTI_CVAL - Current Timer Value Register */
	PIT_RTI_CVAL_32B_tag CVAL;	 /* relative offset: 0x0004 */
															 /* PIT_RTI_TCTRL - Timer Control Register */
	PIT_RTI_TCTRL_32B_tag TCTRL; /* relative offset: 0x0008 */
															 /* PIT_RTI_TFLG - Timer Flag Register */
	PIT_RTI_TFLG_32B_tag TFLG;	 /* relative offset: 0x000C */

} PIT_RTI_CHANNEL_tag;

typedef struct PIT_RTI_struct_tag { /* start of PIT_RTI_tag */
	/* PIT_RTI_PITMCR - PIT Module Control Register */
	PIT_RTI_PITMCR_32B_tag PITMCR; /* offset: 0x0000 size: 32 bit */
	int8_t PIT_RTI_reserved_0004_C[252];
	union {
		/*  Register set CHANNEL */
		PIT_RTI_CHANNEL_tag CHANNEL[4]; /* offset: 0x0100  (0x0010 x 4) */

		struct {
			/* PIT_RTI_LDVAL - Timer Load Value Register */
			PIT_RTI_LDVAL_32B_tag LDVAL0; /* offset: 0x0100 size: 32 bit */
			/* PIT_RTI_CVAL - Current Timer Value Register */
			PIT_RTI_CVAL_32B_tag CVAL0;		/* offset: 0x0104 size: 32 bit */
																		/* PIT_RTI_TCTRL - Timer Control Register */
			PIT_RTI_TCTRL_32B_tag TCTRL0; /* offset: 0x0108 size: 32 bit */
																		/* PIT_RTI_TFLG - Timer Flag Register */
			PIT_RTI_TFLG_32B_tag TFLG0;		/* offset: 0x010C size: 32 bit */
			/* PIT_RTI_LDVAL - Timer Load Value Register */
			PIT_RTI_LDVAL_32B_tag LDVAL1; /* offset: 0x0110 size: 32 bit */
			/* PIT_RTI_CVAL - Current Timer Value Register */
			PIT_RTI_CVAL_32B_tag CVAL1;		/* offset: 0x0114 size: 32 bit */
																		/* PIT_RTI_TCTRL - Timer Control Register */
			PIT_RTI_TCTRL_32B_tag TCTRL1; /* offset: 0x0118 size: 32 bit */
																		/* PIT_RTI_TFLG - Timer Flag Register */
			PIT_RTI_TFLG_32B_tag TFLG1;		/* offset: 0x011C size: 32 bit */
			/* PIT_RTI_LDVAL - Timer Load Value Register */
			PIT_RTI_LDVAL_32B_tag LDVAL2; /* offset: 0x0120 size: 32 bit */
			/* PIT_RTI_CVAL - Current Timer Value Register */
			PIT_RTI_CVAL_32B_tag CVAL2;		/* offset: 0x0124 size: 32 bit */
																		/* PIT_RTI_TCTRL - Timer Control Register */
			PIT_RTI_TCTRL_32B_tag TCTRL2; /* offset: 0x0128 size: 32 bit */
																		/* PIT_RTI_TFLG - Timer Flag Register */
			PIT_RTI_TFLG_32B_tag TFLG2;		/* offset: 0x012C size: 32 bit */
			/* PIT_RTI_LDVAL - Timer Load Value Register */
			PIT_RTI_LDVAL_32B_tag LDVAL3; /* offset: 0x0130 size: 32 bit */
			/* PIT_RTI_CVAL - Current Timer Value Register */
			PIT_RTI_CVAL_32B_tag CVAL3;		/* offset: 0x0134 size: 32 bit */
																		/* PIT_RTI_TCTRL - Timer Control Register */
			PIT_RTI_TCTRL_32B_tag TCTRL3; /* offset: 0x0138 size: 32 bit */
																		/* PIT_RTI_TFLG - Timer Flag Register */
			PIT_RTI_TFLG_32B_tag TFLG3;		/* offset: 0x013C size: 32 bit */
		};
	};
} PIT_RTI_tag;

#define PIT_RTI (*(volatile PIT_RTI_tag*)0xC3FF0000UL)

/****************************************************************/
/*                                                              */
/* Module: ADC  */
/*                                                              */
/****************************************************************/

typedef union { /* module configuration register */
	vuint32_t R;
	struct {
		vuint32_t OWREN : 1;	/* Overwrite enable */
		vuint32_t WLSIDE : 1; /* Write Left/right Alligned */
		vuint32_t MODE : 1;		/* One Shot/Scan Mode Selectiom */
		vuint32_t
				EDGLEV : 1; /* edge or level selection for external start trigger */
		vuint32_t TRGEN : 1;	 /* external trigger enable */
		vuint32_t EDGE : 1;		 /* start trigger egde /level detection */
		vuint32_t XSTRTEN : 1; /* EXTERNAL START ENABLE */
		vuint32_t NSTART : 1;	 /* start normal conversion */
		vuint32_t : 1;
		vuint32_t JTRGEN : 1; /* Injectin External Trigger Enable */
		vuint32_t JEDGE : 1;	/* start trigger egde /level detection for injected */
		vuint32_t JSTART : 1; /* injected conversion start */
		vuint32_t : 2;
		vuint32_t CTUEN : 1; /* CTU enabaled */
		vuint32_t : 8;
		vuint32_t ADCLKSEL : 1;		/* Select which clock for device */
		vuint32_t ABORTCHAIN : 1; /* abort chain conversion */
		vuint32_t ABORT : 1;			/* abort current conversion */
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t ACKO : 1; /* Auto Clock Off Enable */
#else
		vuint32_t ACK0 : 1;					 /* deprecated name - please avoid */
#endif
		vuint32_t OFFREFRESH : 1; /* offset phase selection */
		vuint32_t OFFCANC : 1;		/* offset phase cancellation selection */
		vuint32_t : 2;
		vuint32_t PWDN : 1; /* Power Down Enable */
	} B;
} ADC_MCR_32B_tag;

typedef union { /* module status register */
	vuint32_t R;
	struct {
		vuint32_t : 7;
		vuint32_t NSTART : 1; /* normal conversion status */
		vuint32_t JABORT : 1; /* Injection chain abort status */
		vuint32_t : 2;
		vuint32_t JSTART : 1; /* Injection Start status */
		vuint32_t : 3;
		vuint32_t CTUSTART : 1; /* ctu start status */
		vuint32_t CHADDR : 7;		/* which address conv is goin on */
		vuint32_t : 3;
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t ACKO : 1; /* Auto Clock Off Enable status */
#else
		vuint32_t ACK0 : 1;					 /* deprecated name - please avoid */
#endif
		vuint32_t OFFREFRESH : 1; /* offset refresh status */
		vuint32_t OFFCANC : 1;		/* offset phase cancellation status */
		vuint32_t ADCSTATUS : 3;	/* status of ADC FSM */
	} B;
} ADC_MSR_32B_tag;

typedef union { /* Interrupt status register */
	vuint32_t R;
	struct {
		vuint32_t : 25;
		vuint32_t OFFCANCOVR : 1; /* Offset cancellation phase over */
		vuint32_t EOFFSET : 1;		/* error in offset refresh */
		vuint32_t EOCTU : 1;			/* end of CTU channel conversion */
		vuint32_t JEOC : 1;				/* end of injected channel conversion */
		vuint32_t JECH : 1;				/* end ofinjected chain conversion */
		vuint32_t EOC : 1;				/* end of channel conversion */
		vuint32_t ECH : 1;				/* end of chain conversion */
	} B;
} ADC_ISR_32B_tag;

typedef union { /* CHANNEL PENDING REGISTER 0 */
	vuint32_t R;
	struct {
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH31 : 1; /* Channel 31 conversion over */
#else
		vuint32_t EOC31 : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH30 : 1; /* Channel 30 conversion over */
#else
		vuint32_t EOC30 : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH29 : 1; /* Channel 29 conversion over */
#else
		vuint32_t EOC29 : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH28 : 1; /* Channel 28 conversion over */
#else
		vuint32_t EOC28 : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH27 : 1; /* Channel 27 conversion over */
#else
		vuint32_t EOC27 : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH26 : 1; /* Channel 26 conversion over */
#else
		vuint32_t EOC26 : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH25 : 1; /* Channel 25 conversion over */
#else
		vuint32_t EOC25 : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH24 : 1; /* Channel 24 conversion over */
#else
		vuint32_t EOC24 : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH23 : 1; /* Channel 23 conversion over */
#else
		vuint32_t EOC23 : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH22 : 1; /* Channel 22 conversion over */
#else
		vuint32_t EOC22 : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH21 : 1; /* Channel 21 conversion over */
#else
		vuint32_t EOC21 : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH20 : 1; /* Channel 20 conversion over */
#else
		vuint32_t EOC20 : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH19 : 1; /* Channel 19 conversion over */
#else
		vuint32_t EOC19 : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH18 : 1; /* Channel 18 conversion over */
#else
		vuint32_t EOC18 : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH17 : 1; /* Channel 17 conversion over */
#else
		vuint32_t EOC17 : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH16 : 1; /* Channel 16 conversion over */
#else
		vuint32_t EOC16 : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH15 : 1; /* Channel 15 conversion over */
#else
		vuint32_t EOC15 : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH14 : 1; /* Channel 14 conversion over */
#else
		vuint32_t EOC14 : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH13 : 1; /* Channel 13 conversion over */
#else
		vuint32_t EOC13 : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH12 : 1; /* Channel 12 conversion over */
#else
		vuint32_t EOC12 : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH11 : 1; /* Channel 11 conversion over */
#else
		vuint32_t EOC11 : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH10 : 1; /* Channel 10 conversion over */
#else
		vuint32_t EOC10 : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH9 : 1; /* Channel 9 conversion over */
#else
		vuint32_t EOC9 : 1;					 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH8 : 1; /* Channel 8 conversion over */
#else
		vuint32_t EOC8 : 1;					 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH7 : 1; /* Channel 7 conversion over */
#else
		vuint32_t EOC7 : 1;					 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH6 : 1; /* Channel 6 conversion over */
#else
		vuint32_t EOC6 : 1;					 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH5 : 1; /* Channel 5 conversion over */
#else
		vuint32_t EOC5 : 1;					 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH4 : 1; /* Channel 4 conversion over */
#else
		vuint32_t EOC4 : 1;					 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH3 : 1; /* Channel 3 conversion over */
#else
		vuint32_t EOC3 : 1;					 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH2 : 1; /* Channel 2 conversion over */
#else
		vuint32_t EOC2 : 1;					 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH1 : 1; /* Channel 1 conversion over */
#else
		vuint32_t EOC1 : 1;					 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t EOC_CH0 : 1; /* Channel 0 conversion over */
#else
		vuint32_t EOC0 : 1;					 /* deprecated name - please avoid */
#endif
	} B;
} ADC_CEOCFR0_32B_tag;

typedef union { /* CHANNEL PENDING REGISTER 1 */
	vuint32_t R;
	struct {
		vuint32_t EOC_CH63 : 1; /* Channel 63 conversion over */
		vuint32_t EOC_CH62 : 1; /* Channel 62 conversion over */
		vuint32_t EOC_CH61 : 1; /* Channel 61 conversion over */
		vuint32_t EOC_CH60 : 1; /* Channel 60 conversion over */
		vuint32_t EOC_CH59 : 1; /* Channel 59 conversion over */
		vuint32_t EOC_CH58 : 1; /* Channel 58 conversion over */
		vuint32_t EOC_CH57 : 1; /* Channel 57 conversion over */
		vuint32_t EOC_CH56 : 1; /* Channel 56 conversion over */
		vuint32_t EOC_CH55 : 1; /* Channel 55 conversion over */
		vuint32_t EOC_CH54 : 1; /* Channel 54 conversion over */
		vuint32_t EOC_CH53 : 1; /* Channel 53 conversion over */
		vuint32_t EOC_CH52 : 1; /* Channel 52 conversion over */
		vuint32_t EOC_CH51 : 1; /* Channel 51 conversion over */
		vuint32_t EOC_CH50 : 1; /* Channel 50 conversion over */
		vuint32_t EOC_CH49 : 1; /* Channel 49 conversion over */
		vuint32_t EOC_CH48 : 1; /* Channel 48 conversion over */
		vuint32_t EOC_CH47 : 1; /* Channel 47 conversion over */
		vuint32_t EOC_CH46 : 1; /* Channel 46 conversion over */
		vuint32_t EOC_CH45 : 1; /* Channel 45 conversion over */
		vuint32_t EOC_CH44 : 1; /* Channel 44 conversion over */
		vuint32_t EOC_CH43 : 1; /* Channel 43 conversion over */
		vuint32_t EOC_CH42 : 1; /* Channel 42 conversion over */
		vuint32_t EOC_CH41 : 1; /* Channel 41 conversion over */
		vuint32_t EOC_CH40 : 1; /* Channel 40 conversion over */
		vuint32_t EOC_CH39 : 1; /* Channel 39 conversion over */
		vuint32_t EOC_CH38 : 1; /* Channel 38 conversion over */
		vuint32_t EOC_CH37 : 1; /* Channel 37 conversion over */
		vuint32_t EOC_CH36 : 1; /* Channel 36 conversion over */
		vuint32_t EOC_CH35 : 1; /* Channel 35 conversion over */
		vuint32_t EOC_CH34 : 1; /* Channel 34 conversion over */
		vuint32_t EOC_CH33 : 1; /* Channel 33 conversion over */
		vuint32_t EOC_CH32 : 1; /* Channel 32 conversion over */
	} B;
} ADC_CEOCFR1_32B_tag;

typedef union { /* CHANNEL PENDING REGISTER 2 */
	vuint32_t R;
	struct {
		vuint32_t EOC_CH95 : 1; /* Channel 95 conversion over */
		vuint32_t EOC_CH94 : 1; /* Channel 94 conversion over */
		vuint32_t EOC_CH93 : 1; /* Channel 93 conversion over */
		vuint32_t EOC_CH92 : 1; /* Channel 92 conversion over */
		vuint32_t EOC_CH91 : 1; /* Channel 91 conversion over */
		vuint32_t EOC_CH90 : 1; /* Channel 90 conversion over */
		vuint32_t EOC_CH89 : 1; /* Channel 89 conversion over */
		vuint32_t EOC_CH88 : 1; /* Channel 88 conversion over */
		vuint32_t EOC_CH87 : 1; /* Channel 87 conversion over */
		vuint32_t EOC_CH86 : 1; /* Channel 86 conversion over */
		vuint32_t EOC_CH85 : 1; /* Channel 85 conversion over */
		vuint32_t EOC_CH84 : 1; /* Channel 84 conversion over */
		vuint32_t EOC_CH83 : 1; /* Channel 83 conversion over */
		vuint32_t EOC_CH82 : 1; /* Channel 82 conversion over */
		vuint32_t EOC_CH81 : 1; /* Channel 81 conversion over */
		vuint32_t EOC_CH80 : 1; /* Channel 80 conversion over */
		vuint32_t EOC_CH79 : 1; /* Channel 79 conversion over */
		vuint32_t EOC_CH78 : 1; /* Channel 78 conversion over */
		vuint32_t EOC_CH77 : 1; /* Channel 77 conversion over */
		vuint32_t EOC_CH76 : 1; /* Channel 76 conversion over */
		vuint32_t EOC_CH75 : 1; /* Channel 75 conversion over */
		vuint32_t EOC_CH74 : 1; /* Channel 74 conversion over */
		vuint32_t EOC_CH73 : 1; /* Channel 73 conversion over */
		vuint32_t EOC_CH72 : 1; /* Channel 72 conversion over */
		vuint32_t EOC_CH71 : 1; /* Channel 71 conversion over */
		vuint32_t EOC_CH70 : 1; /* Channel 70 conversion over */
		vuint32_t EOC_CH69 : 1; /* Channel 69 conversion over */
		vuint32_t EOC_CH68 : 1; /* Channel 68 conversion over */
		vuint32_t EOC_CH67 : 1; /* Channel 67 conversion over */
		vuint32_t EOC_CH66 : 1; /* Channel 66 conversion over */
		vuint32_t EOC_CH65 : 1; /* Channel 65 conversion over */
		vuint32_t EOC_CH64 : 1; /* Channel 64 conversion over */
	} B;
} ADC_CEOCFR2_32B_tag;

typedef union { /* interrupt mask register */
	vuint32_t R;
	struct {
		vuint32_t : 25;
		vuint32_t MSKOFFCANCOVR : 1; /* mask bit for Calibration over */
		vuint32_t MSKEOFFSET : 1;		 /* mask bit for Error in offset refresh */
		vuint32_t MSKEOCTU : 1;			 /* mask bit for EOCTU */
		vuint32_t MSKJEOC : 1;			 /* mask bit for JEOC */
		vuint32_t MSKJECH : 1;			 /* mask bit for JECH */
		vuint32_t MSKEOC : 1;				 /* mask bit for EOC */
		vuint32_t MSKECH : 1;				 /* mask bit for ECH */
	} B;
} ADC_IMR_32B_tag;

typedef union { /* CHANNEL INTERRUPT MASK REGISTER 0 */
	vuint32_t R;
	struct {
		vuint32_t CIM31 : 1; /* Channel 31 mask register */
		vuint32_t CIM30 : 1; /* Channel 30 mask register */
		vuint32_t CIM29 : 1; /* Channel 29 mask register */
		vuint32_t CIM28 : 1; /* Channel 28 mask register */
		vuint32_t CIM27 : 1; /* Channel 27 mask register */
		vuint32_t CIM26 : 1; /* Channel 26 mask register */
		vuint32_t CIM25 : 1; /* Channel 25 mask register */
		vuint32_t CIM24 : 1; /* Channel 24 mask register */
		vuint32_t CIM23 : 1; /* Channel 23 mask register */
		vuint32_t CIM22 : 1; /* Channel 22 mask register */
		vuint32_t CIM21 : 1; /* Channel 21 mask register */
		vuint32_t CIM20 : 1; /* Channel 20 mask register */
		vuint32_t CIM19 : 1; /* Channel 19 mask register */
		vuint32_t CIM18 : 1; /* Channel 18 mask register */
		vuint32_t CIM17 : 1; /* Channel 17 mask register */
		vuint32_t CIM16 : 1; /* Channel 16 mask register */
		vuint32_t CIM15 : 1; /* Channel 15 mask register */
		vuint32_t CIM14 : 1; /* Channel 14 mask register */
		vuint32_t CIM13 : 1; /* Channel 13 mask register */
		vuint32_t CIM12 : 1; /* Channel 12 mask register */
		vuint32_t CIM11 : 1; /* Channel 11 mask register */
		vuint32_t CIM10 : 1; /* Channel 10 mask register */
		vuint32_t CIM9 : 1;	 /* Channel 9  mask register */
		vuint32_t CIM8 : 1;	 /* Channel 8  mask register */
		vuint32_t CIM7 : 1;	 /* Channel 7  mask register */
		vuint32_t CIM6 : 1;	 /* Channel 6  mask register */
		vuint32_t CIM5 : 1;	 /* Channel 5  mask register */
		vuint32_t CIM4 : 1;	 /* Channel 4  mask register */
		vuint32_t CIM3 : 1;	 /* Channel 3  mask register */
		vuint32_t CIM2 : 1;	 /* Channel 2  mask register */
		vuint32_t CIM1 : 1;	 /* Channel 1  mask register */
		vuint32_t CIM0 : 1;	 /* Channel 0  mask register */
	} B;
} ADC_CIMR0_32B_tag;

typedef union { /* CHANNEL INTERRUPT MASK REGISTER 1 */
	vuint32_t R;
	struct {
		vuint32_t CIM63 : 1; /* Channel 63 mask register */
		vuint32_t CIM62 : 1; /* Channel 62 mask register */
		vuint32_t CIM61 : 1; /* Channel 61 mask register */
		vuint32_t CIM60 : 1; /* Channel 60 mask register */
		vuint32_t CIM59 : 1; /* Channel 59 mask register */
		vuint32_t CIM58 : 1; /* Channel 58 mask register */
		vuint32_t CIM57 : 1; /* Channel 57 mask register */
		vuint32_t CIM56 : 1; /* Channel 56 mask register */
		vuint32_t CIM55 : 1; /* Channel 55 mask register */
		vuint32_t CIM54 : 1; /* Channel 54 mask register */
		vuint32_t CIM53 : 1; /* Channel 53 mask register */
		vuint32_t CIM52 : 1; /* Channel 52 mask register */
		vuint32_t CIM51 : 1; /* Channel 51 mask register */
		vuint32_t CIM50 : 1; /* Channel 50 mask register */
		vuint32_t CIM49 : 1; /* Channel 49 mask register */
		vuint32_t CIM48 : 1; /* Channel 48 mask register */
		vuint32_t CIM47 : 1; /* Channel 47 mask register */
		vuint32_t CIM46 : 1; /* Channel 46 mask register */
		vuint32_t CIM45 : 1; /* Channel 45 mask register */
		vuint32_t CIM44 : 1; /* Channel 44 mask register */
		vuint32_t CIM43 : 1; /* Channel 43 mask register */
		vuint32_t CIM42 : 1; /* Channel 42 mask register */
		vuint32_t CIM41 : 1; /* Channel 41 mask register */
		vuint32_t CIM40 : 1; /* Channel 40 mask register */
		vuint32_t CIM39 : 1; /* Channel 39 mask register */
		vuint32_t CIM38 : 1; /* Channel 38 mask register */
		vuint32_t CIM37 : 1; /* Channel 37 mask register */
		vuint32_t CIM36 : 1; /* Channel 36 mask register */
		vuint32_t CIM35 : 1; /* Channel 35 mask register */
		vuint32_t CIM34 : 1; /* Channel 34 mask register */
		vuint32_t CIM33 : 1; /* Channel 33 mask register */
		vuint32_t CIM32 : 1; /* Channel 32 mask register */
	} B;
} ADC_CIMR1_32B_tag;

typedef union { /* CHANNEL INTERRUPT MASK REGISTER 2 */
	vuint32_t R;
	struct {
		vuint32_t CIM95 : 1; /* Channel 95 mask register */
		vuint32_t CIM94 : 1; /* Channel 94 mask register */
		vuint32_t CIM93 : 1; /* Channel 93 mask register */
		vuint32_t CIM92 : 1; /* Channel 92 mask register */
		vuint32_t CIM91 : 1; /* Channel 91 mask register */
		vuint32_t CIM90 : 1; /* Channel 90 mask register */
		vuint32_t CIM89 : 1; /* Channel 89 mask register */
		vuint32_t CIM88 : 1; /* Channel 88 mask register */
		vuint32_t CIM87 : 1; /* Channel 87 mask register */
		vuint32_t CIM86 : 1; /* Channel 86 mask register */
		vuint32_t CIM85 : 1; /* Channel 85 mask register */
		vuint32_t CIM84 : 1; /* Channel 84 mask register */
		vuint32_t CIM83 : 1; /* Channel 83 mask register */
		vuint32_t CIM82 : 1; /* Channel 82 mask register */
		vuint32_t CIM81 : 1; /* Channel 81 mask register */
		vuint32_t CIM80 : 1; /* Channel 80 mask register */
		vuint32_t CIM79 : 1; /* Channel 79 mask register */
		vuint32_t CIM78 : 1; /* Channel 78 mask register */
		vuint32_t CIM77 : 1; /* Channel 77 mask register */
		vuint32_t CIM76 : 1; /* Channel 76 mask register */
		vuint32_t CIM75 : 1; /* Channel 75 mask register */
		vuint32_t CIM74 : 1; /* Channel 74 mask register */
		vuint32_t CIM73 : 1; /* Channel 73 mask register */
		vuint32_t CIM72 : 1; /* Channel 72 mask register */
		vuint32_t CIM71 : 1; /* Channel 71 mask register */
		vuint32_t CIM70 : 1; /* Channel 70 mask register */
		vuint32_t CIM69 : 1; /* Channel 69 mask register */
		vuint32_t CIM68 : 1; /* Channel 68 mask register */
		vuint32_t CIM67 : 1; /* Channel 67 mask register */
		vuint32_t CIM66 : 1; /* Channel 66 mask register */
		vuint32_t CIM65 : 1; /* Channel 65 mask register */
		vuint32_t CIM64 : 1; /* Channel 64 mask register */
	} B;
} ADC_CIMR2_32B_tag;

typedef union { /* Watchdog Threshold interrupt status register */
	vuint32_t R;
	struct {
		vuint32_t : 24;
		vuint32_t WDG3H : 1; /* Interrupt generated on the value being higher than
														the HTHV 3 */
		vuint32_t WDG2H : 1; /* Interrupt generated on the value being higher than
														the HTHV 2 */
		vuint32_t WDG1H : 1; /* Interrupt generated on the value being higher than
														the HTHV 1 */
		vuint32_t WDG0H : 1; /* Interrupt generated on the value being higher than
														the HTHV 0 */
		vuint32_t WDG3L : 1; /* Interrupt generated on the value being lower than
														the LTHV 3 */
		vuint32_t WDG2L : 1; /* Interrupt generated on the value being lower than
														the LTHV 2 */
		vuint32_t WDG1L : 1; /* Interrupt generated on the value being lower than
														the LTHV 1 */
		vuint32_t WDG0L : 1; /* Interrupt generated on the value being lower than
														the LTHV 0 */
	} B;
} ADC_WTISR_32B_tag;

typedef union { /* Watchdog interrupt MASK register */
	vuint32_t R;
	struct {
		vuint32_t : 24;
		vuint32_t MSKWDG3H : 1; /* Mask enable for Interrupt generated on the value
															 being higher than the HTHV 3 */
		vuint32_t MSKWDG2H : 1; /* Mask enable for  Interrupt generated on the value
															 being higher than the HTHV 2 */
		vuint32_t MSKWDG1H : 1; /* Mask enable for Interrupt generated on the value
															 being higher than the HTHV 1 */
		vuint32_t MSKWDG0H : 1; /* Mask enable for Interrupt generated on the value
															 being higher than the HTHV 0 */
		vuint32_t MSKWDG3L : 1; /* Mask enable for Interrupt generated on the value
															 being lower than the LTHV 3 */
		vuint32_t MSKWDG2L : 1; /* Mask enable for Interrupt generated on the value
															 being lower than the LTHV 2 */
		vuint32_t MSKWDG1L : 1; /* MAsk enable for Interrupt generated on the value
															 being lower than the LTHV 1 */
		vuint32_t MSKWDG0L : 1; /* Mask enable for Interrupt generated on the value
															 being lower than the LTHV 0 */
	} B;
} ADC_WTIMR_32B_tag;

typedef union { /* DMAE register */
	vuint32_t R;
	struct {
		vuint32_t : 30;
		vuint32_t DCLR : 1;	 /* DMA clear sequence enable */
		vuint32_t DMAEN : 1; /* DMA global enable */
	} B;
} ADC_DMAE_32B_tag;

typedef union { /* DMA  REGISTER 0 */
	vuint32_t R;
	struct {
		vuint32_t DMA31 : 1; /* Channel 31 DMA Enable */
		vuint32_t DMA30 : 1; /* Channel 30 DMA Enable */
		vuint32_t DMA29 : 1; /* Channel 29 DMA Enable */
		vuint32_t DMA28 : 1; /* Channel 28 DMA Enable */
		vuint32_t DMA27 : 1; /* Channel 27 DMA Enable */
		vuint32_t DMA26 : 1; /* Channel 26 DMA Enable */
		vuint32_t DMA25 : 1; /* Channel 25 DMA Enable */
		vuint32_t DMA24 : 1; /* Channel 24 DMA Enable */
		vuint32_t DMA23 : 1; /* Channel 23 DMA Enable */
		vuint32_t DMA22 : 1; /* Channel 22 DMA Enable */
		vuint32_t DMA21 : 1; /* Channel 21 DMA Enable */
		vuint32_t DMA20 : 1; /* Channel 20 DMA Enable */
		vuint32_t DMA19 : 1; /* Channel 19 DMA Enable */
		vuint32_t DMA18 : 1; /* Channel 18 DMA Enable */
		vuint32_t DMA17 : 1; /* Channel 17 DMA Enable */
		vuint32_t DMA16 : 1; /* Channel 16 DMA Enable */
		vuint32_t DMA15 : 1; /* Channel 15 DMA Enable */
		vuint32_t DMA14 : 1; /* Channel 14 DMA Enable */
		vuint32_t DMA13 : 1; /* Channel 13 DMA Enable */
		vuint32_t DMA12 : 1; /* Channel 12 DMA Enable */
		vuint32_t DMA11 : 1; /* Channel 11 DMA Enable */
		vuint32_t DMA10 : 1; /* Channel 10 DMA Enable */
		vuint32_t DMA9 : 1;	 /* Channel 9 DMA Enable */
		vuint32_t DMA8 : 1;	 /* Channel 8 DMA Enable */
		vuint32_t DMA7 : 1;	 /* Channel 7 DMA Enable */
		vuint32_t DMA6 : 1;	 /* Channel 6 DMA Enable */
		vuint32_t DMA5 : 1;	 /* Channel 5 DMA Enable */
		vuint32_t DMA4 : 1;	 /* Channel 4 DMA Enable */
		vuint32_t DMA3 : 1;	 /* Channel 3 DMA Enable */
		vuint32_t DMA2 : 1;	 /* Channel 2 DMA Enable */
		vuint32_t DMA1 : 1;	 /* Channel 1 DMA Enable */
		vuint32_t DMA0 : 1;	 /* Channel 0 DMA Enable */
	} B;
} ADC_DMAR0_32B_tag;

typedef union { /* DMA REGISTER 1 */
	vuint32_t R;
	struct {
		vuint32_t DMA63 : 1; /* Channel 63 DMA Enable */
		vuint32_t DMA62 : 1; /* Channel 62 DMA Enable */
		vuint32_t DMA61 : 1; /* Channel 61 DMA Enable */
		vuint32_t DMA60 : 1; /* Channel 60 DMA Enable */
		vuint32_t DMA59 : 1; /* Channel 59 DMA Enable */
		vuint32_t DMA58 : 1; /* Channel 58 DMA Enable */
		vuint32_t DMA57 : 1; /* Channel 57 DMA Enable */
		vuint32_t DMA56 : 1; /* Channel 56 DMA Enable */
		vuint32_t DMA55 : 1; /* Channel 55 DMA Enable */
		vuint32_t DMA54 : 1; /* Channel 54 DMA Enable */
		vuint32_t DMA53 : 1; /* Channel 53 DMA Enable */
		vuint32_t DMA52 : 1; /* Channel 52 DMA Enable */
		vuint32_t DMA51 : 1; /* Channel 51 DMA Enable */
		vuint32_t DMA50 : 1; /* Channel 50 DMA Enable */
		vuint32_t DMA49 : 1; /* Channel 49 DMA Enable */
		vuint32_t DMA48 : 1; /* Channel 48 DMA Enable */
		vuint32_t DMA47 : 1; /* Channel 47 DMA Enable */
		vuint32_t DMA46 : 1; /* Channel 46 DMA Enable */
		vuint32_t DMA45 : 1; /* Channel 45 DMA Enable */
		vuint32_t DMA44 : 1; /* Channel 44 DMA Enable */
		vuint32_t DMA43 : 1; /* Channel 43 DMA Enable */
		vuint32_t DMA42 : 1; /* Channel 42 DMA Enable */
		vuint32_t DMA41 : 1; /* Channel 41 DMA Enable */
		vuint32_t DMA40 : 1; /* Channel 40 DMA Enable */
		vuint32_t DMA39 : 1; /* Channel 39 DMA Enable */
		vuint32_t DMA38 : 1; /* Channel 38 DMA Enable */
		vuint32_t DMA37 : 1; /* Channel 37 DMA Enable */
		vuint32_t DMA36 : 1; /* Channel 36 DMA Enable */
		vuint32_t DMA35 : 1; /* Channel 35 DMA Enable */
		vuint32_t DMA34 : 1; /* Channel 34 DMA Enable */
		vuint32_t DMA33 : 1; /* Channel 33 DMA Enable */
		vuint32_t DMA32 : 1; /* Channel 32 DMA Enable */
	} B;
} ADC_DMAR1_32B_tag;

typedef union { /* DMA REGISTER 2 */
	vuint32_t R;
	struct {
		vuint32_t DMA95 : 1; /* Channel 95 DMA Enable */
		vuint32_t DMA94 : 1; /* Channel 94 DMA Enable */
		vuint32_t DMA93 : 1; /* Channel 93 DMA Enable */
		vuint32_t DMA92 : 1; /* Channel 92 DMA Enable */
		vuint32_t DMA91 : 1; /* Channel 91 DMA Enable */
		vuint32_t DMA90 : 1; /* Channel 90 DMA Enable */
		vuint32_t DMA89 : 1; /* Channel 89 DMA Enable */
		vuint32_t DMA88 : 1; /* Channel 88 DMA Enable */
		vuint32_t DMA87 : 1; /* Channel 87 DMA Enable */
		vuint32_t DMA86 : 1; /* Channel 86 DMA Enable */
		vuint32_t DMA85 : 1; /* Channel 85 DMA Enable */
		vuint32_t DMA84 : 1; /* Channel 84 DMA Enable */
		vuint32_t DMA83 : 1; /* Channel 83 DMA Enable */
		vuint32_t DMA82 : 1; /* Channel 82 DMA Enable */
		vuint32_t DMA81 : 1; /* Channel 81 DMA Enable */
		vuint32_t DMA80 : 1; /* Channel 80 DMA Enable */
		vuint32_t DMA79 : 1; /* Channel 79 DMA Enable */
		vuint32_t DMA78 : 1; /* Channel 78 DMA Enable */
		vuint32_t DMA77 : 1; /* Channel 77 DMA Enable */
		vuint32_t DMA76 : 1; /* Channel 76 DMA Enable */
		vuint32_t DMA75 : 1; /* Channel 75 DMA Enable */
		vuint32_t DMA74 : 1; /* Channel 74 DMA Enable */
		vuint32_t DMA73 : 1; /* Channel 73 DMA Enable */
		vuint32_t DMA72 : 1; /* Channel 72 DMA Enable */
		vuint32_t DMA71 : 1; /* Channel 71 DMA Enable */
		vuint32_t DMA70 : 1; /* Channel 70 DMA Enable */
		vuint32_t DMA69 : 1; /* Channel 69 DMA Enable */
		vuint32_t DMA68 : 1; /* Channel 68 DMA Enable */
		vuint32_t DMA67 : 1; /* Channel 67 DMA Enable */
		vuint32_t DMA66 : 1; /* Channel 66 DMA Enable */
		vuint32_t DMA65 : 1; /* Channel 65 DMA Enable */
		vuint32_t DMA64 : 1; /* Channel 64 DMA Enable */
	} B;
} ADC_DMAR2_32B_tag;

/* Register layout for all registers TRC... */

typedef union { /* Threshold Control register C */
	vuint32_t R;
	struct {
		vuint32_t : 16;
		vuint32_t THREN : 1;	/* Threshold enable */
		vuint32_t THRINV : 1; /* invert the output pin */
		vuint32_t THROP : 1;	/* output pin register */
		vuint32_t : 6;
		vuint32_t THRCH : 7; /* Choose channel for threshold register */
	} B;
} ADC_TRC_32B_tag;

/* Register layout for all registers THRHLR... */

typedef union { /* Upper Threshold register */
	vuint32_t R;
	struct {
		vuint32_t : 4;
		vuint32_t THRH : 12; /* high threshold value s */
		vuint32_t : 4;
		vuint32_t THRL : 12; /* low threshold value s */
	} B;
} ADC_THRHLR_32B_tag;

/* Register layout for all registers THRALT... */

typedef union { /* alternate Upper Threshold register */
	vuint32_t R;
	struct {
		vuint32_t : 6;
		vuint32_t THRH : 10; /* high threshold value s */
		vuint32_t : 6;
		vuint32_t THRL : 10; /* low threshold value s */
	} B;
} ADC_THRALT_32B_tag;

typedef union { /* PRESAMPLING CONTROL REGISTER */
	vuint32_t R;
	struct {
		vuint32_t : 25;
		vuint32_t PREVAL2 : 2; /* INternal Voltage selection for Presampling */
		vuint32_t PREVAL1 : 2; /* INternal Voltage selection for Presampling */
		vuint32_t PREVAL0 : 2; /* INternal Voltage selection for Presampling */
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t PRECONV : 1; /* Presampled value */
#else
		vuint32_t PREONCE : 1;			 /* deprecated name - please avoid */
#endif
	} B;
} ADC_PSCR_32B_tag;

typedef union { /* Presampling  Register 0 */
	vuint32_t R;
	struct {
		vuint32_t PRES31 : 1; /* Channel 31 Presampling  Enable */
		vuint32_t PRES30 : 1; /* Channel 30 Presampling  Enable */
		vuint32_t PRES29 : 1; /* Channel 29 Presampling  Enable */
		vuint32_t PRES28 : 1; /* Channel 28 Presampling  Enable */
		vuint32_t PRES27 : 1; /* Channel 27 Presampling  Enable */
		vuint32_t PRES26 : 1; /* Channel 26 Presampling  Enable */
		vuint32_t PRES25 : 1; /* Channel 25 Presampling  Enable */
		vuint32_t PRES24 : 1; /* Channel 24 Presampling  Enable */
		vuint32_t PRES23 : 1; /* Channel 23 Presampling  Enable */
		vuint32_t PRES22 : 1; /* Channel 22 Presampling  Enable */
		vuint32_t PRES21 : 1; /* Channel 21 Presampling  Enable */
		vuint32_t PRES20 : 1; /* Channel 20 Presampling  Enable */
		vuint32_t PRES19 : 1; /* Channel 19 Presampling  Enable */
		vuint32_t PRES18 : 1; /* Channel 18 Presampling  Enable */
		vuint32_t PRES17 : 1; /* Channel 17 Presampling  Enable */
		vuint32_t PRES16 : 1; /* Channel 16 Presampling  Enable */
		vuint32_t PRES15 : 1; /* Channel 15   Presampling  Enable */
		vuint32_t PRES14 : 1; /* Channel 14   Presampling  Enable */
		vuint32_t PRES13 : 1; /* Channel 13  Presampling   Enable */
		vuint32_t PRES12 : 1; /* Channel 12   Presampling  Enable */
		vuint32_t PRES11 : 1; /* Channel 11   Presampling  Enable */
		vuint32_t PRES10 : 1; /* Channel 10  Presampling   Enable */
		vuint32_t PRES9 : 1;	/* Channel 9 Presampling   Enable */
		vuint32_t PRES8 : 1;	/* Channel 8 Presampling  Enable */
		vuint32_t PRES7 : 1;	/* Channel 7 Presampling    Enable */
		vuint32_t PRES6 : 1;	/* Channel 6 Presampling   Enable */
		vuint32_t PRES5 : 1;	/* Channel 5 Presampling  Enable */
		vuint32_t PRES4 : 1;	/* Channel 4 Presampling  Enable */
		vuint32_t PRES3 : 1;	/* Channel 3 Presampling  Enable */
		vuint32_t PRES2 : 1;	/* Channel 2 Presampling  Enable */
		vuint32_t PRES1 : 1;	/* Channel 1presampling  Enable */
		vuint32_t PRES0 : 1;	/* Channel 0 Presampling  Enable */
	} B;
} ADC_PSR0_32B_tag;

typedef union { /* Presampling REGISTER 1 */
	vuint32_t R;
	struct {
		vuint32_t PRES63 : 1; /* Channel 63 Presampling  Enable */
		vuint32_t PRES62 : 1; /* Channel 62 Presampling  Enable */
		vuint32_t PRES61 : 1; /* Channel 61 Presampling  Enable */
		vuint32_t PRES60 : 1; /* Channel 60 Presampling  Enable */
		vuint32_t PRES59 : 1; /* Channel 59 Presampling  Enable */
		vuint32_t PRES58 : 1; /* Channel 58 Presampling  Enable */
		vuint32_t PRES57 : 1; /* Channel 57 Presampling  Enable */
		vuint32_t PRES56 : 1; /* Channel 56 Presampling  Enable */
		vuint32_t PRES55 : 1; /* Channel 55 Presampling  Enable */
		vuint32_t PRES54 : 1; /* Channel 54 Presampling  Enable */
		vuint32_t PRES53 : 1; /* Channel 53 Presampling  Enable */
		vuint32_t PRES52 : 1; /* Channel 52 Presampling  Enable */
		vuint32_t PRES51 : 1; /* Channel 51 Presampling  Enable */
		vuint32_t PRES50 : 1; /* Channel 50 Presampling  Enable */
		vuint32_t PRES49 : 1; /* Channel 49 Presampling  Enable */
		vuint32_t PRES48 : 1; /* Channel 48 Presampling  Enable */
		vuint32_t PRES47 : 1; /* Channel 47 Presampling  Enable */
		vuint32_t PRES46 : 1; /* Channel 46 Presampling  Enable */
		vuint32_t PRES45 : 1; /* Channel 45 Presampling  Enable */
		vuint32_t PRES44 : 1; /* Channel 44 Presampling  Enable */
		vuint32_t PRES43 : 1; /* Channel 43 Presampling  Enable */
		vuint32_t PRES42 : 1; /* Channel 42 Presampling  Enable */
		vuint32_t PRES41 : 1; /* Channel 41 Presampling  Enable */
		vuint32_t PRES40 : 1; /* Channel 40 Presampling  Enable */
		vuint32_t PRES39 : 1; /* Channel 39 Presampling  Enable */
		vuint32_t PRES38 : 1; /* Channel 38 Presampling  Enable */
		vuint32_t PRES37 : 1; /* Channel 37 Presampling  Enable */
		vuint32_t PRES36 : 1; /* Channel 36 Presampling  Enable */
		vuint32_t PRES35 : 1; /* Channel 35 Presampling  Enable */
		vuint32_t PRES34 : 1; /* Channel 34 Presampling  Enable */
		vuint32_t PRES33 : 1; /* Channel 33 Presampling  Enable */
		vuint32_t PRES32 : 1; /* Channel 32 Presampling  Enable */
	} B;
} ADC_PSR1_32B_tag;

typedef union { /* Presampling REGISTER 2 */
	vuint32_t R;
	struct {
		vuint32_t PRES95 : 1; /* Channel 95 Presampling  Enable */
		vuint32_t PRES94 : 1; /* Channel 94 Presampling  Enable */
		vuint32_t PRES93 : 1; /* Channel 93 Presampling  Enable */
		vuint32_t PRES92 : 1; /* Channel 92 Presampling  Enable */
		vuint32_t PRES91 : 1; /* Channel 91 Presampling  Enable */
		vuint32_t PRES90 : 1; /* Channel 90 Presampling  Enable */
		vuint32_t PRES89 : 1; /* Channel 89 Presampling  Enable */
		vuint32_t PRES88 : 1; /* Channel 88 Presampling  Enable */
		vuint32_t PRES87 : 1; /* Channel 87 Presampling  Enable */
		vuint32_t PRES86 : 1; /* Channel 86 Presampling  Enable */
		vuint32_t PRES85 : 1; /* Channel 85 Presampling  Enable */
		vuint32_t PRES84 : 1; /* Channel 84 Presampling  Enable */
		vuint32_t PRES83 : 1; /* Channel 83 Presampling  Enable */
		vuint32_t PRES82 : 1; /* Channel 82 Presampling  Enable */
		vuint32_t PRES81 : 1; /* Channel 81 Presampling  Enable */
		vuint32_t PRES80 : 1; /* Channel 80 Presampling  Enable */
		vuint32_t PRES79 : 1; /* Channel 79 Presampling  Enable */
		vuint32_t PRES78 : 1; /* Channel 78 Presampling  Enable */
		vuint32_t PRES77 : 1; /* Channel 77 Presampling  Enable */
		vuint32_t PRES76 : 1; /* Channel 76 Presampling  Enable */
		vuint32_t PRES75 : 1; /* Channel 75 Presampling  Enable */
		vuint32_t PRES74 : 1; /* Channel 74 Presampling  Enable */
		vuint32_t PRES73 : 1; /* Channel 73 Presampling  Enable */
		vuint32_t PRES72 : 1; /* Channel 72 Presampling  Enable */
		vuint32_t PRES71 : 1; /* Channel 71 Presampling  Enable */
		vuint32_t PRES70 : 1; /* Channel 70 Presampling  Enable */
		vuint32_t PRES69 : 1; /* Channel 69 Presampling  Enable */
		vuint32_t PRES68 : 1; /* Channel 68 Presampling  Enable */
		vuint32_t PRES67 : 1; /* Channel 67 Presampling  Enable */
		vuint32_t PRES66 : 1; /* Channel 66 Presampling  Enable */
		vuint32_t PRES65 : 1; /* Channel 65 Presampling  Enable */
		vuint32_t PRES64 : 1; /* Channel 64 Presampling  Enable */
	} B;
} ADC_PSR2_32B_tag;

/* Register layout for all registers CTR... */

typedef union { /* conversion timing register */
	vuint32_t R;
	struct {
		vuint32_t : 16;
		vuint32_t
				INPLATCH : 1; /* configuration bits for the LATCHING PHASE duration */
		vuint32_t : 1;
		vuint32_t OFFSHIFT : 2; /* configuration for offset shift characteristics */
		vuint32_t : 1;
		vuint32_t INPCMP : 2; /* configuration bits for the COMPARISON duration */
		vuint32_t : 1;
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t
				INSAMP : 8; /* configuration bits for the SAMPLING PHASE duration */
#else
		vuint32_t INPSAMP : 8;
#endif
	} B;
} ADC_CTR_32B_tag;

typedef union { /* NORMAL CONVERSION MASK REGISTER 0 */
	vuint32_t R;
	struct {
		vuint32_t CH31 : 1; /* Channel 31 Normal Sampling Enable */
		vuint32_t CH30 : 1; /* Channel 30 Normal Sampling Enable */
		vuint32_t CH29 : 1; /* Channel 29 Normal Sampling Enable */
		vuint32_t CH28 : 1; /* Channel 28 Normal Sampling Enable */
		vuint32_t CH27 : 1; /* Channel 27 Normal Sampling Enable */
		vuint32_t CH26 : 1; /* Channel 26 Normal Sampling Enable */
		vuint32_t CH25 : 1; /* Channel 25 Normal Sampling Enable */
		vuint32_t CH24 : 1; /* Channel 24 Normal Sampling Enable */
		vuint32_t CH23 : 1; /* Channel 23 Normal Sampling Enable */
		vuint32_t CH22 : 1; /* Channel 22 Normal Sampling Enable */
		vuint32_t CH21 : 1; /* Channel 21 Normal Sampling Enable */
		vuint32_t CH20 : 1; /* Channel 20 Normal Sampling Enable */
		vuint32_t CH19 : 1; /* Channel 19 Normal Sampling Enable */
		vuint32_t CH18 : 1; /* Channel 18 Normal Sampling Enable */
		vuint32_t CH17 : 1; /* Channel 17 Normal Sampling Enable */
		vuint32_t CH16 : 1; /* Channel 16 Normal Sampling Enable */
		vuint32_t CH15 : 1; /* Channel 15 Normal Sampling Enable */
		vuint32_t CH14 : 1; /* Channel 14 Normal Sampling Enable */
		vuint32_t CH13 : 1; /* Channel 13 Normal Sampling Enable */
		vuint32_t CH12 : 1; /* Channel 12 Normal Sampling Enable */
		vuint32_t CH11 : 1; /* Channel 11 Normal Sampling Enable */
		vuint32_t CH10 : 1; /* Channel 10 Normal Sampling Enable */
		vuint32_t CH9 : 1;	/* Channel 9 Normal Sampling Enable */
		vuint32_t CH8 : 1;	/* Channel 8 Normal Sampling Enable */
		vuint32_t CH7 : 1;	/* Channel 7 Normal Sampling Enable */
		vuint32_t CH6 : 1;	/* Channel 6 Normal Sampling Enable */
		vuint32_t CH5 : 1;	/* Channel 5 Normal Sampling Enable */
		vuint32_t CH4 : 1;	/* Channel 4 Normal Sampling Enable */
		vuint32_t CH3 : 1;	/* Channel 3 Normal Sampling Enable */
		vuint32_t CH2 : 1;	/* Channel 2 Normal Sampling Enable */
		vuint32_t CH1 : 1;	/* Channel 1 Normal Sampling Enable */
		vuint32_t CH0 : 1;	/* Channel 0 Normal Sampling Enable */
	} B;
} ADC_NCMR0_32B_tag;

typedef union { /* NORMAL CONVERSION MASK REGISTER 1 */
	vuint32_t R;
	struct {
		vuint32_t CH63 : 1; /* Channel 63 Normal Sampling Enable */
		vuint32_t CH62 : 1; /* Channel 62 Normal Sampling Enable */
		vuint32_t CH61 : 1; /* Channel 61 Normal Sampling Enable */
		vuint32_t CH60 : 1; /* Channel 60 Normal Sampling Enable */
		vuint32_t CH59 : 1; /* Channel 59 Normal Sampling Enable */
		vuint32_t CH58 : 1; /* Channel 58 Normal Sampling Enable */
		vuint32_t CH57 : 1; /* Channel 57 Normal Sampling Enable */
		vuint32_t CH56 : 1; /* Channel 56 Normal Sampling Enable */
		vuint32_t CH55 : 1; /* Channel 55 Normal Sampling Enable */
		vuint32_t CH54 : 1; /* Channel 54 Normal Sampling Enable */
		vuint32_t CH53 : 1; /* Channel 53 Normal Sampling Enable */
		vuint32_t CH52 : 1; /* Channel 52 Normal Sampling Enable */
		vuint32_t CH51 : 1; /* Channel 51 Normal Sampling Enable */
		vuint32_t CH50 : 1; /* Channel 50 Normal Sampling Enable */
		vuint32_t CH49 : 1; /* Channel 49 Normal Sampling Enable */
		vuint32_t CH48 : 1; /* Channel 48 Normal Sampling Enable */
		vuint32_t CH47 : 1; /* Channel 47 Normal Sampling Enable */
		vuint32_t CH46 : 1; /* Channel 46 Normal Sampling Enable */
		vuint32_t CH45 : 1; /* Channel 45 Normal Sampling Enable */
		vuint32_t CH44 : 1; /* Channel 44 Normal Sampling Enable */
		vuint32_t CH43 : 1; /* Channel 43 Normal Sampling Enable */
		vuint32_t CH42 : 1; /* Channel 42 Normal Sampling Enable */
		vuint32_t CH41 : 1; /* Channel 41 Normal Sampling Enable */
		vuint32_t CH40 : 1; /* Channel 40 Normal Sampling Enable */
		vuint32_t CH39 : 1; /* Channel 39 Normal Sampling Enable */
		vuint32_t CH38 : 1; /* Channel 38 Normal Sampling Enable */
		vuint32_t CH37 : 1; /* Channel 37 Normal Sampling Enable */
		vuint32_t CH36 : 1; /* Channel 36 Normal Sampling Enable */
		vuint32_t CH35 : 1; /* Channel 35 Normal Sampling Enable */
		vuint32_t CH34 : 1; /* Channel 34 Normal Sampling Enable */
		vuint32_t CH33 : 1; /* Channel 33 Normal Sampling Enable */
		vuint32_t CH32 : 1; /* Channel 32 Normal Sampling Enable */
	} B;
} ADC_NCMR1_32B_tag;

typedef union { /* NORMAL CONVERSION MASK REGISTER 2 */
	vuint32_t R;
	struct {
		vuint32_t CH95 : 1; /* Channel 95 Normal Sampling Enable */
		vuint32_t CH94 : 1; /* Channel 94 Normal Sampling Enable */
		vuint32_t CH93 : 1; /* Channel 93 Normal Sampling Enable */
		vuint32_t CH92 : 1; /* Channel 92 Normal Sampling Enable */
		vuint32_t CH91 : 1; /* Channel 91 Normal Sampling Enable */
		vuint32_t CH90 : 1; /* Channel 90 Normal Sampling Enable */
		vuint32_t CH89 : 1; /* Channel 89 Normal Sampling Enable */
		vuint32_t CH88 : 1; /* Channel 88 Normal Sampling Enable */
		vuint32_t CH87 : 1; /* Channel 87 Normal Sampling Enable */
		vuint32_t CH86 : 1; /* Channel 86 Normal Sampling Enable */
		vuint32_t CH85 : 1; /* Channel 85 Normal Sampling Enable */
		vuint32_t CH84 : 1; /* Channel 84 Normal Sampling Enable */
		vuint32_t CH83 : 1; /* Channel 83 Normal Sampling Enable */
		vuint32_t CH82 : 1; /* Channel 82 Normal Sampling Enable */
		vuint32_t CH81 : 1; /* Channel 81 Normal Sampling Enable */
		vuint32_t CH80 : 1; /* Channel 80 Normal Sampling Enable */
		vuint32_t CH79 : 1; /* Channel 79 Normal Sampling Enable */
		vuint32_t CH78 : 1; /* Channel 78 Normal Sampling Enable */
		vuint32_t CH77 : 1; /* Channel 77 Normal Sampling Enable */
		vuint32_t CH76 : 1; /* Channel 76 Normal Sampling Enable */
		vuint32_t CH75 : 1; /* Channel 75 Normal Sampling Enable */
		vuint32_t CH74 : 1; /* Channel 74 Normal Sampling Enable */
		vuint32_t CH73 : 1; /* Channel 73 Normal Sampling Enable */
		vuint32_t CH72 : 1; /* Channel 72 Normal Sampling Enable */
		vuint32_t CH71 : 1; /* Channel 71 Normal Sampling Enable */
		vuint32_t CH70 : 1; /* Channel 70 Normal Sampling Enable */
		vuint32_t CH69 : 1; /* Channel 69 Normal Sampling Enable */
		vuint32_t CH68 : 1; /* Channel 68 Normal Sampling Enable */
		vuint32_t CH67 : 1; /* Channel 67 Normal Sampling Enable */
		vuint32_t CH66 : 1; /* Channel 66 Normal Sampling Enable */
		vuint32_t CH65 : 1; /* Channel 65 Normal Sampling Enable */
		vuint32_t CH64 : 1; /* Channel 64 Normal Sampling Enable */
	} B;
} ADC_NCMR2_32B_tag;

typedef union { /* Injected Conversion Mask Register 0 */
	vuint32_t R;
	struct {
		vuint32_t CH31 : 1; /* Channel 31 Injected Sampling Enable */
		vuint32_t CH30 : 1; /* Channel 30 Injected Sampling Enable */
		vuint32_t CH29 : 1; /* Channel 29 Injected Sampling Enable */
		vuint32_t CH28 : 1; /* Channel 28 Injected Sampling Enable */
		vuint32_t CH27 : 1; /* Channel 27 Injected Sampling Enable */
		vuint32_t CH26 : 1; /* Channel 26 Injected Sampling Enable */
		vuint32_t CH25 : 1; /* Channel 25 Injected Sampling Enable */
		vuint32_t CH24 : 1; /* Channel 24 Injected Sampling Enable */
		vuint32_t CH23 : 1; /* Channel 23 Injected Sampling Enable */
		vuint32_t CH22 : 1; /* Channel 22 Injected Sampling Enable */
		vuint32_t CH21 : 1; /* Channel 21 Injected Sampling Enable */
		vuint32_t CH20 : 1; /* Channel 20 Injected Sampling Enable */
		vuint32_t CH19 : 1; /* Channel 19 Injected Sampling Enable */
		vuint32_t CH18 : 1; /* Channel 18 Injected Sampling Enable */
		vuint32_t CH17 : 1; /* Channel 17 Injected Sampling Enable */
		vuint32_t CH16 : 1; /* Channel 16 Injected Sampling Enable */
		vuint32_t CH15 : 1; /* Channel 15   Injected Sampling Enable */
		vuint32_t CH14 : 1; /* Channel 14   Injected Sampling Enable */
		vuint32_t CH13 : 1; /* Channel 13  Injected  Sampling Enable */
		vuint32_t CH12 : 1; /* Channel 12   Injected Sampling Enable */
		vuint32_t CH11 : 1; /* Channel 11   Injected Sampling Enable */
		vuint32_t CH10 : 1; /* Channel 10  Injected  Sampling Enable */
		vuint32_t CH9 : 1;	/* Channel 9 Injected  Sampling Enable */
		vuint32_t CH8 : 1;	/* Channel 8 Injected Sampling Enable */
		vuint32_t CH7 : 1;	/* Channel 7 Injected   Sampling Enable */
		vuint32_t CH6 : 1;	/* Channel 6 Injected  Sampling Enable */
		vuint32_t CH5 : 1;	/* Channel 5 Injected Sampling Enable */
		vuint32_t CH4 : 1;	/* Channel 4 Injected Sampling Enable */
		vuint32_t CH3 : 1;	/* Channel 3 Injected Sampling Enable */
		vuint32_t CH2 : 1;	/* Channel 2 Injected Sampling Enable */
		vuint32_t CH1 : 1;	/* Channel 1 injected Sampling Enable */
		vuint32_t CH0 : 1;	/* Channel 0 injected Sampling Enable */
	} B;
} ADC_JCMR0_32B_tag;

typedef union { /* INJECTED CONVERSION MASK REGISTER 1 */
	vuint32_t R;
	struct {
		vuint32_t CH63 : 1; /* Channel 63 Injected Sampling Enable */
		vuint32_t CH62 : 1; /* Channel 62 Injected Sampling Enable */
		vuint32_t CH61 : 1; /* Channel 61 Injected Sampling Enable */
		vuint32_t CH60 : 1; /* Channel 60 Injected Sampling Enable */
		vuint32_t CH59 : 1; /* Channel 59 Injected Sampling Enable */
		vuint32_t CH58 : 1; /* Channel 58 Injected Sampling Enable */
		vuint32_t CH57 : 1; /* Channel 57 Injected Sampling Enable */
		vuint32_t CH56 : 1; /* Channel 56 Injected Sampling Enable */
		vuint32_t CH55 : 1; /* Channel 55 Injected Sampling Enable */
		vuint32_t CH54 : 1; /* Channel 54 Injected Sampling Enable */
		vuint32_t CH53 : 1; /* Channel 53 Injected Sampling Enable */
		vuint32_t CH52 : 1; /* Channel 52 Injected Sampling Enable */
		vuint32_t CH51 : 1; /* Channel 51 Injected Sampling Enable */
		vuint32_t CH50 : 1; /* Channel 50 Injected Sampling Enable */
		vuint32_t CH49 : 1; /* Channel 49 Injected Sampling Enable */
		vuint32_t CH48 : 1; /* Channel 48 Injected Sampling Enable */
		vuint32_t CH47 : 1; /* Channel 47 Injected Sampling Enable */
		vuint32_t CH46 : 1; /* Channel 46 Injected Sampling Enable */
		vuint32_t CH45 : 1; /* Channel 45 Injected Sampling Enable */
		vuint32_t CH44 : 1; /* Channel 44 Injected Sampling Enable */
		vuint32_t CH43 : 1; /* Channel 43 Injected Sampling Enable */
		vuint32_t CH42 : 1; /* Channel 42 Injected Sampling Enable */
		vuint32_t CH41 : 1; /* Channel 41 Injected Sampling Enable */
		vuint32_t CH40 : 1; /* Channel 40 Injected Sampling Enable */
		vuint32_t CH39 : 1; /* Channel 39 Injected Sampling Enable */
		vuint32_t CH38 : 1; /* Channel 38 Injected Sampling Enable */
		vuint32_t CH37 : 1; /* Channel 37 Injected Sampling Enable */
		vuint32_t CH36 : 1; /* Channel 36 Injected Sampling Enable */
		vuint32_t CH35 : 1; /* Channel 35 Injected Sampling Enable */
		vuint32_t CH34 : 1; /* Channel 34 Injected Sampling Enable */
		vuint32_t CH33 : 1; /* Channel 33 Injected Sampling Enable */
		vuint32_t CH32 : 1; /* Channel 32 Injected Sampling Enable */
	} B;
} ADC_JCMR1_32B_tag;

typedef union { /* INJECTED CONVERSION MASK REGISTER 2 */
	vuint32_t R;
	struct {
		vuint32_t CH95 : 1; /* Channel 95 Injected Sampling Enable */
		vuint32_t CH94 : 1; /* Channel 94 Injected Sampling Enable */
		vuint32_t CH93 : 1; /* Channel 93 Injected Sampling Enable */
		vuint32_t CH92 : 1; /* Channel 92 Injected Sampling Enable */
		vuint32_t CH91 : 1; /* Channel 91 Injected Sampling Enable */
		vuint32_t CH90 : 1; /* Channel 90 Injected Sampling Enable */
		vuint32_t CH89 : 1; /* Channel 89 Injected Sampling Enable */
		vuint32_t CH88 : 1; /* Channel 88 Injected Sampling Enable */
		vuint32_t CH87 : 1; /* Channel 87 Injected Sampling Enable */
		vuint32_t CH86 : 1; /* Channel 86 Injected Sampling Enable */
		vuint32_t CH85 : 1; /* Channel 85 Injected Sampling Enable */
		vuint32_t CH84 : 1; /* Channel 84 Injected Sampling Enable */
		vuint32_t CH83 : 1; /* Channel 83 Injected Sampling Enable */
		vuint32_t CH82 : 1; /* Channel 82 Injected Sampling Enable */
		vuint32_t CH81 : 1; /* Channel 81 Injected Sampling Enable */
		vuint32_t CH80 : 1; /* Channel 80 Injected Sampling Enable */
		vuint32_t CH79 : 1; /* Channel 79 Injected Sampling Enable */
		vuint32_t CH78 : 1; /* Channel 78 Injected Sampling Enable */
		vuint32_t CH77 : 1; /* Channel 77 Injected Sampling Enable */
		vuint32_t CH76 : 1; /* Channel 76 Injected Sampling Enable */
		vuint32_t CH75 : 1; /* Channel 75 Injected Sampling Enable */
		vuint32_t CH74 : 1; /* Channel 74 Injected Sampling Enable */
		vuint32_t CH73 : 1; /* Channel 73 Injected Sampling Enable */
		vuint32_t CH72 : 1; /* Channel 72 Injected Sampling Enable */
		vuint32_t CH71 : 1; /* Channel 71 Injected Sampling Enable */
		vuint32_t CH70 : 1; /* Channel 70 Injected Sampling Enable */
		vuint32_t CH69 : 1; /* Channel 69 Injected Sampling Enable */
		vuint32_t CH68 : 1; /* Channel 68 Injected Sampling Enable */
		vuint32_t CH67 : 1; /* Channel 67 Injected Sampling Enable */
		vuint32_t CH66 : 1; /* Channel 66 Injected Sampling Enable */
		vuint32_t CH65 : 1; /* Channel 65 Injected Sampling Enable */
		vuint32_t CH64 : 1; /* Channel 64 Injected Sampling Enable */
	} B;
} ADC_JCMR2_32B_tag;

typedef union { /* Offset Word Regsiter */
	vuint32_t R;
	struct {
		vuint32_t : 15;
		vuint32_t OFFSETLOAD : 1; /* load_offset */
		vuint32_t : 8;
#ifndef USE_FIELD_ALIASES_ADC
		vuint32_t
				OFFSET_WORD : 8; /* OFFSET word coeff.generated at the end of offset
														cancellation is lathed int o this register */
#else
		vuint32_t OFFSETWORD : 8;
#endif
	} B;
} ADC_OFFWR_32B_tag;

typedef union { /* Decode Signal Delay Register */
	vuint32_t R;
	struct {
		vuint32_t : 24;
		vuint32_t
				DSD : 8; /* take into account the settling time of the external mux */
	} B;
} ADC_DSDR_32B_tag;

typedef union { /* Power Down Dealy Register */
	vuint32_t R;
	struct {
		vuint32_t : 24;
		vuint32_t PDED : 8; /* The delay between the power down bit reset and the
													 starting of conversion */
	} B;
} ADC_PDEDR_32B_tag;

/* Register layout for all registers CDR... */

typedef union { /* CHANNEL DATA REGS */
	vuint32_t R;
	struct {
		vuint32_t : 12;
		vuint32_t VALID : 1;	/* validity of data */
		vuint32_t OVERW : 1;	/* overwrite data */
		vuint32_t RESULT : 2; /* reflects mode conversion */
		vuint32_t : 6;
		vuint32_t CDATA : 10; /* Channel 0 converted data */
	} B;
} ADC_CDR_32B_tag;

typedef union { /* Upper Threshold register 4 is not contiguous to 3 */
	vuint32_t R;
	struct {
		vuint32_t : 4;
		vuint32_t THRH : 12; /* high threshold value s */
		vuint32_t : 4;
		vuint32_t THRL : 12; /* low threshold value s */
	} B;
} ADC_THRHLR4_32B_tag;

typedef union { /* Upper Threshold register 5 */
	vuint32_t R;
	struct {
		vuint32_t : 4;
		vuint32_t THRH : 12; /* high threshold value s */
		vuint32_t : 4;
		vuint32_t THRL : 12; /* low threshold value s */
	} B;
} ADC_THRHLR5_32B_tag;

typedef union { /* Upper Threshold register 6 */
	vuint32_t R;
	struct {
		vuint32_t : 4;
		vuint32_t THRH : 12; /* high threshold value s */
		vuint32_t : 4;
		vuint32_t THRL : 12; /* low threshold value s */
	} B;
} ADC_THRHLR6_32B_tag;

typedef union { /* Upper Threshold register 7 */
	vuint32_t R;
	struct {
		vuint32_t : 4;
		vuint32_t THRH : 12; /* high threshold value s */
		vuint32_t : 4;
		vuint32_t THRL : 12; /* low threshold value s */
	} B;
} ADC_THRHLR7_32B_tag;

typedef union { /* Upper Threshold register 8 */
	vuint32_t R;
	struct {
		vuint32_t : 4;
		vuint32_t THRH : 12; /* high threshold value s */
		vuint32_t : 4;
		vuint32_t THRL : 12; /* low threshold value s */
	} B;
} ADC_THRHLR8_32B_tag;

typedef union { /* Upper Threshold register 9 */
	vuint32_t R;
	struct {
		vuint32_t : 4;
		vuint32_t THRH : 12; /* high threshold value s */
		vuint32_t : 4;
		vuint32_t THRL : 12; /* low threshold value s */
	} B;
} ADC_THRHLR9_32B_tag;

typedef union { /* Upper Threshold register 10 */
	vuint32_t R;
	struct {
		vuint32_t : 4;
		vuint32_t THRH : 12; /* high threshold value s */
		vuint32_t : 4;
		vuint32_t THRL : 12; /* low threshold value s */
	} B;
} ADC_THRHLR10_32B_tag;

typedef union { /* Upper Threshold register 11 */
	vuint32_t R;
	struct {
		vuint32_t : 4;
		vuint32_t THRH : 12; /* high threshold value s */
		vuint32_t : 4;
		vuint32_t THRL : 12; /* low threshold value s */
	} B;
} ADC_THRHLR11_32B_tag;

typedef union { /* Upper Threshold register 12 */
	vuint32_t R;
	struct {
		vuint32_t : 4;
		vuint32_t THRH : 12; /* high threshold value s */
		vuint32_t : 4;
		vuint32_t THRL : 12; /* low threshold value s */
	} B;
} ADC_THRHLR12_32B_tag;

typedef union { /* Upper Threshold register 13 */
	vuint32_t R;
	struct {
		vuint32_t : 4;
		vuint32_t THRH : 12; /* high threshold value s */
		vuint32_t : 4;
		vuint32_t THRL : 12; /* low threshold value s */
	} B;
} ADC_THRHLR13_32B_tag;

typedef union { /* Upper Threshold register 14 */
	vuint32_t R;
	struct {
		vuint32_t : 4;
		vuint32_t THRH : 12; /* high threshold value s */
		vuint32_t : 4;
		vuint32_t THRL : 12; /* low threshold value s */
	} B;
} ADC_THRHLR14_32B_tag;

typedef union { /* Upper Threshold register 15 */
	vuint32_t R;
	struct {
		vuint32_t : 4;
		vuint32_t THRH : 12; /* high threshold value s */
		vuint32_t : 4;
		vuint32_t THRL : 12; /* low threshold value s */
	} B;
} ADC_THRHLR15_32B_tag;

/* Register layout for all registers CWSELR... */

typedef union { /* Channel Watchdog Select register */
	vuint32_t R;
	struct {
		vuint32_t WSEL_CH7 : 4; /* Channel Watchdog select for channel 7+R*8 */
		vuint32_t WSEL_CH6 : 4; /* Channel Watchdog select for channel 6+R*8 */
		vuint32_t WSEL_CH5 : 4; /* Channel Watchdog select for channel 5+R*8 */
		vuint32_t WSEL_CH4 : 4; /* Channel Watchdog select for channel 4+R*8 */
		vuint32_t WSEL_CH3 : 4; /* Channel Watchdog select for channel 3+R*8 */
		vuint32_t WSEL_CH2 : 4; /* Channel Watchdog select for channel 2+R*8 */
		vuint32_t WSEL_CH1 : 4; /* Channel Watchdog select for channel 1+R*8 */
		vuint32_t WSEL_CH0 : 4; /* Channel Watchdog select for channel 0+R*8 */
	} B;
} ADC_CWSELR_32B_tag;

/* Register layout for all registers CWENR... */

typedef union { /* Channel Watchdog Enable Register */
	vuint32_t R;
	struct {
		vuint32_t : 16;
		vuint32_t CWEN15PRT32 : 1; /* Channel Watchdog Enable 0+R*32 */
		vuint32_t CWEN14PRT32 : 1; /* Channel Watchdog Enable 0+R*32 */
		vuint32_t CWEN13PRT32 : 1; /* Channel Watchdog Enable 0+R*32 */
		vuint32_t CWEN12PRT32 : 1; /* Channel Watchdog Enable 0+R*32 */
		vuint32_t CWEN11PRT32 : 1; /* Channel Watchdog Enable 0+R*32 */
		vuint32_t CWEN10PRT32 : 1; /* Channel Watchdog Enable 0+R*32 */
		vuint32_t CWEN09PRT32 : 1; /* Channel Watchdog Enable 0+R*32 */
		vuint32_t CWEN08PRT32 : 1; /* Channel Watchdog Enable 0+R*32 */
		vuint32_t CWEN07PRT32 : 1; /* Channel Watchdog Enable 0+R*32 */
		vuint32_t CWEN06PRT32 : 1; /* Channel Watchdog Enable 0+R*32 */
		vuint32_t CWEN05PRT32 : 1; /* Channel Watchdog Enable 0+R*32 */
		vuint32_t CWEN04PRT32 : 1; /* Channel Watchdog Enable 0+R*32 */
		vuint32_t CWEN03PRT32 : 1; /* Channel Watchdog Enable 0+R*32 */
		vuint32_t CWEN02PRT32 : 1; /* Channel Watchdog Enable 0+R*32 */
		vuint32_t CWEN01PRT32 : 1; /* Channel Watchdog Enable 0+R*32 */
		vuint32_t CWEN00PRT32 : 1; /* Channel Watchdog Enable 0+R*32 */
	} B;
} ADC_CWENR_32B_tag;

/* Register layout for all registers AWORR... */

typedef union { /* Analog Watchdog Out of Range Register */
	vuint32_t R;
	struct {
		vuint32_t AWOR_CH31 : 1; /* Channel 31+R*32 converted data out of range */
		vuint32_t AWOR_CH30 : 1; /* Channel 30+R*32 converted data out of range */
		vuint32_t AWOR_CH29 : 1; /* Channel 29+R*32 converted data out of range */
		vuint32_t AWOR_CH28 : 1; /* Channel 28+R*32 converted data out of range */
		vuint32_t AWOR_CH27 : 1; /* Channel 27+R*32 converted data out of range */
		vuint32_t AWOR_CH26 : 1; /* Channel 26+R*32 converted data out of range */
		vuint32_t AWOR_CH25 : 1; /* Channel 25+R*32 converted data out of range */
		vuint32_t AWOR_CH24 : 1; /* Channel 24+R*32 converted data out of range */
		vuint32_t AWOR_CH23 : 1; /* Channel 23+R*32 converted data out of range */
		vuint32_t AWOR_CH22 : 1; /* Channel 22+R*32 converted data out of range */
		vuint32_t AWOR_CH21 : 1; /* Channel 21+R*32 converted data out of range */
		vuint32_t AWOR_CH20 : 1; /* Channel 20+R*32 converted data out of range */
		vuint32_t AWOR_CH19 : 1; /* Channel 19+R*32 converted data out of range */
		vuint32_t AWOR_CH18 : 1; /* Channel 18+R*32 converted data out of range */
		vuint32_t AWOR_CH17 : 1; /* Channel 17+R*32 converted data out of range */
		vuint32_t AWOR_CH16 : 1; /* Channel 16+R*32 converted data out of range */
		vuint32_t AWOR_CH15 : 1; /* Channel 15+R*32 converted data out of range */
		vuint32_t AWOR_CH14 : 1; /* Channel 14+R*32 converted data out of range */
		vuint32_t AWOR_CH13 : 1; /* Channel 13+R*32 converted data out of range */
		vuint32_t AWOR_CH12 : 1; /* Channel 12+R*32 converted data out of range */
		vuint32_t AWOR_CH11 : 1; /* Channel 11+R*32 converted data out of range */
		vuint32_t AWOR_CH10 : 1; /* Channel 10+R*32 converted data out of range */
		vuint32_t AWOR_CH9 : 1;	 /* Channel 9+R*32 converted data out of range */
		vuint32_t AWOR_CH8 : 1;	 /* Channel 8+R*32 converted data out of range */
		vuint32_t AWOR_CH7 : 1;	 /* Channel 7+R*32 converted data out of range */
		vuint32_t AWOR_CH6 : 1;	 /* Channel 6+R*32 converted data out of range */
		vuint32_t AWOR_CH5 : 1;	 /* Channel 5+R*32 converted data out of range */
		vuint32_t AWOR_CH4 : 1;	 /* Channel 4+R*32 converted data out of range */
		vuint32_t AWOR_CH3 : 1;	 /* Channel 3+R*32 converted data out of range */
		vuint32_t AWOR_CH2 : 1;	 /* Channel 2+R*32 converted data out of range */
		vuint32_t AWOR_CH1 : 1;	 /* Channel 1+R*32 converted data out of range */
		vuint32_t AWOR_CH0 : 1;	 /* Channel 0+R*32 converted data out of range */
	} B;
} ADC_AWORR_32B_tag;

typedef union { /* SELF TEST CONFIGURATION REGISTER 1 */
	vuint32_t R;
	struct {
		vuint32_t INPSAMP_C : 8; /* Sampling phase duration for the test conversions
																- algorithm C */
		vuint32_t INPSAMP_RC : 8; /* Sampling phase duration for the test
																 conversions - algorithm RC */
		vuint32_t INPSAMP_S : 8; /* Sampling phase duration for the test conversions
																- algorithm S */
		vuint32_t : 5;
		vuint32_t ST_INPCMP : 2; /* Configuration bit for comparison phase duration
																for self test channel */
		vuint32_t ST_INPLATCH : 1; /* Configuration bit for Latching phase duration
																	for self test channel */
	} B;
} ADC_STCR1_32B_tag;

typedef union { /* SELF TEST CONFIGURATION REGISTER 2 */
	vuint32_t R;
	struct {
		vuint32_t : 5;
		vuint32_t SERR : 1;				 /* Error fault injection bit (write only) */
		vuint32_t MSKSTWDTERR : 1; /* Interrupt enable (STSR2.WDTERR status bit) */
		vuint32_t : 1;
		vuint32_t MSKST_EOC : 1; /* Interrupt enable bit for STSR2.ST_EOC */
		vuint32_t : 4;
		vuint32_t MSKWDG_EOA_C : 1;	 /* Interrupt enable (WDG_EOA_C status bit) */
		vuint32_t MSKWDG_EOA_RC : 1; /* Interrupt enable (WDG_EOA_RC status bit) */
		vuint32_t MSKWDG_EOA_S : 1;	 /* Interrupt enable (WDG_EOA_S status bit) */
		vuint32_t MSKERR_C : 1;			 /* Interrupt enable (ERR_C status bit) */
		vuint32_t MSKERR_RC : 1;		 /* Interrupt enable (ERR_RC status bit) */
		vuint32_t MSKERR_S2 : 1;		 /* Interrupt enable (ERR_S2 status bit) */
		vuint32_t MSKERR_S1 : 1;		 /* Interrupt enable (ERR_S1 status bit) */
		vuint32_t MSKERR_S0 : 1;		 /* Interrupt enable (ERR_S0 status bit) */
		vuint32_t : 3;
		vuint32_t EN : 1; /* Self testing channel enable */
		vuint32_t : 4;
		vuint32_t FMA_C : 1;	/* Fault mapping for the algorithm C */
		vuint32_t FMAR_C : 1; /* Fault mapping for the algorithm RC */
		vuint32_t FMA_S : 1;	/* Fault mapping for the algorithm BGAP */
	} B;
} ADC_STCR2_32B_tag;

typedef union { /* SELF TEST CONFIGURATION REGISTER 3 */
	vuint32_t R;
	struct {
		vuint32_t : 22;
		vuint32_t ALG : 2; /* Algorithm scheduling */
		vuint32_t : 8;
	} B;
} ADC_STCR3_32B_tag;

typedef union { /* SELF TEST BAUD RATE REGISTER */
	vuint32_t R;
	struct {
		vuint32_t : 13;
		vuint32_t WDT : 3; /* Watchdog timer value */
		vuint32_t : 8;
		vuint32_t BR : 8; /* Baud rate for the selected algorithm in SCAN mode */
	} B;
} ADC_STBRR_32B_tag;

typedef union { /* SELF TEST STATUS REGISTER 1 */
	vuint32_t R;
	struct {
		vuint32_t : 6;
		vuint32_t WDTERR : 1; /* Watchdog timer error */
		vuint32_t OVERWR : 1; /* Overwrite error */
		vuint32_t ST_EOC : 1; /* Self test EOC bit */
		vuint32_t : 4;
		vuint32_t WDG_EOA_C : 1;	/* Algorithm  C completed without error */
		vuint32_t WDG_EOA_RC : 1; /* Algorithm RC completed without error */
		vuint32_t WDG_EOA_S : 1;	/* Algorithm  S completed without error */
		vuint32_t ERR_C : 1;	/* Error on the self testing channel (algorithm  C) */
		vuint32_t ERR_RC : 1; /* Error on the self testing channel (algorithm RC) */
		vuint32_t ERR_S2 : 1; /* Error on the self testing channel (algorithm
														 SUPPLY, step 2) */
		vuint32_t ERR_S1 : 1; /* Error on the self testing channel (algorithm
														 SUPPLY, step 1) */
		vuint32_t ERR_S0 : 1; /* Error on the self testing channel (algorithm
														 SUPPLY, step 0) */
		vuint32_t : 1;
		vuint32_t STEP_C : 5;	 /* Step of algorithm  C when ERR_C  has occurred */
		vuint32_t STEP_RC : 5; /* Step of algorithm RC when ERR_RC has occurred */
	} B;
} ADC_STSR1_32B_tag;

typedef union { /* SELF TEST STATUS REGISTER 2 */
	vuint32_t R;
	struct {
		vuint32_t OVFL : 1; /* Overflow bit */
		vuint32_t : 3;
		vuint32_t
				DATA1 : 12; /* Test channel converted data when ERR_S1 has occurred */
		vuint32_t : 4;
		vuint32_t
				DATA0 : 12; /* Test channel converted data when ERR_S1 has occurred */
	} B;
} ADC_STSR2_32B_tag;

typedef union { /* SELF TEST STATUS REGISTER 3 */
	vuint32_t R;
	struct {
		vuint32_t : 4;
		vuint32_t
				DATA1 : 12; /* Test channel converted data when ERR_S0 has occurred */
		vuint32_t : 4;
		vuint32_t
				DATA0 : 12; /* Test channel converted data when ERR_S0 has occurred */
	} B;
} ADC_STSR3_32B_tag;

typedef union { /* SELF TEST STATUS REGISTER 4 */
	vuint32_t R;
	struct {
		vuint32_t : 4;
		vuint32_t
				DATA1 : 12; /* Test channel converted data when ERR_C has occurred */
		vuint32_t : 4;
		vuint32_t
				DATA0 : 12; /* Test channel converted data when ERR_C has occurred */
	} B;
} ADC_STSR4_32B_tag;

typedef union { /* SELF TEST DATA REGISTER 1 */
	vuint32_t R;
	struct {
		vuint32_t : 12;
		vuint32_t VALID : 1;	/* Valid data */
		vuint32_t OVERWR : 1; /* Overwrite data */
		vuint32_t : 6;
		vuint32_t TCDATA : 12; /* Test channel converted data */
	} B;
} ADC_STDR1_32B_tag;

typedef union { /* SELF TEST DATA REGISTER 2 */
	vuint32_t R;
	struct {
		vuint32_t
				FDATA : 12;			 /* Fractional part of the ratio TEST for algorithm S */
		vuint32_t VALID : 1; /* Valid data */
		vuint32_t OVERWR : 1; /* Overwrite data */
		vuint32_t : 6;
		vuint32_t IDATA : 12; /* Integer part of the ratio TEST for algorithm S */
	} B;
} ADC_STDR2_32B_tag;

typedef union { /* SELF TEST ANALOG WATCHDOG REGISTER 0 */
	vuint32_t R;
	struct {
		vuint32_t AWDE : 1; /* Analog WatchDog Enable - algorithm S */
		vuint32_t WDTE : 1; /* WatchDog Timer Enable - algorithm S */
		vuint32_t : 2;
		vuint32_t THRH : 12; /* High threshold value for channel 0 */
		vuint32_t : 4;
		vuint32_t THRL : 12; /* Low threshold value for channel 0 */
	} B;
} ADC_STAW0R_32B_tag;

typedef union { /* SELF TEST ANALOG WATCHDOG REGISTER 1A */
	vuint32_t R;
	struct {
		vuint32_t AWDE : 1; /* Analog WatchDog Enable - algorithm S */
		vuint32_t : 3;
		vuint32_t
				THRH : 12; /* High threshold value for test channel - algorithm S */
		vuint32_t : 4;
		vuint32_t
				THRL : 12; /* Low threshold value for test channel - algorithm S */
	} B;
} ADC_STAW1AR_32B_tag;

typedef union { /* SELF TEST ANALOG WATCHDOG REGISTER 1B */
	vuint32_t R;
	struct {
		vuint32_t : 4;
		vuint32_t
				THRH : 12; /* High threshold value for test channel - algorithm S */
		vuint32_t : 4;
		vuint32_t
				THRL : 12; /* Low threshold value for test channel - algorithm S */
	} B;
} ADC_STAW1BR_32B_tag;

typedef union { /* SELF TEST ANALOG WATCHDOG REGISTER 2 */
	vuint32_t R;
	struct {
		vuint32_t AWDE : 1; /* Analog WatchDog Enable - algorithm S */
		vuint32_t : 19;
		vuint32_t THRL : 12; /* Low threshold value for channel */
	} B;
} ADC_STAW2R_32B_tag;

typedef union { /* SELF TEST ANALOG WATCHDOG REGISTER 3 */
	vuint32_t R;
	struct {
		vuint32_t AWDE : 1; /* Analog WatchDog Enable - algorithm RC */
		vuint32_t WDTE : 1; /* WatchDog Timer Enable - algorithm RC */
		vuint32_t : 2;
		vuint32_t THRH : 12; /* High threshold value for channel 3 */
		vuint32_t : 4;
		vuint32_t THRL : 12; /* Low threshold value for channel 3 */
	} B;
} ADC_STAW3R_32B_tag;

typedef union { /* SELF TEST ANALOG WATCHDOG REGISTER 4 */
	vuint32_t R;
	struct {
		vuint32_t AWDE : 1; /* Analog WatchDog Enable - algorithm C */
		vuint32_t WDTE : 1; /* WatchDog Timer Enable - algorithm C */
		vuint32_t : 2;
		vuint32_t THRH : 12; /* High threshold value for channel 4 */
		vuint32_t : 4;
		vuint32_t THRL : 12; /* Low threshold value for channel 4 */
	} B;
} ADC_STAW4R_32B_tag;

typedef union { /* SELF TEST ANALOG WATCHDOG REGISTER 5 */
	vuint32_t R;
	struct {
		vuint32_t : 4;
		vuint32_t THRH : 12; /* High threshold value for algorithm C */
		vuint32_t : 4;
		vuint32_t THRL : 12; /* Low threshold value for algorithm C */
	} B;
} ADC_STAW5R_32B_tag;

typedef struct ADC_struct_tag { /* start of ADC_tag */
																/* module configuration register */
	ADC_MCR_32B_tag MCR;					/* offset: 0x0000 size: 32 bit */
																/* module status register */
	ADC_MSR_32B_tag MSR;					/* offset: 0x0004 size: 32 bit */
	int8_t ADC_reserved_0008[8];
	/* Interrupt status register */
	ADC_ISR_32B_tag ISR; /* offset: 0x0010 size: 32 bit */
	union {
		ADC_CEOCFR0_32B_tag CEOCFR[3]; /* offset: 0x0014  (0x0004 x 3) */

		struct {
			/* CHANNEL PENDING REGISTER 0 */
			ADC_CEOCFR0_32B_tag CEOCFR0; /* offset: 0x0014 size: 32 bit */
																	 /* CHANNEL PENDING REGISTER 1 */
			ADC_CEOCFR1_32B_tag CEOCFR1; /* offset: 0x0018 size: 32 bit */
																	 /* CHANNEL PENDING REGISTER 2 */
			ADC_CEOCFR2_32B_tag CEOCFR2; /* offset: 0x001C size: 32 bit */
		};
	};
	/* interrupt mask register */
	ADC_IMR_32B_tag IMR; /* offset: 0x0020 size: 32 bit */
	union {
		ADC_CIMR0_32B_tag CIMR[3]; /* offset: 0x0024  (0x0004 x 3) */

		struct {
			/* CHANNEL INTERRUPT MASK REGISTER 0 */
			ADC_CIMR0_32B_tag CIMR0; /* offset: 0x0024 size: 32 bit */
															 /* CHANNEL INTERRUPT MASK REGISTER 1 */
			ADC_CIMR1_32B_tag CIMR1; /* offset: 0x0028 size: 32 bit */
															 /* CHANNEL INTERRUPT MASK REGISTER 2 */
			ADC_CIMR2_32B_tag CIMR2; /* offset: 0x002C size: 32 bit */
		};
	};
	/* Watchdog Threshold interrupt status register */
	ADC_WTISR_32B_tag WTISR; /* offset: 0x0030 size: 32 bit */
													 /* Watchdog interrupt MASK register */
	ADC_WTIMR_32B_tag WTIMR; /* offset: 0x0034 size: 32 bit */
	int8_t ADC_reserved_0038[8];
	/* DMAE register */
	ADC_DMAE_32B_tag DMAE; /* offset: 0x0040 size: 32 bit */
	union {
		ADC_DMAR0_32B_tag DMAR[3]; /* offset: 0x0044  (0x0004 x 3) */

		struct {
			/* DMA  REGISTER 0 */
			ADC_DMAR0_32B_tag DMAR0; /* offset: 0x0044 size: 32 bit */
															 /* DMA REGISTER 1 */
			ADC_DMAR1_32B_tag DMAR1; /* offset: 0x0048 size: 32 bit */
															 /* DMA REGISTER 2 */
			ADC_DMAR2_32B_tag DMAR2; /* offset: 0x004C size: 32 bit */
		};
	};
	union {
		/* Threshold Control register C */
		ADC_TRC_32B_tag TRC[4]; /* offset: 0x0050  (0x0004 x 4) */

		struct {
			/* Threshold Control register C */
			ADC_TRC_32B_tag TRC0; /* offset: 0x0050 size: 32 bit */
			ADC_TRC_32B_tag TRC1; /* offset: 0x0054 size: 32 bit */
			ADC_TRC_32B_tag TRC2; /* offset: 0x0058 size: 32 bit */
			ADC_TRC_32B_tag TRC3; /* offset: 0x005C size: 32 bit */
		};
	};
	union {
		/* Upper Threshold register */
		ADC_THRHLR_32B_tag THRHLR[4]; /* offset: 0x0060  (0x0004 x 4) */

		struct {
			/* Upper Threshold register */
			ADC_THRHLR_32B_tag THRHLR0; /* offset: 0x0060 size: 32 bit */
			ADC_THRHLR_32B_tag THRHLR1; /* offset: 0x0064 size: 32 bit */
			ADC_THRHLR_32B_tag THRHLR2; /* offset: 0x0068 size: 32 bit */
			ADC_THRHLR_32B_tag THRHLR3; /* offset: 0x006C size: 32 bit */
		};
	};
	union {
		/* alternate Upper Threshold register */
		ADC_THRALT_32B_tag THRALT[4]; /* offset: 0x0070  (0x0004 x 4) */

		struct {
			/* alternate Upper Threshold register */
			ADC_THRALT_32B_tag THRALT0; /* offset: 0x0070 size: 32 bit */
			ADC_THRALT_32B_tag THRALT1; /* offset: 0x0074 size: 32 bit */
			ADC_THRALT_32B_tag THRALT2; /* offset: 0x0078 size: 32 bit */
			ADC_THRALT_32B_tag THRALT3; /* offset: 0x007C size: 32 bit */
		};
	};
	/* PRESAMPLING CONTROL REGISTER */
	ADC_PSCR_32B_tag PSCR; /* offset: 0x0080 size: 32 bit */
	union {
		ADC_PSR0_32B_tag PSR[3]; /* offset: 0x0084  (0x0004 x 3) */

		struct {
			/* Presampling  Register 0 */
			ADC_PSR0_32B_tag PSR0; /* offset: 0x0084 size: 32 bit */
														 /* Presampling REGISTER 1 */
			ADC_PSR1_32B_tag PSR1; /* offset: 0x0088 size: 32 bit */
														 /* Presampling REGISTER 2 */
			ADC_PSR2_32B_tag PSR2; /* offset: 0x008C size: 32 bit */
		};
	};
	int8_t ADC_reserved_0090_C[4];
	union {
		/* conversion timing register */
		ADC_CTR_32B_tag CTR[3]; /* offset: 0x0094  (0x0004 x 3) */

		struct {
			/* conversion timing register */
			ADC_CTR_32B_tag CTR0; /* offset: 0x0094 size: 32 bit */
			ADC_CTR_32B_tag CTR1; /* offset: 0x0098 size: 32 bit */
			ADC_CTR_32B_tag CTR2; /* offset: 0x009C size: 32 bit */
		};
	};
	int8_t ADC_reserved_00A0_C[4];
	union {
		ADC_NCMR0_32B_tag NCMR[3]; /* offset: 0x00A4  (0x0004 x 3) */

		struct {
			/* NORMAL CONVERSION MASK REGISTER 0 */
			ADC_NCMR0_32B_tag NCMR0; /* offset: 0x00A4 size: 32 bit */
															 /* NORMAL CONVERSION MASK REGISTER 1 */
			ADC_NCMR1_32B_tag NCMR1; /* offset: 0x00A8 size: 32 bit */
															 /* NORMAL CONVERSION MASK REGISTER 2 */
			ADC_NCMR2_32B_tag NCMR2; /* offset: 0x00AC size: 32 bit */
		};
	};
	int8_t ADC_reserved_00B0_C[4];
	union {
		ADC_JCMR0_32B_tag JCMR[3]; /* offset: 0x00B4  (0x0004 x 3) */

		struct {
			/* Injected Conversion Mask Register 0 */
			ADC_JCMR0_32B_tag JCMR0; /* offset: 0x00B4 size: 32 bit */
															 /* INJECTED CONVERSION MASK REGISTER 1 */
			ADC_JCMR1_32B_tag JCMR1; /* offset: 0x00B8 size: 32 bit */
															 /* INJECTED CONVERSION MASK REGISTER 2 */
			ADC_JCMR2_32B_tag JCMR2; /* offset: 0x00BC size: 32 bit */
		};
	};
	/* Offset Word Regsiter */
	ADC_OFFWR_32B_tag OFFWR; /* offset: 0x00C0 size: 32 bit */
													 /* Decode Signal Delay Register */
	ADC_DSDR_32B_tag DSDR;	 /* offset: 0x00C4 size: 32 bit */
													 /* Power Down Dealy Register */
	ADC_PDEDR_32B_tag PDEDR; /* offset: 0x00C8 size: 32 bit */
	int8_t ADC_reserved_00CC_C[52];
	union {
		/* CHANNEL DATA REGS */
		ADC_CDR_32B_tag CDR[96]; /* offset: 0x0100  (0x0004 x 96) */

		struct {
			/* CHANNEL DATA REGS */
			ADC_CDR_32B_tag CDR0;	 /* offset: 0x0100 size: 32 bit */
			ADC_CDR_32B_tag CDR1;	 /* offset: 0x0104 size: 32 bit */
			ADC_CDR_32B_tag CDR2;	 /* offset: 0x0108 size: 32 bit */
			ADC_CDR_32B_tag CDR3;	 /* offset: 0x010C size: 32 bit */
			ADC_CDR_32B_tag CDR4;	 /* offset: 0x0110 size: 32 bit */
			ADC_CDR_32B_tag CDR5;	 /* offset: 0x0114 size: 32 bit */
			ADC_CDR_32B_tag CDR6;	 /* offset: 0x0118 size: 32 bit */
			ADC_CDR_32B_tag CDR7;	 /* offset: 0x011C size: 32 bit */
			ADC_CDR_32B_tag CDR8;	 /* offset: 0x0120 size: 32 bit */
			ADC_CDR_32B_tag CDR9;	 /* offset: 0x0124 size: 32 bit */
			ADC_CDR_32B_tag CDR10; /* offset: 0x0128 size: 32 bit */
			ADC_CDR_32B_tag CDR11; /* offset: 0x012C size: 32 bit */
			ADC_CDR_32B_tag CDR12; /* offset: 0x0130 size: 32 bit */
			ADC_CDR_32B_tag CDR13; /* offset: 0x0134 size: 32 bit */
			ADC_CDR_32B_tag CDR14; /* offset: 0x0138 size: 32 bit */
			ADC_CDR_32B_tag CDR15; /* offset: 0x013C size: 32 bit */
			ADC_CDR_32B_tag CDR16; /* offset: 0x0140 size: 32 bit */
			ADC_CDR_32B_tag CDR17; /* offset: 0x0144 size: 32 bit */
			ADC_CDR_32B_tag CDR18; /* offset: 0x0148 size: 32 bit */
			ADC_CDR_32B_tag CDR19; /* offset: 0x014C size: 32 bit */
			ADC_CDR_32B_tag CDR20; /* offset: 0x0150 size: 32 bit */
			ADC_CDR_32B_tag CDR21; /* offset: 0x0154 size: 32 bit */
			ADC_CDR_32B_tag CDR22; /* offset: 0x0158 size: 32 bit */
			ADC_CDR_32B_tag CDR23; /* offset: 0x015C size: 32 bit */
			ADC_CDR_32B_tag CDR24; /* offset: 0x0160 size: 32 bit */
			ADC_CDR_32B_tag CDR25; /* offset: 0x0164 size: 32 bit */
			ADC_CDR_32B_tag CDR26; /* offset: 0x0168 size: 32 bit */
			ADC_CDR_32B_tag CDR27; /* offset: 0x016C size: 32 bit */
			ADC_CDR_32B_tag CDR28; /* offset: 0x0170 size: 32 bit */
			ADC_CDR_32B_tag CDR29; /* offset: 0x0174 size: 32 bit */
			ADC_CDR_32B_tag CDR30; /* offset: 0x0178 size: 32 bit */
			ADC_CDR_32B_tag CDR31; /* offset: 0x017C size: 32 bit */
			ADC_CDR_32B_tag CDR32; /* offset: 0x0180 size: 32 bit */
			ADC_CDR_32B_tag CDR33; /* offset: 0x0184 size: 32 bit */
			ADC_CDR_32B_tag CDR34; /* offset: 0x0188 size: 32 bit */
			ADC_CDR_32B_tag CDR35; /* offset: 0x018C size: 32 bit */
			ADC_CDR_32B_tag CDR36; /* offset: 0x0190 size: 32 bit */
			ADC_CDR_32B_tag CDR37; /* offset: 0x0194 size: 32 bit */
			ADC_CDR_32B_tag CDR38; /* offset: 0x0198 size: 32 bit */
			ADC_CDR_32B_tag CDR39; /* offset: 0x019C size: 32 bit */
			ADC_CDR_32B_tag CDR40; /* offset: 0x01A0 size: 32 bit */
			ADC_CDR_32B_tag CDR41; /* offset: 0x01A4 size: 32 bit */
			ADC_CDR_32B_tag CDR42; /* offset: 0x01A8 size: 32 bit */
			ADC_CDR_32B_tag CDR43; /* offset: 0x01AC size: 32 bit */
			ADC_CDR_32B_tag CDR44; /* offset: 0x01B0 size: 32 bit */
			ADC_CDR_32B_tag CDR45; /* offset: 0x01B4 size: 32 bit */
			ADC_CDR_32B_tag CDR46; /* offset: 0x01B8 size: 32 bit */
			ADC_CDR_32B_tag CDR47; /* offset: 0x01BC size: 32 bit */
			ADC_CDR_32B_tag CDR48; /* offset: 0x01C0 size: 32 bit */
			ADC_CDR_32B_tag CDR49; /* offset: 0x01C4 size: 32 bit */
			ADC_CDR_32B_tag CDR50; /* offset: 0x01C8 size: 32 bit */
			ADC_CDR_32B_tag CDR51; /* offset: 0x01CC size: 32 bit */
			ADC_CDR_32B_tag CDR52; /* offset: 0x01D0 size: 32 bit */
			ADC_CDR_32B_tag CDR53; /* offset: 0x01D4 size: 32 bit */
			ADC_CDR_32B_tag CDR54; /* offset: 0x01D8 size: 32 bit */
			ADC_CDR_32B_tag CDR55; /* offset: 0x01DC size: 32 bit */
			ADC_CDR_32B_tag CDR56; /* offset: 0x01E0 size: 32 bit */
			ADC_CDR_32B_tag CDR57; /* offset: 0x01E4 size: 32 bit */
			ADC_CDR_32B_tag CDR58; /* offset: 0x01E8 size: 32 bit */
			ADC_CDR_32B_tag CDR59; /* offset: 0x01EC size: 32 bit */
			ADC_CDR_32B_tag CDR60; /* offset: 0x01F0 size: 32 bit */
			ADC_CDR_32B_tag CDR61; /* offset: 0x01F4 size: 32 bit */
			ADC_CDR_32B_tag CDR62; /* offset: 0x01F8 size: 32 bit */
			ADC_CDR_32B_tag CDR63; /* offset: 0x01FC size: 32 bit */
			ADC_CDR_32B_tag CDR64; /* offset: 0x0200 size: 32 bit */
			ADC_CDR_32B_tag CDR65; /* offset: 0x0204 size: 32 bit */
			ADC_CDR_32B_tag CDR66; /* offset: 0x0208 size: 32 bit */
			ADC_CDR_32B_tag CDR67; /* offset: 0x020C size: 32 bit */
			ADC_CDR_32B_tag CDR68; /* offset: 0x0210 size: 32 bit */
			ADC_CDR_32B_tag CDR69; /* offset: 0x0214 size: 32 bit */
			ADC_CDR_32B_tag CDR70; /* offset: 0x0218 size: 32 bit */
			ADC_CDR_32B_tag CDR71; /* offset: 0x021C size: 32 bit */
			ADC_CDR_32B_tag CDR72; /* offset: 0x0220 size: 32 bit */
			ADC_CDR_32B_tag CDR73; /* offset: 0x0224 size: 32 bit */
			ADC_CDR_32B_tag CDR74; /* offset: 0x0228 size: 32 bit */
			ADC_CDR_32B_tag CDR75; /* offset: 0x022C size: 32 bit */
			ADC_CDR_32B_tag CDR76; /* offset: 0x0230 size: 32 bit */
			ADC_CDR_32B_tag CDR77; /* offset: 0x0234 size: 32 bit */
			ADC_CDR_32B_tag CDR78; /* offset: 0x0238 size: 32 bit */
			ADC_CDR_32B_tag CDR79; /* offset: 0x023C size: 32 bit */
			ADC_CDR_32B_tag CDR80; /* offset: 0x0240 size: 32 bit */
			ADC_CDR_32B_tag CDR81; /* offset: 0x0244 size: 32 bit */
			ADC_CDR_32B_tag CDR82; /* offset: 0x0248 size: 32 bit */
			ADC_CDR_32B_tag CDR83; /* offset: 0x024C size: 32 bit */
			ADC_CDR_32B_tag CDR84; /* offset: 0x0250 size: 32 bit */
			ADC_CDR_32B_tag CDR85; /* offset: 0x0254 size: 32 bit */
			ADC_CDR_32B_tag CDR86; /* offset: 0x0258 size: 32 bit */
			ADC_CDR_32B_tag CDR87; /* offset: 0x025C size: 32 bit */
			ADC_CDR_32B_tag CDR88; /* offset: 0x0260 size: 32 bit */
			ADC_CDR_32B_tag CDR89; /* offset: 0x0264 size: 32 bit */
			ADC_CDR_32B_tag CDR90; /* offset: 0x0268 size: 32 bit */
			ADC_CDR_32B_tag CDR91; /* offset: 0x026C size: 32 bit */
			ADC_CDR_32B_tag CDR92; /* offset: 0x0270 size: 32 bit */
			ADC_CDR_32B_tag CDR93; /* offset: 0x0274 size: 32 bit */
			ADC_CDR_32B_tag CDR94; /* offset: 0x0278 size: 32 bit */
			ADC_CDR_32B_tag CDR95; /* offset: 0x027C size: 32 bit */
		};
	};
	/* Upper Threshold register 4 is not contiguous to 3 */
	ADC_THRHLR4_32B_tag THRHLR4;	 /* offset: 0x0280 size: 32 bit */
																 /* Upper Threshold register 5 */
	ADC_THRHLR5_32B_tag THRHLR5;	 /* offset: 0x0284 size: 32 bit */
																 /* Upper Threshold register 6 */
	ADC_THRHLR6_32B_tag THRHLR6;	 /* offset: 0x0288 size: 32 bit */
																 /* Upper Threshold register 7 */
	ADC_THRHLR7_32B_tag THRHLR7;	 /* offset: 0x028C size: 32 bit */
																 /* Upper Threshold register 8 */
	ADC_THRHLR8_32B_tag THRHLR8;	 /* offset: 0x0290 size: 32 bit */
																 /* Upper Threshold register 9 */
	ADC_THRHLR9_32B_tag THRHLR9;	 /* offset: 0x0294 size: 32 bit */
																 /* Upper Threshold register 10 */
	ADC_THRHLR10_32B_tag THRHLR10; /* offset: 0x0298 size: 32 bit */
																 /* Upper Threshold register 11 */
	ADC_THRHLR11_32B_tag THRHLR11; /* offset: 0x029C size: 32 bit */
																 /* Upper Threshold register 12 */
	ADC_THRHLR12_32B_tag THRHLR12; /* offset: 0x02A0 size: 32 bit */
																 /* Upper Threshold register 13 */
	ADC_THRHLR13_32B_tag THRHLR13; /* offset: 0x02A4 size: 32 bit */
																 /* Upper Threshold register 14 */
	ADC_THRHLR14_32B_tag THRHLR14; /* offset: 0x02A8 size: 32 bit */
																 /* Upper Threshold register 15 */
	ADC_THRHLR15_32B_tag THRHLR15; /* offset: 0x02AC size: 32 bit */
	union {
		/* Channel Watchdog Select register */
		ADC_CWSELR_32B_tag CWSELR[12]; /* offset: 0x02B0  (0x0004 x 12) */

		struct {
			/* Channel Watchdog Select register */
			ADC_CWSELR_32B_tag CWSELR0;	 /* offset: 0x02B0 size: 32 bit */
			ADC_CWSELR_32B_tag CWSELR1;	 /* offset: 0x02B4 size: 32 bit */
			ADC_CWSELR_32B_tag CWSELR2;	 /* offset: 0x02B8 size: 32 bit */
			ADC_CWSELR_32B_tag CWSELR3;	 /* offset: 0x02BC size: 32 bit */
			ADC_CWSELR_32B_tag CWSELR4;	 /* offset: 0x02C0 size: 32 bit */
			ADC_CWSELR_32B_tag CWSELR5;	 /* offset: 0x02C4 size: 32 bit */
			ADC_CWSELR_32B_tag CWSELR6;	 /* offset: 0x02C8 size: 32 bit */
			ADC_CWSELR_32B_tag CWSELR7;	 /* offset: 0x02CC size: 32 bit */
			ADC_CWSELR_32B_tag CWSELR8;	 /* offset: 0x02D0 size: 32 bit */
			ADC_CWSELR_32B_tag CWSELR9;	 /* offset: 0x02D4 size: 32 bit */
			ADC_CWSELR_32B_tag CWSELR10; /* offset: 0x02D8 size: 32 bit */
			ADC_CWSELR_32B_tag CWSELR11; /* offset: 0x02DC size: 32 bit */
		};
	};
	union {
		/* Channel Watchdog Enable Register */
		ADC_CWENR_32B_tag CWENR[3]; /* offset: 0x02E0  (0x0004 x 3) */

		struct {
			/* Channel Watchdog Enable Register */
			ADC_CWENR_32B_tag CWENR0; /* offset: 0x02E0 size: 32 bit */
			ADC_CWENR_32B_tag CWENR1; /* offset: 0x02E4 size: 32 bit */
			ADC_CWENR_32B_tag CWENR2; /* offset: 0x02E8 size: 32 bit */
		};
	};
	int8_t ADC_reserved_02EC_C[4];
	union {
		/* Analog Watchdog Out of Range Register */
		ADC_AWORR_32B_tag AWORR[3]; /* offset: 0x02F0  (0x0004 x 3) */

		struct {
			/* Analog Watchdog Out of Range Register */
			ADC_AWORR_32B_tag AWORR0; /* offset: 0x02F0 size: 32 bit */
			ADC_AWORR_32B_tag AWORR1; /* offset: 0x02F4 size: 32 bit */
			ADC_AWORR_32B_tag AWORR2; /* offset: 0x02F8 size: 32 bit */
		};
	};
	int8_t ADC_reserved_02FC[68];
	/* SELF TEST CONFIGURATION REGISTER 1 */
	ADC_STCR1_32B_tag STCR1; /* offset: 0x0340 size: 32 bit */
													 /* SELF TEST CONFIGURATION REGISTER 2 */
	ADC_STCR2_32B_tag STCR2; /* offset: 0x0344 size: 32 bit */
													 /* SELF TEST CONFIGURATION REGISTER 3 */
	ADC_STCR3_32B_tag STCR3; /* offset: 0x0348 size: 32 bit */
													 /* SELF TEST BAUD RATE REGISTER */
	ADC_STBRR_32B_tag STBRR; /* offset: 0x034C size: 32 bit */
													 /* SELF TEST STATUS REGISTER 1 */
	ADC_STSR1_32B_tag STSR1; /* offset: 0x0350 size: 32 bit */
													 /* SELF TEST STATUS REGISTER 2 */
	ADC_STSR2_32B_tag STSR2; /* offset: 0x0354 size: 32 bit */
													 /* SELF TEST STATUS REGISTER 3 */
	ADC_STSR3_32B_tag STSR3; /* offset: 0x0358 size: 32 bit */
													 /* SELF TEST STATUS REGISTER 4 */
	ADC_STSR4_32B_tag STSR4; /* offset: 0x035C size: 32 bit */
	int8_t ADC_reserved_0360[16];
	/* SELF TEST DATA REGISTER 1 */
	ADC_STDR1_32B_tag STDR1; /* offset: 0x0370 size: 32 bit */
													 /* SELF TEST DATA REGISTER 2 */
	ADC_STDR2_32B_tag STDR2; /* offset: 0x0374 size: 32 bit */
	int8_t ADC_reserved_0378[8];
	/* SELF TEST ANALOG WATCHDOG REGISTER 0 */
	ADC_STAW0R_32B_tag STAW0R;	 /* offset: 0x0380 size: 32 bit */
															 /* SELF TEST ANALOG WATCHDOG REGISTER 1A */
	ADC_STAW1AR_32B_tag STAW1AR; /* offset: 0x0384 size: 32 bit */
															 /* SELF TEST ANALOG WATCHDOG REGISTER 1B */
	ADC_STAW1BR_32B_tag STAW1BR; /* offset: 0x0388 size: 32 bit */
															 /* SELF TEST ANALOG WATCHDOG REGISTER 2 */
	ADC_STAW2R_32B_tag STAW2R;	 /* offset: 0x038C size: 32 bit */
															 /* SELF TEST ANALOG WATCHDOG REGISTER 3 */
	ADC_STAW3R_32B_tag STAW3R;	 /* offset: 0x0390 size: 32 bit */
															 /* SELF TEST ANALOG WATCHDOG REGISTER 4 */
	ADC_STAW4R_32B_tag STAW4R;	 /* offset: 0x0394 size: 32 bit */
															 /* SELF TEST ANALOG WATCHDOG REGISTER 5 */
	ADC_STAW5R_32B_tag STAW5R;	 /* offset: 0x0398 size: 32 bit */
} ADC_tag;

#define ADC0 (*(volatile ADC_tag*)0xFFE00000UL)
#define ADC1 (*(volatile ADC_tag*)0xFFE04000UL)

/****************************************************************/
/*                                                              */
/* Module: CTU  */
/*                                                              */
/****************************************************************/

typedef union { /* Trigger Generator Subunit Input Selection register */
	vuint32_t R;
	struct {
		vuint32_t I15_FE : 1; /* ext_signal Falling Edge */
		vuint32_t I15_RE : 1; /* ext_signal Rising  Edge */
		vuint32_t I14_FE : 1; /* eTimer2 Falling Edge Enable */
		vuint32_t I14_RE : 1; /* eTimer2 Rising  Edge Enable */
		vuint32_t I13_FE : 1; /* eTimer1 Falling Edge Enable */
		vuint32_t I13_RE : 1; /* eTimer1 Rising  Edge Enable */
		vuint32_t I12_FE : 1; /* RPWM ch3 Falling Edge Enable */
		vuint32_t I12_RE : 1; /* RPWM ch3 Rising  Edge Enable */
		vuint32_t I11_FE : 1; /* RPWM ch2 Falling Edge Enable */
		vuint32_t I11_RE : 1; /* RPWM ch2 Rising  Edge Enable */
		vuint32_t I10_FE : 1; /* RPWM ch1 Falling Edge Enable */
		vuint32_t I10_RE : 1; /* RPWM ch1 Rising  Edge Enable */
		vuint32_t I9_FE : 1;	/* RPWM ch0 Falling Edge Enable */
		vuint32_t I9_RE : 1;	/* RPWM ch0 Rising  Edge Enable */
		vuint32_t I8_FE : 1;	/* PWM ch3 even trig Falling edge Enable */
		vuint32_t I8_RE : 1;	/* PWM ch3 even trig Rising  edge Enable */
		vuint32_t I7_FE : 1;	/* PWM ch2 even trig Falling edge Enable */
		vuint32_t I7_RE : 1;	/* PWM ch2 even trig Rising  edge Enable */
		vuint32_t I6_FE : 1;	/* PWM ch1 even trig Falling edge Enable */
		vuint32_t I6_RE : 1;	/* PWM ch1 even trig Rising  edge Enable */
		vuint32_t I5_FE : 1;	/* PWM ch0 even trig Falling edge Enable */
		vuint32_t I5_RE : 1;	/* PWM ch0 even trig Rising edge Enable */
		vuint32_t I4_FE : 1;	/* PWM ch3 odd trig Falling edge Enable */
		vuint32_t I4_RE : 1;	/* PWM ch3 odd trig Rising  edge Enable */
		vuint32_t I3_FE : 1;	/* PWM ch2 odd trig Falling edge Enable */
		vuint32_t I3_RE : 1;	/* PWM ch2 odd trig Rising  edge Enable */
		vuint32_t I2_FE : 1;	/* PWM ch1 odd trig Falling edge Enable */
		vuint32_t I2_RE : 1;	/* PWM ch1 odd trig Rising  edge Enable */
		vuint32_t I1_FE : 1;	/* PWM ch0 odd trig Falling edge Enable */
		vuint32_t I1_RE : 1;	/* PWM ch0 odd trig Rising  edge Enable */
		vuint32_t I0_FE : 1;	/* PWM Reload Falling Edge Enable */
		vuint32_t I0_RE : 1;	/* PWM Reload Rising  Edge Enable */
	} B;
} CTU_TGSISR_32B_tag;

typedef union { /* Trigger Generator Subunit Control Register */
	vuint16_t R;
	struct {
		vuint16_t : 7;
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t ET_TM : 1; /* Toggle Mode Enable */
#else
		vuint16_t ETTM : 1;				/* deprecated name - please avoid */
#endif
		vuint16_t PRES : 2; /* TGS Prescaler Selection */
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t MRS_SM : 5; /* MRS Selection in Sequential Mode */
#else
		vuint16_t MRSSM : 5;			/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t TGS_M : 1; /* Trigger Generator Subunit Mode */
#else
		vuint16_t TGSM : 1;				/* deprecated name - please avoid */
#endif
	} B;
} CTU_TGSCR_16B_tag;

typedef union { /*  */
	vuint16_t R;
} CTU_TCR_16B_tag;

typedef union { /* TGS Counter Compare Register */
	vuint16_t R;
#ifndef USE_FIELD_ALIASES_CTU
	struct {
		vuint16_t TGSCCV : 16; /* deprecated field -- do not use */
	} B;
#endif
} CTU_TGSCCR_16B_tag;

typedef union { /* TGS Counter Reload Register */
	vuint16_t R;
#ifndef USE_FIELD_ALIASES_CTU
	struct {
		vuint16_t TGSCRV : 16; /* deprecated field -- do not use */
	} B;
#endif
} CTU_TGSCRR_16B_tag;

typedef union { /* Commands List Control Register 1 */
	vuint32_t R;
	struct {
		vuint32_t : 3;
		vuint32_t T3INDEX : 5; /* Trigger 3 First Command address */
		vuint32_t : 3;
		vuint32_t T2INDEX : 5; /* Trigger 2 First Command address */
		vuint32_t : 3;
		vuint32_t T1INDEX : 5; /* Trigger 1 First Command address */
		vuint32_t : 3;
		vuint32_t T0INDEX : 5; /* Trigger 0 First Command address */
	} B;
} CTU_CLCR1_32B_tag;

typedef union { /* Commands List Control Register 2 */
	vuint32_t R;
	struct {
		vuint32_t : 3;
		vuint32_t T7INDEX : 5; /* Trigger 7 First Command address */
		vuint32_t : 3;
		vuint32_t T6INDEX : 5; /* Trigger 6 First Command address */
		vuint32_t : 3;
		vuint32_t T5INDEX : 5; /* Trigger 5 First Command address */
		vuint32_t : 3;
		vuint32_t T4INDEX : 5; /* Trigger 4 First Command address */
	} B;
} CTU_CLCR2_32B_tag;

typedef union { /* Trigger Handler Control Register 1 */
	vuint32_t R;
	struct {
		vuint32_t : 1;
		vuint32_t T3_E : 1;		 /* Trigger 3 enable */
		vuint32_t T3_ETE : 1;	 /* Trigger 3 Ext Trigger output enable */
		vuint32_t T3_T4E : 1;	 /* Trigger 3 Timer4 output enable */
		vuint32_t T3_T3E : 1;	 /* Trigger 3 Timer3 output enable */
		vuint32_t T3_T2E : 1;	 /* Trigger 3 Timer2 output enable */
		vuint32_t T3_T1E : 1;	 /* Trigger 3 Timer1 output enable */
		vuint32_t T3_ADCE : 1; /* Trigger 3 ADC Command output enable */
		vuint32_t : 1;
		vuint32_t T2_E : 1;		 /* Trigger 2 enable */
		vuint32_t T2_ETE : 1;	 /* Trigger 2 Ext Trigger output enable */
		vuint32_t T2_T4E : 1;	 /* Trigger 2 Timer4 output enable */
		vuint32_t T2_T3E : 1;	 /* Trigger 2 Timer3 output enable */
		vuint32_t T2_T2E : 1;	 /* Trigger 2 Timer2 output enable */
		vuint32_t T2_T1E : 1;	 /* Trigger 2 Timer1 output enable */
		vuint32_t T2_ADCE : 1; /* Trigger 2 ADC Command output enable */
		vuint32_t : 1;
		vuint32_t T1_E : 1;		 /* Trigger 1 enable */
		vuint32_t T1_ETE : 1;	 /* Trigger 1 Ext Trigger output enable */
		vuint32_t T1_T4E : 1;	 /* Trigger 1 Timer4 output enable */
		vuint32_t T1_T3E : 1;	 /* Trigger 1 Timer3 output enable */
		vuint32_t T1_T2E : 1;	 /* Trigger 1 Timer2 output enable */
		vuint32_t T1_T1E : 1;	 /* Trigger 1 Timer1 output enable */
		vuint32_t T1_ADCE : 1; /* Trigger 1 ADC Command output enable */
		vuint32_t : 1;
		vuint32_t T0_E : 1;		 /* Trigger 0 enable */
		vuint32_t T0_ETE : 1;	 /* Trigger 0 Ext Trigger output enable */
		vuint32_t T0_T4E : 1;	 /* Trigger 0 Timer4 output enable */
		vuint32_t T0_T3E : 1;	 /* Trigger 0 Timer3 output enable */
		vuint32_t T0_T2E : 1;	 /* Trigger 0 Timer2 output enable */
		vuint32_t T0_T1E : 1;	 /* Trigger 0 Timer1 output enable */
		vuint32_t T0_ADCE : 1; /* Trigger 0 ADC Command output enable */
	} B;
} CTU_THCR1_32B_tag;

typedef union { /* Trigger Handler Control Register 2 */
	vuint32_t R;
	struct {
		vuint32_t : 1;
		vuint32_t T7_E : 1;		 /* Trigger 7 enable */
		vuint32_t T7_ETE : 1;	 /* Trigger 7 Ext Trigger output enable */
		vuint32_t T7_T4E : 1;	 /* Trigger 7 Timer4 output enable */
		vuint32_t T7_T3E : 1;	 /* Trigger 7 Timer3 output enable */
		vuint32_t T7_T2E : 1;	 /* Trigger 7 Timer2 output enable */
		vuint32_t T7_T1E : 1;	 /* Trigger 7 Timer1 output enable */
		vuint32_t T7_ADCE : 1; /* Trigger 7 ADC Command output enable */
		vuint32_t : 1;
		vuint32_t T6_E : 1;		 /* Trigger 6 enable */
		vuint32_t T6_ETE : 1;	 /* Trigger 6 Ext Trigger output enable */
		vuint32_t T6_T4E : 1;	 /* Trigger 6 Timer4 output enable */
		vuint32_t T6_T3E : 1;	 /* Trigger 6 Timer3 output enable */
		vuint32_t T6_T2E : 1;	 /* Trigger 6 Timer2 output enable */
		vuint32_t T6_T1E : 1;	 /* Trigger 6 Timer1 output enable */
		vuint32_t T6_ADCE : 1; /* Trigger 6 ADC Command output enable */
		vuint32_t : 1;
		vuint32_t T5_E : 1;		 /* Trigger 5 enable */
		vuint32_t T5_ETE : 1;	 /* Trigger 5 Ext Trigger output enable */
		vuint32_t T5_T4E : 1;	 /* Trigger 5 Timer4 output enable */
		vuint32_t T5_T3E : 1;	 /* Trigger 5 Timer3 output enable */
		vuint32_t T5_T2E : 1;	 /* Trigger 5 Timer2 output enable */
		vuint32_t T5_T1E : 1;	 /* Trigger 5 Timer1 output enable */
		vuint32_t T5_ADCE : 1; /* Trigger 5 ADC Command output enable */
		vuint32_t : 1;
		vuint32_t T4_E : 1;		 /* Trigger 4 enable */
		vuint32_t T4_ETE : 1;	 /* Trigger 4 Ext Trigger output enable */
		vuint32_t T4_T4E : 1;	 /* Trigger 4 Timer4 output enable */
		vuint32_t T4_T3E : 1;	 /* Trigger 4 Timer3 output enable */
		vuint32_t T4_T2E : 1;	 /* Trigger 4 Timer2 output enable */
		vuint32_t T4_T1E : 1;	 /* Trigger 4 Timer1 output enable */
		vuint32_t T4_ADCE : 1; /* Trigger 4 ADC Command output enable */
	} B;
} CTU_THCR2_32B_tag;

/* Register layout for all registers CLR_DCM... */

typedef union { /* Command List Register. View: (CMS=BIT13=)ST1 = 1, (BIT9=)ST0
									 = DONT CARE */
	vuint16_t R;
	struct {
		vuint16_t CIR : 1;	/* Command Interrupt Request */
		vuint16_t LC : 1;		/* Last Command */
		vuint16_t CMS : 1;	/* Conversion Mode Selection */
		vuint16_t FIFO : 3; /* FIFO for ADC A/B */
		vuint16_t : 1;
		vuint16_t CHB : 4; /* ADC unit B channel number */
		vuint16_t : 1;
		vuint16_t CHA : 4; /* ADC unit A channel number */
	} B;
} CTU_CLR_DCM_16B_tag;

/* Register layout for all registers CLR_SCM... */

typedef union { /* Command List Register. View: (CMS=BIT13=)ST1 = 0, (BIT9=)ST0
									 = 0 */
	vuint16_t R;
	struct {
		vuint16_t CIR : 1;	/* Command Interrupt Request */
		vuint16_t LC : 1;		/* Last Command */
		vuint16_t CMS : 1;	/* Conversion Mode Selection */
		vuint16_t FIFO : 3; /* FIFO for ADC A/B */
		vuint16_t : 4;
		vuint16_t SU : 1; /* Selection ADC Unit */
		vuint16_t : 1;
		vuint16_t CH : 4; /* ADC unit channel number */
	} B;
} CTU_CLR_SCM_16B_tag;

/* Register layout for all registers CLR... */

typedef union { /* Control Register */
	vuint16_t R;
	struct {
		vuint16_t EMPTY_CLR7 : 1; /* Empty Clear 7 */
		vuint16_t EMPTY_CLR6 : 1; /* Empty Clear 6 */
		vuint16_t EMPTY_CLR5 : 1; /* Empty Clear 5 */
		vuint16_t EMPTY_CLR4 : 1; /* Empty Clear 4 */
		vuint16_t EMPTY_CLR3 : 1; /* Empty Clear 3 */
		vuint16_t EMPTY_CLR2 : 1; /* Empty Clear 2 */
		vuint16_t EMPTY_CLR1 : 1; /* Empty Clear 1 */
		vuint16_t EMPTY_CLR0 : 1; /* Empty Clear 0 */
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t DMA_EN7 : 1; /* Enable DMA interface for FIFO 7 */
#else
		vuint16_t DMAEN7 : 1;			/* Enable DMA interface for FIFO 7 */
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t DMA_EN6 : 1; /* Enable DMA interface for FIFO 6 */
#else
		vuint16_t DMAEN6 : 1;			/* Enable DMA interface for FIFO 6 */
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t DMA_EN5 : 1; /* Enable DMA interface for FIFO 5 */
#else
		vuint16_t DMAEN5 : 1;			/* Enable DMA interface for FIFO 5 */
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t DMA_EN4 : 1; /* Enable DMA interface for FIFO 4 */
#else
		vuint16_t DMAEN4 : 1;			/* Enable DMA interface for FIFO 4 */
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t DMA_EN3 : 1; /* Enable DMA interface for FIFO 3 */
#else
		vuint16_t DMAEN3 : 1;			/* Enable DMA interface for FIFO 3 */
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t DMA_EN2 : 1; /* Enable DMA interface for FIFO 2 */
#else
		vuint16_t DMAEN2 : 1;			/* Enable DMA interface for FIFO 2 */
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t DMA_EN1 : 1; /* Enable DMA interface for FIFO 1 */
#else
		vuint16_t DMAEN1 : 1;			/* Enable DMA interface for FIFO 1 */
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t DMA_EN0 : 1; /* Enable DMA interface for FIFO 0 */
#else
		vuint16_t DMAEN0 : 1;			/* Enable DMA interface for FIFO 0 */
#endif
	} B;
} CTU_CR_16B_tag;

typedef union { /* Control Register FIFO */
	vuint32_t R;
	struct {
		vuint32_t FIFO_OVERRUN_EN7 : 1;	 /* FIFO 7 OVERRUN  Enable Interrupt */
		vuint32_t FIFO_OVERFLOW_EN7 : 1; /* FIFO 7 OVERFLOW Enable Interrupt */
		vuint32_t FIFO_EMPTY_EN7 : 1;		 /* FIFO 7 EMPTY Enable Interrupt */
		vuint32_t FIFO_FULL_EN7 : 1;		 /* FIFO 7 FULL Enable Interrupt */
		vuint32_t FIFO_OVERRUN_EN6 : 1;	 /* FIFO 6 OVERRUN  Enable Interrupt */
		vuint32_t FIFO_OVERFLOW_EN6 : 1; /* FIFO 6 OVERFLOW Enable Interrupt */
		vuint32_t FIFO_EMPTY_EN6 : 1;		 /* FIFO 6 EMPTY Enable Interrupt */
		vuint32_t FIFO_FULL_EN6 : 1;		 /* FIFO 6 FULL Enable Interrupt */
		vuint32_t FIFO_OVERRUN_EN5 : 1;	 /* FIFO 5 OVERRUN  Enable Interrupt */
		vuint32_t FIFO_OVERFLOW_EN5 : 1; /* FIFO 5 OVERFLOW Enable Interrupt */
		vuint32_t FIFO_EMPTY_EN5 : 1;		 /* FIFO 5 EMPTY Enable Interrupt */
		vuint32_t FIFO_FULL_EN5 : 1;		 /* FIFO 5 FULL Enable Interrupt */
		vuint32_t FIFO_OVERRUN_EN4 : 1;	 /* FIFO 4 OVERRUN  Enable Interrupt */
		vuint32_t FIFO_OVERFLOW_EN4 : 1; /* FIFO 4 OVERFLOW Enable Interrupt */
		vuint32_t FIFO_EMPTY_EN4 : 1;		 /* FIFO 4 EMPTY Enable Interrupt */
		vuint32_t FIFO_FULL_EN4 : 1;		 /* FIFO 4 FULL Enable Interrupt */
		vuint32_t FIFO_OVERRUN_EN3 : 1;	 /* FIFO 3 OVERRUN  Enable Interrupt */
		vuint32_t FIFO_OVERFLOW_EN3 : 1; /* FIFO 3 OVERFLOW Enable Interrupt */
		vuint32_t FIFO_EMPTY_EN3 : 1;		 /* FIFO 3 EMPTY Enable Interrupt */
		vuint32_t FIFO_FULL_EN3 : 1;		 /* FIFO 3 FULL Enable Interrupt */
		vuint32_t FIFO_OVERRUN_EN2 : 1;	 /* FIFO 2 OVERRUN  Enable Interrupt */
		vuint32_t FIFO_OVERFLOW_EN2 : 1; /* FIFO 2 OVERFLOW Enable Interrupt */
		vuint32_t FIFO_EMPTY_EN2 : 1;		 /* FIFO 2 EMPTY Enable Interrupt */
		vuint32_t FIFO_FULL_EN2 : 1;		 /* FIFO 2 FULL Enable Interrupt */
		vuint32_t FIFO_OVERRUN_EN1 : 1;	 /* FIFO 1 OVERRUN  Enable Interrupt */
		vuint32_t FIFO_OVERFLOW_EN1 : 1; /* FIFO 1 OVERFLOW Enable Interrupt */
		vuint32_t FIFO_EMPTY_EN1 : 1;		 /* FIFO 1 EMPTY Enable Interrupt */
		vuint32_t FIFO_FULL_EN1 : 1;		 /* FIFO 1 FULL Enable Interrupt */
		vuint32_t FIFO_OVERRUN_EN0 : 1;	 /* FIFO 0 OVERRUN  Enable Interrupt */
		vuint32_t FIFO_OVERFLOW_EN0 : 1; /* FIFO 0 OVERFLOW Enable Interrupt */
		vuint32_t FIFO_EMPTY_EN0 : 1;		 /* FIFO 0 EMPTY Enable Interrupt */
		vuint32_t FIFO_FULL_EN0 : 1;		 /* FIFO 0 FULL Enable Interrupt */
	} B;
} CTU_FCR_32B_tag;

typedef union { /* Threshold 1 Register */
	vuint32_t R;
	struct {
#ifndef USE_FIELD_ALIASES_CTU
		vuint32_t TRESHOLD3 : 8; /* Threshlod FIFO 3 */
#else
		vuint32_t THRESHOLD3 : 8; /* Threshlod FIFO 3 */
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint32_t TRESHOLD2 : 8; /* Threshlod FIFO 2 */
#else
		vuint32_t THRESHOLD2 : 8; /* Threshlod FIFO 2 */
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint32_t TRESHOLD1 : 8; /* Threshlod FIFO 1 */
#else
		vuint32_t THRESHOLD1 : 8; /* Threshlod FIFO 1 */
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint32_t TRESHOLD0 : 8; /* Threshlod FIFO 0 */
#else
		vuint32_t THRESHOLD0 : 8; /* Threshlod FIFO 0 */
#endif
	} B;
} CTU_TH1_32B_tag;

typedef union { /* Threshold 2 Register */
	vuint32_t R;
	struct {
#ifndef USE_FIELD_ALIASES_CTU
		vuint32_t TRESHOLD7 : 8; /* Threshlod FIFO 7 */
#else
		vuint32_t THRESHOLD7 : 8; /* Threshlod FIFO 7 */
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint32_t TRESHOLD6 : 8; /* Threshlod FIFO 6 */
#else
		vuint32_t THRESHOLD6 : 8; /* Threshlod FIFO 6 */
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint32_t TRESHOLD5 : 8; /* Threshlod FIFO 5 */
#else
		vuint32_t THRESHOLD5 : 8; /* Threshlod FIFO 5 */
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint32_t TRESHOLD4 : 8; /* Threshlod FIFO 4 */
#else
		vuint32_t THRESHOLD4 : 8; /* Threshlod FIFO 4 */
#endif
	} B;
} CTU_TH2_32B_tag;

typedef union { /* Status Register */
	vuint32_t R;
	struct {
		vuint32_t FIFO_OVERRUN7 : 1;	/* FIFO 7 OVERRUN  Flag */
		vuint32_t FIFO_OVERFLOW7 : 1; /* FIFO 7 OVERFLOW Flag */
		vuint32_t FIFO_EMPTY7 : 1;		/* FIFO 7 EMPTY Flag */
		vuint32_t FIFO_FULL7 : 1;			/* FIFO 7 FULL Flag */
		vuint32_t FIFO_OVERRUN6 : 1;	/* FIFO 6 OVERRUN  Flag */
		vuint32_t FIFO_OVERFLOW6 : 1; /* FIFO 6 OVERFLOW Flag */
		vuint32_t FIFO_EMPTY6 : 1;		/* FIFO 6 EMPTY Flag */
		vuint32_t FIFO_FULL6 : 1;			/* FIFO 6 FULL Flag */
		vuint32_t FIFO_OVERRUN5 : 1;	/* FIFO 5 OVERRUN  Flag */
		vuint32_t FIFO_OVERFLOW5 : 1; /* FIFO 5 OVERFLOW Flag */
		vuint32_t FIFO_EMPTY5 : 1;		/* FIFO 5 EMPTY Flag */
		vuint32_t FIFO_FULL5 : 1;			/* FIFO 5 FULL Flag */
		vuint32_t FIFO_OVERRUN4 : 1;	/* FIFO 4 OVERRUN  Flag */
		vuint32_t FIFO_OVERFLOW4 : 1; /* FIFO 4 OVERFLOW Flag */
		vuint32_t FIFO_EMPTY4 : 1;		/* FIFO 4 EMPTY Flag */
		vuint32_t FIFO_FULL4 : 1;			/* FIFO 4 FULL Flag */
		vuint32_t FIFO_OVERRUN3 : 1;	/* FIFO 3 OVERRUN  Flag */
		vuint32_t FIFO_OVERFLOW3 : 1; /* FIFO 3 OVERFLOW Flag */
		vuint32_t FIFO_EMPTY3 : 1;		/* FIFO 3 EMPTY Flag */
		vuint32_t FIFO_FULL3 : 1;			/* FIFO 3 FULL Flag */
		vuint32_t FIFO_OVERRUN2 : 1;	/* FIFO 2 OVERRUN  Flag */
		vuint32_t FIFO_OVERFLOW2 : 1; /* FIFO 2 OVERFLOW Flag */
		vuint32_t FIFO_EMPTY2 : 1;		/* FIFO 2 EMPTY Flag */
		vuint32_t FIFO_FULL2 : 1;			/* FIFO 2 FULL Flag */
		vuint32_t FIFO_OVERRUN1 : 1;	/* FIFO 1 OVERRUN  Flag */
		vuint32_t FIFO_OVERFLOW1 : 1; /* FIFO 1 OVERFLOW Flag */
		vuint32_t FIFO_EMPTY1 : 1;		/* FIFO 1 EMPTY Flag */
		vuint32_t FIFO_FULL1 : 1;			/* FIFO 1 FULL Flag */
		vuint32_t FIFO_OVERRUN0 : 1;	/* FIFO 0 OVERRUN  Flag */
		vuint32_t FIFO_OVERFLOW0 : 1; /* FIFO 0 OVERFLOW Flag */
		vuint32_t FIFO_EMPTY0 : 1;		/* FIFO 0 EMPTY Flag */
		vuint32_t FIFO_FULL0 : 1;			/* FIFO 0 FULL Flag */
	} B;
} CTU_STS_32B_tag;

/* Register layout for all registers FR... */

typedef union { /* FIFO Right Aligned register */
	vuint32_t R;
	struct {
		vuint32_t : 11;
		vuint32_t ADC : 1;	/* ADC Unit */
		vuint32_t N_CH : 4; /* Number Channel */
		vuint32_t : 4;
		vuint32_t DATA : 12; /* Data Fifo */
	} B;
} CTU_FR_32B_tag;

/* Register layout for all registers FL... */

typedef union { /* FIFO Left Aligned register */
	vuint32_t R;
	struct {
		vuint32_t : 11;
		vuint32_t ADC : 1;	/* ADC Unit */
		vuint32_t N_CH : 4; /* Number Channel */
		vuint32_t : 1;
		vuint32_t DATA : 12; /* Data Fifo */
		vuint32_t : 3;
	} B;
} CTU_FL_32B_tag;

typedef union { /* CTU Error Flag Register */
	vuint16_t R;
	struct {
		vuint16_t : 3;
		vuint16_t CS : 1;			 /* Counter Status */
		vuint16_t ET_OE : 1;	 /* ExtTrigger Generation Overrun */
		vuint16_t ERR_CMP : 1; /* Set if counter reaches TGSCCR register */
		vuint16_t T4_OE : 1;	 /* Timer4 Generation Overrun */
		vuint16_t T3_OE : 1;	 /* Timer3 Generation Overrun */
		vuint16_t T2_OE : 1;	 /* Timer2 Generation Overrun */
		vuint16_t T1_OE : 1;	 /* Timer1 Generation Overrun */
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t ADC_OE : 1; /* ADC Command Generation Overrun */
#else
		vuint16_t ADCOE : 1;			/* ADC Command Generation Overrun */
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t TGS_OSM : 1; /* TGS Overrun */
#else
		vuint16_t TGSOSM : 1;			/* TGS Overrun */
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t MRS_O : 1; /* MRS Overrun */
#else
		vuint16_t MRSO : 1;				/* TGS Overrun */
#endif
		vuint16_t ICE : 1; /* Invalid Command Error */
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t SM_TO : 1; /* Trigger Overrun */
#else
		vuint16_t SMTO : 1;				/* Trigger Overrun */
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t MRS_RE : 1; /* MRS Reload Error */
#else
		vuint16_t MRSRE : 1;			/* MRS Reload Error */
#endif
	} B;
} CTU_CTUEFR_16B_tag;

typedef union { /* CTU Interrupt Flag Register */
	vuint16_t R;
	struct {
		vuint16_t : 4;
		vuint16_t S_E_B : 1; /* Slice time OK */
		vuint16_t S_E_A : 1; /* Slice time OK */
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t ADC_I : 1; /* ADC Command Interrupt Flag */
#else
		vuint16_t ADC : 1;
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t T7_I : 1; /* Trigger 7  Interrupt Flag */
#else
		vuint16_t T7 : 1;
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t T6_I : 1; /* Trigger 6  Interrupt Flag */
#else
		vuint16_t T6 : 1;
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t T5_I : 1; /* Trigger 5  Interrupt Flag */
#else
		vuint16_t T5 : 1;
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t T4_I : 1; /* Trigger 4  Interrupt Flag */
#else
		vuint16_t T4 : 1;
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t T3_I : 1; /* Trigger 3  Interrupt Flag */
#else
		vuint16_t T3 : 1;
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t T2_I : 1; /* Trigger 2  Interrupt Flag */
#else
		vuint16_t T2 : 1;
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t T1_I : 1; /* Trigger 1  Interrupt Flag */
#else
		vuint16_t T1 : 1;
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t T0_I : 1; /* Trigger 0  Interrupt Flag */
#else
		vuint16_t T0 : 1;
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t MRS_I : 1; /* MRS Interrupt Flag */
#else
		vuint16_t MRS : 1;
#endif
	} B;
} CTU_CTUIFR_16B_tag;

typedef union { /* CTU Interrupt/DMA Register */
	vuint16_t R;
	struct {
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t T7_I : 1; /* Trigger 7  Interrupt Enable */
#else
		vuint16_t T7IE : 1;
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t T6_I : 1; /* Trigger 6  Interrupt Enable */
#else
		vuint16_t T6IE : 1;
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t T5_I : 1; /* Trigger 5  Interrupt Enable */
#else
		vuint16_t T5IE : 1;
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t T4_I : 1; /* Trigger 4  Interrupt Enable */
#else
		vuint16_t T4IE : 1;
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t T3_I : 1; /* Trigger 3  Interrupt Enable */
#else
		vuint16_t T3IE : 1;
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t T2_I : 1; /* Trigger 2  Interrupt Enable */
#else
		vuint16_t T2IE : 1;
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t T1_I : 1; /* Trigger 1  Interrupt Enable */
#else
		vuint16_t T1IE : 1;
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t T0_I : 1; /* Trigger 0  Interrupt Enable */
#else
		vuint16_t T0IE : 1;
#endif
		vuint16_t : 2;
		vuint16_t SAF_CNT_B_EN : 1; /* Conversion time counter enabled */
		vuint16_t SAF_CNT_A_EN : 1; /* Conversion time counter enabled */
		vuint16_t DMA_DE : 1;				/* DMA and gre bit */
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t MRS_DMAE : 1; /* DMA Transfer Enable */
#else
		vuint16_t MRSDMAE : 1;
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t MRS_IE : 1; /* MRS Interrupt Enable */
#else
		vuint16_t MRSIE : 1;
#endif
		vuint16_t IEE : 1; /* Interrupt Error Enable */
	} B;
} CTU_CTUIR_16B_tag;

typedef union { /* Control On-Time Register */
	vuint16_t R;
	struct {
		vuint16_t : 8;
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t COTR_COTR : 8; /* Control On-Time Register and Guard Time */
#else
		vuint16_t COTR : 8;
#endif
	} B;
} CTU_COTR_16B_tag;

typedef union { /* CTU Control Register */
	vuint16_t R;
	struct {
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t T7_SG : 1; /* Trigger 7 Software Generated */
#else
		vuint16_t T7SG : 1;
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t T6_SG : 1; /* Trigger 6 Software Generated */
#else
		vuint16_t T6SG : 1;
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t T5_SG : 1; /* Trigger 5 Software Generated */
#else
		vuint16_t T5SG : 1;
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t T4_SG : 1; /* Trigger 4 Software Generated */
#else
		vuint16_t T4SG : 1;
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t T3_SG : 1; /* Trigger 3 Software Generated */
#else
		vuint16_t T3SG : 1;
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t T2_SG : 1; /* Trigger 2 Software Generated */
#else
		vuint16_t T2SG : 1;
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t T1_SG : 1; /* Trigger 1 Software Generated */
#else
		vuint16_t T1SG : 1;
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t T0_SG : 1; /* Trigger 0 Software Generated */
#else
		vuint16_t T0SG : 1;
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t CTU_ADC_RESET : 1; /* CTU ADC State Machine Reset */
#else
		vuint16_t CTUADCRESET : 1;
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t CTU_ODIS : 1; /* CTU Output Disable */
#else
		vuint16_t CTUODIS : 1;
#endif
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t FILTER_EN : 1; /* Synchronize Filter Register value */
#else
		vuint16_t FILTERENABLE : 1;
#endif
		vuint16_t CGRE : 1; /* Clear GRE */
		vuint16_t FGRE : 1; /* GRE Flag */
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t MRS_SG : 1; /* MRS Software Generated */
#else
		vuint16_t MRSSG : 1;
#endif
		vuint16_t GRE : 1; /* General Reload Enable */
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t TGSISR_RE : 1; /* TGSISR Reload Enable */
#else
		vuint16_t TGSISRRE : 1;
#endif
	} B;
} CTU_CTUCR_16B_tag;

typedef union { /* CTU Digital Filter Register */
	vuint16_t R;
	struct {
		vuint16_t : 8;
#ifndef USE_FIELD_ALIASES_CTU
		vuint16_t FILTER_VALUE : 8; /* Filter Value */
#else
		vuint16_t FILTERVALUE : 8; /* deprecated name - please avoid */
#endif
	} B;
} CTU_FILTER_16B_tag;

typedef union { /* CTU Expected A Value Register */
	vuint16_t R;
	struct {
		vuint16_t EXPECTED_A_VALUE : 16; /* Expected A Value */
	} B;
} CTU_EXPECTED_A_16B_tag;

typedef union { /* CTU Expected B Value Register */
	vuint16_t R;
	struct {
		vuint16_t EXPECTED_B_VALUE : 16; /* Expected B Value */
	} B;
} CTU_EXPECTED_B_16B_tag;

typedef union { /* CTU Counter Range Register */
	vuint16_t R;
	struct {
		vuint16_t : 8;
		vuint16_t CNT_RANGE_VALUE : 8; /* Counter Range Value */
	} B;
} CTU_CNT_RANGE_16B_tag;

/* Register layout for generated register(s) FRA... */

typedef union { /*  */
	vuint32_t R;
} CTU_FRA_32B_tag;

/* Register layout for generated register(s) FLA... */

typedef union { /*  */
	vuint32_t R;
} CTU_FLA_32B_tag;

typedef struct CTU_struct_tag { /* start of CTU_tag */
	/* Trigger Generator Subunit Input Selection register */
	CTU_TGSISR_32B_tag TGSISR; /* offset: 0x0000 size: 32 bit */
														 /* Trigger Generator Subunit Control Register */
	CTU_TGSCR_16B_tag TGSCR;	 /* offset: 0x0004 size: 16 bit */
	union {
		CTU_TCR_16B_tag TCR[8]; /* offset: 0x0006  (0x0002 x 8) */

		struct {
			CTU_TCR_16B_tag T0CR; /* offset: 0x0006 size: 16 bit */
			CTU_TCR_16B_tag T1CR; /* offset: 0x0008 size: 16 bit */
			CTU_TCR_16B_tag T2CR; /* offset: 0x000A size: 16 bit */
			CTU_TCR_16B_tag T3CR; /* offset: 0x000C size: 16 bit */
			CTU_TCR_16B_tag T4CR; /* offset: 0x000E size: 16 bit */
			CTU_TCR_16B_tag T5CR; /* offset: 0x0010 size: 16 bit */
			CTU_TCR_16B_tag T6CR; /* offset: 0x0012 size: 16 bit */
			CTU_TCR_16B_tag T7CR; /* offset: 0x0014 size: 16 bit */
		};
	};
	/* TGS Counter Compare Register */
	CTU_TGSCCR_16B_tag TGSCCR; /* offset: 0x0016 size: 16 bit */
														 /* TGS Counter Reload Register */
	CTU_TGSCRR_16B_tag TGSCRR; /* offset: 0x0018 size: 16 bit */
	int8_t CTU_reserved_001A[2];
	/* Commands List Control Register 1 */
	CTU_CLCR1_32B_tag CLCR1; /* offset: 0x001C size: 32 bit */
													 /* Commands List Control Register 2 */
	CTU_CLCR2_32B_tag CLCR2; /* offset: 0x0020 size: 32 bit */
													 /* Trigger Handler Control Register 1 */
	CTU_THCR1_32B_tag THCR1; /* offset: 0x0024 size: 32 bit */
													 /* Trigger Handler Control Register 2 */
	CTU_THCR2_32B_tag THCR2; /* offset: 0x0028 size: 32 bit */
	union {
		/* Command List Register. View: BIT13, BIT9 */
		CTU_CLR_SCM_16B_tag CLR[24];
				/* offset: 0x002C  (0x0002 x 24) */ /* deprecated name - please avoid */

		/* Command List Register. View: (CMS=BIT13=)ST1 = 0, (BIT9=)ST0 = 0 */
		CTU_CLR_SCM_16B_tag CLR_SCM[24]; /* offset: 0x002C  (0x0002 x 24) */

		/* Command List Register. View: (CMS=BIT13=)ST1 = 1, (BIT9=)ST0 = DONT CARE
		 */
		CTU_CLR_DCM_16B_tag CLR_DCM[24]; /* offset: 0x002C  (0x0002 x 24) */

		struct {
			/* Command List Register. View: (CMS=BIT13=)ST1 = 0, (BIT9=)ST0 = 0 */
			CTU_CLR_SCM_16B_tag CLR_SCM1;	 /* offset: 0x002C size: 16 bit */
			CTU_CLR_SCM_16B_tag CLR_SCM2;	 /* offset: 0x002E size: 16 bit */
			CTU_CLR_SCM_16B_tag CLR_SCM3;	 /* offset: 0x0030 size: 16 bit */
			CTU_CLR_SCM_16B_tag CLR_SCM4;	 /* offset: 0x0032 size: 16 bit */
			CTU_CLR_SCM_16B_tag CLR_SCM5;	 /* offset: 0x0034 size: 16 bit */
			CTU_CLR_SCM_16B_tag CLR_SCM6;	 /* offset: 0x0036 size: 16 bit */
			CTU_CLR_SCM_16B_tag CLR_SCM7;	 /* offset: 0x0038 size: 16 bit */
			CTU_CLR_SCM_16B_tag CLR_SCM8;	 /* offset: 0x003A size: 16 bit */
			CTU_CLR_SCM_16B_tag CLR_SCM9;	 /* offset: 0x003C size: 16 bit */
			CTU_CLR_SCM_16B_tag CLR_SCM10; /* offset: 0x003E size: 16 bit */
			CTU_CLR_SCM_16B_tag CLR_SCM11; /* offset: 0x0040 size: 16 bit */
			CTU_CLR_SCM_16B_tag CLR_SCM12; /* offset: 0x0042 size: 16 bit */
			CTU_CLR_SCM_16B_tag CLR_SCM13; /* offset: 0x0044 size: 16 bit */
			CTU_CLR_SCM_16B_tag CLR_SCM14; /* offset: 0x0046 size: 16 bit */
			CTU_CLR_SCM_16B_tag CLR_SCM15; /* offset: 0x0048 size: 16 bit */
			CTU_CLR_SCM_16B_tag CLR_SCM16; /* offset: 0x004A size: 16 bit */
			CTU_CLR_SCM_16B_tag CLR_SCM17; /* offset: 0x004C size: 16 bit */
			CTU_CLR_SCM_16B_tag CLR_SCM18; /* offset: 0x004E size: 16 bit */
			CTU_CLR_SCM_16B_tag CLR_SCM19; /* offset: 0x0050 size: 16 bit */
			CTU_CLR_SCM_16B_tag CLR_SCM20; /* offset: 0x0052 size: 16 bit */
			CTU_CLR_SCM_16B_tag CLR_SCM21; /* offset: 0x0054 size: 16 bit */
			CTU_CLR_SCM_16B_tag CLR_SCM22; /* offset: 0x0056 size: 16 bit */
			CTU_CLR_SCM_16B_tag CLR_SCM23; /* offset: 0x0058 size: 16 bit */
			CTU_CLR_SCM_16B_tag CLR_SCM24; /* offset: 0x005A size: 16 bit */
		};

		struct {
			/* Command List Register. View: (CMS=BIT13=)ST1 = 1, (BIT9=)ST0 = DONT
			 * CARE */
			CTU_CLR_DCM_16B_tag CLR_DCM1;	 /* offset: 0x002C size: 16 bit */
			CTU_CLR_DCM_16B_tag CLR_DCM2;	 /* offset: 0x002E size: 16 bit */
			CTU_CLR_DCM_16B_tag CLR_DCM3;	 /* offset: 0x0030 size: 16 bit */
			CTU_CLR_DCM_16B_tag CLR_DCM4;	 /* offset: 0x0032 size: 16 bit */
			CTU_CLR_DCM_16B_tag CLR_DCM5;	 /* offset: 0x0034 size: 16 bit */
			CTU_CLR_DCM_16B_tag CLR_DCM6;	 /* offset: 0x0036 size: 16 bit */
			CTU_CLR_DCM_16B_tag CLR_DCM7;	 /* offset: 0x0038 size: 16 bit */
			CTU_CLR_DCM_16B_tag CLR_DCM8;	 /* offset: 0x003A size: 16 bit */
			CTU_CLR_DCM_16B_tag CLR_DCM9;	 /* offset: 0x003C size: 16 bit */
			CTU_CLR_DCM_16B_tag CLR_DCM10; /* offset: 0x003E size: 16 bit */
			CTU_CLR_DCM_16B_tag CLR_DCM11; /* offset: 0x0040 size: 16 bit */
			CTU_CLR_DCM_16B_tag CLR_DCM12; /* offset: 0x0042 size: 16 bit */
			CTU_CLR_DCM_16B_tag CLR_DCM13; /* offset: 0x0044 size: 16 bit */
			CTU_CLR_DCM_16B_tag CLR_DCM14; /* offset: 0x0046 size: 16 bit */
			CTU_CLR_DCM_16B_tag CLR_DCM15; /* offset: 0x0048 size: 16 bit */
			CTU_CLR_DCM_16B_tag CLR_DCM16; /* offset: 0x004A size: 16 bit */
			CTU_CLR_DCM_16B_tag CLR_DCM17; /* offset: 0x004C size: 16 bit */
			CTU_CLR_DCM_16B_tag CLR_DCM18; /* offset: 0x004E size: 16 bit */
			CTU_CLR_DCM_16B_tag CLR_DCM19; /* offset: 0x0050 size: 16 bit */
			CTU_CLR_DCM_16B_tag CLR_DCM20; /* offset: 0x0052 size: 16 bit */
			CTU_CLR_DCM_16B_tag CLR_DCM21; /* offset: 0x0054 size: 16 bit */
			CTU_CLR_DCM_16B_tag CLR_DCM22; /* offset: 0x0056 size: 16 bit */
			CTU_CLR_DCM_16B_tag CLR_DCM23; /* offset: 0x0058 size: 16 bit */
			CTU_CLR_DCM_16B_tag CLR_DCM24; /* offset: 0x005A size: 16 bit */
		};
	};
	int8_t CTU_reserved_005C[16];
	/* Control Register */
	CTU_CR_16B_tag CR; /* offset: 0x006C size: 16 bit */
	int8_t CTU_reserved_006E[2];
	/* Control Register FIFO */
	CTU_FCR_32B_tag FCR; /* offset: 0x0070 size: 32 bit */
											 /* Threshold 1 Register */
	CTU_TH1_32B_tag TH1; /* offset: 0x0074 size: 32 bit */
											 /* Threshold 2 Register */
	CTU_TH2_32B_tag TH2; /* offset: 0x0078 size: 32 bit */
	union {
		/* Status Register */
		CTU_STS_32B_tag STS; /* offset: 0x007C size: 32 bit */

		CTU_STS_32B_tag STATUS; /* deprecated - please avoid */
	};
	union {
		CTU_FRA_32B_tag FRA[8]; /* offset: 0x0080  (0x0004 x 8) */

		/* FIFO Right Aligned register */
		CTU_FR_32B_tag FR[8]; /* offset: 0x0080  (0x0004 x 8) */

		struct {
			/* FIFO Right Aligned register */
			CTU_FR_32B_tag FR0; /* offset: 0x0080 size: 32 bit */
			CTU_FR_32B_tag FR1; /* offset: 0x0084 size: 32 bit */
			CTU_FR_32B_tag FR2; /* offset: 0x0088 size: 32 bit */
			CTU_FR_32B_tag FR3; /* offset: 0x008C size: 32 bit */
			CTU_FR_32B_tag FR4; /* offset: 0x0090 size: 32 bit */
			CTU_FR_32B_tag FR5; /* offset: 0x0094 size: 32 bit */
			CTU_FR_32B_tag FR6; /* offset: 0x0098 size: 32 bit */
			CTU_FR_32B_tag FR7; /* offset: 0x009C size: 32 bit */
		};
	};
	union {
		CTU_FLA_32B_tag FLA[8]; /* offset: 0x00A0  (0x0004 x 8) */

		/* FIFO Left Aligned register */
		CTU_FL_32B_tag FL[8]; /* offset: 0x00A0  (0x0004 x 8) */

		struct {
			/* FIFO Left Aligned register */
			CTU_FL_32B_tag FL0; /* offset: 0x00A0 size: 32 bit */
			CTU_FL_32B_tag FL1; /* offset: 0x00A4 size: 32 bit */
			CTU_FL_32B_tag FL2; /* offset: 0x00A8 size: 32 bit */
			CTU_FL_32B_tag FL3; /* offset: 0x00AC size: 32 bit */
			CTU_FL_32B_tag FL4; /* offset: 0x00B0 size: 32 bit */
			CTU_FL_32B_tag FL5; /* offset: 0x00B4 size: 32 bit */
			CTU_FL_32B_tag FL6; /* offset: 0x00B8 size: 32 bit */
			CTU_FL_32B_tag FL7; /* offset: 0x00BC size: 32 bit */
		};
	};
	/* CTU Error Flag Register */
	CTU_CTUEFR_16B_tag CTUEFR; /* offset: 0x00C0 size: 16 bit */
														 /* CTU Interrupt Flag Register */
	CTU_CTUIFR_16B_tag CTUIFR; /* offset: 0x00C2 size: 16 bit */
														 /* CTU Interrupt/DMA Register */
	CTU_CTUIR_16B_tag CTUIR;	 /* offset: 0x00C4 size: 16 bit */
														 /* Control On-Time Register */
	CTU_COTR_16B_tag COTR;		 /* offset: 0x00C6 size: 16 bit */
	/* CTU Control Register */
	CTU_CTUCR_16B_tag CTUCR; /* offset: 0x00C8 size: 16 bit */
	union {
		/* CTU Digital Filter Register */
		CTU_FILTER_16B_tag FILTER; /* offset: 0x00CA size: 16 bit */

		CTU_FILTER_16B_tag CTUFILTER; /* deprecated - please avoid */
	};
	/* CTU Expected A Value Register */
	CTU_EXPECTED_A_16B_tag EXPECTED_A; /* offset: 0x00CC size: 16 bit */

	/* CTU Expected B Value Register */
	CTU_EXPECTED_B_16B_tag EXPECTED_B; /* offset: 0x00CE size: 16 bit */
																		 /* CTU Counter Range Register */
	CTU_CNT_RANGE_16B_tag CNT_RANGE;	 /* offset: 0x00D0 size: 16 bit */
} CTU_tag;

#define CTU (*(volatile CTU_tag*)0xFFE0C000UL)

/****************************************************************/
/*                                                              */
/* Module: mcTIMER  */
/*                                                              */
/****************************************************************/

/* Register layout for all registers COMP1... */

typedef union { /* Compare Register 1 */
	vuint16_t R;
	struct {
		vuint16_t COMP1 : 16; /* deprecated definition -- do not use */
	} B;
} mcTIMER_COMP1_16B_tag;

/* Register layout for all registers COMP2... */

typedef union { /* Compare Register 2 */
	vuint16_t R;
	struct {
		vuint16_t COMP2 : 16; /* deprecated definition -- do not use */
	} B;
} mcTIMER_COMP2_16B_tag;

/* Register layout for all registers CAPT1... */

typedef union { /* Capture Register 1 */
	vuint16_t R;
	struct {
		vuint16_t CAPT1 : 16; /* deprecated definition -- do not use */
	} B;
} mcTIMER_CAPT1_16B_tag;

/* Register layout for all registers CAPT2... */

typedef union { /* Capture Register 2 */
	vuint16_t R;
	struct {
		vuint16_t CAPT2 : 16; /* deprecated definition -- do not use */
	} B;
} mcTIMER_CAPT2_16B_tag;

/* Register layout for all registers LOAD... */

typedef union { /* Load Register */
	vuint16_t R;
	struct {
		vuint16_t LOAD : 16; /* deprecated definition -- do not use */
	} B;
} mcTIMER_LOAD_16B_tag;

/* Register layout for all registers HOLD... */

typedef union { /* Hold Register */
	vuint16_t R;
	struct {
		vuint16_t HOLD : 16; /* deprecated definition -- do not use */
	} B;
} mcTIMER_HOLD_16B_tag;

/* Register layout for all registers CNTR... */

typedef union { /* Counter Register */
	vuint16_t R;
	struct {
		vuint16_t CNTR : 16; /* deprecated definition -- do not use */
	} B;
} mcTIMER_CNTR_16B_tag;

/* Register layout for all registers CTRL1... */

typedef union { /* Control Register */
	vuint16_t R;
	struct {
		vuint16_t CNTMODE : 3; /* Count Mode */
		vuint16_t PRISRC : 5;	 /* Primary Count Source */
		vuint16_t ONCE : 1;		 /* Count Once */
		vuint16_t LENGTH : 1;	 /* Count Length */
		vuint16_t DIR : 1;		 /* Count Direction */
		vuint16_t SECSRC : 5;	 /* Secondary Count Source */
	} B;
} mcTIMER_CTRL1_16B_tag;

/* Register layout for all registers CTRL2... */

typedef union { /* Control Register 2 */
	vuint16_t R;
	struct {
		vuint16_t OEN : 1;		 /* Output Enable */
		vuint16_t RDNT : 1;		 /* Redundant Channel Enable */
		vuint16_t INPUT : 1;	 /* External Input Signal */
		vuint16_t VAL : 1;		 /* Forced OFLAG Value */
		vuint16_t FORCE : 1;	 /* Force the OFLAG output */
		vuint16_t COFRC : 1;	 /* Co-channel OFLAG Force */
		vuint16_t COINIT : 2;	 /* Co-channel Initialization */
		vuint16_t SIPS : 1;		 /* Secondary Source Input Polarity Select */
		vuint16_t PIPS : 1;		 /* Primary Source Input Polarity Select */
		vuint16_t OPS : 1;		 /* Output Polarity Select */
		vuint16_t MSTR : 1;		 /* Master Mode */
		vuint16_t OUTMODE : 4; /* Output Mode */
	} B;
} mcTIMER_CTRL2_16B_tag;

/* Register layout for all registers CTRL3... */

typedef union { /* Control Register 3 */
	vuint16_t R;
	struct {
		vuint16_t STPEN : 1;	/* Stop Action Enable */
		vuint16_t ROC : 2;		/* Reload On Capture */
		vuint16_t FMODE : 1;	/* Fault Safing Mode */
		vuint16_t FDIS : 4;		/* Fault Disable Mask */
		vuint16_t C2FCNT : 3; /* CAPT2 FIFO Word Count */
		vuint16_t C1FCNT : 3; /* CAPT1 FIFO Word Count */
		vuint16_t DBGEN : 2;	/* Debug Actions Enable */
	} B;
} mcTIMER_CTRL3_16B_tag;

/* Register layout for all registers STS... */

typedef union { /* Status Register */
	vuint16_t R;
	struct {
		vuint16_t : 6;
		vuint16_t WDF : 1;	/* Watchdog Time-out Flag */
		vuint16_t RCF : 1;	/* Redundant Channel Flag */
		vuint16_t ICF2 : 1; /* Input Capture 2 Flag */
		vuint16_t ICF1 : 1; /* Input Capture 1 Flag */
		vuint16_t IEHF : 1; /* Input Edge High Flag */
		vuint16_t IELF : 1; /* Input Edge Low Flag */
		vuint16_t TOF : 1;	/* Timer Overflow Flag */
		vuint16_t TCF2 : 1; /* Timer Compare 2 Flag */
		vuint16_t TCF1 : 1; /* Timer Compare 1 Flag */
		vuint16_t TCF : 1;	/* Timer Compare Flag */
	} B;
} mcTIMER_STS_16B_tag;

/* Register layout for all registers INTDMA... */

typedef union { /* Interrupt and DMA Enable Register */
	vuint16_t R;
	struct {
		vuint16_t ICF2DE : 1;		/* Input Capture 2 Flag DMA Enable */
		vuint16_t ICF1DE : 1;		/* Input Capture 1 Flag DMA Enable */
		vuint16_t CMPLD2DE : 1; /* Comparator Load Register 2 Flag DMA Enable */
		vuint16_t CMPLD1DE : 1; /* Comparator Load Register 1 Flag DMA Enable */
		vuint16_t : 2;
		vuint16_t WDFIE : 1;	/* Watchdog Flag Interrupt Enable */
		vuint16_t RCFIE : 1;	/* Redundant Channel Flag Interrupt Enable */
		vuint16_t ICF2IE : 1; /* Input Capture 2 Flag Interrupt Enable */
		vuint16_t ICF1IE : 1; /* Input Capture 1 Flag Interrupt Enable */
		vuint16_t IEHFIE : 1; /* Input Edge High Flag Interrupt Enable */
		vuint16_t IELFIE : 1; /* Input Edge Low Flag Interrupt Enable */
		vuint16_t TOFIE : 1;	/* Timer Overflow Flag Interrupt Enable */
		vuint16_t TCF2IE : 1; /* Timer Compare 2 Flag Interrupt Enable */
		vuint16_t TCF1IE : 1; /* Timer Compare 1 Flag Interrupt Enable */
		vuint16_t TCFIE : 1;	/* Timer Compare Flag Interrupt Enable */
	} B;
} mcTIMER_INTDMA_16B_tag;

/* Register layout for all registers CMPLD1... */

typedef union { /* Comparator Load Register 1 */
	vuint16_t R;
	struct {
		vuint16_t CMPLD1 : 16; /* deprecated definition -- do not use */
	} B;
} mcTIMER_CMPLD1_16B_tag;

/* Register layout for all registers CMPLD2... */

typedef union { /* Comparator Load Register 2 */
	vuint16_t R;
	struct {
		vuint16_t CMPLD2 : 16; /* deprecated definition -- do not use */
	} B;
} mcTIMER_CMPLD2_16B_tag;

/* Register layout for all registers CCCTRL... */

typedef union { /* Compare and Capture Control Register */
	vuint16_t R;
	struct {
		vuint16_t CLC2 : 3;			/* Compare Load Control 2 */
		vuint16_t CLC1 : 3;			/* Compare Load Control 1 */
		vuint16_t CMPMODE : 2;	/* Compare Mode */
		vuint16_t CPT2MODE : 2; /* Capture 2 Mode Control */
		vuint16_t CPT1MODE : 2; /* Capture 1 Mode Control */
		vuint16_t CFWM : 2;			/* Capture FIFO Water Mark */
		vuint16_t ONESHOT : 1;	/* One Shot Capture Mode */
		vuint16_t ARM : 1;			/* Arm Capture */
	} B;
} mcTIMER_CCCTRL_16B_tag;

/* Register layout for all registers FILT... */

typedef union { /* Input Filter Register */
	vuint16_t R;
	struct {
		vuint16_t : 5;
#ifndef USE_FIELD_ALIASES_mcTIMER
		vuint16_t FILT_CNT : 3; /* Input Filter Sample Count */
#else
		vuint16_t FILTCNT : 3;		 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_mcTIMER
		vuint16_t FILT_PER : 8; /* Input Filter Sample Period */
#else
		vuint16_t FILTPER : 8;		 /* deprecated name - please avoid */
#endif
	} B;
} mcTIMER_FILT_16B_tag;

typedef union { /* Watchdog Time-out Register */
	vuint16_t R;
	struct {
		vuint16_t WDTOL : 16; /* deprecated definition -- do not use */
	} B;
} mcTIMER_WDTOL_16B_tag;

typedef union { /* Watchdog Time-out Register */
	vuint16_t R;
	struct {
		vuint16_t WDTOH : 16; /* deprecated definition -- do not use */
	} B;
} mcTIMER_WDTOH_16B_tag;

typedef union { /* Fault Control Register */
	vuint16_t R;
	struct {
		vuint16_t : 3;
		vuint16_t FTEST : 1; /* Fault Test */
		vuint16_t FIE : 4;	 /* Fault Interrupt Enable */
		vuint16_t : 4;
		vuint16_t FLVL : 4; /* Fault Active Logic Level */
	} B;
} mcTIMER_FCTRL_16B_tag;

typedef union { /* Fault Status Register */
	vuint16_t R;
	struct {
		vuint16_t : 4;
		vuint16_t FFPIN : 4; /* Filtered Fault Pin */
		vuint16_t : 4;
		vuint16_t FFLAG : 4; /* Fault Flag */
	} B;
} mcTIMER_FSTS_16B_tag;

typedef union { /* Fault Filter Registers */
	vuint16_t R;
	struct {
		vuint16_t : 5;
#ifndef USE_FIELD_ALIASES_mcTIMER
		vuint16_t FFPIN : 3; /* Fault Filter Sample Count */
#else
		vuint16_t FFILTCNT : 3;		 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_mcTIMER
		vuint16_t FFILT_PER : 8; /* Fault Filter Sample Period */
#else
		vuint16_t FFILTPER : 8;		 /* deprecated name - please avoid */
#endif
	} B;
} mcTIMER_FFILT_16B_tag;

typedef union { /* Channel Enable Registers */
	vuint16_t R;
	struct {
		vuint16_t : 8;
		vuint16_t ENBL : 8; /* Timer Channel Enable */
	} B;
} mcTIMER_ENBL_16B_tag;

typedef union { /* DMA Request 0 Select Registers */
	vuint16_t R;
	struct {
		vuint16_t : 11;
		vuint16_t DREQ0V : 5; /* DMA Request Select */
	} B;
} mcTIMER_DREQ0_16B_tag;

typedef union { /* DMA Request 1 Select Registers */
	vuint16_t R;
	struct {
		vuint16_t : 11;
		vuint16_t DREQ1V : 5; /* DMA Request Select */
	} B;
} mcTIMER_DREQ1_16B_tag;

typedef union { /* DMA Request 2 Select Registers */
	vuint16_t R;
	struct {
		vuint16_t : 11;
		vuint16_t DREQ2V : 5; /* DMA Request Select */
	} B;
} mcTIMER_DREQ2_16B_tag;

typedef union { /* DMA Request 3 Select Registers */
	vuint16_t R;
	struct {
		vuint16_t : 11;
		vuint16_t DREQ3V : 5; /* DMA Request Select */
	} B;
} mcTIMER_DREQ3_16B_tag;

/* Register layout for generated register(s) DREQ... */

typedef union { /*  */
	vuint16_t R;
} mcTIMER_DREQ_16B_tag;

typedef struct mcTIMER_CHANNEL_struct_tag {
	/* Compare Register 1 */
	mcTIMER_COMP1_16B_tag COMP1; /* relative offset: 0x0000 */
															 /* Compare Register 2 */
	mcTIMER_COMP2_16B_tag COMP2; /* relative offset: 0x0002 */
															 /* Capture Register 1 */
	mcTIMER_CAPT1_16B_tag CAPT1; /* relative offset: 0x0004 */
															 /* Capture Register 2 */
	mcTIMER_CAPT2_16B_tag CAPT2; /* relative offset: 0x0006 */
															 /* Load Register */
	mcTIMER_LOAD_16B_tag LOAD;	 /* relative offset: 0x0008 */
															 /* Hold Register */
	mcTIMER_HOLD_16B_tag HOLD;	 /* relative offset: 0x000A */
															 /* Counter Register */
	mcTIMER_CNTR_16B_tag CNTR;	 /* relative offset: 0x000C */
	union {
		/* Control Register */
		mcTIMER_CTRL1_16B_tag CTRL1; /* relative offset: 0x000E */
		mcTIMER_CTRL1_16B_tag CTRL;	 /* deprecated - please avoid */
	};
	/* Control Register 2 */
	mcTIMER_CTRL2_16B_tag CTRL2;	 /* relative offset: 0x0010 */
																 /* Control Register 3 */
	mcTIMER_CTRL3_16B_tag CTRL3;	 /* relative offset: 0x0012 */
																 /* Status Register */
	mcTIMER_STS_16B_tag STS;			 /* relative offset: 0x0014 */
																 /* Interrupt and DMA Enable Register */
	mcTIMER_INTDMA_16B_tag INTDMA; /* relative offset: 0x0016 */
																 /* Comparator Load Register 1 */
	mcTIMER_CMPLD1_16B_tag CMPLD1; /* relative offset: 0x0018 */
																 /* Comparator Load Register 2 */
	mcTIMER_CMPLD2_16B_tag CMPLD2; /* relative offset: 0x001A */
																 /* Compare and Capture Control Register */
	mcTIMER_CCCTRL_16B_tag CCCTRL; /* relative offset: 0x001C */
																 /* Input Filter Register */
	mcTIMER_FILT_16B_tag FILT;		 /* relative offset: 0x001E */

} mcTIMER_CHANNEL_tag;

typedef struct mcTIMER_struct_tag { /* start of mcTIMER_tag */
	union {
		/*  Register set CHANNEL */
		mcTIMER_CHANNEL_tag CHANNEL[6]; /* offset: 0x0000  (0x0020 x 6) */

		struct {
			/* Compare Register 1 */
			mcTIMER_COMP1_16B_tag COMP10;		/* offset: 0x0000 size: 16 bit */
																			/* Compare Register 2 */
			mcTIMER_COMP2_16B_tag COMP20;		/* offset: 0x0002 size: 16 bit */
																			/* Capture Register 1 */
			mcTIMER_CAPT1_16B_tag CAPT10;		/* offset: 0x0004 size: 16 bit */
																			/* Capture Register 2 */
			mcTIMER_CAPT2_16B_tag CAPT20;		/* offset: 0x0006 size: 16 bit */
																			/* Load Register */
			mcTIMER_LOAD_16B_tag LOAD0;			/* offset: 0x0008 size: 16 bit */
																			/* Hold Register */
			mcTIMER_HOLD_16B_tag HOLD0;			/* offset: 0x000A size: 16 bit */
																			/* Counter Register */
			mcTIMER_CNTR_16B_tag CNTR0;			/* offset: 0x000C size: 16 bit */
																			/* Control Register */
			mcTIMER_CTRL1_16B_tag CTRL10;		/* offset: 0x000E size: 16 bit */
																			/* Control Register 2 */
			mcTIMER_CTRL2_16B_tag CTRL20;		/* offset: 0x0010 size: 16 bit */
																			/* Control Register 3 */
			mcTIMER_CTRL3_16B_tag CTRL30;		/* offset: 0x0012 size: 16 bit */
																			/* Status Register */
			mcTIMER_STS_16B_tag STS0;				/* offset: 0x0014 size: 16 bit */
																			/* Interrupt and DMA Enable Register */
			mcTIMER_INTDMA_16B_tag INTDMA0; /* offset: 0x0016 size: 16 bit */
																			/* Comparator Load Register 1 */
			mcTIMER_CMPLD1_16B_tag CMPLD10; /* offset: 0x0018 size: 16 bit */
																			/* Comparator Load Register 2 */
			mcTIMER_CMPLD2_16B_tag CMPLD20; /* offset: 0x001A size: 16 bit */
																			/* Compare and Capture Control Register */
			mcTIMER_CCCTRL_16B_tag CCCTRL0; /* offset: 0x001C size: 16 bit */
																			/* Input Filter Register */
			mcTIMER_FILT_16B_tag FILT0;			/* offset: 0x001E size: 16 bit */
																			/* Compare Register 1 */
			mcTIMER_COMP1_16B_tag COMP11;		/* offset: 0x0020 size: 16 bit */
																			/* Compare Register 2 */
			mcTIMER_COMP2_16B_tag COMP21;		/* offset: 0x0022 size: 16 bit */
																			/* Capture Register 1 */
			mcTIMER_CAPT1_16B_tag CAPT11;		/* offset: 0x0024 size: 16 bit */
																			/* Capture Register 2 */
			mcTIMER_CAPT2_16B_tag CAPT21;		/* offset: 0x0026 size: 16 bit */
																			/* Load Register */
			mcTIMER_LOAD_16B_tag LOAD1;			/* offset: 0x0028 size: 16 bit */
																			/* Hold Register */
			mcTIMER_HOLD_16B_tag HOLD1;			/* offset: 0x002A size: 16 bit */
																			/* Counter Register */
			mcTIMER_CNTR_16B_tag CNTR1;			/* offset: 0x002C size: 16 bit */
																			/* Control Register */
			mcTIMER_CTRL1_16B_tag CTRL11;		/* offset: 0x002E size: 16 bit */
																			/* Control Register 2 */
			mcTIMER_CTRL2_16B_tag CTRL21;		/* offset: 0x0030 size: 16 bit */
																			/* Control Register 3 */
			mcTIMER_CTRL3_16B_tag CTRL31;		/* offset: 0x0032 size: 16 bit */
																			/* Status Register */
			mcTIMER_STS_16B_tag STS1;				/* offset: 0x0034 size: 16 bit */
																			/* Interrupt and DMA Enable Register */
			mcTIMER_INTDMA_16B_tag INTDMA1; /* offset: 0x0036 size: 16 bit */
																			/* Comparator Load Register 1 */
			mcTIMER_CMPLD1_16B_tag CMPLD11; /* offset: 0x0038 size: 16 bit */
																			/* Comparator Load Register 2 */
			mcTIMER_CMPLD2_16B_tag CMPLD21; /* offset: 0x003A size: 16 bit */
																			/* Compare and Capture Control Register */
			mcTIMER_CCCTRL_16B_tag CCCTRL1; /* offset: 0x003C size: 16 bit */
																			/* Input Filter Register */
			mcTIMER_FILT_16B_tag FILT1;			/* offset: 0x003E size: 16 bit */
																			/* Compare Register 1 */
			mcTIMER_COMP1_16B_tag COMP12;		/* offset: 0x0040 size: 16 bit */
																			/* Compare Register 2 */
			mcTIMER_COMP2_16B_tag COMP22;		/* offset: 0x0042 size: 16 bit */
																			/* Capture Register 1 */
			mcTIMER_CAPT1_16B_tag CAPT12;		/* offset: 0x0044 size: 16 bit */
																			/* Capture Register 2 */
			mcTIMER_CAPT2_16B_tag CAPT22;		/* offset: 0x0046 size: 16 bit */
																			/* Load Register */
			mcTIMER_LOAD_16B_tag LOAD2;			/* offset: 0x0048 size: 16 bit */
																			/* Hold Register */
			mcTIMER_HOLD_16B_tag HOLD2;			/* offset: 0x004A size: 16 bit */
																			/* Counter Register */
			mcTIMER_CNTR_16B_tag CNTR2;			/* offset: 0x004C size: 16 bit */
																			/* Control Register */
			mcTIMER_CTRL1_16B_tag CTRL12;		/* offset: 0x004E size: 16 bit */
																			/* Control Register 2 */
			mcTIMER_CTRL2_16B_tag CTRL22;		/* offset: 0x0050 size: 16 bit */
																			/* Control Register 3 */
			mcTIMER_CTRL3_16B_tag CTRL32;		/* offset: 0x0052 size: 16 bit */
																			/* Status Register */
			mcTIMER_STS_16B_tag STS2;				/* offset: 0x0054 size: 16 bit */
																			/* Interrupt and DMA Enable Register */
			mcTIMER_INTDMA_16B_tag INTDMA2; /* offset: 0x0056 size: 16 bit */
																			/* Comparator Load Register 1 */
			mcTIMER_CMPLD1_16B_tag CMPLD12; /* offset: 0x0058 size: 16 bit */
																			/* Comparator Load Register 2 */
			mcTIMER_CMPLD2_16B_tag CMPLD22; /* offset: 0x005A size: 16 bit */
																			/* Compare and Capture Control Register */
			mcTIMER_CCCTRL_16B_tag CCCTRL2; /* offset: 0x005C size: 16 bit */
																			/* Input Filter Register */
			mcTIMER_FILT_16B_tag FILT2;			/* offset: 0x005E size: 16 bit */
																			/* Compare Register 1 */
			mcTIMER_COMP1_16B_tag COMP13;		/* offset: 0x0060 size: 16 bit */
																			/* Compare Register 2 */
			mcTIMER_COMP2_16B_tag COMP23;		/* offset: 0x0062 size: 16 bit */
																			/* Capture Register 1 */
			mcTIMER_CAPT1_16B_tag CAPT13;		/* offset: 0x0064 size: 16 bit */
																			/* Capture Register 2 */
			mcTIMER_CAPT2_16B_tag CAPT23;		/* offset: 0x0066 size: 16 bit */
																			/* Load Register */
			mcTIMER_LOAD_16B_tag LOAD3;			/* offset: 0x0068 size: 16 bit */
																			/* Hold Register */
			mcTIMER_HOLD_16B_tag HOLD3;			/* offset: 0x006A size: 16 bit */
																			/* Counter Register */
			mcTIMER_CNTR_16B_tag CNTR3;			/* offset: 0x006C size: 16 bit */
																			/* Control Register */
			mcTIMER_CTRL1_16B_tag CTRL13;		/* offset: 0x006E size: 16 bit */
																			/* Control Register 2 */
			mcTIMER_CTRL2_16B_tag CTRL23;		/* offset: 0x0070 size: 16 bit */
																			/* Control Register 3 */
			mcTIMER_CTRL3_16B_tag CTRL33;		/* offset: 0x0072 size: 16 bit */
																			/* Status Register */
			mcTIMER_STS_16B_tag STS3;				/* offset: 0x0074 size: 16 bit */
																			/* Interrupt and DMA Enable Register */
			mcTIMER_INTDMA_16B_tag INTDMA3; /* offset: 0x0076 size: 16 bit */
																			/* Comparator Load Register 1 */
			mcTIMER_CMPLD1_16B_tag CMPLD13; /* offset: 0x0078 size: 16 bit */
																			/* Comparator Load Register 2 */
			mcTIMER_CMPLD2_16B_tag CMPLD23; /* offset: 0x007A size: 16 bit */
																			/* Compare and Capture Control Register */
			mcTIMER_CCCTRL_16B_tag CCCTRL3; /* offset: 0x007C size: 16 bit */
																			/* Input Filter Register */
			mcTIMER_FILT_16B_tag FILT3;			/* offset: 0x007E size: 16 bit */
																			/* Compare Register 1 */
			mcTIMER_COMP1_16B_tag COMP14;		/* offset: 0x0080 size: 16 bit */
																			/* Compare Register 2 */
			mcTIMER_COMP2_16B_tag COMP24;		/* offset: 0x0082 size: 16 bit */
																			/* Capture Register 1 */
			mcTIMER_CAPT1_16B_tag CAPT14;		/* offset: 0x0084 size: 16 bit */
																			/* Capture Register 2 */
			mcTIMER_CAPT2_16B_tag CAPT24;		/* offset: 0x0086 size: 16 bit */
																			/* Load Register */
			mcTIMER_LOAD_16B_tag LOAD4;			/* offset: 0x0088 size: 16 bit */
																			/* Hold Register */
			mcTIMER_HOLD_16B_tag HOLD4;			/* offset: 0x008A size: 16 bit */
																			/* Counter Register */
			mcTIMER_CNTR_16B_tag CNTR4;			/* offset: 0x008C size: 16 bit */
																			/* Control Register */
			mcTIMER_CTRL1_16B_tag CTRL14;		/* offset: 0x008E size: 16 bit */
																			/* Control Register 2 */
			mcTIMER_CTRL2_16B_tag CTRL24;		/* offset: 0x0090 size: 16 bit */
																			/* Control Register 3 */
			mcTIMER_CTRL3_16B_tag CTRL34;		/* offset: 0x0092 size: 16 bit */
																			/* Status Register */
			mcTIMER_STS_16B_tag STS4;				/* offset: 0x0094 size: 16 bit */
																			/* Interrupt and DMA Enable Register */
			mcTIMER_INTDMA_16B_tag INTDMA4; /* offset: 0x0096 size: 16 bit */
																			/* Comparator Load Register 1 */
			mcTIMER_CMPLD1_16B_tag CMPLD14; /* offset: 0x0098 size: 16 bit */
																			/* Comparator Load Register 2 */
			mcTIMER_CMPLD2_16B_tag CMPLD24; /* offset: 0x009A size: 16 bit */
																			/* Compare and Capture Control Register */
			mcTIMER_CCCTRL_16B_tag CCCTRL4; /* offset: 0x009C size: 16 bit */
																			/* Input Filter Register */
			mcTIMER_FILT_16B_tag FILT4;			/* offset: 0x009E size: 16 bit */
																			/* Compare Register 1 */
			mcTIMER_COMP1_16B_tag COMP15;		/* offset: 0x00A0 size: 16 bit */
																			/* Compare Register 2 */
			mcTIMER_COMP2_16B_tag COMP25;		/* offset: 0x00A2 size: 16 bit */
																			/* Capture Register 1 */
			mcTIMER_CAPT1_16B_tag CAPT15;		/* offset: 0x00A4 size: 16 bit */
																			/* Capture Register 2 */
			mcTIMER_CAPT2_16B_tag CAPT25;		/* offset: 0x00A6 size: 16 bit */
																			/* Load Register */
			mcTIMER_LOAD_16B_tag LOAD5;			/* offset: 0x00A8 size: 16 bit */
																			/* Hold Register */
			mcTIMER_HOLD_16B_tag HOLD5;			/* offset: 0x00AA size: 16 bit */
																			/* Counter Register */
			mcTIMER_CNTR_16B_tag CNTR5;			/* offset: 0x00AC size: 16 bit */
																			/* Control Register */
			mcTIMER_CTRL1_16B_tag CTRL15;		/* offset: 0x00AE size: 16 bit */
																			/* Control Register 2 */
			mcTIMER_CTRL2_16B_tag CTRL25;		/* offset: 0x00B0 size: 16 bit */
																			/* Control Register 3 */
			mcTIMER_CTRL3_16B_tag CTRL35;		/* offset: 0x00B2 size: 16 bit */
																			/* Status Register */
			mcTIMER_STS_16B_tag STS5;				/* offset: 0x00B4 size: 16 bit */
																			/* Interrupt and DMA Enable Register */
			mcTIMER_INTDMA_16B_tag INTDMA5; /* offset: 0x00B6 size: 16 bit */
																			/* Comparator Load Register 1 */
			mcTIMER_CMPLD1_16B_tag CMPLD15; /* offset: 0x00B8 size: 16 bit */
																			/* Comparator Load Register 2 */
			mcTIMER_CMPLD2_16B_tag CMPLD25; /* offset: 0x00BA size: 16 bit */
																			/* Compare and Capture Control Register */
			mcTIMER_CCCTRL_16B_tag CCCTRL5; /* offset: 0x00BC size: 16 bit */
																			/* Input Filter Register */
			mcTIMER_FILT_16B_tag FILT5;			/* offset: 0x00BE size: 16 bit */
		};
	};
	int8_t mcTIMER_reserved_00C0[64];
	/* Watchdog Time-out Register */
	mcTIMER_WDTOL_16B_tag WDTOL; /* offset: 0x0100 size: 16 bit */
															 /* Watchdog Time-out Register */
	mcTIMER_WDTOH_16B_tag WDTOH; /* offset: 0x0102 size: 16 bit */
															 /* Fault Control Register */
	mcTIMER_FCTRL_16B_tag FCTRL; /* offset: 0x0104 size: 16 bit */
															 /* Fault Status Register */
	mcTIMER_FSTS_16B_tag FSTS;	 /* offset: 0x0106 size: 16 bit */
															 /* Fault Filter Registers */
	mcTIMER_FFILT_16B_tag FFILT; /* offset: 0x0108 size: 16 bit */
	int8_t mcTIMER_reserved_010A[2];
	/* Channel Enable Registers */
	mcTIMER_ENBL_16B_tag ENBL; /* offset: 0x010C size: 16 bit */
	int8_t mcTIMER_reserved_010E_C[2];
	union {
		mcTIMER_DREQ_16B_tag DREQ[4]; /* offset: 0x0110  (0x0002 x 4) */

		struct {
			/* DMA Request 0 Select Registers */
			mcTIMER_DREQ0_16B_tag DREQ0; /* offset: 0x0110 size: 16 bit */
																	 /* DMA Request 1 Select Registers */
			mcTIMER_DREQ1_16B_tag DREQ1; /* offset: 0x0112 size: 16 bit */
																	 /* DMA Request 2 Select Registers */
			mcTIMER_DREQ2_16B_tag DREQ2; /* offset: 0x0114 size: 16 bit */
																	 /* DMA Request 3 Select Registers */
			mcTIMER_DREQ3_16B_tag DREQ3; /* offset: 0x0116 size: 16 bit */
		};
	};
} mcTIMER_tag;

#define mcTIMER0 (*(volatile mcTIMER_tag*)0xFFE18000UL)
#define mcTIMER1 (*(volatile mcTIMER_tag*)0xFFE1C000UL)
#define mcTIMER2 (*(volatile mcTIMER_tag*)0xFFE20000UL)

/****************************************************************/
/*                                                              */
/* Module: mcPWM  */
/*                                                              */
/****************************************************************/

/* Register layout for all registers CNT... */

typedef union { /* Counter Register */
	vuint16_t R;
} mcPWM_CNT_16B_tag;

/* Register layout for all registers INIT... */

typedef union { /* Initial Counter Register */
	vuint16_t R;
} mcPWM_INIT_16B_tag;

/* Register layout for all registers CTRL2... */

typedef union { /* Control 2 Register */
	vuint16_t R;
	struct {
		vuint16_t DBGEN : 1;	/* Debug Enable */
		vuint16_t WAITEN : 1; /* Wait Enable */
		vuint16_t INDEP : 1;	/* Independent or Complementary Pair Operation */
#ifndef USE_FIELD_ALIASES_mcPWM
		vuint16_t PWM23_INIT : 1; /* PWM23 Initial Value */
#else
		vuint16_t PWMA_INIT : 1;	 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_mcPWM
		vuint16_t PWM45_INIT : 1; /* PWM23 Initial Value */
#else
		vuint16_t PWMB_INIT : 1;	 /* deprecated name - please avoid */
#endif
		vuint16_t PWMX_INIT : 1;	/* PWMX Initial Value */
		vuint16_t INIT_SEL : 2;		/* Initialization Control Select */
		vuint16_t FRCEN : 1;			/* Force Initialization enable */
		vuint16_t FORCE : 1;			/* Force Initialization */
		vuint16_t FORCE_SEL : 3;	/* Force Source Select */
		vuint16_t RELOAD_SEL : 1; /* Reload Source Select */
		vuint16_t CLK_SEL : 2;		/* Clock Source Select */
	} B;
} mcPWM_CTRL2_16B_tag;

/* Register layout for all registers CTRL1... */

typedef union { /* Control Register */
	vuint16_t R;
	struct {
		vuint16_t LDFQ : 4; /* Load Frequency */
		vuint16_t HALF : 1; /* Half Cycle Reload */
		vuint16_t FULL : 1; /* Full Cycle Reload */
		vuint16_t DT : 2;		/* Deadtime */
		vuint16_t : 1;
		vuint16_t PRSC : 3; /* Prescaler */
		vuint16_t : 1;
		vuint16_t LDMOD : 1; /* Load Mode Select */
		vuint16_t : 1;
#ifndef USE_FIELD_ALIASES_mcPWM
		vuint16_t DBL_EN : 1; /* Double Switching Enable */
#else
		vuint16_t DBLEN : 1;			 /* deprecated name - please avoid */
#endif
	} B;
} mcPWM_CTRL1_16B_tag;

/* Register layout for all registers VAL_0... */

typedef union { /* Value Register 0 */
	vuint16_t R;
} mcPWM_VAL_0_16B_tag;

/* Register layout for all registers VAL_1... */

typedef union { /* Value Register 1 */
	vuint16_t R;
} mcPWM_VAL_1_16B_tag;

/* Register layout for all registers VAL_2... */

typedef union { /* Value Register 2 */
	vuint16_t R;
} mcPWM_VAL_2_16B_tag;

/* Register layout for all registers VAL_3... */

typedef union { /* Value Register 3 */
	vuint16_t R;
} mcPWM_VAL_3_16B_tag;

/* Register layout for all registers VAL_4... */

typedef union { /* Value Register 4 */
	vuint16_t R;
} mcPWM_VAL_4_16B_tag;

/* Register layout for all registers VAL_5... */

typedef union { /* Value Register 5 */
	vuint16_t R;
} mcPWM_VAL_5_16B_tag;

/* Register layout for all registers FRACA... */

typedef union { /* Fractional Delay Register A */
	vuint16_t R;
	struct {
		vuint16_t FRACA_EN : 1; /* Fractional Delay Enable */
		vuint16_t : 10;
		vuint16_t FRACA_DLY : 5; /* Fractional Delay Value */
	} B;
} mcPWM_FRACA_16B_tag;

/* Register layout for all registers FRACB... */

typedef union { /* Fractional Delay Register B */
	vuint16_t R;
	struct {
		vuint16_t FRACA_EN : 1; /* Fractional Delay Enable */
		vuint16_t : 10;
		vuint16_t FRACA_DLY : 5; /* Fractional Delay Value */
	} B;
} mcPWM_FRACB_16B_tag;

/* Register layout for all registers OCTRL... */

typedef union { /* Output Control Register */
	vuint16_t R;
	struct {
		vuint16_t PWMA_IN : 1; /* PWMA Input */
		vuint16_t PWMB_IN : 1; /* PWMB Input */
		vuint16_t PWMX_IN : 1; /* PWMX Input */
		vuint16_t : 2;
		vuint16_t POLA : 1; /* PWMA Output Polarity */
		vuint16_t POLB : 1; /* PWMB Output Polarity */
		vuint16_t POLX : 1; /* PWMX Output Polarity */
		vuint16_t : 2;
		vuint16_t PWMAFS : 2; /* PWMA Fault State */
		vuint16_t PWMBFS : 2; /* PWMB Fault State */
		vuint16_t PWMXFS : 2; /* PWMX Fault State */
	} B;
} mcPWM_OCTRL_16B_tag;

/* Register layout for all registers STS... */

typedef union { /* Status Register */
	vuint16_t R;
	struct {
		vuint16_t : 1;
		vuint16_t RUF : 1;	/* Registers Updated Flag */
		vuint16_t REF : 1;	/* Reload Error Flag */
		vuint16_t RF : 1;		/* Reload Flag */
		vuint16_t CFA1 : 1; /* Capture Flag A1 */
		vuint16_t CFA0 : 1; /* Capture Flag A0 */
		vuint16_t CFB1 : 1; /* Capture Flag B1 */
		vuint16_t CFB0 : 1; /* Capture Flag B0 */
		vuint16_t CFX1 : 1; /* Capture Flag X1 */
		vuint16_t CFX0 : 1; /* Capture Flag X0 */
		vuint16_t CMPF : 6; /* Compare Flags */
	} B;
} mcPWM_STS_16B_tag;

/* Register layout for all registers INTEN... */

typedef union { /* Interrupt Enable Registers */
	vuint16_t R;
	struct {
		vuint16_t : 2;
		vuint16_t REIE : 1;	 /* Reload Error Interrupt Enable */
		vuint16_t RIE : 1;	 /* Reload Interrupt Enable */
		vuint16_t CA1IE : 1; /* Capture A1 Interrupt Enable */
		vuint16_t CA0IE : 1; /* Capture A0 Interrupt Enable */
		vuint16_t CB1IE : 1; /* Capture B1 Interrupt Enable */
		vuint16_t CB0IE : 1; /* Capture B0 Interrupt Enable */
		vuint16_t CX1IE : 1; /* Capture X1 Interrupt Enable */
		vuint16_t CX0IE : 1; /* Capture X0 Interrupt Enable */
		vuint16_t CMPIE : 6; /* Compare Interrupt Enables */
	} B;
} mcPWM_INTEN_16B_tag;

/* Register layout for all registers DMAEN... */

typedef union { /* DMA Enable Registers */
	vuint16_t R;
	struct {
		vuint16_t : 6;
		vuint16_t VALDE : 1;	/* Value Register DMA Enable */
		vuint16_t FAND : 1;		/* FIFO Watermark AND Control */
		vuint16_t CAPTDE : 2; /* Capture DMA Enable Source Select */
		vuint16_t CA1DE : 1;	/* Capture A1 FIFO DMA Enable */
		vuint16_t CA0DE : 1;	/* Capture A0 FIFO DMA Enable */
		vuint16_t CB1DE : 1;	/* Capture B1 FIFO DMA Enable */
		vuint16_t CB0DE : 1;	/* Capture B0 FIFO DMA Enable */
		vuint16_t CX1DE : 1;	/* Capture X1 FIFO DMA Enable */
		vuint16_t CX0DE : 1;	/* Capture X0 FIFO DMA Enable */
	} B;
} mcPWM_DMAEN_16B_tag;

/* Register layout for all registers TCTRL... */

typedef union { /* Output Trigger Control  Registers */
	vuint16_t R;
	struct {
		vuint16_t : 10;
		vuint16_t OUT_TRIG_EN : 6; /* Output Trigger Enables */
	} B;
} mcPWM_TCTRL_16B_tag;

/* Register layout for all registers DISMAP... */

typedef union { /* Fault Disable Mapping   Registers */
	vuint16_t R;
	struct {
		vuint16_t : 4;
		vuint16_t DISX : 4; /* PWMX Fault Disable Mask */
		vuint16_t DISB : 4; /* PWMB Fault Disable Mask */
		vuint16_t DISA : 4; /* PWMA Fault Disable Mask */
	} B;
} mcPWM_DISMAP_16B_tag;

/* Register layout for all registers DTCNT0... */

typedef union { /* Deadtime Count Register 0 */
	vuint16_t R;
	struct {
		vuint16_t : 5;
		vuint16_t DTCNT0 : 11; /* Deadtime Count Register 0 */
	} B;
} mcPWM_DTCNT0_16B_tag;

/* Register layout for all registers DTCNT1... */

typedef union { /* Deadtime Count Register 1 */
	vuint16_t R;
	struct {
		vuint16_t : 5;
		vuint16_t DTCNT1 : 11; /* Deadtime Count Register 1 */
	} B;
} mcPWM_DTCNT1_16B_tag;

/* Register layout for all registers CAPTCTRLA... */

typedef union { /* Capture Control A Register */
	vuint16_t R;
	struct {
		vuint16_t CA1CNT : 3;		 /* Capture A1 FIFO Word Count */
		vuint16_t CA0CNT : 3;		 /* Capture A0 FIFO Word Count */
		vuint16_t CFAWM : 2;		 /* Capture A FIFOs Water Mark */
		vuint16_t EDGCNTAEN : 1; /* Edge Counter A Enable */
		vuint16_t INPSELA : 1;	 /* Input Select A */
		vuint16_t EDGA1 : 2;		 /* Edge A 1 */
		vuint16_t EDGA0 : 2;		 /* Edge A 0 */
		vuint16_t ONESHOTA : 1;	 /* One Shot Mode A */
		vuint16_t ARMA : 1;			 /* Arm A */
	} B;
} mcPWM_CAPTCTRLA_16B_tag;

/* Register layout for all registers CAPTCMPA... */

typedef union { /* Capture Compare A Register */
	vuint16_t R;
	struct {
		vuint16_t EDGCNTA : 8; /* Edge Counter A */
		vuint16_t EDGCMPA : 8; /* Edge Compare A */
	} B;
} mcPWM_CAPTCMPA_16B_tag;

/* Register layout for all registers CAPTCTRLB... */

typedef union { /* Capture Control B Register */
	vuint16_t R;
	struct {
		vuint16_t CB1CNT : 3;		 /* Capture B1 FIFO Word Count */
		vuint16_t CB0CNT : 3;		 /* Capture B0 FIFO Word Count */
		vuint16_t CFBWM : 2;		 /* Capture B FIFOs Water Mark */
		vuint16_t EDGCNTBEN : 1; /* Edge Counter B Enable */
		vuint16_t INPSELB : 1;	 /* Input Select B */
		vuint16_t EDGB1 : 2;		 /* Edge B 1 */
		vuint16_t EDGB0 : 2;		 /* Edge B 0 */
		vuint16_t ONESHOTB : 1;	 /* One Shot Mode B */
		vuint16_t ARMB : 1;			 /* Arm B */
	} B;
} mcPWM_CAPTCTRLB_16B_tag;

/* Register layout for all registers CAPTCMPB... */

typedef union { /* Capture Compare B Register */
	vuint16_t R;
	struct {
		vuint16_t EDGCNTB : 8; /* Edge Counter B */
		vuint16_t EDGCMPB : 8; /* Edge Compare B */
	} B;
} mcPWM_CAPTCMPB_16B_tag;

/* Register layout for all registers CAPTCTRLX... */

typedef union { /* Capture Control X Register */
	vuint16_t R;
	struct {
		vuint16_t CX1CNT : 3; /* Capture X1 FIFO Word Count */
		vuint16_t CX0CNT : 3; /* Capture X0 FIFO Word Count */
		vuint16_t CFXWM : 2;	/* Capture X FIFOs Water Mark */
#ifndef USE_FIELD_ALIASES_mcPWM
		vuint16_t EDGCNTXEN : 1; /* Edge Counter X Enable */
#else
		vuint16_t EDGCNTX_EN : 1;	 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_mcPWM
		vuint16_t INPSELX : 1; /* Input Select X */
#else
		vuint16_t INP_SELX : 1;		 /* deprecated name - please avoid */
#endif
		vuint16_t EDGX1 : 2;		/* Edge X 1 */
		vuint16_t EDGX0 : 2;		/* Edge X 0 */
		vuint16_t ONESHOTX : 1; /* One Shot Mode X */
		vuint16_t ARMX : 1;			/* Arm X */
	} B;
} mcPWM_CAPTCTRLX_16B_tag;

/* Register layout for all registers CAPTCMPX... */

typedef union { /* Capture Compare X Register */
	vuint16_t R;
	struct {
		vuint16_t EDGCNTX : 8; /* Edge Counter X */
		vuint16_t EDGCMPX : 8; /* Edge Compare X */
	} B;
} mcPWM_CAPTCMPX_16B_tag;

/* Register layout for all registers CVAL0... */

typedef union { /* Capture Value 0 Register */
	vuint16_t R;
	struct {
		vuint16_t CAPTVAL0 : 16; /* Captured value from submodule counter */
	} B;
} mcPWM_CVAL0_16B_tag;

/* Register layout for all registers CVAL0CYC... */

typedef union { /* Capture Value 0 Cycle Register */
	vuint16_t R;
	struct {
		vuint16_t : 12;
		vuint16_t CVAL0CYC : 4; /* Capture Value 0 Cycle */
	} B;
} mcPWM_CVAL0CYC_16B_tag;

/* Register layout for all registers CVAL1... */

typedef union { /* Capture Value 1 Register */
	vuint16_t R;
	struct {
		vuint16_t CAPTVAL1 : 16; /* Captured value from submodule counter */
	} B;
} mcPWM_CVAL1_16B_tag;

/* Register layout for all registers CVAL1CYC... */

typedef union { /* Capture Value 1 Cycle Register */
	vuint16_t R;
	struct {
		vuint16_t : 12;
		vuint16_t CVAL1CYC : 4; /* Capture Value 1 Cycle */
	} B;
} mcPWM_CVAL1CYC_16B_tag;

/* Register layout for all registers CVAL2... */

typedef union { /* Capture Value 2 Register */
	vuint16_t R;
	struct {
		vuint16_t CAPTVAL2 : 16; /* Captured value from submodule counter */
	} B;
} mcPWM_CVAL2_16B_tag;

/* Register layout for all registers CVAL2CYC... */

typedef union { /* Capture Value 2 Cycle Register */
	vuint16_t R;
	struct {
		vuint16_t : 12;
		vuint16_t CVAL2CYC : 4; /* Capture Value 2 Cycle */
	} B;
} mcPWM_CVAL2CYC_16B_tag;

/* Register layout for all registers CVAL3... */

typedef union { /* Capture Value 3 Register */
	vuint16_t R;
	struct {
		vuint16_t CAPTVAL3 : 16; /* Captured value from submodule counter */
	} B;
} mcPWM_CVAL3_16B_tag;

/* Register layout for all registers CVAL3CYC... */

typedef union { /* Capture Value 3 Cycle Register */
	vuint16_t R;
	struct {
		vuint16_t : 12;
		vuint16_t CVAL3CYC : 4; /* Capture Value 3 Cycle */
	} B;
} mcPWM_CVAL3CYC_16B_tag;

/* Register layout for all registers CVAL4... */

typedef union { /* Capture Value 4 Register */
	vuint16_t R;
	struct {
		vuint16_t CAPTVAL4 : 16; /* Captured value from submodule counter */
	} B;
} mcPWM_CVAL4_16B_tag;

/* Register layout for all registers CVAL4CYC... */

typedef union { /* Capture Value 4 Cycle Register */
	vuint16_t R;
	struct {
		vuint16_t : 12;
		vuint16_t CVAL4CYC : 4; /* Capture Value 4 Cycle */
	} B;
} mcPWM_CVAL4CYC_16B_tag;

/* Register layout for all registers CVAL5... */

typedef union { /* Capture Value 5 Register */
	vuint16_t R;
	struct {
		vuint16_t CAPTVAL5 : 16; /* Captured value from submodule counter */
	} B;
} mcPWM_CVAL5_16B_tag;

/* Register layout for all registers CVAL5CYC... */

typedef union { /* Capture Value 5 Cycle Register */
	vuint16_t R;
	struct {
		vuint16_t : 12;
		vuint16_t CVAL5CYC : 4; /* Capture Value 5 Cycle */
	} B;
} mcPWM_CVAL5CYC_16B_tag;

typedef union { /* Output Enable Register */
	vuint16_t R;
	struct {
		vuint16_t : 4;
		vuint16_t PWMA_EN : 4; /* PWMA Output Enables */
		vuint16_t PWMB_EN : 4; /* PWMB Output Enables */
		vuint16_t PWMX_EN : 4; /* PWMX Output Enables */
	} B;
} mcPWM_OUTEN_16B_tag;

typedef union { /* Mask Register */
	vuint16_t R;
	struct {
		vuint16_t : 4;
		vuint16_t MASKA : 4; /* PWMA Masks */
		vuint16_t MASKB : 4; /* PWMB Masks */
		vuint16_t MASKX : 4; /* PWMX Masks */
	} B;
} mcPWM_MASK_16B_tag;

typedef union { /* Software Controlled Output Register */
	vuint16_t R;
	struct {
		vuint16_t : 8;
#ifndef USE_FIELD_ALIASES_mcPWM
		vuint16_t OUT23_3 : 1; /* Software Controlled Output 23_3 */
#else
		vuint16_t OUTA_3 : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_mcPWM
		vuint16_t OUT45_3 : 1; /* Software Controlled Output 45_3 */
#else
		vuint16_t OUTB_3 : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_mcPWM
		vuint16_t OUT23_2 : 1; /* Software Controlled Output 23_2 */
#else
		vuint16_t OUTA_2 : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_mcPWM
		vuint16_t OUT45_2 : 1; /* Software Controlled Output 45_2 */
#else
		vuint16_t OUTB_2 : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_mcPWM
		vuint16_t OUT23_1 : 1; /* Software Controlled Output 23_1 */
#else
		vuint16_t OUTA_1 : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_mcPWM
		vuint16_t OUT45_1 : 1; /* Software Controlled Output 45_1 */
#else
		vuint16_t OUTB_1 : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_mcPWM
		vuint16_t OUT23_0 : 1; /* Software Controlled Output 23_0 */
#else
		vuint16_t OUTA_0 : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_mcPWM
		vuint16_t OUT45_0 : 1; /* Software Controlled Output 45_0 */
#else
		vuint16_t OUTB_0 : 1;			 /* deprecated name - please avoid */
#endif
	} B;
} mcPWM_SWCOUT_16B_tag;

typedef union { /* Deadtime Source Select Register */
	vuint16_t R;
	struct {
#ifndef USE_FIELD_ALIASES_mcPWM
		vuint16_t SEL23_3 : 2; /* PWM23_3 Control Select */
#else
		vuint16_t SELA_3 : 2;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_mcPWM
		vuint16_t SEL45_3 : 2; /* PWM45_3 Control Select */
#else
		vuint16_t SELB_3 : 2;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_mcPWM
		vuint16_t SEL23_2 : 2; /* PWM23_2 Control Select */
#else
		vuint16_t SELA_2 : 2;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_mcPWM
		vuint16_t SEL45_2 : 2; /* PWM45_2 Control Select */
#else
		vuint16_t SELB_2 : 2;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_mcPWM
		vuint16_t SEL23_1 : 2; /* PWM23_1 Control Select */
#else
		vuint16_t SELA_1 : 2;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_mcPWM
		vuint16_t SEL45_1 : 2; /* PWM45_1 Control Select */
#else
		vuint16_t SELB_1 : 2;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_mcPWM
		vuint16_t SEL23_0 : 2; /* PWM23_0 Control Select */
#else
		vuint16_t SELA_0 : 2;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_mcPWM
		vuint16_t SEL45_0 : 2; /* PWM45_0 Control Select */
#else
		vuint16_t SELB_0 : 2;			 /* deprecated name - please avoid */
#endif
	} B;
} mcPWM_DTSRCSEL_16B_tag;

typedef union { /* Master Control Register */
	vuint16_t R;
	struct {
		vuint16_t IPOL : 4; /* Current Polarity */
		vuint16_t RUN : 4;	/* Run */
#ifndef USE_FIELD_ALIASES_mcPWM
		vuint16_t CLOK : 4; /* Clear Load Okay */
#else
		vuint16_t CLDOK : 4;			 /* deprecated name - please avoid */
#endif
		vuint16_t LDOK : 4; /* Load Okay */
	} B;
} mcPWM_MCTRL_16B_tag;

typedef union { /* Fault Control Register */
	vuint16_t R;
	struct {
		vuint16_t FLVL : 4;	 /* Fault Level */
		vuint16_t FAUTO : 4; /* Automatic Fault Clearing */
		vuint16_t FSAFE : 4; /* Fault Safety Mode */
		vuint16_t FIE : 4;	 /* Fault Interrupt Enables */
	} B;
} mcPWM_FCTRL_16B_tag;

typedef union { /* Fault Status Register */
	vuint16_t R;
	struct {
		vuint16_t : 3;
		vuint16_t FTEST : 1; /* Fault Test */
		vuint16_t FFPIN : 4; /* Filtered Fault Pins */
		vuint16_t : 4;
		vuint16_t FFLAG : 4; /* Fault Flags */
	} B;
} mcPWM_FSTS_16B_tag;

typedef union { /* Fault Filter Register */
	vuint16_t R;
	struct {
		vuint16_t : 5;
		vuint16_t FILT_CNT : 3; /* Fault Filter Count */
		vuint16_t FILT_PER : 8; /* Fault Filter Period */
	} B;
} mcPWM_FFILT_16B_tag;

/* Register layout for generated register(s) VAL... */

typedef union { /*  */
	vuint16_t R;
} mcPWM_VAL_16B_tag;

typedef struct mcPWM_SUBMOD_struct_tag {
	/* Counter Register */
	mcPWM_CNT_16B_tag CNT;		 /* relative offset: 0x0000 */
														 /* Initial Counter Register */
	mcPWM_INIT_16B_tag INIT;	 /* relative offset: 0x0002 */
														 /* Control 2 Register */
	mcPWM_CTRL2_16B_tag CTRL2; /* relative offset: 0x0004 */
	union {
		/* Control Register */
		mcPWM_CTRL1_16B_tag CTRL1; /* relative offset: 0x0006 */
		mcPWM_CTRL1_16B_tag CTRL;	 /* deprecated - please avoid */
	};
	/* Value Register 0 */

	union {
		struct {
			mcPWM_VAL_0_16B_tag VAL_0; /* relative offset: 0x0008 */
																 /* Value Register 1 */
			mcPWM_VAL_1_16B_tag VAL_1; /* relative offset: 0x000A */
																 /* Value Register 2 */
			mcPWM_VAL_2_16B_tag VAL_2; /* relative offset: 0x000C */
																 /* Value Register 3 */
			mcPWM_VAL_3_16B_tag VAL_3; /* relative offset: 0x000E */
																 /* Value Register 4 */
			mcPWM_VAL_4_16B_tag VAL_4; /* relative offset: 0x0010 */
																 /* Value Register 5 */
			mcPWM_VAL_5_16B_tag VAL_5; /* relative offset: 0x0012 */
		};

		mcPWM_VAL_0_16B_tag VAL[6]; /* offset: 0x0008 size: 16 bit */
	};
	/* Fractional Delay Register A */
	mcPWM_FRACA_16B_tag FRACA;				 /* relative offset: 0x0014 */
																		 /* Fractional Delay Register B */
	mcPWM_FRACB_16B_tag FRACB;				 /* relative offset: 0x0016 */
																		 /* Output Control Register */
	mcPWM_OCTRL_16B_tag OCTRL;				 /* relative offset: 0x0018 */
																		 /* Status Register */
	mcPWM_STS_16B_tag STS;						 /* relative offset: 0x001A */
																		 /* Interrupt Enable Registers */
	mcPWM_INTEN_16B_tag INTEN;				 /* relative offset: 0x001C */
																		 /* DMA Enable Registers */
	mcPWM_DMAEN_16B_tag DMAEN;				 /* relative offset: 0x001E */
																		 /* Output Trigger Control  Registers */
	mcPWM_TCTRL_16B_tag TCTRL;				 /* relative offset: 0x0020 */
																		 /* Fault Disable Mapping   Registers */
	mcPWM_DISMAP_16B_tag DISMAP;			 /* relative offset: 0x0022 */
																		 /* Deadtime Count Register 0 */
	mcPWM_DTCNT0_16B_tag DTCNT0;			 /* relative offset: 0x0024 */
																		 /* Deadtime Count Register 1 */
	mcPWM_DTCNT1_16B_tag DTCNT1;			 /* relative offset: 0x0026 */
																		 /* Capture Control A Register */
	mcPWM_CAPTCTRLA_16B_tag CAPTCTRLA; /* relative offset: 0x0028 */
	union {
		/* Capture Compare A Register */
		mcPWM_CAPTCMPA_16B_tag CAPTCMPA;	/* relative offset: 0x002A */
		mcPWM_CAPTCMPA_16B_tag CAPTCOMPA; /* deprecated - please avoid */
	};
	/* Capture Control B Register */
	mcPWM_CAPTCTRLB_16B_tag CAPTCTRLB; /* relative offset: 0x002C */
	union {
		/* Capture Compare B Register */
		mcPWM_CAPTCMPB_16B_tag CAPTCMPB;	/* relative offset: 0x002E */
		mcPWM_CAPTCMPB_16B_tag CAPTCOMPB; /* deprecated - please avoid */
	};
	/* Capture Control X Register */
	mcPWM_CAPTCTRLX_16B_tag CAPTCTRLX; /* relative offset: 0x0030 */
	union {
		/* Capture Compare X Register */
		mcPWM_CAPTCMPX_16B_tag CAPTCMPX;	/* relative offset: 0x0032 */
		mcPWM_CAPTCMPX_16B_tag CAPTCOMPX; /* deprecated - please avoid */
	};
	/* Capture Value 0 Register */
	mcPWM_CVAL0_16B_tag CVAL0; /* relative offset: 0x0034 */
	union {
		/* Capture Value 0 Cycle Register */
		mcPWM_CVAL0CYC_16B_tag CVAL0CYC; /* relative offset: 0x0036 */
		mcPWM_CVAL0CYC_16B_tag CVAL0C;	 /* deprecated - please avoid */
	};
	/* Capture Value 1 Register */
	mcPWM_CVAL1_16B_tag CVAL1; /* relative offset: 0x0038 */
	union {
		/* Capture Value 1 Cycle Register */
		mcPWM_CVAL1CYC_16B_tag CVAL1CYC; /* relative offset: 0x003A */
		mcPWM_CVAL1CYC_16B_tag CVAL1C;	 /* deprecated - please avoid */
	};
	/* Capture Value 2 Register */
	mcPWM_CVAL2_16B_tag CVAL2; /* relative offset: 0x003C */
	union {
		/* Capture Value 2 Cycle Register */
		mcPWM_CVAL2CYC_16B_tag CVAL2CYC; /* relative offset: 0x003E */
		mcPWM_CVAL2CYC_16B_tag CVAL2C;	 /* deprecated - please avoid */
	};
	/* Capture Value 3 Register */
	mcPWM_CVAL3_16B_tag CVAL3; /* relative offset: 0x0040 */
	union {
		/* Capture Value 3 Cycle Register */
		mcPWM_CVAL3CYC_16B_tag CVAL3CYC; /* relative offset: 0x0042 */
		mcPWM_CVAL3CYC_16B_tag CVAL3C;	 /* deprecated - please avoid */
	};
	/* Capture Value 4 Register */
	mcPWM_CVAL4_16B_tag CVAL4; /* relative offset: 0x0044 */
	union {
		/* Capture Value 4 Cycle Register */
		mcPWM_CVAL4CYC_16B_tag CVAL4CYC; /* relative offset: 0x0046 */
		mcPWM_CVAL4CYC_16B_tag CVAL4C;	 /* deprecated - please avoid */
	};
	/* Capture Value 5 Register */
	mcPWM_CVAL5_16B_tag CVAL5; /* relative offset: 0x0048 */
	union {
		/* Capture Value 5 Cycle Register */
		mcPWM_CVAL5CYC_16B_tag CVAL5CYC; /* relative offset: 0x004A */
		mcPWM_CVAL5CYC_16B_tag CVAL5C;	 /* deprecated - please avoid */
	};
	int8_t mcPWM_SUBMOD_reserved_004C[4];

} mcPWM_SUBMOD_tag;

typedef struct mcPWM_struct_tag { /* start of mcPWM_tag */
	union {
		/*  Register set SUBMOD */
		mcPWM_SUBMOD_tag SUBMOD[4]; /* offset: 0x0000  (0x0050 x 4) */

		struct {
			/* Counter Register */
			mcPWM_CNT_16B_tag CNT0;				/* offset: 0x0000 size: 16 bit */
																		/* Initial Counter Register */
			mcPWM_INIT_16B_tag INIT0;			/* offset: 0x0002 size: 16 bit */
																		/* Control 2 Register */
			mcPWM_CTRL2_16B_tag CTRL20;		/* offset: 0x0004 size: 16 bit */
																		/* Control Register */
			mcPWM_CTRL1_16B_tag CTRL10;		/* offset: 0x0006 size: 16 bit */
																		/* Value Register 0 */
			mcPWM_VAL_0_16B_tag VAL_00;		/* offset: 0x0008 size: 16 bit */
																		/* Value Register 1 */
			mcPWM_VAL_1_16B_tag VAL_10;		/* offset: 0x000A size: 16 bit */
																		/* Value Register 2 */
			mcPWM_VAL_2_16B_tag VAL_20;		/* offset: 0x000C size: 16 bit */
																		/* Value Register 3 */
			mcPWM_VAL_3_16B_tag VAL_30;		/* offset: 0x000E size: 16 bit */
																		/* Value Register 4 */
			mcPWM_VAL_4_16B_tag VAL_40;		/* offset: 0x0010 size: 16 bit */
																		/* Value Register 5 */
			mcPWM_VAL_5_16B_tag VAL_50;		/* offset: 0x0012 size: 16 bit */
																		/* Fractional Delay Register A */
			mcPWM_FRACA_16B_tag FRACA0;		/* offset: 0x0014 size: 16 bit */
																		/* Fractional Delay Register B */
			mcPWM_FRACB_16B_tag FRACB0;		/* offset: 0x0016 size: 16 bit */
																		/* Output Control Register */
			mcPWM_OCTRL_16B_tag OCTRL0;		/* offset: 0x0018 size: 16 bit */
																		/* Status Register */
			mcPWM_STS_16B_tag STS0;				/* offset: 0x001A size: 16 bit */
																		/* Interrupt Enable Registers */
			mcPWM_INTEN_16B_tag INTEN0;		/* offset: 0x001C size: 16 bit */
																		/* DMA Enable Registers */
			mcPWM_DMAEN_16B_tag DMAEN0;		/* offset: 0x001E size: 16 bit */
																		/* Output Trigger Control  Registers */
			mcPWM_TCTRL_16B_tag TCTRL0;		/* offset: 0x0020 size: 16 bit */
																		/* Fault Disable Mapping   Registers */
			mcPWM_DISMAP_16B_tag DISMAP0; /* offset: 0x0022 size: 16 bit */
																		/* Deadtime Count Register 0 */
			mcPWM_DTCNT0_16B_tag DTCNT00; /* offset: 0x0024 size: 16 bit */
																		/* Deadtime Count Register 1 */
			mcPWM_DTCNT1_16B_tag DTCNT10; /* offset: 0x0026 size: 16 bit */
																		/* Capture Control A Register */
			mcPWM_CAPTCTRLA_16B_tag CAPTCTRLA0; /* offset: 0x0028 size: 16 bit */
																					/* Capture Compare A Register */
			mcPWM_CAPTCMPA_16B_tag CAPTCMPA0;		/* offset: 0x002A size: 16 bit */
																					/* Capture Control B Register */
			mcPWM_CAPTCTRLB_16B_tag CAPTCTRLB0; /* offset: 0x002C size: 16 bit */
																					/* Capture Compare B Register */
			mcPWM_CAPTCMPB_16B_tag CAPTCMPB0;		/* offset: 0x002E size: 16 bit */
																					/* Capture Control X Register */
			mcPWM_CAPTCTRLX_16B_tag CAPTCTRLX0; /* offset: 0x0030 size: 16 bit */
																					/* Capture Compare X Register */
			mcPWM_CAPTCMPX_16B_tag CAPTCMPX0;		/* offset: 0x0032 size: 16 bit */
																					/* Capture Value 0 Register */
			mcPWM_CVAL0_16B_tag CVAL00;					/* offset: 0x0034 size: 16 bit */
																					/* Capture Value 0 Cycle Register */
			mcPWM_CVAL0CYC_16B_tag CVAL0CYC0;		/* offset: 0x0036 size: 16 bit */
																					/* Capture Value 1 Register */
			mcPWM_CVAL1_16B_tag CVAL10;					/* offset: 0x0038 size: 16 bit */
																					/* Capture Value 1 Cycle Register */
			mcPWM_CVAL1CYC_16B_tag CVAL1CYC0;		/* offset: 0x003A size: 16 bit */
																					/* Capture Value 2 Register */
			mcPWM_CVAL2_16B_tag CVAL20;					/* offset: 0x003C size: 16 bit */
																					/* Capture Value 2 Cycle Register */
			mcPWM_CVAL2CYC_16B_tag CVAL2CYC0;		/* offset: 0x003E size: 16 bit */
																					/* Capture Value 3 Register */
			mcPWM_CVAL3_16B_tag CVAL30;					/* offset: 0x0040 size: 16 bit */
																					/* Capture Value 3 Cycle Register */
			mcPWM_CVAL3CYC_16B_tag CVAL3CYC0;		/* offset: 0x0042 size: 16 bit */
																					/* Capture Value 4 Register */
			mcPWM_CVAL4_16B_tag CVAL40;					/* offset: 0x0044 size: 16 bit */
																					/* Capture Value 4 Cycle Register */
			mcPWM_CVAL4CYC_16B_tag CVAL4CYC0;		/* offset: 0x0046 size: 16 bit */
																					/* Capture Value 5 Register */
			mcPWM_CVAL5_16B_tag CVAL50;					/* offset: 0x0048 size: 16 bit */
																					/* Capture Value 5 Cycle Register */
			mcPWM_CVAL5CYC_16B_tag CVAL5CYC0;		/* offset: 0x004A size: 16 bit */
			int8_t mcPWM_reserved_004C_I2[4];
			/* Counter Register */
			mcPWM_CNT_16B_tag CNT1;				/* offset: 0x0050 size: 16 bit */
																		/* Initial Counter Register */
			mcPWM_INIT_16B_tag INIT1;			/* offset: 0x0052 size: 16 bit */
																		/* Control 2 Register */
			mcPWM_CTRL2_16B_tag CTRL21;		/* offset: 0x0054 size: 16 bit */
																		/* Control Register */
			mcPWM_CTRL1_16B_tag CTRL11;		/* offset: 0x0056 size: 16 bit */
																		/* Value Register 0 */
			mcPWM_VAL_0_16B_tag VAL_01;		/* offset: 0x0058 size: 16 bit */
																		/* Value Register 1 */
			mcPWM_VAL_1_16B_tag VAL_11;		/* offset: 0x005A size: 16 bit */
																		/* Value Register 2 */
			mcPWM_VAL_2_16B_tag VAL_21;		/* offset: 0x005C size: 16 bit */
																		/* Value Register 3 */
			mcPWM_VAL_3_16B_tag VAL_31;		/* offset: 0x005E size: 16 bit */
																		/* Value Register 4 */
			mcPWM_VAL_4_16B_tag VAL_41;		/* offset: 0x0060 size: 16 bit */
																		/* Value Register 5 */
			mcPWM_VAL_5_16B_tag VAL_51;		/* offset: 0x0062 size: 16 bit */
																		/* Fractional Delay Register A */
			mcPWM_FRACA_16B_tag FRACA1;		/* offset: 0x0064 size: 16 bit */
																		/* Fractional Delay Register B */
			mcPWM_FRACB_16B_tag FRACB1;		/* offset: 0x0066 size: 16 bit */
																		/* Output Control Register */
			mcPWM_OCTRL_16B_tag OCTRL1;		/* offset: 0x0068 size: 16 bit */
																		/* Status Register */
			mcPWM_STS_16B_tag STS1;				/* offset: 0x006A size: 16 bit */
																		/* Interrupt Enable Registers */
			mcPWM_INTEN_16B_tag INTEN1;		/* offset: 0x006C size: 16 bit */
																		/* DMA Enable Registers */
			mcPWM_DMAEN_16B_tag DMAEN1;		/* offset: 0x006E size: 16 bit */
																		/* Output Trigger Control  Registers */
			mcPWM_TCTRL_16B_tag TCTRL1;		/* offset: 0x0070 size: 16 bit */
																		/* Fault Disable Mapping   Registers */
			mcPWM_DISMAP_16B_tag DISMAP1; /* offset: 0x0072 size: 16 bit */
																		/* Deadtime Count Register 0 */
			mcPWM_DTCNT0_16B_tag DTCNT01; /* offset: 0x0074 size: 16 bit */
																		/* Deadtime Count Register 1 */
			mcPWM_DTCNT1_16B_tag DTCNT11; /* offset: 0x0076 size: 16 bit */
																		/* Capture Control A Register */
			mcPWM_CAPTCTRLA_16B_tag CAPTCTRLA1; /* offset: 0x0078 size: 16 bit */
																					/* Capture Compare A Register */
			mcPWM_CAPTCMPA_16B_tag CAPTCMPA1;		/* offset: 0x007A size: 16 bit */
																					/* Capture Control B Register */
			mcPWM_CAPTCTRLB_16B_tag CAPTCTRLB1; /* offset: 0x007C size: 16 bit */
																					/* Capture Compare B Register */
			mcPWM_CAPTCMPB_16B_tag CAPTCMPB1;		/* offset: 0x007E size: 16 bit */
																					/* Capture Control X Register */
			mcPWM_CAPTCTRLX_16B_tag CAPTCTRLX1; /* offset: 0x0080 size: 16 bit */
																					/* Capture Compare X Register */
			mcPWM_CAPTCMPX_16B_tag CAPTCMPX1;		/* offset: 0x0082 size: 16 bit */
																					/* Capture Value 0 Register */
			mcPWM_CVAL0_16B_tag CVAL01;					/* offset: 0x0084 size: 16 bit */
																					/* Capture Value 0 Cycle Register */
			mcPWM_CVAL0CYC_16B_tag CVAL0CYC1;		/* offset: 0x0086 size: 16 bit */
																					/* Capture Value 1 Register */
			mcPWM_CVAL1_16B_tag CVAL11;					/* offset: 0x0088 size: 16 bit */
																					/* Capture Value 1 Cycle Register */
			mcPWM_CVAL1CYC_16B_tag CVAL1CYC1;		/* offset: 0x008A size: 16 bit */
																					/* Capture Value 2 Register */
			mcPWM_CVAL2_16B_tag CVAL21;					/* offset: 0x008C size: 16 bit */
																					/* Capture Value 2 Cycle Register */
			mcPWM_CVAL2CYC_16B_tag CVAL2CYC1;		/* offset: 0x008E size: 16 bit */
																					/* Capture Value 3 Register */
			mcPWM_CVAL3_16B_tag CVAL31;					/* offset: 0x0090 size: 16 bit */
																					/* Capture Value 3 Cycle Register */
			mcPWM_CVAL3CYC_16B_tag CVAL3CYC1;		/* offset: 0x0092 size: 16 bit */
																					/* Capture Value 4 Register */
			mcPWM_CVAL4_16B_tag CVAL41;					/* offset: 0x0094 size: 16 bit */
																					/* Capture Value 4 Cycle Register */
			mcPWM_CVAL4CYC_16B_tag CVAL4CYC1;		/* offset: 0x0096 size: 16 bit */
																					/* Capture Value 5 Register */
			mcPWM_CVAL5_16B_tag CVAL51;					/* offset: 0x0098 size: 16 bit */
																					/* Capture Value 5 Cycle Register */
			mcPWM_CVAL5CYC_16B_tag CVAL5CYC1;		/* offset: 0x009A size: 16 bit */
			int8_t mcPWM_reserved_009C_I2[4];
			/* Counter Register */
			mcPWM_CNT_16B_tag CNT2;				/* offset: 0x00A0 size: 16 bit */
																		/* Initial Counter Register */
			mcPWM_INIT_16B_tag INIT2;			/* offset: 0x00A2 size: 16 bit */
																		/* Control 2 Register */
			mcPWM_CTRL2_16B_tag CTRL22;		/* offset: 0x00A4 size: 16 bit */
																		/* Control Register */
			mcPWM_CTRL1_16B_tag CTRL12;		/* offset: 0x00A6 size: 16 bit */
																		/* Value Register 0 */
			mcPWM_VAL_0_16B_tag VAL_02;		/* offset: 0x00A8 size: 16 bit */
																		/* Value Register 1 */
			mcPWM_VAL_1_16B_tag VAL_12;		/* offset: 0x00AA size: 16 bit */
																		/* Value Register 2 */
			mcPWM_VAL_2_16B_tag VAL_22;		/* offset: 0x00AC size: 16 bit */
																		/* Value Register 3 */
			mcPWM_VAL_3_16B_tag VAL_32;		/* offset: 0x00AE size: 16 bit */
																		/* Value Register 4 */
			mcPWM_VAL_4_16B_tag VAL_42;		/* offset: 0x00B0 size: 16 bit */
																		/* Value Register 5 */
			mcPWM_VAL_5_16B_tag VAL_52;		/* offset: 0x00B2 size: 16 bit */
																		/* Fractional Delay Register A */
			mcPWM_FRACA_16B_tag FRACA2;		/* offset: 0x00B4 size: 16 bit */
																		/* Fractional Delay Register B */
			mcPWM_FRACB_16B_tag FRACB2;		/* offset: 0x00B6 size: 16 bit */
																		/* Output Control Register */
			mcPWM_OCTRL_16B_tag OCTRL2;		/* offset: 0x00B8 size: 16 bit */
																		/* Status Register */
			mcPWM_STS_16B_tag STS2;				/* offset: 0x00BA size: 16 bit */
																		/* Interrupt Enable Registers */
			mcPWM_INTEN_16B_tag INTEN2;		/* offset: 0x00BC size: 16 bit */
																		/* DMA Enable Registers */
			mcPWM_DMAEN_16B_tag DMAEN2;		/* offset: 0x00BE size: 16 bit */
																		/* Output Trigger Control  Registers */
			mcPWM_TCTRL_16B_tag TCTRL2;		/* offset: 0x00C0 size: 16 bit */
																		/* Fault Disable Mapping   Registers */
			mcPWM_DISMAP_16B_tag DISMAP2; /* offset: 0x00C2 size: 16 bit */
																		/* Deadtime Count Register 0 */
			mcPWM_DTCNT0_16B_tag DTCNT02; /* offset: 0x00C4 size: 16 bit */
																		/* Deadtime Count Register 1 */
			mcPWM_DTCNT1_16B_tag DTCNT12; /* offset: 0x00C6 size: 16 bit */
																		/* Capture Control A Register */
			mcPWM_CAPTCTRLA_16B_tag CAPTCTRLA2; /* offset: 0x00C8 size: 16 bit */
																					/* Capture Compare A Register */
			mcPWM_CAPTCMPA_16B_tag CAPTCMPA2;		/* offset: 0x00CA size: 16 bit */
																					/* Capture Control B Register */
			mcPWM_CAPTCTRLB_16B_tag CAPTCTRLB2; /* offset: 0x00CC size: 16 bit */
																					/* Capture Compare B Register */
			mcPWM_CAPTCMPB_16B_tag CAPTCMPB2;		/* offset: 0x00CE size: 16 bit */
																					/* Capture Control X Register */
			mcPWM_CAPTCTRLX_16B_tag CAPTCTRLX2; /* offset: 0x00D0 size: 16 bit */
																					/* Capture Compare X Register */
			mcPWM_CAPTCMPX_16B_tag CAPTCMPX2;		/* offset: 0x00D2 size: 16 bit */
																					/* Capture Value 0 Register */
			mcPWM_CVAL0_16B_tag CVAL02;					/* offset: 0x00D4 size: 16 bit */
																					/* Capture Value 0 Cycle Register */
			mcPWM_CVAL0CYC_16B_tag CVAL0CYC2;		/* offset: 0x00D6 size: 16 bit */
																					/* Capture Value 1 Register */
			mcPWM_CVAL1_16B_tag CVAL12;					/* offset: 0x00D8 size: 16 bit */
																					/* Capture Value 1 Cycle Register */
			mcPWM_CVAL1CYC_16B_tag CVAL1CYC2;		/* offset: 0x00DA size: 16 bit */
																					/* Capture Value 2 Register */
			mcPWM_CVAL2_16B_tag CVAL22;					/* offset: 0x00DC size: 16 bit */
																					/* Capture Value 2 Cycle Register */
			mcPWM_CVAL2CYC_16B_tag CVAL2CYC2;		/* offset: 0x00DE size: 16 bit */
																					/* Capture Value 3 Register */
			mcPWM_CVAL3_16B_tag CVAL32;					/* offset: 0x00E0 size: 16 bit */
																					/* Capture Value 3 Cycle Register */
			mcPWM_CVAL3CYC_16B_tag CVAL3CYC2;		/* offset: 0x00E2 size: 16 bit */
																					/* Capture Value 4 Register */
			mcPWM_CVAL4_16B_tag CVAL42;					/* offset: 0x00E4 size: 16 bit */
																					/* Capture Value 4 Cycle Register */
			mcPWM_CVAL4CYC_16B_tag CVAL4CYC2;		/* offset: 0x00E6 size: 16 bit */
																					/* Capture Value 5 Register */
			mcPWM_CVAL5_16B_tag CVAL52;					/* offset: 0x00E8 size: 16 bit */
																					/* Capture Value 5 Cycle Register */
			mcPWM_CVAL5CYC_16B_tag CVAL5CYC2;		/* offset: 0x00EA size: 16 bit */
			int8_t mcPWM_reserved_00EC_I2[4];
			/* Counter Register */
			mcPWM_CNT_16B_tag CNT3;				/* offset: 0x00F0 size: 16 bit */
																		/* Initial Counter Register */
			mcPWM_INIT_16B_tag INIT3;			/* offset: 0x00F2 size: 16 bit */
																		/* Control 2 Register */
			mcPWM_CTRL2_16B_tag CTRL23;		/* offset: 0x00F4 size: 16 bit */
																		/* Control Register */
			mcPWM_CTRL1_16B_tag CTRL13;		/* offset: 0x00F6 size: 16 bit */
																		/* Value Register 0 */
			mcPWM_VAL_0_16B_tag VAL_03;		/* offset: 0x00F8 size: 16 bit */
																		/* Value Register 1 */
			mcPWM_VAL_1_16B_tag VAL_13;		/* offset: 0x00FA size: 16 bit */
																		/* Value Register 2 */
			mcPWM_VAL_2_16B_tag VAL_23;		/* offset: 0x00FC size: 16 bit */
																		/* Value Register 3 */
			mcPWM_VAL_3_16B_tag VAL_33;		/* offset: 0x00FE size: 16 bit */
																		/* Value Register 4 */
			mcPWM_VAL_4_16B_tag VAL_43;		/* offset: 0x0100 size: 16 bit */
																		/* Value Register 5 */
			mcPWM_VAL_5_16B_tag VAL_53;		/* offset: 0x0102 size: 16 bit */
																		/* Fractional Delay Register A */
			mcPWM_FRACA_16B_tag FRACA3;		/* offset: 0x0104 size: 16 bit */
																		/* Fractional Delay Register B */
			mcPWM_FRACB_16B_tag FRACB3;		/* offset: 0x0106 size: 16 bit */
																		/* Output Control Register */
			mcPWM_OCTRL_16B_tag OCTRL3;		/* offset: 0x0108 size: 16 bit */
																		/* Status Register */
			mcPWM_STS_16B_tag STS3;				/* offset: 0x010A size: 16 bit */
																		/* Interrupt Enable Registers */
			mcPWM_INTEN_16B_tag INTEN3;		/* offset: 0x010C size: 16 bit */
																		/* DMA Enable Registers */
			mcPWM_DMAEN_16B_tag DMAEN3;		/* offset: 0x010E size: 16 bit */
																		/* Output Trigger Control  Registers */
			mcPWM_TCTRL_16B_tag TCTRL3;		/* offset: 0x0110 size: 16 bit */
																		/* Fault Disable Mapping   Registers */
			mcPWM_DISMAP_16B_tag DISMAP3; /* offset: 0x0112 size: 16 bit */
																		/* Deadtime Count Register 0 */
			mcPWM_DTCNT0_16B_tag DTCNT03; /* offset: 0x0114 size: 16 bit */
																		/* Deadtime Count Register 1 */
			mcPWM_DTCNT1_16B_tag DTCNT13; /* offset: 0x0116 size: 16 bit */
																		/* Capture Control A Register */
			mcPWM_CAPTCTRLA_16B_tag CAPTCTRLA3; /* offset: 0x0118 size: 16 bit */
																					/* Capture Compare A Register */
			mcPWM_CAPTCMPA_16B_tag CAPTCMPA3;		/* offset: 0x011A size: 16 bit */
																					/* Capture Control B Register */
			mcPWM_CAPTCTRLB_16B_tag CAPTCTRLB3; /* offset: 0x011C size: 16 bit */
																					/* Capture Compare B Register */
			mcPWM_CAPTCMPB_16B_tag CAPTCMPB3;		/* offset: 0x011E size: 16 bit */
																					/* Capture Control X Register */
			mcPWM_CAPTCTRLX_16B_tag CAPTCTRLX3; /* offset: 0x0120 size: 16 bit */
																					/* Capture Compare X Register */
			mcPWM_CAPTCMPX_16B_tag CAPTCMPX3;		/* offset: 0x0122 size: 16 bit */
																					/* Capture Value 0 Register */
			mcPWM_CVAL0_16B_tag CVAL03;					/* offset: 0x0124 size: 16 bit */
																					/* Capture Value 0 Cycle Register */
			mcPWM_CVAL0CYC_16B_tag CVAL0CYC3;		/* offset: 0x0126 size: 16 bit */
																					/* Capture Value 1 Register */
			mcPWM_CVAL1_16B_tag CVAL13;					/* offset: 0x0128 size: 16 bit */
																					/* Capture Value 1 Cycle Register */
			mcPWM_CVAL1CYC_16B_tag CVAL1CYC3;		/* offset: 0x012A size: 16 bit */
																					/* Capture Value 2 Register */
			mcPWM_CVAL2_16B_tag CVAL23;					/* offset: 0x012C size: 16 bit */
																					/* Capture Value 2 Cycle Register */
			mcPWM_CVAL2CYC_16B_tag CVAL2CYC3;		/* offset: 0x012E size: 16 bit */
																					/* Capture Value 3 Register */
			mcPWM_CVAL3_16B_tag CVAL33;					/* offset: 0x0130 size: 16 bit */
																					/* Capture Value 3 Cycle Register */
			mcPWM_CVAL3CYC_16B_tag CVAL3CYC3;		/* offset: 0x0132 size: 16 bit */
																					/* Capture Value 4 Register */
			mcPWM_CVAL4_16B_tag CVAL43;					/* offset: 0x0134 size: 16 bit */
																					/* Capture Value 4 Cycle Register */
			mcPWM_CVAL4CYC_16B_tag CVAL4CYC3;		/* offset: 0x0136 size: 16 bit */
																					/* Capture Value 5 Register */
			mcPWM_CVAL5_16B_tag CVAL53;					/* offset: 0x0138 size: 16 bit */
																					/* Capture Value 5 Cycle Register */
			mcPWM_CVAL5CYC_16B_tag CVAL5CYC3;		/* offset: 0x013A size: 16 bit */
			int8_t mcPWM_reserved_013C_E2[4];
		};
	};
	/* Output Enable Register */
	mcPWM_OUTEN_16B_tag OUTEN;			 /* offset: 0x0140 size: 16 bit */
																	 /* Mask Register */
	mcPWM_MASK_16B_tag MASK;				 /* offset: 0x0142 size: 16 bit */
																	 /* Software Controlled Output Register */
	mcPWM_SWCOUT_16B_tag SWCOUT;		 /* offset: 0x0144 size: 16 bit */
																	 /* Deadtime Source Select Register */
	mcPWM_DTSRCSEL_16B_tag DTSRCSEL; /* offset: 0x0146 size: 16 bit */
																	 /* Master Control Register */
	mcPWM_MCTRL_16B_tag MCTRL;			 /* offset: 0x0148 size: 16 bit */
	int8_t mcPWM_reserved_014A[2];
	/* Fault Control Register */
	mcPWM_FCTRL_16B_tag FCTRL; /* offset: 0x014C size: 16 bit */
														 /* Fault Status Register */
	mcPWM_FSTS_16B_tag FSTS;	 /* offset: 0x014E size: 16 bit */
														 /* Fault Filter Register */
	mcPWM_FFILT_16B_tag FFILT; /* offset: 0x0150 size: 16 bit */
} mcPWM_tag;

#define mcPWM_A (*(volatile mcPWM_tag*)0xFFE24000UL)
#define mcPWM_B (*(volatile mcPWM_tag*)0xFFE28000UL)

/****************************************************************/
/*                                                              */
/* Module: LINFLEX  */
/*                                                              */
/****************************************************************/

typedef union { /* LIN Control Register */
	vuint32_t R;
	struct {
		vuint32_t : 16;
		vuint32_t CCD : 1;	/* Checksum Calculation Disable */
		vuint32_t CFD : 1;	/* Checksum Field Disable */
		vuint32_t LASE : 1; /* LIN Auto Synchronization Enable */
#ifndef USE_FIELD_ALIASES_LINFLEX
		vuint32_t AUTOWU : 1; /* Auto Wake Up */
#else
		vuint32_t AWUM : 1;				 /* deprecated name - please avoid */
#endif
		vuint32_t MBL : 4; /* Master Break Length */
		vuint32_t BF : 1;	 /* By-Pass Filter */
#ifndef USE_FIELD_ALIASES_LINFLEX
		vuint32_t SLFM : 1; /* Selftest Mode */
#else
		vuint32_t SFTM : 1;				 /* deprecated name - please avoid */
#endif
		vuint32_t LBKM : 1; /* Loopback Mode */
		vuint32_t MME : 1;	/* Master Mode Enable */
#ifndef USE_FIELD_ALIASES_LINFLEX
		vuint32_t SSBL : 1; /* Slave Mode Synch Break Length */
#else
		vuint32_t SSDT : 1;				 /* deprecated name - please avoid */
#endif
		vuint32_t RBLM : 1;	 /* Receiver Buffer Locked Mode */
		vuint32_t SLEEP : 1; /* Sleep Mode Request */
		vuint32_t INIT : 1;	 /* Initialization Mode Request */
	} B;
} LINFLEX_LINCR1_32B_tag;

typedef union { /* LIN Interrupt Enable Register */
	vuint32_t R;
	struct {
		vuint32_t : 16;
		vuint32_t SZIE : 1; /* Stuck at Zero Interrupt Enable */
		vuint32_t OCIE : 1; /* Output Compare Interrupt Enable */
		vuint32_t BEIE : 1; /* Bit Error Interrupt Enable */
		vuint32_t CEIE : 1; /* Checksum Error Interrupt Enable */
		vuint32_t HEIE : 1; /* Header Error Interrupt Enable */
		vuint32_t : 2;
		vuint32_t FEIE : 1;	 /* Frame Error Interrupt Enable */
		vuint32_t BOIE : 1;	 /* Buffer Overrun Error Interrupt Enable */
		vuint32_t LSIE : 1;	 /* LIN State Interrupt Enable */
		vuint32_t WUIE : 1;	 /* Wakeup Interrupt Enable */
		vuint32_t DBFIE : 1; /* Data Buffer Full Interrupt Enable */
#ifndef USE_FIELD_ALIASES_LINFLEX
		vuint32_t DBEIE_TOIE : 1; /* Data Buffer Empty Interrupt Enable */
#else
		vuint32_t DBEIE : 1;			 /* deprecated name - please avoid */
#endif
		vuint32_t DRIE : 1; /* Data Reception complete Interrupt Enable */
		vuint32_t DTIE : 1; /* Data Transmitted Interrupt Enable */
		vuint32_t HRIE : 1; /* Header Received Interrupt Enable */
	} B;
} LINFLEX_LINIER_32B_tag;

typedef union { /* LIN Status Register */
	vuint32_t R;
	struct {
		vuint32_t : 16;
		vuint32_t LINS : 4; /* LIN State */
		vuint32_t : 2;
		vuint32_t RMB : 1; /* Release Message Buffer */
		vuint32_t : 1;
#ifndef USE_FIELD_ALIASES_LINFLEX
		vuint32_t RXBUSY : 1; /* Receiver Busy Flag */
#else
		vuint32_t RBSY : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_LINFLEX
		vuint32_t RDI : 1; /* LIN Receive Signal */
#else
		vuint32_t RPS : 1;				 /* deprecated name - please avoid */
#endif
		vuint32_t WUF : 1;	/* Wake Up Flag */
		vuint32_t DBFF : 1; /* Data Buffer Full Flag */
		vuint32_t DBEF : 1; /* Data Buffer Empty Flag */
		vuint32_t DRF : 1;	/* Data Reception Completed Flag */
		vuint32_t DTF : 1;	/* Data Transmission Completed Flag */
		vuint32_t HRF : 1;	/* Header Received Flag */
	} B;
} LINFLEX_LINSR_32B_tag;

typedef union { /* LIN Error Status Register */
	vuint32_t R;
	struct {
		vuint32_t : 16;
		vuint32_t SZF : 1;	/* Stuck at Zero Flag */
		vuint32_t OCF : 1;	/* Output Compare Flag */
		vuint32_t BEF : 1;	/* Bit Error Flag */
		vuint32_t CEF : 1;	/* Checksum Error Flag */
		vuint32_t SFEF : 1; /* Sync Field Error Flag */
#ifndef USE_FIELD_ALIASES_LINFLEX
		vuint32_t SDEF : 1; /* Sync Delimiter Error Flag */
#else
		vuint32_t BDEF : 1;				 /* deprecated name - please avoid */
#endif
		vuint32_t IDPEF : 1; /* ID Parity Error Flag */
		vuint32_t FEF : 1;	 /* Framing Error Flag */
		vuint32_t BOF : 1;	 /* Buffer Overrun Flag */
		vuint32_t : 6;
		vuint32_t NF : 1; /* Noise Flag */
	} B;
} LINFLEX_LINESR_32B_tag;

typedef union { /* UART Mode Control Register */
	vuint32_t R;
	struct {
		vuint32_t : 16;
		vuint32_t TDFL_TFC : 3;	 /* Transmitter Data Field Length/TX FIFO Counter */
		vuint32_t RDFL_RFC0 : 3; /* Reception Data Field Length/RX FIFO Counter */
		vuint32_t RFBM : 1;			 /* RX FIFO/ Buffer Mode */
		vuint32_t TFBM : 1;			 /* TX FIFO/ Buffer Mode */
		vuint32_t WL1 : 1;			 /* Word Length in UART mode - bit 1 */
		vuint32_t PC1 : 1;			 /* Parity Check - bit 1 */
		vuint32_t RXEN : 1;			 /* Receiver Enable */
		vuint32_t TXEN : 1;			 /* Transmitter Enable */
#ifndef USE_FIELD_ALIASES_LINFLEX
		vuint32_t PC0 : 1; /* Parity Check - bit 0 */
#else
		vuint32_t OP : 1;					 /* deprecated name - please avoid */
#endif
		vuint32_t PCE : 1; /* Parity Control Enable */
#ifndef USE_FIELD_ALIASES_LINFLEX
		vuint32_t WL0 : 1; /* Word Length in UART Mode - bit 0 */
#else
		vuint32_t WL : 1;					 /* deprecated name - please avoid */
#endif
		vuint32_t UART : 1; /* UART Mode */
	} B;
} LINFLEX_UARTCR_32B_tag;

typedef union { /* UART Mode Status Register */
	vuint32_t R;
	struct {
		vuint32_t : 16;
		vuint32_t SZF : 1; /* Stuck at Zero Flag */
		vuint32_t OCF : 1; /* Output Compare Flag */
		vuint32_t PE : 4;	 /* Parity Error Flag */
		vuint32_t RMB : 1; /* Release Message Buffer */
		vuint32_t FEF : 1; /* Framing Error Flag */
		vuint32_t BOF : 1; /* Buffer Overrun Flag */
		vuint32_t RDI : 1; /* Receiver Data Input Signal */
		vuint32_t WUF : 1; /* Wakeup Flag */
		vuint32_t : 1;
		vuint32_t TO : 1; /* Time Out */
#ifndef USE_FIELD_ALIASES_LINFLEX
		vuint32_t
				DRF_RFE : 1; /* Data Reception Completed Flag/RX FIFO Empty Flag */
#else
		vuint32_t DRF : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_LINFLEX
		vuint32_t
				DTF_TFF : 1; /* Data Transmission Completed Flag/TX FIFO Full Flag */
#else
		vuint32_t DTF : 1;				 /* deprecated name - please avoid */
#endif
		vuint32_t NF : 1; /* Noise Flag */
	} B;
} LINFLEX_UARTSR_32B_tag;

typedef union { /* LIN Time-Out Control Status Register */
	vuint32_t R;
	struct {
		vuint32_t : 21;
#ifndef USE_FIELD_ALIASES_LINFLEX
		vuint32_t MODE : 1; /* Time-out Counter Mode */
#else
		vuint32_t LTOM : 1;				 /* deprecated name - please avoid */
#endif
		vuint32_t IOT : 1;	/* Idle on Timeout */
		vuint32_t TOCE : 1; /* Time-Out Counter Enable */
		vuint32_t CNT : 8;	/* Counter Value */
	} B;
} LINFLEX_LINTCSR_32B_tag;

typedef union { /* LIN Output Compare Register */
	vuint32_t R;
	struct {
		vuint32_t : 16;
		vuint32_t OC2 : 8; /* Output Compare Value 2 */
		vuint32_t OC1 : 8; /* Output Compare Value 1 */
	} B;
} LINFLEX_LINOCR_32B_tag;

typedef union { /* LIN Time-Out Control Register */
	vuint32_t R;
	struct {
		vuint32_t : 20;
		vuint32_t RTO : 4; /* Response Time-Out Value */
		vuint32_t : 1;
		vuint32_t HTO : 7; /* Header Time-Out Value */
	} B;
} LINFLEX_LINTOCR_32B_tag;

typedef union { /* LIN Fractional Baud Rate Register */
	vuint32_t R;
	struct {
		vuint32_t : 28;
#ifndef USE_FIELD_ALIASES_LINFLEX
		vuint32_t FBR : 4; /* Fractional Baud Rates */
#else
		vuint32_t DIV_F : 4;			 /* deprecated name - please avoid */
#endif
	} B;
} LINFLEX_LINFBRR_32B_tag;

typedef union { /* LIN Integer Baud Rate Register */
	vuint32_t R;
	struct {
		vuint32_t : 13;
#ifndef USE_FIELD_ALIASES_LINFLEX
		vuint32_t IBR : 19; /* Integer Baud Rates */
#else
		vuint32_t DIV_M : 19;			 /* deprecated name - please avoid */
#endif
	} B;
} LINFLEX_LINIBRR_32B_tag;

typedef union { /* LIN Checksum Field Register */
	vuint32_t R;
	struct {
		vuint32_t : 24;
		vuint32_t CF : 8; /* Checksum Bits */
	} B;
} LINFLEX_LINCFR_32B_tag;

typedef union { /* LIN Control Register 2 */
	vuint32_t R;
	struct {
		vuint32_t : 17;
		vuint32_t IOBE : 1; /* Idle on Bit Error */
		vuint32_t IOPE : 1; /* Idle on Identifier Parity Error */
		vuint32_t WURQ : 1; /* Wakeup Generate Request */
		vuint32_t DDRQ : 1; /* Data Discard Request */
		vuint32_t DTRQ : 1; /* Data Transmission Request */
		vuint32_t ABRQ : 1; /* Abort Request */
		vuint32_t HTRQ : 1; /* Header Transmission Request */
		vuint32_t : 8;
	} B;
} LINFLEX_LINCR2_32B_tag;

typedef union { /* Buffer Identifier Register */
	vuint32_t R;
	struct {
		vuint32_t : 16;
		vuint32_t DFL : 6; /* Data Field Length */
		vuint32_t DIR : 1; /* Direction */
		vuint32_t CCS : 1; /* Classic Checksum */
		vuint32_t : 2;
		vuint32_t ID : 6; /* Identifier */
	} B;
} LINFLEX_BIDR_32B_tag;

typedef union { /* Buffer Data Register Least Significant */
	vuint32_t R;
	struct {
		vuint32_t DATA3 : 8; /* Data3 */
		vuint32_t DATA2 : 8; /* Data2 */
		vuint32_t DATA1 : 8; /* Data1 */
		vuint32_t DATA0 : 8; /* Data0 */
	} B;
} LINFLEX_BDRL_32B_tag;

typedef union { /* Buffer Data Register Most Significant */
	vuint32_t R;
	struct {
		vuint32_t DATA7 : 8; /* Data7 */
		vuint32_t DATA6 : 8; /* Data6 */
		vuint32_t DATA5 : 8; /* Data5 */
		vuint32_t DATA4 : 8; /* Data4 */
	} B;
} LINFLEX_BDRM_32B_tag;

typedef union { /* Identifier Filter Enable Register */
	vuint32_t R;
	struct {
		vuint32_t : 24;
		vuint32_t FACT : 8; /* Filter Active */
	} B;
} LINFLEX_IFER_32B_tag;

typedef union { /* Identifier Filter Match Index */
	vuint32_t R;
	struct {
		vuint32_t : 28;
		vuint32_t IFMI_IFMI : 4; /* Filter Match Index */
	} B;
} LINFLEX_IFMI_32B_tag;

typedef union { /* Identifier Filter Mode Register */
	vuint32_t R;
	struct {
		vuint32_t : 28;
		vuint32_t IFM : 4; /* Filter Mode */
	} B;
} LINFLEX_IFMR_32B_tag;

/* Register layout for all registers IFCR... */

typedef union { /* Identifier Filter Control Register */
	vuint32_t R;
	struct {
		vuint32_t : 16;
		vuint32_t DFL : 6; /* Data Field Length */
		vuint32_t DIR : 1; /* Direction */
		vuint32_t CCS : 1; /* Classic Checksum */
		vuint32_t : 2;
		vuint32_t ID : 6; /* Identifier */
	} B;
} LINFLEX_IFCR_32B_tag;

typedef union { /* Global Control Register */
	vuint32_t R;
	struct {
		vuint32_t : 26;
		vuint32_t TDFBM : 1; /* Transmit Data First Bit MSB */
		vuint32_t RDFBM : 1; /* Received Data First Bit MSB */
		vuint32_t TDLIS : 1; /* Transmit Data Level Inversion Selection */
		vuint32_t RDLIS : 1; /* Received Data Level Inversion Selection */
		vuint32_t STOP : 1;	 /* 1/2 stop bit configuration */
		vuint32_t SR : 1;		 /* Soft Reset */
	} B;
} LINFLEX_GCR_32B_tag;

typedef union { /* UART Preset Time Out Register */
	vuint32_t R;
	struct {
		vuint32_t : 20;
		vuint32_t PTO : 12; /* Preset Time Out */
	} B;
} LINFLEX_UARTPTO_32B_tag;

typedef union { /* UART Current Time Out Register */
	vuint32_t R;
	struct {
		vuint32_t : 20;
		vuint32_t CTO : 12; /* Current Time Out */
	} B;
} LINFLEX_UARTCTO_32B_tag;

typedef union { /* DMA TX Enable Register */
	vuint32_t R;
	struct {
		vuint32_t : 17;
		vuint32_t DTE : 15; /* DMA Tx channel Enable */
	} B;
} LINFLEX_DMATXE_32B_tag;

typedef union { /* DMA RX Enable Register */
	vuint32_t R;
	struct {
		vuint32_t : 17;
		vuint32_t DRE : 15; /* DMA Rx channel Enable */
	} B;
} LINFLEX_DMARXE_32B_tag;

typedef struct LINFLEX_tag {			 /* start of LINFLEX_tag */
																	 /* LIN Control Register */
	LINFLEX_LINCR1_32B_tag LINCR1;	 /* offset: 0x0000 size: 32 bit */
																	 /* LIN Interrupt Enable Register */
	LINFLEX_LINIER_32B_tag LINIER;	 /* offset: 0x0004 size: 32 bit */
																	 /* LIN Status Register */
	LINFLEX_LINSR_32B_tag LINSR;		 /* offset: 0x0008 size: 32 bit */
																	 /* LIN Error Status Register */
	LINFLEX_LINESR_32B_tag LINESR;	 /* offset: 0x000C size: 32 bit */
																	 /* UART Mode Control Register */
	LINFLEX_UARTCR_32B_tag UARTCR;	 /* offset: 0x0010 size: 32 bit */
																	 /* UART Mode Status Register */
	LINFLEX_UARTSR_32B_tag UARTSR;	 /* offset: 0x0014 size: 32 bit */
																	 /* LIN Time-Out Control Status Register */
	LINFLEX_LINTCSR_32B_tag LINTCSR; /* offset: 0x0018 size: 32 bit */
																	 /* LIN Output Compare Register */
	LINFLEX_LINOCR_32B_tag LINOCR;	 /* offset: 0x001C size: 32 bit */
																	 /* LIN Time-Out Control Register */
	LINFLEX_LINTOCR_32B_tag LINTOCR; /* offset: 0x0020 size: 32 bit */
																	 /* LIN Fractional Baud Rate Register */
	LINFLEX_LINFBRR_32B_tag LINFBRR; /* offset: 0x0024 size: 32 bit */
																	 /* LIN Integer Baud Rate Register */
	LINFLEX_LINIBRR_32B_tag LINIBRR; /* offset: 0x0028 size: 32 bit */
																	 /* LIN Checksum Field Register */
	LINFLEX_LINCFR_32B_tag LINCFR;	 /* offset: 0x002C size: 32 bit */
																	 /* LIN Control Register 2 */
	LINFLEX_LINCR2_32B_tag LINCR2;	 /* offset: 0x0030 size: 32 bit */
																	 /* Buffer Identifier Register */
	LINFLEX_BIDR_32B_tag BIDR;			 /* offset: 0x0034 size: 32 bit */
																	 /* Buffer Data Register Least Significant */
	LINFLEX_BDRL_32B_tag BDRL;			 /* offset: 0x0038 size: 32 bit */
																	 /* Buffer Data Register Most Significant */
	LINFLEX_BDRM_32B_tag BDRM;			 /* offset: 0x003C size: 32 bit */
																	 /* Identifier Filter Enable Register */
	LINFLEX_IFER_32B_tag IFER;			 /* offset: 0x0040 size: 32 bit */
																	 /* Identifier Filter Match Index */
	LINFLEX_IFMI_32B_tag IFMI;			 /* offset: 0x0044 size: 32 bit */
																	 /* Identifier Filter Mode Register */
	LINFLEX_IFMR_32B_tag IFMR;			 /* offset: 0x0048 size: 32 bit */
	union {
		/* Identifier Filter Control Register */
		LINFLEX_IFCR_32B_tag IFCR[8]; /* offset: 0x004C  (0x0004 x 8) */

		struct {
			/* Identifier Filter Control Register */
			LINFLEX_IFCR_32B_tag IFCR0; /* offset: 0x004C size: 32 bit */
			LINFLEX_IFCR_32B_tag IFCR1; /* offset: 0x0050 size: 32 bit */
			LINFLEX_IFCR_32B_tag IFCR2; /* offset: 0x0054 size: 32 bit */
			LINFLEX_IFCR_32B_tag IFCR3; /* offset: 0x0058 size: 32 bit */
			LINFLEX_IFCR_32B_tag IFCR4; /* offset: 0x005C size: 32 bit */
			LINFLEX_IFCR_32B_tag IFCR5; /* offset: 0x0060 size: 32 bit */
			LINFLEX_IFCR_32B_tag IFCR6; /* offset: 0x0064 size: 32 bit */
			LINFLEX_IFCR_32B_tag IFCR7; /* offset: 0x0068 size: 32 bit */
		};
	};
	int8_t LINFLEX_reserved_006C[32];
	/* Global Control Register */
	LINFLEX_GCR_32B_tag GCR;				 /* offset: 0x008C size: 32 bit */
																	 /* UART Preset Time Out Register */
	LINFLEX_UARTPTO_32B_tag UARTPTO; /* offset: 0x0090 size: 32 bit */
																	 /* UART Current Time Out Register */
	LINFLEX_UARTCTO_32B_tag UARTCTO; /* offset: 0x0094 size: 32 bit */
																	 /* DMA TX Enable Register */
	LINFLEX_DMATXE_32B_tag DMATXE;	 /* offset: 0x0098 size: 32 bit */
																	 /* DMA RX Enable Register */
	LINFLEX_DMARXE_32B_tag DMARXE;	 /* offset: 0x009C size: 32 bit */
} LINFLEX_tag;

#define LINFLEX0 (*(volatile LINFLEX_tag*)0xFFE40000UL)
#define LINFLEX1 (*(volatile LINFLEX_tag*)0xFFE44000UL)

/****************************************************************/
/*                                                              */
/* Module: CRC  */
/*                                                              */
/****************************************************************/

/* Register layout for all registers CFG... */

typedef union { /* CRC_CFG - CRC Configuration register */
	vuint32_t R;
	vuint8_t BYTE[4];	 /* individual bytes can be accessed */
	vuint16_t HALF[2]; /* individual halfwords can be accessed */
	vuint32_t WORD;		 /* individual words can be accessed */
	struct {
		vuint32_t : 29;
		vuint32_t POLYG : 1; /* Polynomal selection 0- CRC-CCITT, 1- CRC-CRC-32 INV
														selection */
		vuint32_t SWAP : 1;	 /* SWAP selection */
		vuint32_t INV : 1;	 /* INV selection */
	} B;
} CRC_CFG_32B_tag;

/* Register layout for all registers INP... */

typedef union { /* CRC_INP - CRC Input register */
	vuint32_t R;
	vuint8_t BYTE[4];	 /* individual bytes can be accessed */
	vuint16_t HALF[2]; /* individual halfwords can be accessed */
	vuint32_t WORD;		 /* individual words can be accessed */
} CRC_INP_32B_tag;

/* Register layout for all registers CSTAT... */

typedef union { /* CRC_STATUS - CRC Status register */
	vuint32_t R;
	vuint8_t BYTE[4];	 /* individual bytes can be accessed */
	vuint16_t HALF[2]; /* individual halfwords can be accessed */
	vuint32_t WORD;		 /* individual words can be accessed */
} CRC_CSTAT_32B_tag;

/* Register layout for all registers OUTP... */

typedef union { /* CRC_STATUS - CRC OUTPUT register */
	vuint32_t R;
	vuint8_t BYTE[4];	 /* individual bytes can be accessed */
	vuint16_t HALF[2]; /* individual halfwords can be accessed */
	vuint32_t WORD;		 /* individual words can be accessed */
} CRC_OUTP_32B_tag;

typedef struct CRC_CNTX_struct_tag {
	/* CRC_CFG - CRC Configuration register */
	CRC_CFG_32B_tag CFG;		 /* relative offset: 0x0000 */
													 /* CRC_INP - CRC Input register */
	CRC_INP_32B_tag INP;		 /* relative offset: 0x0004 */
													 /* CRC_STATUS - CRC Status register */
	CRC_CSTAT_32B_tag CSTAT; /* relative offset: 0x0008 */
													 /* CRC_STATUS - CRC OUTPUT register */
	CRC_OUTP_32B_tag OUTP;	 /* relative offset: 0x000C */

} CRC_CNTX_tag;

typedef struct CRC_struct_tag { /* start of CRC_tag */
	union {
		/*  Register set CNTX */
		CRC_CNTX_tag CNTX[3]; /* offset: 0x0000  (0x0010 x 3) */

		struct {
			/* CRC_CFG - CRC Configuration register */
			CRC_CFG_32B_tag CFG0;			/* offset: 0x0000 size: 32 bit */
																/* CRC_INP - CRC Input register */
			CRC_INP_32B_tag INP0;			/* offset: 0x0004 size: 32 bit */
																/* CRC_STATUS - CRC Status register */
			CRC_CSTAT_32B_tag CSTAT0; /* offset: 0x0008 size: 32 bit */
																/* CRC_STATUS - CRC OUTPUT register */
			CRC_OUTP_32B_tag OUTP0;		/* offset: 0x000C size: 32 bit */
																/* CRC_CFG - CRC Configuration register */
			CRC_CFG_32B_tag CFG1;			/* offset: 0x0010 size: 32 bit */
																/* CRC_INP - CRC Input register */
			CRC_INP_32B_tag INP1;			/* offset: 0x0014 size: 32 bit */
																/* CRC_STATUS - CRC Status register */
			CRC_CSTAT_32B_tag CSTAT1; /* offset: 0x0018 size: 32 bit */
																/* CRC_STATUS - CRC OUTPUT register */
			CRC_OUTP_32B_tag OUTP1;		/* offset: 0x001C size: 32 bit */
																/* CRC_CFG - CRC Configuration register */
			CRC_CFG_32B_tag CFG2;			/* offset: 0x0020 size: 32 bit */
																/* CRC_INP - CRC Input register */
			CRC_INP_32B_tag INP2;			/* offset: 0x0024 size: 32 bit */
																/* CRC_STATUS - CRC Status register */
			CRC_CSTAT_32B_tag CSTAT2; /* offset: 0x0028 size: 32 bit */
																/* CRC_STATUS - CRC OUTPUT register */
			CRC_OUTP_32B_tag OUTP2;		/* offset: 0x002C size: 32 bit */
		};
	};
} CRC_tag;

#define CRC (*(volatile CRC_tag*)0xFFE68000UL)

/****************************************************************/
/*                                                              */
/* Module: FCCU  */
/*                                                              */
/****************************************************************/

typedef union { /* FCCU Control Register */
	vuint32_t R;
	struct {
		vuint32_t : 23;
		vuint32_t NVML : 1; /* NVM configuration loaded */
		vuint32_t OPS : 2;	/* Operation status */
		vuint32_t : 1;
		vuint32_t OPR : 5; /* Operation run */
	} B;
} FCCU_CTRL_32B_tag;

typedef union { /* FCCU CTRL Key Register */
	vuint32_t R;
} FCCU_CTRLK_32B_tag;

typedef union { /* FCCU Configuration Register */
	vuint32_t R;
	struct {
		vuint32_t : 10;
		vuint32_t RCCE1 : 1; /* RCC1 enable */
		vuint32_t RCCE0 : 1; /* RCC0 enable */
		vuint32_t SMRT : 4;	 /* Safe Mode Request Timer */
		vuint32_t : 4;
		vuint32_t CM : 1;	 /* Config mode */
		vuint32_t SM : 1;	 /* Switching mode */
		vuint32_t PS : 1;	 /* Polarity Selection */
		vuint32_t FOM : 3; /* Fault Output Mode Selection */
		vuint32_t FOP : 6; /* Fault Output Prescaler */
	} B;
} FCCU_CFG_32B_tag;

typedef union { /* FCCU CF Configuration Register 0 */
	vuint32_t R;
	struct {
		vuint32_t CFC31 : 1; /* CF 31 configuration */
		vuint32_t CFC30 : 1; /* CF 30 configuration */
		vuint32_t CFC29 : 1; /* CF 29 configuration */
		vuint32_t CFC28 : 1; /* CF 28 configuration */
		vuint32_t CFC27 : 1; /* CF 27 configuration */
		vuint32_t CFC26 : 1; /* CF 26 configuration */
		vuint32_t CFC25 : 1; /* CF 25 configuration */
		vuint32_t CFC24 : 1; /* CF 24 configuration */
		vuint32_t CFC23 : 1; /* CF 23 configuration */
		vuint32_t CFC22 : 1; /* CF 22 configuration */
		vuint32_t CFC21 : 1; /* CF 21 configuration */
		vuint32_t CFC20 : 1; /* CF 20 configuration */
		vuint32_t CFC19 : 1; /* CF 19 configuration */
		vuint32_t CFC18 : 1; /* CF 18 configuration */
		vuint32_t CFC17 : 1; /* CF 17 configuration */
		vuint32_t CFC16 : 1; /* CF 16 configuration */
		vuint32_t CFC15 : 1; /* CF 15 configuration */
		vuint32_t CFC14 : 1; /* CF 14 configuration */
		vuint32_t CFC13 : 1; /* CF 13 configuration */
		vuint32_t CFC12 : 1; /* CF 12 configuration */
		vuint32_t CFC11 : 1; /* CF 11 configuration */
		vuint32_t CFC10 : 1; /* CF 10 configuration */
		vuint32_t CFC9 : 1;	 /* CF 9 configuration */
		vuint32_t CFC8 : 1;	 /* CF 8 configuration */
		vuint32_t CFC7 : 1;	 /* CF 7 configuration */
		vuint32_t CFC6 : 1;	 /* CF 6 configuration */
		vuint32_t CFC5 : 1;	 /* CF 5 configuration */
		vuint32_t CFC4 : 1;	 /* CF 4 configuration */
		vuint32_t CFC3 : 1;	 /* CF 3 configuration */
		vuint32_t CFC2 : 1;	 /* CF 2 configuration */
		vuint32_t CFC1 : 1;	 /* CF 1 configuration */
		vuint32_t CFC0 : 1;	 /* CF 0 configuration */
	} B;
} FCCU_CF_CFG0_32B_tag;

typedef union { /* FCCU CF Configuration Register 1 */
	vuint32_t R;
	struct {
		vuint32_t CFC63 : 1; /* CF 63 configuration */
		vuint32_t CFC62 : 1; /* CF 62 configuration */
		vuint32_t CFC61 : 1; /* CF 61 configuration */
		vuint32_t CFC60 : 1; /* CF 60 configuration */
		vuint32_t CFC59 : 1; /* CF 59 configuration */
		vuint32_t CFC58 : 1; /* CF 58 configuration */
		vuint32_t CFC57 : 1; /* CF 57 configuration */
		vuint32_t CFC56 : 1; /* CF 56 configuration */
		vuint32_t CFC55 : 1; /* CF 55 configuration */
		vuint32_t CFC54 : 1; /* CF 54 configuration */
		vuint32_t CFC53 : 1; /* CF 53 configuration */
		vuint32_t CFC52 : 1; /* CF 52 configuration */
		vuint32_t CFC51 : 1; /* CF 51 configuration */
		vuint32_t CFC50 : 1; /* CF 50 configuration */
		vuint32_t CFC49 : 1; /* CF 49 configuration */
		vuint32_t CFC48 : 1; /* CF 48 configuration */
		vuint32_t CFC47 : 1; /* CF 47 configuration */
		vuint32_t CFC46 : 1; /* CF 46 configuration */
		vuint32_t CFC45 : 1; /* CF 45 configuration */
		vuint32_t CFC44 : 1; /* CF 44 configuration */
		vuint32_t CFC43 : 1; /* CF 43 configuration */
		vuint32_t CFC42 : 1; /* CF 42 configuration */
		vuint32_t CFC41 : 1; /* CF 41 configuration */
		vuint32_t CFC40 : 1; /* CF 40 configuration */
		vuint32_t CFC39 : 1; /* CF 39 configuration */
		vuint32_t CFC38 : 1; /* CF 38 configuration */
		vuint32_t CFC37 : 1; /* CF 37 configuration */
		vuint32_t CFC36 : 1; /* CF 36 configuration */
		vuint32_t CFC35 : 1; /* CF 35 configuration */
		vuint32_t CFC34 : 1; /* CF 34 configuration */
		vuint32_t CFC33 : 1; /* CF 33 configuration */
		vuint32_t CFC32 : 1; /* CF 32 configuration */
	} B;
} FCCU_CF_CFG1_32B_tag;

typedef union { /* FCCU CF Configuration Register 2 */
	vuint32_t R;
	struct {
		vuint32_t CFC95 : 1; /* CF 95 configuration */
		vuint32_t CFC94 : 1; /* CF 94 configuration */
		vuint32_t CFC93 : 1; /* CF 93 configuration */
		vuint32_t CFC92 : 1; /* CF 92 configuration */
		vuint32_t CFC91 : 1; /* CF 91 configuration */
		vuint32_t CFC90 : 1; /* CF 90 configuration */
		vuint32_t CFC89 : 1; /* CF 89 configuration */
		vuint32_t CFC88 : 1; /* CF 88 configuration */
		vuint32_t CFC87 : 1; /* CF 87 configuration */
		vuint32_t CFC86 : 1; /* CF 86 configuration */
		vuint32_t CFC85 : 1; /* CF 85 configuration */
		vuint32_t CFC84 : 1; /* CF 84 configuration */
		vuint32_t CFC83 : 1; /* CF 83 configuration */
		vuint32_t CFC82 : 1; /* CF 82 configuration */
		vuint32_t CFC81 : 1; /* CF 81 configuration */
		vuint32_t CFC80 : 1; /* CF 80 configuration */
		vuint32_t CFC79 : 1; /* CF 79 configuration */
		vuint32_t CFC78 : 1; /* CF 78 configuration */
		vuint32_t CFC77 : 1; /* CF 77 configuration */
		vuint32_t CFC76 : 1; /* CF 76 configuration */
		vuint32_t CFC75 : 1; /* CF 75 configuration */
		vuint32_t CFC74 : 1; /* CF 74 configuration */
		vuint32_t CFC73 : 1; /* CF 73 configuration */
		vuint32_t CFC72 : 1; /* CF 72 configuration */
		vuint32_t CFC71 : 1; /* CF 71 configuration */
		vuint32_t CFC70 : 1; /* CF 70 configuration */
		vuint32_t CFC69 : 1; /* CF 69 configuration */
		vuint32_t CFC68 : 1; /* CF 68 configuration */
		vuint32_t CFC67 : 1; /* CF 67 configuration */
		vuint32_t CFC66 : 1; /* CF 66 configuration */
		vuint32_t CFC65 : 1; /* CF 65 configuration */
		vuint32_t CFC64 : 1; /* CF 64 configuration */
	} B;
} FCCU_CF_CFG2_32B_tag;

typedef union { /* FCCU CF Configuration Register 3 */
	vuint32_t R;
	struct {
		vuint32_t CFC127 : 1; /* CF 127 configuration */
		vuint32_t CFC126 : 1; /* CF 126 configuration */
		vuint32_t CFC125 : 1; /* CF 125 configuration */
		vuint32_t CFC124 : 1; /* CF 124 configuration */
		vuint32_t CFC123 : 1; /* CF 123 configuration */
		vuint32_t CFC122 : 1; /* CF 122 configuration */
		vuint32_t CFC121 : 1; /* CF 121 configuration */
		vuint32_t CFC120 : 1; /* CF 120 configuration */
		vuint32_t CFC119 : 1; /* CF 119 configuration */
		vuint32_t CFC118 : 1; /* CF 118 configuration */
		vuint32_t CFC117 : 1; /* CF 117 configuration */
		vuint32_t CFC116 : 1; /* CF 116 configuration */
		vuint32_t CFC115 : 1; /* CF 115 configuration */
		vuint32_t CFC114 : 1; /* CF 114 configuration */
		vuint32_t CFC113 : 1; /* CF 113 configuration */
		vuint32_t CFC112 : 1; /* CF 112 configuration */
		vuint32_t CFC111 : 1; /* CF 111 configuration */
		vuint32_t CFC110 : 1; /* CF 110 configuration */
		vuint32_t CFC109 : 1; /* CF 109 configuration */
		vuint32_t CFC108 : 1; /* CF 108 configuration */
		vuint32_t CFC107 : 1; /* CF 107 configuration */
		vuint32_t CFC106 : 1; /* CF 106 configuration */
		vuint32_t CFC105 : 1; /* CF 105 configuration */
		vuint32_t CFC104 : 1; /* CF 104 configuration */
		vuint32_t CFC103 : 1; /* CF 103 configuration */
		vuint32_t CFC102 : 1; /* CF 102 configuration */
		vuint32_t CFC101 : 1; /* CF 101 configuration */
		vuint32_t CFC100 : 1; /* CF 100 configuration */
		vuint32_t CFC99 : 1;	/* CF 99 configuration */
		vuint32_t CFC98 : 1;	/* CF 98 configuration */
		vuint32_t CFC97 : 1;	/* CF 97 configuration */
		vuint32_t CFC96 : 1;	/* CF 96 configuration */
	} B;
} FCCU_CF_CFG3_32B_tag;

typedef union { /* FCCU NCF Configuration Register 0 */
	vuint32_t R;
	struct {
		vuint32_t NCFC31 : 1; /* NCF 31 configuration */
		vuint32_t NCFC30 : 1; /* NCF 30 configuration */
		vuint32_t NCFC29 : 1; /* NCF 29 configuration */
		vuint32_t NCFC28 : 1; /* NCF 28 configuration */
		vuint32_t NCFC27 : 1; /* NCF 27 configuration */
		vuint32_t NCFC26 : 1; /* NCF 26 configuration */
		vuint32_t NCFC25 : 1; /* NCF 25 configuration */
		vuint32_t NCFC24 : 1; /* NCF 24 configuration */
		vuint32_t NCFC23 : 1; /* NCF 23 configuration */
		vuint32_t NCFC22 : 1; /* NCF 22 configuration */
		vuint32_t NCFC21 : 1; /* NCF 21 configuration */
		vuint32_t NCFC20 : 1; /* NCF 20 configuration */
		vuint32_t NCFC19 : 1; /* NCF 19 configuration */
		vuint32_t NCFC18 : 1; /* NCF 18 configuration */
		vuint32_t NCFC17 : 1; /* NCF 17 configuration */
		vuint32_t NCFC16 : 1; /* NCF 16 configuration */
		vuint32_t NCFC15 : 1; /* NCF 15 configuration */
		vuint32_t NCFC14 : 1; /* NCF 14 configuration */
		vuint32_t NCFC13 : 1; /* NCF 13 configuration */
		vuint32_t NCFC12 : 1; /* NCF 12 configuration */
		vuint32_t NCFC11 : 1; /* NCF 11 configuration */
		vuint32_t NCFC10 : 1; /* NCF 10 configuration */
		vuint32_t NCFC9 : 1;	/* NCF 9 configuration */
		vuint32_t NCFC8 : 1;	/* NCF 8 configuration */
		vuint32_t NCFC7 : 1;	/* NCF 7 configuration */
		vuint32_t NCFC6 : 1;	/* NCF 6 configuration */
		vuint32_t NCFC5 : 1;	/* NCF 5 configuration */
		vuint32_t NCFC4 : 1;	/* NCF 4 configuration */
		vuint32_t NCFC3 : 1;	/* NCF 3 configuration */
		vuint32_t NCFC2 : 1;	/* NCF 2 configuration */
		vuint32_t NCFC1 : 1;	/* NCF 1 configuration */
		vuint32_t NCFC0 : 1;	/* NCF 0 configuration */
	} B;
} FCCU_NCF_CFG0_32B_tag;

typedef union { /* FCCU NCF Configuration Register 1 */
	vuint32_t R;
	struct {
		vuint32_t NCFC63 : 1; /* NCF 63 configuration */
		vuint32_t NCFC62 : 1; /* NCF 62 configuration */
		vuint32_t NCFC61 : 1; /* NCF 61 configuration */
		vuint32_t NCFC60 : 1; /* NCF 60 configuration */
		vuint32_t NCFC59 : 1; /* NCF 59 configuration */
		vuint32_t NCFC58 : 1; /* NCF 58 configuration */
		vuint32_t NCFC57 : 1; /* NCF 57 configuration */
		vuint32_t NCFC56 : 1; /* NCF 56 configuration */
		vuint32_t NCFC55 : 1; /* NCF 55 configuration */
		vuint32_t NCFC54 : 1; /* NCF 54 configuration */
		vuint32_t NCFC53 : 1; /* NCF 53 configuration */
		vuint32_t NCFC52 : 1; /* NCF 52 configuration */
		vuint32_t NCFC51 : 1; /* NCF 51 configuration */
		vuint32_t NCFC50 : 1; /* NCF 50 configuration */
		vuint32_t NCFC49 : 1; /* NCF 49 configuration */
		vuint32_t NCFC48 : 1; /* NCF 48 configuration */
		vuint32_t NCFC47 : 1; /* NCF 47 configuration */
		vuint32_t NCFC46 : 1; /* NCF 46 configuration */
		vuint32_t NCFC45 : 1; /* NCF 45 configuration */
		vuint32_t NCFC44 : 1; /* NCF 44 configuration */
		vuint32_t NCFC43 : 1; /* NCF 43 configuration */
		vuint32_t NCFC42 : 1; /* NCF 42 configuration */
		vuint32_t NCFC41 : 1; /* NCF 41 configuration */
		vuint32_t NCFC40 : 1; /* NCF 40 configuration */
		vuint32_t NCFC39 : 1; /* NCF 39 configuration */
		vuint32_t NCFC38 : 1; /* NCF 38 configuration */
		vuint32_t NCFC37 : 1; /* NCF 37 configuration */
		vuint32_t NCFC36 : 1; /* NCF 36 configuration */
		vuint32_t NCFC35 : 1; /* NCF 35 configuration */
		vuint32_t NCFC34 : 1; /* NCF 34 configuration */
		vuint32_t NCFC33 : 1; /* NCF 33 configuration */
		vuint32_t NCFC32 : 1; /* NCF 32 configuration */
	} B;
} FCCU_NCF_CFG1_32B_tag;

typedef union { /* FCCU NCF Configuration Register 2 */
	vuint32_t R;
	struct {
		vuint32_t NCFC95 : 1; /* NCF 95 configuration */
		vuint32_t NCFC94 : 1; /* NCF 94 configuration */
		vuint32_t NCFC93 : 1; /* NCF 93 configuration */
		vuint32_t NCFC92 : 1; /* NCF 92 configuration */
		vuint32_t NCFC91 : 1; /* NCF 91 configuration */
		vuint32_t NCFC90 : 1; /* NCF 90 configuration */
		vuint32_t NCFC89 : 1; /* NCF 89 configuration */
		vuint32_t NCFC88 : 1; /* NCF 88 configuration */
		vuint32_t NCFC87 : 1; /* NCF 87 configuration */
		vuint32_t NCFC86 : 1; /* NCF 86 configuration */
		vuint32_t NCFC85 : 1; /* NCF 85 configuration */
		vuint32_t NCFC84 : 1; /* NCF 84 configuration */
		vuint32_t NCFC83 : 1; /* NCF 83 configuration */
		vuint32_t NCFC82 : 1; /* NCF 82 configuration */
		vuint32_t NCFC81 : 1; /* NCF 81 configuration */
		vuint32_t NCFC80 : 1; /* NCF 80 configuration */
		vuint32_t NCFC79 : 1; /* NCF 79 configuration */
		vuint32_t NCFC78 : 1; /* NCF 78 configuration */
		vuint32_t NCFC77 : 1; /* NCF 77 configuration */
		vuint32_t NCFC76 : 1; /* NCF 76 configuration */
		vuint32_t NCFC75 : 1; /* NCF 75 configuration */
		vuint32_t NCFC74 : 1; /* NCF 74 configuration */
		vuint32_t NCFC73 : 1; /* NCF 73 configuration */
		vuint32_t NCFC72 : 1; /* NCF 72 configuration */
		vuint32_t NCFC71 : 1; /* NCF 71 configuration */
		vuint32_t NCFC70 : 1; /* NCF 70 configuration */
		vuint32_t NCFC69 : 1; /* NCF 69 configuration */
		vuint32_t NCFC68 : 1; /* NCF 68 configuration */
		vuint32_t NCFC67 : 1; /* NCF 67 configuration */
		vuint32_t NCFC66 : 1; /* NCF 66 configuration */
		vuint32_t NCFC65 : 1; /* NCF 65 configuration */
		vuint32_t NCFC64 : 1; /* NCF 64 configuration */
	} B;
} FCCU_NCF_CFG2_32B_tag;

typedef union { /* FCCU NCF Configuration Register 3 */
	vuint32_t R;
	struct {
		vuint32_t NCFC127 : 1; /* NCF 127 configuration */
		vuint32_t NCFC126 : 1; /* NCF 126 configuration */
		vuint32_t NCFC125 : 1; /* NCF 125 configuration */
		vuint32_t NCFC124 : 1; /* NCF 124 configuration */
		vuint32_t NCFC123 : 1; /* NCF 123 configuration */
		vuint32_t NCFC122 : 1; /* NCF 122 configuration */
		vuint32_t NCFC121 : 1; /* NCF 121 configuration */
		vuint32_t NCFC120 : 1; /* NCF 120 configuration */
		vuint32_t NCFC119 : 1; /* NCF 119 configuration */
		vuint32_t NCFC118 : 1; /* NCF 118 configuration */
		vuint32_t NCFC117 : 1; /* NCF 117 configuration */
		vuint32_t NCFC116 : 1; /* NCF 116 configuration */
		vuint32_t NCFC115 : 1; /* NCF 115 configuration */
		vuint32_t NCFC114 : 1; /* NCF 114 configuration */
		vuint32_t NCFC113 : 1; /* NCF 113 configuration */
		vuint32_t NCFC112 : 1; /* NCF 112 configuration */
		vuint32_t NCFC111 : 1; /* NCF 111 configuration */
		vuint32_t NCFC110 : 1; /* NCF 110 configuration */
		vuint32_t NCFC109 : 1; /* NCF 109 configuration */
		vuint32_t NCFC108 : 1; /* NCF 108 configuration */
		vuint32_t NCFC107 : 1; /* NCF 107 configuration */
		vuint32_t NCFC106 : 1; /* NCF 106 configuration */
		vuint32_t NCFC105 : 1; /* NCF 105 configuration */
		vuint32_t NCFC104 : 1; /* NCF 104 configuration */
		vuint32_t NCFC103 : 1; /* NCF 103 configuration */
		vuint32_t NCFC102 : 1; /* NCF 102 configuration */
		vuint32_t NCFC101 : 1; /* NCF 101 configuration */
		vuint32_t NCFC100 : 1; /* NCF 100 configuration */
		vuint32_t NCFC99 : 1;	 /* NCF 99 configuration */
		vuint32_t NCFC98 : 1;	 /* NCF 98 configuration */
		vuint32_t NCFC97 : 1;	 /* NCF 97 configuration */
		vuint32_t NCFC96 : 1;	 /* NCF 96 configuration */
	} B;
} FCCU_NCF_CFG3_32B_tag;

typedef union { /* FCCU CFS Configuration Register 0 */
	vuint32_t R;
	struct {
		vuint32_t CFSC15 : 2; /* CF 15 state configuration */
		vuint32_t CFSC14 : 2; /* CF 14 state configuration */
		vuint32_t CFSC13 : 2; /* CF 13 state configuration */
		vuint32_t CFSC12 : 2; /* CF 12 state configuration */
		vuint32_t CFSC11 : 2; /* CF 11 state configuration */
		vuint32_t CFSC10 : 2; /* CF 10 state configuration */
		vuint32_t CFSC9 : 2;	/* CF 9 state configuration */
		vuint32_t CFSC8 : 2;	/* CF 8 state configuration */
		vuint32_t CFSC7 : 2;	/* CF 7 state configuration */
		vuint32_t CFSC6 : 2;	/* CF 6 state configuration */
		vuint32_t CFSC5 : 2;	/* CF 5 state configuration */
		vuint32_t CFSC4 : 2;	/* CF 4 state configuration */
		vuint32_t CFSC3 : 2;	/* CF 3 state configuration */
		vuint32_t CFSC2 : 2;	/* CF 2 state configuration */
		vuint32_t CFSC1 : 2;	/* CF 1 state configuration */
		vuint32_t CFSC0 : 2;	/* CF 0 state configuration */
	} B;
} FCCU_CFS_CFG0_32B_tag;

typedef union { /* FCCU CFS Configuration Register 1 */
	vuint32_t R;
	struct {
		vuint32_t CFSC31 : 2; /* CF 31 state configuration */
		vuint32_t CFSC30 : 2; /* CF 30 state configuration */
		vuint32_t CFSC29 : 2; /* CF 29 state configuration */
		vuint32_t CFSC28 : 2; /* CF 28 state configuration */
		vuint32_t CFSC27 : 2; /* CF 27 state configuration */
		vuint32_t CFSC26 : 2; /* CF 26 state configuration */
		vuint32_t CFSC25 : 2; /* CF 25 state configuration */
		vuint32_t CFSC24 : 2; /* CF 24 state configuration */
		vuint32_t CFSC23 : 2; /* CF 23 state configuration */
		vuint32_t CFSC22 : 2; /* CF 22 state configuration */
		vuint32_t CFSC21 : 2; /* CF 21 state configuration */
		vuint32_t CFSC20 : 2; /* CF 20 state configuration */
		vuint32_t CFSC19 : 2; /* CF 19 state configuration */
		vuint32_t CFSC18 : 2; /* CF 18 state configuration */
		vuint32_t CFSC17 : 2; /* CF 17 state configuration */
		vuint32_t CFSC16 : 2; /* CF 16 state configuration */
	} B;
} FCCU_CFS_CFG1_32B_tag;

typedef union { /* FCCU CFS Configuration Register 2 */
	vuint32_t R;
	struct {
		vuint32_t CFSC47 : 2; /* CF 47 state configuration */
		vuint32_t CFSC46 : 2; /* CF 46 state configuration */
		vuint32_t CFSC45 : 2; /* CF 45 state configuration */
		vuint32_t CFSC44 : 2; /* CF 44 state configuration */
		vuint32_t CFSC43 : 2; /* CF 43 state configuration */
		vuint32_t CFSC42 : 2; /* CF 42 state configuration */
		vuint32_t CFSC41 : 2; /* CF 41 state configuration */
		vuint32_t CFSC40 : 2; /* CF 40 state configuration */
		vuint32_t CFSC39 : 2; /* CF 39 state configuration */
		vuint32_t CFSC38 : 2; /* CF 38 state configuration */
		vuint32_t CFSC37 : 2; /* CF 37 state configuration */
		vuint32_t CFSC36 : 2; /* CF 36 state configuration */
		vuint32_t CFSC35 : 2; /* CF 35 state configuration */
		vuint32_t CFSC34 : 2; /* CF 34 state configuration */
		vuint32_t CFSC33 : 2; /* CF 33 state configuration */
		vuint32_t CFSC32 : 2; /* CF 32 state configuration */
	} B;
} FCCU_CFS_CFG2_32B_tag;

typedef union { /* FCCU CFS Configuration Register 3 */
	vuint32_t R;
	struct {
		vuint32_t CFSC63 : 2; /* CF 63 state configuration */
		vuint32_t CFSC62 : 2; /* CF 62 state configuration */
		vuint32_t CFSC61 : 2; /* CF 61 state configuration */
		vuint32_t CFSC60 : 2; /* CF 60 state configuration */
		vuint32_t CFSC59 : 2; /* CF 59 state configuration */
		vuint32_t CFSC58 : 2; /* CF 58 state configuration */
		vuint32_t CFSC57 : 2; /* CF 57 state configuration */
		vuint32_t CFSC56 : 2; /* CF 56 state configuration */
		vuint32_t CFSC55 : 2; /* CF 55 state configuration */
		vuint32_t CFSC54 : 2; /* CF 54 state configuration */
		vuint32_t CFSC53 : 2; /* CF 53 state configuration */
		vuint32_t CFSC52 : 2; /* CF 52 state configuration */
		vuint32_t CFSC51 : 2; /* CF 51 state configuration */
		vuint32_t CFSC50 : 2; /* CF 50 state configuration */
		vuint32_t CFSC49 : 2; /* CF 49 state configuration */
		vuint32_t CFSC48 : 2; /* CF 48 state configuration */
	} B;
} FCCU_CFS_CFG3_32B_tag;

typedef union { /* FCCU CFS Configuration Register 4 */
	vuint32_t R;
	struct {
		vuint32_t CFSC79 : 2; /* CF 79 state configuration */
		vuint32_t CFSC78 : 2; /* CF 78 state configuration */
		vuint32_t CFSC77 : 2; /* CF 77 state configuration */
		vuint32_t CFSC76 : 2; /* CF 76 state configuration */
		vuint32_t CFSC75 : 2; /* CF 75 state configuration */
		vuint32_t CFSC74 : 2; /* CF 74 state configuration */
		vuint32_t CFSC73 : 2; /* CF 73 state configuration */
		vuint32_t CFSC72 : 2; /* CF 72 state configuration */
		vuint32_t CFSC71 : 2; /* CF 71 state configuration */
		vuint32_t CFSC70 : 2; /* CF 70 state configuration */
		vuint32_t CFSC69 : 2; /* CF 69 state configuration */
		vuint32_t CFSC68 : 2; /* CF 68 state configuration */
		vuint32_t CFSC67 : 2; /* CF 67 state configuration */
		vuint32_t CFSC66 : 2; /* CF 66 state configuration */
		vuint32_t CFSC65 : 2; /* CF 65 state configuration */
		vuint32_t CFSC64 : 2; /* CF 64 state configuration */
	} B;
} FCCU_CFS_CFG4_32B_tag;

typedef union { /* FCCU CFS Configuration Register 5 */
	vuint32_t R;
	struct {
		vuint32_t CFSC95 : 2; /* CF 95 state configuration */
		vuint32_t CFSC94 : 2; /* CF 94 state configuration */
		vuint32_t CFSC93 : 2; /* CF 93 state configuration */
		vuint32_t CFSC92 : 2; /* CF 92 state configuration */
		vuint32_t CFSC91 : 2; /* CF 91 state configuration */
		vuint32_t CFSC90 : 2; /* CF 90 state configuration */
		vuint32_t CFSC89 : 2; /* CF 89 state configuration */
		vuint32_t CFSC88 : 2; /* CF 88 state configuration */
		vuint32_t CFSC87 : 2; /* CF 87 state configuration */
		vuint32_t CFSC86 : 2; /* CF 86 state configuration */
		vuint32_t CFSC85 : 2; /* CF 85 state configuration */
		vuint32_t CFSC84 : 2; /* CF 84 state configuration */
		vuint32_t CFSC83 : 2; /* CF 83 state configuration */
		vuint32_t CFSC82 : 2; /* CF 82 state configuration */
		vuint32_t CFSC81 : 2; /* CF 81 state configuration */
		vuint32_t CFSC80 : 2; /* CF 80 state configuration */
	} B;
} FCCU_CFS_CFG5_32B_tag;

typedef union { /* FCCU CFS Configuration Register 6 */
	vuint32_t R;
	struct {
		vuint32_t CFSC111 : 2; /* CF 111 state configuration */
		vuint32_t CFSC110 : 2; /* CF 110 state configuration */
		vuint32_t CFSC109 : 2; /* CF 109 state configuration */
		vuint32_t CFSC108 : 2; /* CF 108 state configuration */
		vuint32_t CFSC107 : 2; /* CF 107 state configuration */
		vuint32_t CFSC106 : 2; /* CF 106 state configuration */
		vuint32_t CFSC105 : 2; /* CF 105 state configuration */
		vuint32_t CFSC104 : 2; /* CF 104 state configuration */
		vuint32_t CFSC103 : 2; /* CF 103 state configuration */
		vuint32_t CFSC102 : 2; /* CF 102 state configuration */
		vuint32_t CFSC101 : 2; /* CF 101 state configuration */
		vuint32_t CFSC100 : 2; /* CF 100 state configuration */
		vuint32_t CFSC99 : 2;	 /* CF 99 state configuration */
		vuint32_t CFSC98 : 2;	 /* CF 98 state configuration */
		vuint32_t CFSC97 : 2;	 /* CF 97 state configuration */
		vuint32_t CFSC96 : 2;	 /* CF 96 state configuration */
	} B;
} FCCU_CFS_CFG6_32B_tag;

typedef union { /* FCCU CFS Configuration Register 7 */
	vuint32_t R;
	struct {
		vuint32_t CFSC127 : 2; /* CF 127 state configuration */
		vuint32_t CFSC126 : 2; /* CF 126 state configuration */
		vuint32_t CFSC125 : 2; /* CF 125 state configuration */
		vuint32_t CFSC124 : 2; /* CF 124 state configuration */
		vuint32_t CFSC123 : 2; /* CF 123 state configuration */
		vuint32_t CFSC122 : 2; /* CF 122 state configuration */
		vuint32_t CFSC121 : 2; /* CF 121 state configuration */
		vuint32_t CFSC120 : 2; /* CF 120 state configuration */
		vuint32_t CFSC119 : 2; /* CF 119 state configuration */
		vuint32_t CFSC118 : 2; /* CF 118 state configuration */
		vuint32_t CFSC117 : 2; /* CF 117 state configuration */
		vuint32_t CFSC116 : 2; /* CF 116 state configuration */
		vuint32_t CFSC115 : 2; /* CF 115 state configuration */
		vuint32_t CFSC114 : 2; /* CF 114 state configuration */
		vuint32_t CFSC113 : 2; /* CF 113 state configuration */
		vuint32_t CFSC112 : 2; /* CF 112 state configuration */
	} B;
} FCCU_CFS_CFG7_32B_tag;

typedef union { /* FCCU NCFS Configuration Register 0 */
	vuint32_t R;
	struct {
		vuint32_t NCFSC15 : 2; /* NCF 15 state configuration */
		vuint32_t NCFSC14 : 2; /* NCF 14 state configuration */
		vuint32_t NCFSC13 : 2; /* NCF 13 state configuration */
		vuint32_t NCFSC12 : 2; /* NCF 12 state configuration */
		vuint32_t NCFSC11 : 2; /* NCF 11 state configuration */
		vuint32_t NCFSC10 : 2; /* NCF 10 state configuration */
		vuint32_t NCFSC9 : 2;	 /* NCF 9 state configuration */
		vuint32_t NCFSC8 : 2;	 /* NCF 8 state configuration */
		vuint32_t NCFSC7 : 2;	 /* NCF 7 state configuration */
		vuint32_t NCFSC6 : 2;	 /* NCF 6 state configuration */
		vuint32_t NCFSC5 : 2;	 /* NCF 5 state configuration */
		vuint32_t NCFSC4 : 2;	 /* NCF 4 state configuration */
		vuint32_t NCFSC3 : 2;	 /* NCF 3 state configuration */
		vuint32_t NCFSC2 : 2;	 /* NCF 2 state configuration */
		vuint32_t NCFSC1 : 2;	 /* NCF 1 state configuration */
		vuint32_t NCFSC0 : 2;	 /* NCF 0 state configuration */
	} B;
} FCCU_NCFS_CFG0_32B_tag;

typedef union { /* FCCU NCFS Configuration Register 1 */
	vuint32_t R;
	struct {
		vuint32_t NCFSC31 : 2; /* NCF 31 state configuration */
		vuint32_t NCFSC30 : 2; /* NCF 30 state configuration */
		vuint32_t NCFSC29 : 2; /* NCF 29 state configuration */
		vuint32_t NCFSC28 : 2; /* NCF 28 state configuration */
		vuint32_t NCFSC27 : 2; /* NCF 27 state configuration */
		vuint32_t NCFSC26 : 2; /* NCF 26 state configuration */
		vuint32_t NCFSC25 : 2; /* NCF 25 state configuration */
		vuint32_t NCFSC24 : 2; /* NCF 24 state configuration */
		vuint32_t NCFSC23 : 2; /* NCF 23 state configuration */
		vuint32_t NCFSC22 : 2; /* NCF 22 state configuration */
		vuint32_t NCFSC21 : 2; /* NCF 21 state configuration */
		vuint32_t NCFSC20 : 2; /* NCF 20 state configuration */
		vuint32_t NCFSC19 : 2; /* NCF 19 state configuration */
		vuint32_t NCFSC18 : 2; /* NCF 18 state configuration */
		vuint32_t NCFSC17 : 2; /* NCF 17 state configuration */
		vuint32_t NCFSC16 : 2; /* NCF 16 state configuration */
	} B;
} FCCU_NCFS_CFG1_32B_tag;

typedef union { /* FCCU NCFS Configuration Register 2 */
	vuint32_t R;
	struct {
		vuint32_t NCFSC47 : 2; /* NCF 47 state configuration */
		vuint32_t NCFSC46 : 2; /* NCF 46 state configuration */
		vuint32_t NCFSC45 : 2; /* NCF 45 state configuration */
		vuint32_t NCFSC44 : 2; /* NCF 44 state configuration */
		vuint32_t NCFSC43 : 2; /* NCF 43 state configuration */
		vuint32_t NCFSC42 : 2; /* NCF 42 state configuration */
		vuint32_t NCFSC41 : 2; /* NCF 41 state configuration */
		vuint32_t NCFSC40 : 2; /* NCF 40 state configuration */
		vuint32_t NCFSC39 : 2; /* NCF 39 state configuration */
		vuint32_t NCFSC38 : 2; /* NCF 38 state configuration */
		vuint32_t NCFSC37 : 2; /* NCF 37 state configuration */
		vuint32_t NCFSC36 : 2; /* NCF 36 state configuration */
		vuint32_t NCFSC35 : 2; /* NCF 35 state configuration */
		vuint32_t NCFSC34 : 2; /* NCF 34 state configuration */
		vuint32_t NCFSC33 : 2; /* NCF 33 state configuration */
		vuint32_t NCFSC32 : 2; /* NCF 32 state configuration */
	} B;
} FCCU_NCFS_CFG2_32B_tag;

typedef union { /* FCCU NCFS Configuration Register 3 */
	vuint32_t R;
	struct {
		vuint32_t NCFSC63 : 2; /* NCF 63 state configuration */
		vuint32_t NCFSC62 : 2; /* NCF 62 state configuration */
		vuint32_t NCFSC61 : 2; /* NCF 61 state configuration */
		vuint32_t NCFSC60 : 2; /* NCF 60 state configuration */
		vuint32_t NCFSC59 : 2; /* NCF 59 state configuration */
		vuint32_t NCFSC58 : 2; /* NCF 58 state configuration */
		vuint32_t NCFSC57 : 2; /* NCF 57 state configuration */
		vuint32_t NCFSC56 : 2; /* NCF 56 state configuration */
		vuint32_t NCFSC55 : 2; /* NCF 55 state configuration */
		vuint32_t NCFSC54 : 2; /* NCF 54 state configuration */
		vuint32_t NCFSC53 : 2; /* NCF 53 state configuration */
		vuint32_t NCFSC52 : 2; /* NCF 52 state configuration */
		vuint32_t NCFSC51 : 2; /* NCF 51 state configuration */
		vuint32_t NCFSC50 : 2; /* NCF 50 state configuration */
		vuint32_t NCFSC49 : 2; /* NCF 49 state configuration */
		vuint32_t NCFSC48 : 2; /* NCF 48 state configuration */
	} B;
} FCCU_NCFS_CFG3_32B_tag;

typedef union { /* FCCU NCFS Configuration Register 4 */
	vuint32_t R;
	struct {
		vuint32_t NCFSC79 : 2; /* NCF 79 state configuration */
		vuint32_t NCFSC78 : 2; /* NCF 78 state configuration */
		vuint32_t NCFSC77 : 2; /* NCF 77 state configuration */
		vuint32_t NCFSC76 : 2; /* NCF 76 state configuration */
		vuint32_t NCFSC75 : 2; /* NCF 75 state configuration */
		vuint32_t NCFSC74 : 2; /* NCF 74 state configuration */
		vuint32_t NCFSC73 : 2; /* NCF 73 state configuration */
		vuint32_t NCFSC72 : 2; /* NCF 72 state configuration */
		vuint32_t NCFSC71 : 2; /* NCF 71 state configuration */
		vuint32_t NCFSC70 : 2; /* NCF 70 state configuration */
		vuint32_t NCFSC69 : 2; /* NCF 69 state configuration */
		vuint32_t NCFSC68 : 2; /* NCF 68 state configuration */
		vuint32_t NCFSC67 : 2; /* NCF 67 state configuration */
		vuint32_t NCFSC66 : 2; /* NCF 66 state configuration */
		vuint32_t NCFSC65 : 2; /* NCF 65 state configuration */
		vuint32_t NCFSC64 : 2; /* NCF 64 state configuration */
	} B;
} FCCU_NCFS_CFG4_32B_tag;

typedef union { /* FCCU NCFS Configuration Register 5 */
	vuint32_t R;
	struct {
		vuint32_t NCFSC95 : 2; /* NCF 95 state configuration */
		vuint32_t NCFSC94 : 2; /* NCF 94 state configuration */
		vuint32_t NCFSC93 : 2; /* NCF 93 state configuration */
		vuint32_t NCFSC92 : 2; /* NCF 92 state configuration */
		vuint32_t NCFSC91 : 2; /* NCF 91 state configuration */
		vuint32_t NCFSC90 : 2; /* NCF 90 state configuration */
		vuint32_t NCFSC89 : 2; /* NCF 89 state configuration */
		vuint32_t NCFSC88 : 2; /* NCF 88 state configuration */
		vuint32_t NCFSC87 : 2; /* NCF 87 state configuration */
		vuint32_t NCFSC86 : 2; /* NCF 86 state configuration */
		vuint32_t NCFSC85 : 2; /* NCF 85 state configuration */
		vuint32_t NCFSC84 : 2; /* NCF 84 state configuration */
		vuint32_t NCFSC83 : 2; /* NCF 83 state configuration */
		vuint32_t NCFSC82 : 2; /* NCF 82 state configuration */
		vuint32_t NCFSC81 : 2; /* NCF 81 state configuration */
		vuint32_t NCFSC80 : 2; /* NCF 80 state configuration */
	} B;
} FCCU_NCFS_CFG5_32B_tag;

typedef union { /* FCCU NCFS Configuration Register 6 */
	vuint32_t R;
	struct {
		vuint32_t NCFSC111 : 2; /* NCF 111 state configuration */
		vuint32_t NCFSC110 : 2; /* NCF 110 state configuration */
		vuint32_t NCFSC109 : 2; /* NCF 109 state configuration */
		vuint32_t NCFSC108 : 2; /* NCF 108 state configuration */
		vuint32_t NCFSC107 : 2; /* NCF 107 state configuration */
		vuint32_t NCFSC106 : 2; /* NCF 106 state configuration */
		vuint32_t NCFSC105 : 2; /* NCF 105 state configuration */
		vuint32_t NCFSC104 : 2; /* NCF 104 state configuration */
		vuint32_t NCFSC103 : 2; /* NCF 103 state configuration */
		vuint32_t NCFSC102 : 2; /* NCF 102 state configuration */
		vuint32_t NCFSC101 : 2; /* NCF 101 state configuration */
		vuint32_t NCFSC100 : 2; /* NCF 100 state configuration */
		vuint32_t NCFSC99 : 2;	/* NCF 99 state configuration */
		vuint32_t NCFSC98 : 2;	/* NCF 98 state configuration */
		vuint32_t NCFSC97 : 2;	/* NCF 97 state configuration */
		vuint32_t NCFSC96 : 2;	/* NCF 96 state configuration */
	} B;
} FCCU_NCFS_CFG6_32B_tag;

typedef union { /* FCCU NCFS Configuration Register 7 */
	vuint32_t R;
	struct {
		vuint32_t NCFSC127 : 2; /* NCF 127 state configuration */
		vuint32_t NCFSC126 : 2; /* NCF 126 state configuration */
		vuint32_t NCFSC125 : 2; /* NCF 125 state configuration */
		vuint32_t NCFSC124 : 2; /* NCF 124 state configuration */
		vuint32_t NCFSC123 : 2; /* NCF 123 state configuration */
		vuint32_t NCFSC122 : 2; /* NCF 122 state configuration */
		vuint32_t NCFSC121 : 2; /* NCF 121 state configuration */
		vuint32_t NCFSC120 : 2; /* NCF 120 state configuration */
		vuint32_t NCFSC119 : 2; /* NCF 119 state configuration */
		vuint32_t NCFSC118 : 2; /* NCF 118 state configuration */
		vuint32_t NCFSC117 : 2; /* NCF 117 state configuration */
		vuint32_t NCFSC116 : 2; /* NCF 116 state configuration */
		vuint32_t NCFSC115 : 2; /* NCF 115 state configuration */
		vuint32_t NCFSC114 : 2; /* NCF 114 state configuration */
		vuint32_t NCFSC113 : 2; /* NCF 113 state configuration */
		vuint32_t NCFSC112 : 2; /* NCF 112 state configuration */
	} B;
} FCCU_NCFS_CFG7_32B_tag;

typedef union { /* FCCU CF Status Register 0 */
	vuint32_t R;
	struct {
		vuint32_t CFS31 : 1; /* CF 31 status */
		vuint32_t CFS30 : 1; /* CF 30 status */
		vuint32_t CFS29 : 1; /* CF 29 status */
		vuint32_t CFS28 : 1; /* CF 28 status */
		vuint32_t CFS27 : 1; /* CF 27 status */
		vuint32_t CFS26 : 1; /* CF 26 status */
		vuint32_t CFS25 : 1; /* CF 25 status */
		vuint32_t CFS24 : 1; /* CF 24 status */
		vuint32_t CFS23 : 1; /* CF 23 status */
		vuint32_t CFS22 : 1; /* CF 22 status */
		vuint32_t CFS21 : 1; /* CF 21 status */
		vuint32_t CFS20 : 1; /* CF 20 status */
		vuint32_t CFS19 : 1; /* CF 19 status */
		vuint32_t CFS18 : 1; /* CF 18 status */
		vuint32_t CFS17 : 1; /* CF 17 status */
		vuint32_t CFS16 : 1; /* CF 16 status */
		vuint32_t CFS15 : 1; /* CF 15 status */
		vuint32_t CFS14 : 1; /* CF 14 status */
		vuint32_t CFS13 : 1; /* CF 13 status */
		vuint32_t CFS12 : 1; /* CF 12 status */
		vuint32_t CFS11 : 1; /* CF 11 status */
		vuint32_t CFS10 : 1; /* CF 10 status */
		vuint32_t CFS9 : 1;	 /* CF 9 status */
		vuint32_t CFS8 : 1;	 /* CF 8 status */
		vuint32_t CFS7 : 1;	 /* CF 7 status */
		vuint32_t CFS6 : 1;	 /* CF 6 status */
		vuint32_t CFS5 : 1;	 /* CF 5 status */
		vuint32_t CFS4 : 1;	 /* CF 4 status */
		vuint32_t CFS3 : 1;	 /* CF 3 status */
		vuint32_t CFS2 : 1;	 /* CF 2 status */
		vuint32_t CFS1 : 1;	 /* CF 1 status */
		vuint32_t CFS0 : 1;	 /* CF 0 status */
	} B;
} FCCU_CFS0_32B_tag;

typedef union { /* FCCU CF Status Register 1 */
	vuint32_t R;
	struct {
		vuint32_t CFS63 : 1; /* CF 63 status */
		vuint32_t CFS62 : 1; /* CF 62 status */
		vuint32_t CFS61 : 1; /* CF 61 status */
		vuint32_t CFS60 : 1; /* CF 60 status */
		vuint32_t CFS59 : 1; /* CF 59 status */
		vuint32_t CFS58 : 1; /* CF 58 status */
		vuint32_t CFS57 : 1; /* CF 57 status */
		vuint32_t CFS56 : 1; /* CF 56 status */
		vuint32_t CFS55 : 1; /* CF 55 status */
		vuint32_t CFS54 : 1; /* CF 54 status */
		vuint32_t CFS53 : 1; /* CF 53 status */
		vuint32_t CFS52 : 1; /* CF 52 status */
		vuint32_t CFS51 : 1; /* CF 51 status */
		vuint32_t CFS50 : 1; /* CF 50 status */
		vuint32_t CFS49 : 1; /* CF 49 status */
		vuint32_t CFS48 : 1; /* CF 48 status */
		vuint32_t CFS47 : 1; /* CF 47 status */
		vuint32_t CFS46 : 1; /* CF 46 status */
		vuint32_t CFS45 : 1; /* CF 45 status */
		vuint32_t CFS44 : 1; /* CF 44 status */
		vuint32_t CFS43 : 1; /* CF 43 status */
		vuint32_t CFS42 : 1; /* CF 42 status */
		vuint32_t CFS41 : 1; /* CF 41 status */
		vuint32_t CFS40 : 1; /* CF 40 status */
		vuint32_t CFS39 : 1; /* CF 39 status */
		vuint32_t CFS38 : 1; /* CF 38 status */
		vuint32_t CFS37 : 1; /* CF 37 status */
		vuint32_t CFS36 : 1; /* CF 36 status */
		vuint32_t CFS35 : 1; /* CF 35 status */
		vuint32_t CFS34 : 1; /* CF 34 status */
		vuint32_t CFS33 : 1; /* CF 33 status */
		vuint32_t CFS32 : 1; /* CF 32 status */
	} B;
} FCCU_CFS1_32B_tag;

typedef union { /* FCCU CF Status Register 2 */
	vuint32_t R;
	struct {
		vuint32_t CFS95 : 1; /* CF 95 status */
		vuint32_t CFS94 : 1; /* CF 94 status */
		vuint32_t CFS93 : 1; /* CF 93 status */
		vuint32_t CFS92 : 1; /* CF 92 status */
		vuint32_t CFS91 : 1; /* CF 91 status */
		vuint32_t CFS90 : 1; /* CF 90 status */
		vuint32_t CFS89 : 1; /* CF 89 status */
		vuint32_t CFS88 : 1; /* CF 88 status */
		vuint32_t CFS87 : 1; /* CF 87 status */
		vuint32_t CFS86 : 1; /* CF 86 status */
		vuint32_t CFS85 : 1; /* CF 85 status */
		vuint32_t CFS84 : 1; /* CF 84 status */
		vuint32_t CFS83 : 1; /* CF 83 status */
		vuint32_t CFS82 : 1; /* CF 82 status */
		vuint32_t CFS81 : 1; /* CF 81 status */
		vuint32_t CFS80 : 1; /* CF 80 status */
		vuint32_t CFS79 : 1; /* CF 79 status */
		vuint32_t CFS78 : 1; /* CF 78 status */
		vuint32_t CFS77 : 1; /* CF 77 status */
		vuint32_t CFS76 : 1; /* CF 76 status */
		vuint32_t CFS75 : 1; /* CF 75 status */
		vuint32_t CFS74 : 1; /* CF 74 status */
		vuint32_t CFS73 : 1; /* CF 73 status */
		vuint32_t CFS72 : 1; /* CF 72 status */
		vuint32_t CFS71 : 1; /* CF 71 status */
		vuint32_t CFS70 : 1; /* CF 70 status */
		vuint32_t CFS69 : 1; /* CF 69 status */
		vuint32_t CFS68 : 1; /* CF 68 status */
		vuint32_t CFS67 : 1; /* CF 67 status */
		vuint32_t CFS66 : 1; /* CF 66 status */
		vuint32_t CFS65 : 1; /* CF 65 status */
		vuint32_t CFS64 : 1; /* CF 64 status */
	} B;
} FCCU_CFS2_32B_tag;

typedef union { /* FCCU CF Status Register 3 */
	vuint32_t R;
	struct {
		vuint32_t CFS127 : 1; /* CF 127 status */
		vuint32_t CFS126 : 1; /* CF 126 status */
		vuint32_t CFS125 : 1; /* CF 125 status */
		vuint32_t CFS124 : 1; /* CF 124 status */
		vuint32_t CFS123 : 1; /* CF 123 status */
		vuint32_t CFS122 : 1; /* CF 122 status */
		vuint32_t CFS121 : 1; /* CF 121 status */
		vuint32_t CFS120 : 1; /* CF 120 status */
		vuint32_t CFS119 : 1; /* CF 119 status */
		vuint32_t CFS118 : 1; /* CF 118 status */
		vuint32_t CFS117 : 1; /* CF 117 status */
		vuint32_t CFS116 : 1; /* CF 116 status */
		vuint32_t CFS115 : 1; /* CF 115 status */
		vuint32_t CFS114 : 1; /* CF 114 status */
		vuint32_t CFS113 : 1; /* CF 113 status */
		vuint32_t CFS112 : 1; /* CF 112 status */
		vuint32_t CFS111 : 1; /* CF 111 status */
		vuint32_t CFS110 : 1; /* CF 110 status */
		vuint32_t CFS109 : 1; /* CF 109 status */
		vuint32_t CFS108 : 1; /* CF 108 status */
		vuint32_t CFS107 : 1; /* CF 107 status */
		vuint32_t CFS106 : 1; /* CF 106 status */
		vuint32_t CFS105 : 1; /* CF 105 status */
		vuint32_t CFS104 : 1; /* CF 104 status */
		vuint32_t CFS103 : 1; /* CF 103 status */
		vuint32_t CFS102 : 1; /* CF 102 status */
		vuint32_t CFS101 : 1; /* CF 101 status */
		vuint32_t CFS100 : 1; /* CF 100 status */
		vuint32_t CFS99 : 1;	/* CF 99 status */
		vuint32_t CFS98 : 1;	/* CF 98 status */
		vuint32_t CFS97 : 1;	/* CF 97 status */
		vuint32_t CFS96 : 1;	/* CF 96 status */
	} B;
} FCCU_CFS3_32B_tag;

typedef union { /* FCCU_CFK - FCCU CF Key Register */
	vuint32_t R;
} FCCU_CFK_32B_tag;

typedef union { /* FCCU NCF Status Register 0 */
	vuint32_t R;
	struct {
		vuint32_t NCFS31 : 1; /* NCF 31 status */
		vuint32_t NCFS30 : 1; /* NCF 30 status */
		vuint32_t NCFS29 : 1; /* NCF 29 status */
		vuint32_t NCFS28 : 1; /* NCF 28 status */
		vuint32_t NCFS27 : 1; /* NCF 27 status */
		vuint32_t NCFS26 : 1; /* NCF 26 status */
		vuint32_t NCFS25 : 1; /* NCF 25 status */
		vuint32_t NCFS24 : 1; /* NCF 24 status */
		vuint32_t NCFS23 : 1; /* NCF 23 status */
		vuint32_t NCFS22 : 1; /* NCF 22 status */
		vuint32_t NCFS21 : 1; /* NCF 21 status */
		vuint32_t NCFS20 : 1; /* NCF 20 status */
		vuint32_t NCFS19 : 1; /* NCF 19 status */
		vuint32_t NCFS18 : 1; /* NCF 18 status */
		vuint32_t NCFS17 : 1; /* NCF 17 status */
		vuint32_t NCFS16 : 1; /* NCF 16 status */
		vuint32_t NCFS15 : 1; /* NCF 15 status */
		vuint32_t NCFS14 : 1; /* NCF 14 status */
		vuint32_t NCFS13 : 1; /* NCF 13 status */
		vuint32_t NCFS12 : 1; /* NCF 12 status */
		vuint32_t NCFS11 : 1; /* NCF 11 status */
		vuint32_t NCFS10 : 1; /* NCF 10 status */
		vuint32_t NCFS9 : 1;	/* NCF 9 status */
		vuint32_t NCFS8 : 1;	/* NCF 8 status */
		vuint32_t NCFS7 : 1;	/* NCF 7 status */
		vuint32_t NCFS6 : 1;	/* NCF 6 status */
		vuint32_t NCFS5 : 1;	/* NCF 5 status */
		vuint32_t NCFS4 : 1;	/* NCF 4 status */
		vuint32_t NCFS3 : 1;	/* NCF 3 status */
		vuint32_t NCFS2 : 1;	/* NCF 2 status */
		vuint32_t NCFS1 : 1;	/* NCF 1 status */
		vuint32_t NCFS0 : 1;	/* NCF 0 status */
	} B;
} FCCU_NCFS0_32B_tag;

typedef union { /* FCCU NCF Status Register 1 */
	vuint32_t R;
	struct {
		vuint32_t NCFS63 : 1; /* NCF 63 status */
		vuint32_t NCFS62 : 1; /* NCF 62 status */
		vuint32_t NCFS61 : 1; /* NCF 61 status */
		vuint32_t NCFS60 : 1; /* NCF 60 status */
		vuint32_t NCFS59 : 1; /* NCF 59 status */
		vuint32_t NCFS58 : 1; /* NCF 58 status */
		vuint32_t NCFS57 : 1; /* NCF 57 status */
		vuint32_t NCFS56 : 1; /* NCF 56 status */
		vuint32_t NCFS55 : 1; /* NCF 55 status */
		vuint32_t NCFS54 : 1; /* NCF 54 status */
		vuint32_t NCFS53 : 1; /* NCF 53 status */
		vuint32_t NCFS52 : 1; /* NCF 52 status */
		vuint32_t NCFS51 : 1; /* NCF 51 status */
		vuint32_t NCFS50 : 1; /* NCF 50 status */
		vuint32_t NCFS49 : 1; /* NCF 49 status */
		vuint32_t NCFS48 : 1; /* NCF 48 status */
		vuint32_t NCFS47 : 1; /* NCF 47 status */
		vuint32_t NCFS46 : 1; /* NCF 46 status */
		vuint32_t NCFS45 : 1; /* NCF 45 status */
		vuint32_t NCFS44 : 1; /* NCF 44 status */
		vuint32_t NCFS43 : 1; /* NCF 43 status */
		vuint32_t NCFS42 : 1; /* NCF 42 status */
		vuint32_t NCFS41 : 1; /* NCF 41 status */
		vuint32_t NCFS40 : 1; /* NCF 40 status */
		vuint32_t NCFS39 : 1; /* NCF 39 status */
		vuint32_t NCFS38 : 1; /* NCF 38 status */
		vuint32_t NCFS37 : 1; /* NCF 37 status */
		vuint32_t NCFS36 : 1; /* NCF 36 status */
		vuint32_t NCFS35 : 1; /* NCF 35 status */
		vuint32_t NCFS34 : 1; /* NCF 34 status */
		vuint32_t NCFS33 : 1; /* NCF 33 status */
		vuint32_t NCFS32 : 1; /* NCF 32 status */
	} B;
} FCCU_NCFS1_32B_tag;

typedef union { /* FCCU NCF Status Register 2 */
	vuint32_t R;
	struct {
		vuint32_t NCFS95 : 1; /* NCF 95 status */
		vuint32_t NCFS94 : 1; /* NCF 94 status */
		vuint32_t NCFS93 : 1; /* NCF 93 status */
		vuint32_t NCFS92 : 1; /* NCF 92 status */
		vuint32_t NCFS91 : 1; /* NCF 91 status */
		vuint32_t NCFS90 : 1; /* NCF 90 status */
		vuint32_t NCFS89 : 1; /* NCF 89 status */
		vuint32_t NCFS88 : 1; /* NCF 88 status */
		vuint32_t NCFS87 : 1; /* NCF 87 status */
		vuint32_t NCFS86 : 1; /* NCF 86 status */
		vuint32_t NCFS85 : 1; /* NCF 85 status */
		vuint32_t NCFS84 : 1; /* NCF 84 status */
		vuint32_t NCFS83 : 1; /* NCF 83 status */
		vuint32_t NCFS82 : 1; /* NCF 82 status */
		vuint32_t NCFS81 : 1; /* NCF 81 status */
		vuint32_t NCFS80 : 1; /* NCF 80 status */
		vuint32_t NCFS79 : 1; /* NCF 79 status */
		vuint32_t NCFS78 : 1; /* NCF 78 status */
		vuint32_t NCFS77 : 1; /* NCF 77 status */
		vuint32_t NCFS76 : 1; /* NCF 76 status */
		vuint32_t NCFS75 : 1; /* NCF 75 status */
		vuint32_t NCFS74 : 1; /* NCF 74 status */
		vuint32_t NCFS73 : 1; /* NCF 73 status */
		vuint32_t NCFS72 : 1; /* NCF 72 status */
		vuint32_t NCFS71 : 1; /* NCF 71 status */
		vuint32_t NCFS70 : 1; /* NCF 70 status */
		vuint32_t NCFS69 : 1; /* NCF 69 status */
		vuint32_t NCFS68 : 1; /* NCF 68 status */
		vuint32_t NCFS67 : 1; /* NCF 67 status */
		vuint32_t NCFS66 : 1; /* NCF 66 status */
		vuint32_t NCFS65 : 1; /* NCF 65 status */
		vuint32_t NCFS64 : 1; /* NCF 64 status */
	} B;
} FCCU_NCFS2_32B_tag;

typedef union { /* FCCU NCF Status Register 3 */
	vuint32_t R;
	struct {
		vuint32_t NCFS127 : 1; /* NCF 127 status */
		vuint32_t NCFS126 : 1; /* NCF 126 status */
		vuint32_t NCFS125 : 1; /* NCF 125 status */
		vuint32_t NCFS124 : 1; /* NCF 124 status */
		vuint32_t NCFS123 : 1; /* NCF 123 status */
		vuint32_t NCFS122 : 1; /* NCF 122 status */
		vuint32_t NCFS121 : 1; /* NCF 121 status */
		vuint32_t NCFS120 : 1; /* NCF 120 status */
		vuint32_t NCFS119 : 1; /* NCF 119 status */
		vuint32_t NCFS118 : 1; /* NCF 118 status */
		vuint32_t NCFS117 : 1; /* NCF 117 status */
		vuint32_t NCFS116 : 1; /* NCF 116 status */
		vuint32_t NCFS115 : 1; /* NCF 115 status */
		vuint32_t NCFS114 : 1; /* NCF 114 status */
		vuint32_t NCFS113 : 1; /* NCF 113 status */
		vuint32_t NCFS112 : 1; /* NCF 112 status */
		vuint32_t NCFS111 : 1; /* NCF 111 status */
		vuint32_t NCFS110 : 1; /* NCF 110 status */
		vuint32_t NCFS109 : 1; /* NCF 109 status */
		vuint32_t NCFS108 : 1; /* NCF 108 status */
		vuint32_t NCFS107 : 1; /* NCF 107 status */
		vuint32_t NCFS106 : 1; /* NCF 106 status */
		vuint32_t NCFS105 : 1; /* NCF 105 status */
		vuint32_t NCFS104 : 1; /* NCF 104 status */
		vuint32_t NCFS103 : 1; /* NCF 103 status */
		vuint32_t NCFS102 : 1; /* NCF 102 status */
		vuint32_t NCFS101 : 1; /* NCF 101 status */
		vuint32_t NCFS100 : 1; /* NCF 100 status */
		vuint32_t NCFS99 : 1;	 /* NCF 99 status */
		vuint32_t NCFS98 : 1;	 /* NCF 98 status */
		vuint32_t NCFS97 : 1;	 /* NCF 97 status */
		vuint32_t NCFS96 : 1;	 /* NCF 96 status */
	} B;
} FCCU_NCFS3_32B_tag;

typedef union { /* FCCU_NCFK - FCCU NCF Key Register */
	vuint32_t R;
} FCCU_NCFK_32B_tag;

typedef union { /* FCCU NCF Enable Register 0 */
	vuint32_t R;
	struct {
		vuint32_t NCFE31 : 1; /* NCF 31 enable */
		vuint32_t NCFE30 : 1; /* NCF 30 enable */
		vuint32_t NCFE29 : 1; /* NCF 29 enable */
		vuint32_t NCFE28 : 1; /* NCF 28 enable */
		vuint32_t NCFE27 : 1; /* NCF 27 enable */
		vuint32_t NCFE26 : 1; /* NCF 26 enable */
		vuint32_t NCFE25 : 1; /* NCF 25 enable */
		vuint32_t NCFE24 : 1; /* NCF 24 enable */
		vuint32_t NCFE23 : 1; /* NCF 23 enable */
		vuint32_t NCFE22 : 1; /* NCF 22 enable */
		vuint32_t NCFE21 : 1; /* NCF 21 enable */
		vuint32_t NCFE20 : 1; /* NCF 20 enable */
		vuint32_t NCFE19 : 1; /* NCF 19 enable */
		vuint32_t NCFE18 : 1; /* NCF 18 enable */
		vuint32_t NCFE17 : 1; /* NCF 17 enable */
		vuint32_t NCFE16 : 1; /* NCF 16 enable */
		vuint32_t NCFE15 : 1; /* NCF 15 enable */
		vuint32_t NCFE14 : 1; /* NCF 14 enable */
		vuint32_t NCFE13 : 1; /* NCF 13 enable */
		vuint32_t NCFE12 : 1; /* NCF 12 enable */
		vuint32_t NCFE11 : 1; /* NCF 11 enable */
		vuint32_t NCFE10 : 1; /* NCF 10 enable */
		vuint32_t NCFE9 : 1;	/* NCF 9 enable */
		vuint32_t NCFE8 : 1;	/* NCF 8 enable */
		vuint32_t NCFE7 : 1;	/* NCF 7 enable */
		vuint32_t NCFE6 : 1;	/* NCF 6 enable */
		vuint32_t NCFE5 : 1;	/* NCF 5 enable */
		vuint32_t NCFE4 : 1;	/* NCF 4 enable */
		vuint32_t NCFE3 : 1;	/* NCF 3 enable */
		vuint32_t NCFE2 : 1;	/* NCF 2 enable */
		vuint32_t NCFE1 : 1;	/* NCF 1 enable */
		vuint32_t NCFE0 : 1;	/* NCF 0 enable */
	} B;
} FCCU_NCFE0_32B_tag;

typedef union { /* FCCU NCF Enable Register 1 */
	vuint32_t R;
	struct {
		vuint32_t NCFE63 : 1; /* NCF 63 enable */
		vuint32_t NCFE62 : 1; /* NCF 62 enable */
		vuint32_t NCFE61 : 1; /* NCF 61 enable */
		vuint32_t NCFE60 : 1; /* NCF 60 enable */
		vuint32_t NCFE59 : 1; /* NCF 59 enable */
		vuint32_t NCFE58 : 1; /* NCF 58 enable */
		vuint32_t NCFE57 : 1; /* NCF 57 enable */
		vuint32_t NCFE56 : 1; /* NCF 56 enable */
		vuint32_t NCFE55 : 1; /* NCF 55 enable */
		vuint32_t NCFE54 : 1; /* NCF 54 enable */
		vuint32_t NCFE53 : 1; /* NCF 53 enable */
		vuint32_t NCFE52 : 1; /* NCF 52 enable */
		vuint32_t NCFE51 : 1; /* NCF 51 enable */
		vuint32_t NCFE50 : 1; /* NCF 50 enable */
		vuint32_t NCFE49 : 1; /* NCF 49 enable */
		vuint32_t NCFE48 : 1; /* NCF 48 enable */
		vuint32_t NCFE47 : 1; /* NCF 47 enable */
		vuint32_t NCFE46 : 1; /* NCF 46 enable */
		vuint32_t NCFE45 : 1; /* NCF 45 enable */
		vuint32_t NCFE44 : 1; /* NCF 44 enable */
		vuint32_t NCFE43 : 1; /* NCF 43 enable */
		vuint32_t NCFE42 : 1; /* NCF 42 enable */
		vuint32_t NCFE41 : 1; /* NCF 41 enable */
		vuint32_t NCFE40 : 1; /* NCF 40 enable */
		vuint32_t NCFE39 : 1; /* NCF 39 enable */
		vuint32_t NCFE38 : 1; /* NCF 38 enable */
		vuint32_t NCFE37 : 1; /* NCF 37 enable */
		vuint32_t NCFE36 : 1; /* NCF 36 enable */
		vuint32_t NCFE35 : 1; /* NCF 35 enable */
		vuint32_t NCFE34 : 1; /* NCF 34 enable */
		vuint32_t NCFE33 : 1; /* NCF 33 enable */
		vuint32_t NCFE32 : 1; /* NCF 32 enable */
	} B;
} FCCU_NCFE1_32B_tag;

typedef union { /* FCCU NCF Enable Register 2 */
	vuint32_t R;
	struct {
		vuint32_t NCFE95 : 1; /* NCF 95 enable */
		vuint32_t NCFE94 : 1; /* NCF 94 enable */
		vuint32_t NCFE93 : 1; /* NCF 93 enable */
		vuint32_t NCFE92 : 1; /* NCF 92 enable */
		vuint32_t NCFE91 : 1; /* NCF 91 enable */
		vuint32_t NCFE90 : 1; /* NCF 90 enable */
		vuint32_t NCFE89 : 1; /* NCF 89 enable */
		vuint32_t NCFE88 : 1; /* NCF 88 enable */
		vuint32_t NCFE87 : 1; /* NCF 87 enable */
		vuint32_t NCFE86 : 1; /* NCF 86 enable */
		vuint32_t NCFE85 : 1; /* NCF 85 enable */
		vuint32_t NCFE84 : 1; /* NCF 84 enable */
		vuint32_t NCFE83 : 1; /* NCF 83 enable */
		vuint32_t NCFE82 : 1; /* NCF 82 enable */
		vuint32_t NCFE81 : 1; /* NCF 81 enable */
		vuint32_t NCFE80 : 1; /* NCF 80 enable */
		vuint32_t NCFE79 : 1; /* NCF 79 enable */
		vuint32_t NCFE78 : 1; /* NCF 78 enable */
		vuint32_t NCFE77 : 1; /* NCF 77 enable */
		vuint32_t NCFE76 : 1; /* NCF 76 enable */
		vuint32_t NCFE75 : 1; /* NCF 75 enable */
		vuint32_t NCFE74 : 1; /* NCF 74 enable */
		vuint32_t NCFE73 : 1; /* NCF 73 enable */
		vuint32_t NCFE72 : 1; /* NCF 72 enable */
		vuint32_t NCFE71 : 1; /* NCF 71 enable */
		vuint32_t NCFE70 : 1; /* NCF 70 enable */
		vuint32_t NCFE69 : 1; /* NCF 69 enable */
		vuint32_t NCFE68 : 1; /* NCF 68 enable */
		vuint32_t NCFE67 : 1; /* NCF 67 enable */
		vuint32_t NCFE66 : 1; /* NCF 66 enable */
		vuint32_t NCFE65 : 1; /* NCF 65 enable */
		vuint32_t NCFE64 : 1; /* NCF 64 enable */
	} B;
} FCCU_NCFE2_32B_tag;

typedef union { /* FCCU NCF Enable Register 3 */
	vuint32_t R;
	struct {
		vuint32_t NCFE127 : 1; /* NCF 127 enable */
		vuint32_t NCFE126 : 1; /* NCF 126 enable */
		vuint32_t NCFE125 : 1; /* NCF 125 enable */
		vuint32_t NCFE124 : 1; /* NCF 124 enable */
		vuint32_t NCFE123 : 1; /* NCF 123 enable */
		vuint32_t NCFE122 : 1; /* NCF 122 enable */
		vuint32_t NCFE121 : 1; /* NCF 121 enable */
		vuint32_t NCFE120 : 1; /* NCF 120 enable */
		vuint32_t NCFE119 : 1; /* NCF 119 enable */
		vuint32_t NCFE118 : 1; /* NCF 118 enable */
		vuint32_t NCFE117 : 1; /* NCF 117 enable */
		vuint32_t NCFE116 : 1; /* NCF 116 enable */
		vuint32_t NCFE115 : 1; /* NCF 115 enable */
		vuint32_t NCFE114 : 1; /* NCF 114 enable */
		vuint32_t NCFE113 : 1; /* NCF 113 enable */
		vuint32_t NCFE112 : 1; /* NCF 112 enable */
		vuint32_t NCFE111 : 1; /* NCF 111 enable */
		vuint32_t NCFE110 : 1; /* NCF 110 enable */
		vuint32_t NCFE109 : 1; /* NCF 109 enable */
		vuint32_t NCFE108 : 1; /* NCF 108 enable */
		vuint32_t NCFE107 : 1; /* NCF 107 enable */
		vuint32_t NCFE106 : 1; /* NCF 106 enable */
		vuint32_t NCFE105 : 1; /* NCF 105 enable */
		vuint32_t NCFE104 : 1; /* NCF 104 enable */
		vuint32_t NCFE103 : 1; /* NCF 103 enable */
		vuint32_t NCFE102 : 1; /* NCF 102 enable */
		vuint32_t NCFE101 : 1; /* NCF 101 enable */
		vuint32_t NCFE100 : 1; /* NCF 100 enable */
		vuint32_t NCFE99 : 1;	 /* NCF 99 enable */
		vuint32_t NCFE98 : 1;	 /* NCF 98 enable */
		vuint32_t NCFE97 : 1;	 /* NCF 97 enable */
		vuint32_t NCFE96 : 1;	 /* NCF 96 enable */
	} B;
} FCCU_NCFE3_32B_tag;

typedef union { /* FCCU NCF Time-out Enable Register 0 */
	vuint32_t R;
	struct {
		vuint32_t NCFTOE31 : 1; /* NCF 31 time-out enable */
		vuint32_t NCFTOE30 : 1; /* NCF 30 time-out enable */
		vuint32_t NCFTOE29 : 1; /* NCF 29 time-out enable */
		vuint32_t NCFTOE28 : 1; /* NCF 28 time-out enable */
		vuint32_t NCFTOE27 : 1; /* NCF 27 time-out enable */
		vuint32_t NCFTOE26 : 1; /* NCF 26 time-out enable */
		vuint32_t NCFTOE25 : 1; /* NCF 25 time-out enable */
		vuint32_t NCFTOE24 : 1; /* NCF 24 time-out enable */
		vuint32_t NCFTOE23 : 1; /* NCF 23 time-out enable */
		vuint32_t NCFTOE22 : 1; /* NCF 22 time-out enable */
		vuint32_t NCFTOE21 : 1; /* NCF 21 time-out enable */
		vuint32_t NCFTOE20 : 1; /* NCF 20 time-out enable */
		vuint32_t NCFTOE19 : 1; /* NCF 19 time-out enable */
		vuint32_t NCFTOE18 : 1; /* NCF 18 time-out enable */
		vuint32_t NCFTOE17 : 1; /* NCF 17 time-out enable */
		vuint32_t NCFTOE16 : 1; /* NCF 16 time-out enable */
		vuint32_t NCFTOE15 : 1; /* NCF 15 time-out enable */
		vuint32_t NCFTOE14 : 1; /* NCF 14 time-out enable */
		vuint32_t NCFTOE13 : 1; /* NCF 13 time-out enable */
		vuint32_t NCFTOE12 : 1; /* NCF 12 time-out enable */
		vuint32_t NCFTOE11 : 1; /* NCF 11 time-out enable */
		vuint32_t NCFTOE10 : 1; /* NCF 10 time-out enable */
		vuint32_t NCFTOE9 : 1;	/* NCF 9 time-out enable */
		vuint32_t NCFTOE8 : 1;	/* NCF 8 time-out enable */
		vuint32_t NCFTOE7 : 1;	/* NCF 7 time-out enable */
		vuint32_t NCFTOE6 : 1;	/* NCF 6 time-out enable */
		vuint32_t NCFTOE5 : 1;	/* NCF 5 time-out enable */
		vuint32_t NCFTOE4 : 1;	/* NCF 4 time-out enable */
		vuint32_t NCFTOE3 : 1;	/* NCF 3 time-out enable */
		vuint32_t NCFTOE2 : 1;	/* NCF 2 time-out enable */
		vuint32_t NCFTOE1 : 1;	/* NCF 1 time-out enable */
		vuint32_t NCFTOE0 : 1;	/* NCF 0 time-out enable */
	} B;
} FCCU_NCF_TOE0_32B_tag;

typedef union { /* FCCU NCF Time-out Enable Register 1 */
	vuint32_t R;
	struct {
		vuint32_t NCFTOE63 : 1; /* NCF 63 time-out enable */
		vuint32_t NCFTOE62 : 1; /* NCF 62 time-out enable */
		vuint32_t NCFTOE61 : 1; /* NCF 61 time-out enable */
		vuint32_t NCFTOE60 : 1; /* NCF 60 time-out enable */
		vuint32_t NCFTOE59 : 1; /* NCF 59 time-out enable */
		vuint32_t NCFTOE58 : 1; /* NCF 58 time-out enable */
		vuint32_t NCFTOE57 : 1; /* NCF 57 time-out enable */
		vuint32_t NCFTOE56 : 1; /* NCF 56 time-out enable */
		vuint32_t NCFTOE55 : 1; /* NCF 55 time-out enable */
		vuint32_t NCFTOE54 : 1; /* NCF 54 time-out enable */
		vuint32_t NCFTOE53 : 1; /* NCF 53 time-out enable */
		vuint32_t NCFTOE52 : 1; /* NCF 52 time-out enable */
		vuint32_t NCFTOE51 : 1; /* NCF 51 time-out enable */
		vuint32_t NCFTOE50 : 1; /* NCF 50 time-out enable */
		vuint32_t NCFTOE49 : 1; /* NCF 49 time-out enable */
		vuint32_t NCFTOE48 : 1; /* NCF 48 time-out enable */
		vuint32_t NCFTOE47 : 1; /* NCF 47 time-out enable */
		vuint32_t NCFTOE46 : 1; /* NCF 46 time-out enable */
		vuint32_t NCFTOE45 : 1; /* NCF 45 time-out enable */
		vuint32_t NCFTOE44 : 1; /* NCF 44 time-out enable */
		vuint32_t NCFTOE43 : 1; /* NCF 43 time-out enable */
		vuint32_t NCFTOE42 : 1; /* NCF 42 time-out enable */
		vuint32_t NCFTOE41 : 1; /* NCF 41 time-out enable */
		vuint32_t NCFTOE40 : 1; /* NCF 40 time-out enable */
		vuint32_t NCFTOE39 : 1; /* NCF 39 time-out enable */
		vuint32_t NCFTOE38 : 1; /* NCF 38 time-out enable */
		vuint32_t NCFTOE37 : 1; /* NCF 37 time-out enable */
		vuint32_t NCFTOE36 : 1; /* NCF 36 time-out enable */
		vuint32_t NCFTOE35 : 1; /* NCF 35 time-out enable */
		vuint32_t NCFTOE34 : 1; /* NCF 34 time-out enable */
		vuint32_t NCFTOE33 : 1; /* NCF 33 time-out enable */
		vuint32_t NCFTOE32 : 1; /* NCF 32 time-out enable */
	} B;
} FCCU_NCF_TOE1_32B_tag;

typedef union { /* FCCU NCF Time-out Enable Register 2 */
	vuint32_t R;
	struct {
		vuint32_t NCFTOE95 : 1; /* NCF 95 time-out enable */
		vuint32_t NCFTOE94 : 1; /* NCF 94 time-out enable */
		vuint32_t NCFTOE93 : 1; /* NCF 93 time-out enable */
		vuint32_t NCFTOE92 : 1; /* NCF 92 time-out enable */
		vuint32_t NCFTOE91 : 1; /* NCF 91 time-out enable */
		vuint32_t NCFTOE90 : 1; /* NCF 90 time-out enable */
		vuint32_t NCFTOE89 : 1; /* NCF 89 time-out enable */
		vuint32_t NCFTOE88 : 1; /* NCF 88 time-out enable */
		vuint32_t NCFTOE87 : 1; /* NCF 87 time-out enable */
		vuint32_t NCFTOE86 : 1; /* NCF 86 time-out enable */
		vuint32_t NCFTOE85 : 1; /* NCF 85 time-out enable */
		vuint32_t NCFTOE84 : 1; /* NCF 84 time-out enable */
		vuint32_t NCFTOE83 : 1; /* NCF 83 time-out enable */
		vuint32_t NCFTOE82 : 1; /* NCF 82 time-out enable */
		vuint32_t NCFTOE81 : 1; /* NCF 81 time-out enable */
		vuint32_t NCFTOE80 : 1; /* NCF 80 time-out enable */
		vuint32_t NCFTOE79 : 1; /* NCF 79 time-out enable */
		vuint32_t NCFTOE78 : 1; /* NCF 78 time-out enable */
		vuint32_t NCFTOE77 : 1; /* NCF 77 time-out enable */
		vuint32_t NCFTOE76 : 1; /* NCF 76 time-out enable */
		vuint32_t NCFTOE75 : 1; /* NCF 75 time-out enable */
		vuint32_t NCFTOE74 : 1; /* NCF 74 time-out enable */
		vuint32_t NCFTOE73 : 1; /* NCF 73 time-out enable */
		vuint32_t NCFTOE72 : 1; /* NCF 72 time-out enable */
		vuint32_t NCFTOE71 : 1; /* NCF 71 time-out enable */
		vuint32_t NCFTOE70 : 1; /* NCF 70 time-out enable */
		vuint32_t NCFTOE69 : 1; /* NCF 69 time-out enable */
		vuint32_t NCFTOE68 : 1; /* NCF 68 time-out enable */
		vuint32_t NCFTOE67 : 1; /* NCF 67 time-out enable */
		vuint32_t NCFTOE66 : 1; /* NCF 66 time-out enable */
		vuint32_t NCFTOE65 : 1; /* NCF 65 time-out enable */
		vuint32_t NCFTOE64 : 1; /* NCF 64 time-out enable */
	} B;
} FCCU_NCF_TOE2_32B_tag;

typedef union { /* FCCU NCF Time-out Enable Register 3 */
	vuint32_t R;
	struct {
		vuint32_t NCFTOE127 : 1; /* NCF 127 time-out enable */
		vuint32_t NCFTOE126 : 1; /* NCF 126 time-out enable */
		vuint32_t NCFTOE125 : 1; /* NCF 125 time-out enable */
		vuint32_t NCFTOE124 : 1; /* NCF 124 time-out enable */
		vuint32_t NCFTOE123 : 1; /* NCF 123 time-out enable */
		vuint32_t NCFTOE122 : 1; /* NCF 122 time-out enable */
		vuint32_t NCFTOE121 : 1; /* NCF 121 time-out enable */
		vuint32_t NCFTOE120 : 1; /* NCF 120 time-out enable */
		vuint32_t NCFTOE119 : 1; /* NCF 119 time-out enable */
		vuint32_t NCFTOE118 : 1; /* NCF 118 time-out enable */
		vuint32_t NCFTOE117 : 1; /* NCF 117 time-out enable */
		vuint32_t NCFTOE116 : 1; /* NCF 116 time-out enable */
		vuint32_t NCFTOE115 : 1; /* NCF 115 time-out enable */
		vuint32_t NCFTOE114 : 1; /* NCF 114 time-out enable */
		vuint32_t NCFTOE113 : 1; /* NCF 113 time-out enable */
		vuint32_t NCFTOE112 : 1; /* NCF 112 time-out enable */
		vuint32_t NCFTOE111 : 1; /* NCF 111 time-out enable */
		vuint32_t NCFTOE110 : 1; /* NCF 110 time-out enable */
		vuint32_t NCFTOE109 : 1; /* NCF 109 time-out enable */
		vuint32_t NCFTOE108 : 1; /* NCF 108 time-out enable */
		vuint32_t NCFTOE107 : 1; /* NCF 107 time-out enable */
		vuint32_t NCFTOE106 : 1; /* NCF 106 time-out enable */
		vuint32_t NCFTOE105 : 1; /* NCF 105 time-out enable */
		vuint32_t NCFTOE104 : 1; /* NCF 104 time-out enable */
		vuint32_t NCFTOE103 : 1; /* NCF 103 time-out enable */
		vuint32_t NCFTOE102 : 1; /* NCF 102 time-out enable */
		vuint32_t NCFTOE101 : 1; /* NCF 101 time-out enable */
		vuint32_t NCFTOE100 : 1; /* NCF 100 time-out enable */
		vuint32_t NCFTOE99 : 1;	 /* NCF 99 time-out enable */
		vuint32_t NCFTOE98 : 1;	 /* NCF 98 time-out enable */
		vuint32_t NCFTOE97 : 1;	 /* NCF 97 time-out enable */
		vuint32_t NCFTOE96 : 1;	 /* NCF 96 time-out enable */
	} B;
} FCCU_NCF_TOE3_32B_tag;

typedef union { /* FCCU_NCF_TO - FCCU NCF Time-out Register */
	vuint32_t R;
} FCCU_NCF_TO_32B_tag;

typedef union { /* FCCU_CFG_TO - FCCU CFG Timeout Register */
	vuint32_t R;
	struct {
		vuint32_t : 29;
		vuint32_t TO : 3; /* Configuration time-out */
	} B;
} FCCU_CFG_TO_32B_tag;

typedef union { /* FCCU_EINOUT - FCCU IO Control Register */
	vuint32_t R;
	struct {
		vuint32_t : 26;
		vuint32_t EIN1 : 1; /* Error input 1 */
		vuint32_t EIN0 : 1; /* Error input 0 */
		vuint32_t : 2;
		vuint32_t EOUT1 : 1; /* Error out 1 */
		vuint32_t EOUT0 : 1; /* Error out 0 */
	} B;
} FCCU_EINOUT_32B_tag;

typedef union { /* FCCU_STAT - FCCU Status Register */
	vuint32_t R;
	struct {
		vuint32_t : 29;
		vuint32_t STATUS : 3; /* FCCU status */
	} B;
} FCCU_STAT_32B_tag;

typedef union { /* FCCU_NAFS - FCCU NA Freeze Status Register */
	vuint32_t R;
	struct {
		vuint32_t : 24;
		vuint32_t N2AFSTATUS : 8; /* Normal to Alarm Frozen Status */
	} B;
} FCCU_NAFS_32B_tag;

typedef union { /* FCCU_AFFS - FCCU AF Freeze Status Register */
	vuint32_t R;
	struct {
		vuint32_t : 22;
		vuint32_t AFFS_SRC : 2;		/* Fault source */
		vuint32_t A2AFSTATUS : 8; /* Alarm to Fault Frozen Status */
	} B;
} FCCU_AFFS_32B_tag;

typedef union { /* FCCU_NFFS - FCCU NF Freeze Status Register */
	vuint32_t R;
	struct {
		vuint32_t : 22;
		vuint32_t NFFS_SRC : 2;	 /* Fault source */
		vuint32_t NFFS_NFFS : 8; /* Normal to Fault Frozen Status */
	} B;
} FCCU_NFFS_32B_tag;

typedef union { /* FCCU_FAFS - FCCU FA Freeze Status Register */
	vuint32_t R;
	struct {
		vuint32_t : 24;
		vuint32_t FAFS_FAFS : 8; /* Fault to Normal Frozen Status */
	} B;
} FCCU_FAFS_32B_tag;

typedef union { /* FCCU_SCFS - FCCU SC Freeze Status Register */
	vuint32_t R;
	struct {
		vuint32_t : 30;
		vuint32_t RCCS1 : 1; /* RCC1 Status */
		vuint32_t RCCS0 : 1; /* RCC0 Status */
	} B;
} FCCU_SCFS_32B_tag;

typedef union { /* FCCU_CFF - FCCU CF Fake Register */
	vuint32_t R;
	struct {
		vuint32_t : 25;
		vuint32_t FCFC : 7; /* Fake critical fault code */
	} B;
} FCCU_CFF_32B_tag;

typedef union { /* FCCU_NCFF - FCCU NCF Fake Register */
	vuint32_t R;
	struct {
		vuint32_t : 25;
		vuint32_t FNCFC : 7; /* Fake non-critical fault code */
	} B;
} FCCU_NCFF_32B_tag;

typedef union { /* FCCU_IRQ_STAT - FCCU IRQ Status Register */
	vuint32_t R;
	struct {
		vuint32_t : 29;
		vuint32_t NMI_STAT : 1;		 /* NMI Interrupt Status */
		vuint32_t ALRM_STAT : 1;	 /* Alarm Interrupt Status */
		vuint32_t CFG_TO_STAT : 1; /* Configuration Time-out Status */
	} B;
} FCCU_IRQ_STAT_32B_tag;

typedef union { /* FCCU_IRQ_EN - FCCU IRQ Enable Register */
	vuint32_t R;
	struct {
		vuint32_t : 31;
		vuint32_t CFG_TO_IEN : 1; /* Configuration Time-out Interrupt Enable */
	} B;
} FCCU_IRQ_EN_32B_tag;

typedef union { /* FCCU_XTMR - FCCU XTMR Register */
	vuint32_t R;
	struct {
		vuint32_t XTMR_XTMR : 32; /* Alarm/Watchdog/safe request timer */
	} B;
} FCCU_XTMR_32B_tag;

typedef union { /* FCCU_MCS - FCCU MCS Register */
	vuint32_t R;
	struct {
		vuint32_t VL3 : 1; /* Valid */
		vuint32_t FS3 : 1; /* Fault Status */
		vuint32_t : 2;
		vuint32_t MCS3 : 4; /* Magic Carpet oldest state */
		vuint32_t VL2 : 1;	/* Valid */
		vuint32_t FS2 : 1;	/* Fault Status */
		vuint32_t : 2;
		vuint32_t MCS2 : 4; /* Magic Carpet previous-previous state */
		vuint32_t VL1 : 1;	/* Valid */
		vuint32_t FS1 : 1;	/* Fault Status */
		vuint32_t : 2;
		vuint32_t MCS1 : 4; /* Magic Carpet previous state */
		vuint32_t VL0 : 1;	/* Valid */
		vuint32_t FS0 : 1;	/* Fault Status */
		vuint32_t : 2;
		vuint32_t MCS0 : 4; /* Magic Carpet latest state */
	} B;
} FCCU_MCS_32B_tag;

/* Register layout for generated register(s) CF_CFG... */

typedef union { /*  */
	vuint32_t R;
} FCCU_CF_CFG_32B_tag;

/* Register layout for generated register(s) NCF_CFG... */

typedef union { /*  */
	vuint32_t R;
} FCCU_NCF_CFG_32B_tag;

/* Register layout for generated register(s) CFS_CFG... */

typedef union { /*  */
	vuint32_t R;
} FCCU_CFS_CFG_32B_tag;

/* Register layout for generated register(s) NCFS_CFG... */

typedef union { /*  */
	vuint32_t R;
} FCCU_NCFS_CFG_32B_tag;

/* Register layout for generated register(s) CFS... */

typedef union { /*  */
	vuint32_t R;
} FCCU_CFS_32B_tag;

/* Register layout for generated register(s) NCFS... */

typedef union { /*  */
	vuint32_t R;
} FCCU_NCFS_32B_tag;

/* Register layout for generated register(s) NCFE... */

typedef union { /*  */
	vuint32_t R;
} FCCU_NCFE_32B_tag;

/* Register layout for generated register(s) NCF_TOE... */

typedef union { /*  */
	vuint32_t R;
} FCCU_NCF_TOE_32B_tag;

typedef struct FCCU_struct_tag { /* start of FCCU_tag */
																 /* FCCU Control Register */
	FCCU_CTRL_32B_tag CTRL;				 /* offset: 0x0000 size: 32 bit */
																 /* FCCU CTRL Key Register */
	FCCU_CTRLK_32B_tag CTRLK;			 /* offset: 0x0004 size: 32 bit */
																 /* FCCU Configuration Register */
	FCCU_CFG_32B_tag CFG;					 /* offset: 0x0008 size: 32 bit */
	union {
		FCCU_CF_CFG_32B_tag CF_CFG[4]; /* offset: 0x000C  (0x0004 x 4) */

		struct {
			/* FCCU CF Configuration Register 0 */
			FCCU_CF_CFG0_32B_tag CF_CFG0; /* offset: 0x000C size: 32 bit */
																		/* FCCU CF Configuration Register 1 */
			FCCU_CF_CFG1_32B_tag CF_CFG1; /* offset: 0x0010 size: 32 bit */
																		/* FCCU CF Configuration Register 2 */
			FCCU_CF_CFG2_32B_tag CF_CFG2; /* offset: 0x0014 size: 32 bit */
																		/* FCCU CF Configuration Register 3 */
			FCCU_CF_CFG3_32B_tag CF_CFG3; /* offset: 0x0018 size: 32 bit */
		};
	};
	union {
		FCCU_NCF_CFG_32B_tag NCF_CFG[4]; /* offset: 0x001C  (0x0004 x 4) */

		struct {
			/* FCCU NCF Configuration Register 0 */
			FCCU_NCF_CFG0_32B_tag NCF_CFG0; /* offset: 0x001C size: 32 bit */
																			/* FCCU NCF Configuration Register 1 */
			FCCU_NCF_CFG1_32B_tag NCF_CFG1; /* offset: 0x0020 size: 32 bit */
																			/* FCCU NCF Configuration Register 2 */
			FCCU_NCF_CFG2_32B_tag NCF_CFG2; /* offset: 0x0024 size: 32 bit */
																			/* FCCU NCF Configuration Register 3 */
			FCCU_NCF_CFG3_32B_tag NCF_CFG3; /* offset: 0x0028 size: 32 bit */
		};
	};
	union {
		FCCU_CFS_CFG_32B_tag CFS_CFG[8]; /* offset: 0x002C  (0x0004 x 8) */

		struct {
			/* FCCU CFS Configuration Register 0 */
			FCCU_CFS_CFG0_32B_tag CFS_CFG0; /* offset: 0x002C size: 32 bit */
																			/* FCCU CFS Configuration Register 1 */
			FCCU_CFS_CFG1_32B_tag CFS_CFG1; /* offset: 0x0030 size: 32 bit */
																			/* FCCU CFS Configuration Register 2 */
			FCCU_CFS_CFG2_32B_tag CFS_CFG2; /* offset: 0x0034 size: 32 bit */
																			/* FCCU CFS Configuration Register 3 */
			FCCU_CFS_CFG3_32B_tag CFS_CFG3; /* offset: 0x0038 size: 32 bit */
																			/* FCCU CFS Configuration Register 4 */
			FCCU_CFS_CFG4_32B_tag CFS_CFG4; /* offset: 0x003C size: 32 bit */
																			/* FCCU CFS Configuration Register 5 */
			FCCU_CFS_CFG5_32B_tag CFS_CFG5; /* offset: 0x0040 size: 32 bit */
																			/* FCCU CFS Configuration Register 6 */
			FCCU_CFS_CFG6_32B_tag CFS_CFG6; /* offset: 0x0044 size: 32 bit */
																			/* FCCU CFS Configuration Register 7 */
			FCCU_CFS_CFG7_32B_tag CFS_CFG7; /* offset: 0x0048 size: 32 bit */
		};
	};
	union {
		FCCU_NCFS_CFG_32B_tag NCFS_CFG[8]; /* offset: 0x004C  (0x0004 x 8) */

		struct {
			/* FCCU NCFS Configuration Register 0 */
			FCCU_NCFS_CFG0_32B_tag NCFS_CFG0; /* offset: 0x004C size: 32 bit */
																				/* FCCU NCFS Configuration Register 1 */
			FCCU_NCFS_CFG1_32B_tag NCFS_CFG1; /* offset: 0x0050 size: 32 bit */
																				/* FCCU NCFS Configuration Register 2 */
			FCCU_NCFS_CFG2_32B_tag NCFS_CFG2; /* offset: 0x0054 size: 32 bit */
																				/* FCCU NCFS Configuration Register 3 */
			FCCU_NCFS_CFG3_32B_tag NCFS_CFG3; /* offset: 0x0058 size: 32 bit */
																				/* FCCU NCFS Configuration Register 4 */
			FCCU_NCFS_CFG4_32B_tag NCFS_CFG4; /* offset: 0x005C size: 32 bit */
																				/* FCCU NCFS Configuration Register 5 */
			FCCU_NCFS_CFG5_32B_tag NCFS_CFG5; /* offset: 0x0060 size: 32 bit */
																				/* FCCU NCFS Configuration Register 6 */
			FCCU_NCFS_CFG6_32B_tag NCFS_CFG6; /* offset: 0x0064 size: 32 bit */
																				/* FCCU NCFS Configuration Register 7 */
			FCCU_NCFS_CFG7_32B_tag NCFS_CFG7; /* offset: 0x0068 size: 32 bit */
		};
	};
	union {
		FCCU_CFS_32B_tag CFS[4]; /* offset: 0x006C  (0x0004 x 4) */

		struct {
			/* FCCU CF Status Register 0 */
			FCCU_CFS0_32B_tag CFS0; /* offset: 0x006C size: 32 bit */
															/* FCCU CF Status Register 1 */
			FCCU_CFS1_32B_tag CFS1; /* offset: 0x0070 size: 32 bit */
															/* FCCU CF Status Register 2 */
			FCCU_CFS2_32B_tag CFS2; /* offset: 0x0074 size: 32 bit */
															/* FCCU CF Status Register 3 */
			FCCU_CFS3_32B_tag CFS3; /* offset: 0x0078 size: 32 bit */
		};
	};
	/* FCCU_CFK - FCCU CF Key Register */
	FCCU_CFK_32B_tag CFK; /* offset: 0x007C size: 32 bit */
	union {
		FCCU_NCFS_32B_tag NCFS[4]; /* offset: 0x0080  (0x0004 x 4) */

		struct {
			/* FCCU NCF Status Register 0 */
			FCCU_NCFS0_32B_tag NCFS0; /* offset: 0x0080 size: 32 bit */
																/* FCCU NCF Status Register 1 */
			FCCU_NCFS1_32B_tag NCFS1; /* offset: 0x0084 size: 32 bit */
																/* FCCU NCF Status Register 2 */
			FCCU_NCFS2_32B_tag NCFS2; /* offset: 0x0088 size: 32 bit */
																/* FCCU NCF Status Register 3 */
			FCCU_NCFS3_32B_tag NCFS3; /* offset: 0x008C size: 32 bit */
		};
	};
	/* FCCU_NCFK - FCCU NCF Key Register */
	FCCU_NCFK_32B_tag NCFK; /* offset: 0x0090 size: 32 bit */
	union {
		FCCU_NCFE_32B_tag NCFE[4]; /* offset: 0x0094  (0x0004 x 4) */

		struct {
			/* FCCU NCF Enable Register 0 */
			FCCU_NCFE0_32B_tag NCFE0; /* offset: 0x0094 size: 32 bit */
																/* FCCU NCF Enable Register 1 */
			FCCU_NCFE1_32B_tag NCFE1; /* offset: 0x0098 size: 32 bit */
																/* FCCU NCF Enable Register 2 */
			FCCU_NCFE2_32B_tag NCFE2; /* offset: 0x009C size: 32 bit */
																/* FCCU NCF Enable Register 3 */
			FCCU_NCFE3_32B_tag NCFE3; /* offset: 0x00A0 size: 32 bit */
		};
	};
	union {
		FCCU_NCF_TOE_32B_tag NCF_TOE[4]; /* offset: 0x00A4  (0x0004 x 4) */

		struct {
			/* FCCU NCF Time-out Enable Register 0 */
			FCCU_NCF_TOE0_32B_tag NCF_TOE0; /* offset: 0x00A4 size: 32 bit */
																			/* FCCU NCF Time-out Enable Register 1 */
			FCCU_NCF_TOE1_32B_tag NCF_TOE1; /* offset: 0x00A8 size: 32 bit */
																			/* FCCU NCF Time-out Enable Register 2 */
			FCCU_NCF_TOE2_32B_tag NCF_TOE2; /* offset: 0x00AC size: 32 bit */
																			/* FCCU NCF Time-out Enable Register 3 */
			FCCU_NCF_TOE3_32B_tag NCF_TOE3; /* offset: 0x00B0 size: 32 bit */
		};
	};
	/* FCCU_NCF_TO - FCCU NCF Time-out Register */
	FCCU_NCF_TO_32B_tag NCF_TO; /* offset: 0x00B4 size: 32 bit */
															/* FCCU_CFG_TO - FCCU CFG Timeout Register */
	FCCU_CFG_TO_32B_tag CFG_TO; /* offset: 0x00B8 size: 32 bit */
															/* FCCU_EINOUT - FCCU IO Control Register */
	FCCU_EINOUT_32B_tag EINOUT; /* offset: 0x00BC size: 32 bit */
															/* FCCU_STAT - FCCU Status Register */
	FCCU_STAT_32B_tag STAT;			/* offset: 0x00C0 size: 32 bit */
															/* FCCU_NAFS - FCCU NA Freeze Status Register */
	FCCU_NAFS_32B_tag NAFS;			/* offset: 0x00C4 size: 32 bit */
															/* FCCU_AFFS - FCCU AF Freeze Status Register */
	FCCU_AFFS_32B_tag AFFS;			/* offset: 0x00C8 size: 32 bit */
															/* FCCU_NFFS - FCCU NF Freeze Status Register */
	FCCU_NFFS_32B_tag NFFS;			/* offset: 0x00CC size: 32 bit */
															/* FCCU_FAFS - FCCU FA Freeze Status Register */
	FCCU_FAFS_32B_tag FAFS;			/* offset: 0x00D0 size: 32 bit */
															/* FCCU_SCFS - FCCU SC Freeze Status Register */
	FCCU_SCFS_32B_tag SCFS;			/* offset: 0x00D4 size: 32 bit */
															/* FCCU_CFF - FCCU CF Fake Register */
	FCCU_CFF_32B_tag CFF;				/* offset: 0x00D8 size: 32 bit */
															/* FCCU_NCFF - FCCU NCF Fake Register */
	FCCU_NCFF_32B_tag NCFF;			/* offset: 0x00DC size: 32 bit */
															/* FCCU_IRQ_STAT - FCCU IRQ Status Register */
	FCCU_IRQ_STAT_32B_tag IRQ_STAT; /* offset: 0x00E0 size: 32 bit */
																	/* FCCU_IRQ_EN - FCCU IRQ Enable Register */
	FCCU_IRQ_EN_32B_tag IRQ_EN;			/* offset: 0x00E4 size: 32 bit */
																	/* FCCU_XTMR - FCCU XTMR Register */
	FCCU_XTMR_32B_tag XTMR;					/* offset: 0x00E8 size: 32 bit */
																	/* FCCU_MCS - FCCU MCS Register */
	FCCU_MCS_32B_tag MCS;						/* offset: 0x00EC size: 32 bit */
} FCCU_tag;

#define FCCU (*(volatile FCCU_tag*)0xFFE6C000UL)

/****************************************************************/
/*                                                              */
/* Module: SGENDIG  */
/*                                                              */
/****************************************************************/

typedef union { /* SGENDIG_CTRL - SGENDIG Control Register */
	vuint32_t R;
	struct {
		vuint32_t LDOS : 1;		/* Operation Status */
		vuint32_t IOAMPL : 5; /* Define the AMPLitude value on I/O pad */
		vuint32_t : 2;
		vuint32_t
				SEMASK : 1; /* Sine wave generator Error MASK interrupt register */
		vuint32_t : 5;
		vuint32_t S0H1 : 1;		 /* Operation Status */
		vuint32_t PDS : 1;		 /* Operation Status */
		vuint32_t IOFREQ : 16; /* Define the FREQuency value on I/O pad */
	} B;
} SGENDIG_CTRL_32B_tag;

typedef union { /* SGENDIG_IRQE - SGENDIG Interrupt Request Enable  Register */
	vuint32_t R;
	struct {
		vuint32_t : 8;
		vuint32_t SERR : 1; /* Sine wave generator Error bit */
		vuint32_t : 3;
		vuint32_t FERR : 1; /* Sine wave generator Force Error bit */
		vuint32_t : 19;
	} B;
} SGENDIG_IRQE_32B_tag;

typedef struct SGENDIG_struct_tag { /* start of SGENDIG_tag */
	/* SGENDIG_CTRL - SGENDIG Control Register */
	SGENDIG_CTRL_32B_tag CTRL; /* offset: 0x0000 size: 32 bit */
	/* SGENDIG_IRQE - SGENDIG Interrupt Request Enable  Register */
	SGENDIG_IRQE_32B_tag IRQE; /* offset: 0x0004 size: 32 bit */
} SGENDIG_tag;

#define SGENDIG (*(volatile SGENDIG_tag*)0xFFE78000UL)

/****************************************************************/
/*                                                              */
/* Module: AIPS  */
/*                                                              */
/****************************************************************/

typedef union { /* MPROT - Master Privilege Registers */
	vuint32_t R;
	struct {
		vuint32_t MPROT0_MBW : 1; /* Master 0 Buffer Writes */
		vuint32_t MPROT0_MTR : 1; /* Master 0 Trusted for Reads */
		vuint32_t MPROT0_MTW : 1; /* Master 0 Trusted for Writes */
		vuint32_t MPROT0_MPL : 1; /* Master 0 Priviledge Level */
		vuint32_t MPROT1_MBW : 1; /* Master 1 Buffer Writes */
		vuint32_t MPROT1_MTR : 1; /* Master 1 Trusted for Reads */
		vuint32_t MPROT1_MTW : 1; /* Master 1 Trusted for Writes */
		vuint32_t MPROT1_MPL : 1; /* Master 1 Priviledge Level */
		vuint32_t MPROT2_MBW : 1; /* Master 2 Buffer Writes */
		vuint32_t MPROT2_MTR : 1; /* Master 2 Trusted for Reads */
		vuint32_t MPROT2_MTW : 1; /* Master 2 Trusted for Writes */
		vuint32_t MPROT2_MPL : 1; /* Master 2 Priviledge Level */
		vuint32_t MPROT3_MBW : 1; /* Master 3 Buffer Writes */
		vuint32_t MPROT3_MTR : 1; /* Master 3 Trusted for Reads */
		vuint32_t MPROT3_MTW : 1; /* Master 3 Trusted for Writes */
		vuint32_t MPROT3_MPL : 1; /* Master 3 Priviledge Level */
		vuint32_t MPROT4_MBW : 1; /* Master 4 Buffer Writes */
		vuint32_t MPROT4_MTR : 1; /* Master 4 Trusted for Reads */
		vuint32_t MPROT4_MTW : 1; /* Master 4 Trusted for Writes */
		vuint32_t MPROT4_MPL : 1; /* Master 4 Priviledge Level */
		vuint32_t MPROT5_MBW : 1; /* Master 5 Buffer Writes */
		vuint32_t MPROT5_MTR : 1; /* Master 5 Trusted for Reads */
		vuint32_t MPROT5_MTW : 1; /* Master 5 Trusted for Writes */
		vuint32_t MPROT5_MPL : 1; /* Master 5 Priviledge Level */
		vuint32_t MPROT6_MBW : 1; /* Master 6 Buffer Writes */
		vuint32_t MPROT6_MTR : 1; /* Master 6 Trusted for Reads */
		vuint32_t MPROT6_MTW : 1; /* Master 6 Trusted for Writes */
		vuint32_t MPROT6_MPL : 1; /* Master 6 Priviledge Level */
		vuint32_t MPROT7_MBW : 1; /* Master 7 Buffer Writes */
		vuint32_t MPROT7_MTR : 1; /* Master 7 Trusted for Reads */
		vuint32_t MPROT7_MTW : 1; /* Master 7 Trusted for Writes */
		vuint32_t MPROT7_MPL : 1; /* Master 7 Priviledge Level */
	} B;
} AIPS_MPROT_32B_tag;

typedef union { /* PACR0_7 - Peripheral Access Control Registers */
	vuint32_t R;
	struct {
		vuint32_t PACR0_BW : 1; /* Buffer Writes */
		vuint32_t PACR0_SP : 1; /* Supervisor Protect */
		vuint32_t PACR0_WP : 1; /* Write Protect */
		vuint32_t PACR0_TP : 1; /* Trusted Protect */
		vuint32_t PACR1_BW : 1; /* Buffer Writes */
		vuint32_t PACR1_SP : 1; /* Supervisor Protect */
		vuint32_t PACR1_WP : 1; /* Write Protect */
		vuint32_t PACR1_TP : 1; /* Trusted Protect */
		vuint32_t PACR2_BW : 1; /* Buffer Writes */
		vuint32_t PACR2_SP : 1; /* Supervisor Protect */
		vuint32_t PACR2_WP : 1; /* Write Protect */
		vuint32_t PACR2_TP : 1; /* Trusted Protect */
		vuint32_t PACR3_BW : 1; /* Buffer Writes */
		vuint32_t PACR3_SP : 1; /* Supervisor Protect */
		vuint32_t PACR3_WP : 1; /* Write Protect */
		vuint32_t PACR3_TP : 1; /* Trusted Protect */
		vuint32_t PACR4_BW : 1; /* Buffer Writes */
		vuint32_t PACR4_SP : 1; /* Supervisor Protect */
		vuint32_t PACR4_WP : 1; /* Write Protect */
		vuint32_t PACR4_TP : 1; /* Trusted Protect */
		vuint32_t PACR5_BW : 1; /* Buffer Writes */
		vuint32_t PACR5_SP : 1; /* Supervisor Protect */
		vuint32_t PACR5_WP : 1; /* Write Protect */
		vuint32_t PACR5_TP : 1; /* Trusted Protect */
		vuint32_t PACR6_BW : 1; /* Buffer Writes */
		vuint32_t PACR6_SP : 1; /* Supervisor Protect */
		vuint32_t PACR6_WP : 1; /* Write Protect */
		vuint32_t PACR6_TP : 1; /* Trusted Protect */
		vuint32_t PACR7_BW : 1; /* Buffer Writes */
		vuint32_t PACR7_SP : 1; /* Supervisor Protect */
		vuint32_t PACR7_WP : 1; /* Write Protect */
		vuint32_t PACR7_TP : 1; /* Trusted Protect */
	} B;
} AIPS_PACR0_7_32B_tag;

typedef union { /* PACR8_15 - Peripheral Access Control Registers */
	vuint32_t R;
	struct {
		vuint32_t PACR8_BW : 1;	 /* Buffer Writes */
		vuint32_t PACR8_SP : 1;	 /* Supervisor Protect */
		vuint32_t PACR8_WP : 1;	 /* Write Protect */
		vuint32_t PACR8_TP : 1;	 /* Trusted Protect */
		vuint32_t PACR9_BW : 1;	 /* Buffer Writes */
		vuint32_t PACR9_SP : 1;	 /* Supervisor Protect */
		vuint32_t PACR9_WP : 1;	 /* Write Protect */
		vuint32_t PACR9_TP : 1;	 /* Trusted Protect */
		vuint32_t PACR10_BW : 1; /* Buffer Writes */
		vuint32_t PACR10_SP : 1; /* Supervisor Protect */
		vuint32_t PACR10_WP : 1; /* Write Protect */
		vuint32_t PACR10_TP : 1; /* Trusted Protect */
		vuint32_t PACR11_BW : 1; /* Buffer Writes */
		vuint32_t PACR11_SP : 1; /* Supervisor Protect */
		vuint32_t PACR11_WP : 1; /* Write Protect */
		vuint32_t PACR11_TP : 1; /* Trusted Protect */
		vuint32_t PACR12_BW : 1; /* Buffer Writes */
		vuint32_t PACR12_SP : 1; /* Supervisor Protect */
		vuint32_t PACR12_WP : 1; /* Write Protect */
		vuint32_t PACR12_TP : 1; /* Trusted Protect */
		vuint32_t PACR13_BW : 1; /* Buffer Writes */
		vuint32_t PACR13_SP : 1; /* Supervisor Protect */
		vuint32_t PACR13_WP : 1; /* Write Protect */
		vuint32_t PACR13_TP : 1; /* Trusted Protect */
		vuint32_t PACR14_BW : 1; /* Buffer Writes */
		vuint32_t PACR14_SP : 1; /* Supervisor Protect */
		vuint32_t PACR14_WP : 1; /* Write Protect */
		vuint32_t PACR14_TP : 1; /* Trusted Protect */
		vuint32_t PACR15_BW : 1; /* Buffer Writes */
		vuint32_t PACR15_SP : 1; /* Supervisor Protect */
		vuint32_t PACR15_WP : 1; /* Write Protect */
		vuint32_t PACR15_TP : 1; /* Trusted Protect */
	} B;
} AIPS_PACR8_15_32B_tag;

typedef union { /* PACR16_23 - Peripheral Access Control Registers */
	vuint32_t R;
	struct {
		vuint32_t PACR16_BW : 1; /* Buffer Writes */
		vuint32_t PACR16_SP : 1; /* Supervisor Protect */
		vuint32_t PACR16_WP : 1; /* Write Protect */
		vuint32_t PACR16_TP : 1; /* Trusted Protect */
		vuint32_t PACR17_BW : 1; /* Buffer Writes */
		vuint32_t PACR17_SP : 1; /* Supervisor Protect */
		vuint32_t PACR17_WP : 1; /* Write Protect */
		vuint32_t PACR17_TP : 1; /* Trusted Protect */
		vuint32_t PACR18_BW : 1; /* Buffer Writes */
		vuint32_t PACR18_SP : 1; /* Supervisor Protect */
		vuint32_t PACR18_WP : 1; /* Write Protect */
		vuint32_t PACR18_TP : 1; /* Trusted Protect */
		vuint32_t PACR19_BW : 1; /* Buffer Writes */
		vuint32_t PACR19_SP : 1; /* Supervisor Protect */
		vuint32_t PACR19_WP : 1; /* Write Protect */
		vuint32_t PACR19_TP : 1; /* Trusted Protect */
		vuint32_t PACR20_BW : 1; /* Buffer Writes */
		vuint32_t PACR20_SP : 1; /* Supervisor Protect */
		vuint32_t PACR20_WP : 1; /* Write Protect */
		vuint32_t PACR20_TP : 1; /* Trusted Protect */
		vuint32_t PACR21_BW : 1; /* Buffer Writes */
		vuint32_t PACR21_SP : 1; /* Supervisor Protect */
		vuint32_t PACR21_WP : 1; /* Write Protect */
		vuint32_t PACR21_TP : 1; /* Trusted Protect */
		vuint32_t PACR22_BW : 1; /* Buffer Writes */
		vuint32_t PACR22_SP : 1; /* Supervisor Protect */
		vuint32_t PACR22_WP : 1; /* Write Protect */
		vuint32_t PACR22_TP : 1; /* Trusted Protect */
		vuint32_t PACR23_BW : 1; /* Buffer Writes */
		vuint32_t PACR23_SP : 1; /* Supervisor Protect */
		vuint32_t PACR23_WP : 1; /* Write Protect */
		vuint32_t PACR23_TP : 1; /* Trusted Protect */
	} B;
} AIPS_PACR16_23_32B_tag;

typedef union { /* PACR24_31 - Peripheral Access Control Registers */
	vuint32_t R;
	struct {
		vuint32_t PACR24_BW : 1; /* Buffer Writes */
		vuint32_t PACR24_SP : 1; /* Supervisor Protect */
		vuint32_t PACR24_WP : 1; /* Write Protect */
		vuint32_t PACR24_TP : 1; /* Trusted Protect */
		vuint32_t PACR25_BW : 1; /* Buffer Writes */
		vuint32_t PACR25_SP : 1; /* Supervisor Protect */
		vuint32_t PACR25_WP : 1; /* Write Protect */
		vuint32_t PACR25_TP : 1; /* Trusted Protect */
		vuint32_t PACR26_BW : 1; /* Buffer Writes */
		vuint32_t PACR26_SP : 1; /* Supervisor Protect */
		vuint32_t PACR26_WP : 1; /* Write Protect */
		vuint32_t PACR26_TP : 1; /* Trusted Protect */
		vuint32_t PACR27_BW : 1; /* Buffer Writes */
		vuint32_t PACR27_SP : 1; /* Supervisor Protect */
		vuint32_t PACR27_WP : 1; /* Write Protect */
		vuint32_t PACR27_TP : 1; /* Trusted Protect */
		vuint32_t PACR28_BW : 1; /* Buffer Writes */
		vuint32_t PACR28_SP : 1; /* Supervisor Protect */
		vuint32_t PACR28_WP : 1; /* Write Protect */
		vuint32_t PACR28_TP : 1; /* Trusted Protect */
		vuint32_t PACR29_BW : 1; /* Buffer Writes */
		vuint32_t PACR29_SP : 1; /* Supervisor Protect */
		vuint32_t PACR29_WP : 1; /* Write Protect */
		vuint32_t PACR29_TP : 1; /* Trusted Protect */
		vuint32_t PACR30_BW : 1; /* Buffer Writes */
		vuint32_t PACR30_SP : 1; /* Supervisor Protect */
		vuint32_t PACR30_WP : 1; /* Write Protect */
		vuint32_t PACR30_TP : 1; /* Trusted Protect */
		vuint32_t PACR31_BW : 1; /* Buffer Writes */
		vuint32_t PACR31_SP : 1; /* Supervisor Protect */
		vuint32_t PACR31_WP : 1; /* Write Protect */
		vuint32_t PACR31_TP : 1; /* Trusted Protect */
	} B;
} AIPS_PACR24_31_32B_tag;

typedef union { /* OPACR0_7 - Off-Platform Peripheral Access Control Registers
								 */
	vuint32_t R;
	struct {
		vuint32_t OPACR0_BW : 1; /* Buffer Writes */
		vuint32_t OPACR0_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR0_WP : 1; /* Write Protect */
		vuint32_t OPACR0_TP : 1; /* Trusted Protect */
		vuint32_t OPACR1_BW : 1; /* Buffer Writes */
		vuint32_t OPACR1_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR1_WP : 1; /* Write Protect */
		vuint32_t OPACR1_TP : 1; /* Trusted Protect */
		vuint32_t OPACR2_BW : 1; /* Buffer Writes */
		vuint32_t OPACR2_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR2_WP : 1; /* Write Protect */
		vuint32_t OPACR2_TP : 1; /* Trusted Protect */
		vuint32_t OPACR3_BW : 1; /* Buffer Writes */
		vuint32_t OPACR3_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR3_WP : 1; /* Write Protect */
		vuint32_t OPACR3_TP : 1; /* Trusted Protect */
		vuint32_t OPACR4_BW : 1; /* Buffer Writes */
		vuint32_t OPACR4_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR4_WP : 1; /* Write Protect */
		vuint32_t OPACR4_TP : 1; /* Trusted Protect */
		vuint32_t OPACR5_BW : 1; /* Buffer Writes */
		vuint32_t OPACR5_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR5_WP : 1; /* Write Protect */
		vuint32_t OPACR5_TP : 1; /* Trusted Protect */
		vuint32_t OPACR6_BW : 1; /* Buffer Writes */
		vuint32_t OPACR6_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR6_WP : 1; /* Write Protect */
		vuint32_t OPACR6_TP : 1; /* Trusted Protect */
		vuint32_t OPACR7_BW : 1; /* Buffer Writes */
		vuint32_t OPACR7_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR7_WP : 1; /* Write Protect */
		vuint32_t OPACR7_TP : 1; /* Trusted Protect */
	} B;
} AIPS_OPACR0_7_32B_tag;

typedef union { /* OPACR8_15 - Off-Platform Peripheral Access Control Registers
								 */
	vuint32_t R;
	struct {
		vuint32_t OPACR8_BW : 1;	/* Buffer Writes */
		vuint32_t OPACR8_SP : 1;	/* Supervisor Protect */
		vuint32_t OPACR8_WP : 1;	/* Write Protect */
		vuint32_t OPACR8_TP : 1;	/* Trusted Protect */
		vuint32_t OPACR9_BW : 1;	/* Buffer Writes */
		vuint32_t OPACR9_SP : 1;	/* Supervisor Protect */
		vuint32_t OPACR9_WP : 1;	/* Write Protect */
		vuint32_t OPACR9_TP : 1;	/* Trusted Protect */
		vuint32_t OPACR10_BW : 1; /* Buffer Writes */
		vuint32_t OPACR10_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR10_WP : 1; /* Write Protect */
		vuint32_t OPACR10_TP : 1; /* Trusted Protect */
		vuint32_t OPACR11_BW : 1; /* Buffer Writes */
		vuint32_t OPACR11_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR11_WP : 1; /* Write Protect */
		vuint32_t OPACR11_TP : 1; /* Trusted Protect */
		vuint32_t OPACR12_BW : 1; /* Buffer Writes */
		vuint32_t OPACR12_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR12_WP : 1; /* Write Protect */
		vuint32_t OPACR12_TP : 1; /* Trusted Protect */
		vuint32_t OPACR13_BW : 1; /* Buffer Writes */
		vuint32_t OPACR13_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR13_WP : 1; /* Write Protect */
		vuint32_t OPACR13_TP : 1; /* Trusted Protect */
		vuint32_t OPACR14_BW : 1; /* Buffer Writes */
		vuint32_t OPACR14_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR14_WP : 1; /* Write Protect */
		vuint32_t OPACR14_TP : 1; /* Trusted Protect */
		vuint32_t OPACR15_BW : 1; /* Buffer Writes */
		vuint32_t OPACR15_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR15_WP : 1; /* Write Protect */
		vuint32_t OPACR15_TP : 1; /* Trusted Protect */
	} B;
} AIPS_OPACR8_15_32B_tag;

typedef union { /* OPACR16_23 - Off-Platform Peripheral Access Control Registers
								 */
	vuint32_t R;
	struct {
		vuint32_t OPACR16_BW : 1; /* Buffer Writes */
		vuint32_t OPACR16_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR16_WP : 1; /* Write Protect */
		vuint32_t OPACR16_TP : 1; /* Trusted Protect */
		vuint32_t OPACR17_BW : 1; /* Buffer Writes */
		vuint32_t OPACR17_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR17_WP : 1; /* Write Protect */
		vuint32_t OPACR17_TP : 1; /* Trusted Protect */
		vuint32_t OPACR18_BW : 1; /* Buffer Writes */
		vuint32_t OPACR18_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR18_WP : 1; /* Write Protect */
		vuint32_t OPACR18_TP : 1; /* Trusted Protect */
		vuint32_t OPACR19_BW : 1; /* Buffer Writes */
		vuint32_t OPACR19_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR19_WP : 1; /* Write Protect */
		vuint32_t OPACR19_TP : 1; /* Trusted Protect */
		vuint32_t OPACR20_BW : 1; /* Buffer Writes */
		vuint32_t OPACR20_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR20_WP : 1; /* Write Protect */
		vuint32_t OPACR20_TP : 1; /* Trusted Protect */
		vuint32_t OPACR21_BW : 1; /* Buffer Writes */
		vuint32_t OPACR21_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR21_WP : 1; /* Write Protect */
		vuint32_t OPACR21_TP : 1; /* Trusted Protect */
		vuint32_t OPACR22_BW : 1; /* Buffer Writes */
		vuint32_t OPACR22_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR22_WP : 1; /* Write Protect */
		vuint32_t OPACR22_TP : 1; /* Trusted Protect */
		vuint32_t OPACR23_BW : 1; /* Buffer Writes */
		vuint32_t OPACR23_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR23_WP : 1; /* Write Protect */
		vuint32_t OPACR23_TP : 1; /* Trusted Protect */
	} B;
} AIPS_OPACR16_23_32B_tag;

typedef union { /* OPACR24_31 - Off-Platform Peripheral Access Control Registers
								 */
	vuint32_t R;
	struct {
		vuint32_t OPACR24_BW : 1; /* Buffer Writes */
		vuint32_t OPACR24_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR24_WP : 1; /* Write Protect */
		vuint32_t OPACR24_TP : 1; /* Trusted Protect */
		vuint32_t OPACR25_BW : 1; /* Buffer Writes */
		vuint32_t OPACR25_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR25_WP : 1; /* Write Protect */
		vuint32_t OPACR25_TP : 1; /* Trusted Protect */
		vuint32_t OPACR26_BW : 1; /* Buffer Writes */
		vuint32_t OPACR26_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR26_WP : 1; /* Write Protect */
		vuint32_t OPACR26_TP : 1; /* Trusted Protect */
		vuint32_t OPACR27_BW : 1; /* Buffer Writes */
		vuint32_t OPACR27_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR27_WP : 1; /* Write Protect */
		vuint32_t OPACR27_TP : 1; /* Trusted Protect */
		vuint32_t OPACR28_BW : 1; /* Buffer Writes */
		vuint32_t OPACR28_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR28_WP : 1; /* Write Protect */
		vuint32_t OPACR28_TP : 1; /* Trusted Protect */
		vuint32_t OPACR29_BW : 1; /* Buffer Writes */
		vuint32_t OPACR29_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR29_WP : 1; /* Write Protect */
		vuint32_t OPACR29_TP : 1; /* Trusted Protect */
		vuint32_t OPACR30_BW : 1; /* Buffer Writes */
		vuint32_t OPACR30_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR30_WP : 1; /* Write Protect */
		vuint32_t OPACR30_TP : 1; /* Trusted Protect */
		vuint32_t OPACR31_BW : 1; /* Buffer Writes */
		vuint32_t OPACR31_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR31_WP : 1; /* Write Protect */
		vuint32_t OPACR31_TP : 1; /* Trusted Protect */
	} B;
} AIPS_OPACR24_31_32B_tag;

typedef union { /* OPACR32_39 - Off-Platform Peripheral Access Control Registers
								 */
	vuint32_t R;
	struct {
		vuint32_t OPACR32_BW : 1; /* Buffer Writes */
		vuint32_t OPACR32_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR32_WP : 1; /* Write Protect */
		vuint32_t OPACR32_TP : 1; /* Trusted Protect */
		vuint32_t OPACR33_BW : 1; /* Buffer Writes */
		vuint32_t OPACR33_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR33_WP : 1; /* Write Protect */
		vuint32_t OPACR33_TP : 1; /* Trusted Protect */
		vuint32_t OPACR34_BW : 1; /* Buffer Writes */
		vuint32_t OPACR34_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR34_WP : 1; /* Write Protect */
		vuint32_t OPACR34_TP : 1; /* Trusted Protect */
		vuint32_t OPACR35_BW : 1; /* Buffer Writes */
		vuint32_t OPACR35_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR35_WP : 1; /* Write Protect */
		vuint32_t OPACR35_TP : 1; /* Trusted Protect */
		vuint32_t OPACR36_BW : 1; /* Buffer Writes */
		vuint32_t OPACR36_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR36_WP : 1; /* Write Protect */
		vuint32_t OPACR36_TP : 1; /* Trusted Protect */
		vuint32_t OPACR37_BW : 1; /* Buffer Writes */
		vuint32_t OPACR37_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR37_WP : 1; /* Write Protect */
		vuint32_t OPACR37_TP : 1; /* Trusted Protect */
		vuint32_t OPACR38_BW : 1; /* Buffer Writes */
		vuint32_t OPACR38_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR38_WP : 1; /* Write Protect */
		vuint32_t OPACR38_TP : 1; /* Trusted Protect */
		vuint32_t OPACR39_BW : 1; /* Buffer Writes */
		vuint32_t OPACR39_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR39_WP : 1; /* Write Protect */
		vuint32_t OPACR39_TP : 1; /* Trusted Protect */
	} B;
} AIPS_OPACR32_39_32B_tag;

typedef union { /* OPACR40_47 - Off-Platform Peripheral Access Control Registers
								 */
	vuint32_t R;
	struct {
		vuint32_t OPACR40_BW : 1; /* Buffer Writes */
		vuint32_t OPACR40_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR40_WP : 1; /* Write Protect */
		vuint32_t OPACR40_TP : 1; /* Trusted Protect */
		vuint32_t OPACR41_BW : 1; /* Buffer Writes */
		vuint32_t OPACR41_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR41_WP : 1; /* Write Protect */
		vuint32_t OPACR41_TP : 1; /* Trusted Protect */
		vuint32_t OPACR42_BW : 1; /* Buffer Writes */
		vuint32_t OPACR42_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR42_WP : 1; /* Write Protect */
		vuint32_t OPACR42_TP : 1; /* Trusted Protect */
		vuint32_t OPACR43_BW : 1; /* Buffer Writes */
		vuint32_t OPACR43_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR43_WP : 1; /* Write Protect */
		vuint32_t OPACR43_TP : 1; /* Trusted Protect */
		vuint32_t OPACR44_BW : 1; /* Buffer Writes */
		vuint32_t OPACR44_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR44_WP : 1; /* Write Protect */
		vuint32_t OPACR44_TP : 1; /* Trusted Protect */
		vuint32_t OPACR45_BW : 1; /* Buffer Writes */
		vuint32_t OPACR45_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR45_WP : 1; /* Write Protect */
		vuint32_t OPACR45_TP : 1; /* Trusted Protect */
		vuint32_t OPACR46_BW : 1; /* Buffer Writes */
		vuint32_t OPACR46_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR46_WP : 1; /* Write Protect */
		vuint32_t OPACR46_TP : 1; /* Trusted Protect */
		vuint32_t OPACR47_BW : 1; /* Buffer Writes */
		vuint32_t OPACR47_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR47_WP : 1; /* Write Protect */
		vuint32_t OPACR47_TP : 1; /* Trusted Protect */
	} B;
} AIPS_OPACR40_47_32B_tag;

typedef union { /* OPACR48_55 - Off-Platform Peripheral Access Control Registers
								 */
	vuint32_t R;
	struct {
		vuint32_t OPACR48_BW : 1; /* Buffer Writes */
		vuint32_t OPACR48_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR48_WP : 1; /* Write Protect */
		vuint32_t OPACR48_TP : 1; /* Trusted Protect */
		vuint32_t OPACR49_BW : 1; /* Buffer Writes */
		vuint32_t OPACR49_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR49_WP : 1; /* Write Protect */
		vuint32_t OPACR49_TP : 1; /* Trusted Protect */
		vuint32_t OPACR50_BW : 1; /* Buffer Writes */
		vuint32_t OPACR50_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR50_WP : 1; /* Write Protect */
		vuint32_t OPACR50_TP : 1; /* Trusted Protect */
		vuint32_t OPACR51_BW : 1; /* Buffer Writes */
		vuint32_t OPACR51_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR51_WP : 1; /* Write Protect */
		vuint32_t OPACR51_TP : 1; /* Trusted Protect */
		vuint32_t OPACR52_BW : 1; /* Buffer Writes */
		vuint32_t OPACR52_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR52_WP : 1; /* Write Protect */
		vuint32_t OPACR52_TP : 1; /* Trusted Protect */
		vuint32_t OPACR53_BW : 1; /* Buffer Writes */
		vuint32_t OPACR53_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR53_WP : 1; /* Write Protect */
		vuint32_t OPACR53_TP : 1; /* Trusted Protect */
		vuint32_t OPACR54_BW : 1; /* Buffer Writes */
		vuint32_t OPACR54_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR54_WP : 1; /* Write Protect */
		vuint32_t OPACR54_TP : 1; /* Trusted Protect */
		vuint32_t OPACR55_BW : 1; /* Buffer Writes */
		vuint32_t OPACR55_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR55_WP : 1; /* Write Protect */
		vuint32_t OPACR55_TP : 1; /* Trusted Protect */
	} B;
} AIPS_OPACR48_55_32B_tag;

typedef union { /* OPACR56_63 - Off-Platform Peripheral Access Control Registers
								 */
	vuint32_t R;
	struct {
		vuint32_t OPACR56_BW : 1; /* Buffer Writes */
		vuint32_t OPACR56_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR56_WP : 1; /* Write Protect */
		vuint32_t OPACR56_TP : 1; /* Trusted Protect */
		vuint32_t OPACR57_BW : 1; /* Buffer Writes */
		vuint32_t OPACR57_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR57_WP : 1; /* Write Protect */
		vuint32_t OPACR57_TP : 1; /* Trusted Protect */
		vuint32_t OPACR58_BW : 1; /* Buffer Writes */
		vuint32_t OPACR58_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR58_WP : 1; /* Write Protect */
		vuint32_t OPACR58_TP : 1; /* Trusted Protect */
		vuint32_t OPACR59_BW : 1; /* Buffer Writes */
		vuint32_t OPACR59_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR59_WP : 1; /* Write Protect */
		vuint32_t OPACR59_TP : 1; /* Trusted Protect */
		vuint32_t OPACR60_BW : 1; /* Buffer Writes */
		vuint32_t OPACR60_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR60_WP : 1; /* Write Protect */
		vuint32_t OPACR60_TP : 1; /* Trusted Protect */
		vuint32_t OPACR61_BW : 1; /* Buffer Writes */
		vuint32_t OPACR61_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR61_WP : 1; /* Write Protect */
		vuint32_t OPACR61_TP : 1; /* Trusted Protect */
		vuint32_t OPACR62_BW : 1; /* Buffer Writes */
		vuint32_t OPACR62_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR62_WP : 1; /* Write Protect */
		vuint32_t OPACR62_TP : 1; /* Trusted Protect */
		vuint32_t OPACR63_BW : 1; /* Buffer Writes */
		vuint32_t OPACR63_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR63_WP : 1; /* Write Protect */
		vuint32_t OPACR63_TP : 1; /* Trusted Protect */
	} B;
} AIPS_OPACR56_63_32B_tag;

typedef union { /* OPACR64_71 - Off-Platform Peripheral Access Control Registers
								 */
	vuint32_t R;
	struct {
		vuint32_t OPACR64_BW : 1; /* Buffer Writes */
		vuint32_t OPACR64_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR64_WP : 1; /* Write Protect */
		vuint32_t OPACR64_TP : 1; /* Trusted Protect */
		vuint32_t OPACR65_BW : 1; /* Buffer Writes */
		vuint32_t OPACR65_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR65_WP : 1; /* Write Protect */
		vuint32_t OPACR65_TP : 1; /* Trusted Protect */
		vuint32_t OPACR66_BW : 1; /* Buffer Writes */
		vuint32_t OPACR66_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR66_WP : 1; /* Write Protect */
		vuint32_t OPACR66_TP : 1; /* Trusted Protect */
		vuint32_t OPACR67_BW : 1; /* Buffer Writes */
		vuint32_t OPACR67_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR67_WP : 1; /* Write Protect */
		vuint32_t OPACR67_TP : 1; /* Trusted Protect */
		vuint32_t OPACR68_BW : 1; /* Buffer Writes */
		vuint32_t OPACR68_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR68_WP : 1; /* Write Protect */
		vuint32_t OPACR68_TP : 1; /* Trusted Protect */
		vuint32_t OPACR69_BW : 1; /* Buffer Writes */
		vuint32_t OPACR69_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR69_WP : 1; /* Write Protect */
		vuint32_t OPACR69_TP : 1; /* Trusted Protect */
		vuint32_t OPACR70_BW : 1; /* Buffer Writes */
		vuint32_t OPACR70_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR70_WP : 1; /* Write Protect */
		vuint32_t OPACR70_TP : 1; /* Trusted Protect */
		vuint32_t OPACR71_BW : 1; /* Buffer Writes */
		vuint32_t OPACR71_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR71_WP : 1; /* Write Protect */
		vuint32_t OPACR71_TP : 1; /* Trusted Protect */
	} B;
} AIPS_OPACR64_71_32B_tag;

typedef union { /* OPACR72_79 - Off-Platform Peripheral Access Control Registers
								 */
	vuint32_t R;
	struct {
		vuint32_t OPACR72_BW : 1; /* Buffer Writes */
		vuint32_t OPACR72_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR72_WP : 1; /* Write Protect */
		vuint32_t OPACR72_TP : 1; /* Trusted Protect */
		vuint32_t OPACR73_BW : 1; /* Buffer Writes */
		vuint32_t OPACR73_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR73_WP : 1; /* Write Protect */
		vuint32_t OPACR73_TP : 1; /* Trusted Protect */
		vuint32_t OPACR74_BW : 1; /* Buffer Writes */
		vuint32_t OPACR74_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR74_WP : 1; /* Write Protect */
		vuint32_t OPACR74_TP : 1; /* Trusted Protect */
		vuint32_t OPACR75_BW : 1; /* Buffer Writes */
		vuint32_t OPACR75_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR75_WP : 1; /* Write Protect */
		vuint32_t OPACR75_TP : 1; /* Trusted Protect */
		vuint32_t OPACR76_BW : 1; /* Buffer Writes */
		vuint32_t OPACR76_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR76_WP : 1; /* Write Protect */
		vuint32_t OPACR76_TP : 1; /* Trusted Protect */
		vuint32_t OPACR77_BW : 1; /* Buffer Writes */
		vuint32_t OPACR77_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR77_WP : 1; /* Write Protect */
		vuint32_t OPACR77_TP : 1; /* Trusted Protect */
		vuint32_t OPACR78_BW : 1; /* Buffer Writes */
		vuint32_t OPACR78_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR78_WP : 1; /* Write Protect */
		vuint32_t OPACR78_TP : 1; /* Trusted Protect */
		vuint32_t OPACR79_BW : 1; /* Buffer Writes */
		vuint32_t OPACR79_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR79_WP : 1; /* Write Protect */
		vuint32_t OPACR79_TP : 1; /* Trusted Protect */
	} B;
} AIPS_OPACR72_79_32B_tag;

typedef union { /* OPACR80_87 - Off-Platform Peripheral Access Control Registers
								 */
	vuint32_t R;
	struct {
		vuint32_t OPACR80_BW : 1; /* Buffer Writes */
		vuint32_t OPACR80_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR80_WP : 1; /* Write Protect */
		vuint32_t OPACR80_TP : 1; /* Trusted Protect */
		vuint32_t OPACR81_BW : 1; /* Buffer Writes */
		vuint32_t OPACR81_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR81_WP : 1; /* Write Protect */
		vuint32_t OPACR81_TP : 1; /* Trusted Protect */
		vuint32_t OPACR82_BW : 1; /* Buffer Writes */
		vuint32_t OPACR82_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR82_WP : 1; /* Write Protect */
		vuint32_t OPACR82_TP : 1; /* Trusted Protect */
		vuint32_t OPACR83_BW : 1; /* Buffer Writes */
		vuint32_t OPACR83_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR83_WP : 1; /* Write Protect */
		vuint32_t OPACR83_TP : 1; /* Trusted Protect */
		vuint32_t OPACR84_BW : 1; /* Buffer Writes */
		vuint32_t OPACR84_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR84_WP : 1; /* Write Protect */
		vuint32_t OPACR84_TP : 1; /* Trusted Protect */
		vuint32_t OPACR85_BW : 1; /* Buffer Writes */
		vuint32_t OPACR85_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR85_WP : 1; /* Write Protect */
		vuint32_t OPACR85_TP : 1; /* Trusted Protect */
		vuint32_t OPACR86_BW : 1; /* Buffer Writes */
		vuint32_t OPACR86_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR86_WP : 1; /* Write Protect */
		vuint32_t OPACR86_TP : 1; /* Trusted Protect */
		vuint32_t OPACR87_BW : 1; /* Buffer Writes */
		vuint32_t OPACR87_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR87_WP : 1; /* Write Protect */
		vuint32_t OPACR87_TP : 1; /* Trusted Protect */
	} B;
} AIPS_OPACR80_87_32B_tag;

typedef union { /* OPACR88_95 - Off-Platform Peripheral Access Control Registers
								 */
	vuint32_t R;
	struct {
		vuint32_t OPACR88_BW : 1; /* Buffer Writes */
		vuint32_t OPACR88_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR88_WP : 1; /* Write Protect */
		vuint32_t OPACR88_TP : 1; /* Trusted Protect */
		vuint32_t OPACR89_BW : 1; /* Buffer Writes */
		vuint32_t OPACR89_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR89_WP : 1; /* Write Protect */
		vuint32_t OPACR89_TP : 1; /* Trusted Protect */
		vuint32_t OPACR90_BW : 1; /* Buffer Writes */
		vuint32_t OPACR90_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR90_WP : 1; /* Write Protect */
		vuint32_t OPACR90_TP : 1; /* Trusted Protect */
		vuint32_t OPACR91_BW : 1; /* Buffer Writes */
		vuint32_t OPACR91_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR91_WP : 1; /* Write Protect */
		vuint32_t OPACR91_TP : 1; /* Trusted Protect */
		vuint32_t OPACR92_BW : 1; /* Buffer Writes */
		vuint32_t OPACR92_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR92_WP : 1; /* Write Protect */
		vuint32_t OPACR92_TP : 1; /* Trusted Protect */
		vuint32_t OPACR93_BW : 1; /* Buffer Writes */
		vuint32_t OPACR93_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR93_WP : 1; /* Write Protect */
		vuint32_t OPACR93_TP : 1; /* Trusted Protect */
		vuint32_t OPACR94_BW : 1; /* Buffer Writes */
		vuint32_t OPACR94_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR94_WP : 1; /* Write Protect */
		vuint32_t OPACR94_TP : 1; /* Trusted Protect */
		vuint32_t OPACR95_BW : 1; /* Buffer Writes */
		vuint32_t OPACR95_SP : 1; /* Supervisor Protect */
		vuint32_t OPACR95_WP : 1; /* Write Protect */
		vuint32_t OPACR95_TP : 1; /* Trusted Protect */
	} B;
} AIPS_OPACR88_95_32B_tag;

typedef struct AIPS_struct_tag { /* start of AIPS_tag */
																 /* MPROT - Master Privilege Registers */
	AIPS_MPROT_32B_tag MPROT;			 /* offset: 0x0000 size: 32 bit */
	int8_t AIPS_reserved_0004[28];
	/* PACR0_7 - Peripheral Access Control Registers */
	AIPS_PACR0_7_32B_tag PACR0_7; /* offset: 0x0020 size: 32 bit */
	/* PACR8_15 - Peripheral Access Control Registers */
	AIPS_PACR8_15_32B_tag PACR8_15; /* offset: 0x0024 size: 32 bit */
	/* PACR16_23 - Peripheral Access Control Registers */
	AIPS_PACR16_23_32B_tag PACR16_23; /* offset: 0x0028 size: 32 bit */
	/* PACR24_31 - Peripheral Access Control Registers */
	AIPS_PACR24_31_32B_tag PACR24_31; /* offset: 0x002C size: 32 bit */
	int8_t AIPS_reserved_0030[16];
	/* OPACR0_7 - Off-Platform Peripheral Access Control Registers */
	AIPS_OPACR0_7_32B_tag OPACR0_7; /* offset: 0x0040 size: 32 bit */
	/* OPACR8_15 - Off-Platform Peripheral Access Control Registers */
	AIPS_OPACR8_15_32B_tag OPACR8_15; /* offset: 0x0044 size: 32 bit */
	/* OPACR16_23 - Off-Platform Peripheral Access Control Registers */
	AIPS_OPACR16_23_32B_tag OPACR16_23; /* offset: 0x0048 size: 32 bit */
	/* OPACR24_31 - Off-Platform Peripheral Access Control Registers */
	AIPS_OPACR24_31_32B_tag OPACR24_31; /* offset: 0x004C size: 32 bit */
	/* OPACR32_39 - Off-Platform Peripheral Access Control Registers */
	AIPS_OPACR32_39_32B_tag OPACR32_39; /* offset: 0x0050 size: 32 bit */
	/* OPACR40_47 - Off-Platform Peripheral Access Control Registers */
	AIPS_OPACR40_47_32B_tag OPACR40_47; /* offset: 0x0054 size: 32 bit */
	/* OPACR48_55 - Off-Platform Peripheral Access Control Registers */
	AIPS_OPACR48_55_32B_tag OPACR48_55; /* offset: 0x0058 size: 32 bit */
	/* OPACR56_63 - Off-Platform Peripheral Access Control Registers */
	AIPS_OPACR56_63_32B_tag OPACR56_63; /* offset: 0x005C size: 32 bit */
	/* OPACR64_71 - Off-Platform Peripheral Access Control Registers */
	AIPS_OPACR64_71_32B_tag OPACR64_71; /* offset: 0x0060 size: 32 bit */
	/* OPACR72_79 - Off-Platform Peripheral Access Control Registers */
	AIPS_OPACR72_79_32B_tag OPACR72_79; /* offset: 0x0064 size: 32 bit */
	/* OPACR80_87 - Off-Platform Peripheral Access Control Registers */
	AIPS_OPACR80_87_32B_tag OPACR80_87; /* offset: 0x0068 size: 32 bit */
	/* OPACR88_95 - Off-Platform Peripheral Access Control Registers */
	AIPS_OPACR88_95_32B_tag OPACR88_95; /* offset: 0x006C size: 32 bit */
} AIPS_tag;

#define AIPS (*(volatile AIPS_tag*)0xFFF00000UL)

/****************************************************************/
/*                                                              */
/* Module: MAX  */
/*                                                              */
/****************************************************************/

/* Register layout for all registers MPR... */

typedef union { /* Master Priority Register for slave port n */
	vuint32_t R;
	struct {
		vuint32_t : 1;
		vuint32_t MSTR_7 : 3; /* Master 7 Priority */
		vuint32_t : 1;
		vuint32_t MSTR_6 : 3; /* Master 6 Priority */
		vuint32_t : 1;
		vuint32_t MSTR_5 : 3; /* Master 5 Priority */
		vuint32_t : 1;
		vuint32_t MSTR_4 : 3; /* Master 4 Priority */
		vuint32_t : 1;
		vuint32_t MSTR_3 : 3; /* Master 3 Priority */
		vuint32_t : 1;
		vuint32_t MSTR_2 : 3; /* Master 2 Priority */
		vuint32_t : 1;
		vuint32_t MSTR_1 : 3; /* Master 1 Priority */
		vuint32_t : 1;
		vuint32_t MSTR_0 : 3; /* Master 0 Priority */
	} B;
} MAX_MPR_32B_tag;

/* Register layout for all registers AMPR matches xxx */

/* Register layout for all registers SGPCR... */

typedef union { /* MAX_SGPCRn - MAX General Purpose Control Register for Slave
									 Port n */
	vuint32_t R;
	struct {
		vuint32_t RO : 1;	 /* Read Only */
		vuint32_t HLP : 1; /* Halt Low Priority */
		vuint32_t : 6;
		vuint32_t HPE7 : 1; /* High Priority Enable */
		vuint32_t HPE6 : 1; /* High Priority Enable */
		vuint32_t HPE5 : 1; /* High Priority Enable */
		vuint32_t HPE4 : 1; /* High Priority Enable */
		vuint32_t HPE3 : 1; /* High Priority Enable */
		vuint32_t HPE2 : 1; /* High Priority Enable */
		vuint32_t HPE1 : 1; /* High Priority Enable */
		vuint32_t HPE0 : 1; /* High Priority Enable */
		vuint32_t : 6;
		vuint32_t ARB : 2; /* Arbitration Mode */
		vuint32_t : 2;
		vuint32_t PCTL : 2; /* Parking Control */
		vuint32_t : 1;
		vuint32_t PARK : 3; /* Park */
	} B;
} MAX_SGPCR_32B_tag;

/* Register layout for all registers ASGPCR... */

typedef union { /* MAX_ASGPCRn - MAX Alternate General Purpose Control Register
									 n */
	vuint32_t R;
	struct {
		vuint32_t : 1;
		vuint32_t HLP : 1; /* Halt Low Priority */
		vuint32_t : 6;
		vuint32_t HPE7 : 1; /* High Priority Enable */
		vuint32_t HPE6 : 1; /* High Priority Enable */
		vuint32_t HPE5 : 1; /* High Priority Enable */
		vuint32_t HPE4 : 1; /* High Priority Enable */
		vuint32_t HPE3 : 1; /* High Priority Enable */
		vuint32_t HPE2 : 1; /* High Priority Enable */
		vuint32_t HPE1 : 1; /* High Priority Enable */
		vuint32_t HPE0 : 1; /* High Priority Enable */
		vuint32_t : 6;
		vuint32_t ARB : 2; /* Arbitration Mode */
		vuint32_t : 2;
		vuint32_t PCTL : 2; /* Parking Control */
		vuint32_t : 1;
		vuint32_t PARK : 3; /* Park */
	} B;
} MAX_ASGPCR_32B_tag;

/* Register layout for all registers MGPCR... */

typedef union { /* MAX_MGPCRn - Master General Purpose Control Register n */
	vuint32_t R;
	struct {
		vuint32_t : 29;
		vuint32_t AULB : 3; /* Arbitrate on Undefined Length Bursts */
	} B;
} MAX_MGPCR_32B_tag;

typedef struct MAX_SLAVE_PORT_struct_tag {
	/* Master Priority Register for slave port n */
	MAX_MPR_32B_tag MPR; /* relative offset: 0x0000 */
											 /* Alternate Master Priority Register for slave port n */
	MAX_MPR_32B_tag AMPR; /* relative offset: 0x0004 */
	int8_t MAX_SLAVE_PORT_reserved_0008[8];
	/* MAX_SGPCRn - MAX General Purpose Control Register for Slave Port n */
	MAX_SGPCR_32B_tag SGPCR; /* relative offset: 0x0010 */
	/* MAX_ASGPCRn - MAX Alternate General Purpose Control Register n */
	MAX_ASGPCR_32B_tag ASGPCR; /* relative offset: 0x0014 */
	int8_t MAX_SLAVE_PORT_reserved_0018[232];

} MAX_SLAVE_PORT_tag;

typedef struct MAX_MASTER_PORT_struct_tag {
	/* MAX_MGPCRn - Master General Purpose Control Register n */
	MAX_MGPCR_32B_tag MGPCR; /* relative offset: 0x0000 */
	int8_t MAX_MASTER_PORT_reserved_0004[252];

} MAX_MASTER_PORT_tag;

typedef struct MAX_struct_tag { /* start of MAX_tag */
	union {
		/*  Register set SLAVE_PORT */
		MAX_SLAVE_PORT_tag SLAVE_PORT[8]; /* offset: 0x0000  (0x0100 x 8) */

		struct {
			/* Master Priority Register for slave port n */
			MAX_MPR_32B_tag MPR0; /* offset: 0x0000 size: 32 bit */
			/* Alternate Master Priority Register for slave port n */
			MAX_MPR_32B_tag AMPR0; /* offset: 0x0004 size: 32 bit */
			int8_t MAX_reserved_0008_I1[8];
			/* MAX_SGPCRn - MAX General Purpose Control Register for Slave Port n */
			MAX_SGPCR_32B_tag SGPCR0; /* offset: 0x0010 size: 32 bit */
			/* MAX_ASGPCRn - MAX Alternate General Purpose Control Register n */
			MAX_ASGPCR_32B_tag ASGPCR0; /* offset: 0x0014 size: 32 bit */
			int8_t MAX_reserved_0018_I1[232];
			/* Master Priority Register for slave port n */
			MAX_MPR_32B_tag MPR1; /* offset: 0x0100 size: 32 bit */
			/* Alternate Master Priority Register for slave port n */
			MAX_MPR_32B_tag AMPR1; /* offset: 0x0104 size: 32 bit */
			int8_t MAX_reserved_0108_I1[8];
			/* MAX_SGPCRn - MAX General Purpose Control Register for Slave Port n */
			MAX_SGPCR_32B_tag SGPCR1; /* offset: 0x0110 size: 32 bit */
			/* MAX_ASGPCRn - MAX Alternate General Purpose Control Register n */
			MAX_ASGPCR_32B_tag ASGPCR1; /* offset: 0x0114 size: 32 bit */
			int8_t MAX_reserved_0118_I1[232];
			/* Master Priority Register for slave port n */
			MAX_MPR_32B_tag MPR2; /* offset: 0x0200 size: 32 bit */
			/* Alternate Master Priority Register for slave port n */
			MAX_MPR_32B_tag AMPR2; /* offset: 0x0204 size: 32 bit */
			int8_t MAX_reserved_0208_I1[8];
			/* MAX_SGPCRn - MAX General Purpose Control Register for Slave Port n */
			MAX_SGPCR_32B_tag SGPCR2; /* offset: 0x0210 size: 32 bit */
			/* MAX_ASGPCRn - MAX Alternate General Purpose Control Register n */
			MAX_ASGPCR_32B_tag ASGPCR2; /* offset: 0x0214 size: 32 bit */
			int8_t MAX_reserved_0218_I1[232];
			/* Master Priority Register for slave port n */
			MAX_MPR_32B_tag MPR3; /* offset: 0x0300 size: 32 bit */
			/* Alternate Master Priority Register for slave port n */
			MAX_MPR_32B_tag AMPR3; /* offset: 0x0304 size: 32 bit */
			int8_t MAX_reserved_0308_I1[8];
			/* MAX_SGPCRn - MAX General Purpose Control Register for Slave Port n */
			MAX_SGPCR_32B_tag SGPCR3; /* offset: 0x0310 size: 32 bit */
			/* MAX_ASGPCRn - MAX Alternate General Purpose Control Register n */
			MAX_ASGPCR_32B_tag ASGPCR3; /* offset: 0x0314 size: 32 bit */
			int8_t MAX_reserved_0318_I1[232];
			/* Master Priority Register for slave port n */
			MAX_MPR_32B_tag MPR4; /* offset: 0x0400 size: 32 bit */
			/* Alternate Master Priority Register for slave port n */
			MAX_MPR_32B_tag AMPR4; /* offset: 0x0404 size: 32 bit */
			int8_t MAX_reserved_0408_I1[8];
			/* MAX_SGPCRn - MAX General Purpose Control Register for Slave Port n */
			MAX_SGPCR_32B_tag SGPCR4; /* offset: 0x0410 size: 32 bit */
			/* MAX_ASGPCRn - MAX Alternate General Purpose Control Register n */
			MAX_ASGPCR_32B_tag ASGPCR4; /* offset: 0x0414 size: 32 bit */
			int8_t MAX_reserved_0418_I1[232];
			/* Master Priority Register for slave port n */
			MAX_MPR_32B_tag MPR5; /* offset: 0x0500 size: 32 bit */
			/* Alternate Master Priority Register for slave port n */
			MAX_MPR_32B_tag AMPR5; /* offset: 0x0504 size: 32 bit */
			int8_t MAX_reserved_0508_I1[8];
			/* MAX_SGPCRn - MAX General Purpose Control Register for Slave Port n */
			MAX_SGPCR_32B_tag SGPCR5; /* offset: 0x0510 size: 32 bit */
			/* MAX_ASGPCRn - MAX Alternate General Purpose Control Register n */
			MAX_ASGPCR_32B_tag ASGPCR5; /* offset: 0x0514 size: 32 bit */
			int8_t MAX_reserved_0518_I1[232];
			/* Master Priority Register for slave port n */
			MAX_MPR_32B_tag MPR6; /* offset: 0x0600 size: 32 bit */
			/* Alternate Master Priority Register for slave port n */
			MAX_MPR_32B_tag AMPR6; /* offset: 0x0604 size: 32 bit */
			int8_t MAX_reserved_0608_I1[8];
			/* MAX_SGPCRn - MAX General Purpose Control Register for Slave Port n */
			MAX_SGPCR_32B_tag SGPCR6; /* offset: 0x0610 size: 32 bit */
			/* MAX_ASGPCRn - MAX Alternate General Purpose Control Register n */
			MAX_ASGPCR_32B_tag ASGPCR6; /* offset: 0x0614 size: 32 bit */
			int8_t MAX_reserved_0618_I1[232];
			/* Master Priority Register for slave port n */
			MAX_MPR_32B_tag MPR7; /* offset: 0x0700 size: 32 bit */
			/* Alternate Master Priority Register for slave port n */
			MAX_MPR_32B_tag AMPR7; /* offset: 0x0704 size: 32 bit */
			int8_t MAX_reserved_0708_I1[8];
			/* MAX_SGPCRn - MAX General Purpose Control Register for Slave Port n */
			MAX_SGPCR_32B_tag SGPCR7; /* offset: 0x0710 size: 32 bit */
			/* MAX_ASGPCRn - MAX Alternate General Purpose Control Register n */
			MAX_ASGPCR_32B_tag ASGPCR7; /* offset: 0x0714 size: 32 bit */
			int8_t MAX_reserved_0718_E1[232];
		};
	};
	union {
		/*  Register set MASTER_PORT */
		MAX_MASTER_PORT_tag MASTER_PORT[8]; /* offset: 0x0800  (0x0100 x 8) */

		struct {
			/* MAX_MGPCRn - Master General Purpose Control Register n */
			MAX_MGPCR_32B_tag MGPCR0; /* offset: 0x0800 size: 32 bit */
			int8_t MAX_reserved_0804_I1[252];
			MAX_MGPCR_32B_tag MGPCR1; /* offset: 0x0900 size: 32 bit */
			int8_t MAX_reserved_0904_I1[252];
			MAX_MGPCR_32B_tag MGPCR2; /* offset: 0x0A00 size: 32 bit */
			int8_t MAX_reserved_0A04_I1[252];
			MAX_MGPCR_32B_tag MGPCR3; /* offset: 0x0B00 size: 32 bit */
			int8_t MAX_reserved_0B04_I1[252];
			MAX_MGPCR_32B_tag MGPCR4; /* offset: 0x0C00 size: 32 bit */
			int8_t MAX_reserved_0C04_I1[252];
			MAX_MGPCR_32B_tag MGPCR5; /* offset: 0x0D00 size: 32 bit */
			int8_t MAX_reserved_0D04_I1[252];
			MAX_MGPCR_32B_tag MGPCR6; /* offset: 0x0E00 size: 32 bit */
			int8_t MAX_reserved_0E04_I1[252];
			MAX_MGPCR_32B_tag MGPCR7; /* offset: 0x0F00 size: 32 bit */
			int8_t MAX_reserved_0F04_E1[252];
		};
	};
} MAX_tag;

#define MAX (*(volatile MAX_tag*)0xFFF04000UL)

/****************************************************************/
/*                                                              */
/* Module: MPU  */
/*                                                              */
/****************************************************************/

typedef union { /* MPU_CESR - MPU Control/Error Status Register */
	vuint32_t R;
	struct {
		vuint32_t SPERR : 8; /* Slave Port n Error */
		vuint32_t : 4;
		vuint32_t HRL : 4;	/* Hardware Revision Level */
		vuint32_t NSP : 4;	/* Number of Slave Ports */
		vuint32_t NRGD : 4; /* Number of Region Descriptors */
		vuint32_t : 7;
		vuint32_t VLD : 1; /* Valid bit */
	} B;
} MPU_CESR_32B_tag;

/* Register layout for all registers EAR... */

typedef union { /* MPU_EARn - MPU Error Address Register, Slave Port n */
	vuint32_t R;
	struct {
		vuint32_t EADDR : 32; /* Error Address */
	} B;
} MPU_EAR_32B_tag;

/* Register layout for all registers EDR... */

typedef union { /* MPU_EDRn - MPU Error Detail Register, Slave Port n */
	vuint32_t R;
	struct {
		vuint32_t EACD : 16; /* Error Access Control Detail */
		vuint32_t EPID : 8;	 /* Error Process Identification */
		vuint32_t EMN : 4;	 /* Error Master Number */
		vuint32_t EATTR : 3; /* Error Attributes */
		vuint32_t ERW : 1;	 /* Error Read/Write */
	} B;
} MPU_EDR_32B_tag;

/* Register layout for all registers RGD_WORD0... */

typedef union { /* MPU_RGDn_Word0 - MPU Region Descriptor */
	vuint32_t R;
	struct {
		vuint32_t SRTADDR : 27; /* Start Address */
		vuint32_t : 5;
	} B;
} MPU_RGD_WORD0_32B_tag;

/* Register layout for all registers RGD_WORD1... */

typedef union { /* MPU_RGDn_Word1 - MPU Region Descriptor */
	vuint32_t R;
	struct {
		vuint32_t ENDADDR : 27; /* End Address */
		vuint32_t : 5;
	} B;
} MPU_RGD_WORD1_32B_tag;

/* Register layout for all registers RGD_WORD2... */

typedef union { /* MPU_RGDn_Word2 - MPU Region Descriptor */
	vuint32_t R;
	struct {
		vuint32_t M7RE : 1; /* Bus Master 7 Read Enable */
		vuint32_t M7WE : 1; /* Bus Master 7 Write Enable */
		vuint32_t M6RE : 1; /* Bus Master 6 Read Enable */
		vuint32_t M6WE : 1; /* Bus Master 7 Write Enable */
		vuint32_t M5RE : 1; /* Bus Master 5 Read Enable */
		vuint32_t M5WE : 1; /* Bus Master 5 Write Enable */
		vuint32_t M4RE : 1; /* Bus Master 4 Read Enable */
		vuint32_t M4WE : 1; /* Bus Master 4 Write Enable */
		vuint32_t M3PE : 1; /* Bus Master 3 Process Identifier Enable */
		vuint32_t M3SM : 2; /* Bus Master 3 Supervisor Mode Access Control */
		vuint32_t M3UM : 3; /* Bus Master 3 User Mode Access Control */
		vuint32_t M2PE : 1; /* Bus Master 2 Process Identifier Enable */
		vuint32_t M2SM : 2; /* Bus Master 2 Supervisor Mode Access Control */
		vuint32_t M2UM : 3; /* Bus Master 2 User Mode Access Control */
		vuint32_t M1PE : 1; /* Bus Master 1 Process Identifier Enable */
		vuint32_t M1SM : 2; /* Bus Master 1 Supervisor Mode Access Control */
		vuint32_t M1UM : 3; /* Bus Master 1 User Mode Access Control */
		vuint32_t M0PE : 1; /* Bus Master 0 Process Identifier Enable */
		vuint32_t M0SM : 2; /* Bus Master 0 Supervisor Mode Access Control */
		vuint32_t M0UM : 3; /* Bus Master 0 User Mode Access Control */
	} B;
} MPU_RGD_WORD2_32B_tag;

/* Register layout for all registers RGD_WORD3... */

typedef union { /* MPU_RGDn_Word3 - MPU Region Descriptor */
	vuint32_t R;
	struct {
		vuint32_t PID : 8;		 /* Process Identifier */
		vuint32_t PIDMASK : 8; /* Process Identifier Mask */
		vuint32_t : 15;
		vuint32_t VLD : 1; /* Valid */
	} B;
} MPU_RGD_WORD3_32B_tag;

/* Register layout for all registers RGDAAC... */

typedef union { /* MPU_RGDAACn -  MPU Region Descriptor Alternate Access Control
								 */
	vuint32_t R;
	struct {
		vuint32_t M7RE : 1; /* Bus Master 7 Read Enable */
		vuint32_t M7WE : 1; /* Bus Master 7 Write Enable */
		vuint32_t M6RE : 1; /* Bus Master 6 Read Enable */
		vuint32_t M6WE : 1; /* Bus Master 7 Write Enable */
		vuint32_t M5RE : 1; /* Bus Master 5 Read Enable */
		vuint32_t M5WE : 1; /* Bus Master 5 Write Enable */
		vuint32_t M4RE : 1; /* Bus Master 4 Read Enable */
		vuint32_t M4WE : 1; /* Bus Master 4 Write Enable */
		vuint32_t M3PE : 1; /* Bus Master 3 Process Identifier Enable */
		vuint32_t M3SM : 2; /* Bus Master 3 Supervisor Mode Access Control */
		vuint32_t M3UM : 3; /* Bus Master 3 User Mode Access Control */
		vuint32_t M2PE : 1; /* Bus Master 2 Process Identifier Enable */
		vuint32_t M2SM : 2; /* Bus Master 2 Supervisor Mode Access Control */
		vuint32_t M2UM : 3; /* Bus Master 2 User Mode Access Control */
		vuint32_t M1PE : 1; /* Bus Master 1 Process Identifier Enable */
		vuint32_t M1SM : 2; /* Bus Master 1 Supervisor Mode Access Control */
		vuint32_t M1UM : 3; /* Bus Master 1 User Mode Access Control */
		vuint32_t M0PE : 1; /* Bus Master 0 Process Identifier Enable */
		vuint32_t M0SM : 2; /* Bus Master 0 Supervisor Mode Access Control */
		vuint32_t M0UM : 3; /* Bus Master 0 User Mode Access Control */
	} B;
} MPU_RGDAAC_32B_tag;

typedef struct MPU_SLAVE_PORT_struct_tag {
	/* MPU_EARn - MPU Error Address Register, Slave Port n */
	MPU_EAR_32B_tag EAR; /* relative offset: 0x0000 */
											 /* MPU_EDRn - MPU Error Detail Register, Slave Port n */
	MPU_EDR_32B_tag EDR; /* relative offset: 0x0004 */

} MPU_SLAVE_PORT_tag;

typedef struct MPU_REGION_struct_tag {
	/* MPU_RGDn_Word0 - MPU Region Descriptor */
	MPU_RGD_WORD0_32B_tag RGD_WORD0; /* relative offset: 0x0000 */
																	 /* MPU_RGDn_Word1 - MPU Region Descriptor */
	MPU_RGD_WORD1_32B_tag RGD_WORD1; /* relative offset: 0x0004 */
																	 /* MPU_RGDn_Word2 - MPU Region Descriptor */
	MPU_RGD_WORD2_32B_tag RGD_WORD2; /* relative offset: 0x0008 */
																	 /* MPU_RGDn_Word3 - MPU Region Descriptor */
	MPU_RGD_WORD3_32B_tag RGD_WORD3; /* relative offset: 0x000C */

} MPU_REGION_tag;

typedef struct MPU_struct_tag { /* start of MPU_tag */
	/* MPU_CESR - MPU Control/Error Status Register */
	MPU_CESR_32B_tag CESR; /* offset: 0x0000 size: 32 bit */
	int8_t MPU_reserved_0004_C[12];
	union {
		/*  Register set SLAVE_PORT */
		MPU_SLAVE_PORT_tag SLAVE_PORT[4]; /* offset: 0x0010  (0x0008 x 4) */

		struct {
			/* MPU_EARn - MPU Error Address Register, Slave Port n */
			MPU_EAR_32B_tag EAR0; /* offset: 0x0010 size: 32 bit */
			/* MPU_EDRn - MPU Error Detail Register, Slave Port n */
			MPU_EDR_32B_tag EDR0; /* offset: 0x0014 size: 32 bit */
			/* MPU_EARn - MPU Error Address Register, Slave Port n */
			MPU_EAR_32B_tag EAR1; /* offset: 0x0018 size: 32 bit */
			/* MPU_EDRn - MPU Error Detail Register, Slave Port n */
			MPU_EDR_32B_tag EDR1; /* offset: 0x001C size: 32 bit */
			/* MPU_EARn - MPU Error Address Register, Slave Port n */
			MPU_EAR_32B_tag EAR2; /* offset: 0x0020 size: 32 bit */
			/* MPU_EDRn - MPU Error Detail Register, Slave Port n */
			MPU_EDR_32B_tag EDR2; /* offset: 0x0024 size: 32 bit */
			/* MPU_EARn - MPU Error Address Register, Slave Port n */
			MPU_EAR_32B_tag EAR3; /* offset: 0x0028 size: 32 bit */
			/* MPU_EDRn - MPU Error Detail Register, Slave Port n */
			MPU_EDR_32B_tag EDR3; /* offset: 0x002C size: 32 bit */
		};
	};
	int8_t MPU_reserved_0030_C[976];
	union {
		/*  Register set REGION */
		MPU_REGION_tag REGION[16]; /* offset: 0x0400  (0x0010 x 16) */

		struct {
			/* MPU_RGDn_Word0 - MPU Region Descriptor */
			MPU_RGD_WORD0_32B_tag RGD0_WORD0; /* offset: 0x0400 size: 32 bit */
			/* MPU_RGDn_Word1 - MPU Region Descriptor */
			MPU_RGD_WORD1_32B_tag RGD0_WORD1; /* offset: 0x0404 size: 32 bit */
			/* MPU_RGDn_Word2 - MPU Region Descriptor */
			MPU_RGD_WORD2_32B_tag RGD0_WORD2; /* offset: 0x0408 size: 32 bit */
			/* MPU_RGDn_Word3 - MPU Region Descriptor */
			MPU_RGD_WORD3_32B_tag RGD0_WORD3; /* offset: 0x040C size: 32 bit */
			/* MPU_RGDn_Word0 - MPU Region Descriptor */
			MPU_RGD_WORD0_32B_tag RGD1_WORD0; /* offset: 0x0410 size: 32 bit */
			/* MPU_RGDn_Word1 - MPU Region Descriptor */
			MPU_RGD_WORD1_32B_tag RGD1_WORD1; /* offset: 0x0414 size: 32 bit */
			/* MPU_RGDn_Word2 - MPU Region Descriptor */
			MPU_RGD_WORD2_32B_tag RGD1_WORD2; /* offset: 0x0418 size: 32 bit */
			/* MPU_RGDn_Word3 - MPU Region Descriptor */
			MPU_RGD_WORD3_32B_tag RGD1_WORD3; /* offset: 0x041C size: 32 bit */
			/* MPU_RGDn_Word0 - MPU Region Descriptor */
			MPU_RGD_WORD0_32B_tag RGD2_WORD0; /* offset: 0x0420 size: 32 bit */
			/* MPU_RGDn_Word1 - MPU Region Descriptor */
			MPU_RGD_WORD1_32B_tag RGD2_WORD1; /* offset: 0x0424 size: 32 bit */
			/* MPU_RGDn_Word2 - MPU Region Descriptor */
			MPU_RGD_WORD2_32B_tag RGD2_WORD2; /* offset: 0x0428 size: 32 bit */
			/* MPU_RGDn_Word3 - MPU Region Descriptor */
			MPU_RGD_WORD3_32B_tag RGD2_WORD3; /* offset: 0x042C size: 32 bit */
			/* MPU_RGDn_Word0 - MPU Region Descriptor */
			MPU_RGD_WORD0_32B_tag RGD3_WORD0; /* offset: 0x0430 size: 32 bit */
			/* MPU_RGDn_Word1 - MPU Region Descriptor */
			MPU_RGD_WORD1_32B_tag RGD3_WORD1; /* offset: 0x0434 size: 32 bit */
			/* MPU_RGDn_Word2 - MPU Region Descriptor */
			MPU_RGD_WORD2_32B_tag RGD3_WORD2; /* offset: 0x0438 size: 32 bit */
			/* MPU_RGDn_Word3 - MPU Region Descriptor */
			MPU_RGD_WORD3_32B_tag RGD3_WORD3; /* offset: 0x043C size: 32 bit */
			/* MPU_RGDn_Word0 - MPU Region Descriptor */
			MPU_RGD_WORD0_32B_tag RGD4_WORD0; /* offset: 0x0440 size: 32 bit */
			/* MPU_RGDn_Word1 - MPU Region Descriptor */
			MPU_RGD_WORD1_32B_tag RGD4_WORD1; /* offset: 0x0444 size: 32 bit */
			/* MPU_RGDn_Word2 - MPU Region Descriptor */
			MPU_RGD_WORD2_32B_tag RGD4_WORD2; /* offset: 0x0448 size: 32 bit */
			/* MPU_RGDn_Word3 - MPU Region Descriptor */
			MPU_RGD_WORD3_32B_tag RGD4_WORD3; /* offset: 0x044C size: 32 bit */
			/* MPU_RGDn_Word0 - MPU Region Descriptor */
			MPU_RGD_WORD0_32B_tag RGD5_WORD0; /* offset: 0x0450 size: 32 bit */
			/* MPU_RGDn_Word1 - MPU Region Descriptor */
			MPU_RGD_WORD1_32B_tag RGD5_WORD1; /* offset: 0x0454 size: 32 bit */
			/* MPU_RGDn_Word2 - MPU Region Descriptor */
			MPU_RGD_WORD2_32B_tag RGD5_WORD2; /* offset: 0x0458 size: 32 bit */
			/* MPU_RGDn_Word3 - MPU Region Descriptor */
			MPU_RGD_WORD3_32B_tag RGD5_WORD3; /* offset: 0x045C size: 32 bit */
			/* MPU_RGDn_Word0 - MPU Region Descriptor */
			MPU_RGD_WORD0_32B_tag RGD6_WORD0; /* offset: 0x0460 size: 32 bit */
			/* MPU_RGDn_Word1 - MPU Region Descriptor */
			MPU_RGD_WORD1_32B_tag RGD6_WORD1; /* offset: 0x0464 size: 32 bit */
			/* MPU_RGDn_Word2 - MPU Region Descriptor */
			MPU_RGD_WORD2_32B_tag RGD6_WORD2; /* offset: 0x0468 size: 32 bit */
			/* MPU_RGDn_Word3 - MPU Region Descriptor */
			MPU_RGD_WORD3_32B_tag RGD6_WORD3; /* offset: 0x046C size: 32 bit */
			/* MPU_RGDn_Word0 - MPU Region Descriptor */
			MPU_RGD_WORD0_32B_tag RGD7_WORD0; /* offset: 0x0470 size: 32 bit */
			/* MPU_RGDn_Word1 - MPU Region Descriptor */
			MPU_RGD_WORD1_32B_tag RGD7_WORD1; /* offset: 0x0474 size: 32 bit */
			/* MPU_RGDn_Word2 - MPU Region Descriptor */
			MPU_RGD_WORD2_32B_tag RGD7_WORD2; /* offset: 0x0478 size: 32 bit */
			/* MPU_RGDn_Word3 - MPU Region Descriptor */
			MPU_RGD_WORD3_32B_tag RGD7_WORD3; /* offset: 0x047C size: 32 bit */
			/* MPU_RGDn_Word0 - MPU Region Descriptor */
			MPU_RGD_WORD0_32B_tag RGD8_WORD0; /* offset: 0x0480 size: 32 bit */
			/* MPU_RGDn_Word1 - MPU Region Descriptor */
			MPU_RGD_WORD1_32B_tag RGD8_WORD1; /* offset: 0x0484 size: 32 bit */
			/* MPU_RGDn_Word2 - MPU Region Descriptor */
			MPU_RGD_WORD2_32B_tag RGD8_WORD2; /* offset: 0x0488 size: 32 bit */
			/* MPU_RGDn_Word3 - MPU Region Descriptor */
			MPU_RGD_WORD3_32B_tag RGD8_WORD3; /* offset: 0x048C size: 32 bit */
			/* MPU_RGDn_Word0 - MPU Region Descriptor */
			MPU_RGD_WORD0_32B_tag RGD9_WORD0; /* offset: 0x0490 size: 32 bit */
			/* MPU_RGDn_Word1 - MPU Region Descriptor */
			MPU_RGD_WORD1_32B_tag RGD9_WORD1; /* offset: 0x0494 size: 32 bit */
			/* MPU_RGDn_Word2 - MPU Region Descriptor */
			MPU_RGD_WORD2_32B_tag RGD9_WORD2; /* offset: 0x0498 size: 32 bit */
			/* MPU_RGDn_Word3 - MPU Region Descriptor */
			MPU_RGD_WORD3_32B_tag RGD9_WORD3; /* offset: 0x049C size: 32 bit */
			/* MPU_RGDn_Word0 - MPU Region Descriptor */
			MPU_RGD_WORD0_32B_tag RGD10_WORD0; /* offset: 0x04A0 size: 32 bit */
			/* MPU_RGDn_Word1 - MPU Region Descriptor */
			MPU_RGD_WORD1_32B_tag RGD10_WORD1; /* offset: 0x04A4 size: 32 bit */
			/* MPU_RGDn_Word2 - MPU Region Descriptor */
			MPU_RGD_WORD2_32B_tag RGD10_WORD2; /* offset: 0x04A8 size: 32 bit */
			/* MPU_RGDn_Word3 - MPU Region Descriptor */
			MPU_RGD_WORD3_32B_tag RGD10_WORD3; /* offset: 0x04AC size: 32 bit */
			/* MPU_RGDn_Word0 - MPU Region Descriptor */
			MPU_RGD_WORD0_32B_tag RGD11_WORD0; /* offset: 0x04B0 size: 32 bit */
			/* MPU_RGDn_Word1 - MPU Region Descriptor */
			MPU_RGD_WORD1_32B_tag RGD11_WORD1; /* offset: 0x04B4 size: 32 bit */
			/* MPU_RGDn_Word2 - MPU Region Descriptor */
			MPU_RGD_WORD2_32B_tag RGD11_WORD2; /* offset: 0x04B8 size: 32 bit */
			/* MPU_RGDn_Word3 - MPU Region Descriptor */
			MPU_RGD_WORD3_32B_tag RGD11_WORD3; /* offset: 0x04BC size: 32 bit */
			/* MPU_RGDn_Word0 - MPU Region Descriptor */
			MPU_RGD_WORD0_32B_tag RGD12_WORD0; /* offset: 0x04C0 size: 32 bit */
			/* MPU_RGDn_Word1 - MPU Region Descriptor */
			MPU_RGD_WORD1_32B_tag RGD12_WORD1; /* offset: 0x04C4 size: 32 bit */
			/* MPU_RGDn_Word2 - MPU Region Descriptor */
			MPU_RGD_WORD2_32B_tag RGD12_WORD2; /* offset: 0x04C8 size: 32 bit */
			/* MPU_RGDn_Word3 - MPU Region Descriptor */
			MPU_RGD_WORD3_32B_tag RGD12_WORD3; /* offset: 0x04CC size: 32 bit */
			/* MPU_RGDn_Word0 - MPU Region Descriptor */
			MPU_RGD_WORD0_32B_tag RGD13_WORD0; /* offset: 0x04D0 size: 32 bit */
			/* MPU_RGDn_Word1 - MPU Region Descriptor */
			MPU_RGD_WORD1_32B_tag RGD13_WORD1; /* offset: 0x04D4 size: 32 bit */
			/* MPU_RGDn_Word2 - MPU Region Descriptor */
			MPU_RGD_WORD2_32B_tag RGD13_WORD2; /* offset: 0x04D8 size: 32 bit */
			/* MPU_RGDn_Word3 - MPU Region Descriptor */
			MPU_RGD_WORD3_32B_tag RGD13_WORD3; /* offset: 0x04DC size: 32 bit */
			/* MPU_RGDn_Word0 - MPU Region Descriptor */
			MPU_RGD_WORD0_32B_tag RGD14_WORD0; /* offset: 0x04E0 size: 32 bit */
			/* MPU_RGDn_Word1 - MPU Region Descriptor */
			MPU_RGD_WORD1_32B_tag RGD14_WORD1; /* offset: 0x04E4 size: 32 bit */
			/* MPU_RGDn_Word2 - MPU Region Descriptor */
			MPU_RGD_WORD2_32B_tag RGD14_WORD2; /* offset: 0x04E8 size: 32 bit */
			/* MPU_RGDn_Word3 - MPU Region Descriptor */
			MPU_RGD_WORD3_32B_tag RGD14_WORD3; /* offset: 0x04EC size: 32 bit */
			/* MPU_RGDn_Word0 - MPU Region Descriptor */
			MPU_RGD_WORD0_32B_tag RGD15_WORD0; /* offset: 0x04F0 size: 32 bit */
			/* MPU_RGDn_Word1 - MPU Region Descriptor */
			MPU_RGD_WORD1_32B_tag RGD15_WORD1; /* offset: 0x04F4 size: 32 bit */
			/* MPU_RGDn_Word2 - MPU Region Descriptor */
			MPU_RGD_WORD2_32B_tag RGD15_WORD2; /* offset: 0x04F8 size: 32 bit */
			/* MPU_RGDn_Word3 - MPU Region Descriptor */
			MPU_RGD_WORD3_32B_tag RGD15_WORD3; /* offset: 0x04FC size: 32 bit */
		};
	};
	int8_t MPU_reserved_0500_C[768];
	union {
		/* MPU_RGDAACn -  MPU Region Descriptor Alternate Access Control */
		MPU_RGDAAC_32B_tag RGDAAC[16]; /* offset: 0x0800  (0x0004 x 16) */

		struct {
			/* MPU_RGDAACn -  MPU Region Descriptor Alternate Access Control */
			MPU_RGDAAC_32B_tag RGDAAC0;	 /* offset: 0x0800 size: 32 bit */
			MPU_RGDAAC_32B_tag RGDAAC1;	 /* offset: 0x0804 size: 32 bit */
			MPU_RGDAAC_32B_tag RGDAAC2;	 /* offset: 0x0808 size: 32 bit */
			MPU_RGDAAC_32B_tag RGDAAC3;	 /* offset: 0x080C size: 32 bit */
			MPU_RGDAAC_32B_tag RGDAAC4;	 /* offset: 0x0810 size: 32 bit */
			MPU_RGDAAC_32B_tag RGDAAC5;	 /* offset: 0x0814 size: 32 bit */
			MPU_RGDAAC_32B_tag RGDAAC6;	 /* offset: 0x0818 size: 32 bit */
			MPU_RGDAAC_32B_tag RGDAAC7;	 /* offset: 0x081C size: 32 bit */
			MPU_RGDAAC_32B_tag RGDAAC8;	 /* offset: 0x0820 size: 32 bit */
			MPU_RGDAAC_32B_tag RGDAAC9;	 /* offset: 0x0824 size: 32 bit */
			MPU_RGDAAC_32B_tag RGDAAC10; /* offset: 0x0828 size: 32 bit */
			MPU_RGDAAC_32B_tag RGDAAC11; /* offset: 0x082C size: 32 bit */
			MPU_RGDAAC_32B_tag RGDAAC12; /* offset: 0x0830 size: 32 bit */
			MPU_RGDAAC_32B_tag RGDAAC13; /* offset: 0x0834 size: 32 bit */
			MPU_RGDAAC_32B_tag RGDAAC14; /* offset: 0x0838 size: 32 bit */
			MPU_RGDAAC_32B_tag RGDAAC15; /* offset: 0x083C size: 32 bit */
		};
	};
} MPU_tag;

#define MPU (*(volatile MPU_tag*)0xFFF10000UL)

/****************************************************************/
/*                                                              */
/* Module: SEMA4  */
/*                                                              */
/****************************************************************/

/* Register layout for all registers GATE... */

typedef union { /* SEMA4_GATEn - Semephores Gate Register */
	vuint8_t R;
	struct {
		vuint8_t : 6;
		vuint8_t GTFSM : 2; /* Gate Finite State machine */
	} B;
} SEMA4_GATE_8B_tag;

typedef union { /* SEMA4_CP0INE - Semaphores Processor IRQ Notification Enable
								 */
	vuint16_t R;
	struct {
		vuint16_t INE : 16; /* Interrupt Request Notification Enable */
	} B;
} SEMA4_CP0INE_16B_tag;

typedef union { /* SEMA4_CP1INE - Semaphores Processor IRQ Notification Enable
								 */
	vuint16_t R;
	struct {
		vuint16_t INE : 16; /* Interrupt Request Notification Enable */
	} B;
} SEMA4_CP1INE_16B_tag;

typedef union { /* SEMA4_CP0NTF - Semaphores Processor IRQ Notification */
	vuint16_t R;
	struct {
		vuint16_t GN : 16; /* Gate 0 Notification */
	} B;
} SEMA4_CP0NTF_16B_tag;

typedef union { /* SEMA4_CP1NTF - Semaphores Processor IRQ Notification */
	vuint16_t R;
	struct {
		vuint16_t GN : 16; /* Gate 1 Notification */
	} B;
} SEMA4_CP1NTF_16B_tag;

typedef union { /* SEMA4_RSTGT -  Semaphores Reset Gate */
	vuint16_t R;
	struct {
		vuint16_t : 2;
		vuint16_t RSTGSM : 2; /* Reset Gate Finite State Machine */
		vuint16_t RSTGDP : 7; /* Reset Gate Data Pattern */
		vuint16_t RSTGMS : 3; /* Reset Gate Bus Master */
		vuint16_t RSTGTN : 8; /* Reset Gate Number */
	} B;
} SEMA4_RSTGT_16B_tag;

typedef union { /* SEMA4_RSTNTF - Semaphores Reset Reset IRQ Notification */
	vuint16_t R;
	struct {
		vuint16_t : 2;
		vuint16_t RSTNSM : 2; /* Reset Gate Finite State Machine */
		vuint16_t RSTNDP : 7; /* Reset Gate Data Pattern */
		vuint16_t RSTNMS : 3; /* Reset Gate Bus Master */
		vuint16_t RSTNTN : 8; /* Reset Gate Number */
	} B;
} SEMA4_RSTNTF_16B_tag;

typedef struct SEMA4_struct_tag { /* start of SEMA4_tag */
	union {
		/* SEMA4_GATEn - Semephores Gate Register */
		SEMA4_GATE_8B_tag GATE[16]; /* offset: 0x0000  (0x0001 x 16) */

		struct {
			/* SEMA4_GATEn - Semephores Gate Register */
			SEMA4_GATE_8B_tag GATE0;	/* offset: 0x0000 size: 8 bit */
			SEMA4_GATE_8B_tag GATE1;	/* offset: 0x0001 size: 8 bit */
			SEMA4_GATE_8B_tag GATE2;	/* offset: 0x0002 size: 8 bit */
			SEMA4_GATE_8B_tag GATE3;	/* offset: 0x0003 size: 8 bit */
			SEMA4_GATE_8B_tag GATE4;	/* offset: 0x0004 size: 8 bit */
			SEMA4_GATE_8B_tag GATE5;	/* offset: 0x0005 size: 8 bit */
			SEMA4_GATE_8B_tag GATE6;	/* offset: 0x0006 size: 8 bit */
			SEMA4_GATE_8B_tag GATE7;	/* offset: 0x0007 size: 8 bit */
			SEMA4_GATE_8B_tag GATE8;	/* offset: 0x0008 size: 8 bit */
			SEMA4_GATE_8B_tag GATE9;	/* offset: 0x0009 size: 8 bit */
			SEMA4_GATE_8B_tag GATE10; /* offset: 0x000A size: 8 bit */
			SEMA4_GATE_8B_tag GATE11; /* offset: 0x000B size: 8 bit */
			SEMA4_GATE_8B_tag GATE12; /* offset: 0x000C size: 8 bit */
			SEMA4_GATE_8B_tag GATE13; /* offset: 0x000D size: 8 bit */
			SEMA4_GATE_8B_tag GATE14; /* offset: 0x000E size: 8 bit */
			SEMA4_GATE_8B_tag GATE15; /* offset: 0x000F size: 8 bit */
		};
	};
	int8_t SEMA4_reserved_0010[48];
	/* SEMA4_CP0INE - Semaphores Processor IRQ Notification Enable */
	SEMA4_CP0INE_16B_tag CP0INE; /* offset: 0x0040 size: 16 bit */
	int8_t SEMA4_reserved_0042[6];
	/* SEMA4_CP1INE - Semaphores Processor IRQ Notification Enable */
	SEMA4_CP1INE_16B_tag CP1INE; /* offset: 0x0048 size: 16 bit */
	int8_t SEMA4_reserved_004A[54];
	/* SEMA4_CP0NTF - Semaphores Processor IRQ Notification */
	SEMA4_CP0NTF_16B_tag CP0NTF; /* offset: 0x0080 size: 16 bit */
	int8_t SEMA4_reserved_0082[6];
	/* SEMA4_CP1NTF - Semaphores Processor IRQ Notification */
	SEMA4_CP1NTF_16B_tag CP1NTF; /* offset: 0x0088 size: 16 bit */
	int8_t SEMA4_reserved_008A[118];
	/* SEMA4_RSTGT -  Semaphores Reset Gate */
	SEMA4_RSTGT_16B_tag RSTGT; /* offset: 0x0100 size: 16 bit */
	int8_t SEMA4_reserved_0102[2];
	/* SEMA4_RSTNTF - Semaphores Reset Reset IRQ Notification */
	SEMA4_RSTNTF_16B_tag RSTNTF; /* offset: 0x0104 size: 16 bit */
} SEMA4_tag;

#define SEMA4 (*(volatile SEMA4_tag*)0xFFF24000UL)

/****************************************************************/
/*                                                              */
/* Module: SWT  */
/*                                                              */
/****************************************************************/

typedef union { /* SWT_CR - Control Register */
	vuint32_t R;
	struct {
		vuint32_t MAP0 : 1; /* Master Acces Protection for Master 0 */
		vuint32_t MAP1 : 1; /* Master Acces Protection for Master 1 */
		vuint32_t MAP2 : 1; /* Master Acces Protection for Master 2 */
		vuint32_t MAP3 : 1; /* Master Acces Protection for Master 3 */
		vuint32_t MAP4 : 1; /* Master Acces Protection for Master 4 */
		vuint32_t MAP5 : 1; /* Master Acces Protection for Master 5 */
		vuint32_t MAP6 : 1; /* Master Acces Protection for Master 6 */
		vuint32_t MAP7 : 1; /* Master Acces Protection for Master 7 */
		vuint32_t : 14;
		vuint32_t KEY : 1; /* Keyed Service Mode */
		vuint32_t RIA : 1; /* Reset on Invalid Access */
		vuint32_t WND : 1; /* Window Mode */
		vuint32_t ITR : 1; /* Interrupt Then Reset */
		vuint32_t HLK : 1; /* Hard Lock */
		vuint32_t SLK : 1; /* Soft Lock */
		vuint32_t CSL : 1; /* Clock Selection */
		vuint32_t STP : 1; /* Stop Mode Control */
		vuint32_t FRZ : 1; /* Debug Mode Control */
		vuint32_t WEN : 1; /* Watchdog Enabled */
	} B;
} SWT_CR_32B_tag;

typedef union { /* SWT_IR - SWT Interrupt Register */
	vuint32_t R;
	struct {
		vuint32_t : 31;
		vuint32_t TIF : 1; /* Time Out Interrupt Flag */
	} B;
} SWT_IR_32B_tag;

typedef union { /* SWT_TO - SWT Time-Out Register */
	vuint32_t R;
	struct {
		vuint32_t WTO : 32; /* Watchdog Time Out Period */
	} B;
} SWT_TO_32B_tag;

typedef union { /* SWT_WN - SWT Window Register */
	vuint32_t R;
	struct {
		vuint32_t WST : 32; /* Watchdog Time Out Period */
	} B;
} SWT_WN_32B_tag;

typedef union { /* SWT_SR - SWT Service Register */
	vuint32_t R;
	struct {
		vuint32_t : 16;
		vuint32_t WSC : 16; /* Watchdog Service Code */
	} B;
} SWT_SR_32B_tag;

typedef union { /* SWT_CO - SWT Counter Output Register */
	vuint32_t R;
	struct {
		vuint32_t CNT : 32; /* Watchdog Count */
	} B;
} SWT_CO_32B_tag;

typedef union { /* SWT_SK - SWT Service Key Register */
	vuint32_t R;
	struct {
		vuint32_t : 16;
		vuint32_t SERVICEKEY : 16; /* Service Key */
	} B;
} SWT_SK_32B_tag;

typedef struct SWT_struct_tag { /* start of SWT_tag */
																/* SWT_CR - Control Register */
	SWT_CR_32B_tag CR;						/* offset: 0x0000 size: 32 bit */
																/* SWT_IR - SWT Interrupt Register */
	SWT_IR_32B_tag IR;						/* offset: 0x0004 size: 32 bit */
																/* SWT_TO - SWT Time-Out Register */
	SWT_TO_32B_tag TO;						/* offset: 0x0008 size: 32 bit */
																/* SWT_WN - SWT Window Register */
	SWT_WN_32B_tag WN;						/* offset: 0x000C size: 32 bit */
																/* SWT_SR - SWT Service Register */
	SWT_SR_32B_tag SR;						/* offset: 0x0010 size: 32 bit */
																/* SWT_CO - SWT Counter Output Register */
	SWT_CO_32B_tag CO;						/* offset: 0x0014 size: 32 bit */
																/* SWT_SK - SWT Service Key Register */
	SWT_SK_32B_tag SK;						/* offset: 0x0018 size: 32 bit */
} SWT_tag;

#define SWT (*(volatile SWT_tag*)0xFFF38000UL)

/****************************************************************/
/*                                                              */
/* Module: STM  */
/*                                                              */
/****************************************************************/

typedef union { /* STM_CR - Control Register */
	vuint32_t R;
	struct {
		vuint32_t : 16;
		vuint32_t CPS : 8; /* Counter Prescaler */
		vuint32_t : 6;
		vuint32_t FRZ : 1; /* Freeze Control */
		vuint32_t TEN : 1; /* Timer Counter Enabled */
	} B;
} STM_CR_32B_tag;

typedef union { /* STM_CNT - STM Count Register */
	vuint32_t R;
} STM_CNT_32B_tag;

/* Register layout for all registers CCR... */

typedef union { /* STM_CCRn - STM Channel Control Register */
	vuint32_t R;
	struct {
		vuint32_t : 31;
		vuint32_t CEN : 1; /* Channel Enable */
	} B;
} STM_CCR_32B_tag;

/* Register layout for all registers CIR... */

typedef union { /* STM_CIRn - STM Channel Interrupt Register */
	vuint32_t R;
	struct {
		vuint32_t : 31;
		vuint32_t CIF : 1; /* Channel Interrupt Flag */
	} B;
} STM_CIR_32B_tag;

/* Register layout for all registers CMP... */

typedef union { /* STM_CMPn - STM Channel Compare Register */
	vuint32_t R;
} STM_CMP_32B_tag;

typedef struct STM_CHANNEL_struct_tag {
	/* STM_CCRn - STM Channel Control Register */
	STM_CCR_32B_tag CCR; /* relative offset: 0x0000 */
											 /* STM_CIRn - STM Channel Interrupt Register */
	STM_CIR_32B_tag CIR; /* relative offset: 0x0004 */
											 /* STM_CMPn - STM Channel Compare Register */
	STM_CMP_32B_tag CMP; /* relative offset: 0x0008 */
	int8_t STM_CHANNEL_reserved_000C[4];

} STM_CHANNEL_tag;

typedef struct STM_struct_tag { /* start of STM_tag */
	union {
		STM_CR_32B_tag CR0; /* deprecated - please avoid */

		/* STM_CR - Control Register */
		STM_CR_32B_tag CR; /* offset: 0x0000 size: 32 bit */
	};
	union {
		STM_CNT_32B_tag CNT0; /* deprecated - please avoid */

		/* STM_CNT - STM Count Register */
		STM_CNT_32B_tag CNT; /* offset: 0x0004 size: 32 bit */
	};
	int8_t STM_reserved_0008_C[8];
	union {
		/*  Register set CHANNEL */
		STM_CHANNEL_tag CHANNEL[4]; /* offset: 0x0010  (0x0010 x 4) */

		struct {
			/* STM_CCRn - STM Channel Control Register */
			STM_CCR_32B_tag CCR0; /* offset: 0x0010 size: 32 bit */
														/* STM_CIRn - STM Channel Interrupt Register */
			STM_CIR_32B_tag CIR0; /* offset: 0x0014 size: 32 bit */
														/* STM_CMPn - STM Channel Compare Register */
			STM_CMP_32B_tag CMP0; /* offset: 0x0018 size: 32 bit */
			int8_t STM_reserved_001C_I1[4];
			/* STM_CCRn - STM Channel Control Register */
			STM_CCR_32B_tag CCR1; /* offset: 0x0020 size: 32 bit */
														/* STM_CIRn - STM Channel Interrupt Register */
			STM_CIR_32B_tag CIR1; /* offset: 0x0024 size: 32 bit */
														/* STM_CMPn - STM Channel Compare Register */
			STM_CMP_32B_tag CMP1; /* offset: 0x0028 size: 32 bit */
			int8_t STM_reserved_002C_I1[4];
			/* STM_CCRn - STM Channel Control Register */
			STM_CCR_32B_tag CCR2; /* offset: 0x0030 size: 32 bit */
														/* STM_CIRn - STM Channel Interrupt Register */
			STM_CIR_32B_tag CIR2; /* offset: 0x0034 size: 32 bit */
														/* STM_CMPn - STM Channel Compare Register */
			STM_CMP_32B_tag CMP2; /* offset: 0x0038 size: 32 bit */
			int8_t STM_reserved_003C_I1[4];
			/* STM_CCRn - STM Channel Control Register */
			STM_CCR_32B_tag CCR3; /* offset: 0x0040 size: 32 bit */
														/* STM_CIRn - STM Channel Interrupt Register */
			STM_CIR_32B_tag CIR3; /* offset: 0x0044 size: 32 bit */
														/* STM_CMPn - STM Channel Compare Register */
			STM_CMP_32B_tag CMP3; /* offset: 0x0048 size: 32 bit */
			int8_t STM_reserved_004C_E1[4];
		};
	};
} STM_tag;

#define STM (*(volatile STM_tag*)0xFFF3C000UL)

/****************************************************************/
/*                                                              */
/* Module: SPP_MCM  */
/*                                                              */
/****************************************************************/

typedef union { /* SPP_MCM_PCT - Processor Core Type */
	vuint16_t R;
	struct {
		vuint16_t PCTYPE : 16; /* Processor Core Type */
	} B;
} SPP_MCM_PCT_16B_tag;

typedef union { /* SPP_MCM_PLREV - SOC-Defined Platform Revision */
	vuint16_t R;
	struct {
		vuint16_t PLREVISION : 16; /* Platform Revision */
	} B;
} SPP_MCM_PLREV_16B_tag;

typedef union { /* SPP_MCM_IOPMC - IPS On-Platform Module Configuration */
	vuint32_t R;
	struct {
		vuint32_t PMC : 32; /* IPS Module Configuration */
	} B;
} SPP_MCM_IOPMC_32B_tag;

typedef union { /* SPP_MCM_MRSR - Miscellaneous Reset Status Register */
	vuint8_t R;
	struct {
		vuint8_t POR : 1; /* Power on Reset */
#ifndef USE_FIELD_ALIASES_SPP_MCM
		vuint8_t OFPLR : 1; /* Off-Platform Reset */
#else
		vuint8_t DIR : 1;					 /* deprecated name - please avoid */
#endif
		vuint8_t : 6;
	} B;
} SPP_MCM_MRSR_8B_tag;

typedef union { /* SPP_MCM_MWCR - Miscellaneous Wakeup Control Register */
	vuint8_t R;
	struct {
		vuint8_t ENBWCR : 1; /* Enable WCR */
		vuint8_t : 3;
		vuint8_t PRILVL : 4; /* Interrupt Priority Level */
	} B;
} SPP_MCM_MWCR_8B_tag;

typedef union { /* SPP_MCM_MIR - Miscellaneous Interrupt Register */
	vuint8_t R;
	struct {
		vuint8_t FB0AI : 1; /* Flash Bank 0 Abort Interrupt */
		vuint8_t FB0SI : 1; /* Flash Bank 0 Stall Interrupt */
		vuint8_t FB1AI : 1; /* Flash Bank 1 Abort Interrupt */
		vuint8_t FB1SI : 1; /* Flash Bank 1 Stall Interrupt */
		vuint8_t FB2AI : 1; /* Flash Bank 2 Abort Interrupt */
		vuint8_t FB2SI : 1; /* Flash Bank 2 Stall Interrupt */
		vuint8_t : 2;
	} B;
} SPP_MCM_MIR_8B_tag;

typedef union { /* SPP_MCM_MUDCR - Miscellaneous User Defined Control Register
								 */
	vuint32_t R;
	struct {
		vuint32_t MUSERDCR : 32; /* User Defined Control Register */
	} B;
} SPP_MCM_MUDCR_32B_tag;

typedef union { /* SPP_MCM_ECR - ECC Configuration Register */
	vuint8_t R;
	struct {
		vuint8_t : 2;
#ifndef USE_FIELD_ALIASES_SPP_MCM
		vuint8_t EPR1BR : 1; /* Enable Platform RAM 1-bit Reporting */
#else
		vuint8_t ER1BR : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_SPP_MCM
		vuint8_t EPF1BR : 1; /* Enable Platform FLASH 1-bit Reporting */
#else
		vuint8_t EF1BR : 1;				 /* deprecated name - please avoid */
#endif
		vuint8_t : 2;
#ifndef USE_FIELD_ALIASES_SPP_MCM
		vuint8_t EPRNCR : 1; /* Enable Platform RAM Non-Correctable Reporting */
#else
		vuint8_t ERNCR : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_SPP_MCM
		vuint8_t EPFNCR : 1; /* Enable Platform FLASH Non-Correctable Reporting */
#else
		vuint8_t EFNCR : 1;				 /* deprecated name - please avoid */
#endif
	} B;
} SPP_MCM_ECR_8B_tag;

typedef union { /* SPP_MCM_ESR - ECC Status Register */
	vuint8_t R;
	struct {
		vuint8_t : 2;
#ifndef USE_FIELD_ALIASES_SPP_MCM
		vuint8_t PR1BC : 1; /* Platform RAM 1-bit Correction */
#else
		vuint8_t R1BC : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_SPP_MCM
		vuint8_t PF1BC : 1; /* Platform FLASH 1-bit Correction */
#else
		vuint8_t F1BC : 1;				 /* deprecated name - please avoid */
#endif
		vuint8_t : 2;
#ifndef USE_FIELD_ALIASES_SPP_MCM
		vuint8_t PRNCE : 1; /* Platform RAM Non-Correctable Error */
#else
		vuint8_t RNCE : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_SPP_MCM
		vuint8_t PFNCE : 1; /* Platform FLASH Non-Correctable Error */
#else
		vuint8_t FNCE : 1;				 /* deprecated name - please avoid */
#endif
	} B;
} SPP_MCM_ESR_8B_tag;

typedef union { /* SPP_MCM_EEGR - ECC Error Generation Register */
	vuint16_t R;
	struct {
		vuint16_t
				FRCAP : 1; /* Force Platform RAM Error Injection Access Protection */
		vuint16_t : 1;
		vuint16_t
				FRC1BI : 1; /* Force Platform RAM Continuous 1-Bit Data Inversions */
		vuint16_t FR11BI : 1; /* Force Platform RAM One 1-Bit Data Inversion */
		vuint16_t : 2;
		vuint16_t FRCNCI : 1; /* Force Platform RAM Continuous Noncorrectable Data
														 Inversions */
		vuint16_t
				FR1NCI : 1; /* Force Platform RAM One Noncorrectable Data Inversions */
		vuint16_t : 1;
		vuint16_t ERRBIT : 7; /* Error Bit Position */
	} B;
} SPP_MCM_EEGR_16B_tag;

typedef union { /* SPP_MCM_PFEAR - Platform Flash ECC Error Address Register */
	vuint32_t R;
} SPP_MCM_PFEAR_32B_tag;

typedef union { /* SPP_MCM_PFEMR - Platform Flash ECC Master Number Register */
	vuint8_t R;
} SPP_MCM_PFEMR_8B_tag;

typedef union { /* SPP_MCM_PFEAT - Platform Flash ECC Attributes Register */
	vuint8_t R;
	struct {
#ifndef USE_FIELD_ALIASES_SPP_MCM
		vuint8_t F_WRITE : 1; /* AMBA-AHBH Write */
#else
		vuint8_t WRITE : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_SPP_MCM
		vuint8_t F_SIZE : 3; /* AMBA-AHBH Size */
#else
		vuint8_t SIZE : 3;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_SPP_MCM
		vuint8_t F_PROTECT : 4; /* AMBA-AHBH PROT */
#else
		vuint8_t PROTECTION : 4;	 /* deprecated name - please avoid */
#endif
	} B;
} SPP_MCM_PFEAT_8B_tag;

typedef union { /* SPP_MCM_PFEDRH - Platform Flash ECC Data Register High Word
								 */
	vuint32_t R;
} SPP_MCM_PFEDRH_32B_tag;

typedef union { /* SPP_MCM_PFEDR - Platform Flash ECC Data Register */
	vuint32_t R;
} SPP_MCM_PFEDR_32B_tag;

typedef union { /* SPP_MCM_PREAR - Platform RAM ECC Address Register */
	vuint32_t R;
} SPP_MCM_PREAR_32B_tag;

typedef union { /* SPP_MCM_PRESR - Platform RAM ECC Syndrome Register */
	vuint8_t R;
} SPP_MCM_PRESR_8B_tag;

typedef union { /* SPP_MCM_PREMR - Platform RAM ECC Master Number Register */
	vuint8_t R;
	struct {
		vuint8_t : 4;
#ifndef USE_FIELD_ALIASES_SPP_MCM
		vuint8_t PR_EMR : 4; /* Platform RAM ECC Master Number */
#else
		vuint8_t REMR : 4;				 /* deprecated name - please avoid */
#endif
	} B;
} SPP_MCM_PREMR_8B_tag;

typedef union { /* SPP_MCM_PREAT - Platform RAM ECC Attributes Register */
	vuint8_t R;
	struct {
#ifndef USE_FIELD_ALIASES_SPP_MCM
		vuint8_t R_WRITE : 1; /* AMBA-AHBH Write */
#else
		vuint8_t WRITE : 1;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_SPP_MCM
		vuint8_t R_SIZE : 3; /* AMBA-AHBH Size */
#else
		vuint8_t SIZE : 3;				 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_SPP_MCM
		vuint8_t R_PROTECT : 4; /* AMBA-AHBH PROT */
#else
		vuint8_t PROTECTION : 4;	 /* deprecated name - please avoid */
#endif
	} B;
} SPP_MCM_PREAT_8B_tag;

typedef union { /* SPP_MCM_PREDR - Platform RAM ECC Data Register High Word */
	vuint32_t R;
} SPP_MCM_PREDRH_32B_tag;

typedef union { /* SPP_MCM_PREDR - Platform RAM ECC Data Register */
	vuint32_t R;
} SPP_MCM_PREDR_32B_tag;

typedef struct SPP_MCM_struct_tag { /* start of SPP_MCM_tag */
																		/* SPP_MCM_PCT - Processor Core Type */
	SPP_MCM_PCT_16B_tag PCT;					/* offset: 0x0000 size: 16 bit */
	union {
		SPP_MCM_PLREV_16B_tag REV; /* deprecated - please avoid */

		/* SPP_MCM_PLREV - SOC-Defined Platform Revision */
		SPP_MCM_PLREV_16B_tag PLREV; /* offset: 0x0002 size: 16 bit */
	};
	int8_t SPP_MCM_reserved_0004_C[4];
	union {
		SPP_MCM_IOPMC_32B_tag MC; /* deprecated - please avoid */

		/* SPP_MCM_IOPMC - IPS On-Platform Module Configuration */
		SPP_MCM_IOPMC_32B_tag IOPMC; /* offset: 0x0008 size: 32 bit */
	};
	int8_t SPP_MCM_reserved_000C[3];
	/* SPP_MCM_MRSR - Miscellaneous Reset Status Register */
	SPP_MCM_MRSR_8B_tag MRSR; /* offset: 0x000F size: 8 bit */
	int8_t SPP_MCM_reserved_0010[3];
	/* SPP_MCM_MWCR - Miscellaneous Wakeup Control Register */
	SPP_MCM_MWCR_8B_tag MWCR; /* offset: 0x0013 size: 8 bit */
	int8_t SPP_MCM_reserved_0014[11];
	/* SPP_MCM_MIR - Miscellaneous Interrupt Register */
	SPP_MCM_MIR_8B_tag MIR; /* offset: 0x001F size: 8 bit */
	int8_t SPP_MCM_reserved_0020[4];
	/* SPP_MCM_MUDCR - Miscellaneous User Defined Control Register */
	SPP_MCM_MUDCR_32B_tag MUDCR; /* offset: 0x0024 size: 32 bit */
	int8_t SPP_MCM_reserved_0028[27];
	/* SPP_MCM_ECR - ECC Configuration Register */
	SPP_MCM_ECR_8B_tag ECR; /* offset: 0x0043 size: 8 bit */
	int8_t SPP_MCM_reserved_0044[3];
	/* SPP_MCM_ESR - ECC Status Register */
	SPP_MCM_ESR_8B_tag ESR; /* offset: 0x0047 size: 8 bit */
	int8_t SPP_MCM_reserved_0048[2];
	/* SPP_MCM_EEGR - ECC Error Generation Register */
	SPP_MCM_EEGR_16B_tag EEGR; /* offset: 0x004A size: 16 bit */
	int8_t SPP_MCM_reserved_004C_C[4];
	union {
		/* SPP_MCM_PFEAR - Platform Flash ECC Error Address Register */
		SPP_MCM_PFEAR_32B_tag PFEAR; /* offset: 0x0050 size: 32 bit */

		SPP_MCM_PFEAR_32B_tag FEAR; /* deprecated - please avoid */
	};
	int8_t SPP_MCM_reserved_0054_C[2];
	union {
		/* SPP_MCM_PFEMR - Platform Flash ECC Master Number Register */
		SPP_MCM_PFEMR_8B_tag PFEMR; /* offset: 0x0056 size: 8 bit */

		SPP_MCM_PFEMR_8B_tag FEMR; /* deprecated - please avoid */
	};
	union {
		/* SPP_MCM_PFEAT - Platform Flash ECC Attributes Register */
		SPP_MCM_PFEAT_8B_tag PFEAT; /* offset: 0x0057 size: 8 bit */

		SPP_MCM_PFEAT_8B_tag FEAT; /* deprecated - please avoid */
	};
	/* SPP_MCM_PFEDRH - Platform Flash ECC Data Register High Word */
	SPP_MCM_PFEDRH_32B_tag PFEDRH; /* offset: 0x0058 size: 32 bit */
	union {
		/* SPP_MCM_PFEDR - Platform Flash ECC Data Register */
		SPP_MCM_PFEDR_32B_tag PFEDR; /* offset: 0x005C size: 32 bit */

		SPP_MCM_PFEDR_32B_tag FEDR; /* deprecated - please avoid */
	};
	union {
		SPP_MCM_PREAR_32B_tag REAR; /* deprecated - please avoid */

		/* SPP_MCM_PREAR - Platform RAM ECC Address Register */
		SPP_MCM_PREAR_32B_tag PREAR; /* offset: 0x0060 size: 32 bit */
	};
	int8_t SPP_MCM_reserved_0064_C;
	union {
		SPP_MCM_PRESR_8B_tag RESR; /* deprecated - please avoid */

		/* SPP_MCM_PRESR - Platform RAM ECC Syndrome Register */
		SPP_MCM_PRESR_8B_tag PRESR; /* offset: 0x0065 size: 8 bit */
	};
	union {
		SPP_MCM_PREMR_8B_tag REMR; /* deprecated - please avoid */

		/* SPP_MCM_PREMR - Platform RAM ECC Master Number Register */
		SPP_MCM_PREMR_8B_tag PREMR; /* offset: 0x0066 size: 8 bit */
	};
	union {
		SPP_MCM_PREAT_8B_tag REAT; /* deprecated - please avoid */

		/* SPP_MCM_PREAT - Platform RAM ECC Attributes Register */
		SPP_MCM_PREAT_8B_tag PREAT; /* offset: 0x0067 size: 8 bit */
	};
	/* SPP_MCM_PREDR - Platform RAM ECC Data Register High Word */
	SPP_MCM_PREDRH_32B_tag PREDRH; /* offset: 0x0068 size: 32 bit */
	union {
		SPP_MCM_PREDR_32B_tag REDR; /* deprecated - please avoid */

		/* SPP_MCM_PREDR - Platform RAM ECC Data Register */
		SPP_MCM_PREDR_32B_tag PREDR; /* offset: 0x006C size: 32 bit */
	};
} SPP_MCM_tag;

#define SPP_MCM (*(volatile SPP_MCM_tag*)0xFFF40000UL)

/****************************************************************/
/*                                                              */
/* Module: SPP_DMA2  */
/*                                                              */
/****************************************************************/

typedef union { /* SPP_DMA2_DMACR - DMA Control Register */
	vuint32_t R;
	struct {
		vuint32_t : 14;
		vuint32_t CX : 1;			 /* Cancel Transfer */
		vuint32_t ECX : 1;		 /* Error Cancel Transfer */
		vuint32_t GRP3PRI : 2; /* Channel Group 3 Priority */
		vuint32_t GRP2PRI : 2; /* Channel Group 2 Priority */
		vuint32_t GRP1PRI : 2; /* Channel Group 1 Priority */
		vuint32_t GRP0PRI : 2; /* Channel Group 0 Priority */
		vuint32_t EMLM : 1;		 /* Enable Minor Loop Mapping */
		vuint32_t CLM : 1;		 /* Continuous Link Mode */
		vuint32_t HALT : 1;		 /* Halt DMA Operations */
		vuint32_t HOE : 1;		 /* Halt on Error */
		vuint32_t ERGA : 1;		 /* Enable Round Robin Group Arbitration */
		vuint32_t ERCA : 1;		 /* Enable Round Robin Channel Arbitration */
		vuint32_t EDBG : 1;		 /* Enable Debug */
		vuint32_t EBW : 1;		 /* Enable Buffered Writes */
	} B;
} SPP_DMA2_DMACR_32B_tag;

typedef union { /* SPP_DMA2_DMAES - DMA Error Status Register */
	vuint32_t R;
	struct {
		vuint32_t VLD : 1; /* Logical OR of DMAERRH and DMAERRL status bits */
		vuint32_t : 14;
		vuint32_t ECX : 1;		/* Transfer Cancelled */
		vuint32_t GPE : 1;		/* Group Priority Error */
		vuint32_t CPE : 1;		/* Channel Priority Error */
		vuint32_t ERRCHN : 6; /* Error Channel Number or Cancelled Channel Number */
		vuint32_t SAE : 1;		/* Source Address Error */
		vuint32_t SOE : 1;		/* Source Offset Error */
		vuint32_t DAE : 1;		/* Destination Address Error */
		vuint32_t DOE : 1;		/* Destination Offset Error */
		vuint32_t NCE : 1;		/* Nbytes/Citer Configuration Error */
		vuint32_t SGE : 1;		/* Scatter/Gather Configuration Error */
		vuint32_t SBE : 1;		/* Source Bus Error */
		vuint32_t DBE : 1;		/* Destination Bus Error */
	} B;
} SPP_DMA2_DMAES_32B_tag;

typedef union { /* SPP_DMA2_DMAERQH - DMA Enable Request Register */
	vuint32_t R;
	struct {
		vuint32_t ERQ : 32; /* DMA Enable Request */
	} B;
} SPP_DMA2_DMAERQH_32B_tag;

typedef union { /* SPP_DMA2_DMAERQL - DMA Enable Request Register */
	vuint32_t R;
	struct {
		vuint32_t ERQ : 32; /* DMA Enable Request */
	} B;
} SPP_DMA2_DMAERQL_32B_tag;

typedef union { /* SPP_DMA2_DMAEEIH - DMA Enable Error Interrupt Register */
	vuint32_t R;
	struct {
		vuint32_t EEI : 32; /* DMA Enable Error Interrupt */
	} B;
} SPP_DMA2_DMAEEIH_32B_tag;

typedef union { /* SPP_DMA2_DMAEEIL - DMA Enable Error Interrupt Register */
	vuint32_t R;
	struct {
		vuint32_t EEI : 32; /* DMA Enable Error Interrupt */
	} B;
} SPP_DMA2_DMAEEIL_32B_tag;

typedef union { /* SPP_DMA2_DMASERQ - DMA Set Enable Request Register */
	vuint8_t R;
	struct {
		vuint8_t : 1;
		vuint8_t SERQ : 7; /* Set Enable Request */
	} B;
} SPP_DMA2_DMASERQ_8B_tag;

typedef union { /* SPP_DMA2_DMACERQ - DMA Clear Enable Request Register */
	vuint8_t R;
	struct {
		vuint8_t : 1;
		vuint8_t CERQ : 7; /* Clear Enable Request */
	} B;
} SPP_DMA2_DMACERQ_8B_tag;

typedef union { /* SPP_DMA2_DMASEEI - DMA Set Enable Error Interrupt Register */
	vuint8_t R;
	struct {
		vuint8_t : 1;
		vuint8_t SEEI : 7; /* Set Enable Error Interrupt */
	} B;
} SPP_DMA2_DMASEEI_8B_tag;

typedef union { /* SPP_DMA2_DMACEEI - DMA Clear Enable Error Interrupt Register
								 */
	vuint8_t R;
	struct {
		vuint8_t : 1;
		vuint8_t CEEI : 7; /* Clear Enable Error Interrupt */
	} B;
} SPP_DMA2_DMACEEI_8B_tag;

typedef union { /* SPP_DMA2_DMACINT - DMA Clear Interrupt Request */
	vuint8_t R;
	struct {
		vuint8_t : 1;
		vuint8_t CINT : 7; /* Clear Interrupt Request */
	} B;
} SPP_DMA2_DMACINT_8B_tag;

typedef union { /* SPP_DMA2_DMACERR - DMA Clear Error */
	vuint8_t R;
	struct {
		vuint8_t : 1;
		vuint8_t CERR : 7; /* Clear Error Indicator */
	} B;
} SPP_DMA2_DMACERR_8B_tag;

typedef union { /* SPP_DMA2_DMASSRT - DMA Set START Bit */
	vuint8_t R;
	struct {
		vuint8_t : 1;
		vuint8_t SSRT : 7; /* Set START Bit */
	} B;
} SPP_DMA2_DMASSRT_8B_tag;

typedef union { /* SPP_DMA2_DMACDNE - DMA Clear DONE Status */
	vuint8_t R;
	struct {
		vuint8_t : 1;
		vuint8_t CDNE : 7; /* Clear DONE Status Bit */
	} B;
} SPP_DMA2_DMACDNE_8B_tag;

typedef union { /* SPP_DMA2_DMAINTH - DMA Interrupt Request Register */
	vuint32_t R;
	struct {
		vuint32_t INT : 32; /* DMA Interrupt Request */
	} B;
} SPP_DMA2_DMAINTH_32B_tag;

typedef union { /* SPP_DMA2_DMAINTL - DMA Interrupt Request Register */
	vuint32_t R;
	struct {
		vuint32_t INT : 32; /* DMA Interrupt Request */
	} B;
} SPP_DMA2_DMAINTL_32B_tag;

typedef union { /* SPP_DMA2_DMAERRH - DMA Error Register */
	vuint32_t R;
	struct {
		vuint32_t ERR : 32; /* DMA Error n */
	} B;
} SPP_DMA2_DMAERRH_32B_tag;

typedef union { /* SPP_DMA2_DMAERRL - DMA Error Register */
	vuint32_t R;
	struct {
		vuint32_t ERR : 32; /* DMA Error n */
	} B;
} SPP_DMA2_DMAERRL_32B_tag;

typedef union { /* SPP_DMA2_DMAHRSH - DMA Hardware Request Status Register */
	vuint32_t R;
	struct {
		vuint32_t HRS : 32; /* DMA Hardware Request Status */
	} B;
} SPP_DMA2_DMAHRSH_32B_tag;

typedef union { /* SPP_DMA2_DMAHRSL - DMA Hardware Request Status Register */
	vuint32_t R;
	struct {
		vuint32_t HRS : 32; /* DMA Hardware Request Status */
	} B;
} SPP_DMA2_DMAHRSL_32B_tag;

typedef union { /* SPP_DMA2_DMAGPOR - DMA General Purpose Output Register */
	vuint32_t R;
	struct {
		vuint32_t GPOR : 32; /* DMA General Purpose Output */
	} B;
} SPP_DMA2_DMAGPOR_32B_tag;

/* Register layout for all registers DCHPRI... */

typedef union { /* SPP_DMA2_DCHPRIn - DMA Channel n Priority */
	vuint8_t R;
	struct {
		vuint8_t ECP : 1;		 /* Enable Channel Preemption */
		vuint8_t DPA : 1;		 /* Disable Preempt Ability */
		vuint8_t GRPPRI : 2; /* Channel n Current Group Priority */
		vuint8_t CHPRI : 4;	 /* Channel n Arbitration Priority */
	} B;
} SPP_DMA2_DCHPRI_8B_tag;

/* Register layout for all registers TCDWORD0_... */

typedef union { /* SPP_DMA2_TCDn Word0 - Source Address */
	vuint32_t R;
	struct {
		vuint32_t SADDR : 32; /* Source Address */
	} B;
} SPP_DMA2_TCDWORD0__32B_tag;

/* Register layout for all registers TCDWORD4_... */

typedef union { /* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
	vuint32_t R;
	struct {
		vuint32_t SMOD : 5;	 /* Source Address Modulo */
		vuint32_t SSIZE : 3; /* Source Data Transfer Size */
		vuint32_t DMOD : 5;	 /* Destination Address Module */
		vuint32_t DSIZE : 3; /* Destination Data Transfer Size */
		vuint32_t SOFF : 16; /* Source Address Signed Offset */
	} B;
} SPP_DMA2_TCDWORD4__32B_tag;

/* Register layout for all registers TCDWORD8_... */

typedef union { /* SPP_DMA2_TCDn Word2 - nbytes */
	vuint32_t R;
	struct {
		vuint32_t SMLOE : 1;	 /* Source Minor Loop Offset Enable */
		vuint32_t DMLOE : 1;	 /* Destination Minor Loop Offset Enable */
		vuint32_t MLOFF : 20;	 /* Minor Loop Offset */
		vuint32_t NBYTES : 10; /* Inner Minor byte transfer Count */
	} B;
} SPP_DMA2_TCDWORD8__32B_tag;

/* Register layout for all registers TCDWORD12_... */

typedef union { /* SPP_DMA2_TCDn Word3 - slast */
	vuint32_t R;
	struct {
		vuint32_t SLAST : 32; /* Last Source Address Adjustment */
	} B;
} SPP_DMA2_TCDWORD12__32B_tag;

/* Register layout for all registers TCDWORD16_... */

typedef union { /* SPP_DMA2_TCDn Word4 - daddr */
	vuint32_t R;
	struct {
		vuint32_t DADDR : 32; /* Destination Address */
	} B;
} SPP_DMA2_TCDWORD16__32B_tag;

/* Register layout for all registers TCDWORD20_... */

typedef union { /* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
	vuint32_t R;
	struct {
		vuint32_t CITER_E_LINK : 1; /* Enable Channel to channel linking on minor
																	 loop complete */
		vuint32_t CITER_LINKCH : 6; /* Link Channel Number */
		vuint32_t CITER : 9;				/* Current Major Iteration Count */
		vuint32_t DOFF : 16;				/* Destination Address Signed Offset */
	} B;
} SPP_DMA2_TCDWORD20__32B_tag;

/* Register layout for all registers TCDWORD24_... */

typedef union { /* SPP_DMA2_TCDn Word6 -  dlast_sga */
	vuint32_t R;
	struct {
		vuint32_t DLAST_SGA : 32; /* Last destination address adjustment */
	} B;
} SPP_DMA2_TCDWORD24__32B_tag;

/* Register layout for all registers TCDWORD28_... */

typedef union { /* SPP_DMA2_TCDn Word7 -  biter, etc. */
	vuint32_t R;
	struct {
		vuint32_t BITER : 16; /* Enable Channel to Channel linking on minor loop
														 complete */
		vuint32_t BWC : 2;		/* Bandwidth Control */
		vuint32_t MAJOR_LINKCH : 6; /* Link Channel Number */
		vuint32_t DONE : 1;					/* channel done */
		vuint32_t ACTIVE : 1;				/* Channel Active */
		vuint32_t MAJOR_E_LINK : 1; /* Enable Channel to Channel Linking on major
																	 loop complete */
		vuint32_t E_SG : 1;					/* Enable Scatter/Gather Processing */
		vuint32_t D_REQ : 1;				/* Disable Request */
		vuint32_t INT_HALF : 1; /* Enable an Interrupt when Major Counter is half
															 complete */
		vuint32_t INT_MAJ : 1;	/* Enable an Interrupt when Major Iteration count
															 completes */
		vuint32_t START : 1;		/* Channel Start */
	} B;
} SPP_DMA2_TCDWORD28__32B_tag;

typedef struct SPP_DMA2_CHANNEL_struct_tag {
	/* SPP_DMA2_TCDn Word0 - Source Address */
	SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_; /* relative offset: 0x0000 */
	/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
	SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_;		/* relative offset: 0x0004 */
																					/* SPP_DMA2_TCDn Word2 - nbytes */
	SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_;		/* relative offset: 0x0008 */
																					/* SPP_DMA2_TCDn Word3 - slast */
	SPP_DMA2_TCDWORD12__32B_tag TCDWORD12_; /* relative offset: 0x000C */
																					/* SPP_DMA2_TCDn Word4 - daddr */
	SPP_DMA2_TCDWORD16__32B_tag TCDWORD16_; /* relative offset: 0x0010 */
	/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
	SPP_DMA2_TCDWORD20__32B_tag TCDWORD20_; /* relative offset: 0x0014 */
																					/* SPP_DMA2_TCDn Word6 -  dlast_sga */
	SPP_DMA2_TCDWORD24__32B_tag TCDWORD24_; /* relative offset: 0x0018 */
	/* SPP_DMA2_TCDn Word7 -  biter, etc. */
	SPP_DMA2_TCDWORD28__32B_tag TCDWORD28_; /* relative offset: 0x001C */

} SPP_DMA2_CHANNEL_tag;

typedef struct SPP_DMA2_struct_tag { /* start of SPP_DMA2_tag */
																		 /* SPP_DMA2_DMACR - DMA Control Register */
	SPP_DMA2_DMACR_32B_tag DMACR;			 /* offset: 0x0000 size: 32 bit */
	/* SPP_DMA2_DMAES - DMA Error Status Register */
	SPP_DMA2_DMAES_32B_tag DMAES; /* offset: 0x0004 size: 32 bit */
	/* SPP_DMA2_DMAERQH - DMA Enable Request Register */
	SPP_DMA2_DMAERQH_32B_tag DMAERQH; /* offset: 0x0008 size: 32 bit */
	/* SPP_DMA2_DMAERQL - DMA Enable Request Register */
	SPP_DMA2_DMAERQL_32B_tag DMAERQL; /* offset: 0x000C size: 32 bit */
	/* SPP_DMA2_DMAEEIH - DMA Enable Error Interrupt Register */
	SPP_DMA2_DMAEEIH_32B_tag DMAEEIH; /* offset: 0x0010 size: 32 bit */
	/* SPP_DMA2_DMAEEIL - DMA Enable Error Interrupt Register */
	SPP_DMA2_DMAEEIL_32B_tag DMAEEIL; /* offset: 0x0014 size: 32 bit */
	/* SPP_DMA2_DMASERQ - DMA Set Enable Request Register */
	SPP_DMA2_DMASERQ_8B_tag DMASERQ; /* offset: 0x0018 size: 8 bit */
	/* SPP_DMA2_DMACERQ - DMA Clear Enable Request Register */
	SPP_DMA2_DMACERQ_8B_tag DMACERQ; /* offset: 0x0019 size: 8 bit */
	/* SPP_DMA2_DMASEEI - DMA Set Enable Error Interrupt Register */
	SPP_DMA2_DMASEEI_8B_tag DMASEEI; /* offset: 0x001A size: 8 bit */
	/* SPP_DMA2_DMACEEI - DMA Clear Enable Error Interrupt Register */
	SPP_DMA2_DMACEEI_8B_tag DMACEEI; /* offset: 0x001B size: 8 bit */
	/* SPP_DMA2_DMACINT - DMA Clear Interrupt Request */
	SPP_DMA2_DMACINT_8B_tag DMACINT; /* offset: 0x001C size: 8 bit */
																	 /* SPP_DMA2_DMACERR - DMA Clear Error */
	SPP_DMA2_DMACERR_8B_tag DMACERR; /* offset: 0x001D size: 8 bit */
																	 /* SPP_DMA2_DMASSRT - DMA Set START Bit */
	SPP_DMA2_DMASSRT_8B_tag DMASSRT; /* offset: 0x001E size: 8 bit */
	/* SPP_DMA2_DMACDNE - DMA Clear DONE Status */
	SPP_DMA2_DMACDNE_8B_tag DMACDNE; /* offset: 0x001F size: 8 bit */
	/* SPP_DMA2_DMAINTH - DMA Interrupt Request Register */
	SPP_DMA2_DMAINTH_32B_tag DMAINTH; /* offset: 0x0020 size: 32 bit */
	/* SPP_DMA2_DMAINTL - DMA Interrupt Request Register */
	SPP_DMA2_DMAINTL_32B_tag DMAINTL; /* offset: 0x0024 size: 32 bit */
																		/* SPP_DMA2_DMAERRH - DMA Error Register */
	SPP_DMA2_DMAERRH_32B_tag DMAERRH; /* offset: 0x0028 size: 32 bit */
																		/* SPP_DMA2_DMAERRL - DMA Error Register */
	SPP_DMA2_DMAERRL_32B_tag DMAERRL; /* offset: 0x002C size: 32 bit */
	/* SPP_DMA2_DMAHRSH - DMA Hardware Request Status Register */
	SPP_DMA2_DMAHRSH_32B_tag DMAHRSH; /* offset: 0x0030 size: 32 bit */
	/* SPP_DMA2_DMAHRSL - DMA Hardware Request Status Register */
	SPP_DMA2_DMAHRSL_32B_tag DMAHRSL; /* offset: 0x0034 size: 32 bit */
	/* SPP_DMA2_DMAGPOR - DMA General Purpose Output Register */
	SPP_DMA2_DMAGPOR_32B_tag DMAGPOR; /* offset: 0x0038 size: 32 bit */
	int8_t SPP_DMA2_reserved_003C_C[196];
	union {
		/* SPP_DMA2_DCHPRIn - DMA Channel n Priority */
		SPP_DMA2_DCHPRI_8B_tag DCHPRI[64]; /* offset: 0x0100  (0x0001 x 64) */

		struct {
			/* SPP_DMA2_DCHPRIn - DMA Channel n Priority */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI0;	 /* offset: 0x0100 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI1;	 /* offset: 0x0101 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI2;	 /* offset: 0x0102 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI3;	 /* offset: 0x0103 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI4;	 /* offset: 0x0104 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI5;	 /* offset: 0x0105 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI6;	 /* offset: 0x0106 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI7;	 /* offset: 0x0107 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI8;	 /* offset: 0x0108 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI9;	 /* offset: 0x0109 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI10; /* offset: 0x010A size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI11; /* offset: 0x010B size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI12; /* offset: 0x010C size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI13; /* offset: 0x010D size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI14; /* offset: 0x010E size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI15; /* offset: 0x010F size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI16; /* offset: 0x0110 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI17; /* offset: 0x0111 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI18; /* offset: 0x0112 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI19; /* offset: 0x0113 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI20; /* offset: 0x0114 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI21; /* offset: 0x0115 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI22; /* offset: 0x0116 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI23; /* offset: 0x0117 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI24; /* offset: 0x0118 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI25; /* offset: 0x0119 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI26; /* offset: 0x011A size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI27; /* offset: 0x011B size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI28; /* offset: 0x011C size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI29; /* offset: 0x011D size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI30; /* offset: 0x011E size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI31; /* offset: 0x011F size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI32; /* offset: 0x0120 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI33; /* offset: 0x0121 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI34; /* offset: 0x0122 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI35; /* offset: 0x0123 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI36; /* offset: 0x0124 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI37; /* offset: 0x0125 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI38; /* offset: 0x0126 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI39; /* offset: 0x0127 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI40; /* offset: 0x0128 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI41; /* offset: 0x0129 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI42; /* offset: 0x012A size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI43; /* offset: 0x012B size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI44; /* offset: 0x012C size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI45; /* offset: 0x012D size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI46; /* offset: 0x012E size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI47; /* offset: 0x012F size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI48; /* offset: 0x0130 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI49; /* offset: 0x0131 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI50; /* offset: 0x0132 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI51; /* offset: 0x0133 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI52; /* offset: 0x0134 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI53; /* offset: 0x0135 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI54; /* offset: 0x0136 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI55; /* offset: 0x0137 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI56; /* offset: 0x0138 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI57; /* offset: 0x0139 size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI58; /* offset: 0x013A size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI59; /* offset: 0x013B size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI60; /* offset: 0x013C size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI61; /* offset: 0x013D size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI62; /* offset: 0x013E size: 8 bit */
			SPP_DMA2_DCHPRI_8B_tag DCHPRI63; /* offset: 0x013F size: 8 bit */
		};
	};
	int8_t SPP_DMA2_reserved_0140_C[3776];
	union {
		/*  Register set CHANNEL */
		SPP_DMA2_CHANNEL_tag CHANNEL[64]; /* offset: 0x1000  (0x0020 x 64) */

		struct {
			/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_0; /* offset: 0x1000 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_0; /* offset: 0x1004 size: 32 bit */
																						 /* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_0; /* offset: 0x1008 size: 32 bit */
																						 /* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag TCDWORD12_0; /* offset: 0x100C size: 32 bit */
																							 /* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag TCDWORD16_0; /* offset: 0x1010 size: 32 bit */
			/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag TCDWORD20_0; /* offset: 0x1014 size: 32 bit */
			/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag TCDWORD24_0; /* offset: 0x1018 size: 32 bit */
			/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag TCDWORD28_0; /* offset: 0x101C size: 32 bit */
			/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_1; /* offset: 0x1020 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_1; /* offset: 0x1024 size: 32 bit */
																						 /* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_1; /* offset: 0x1028 size: 32 bit */
																						 /* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag TCDWORD12_1; /* offset: 0x102C size: 32 bit */
																							 /* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag TCDWORD16_1; /* offset: 0x1030 size: 32 bit */
			/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag TCDWORD20_1; /* offset: 0x1034 size: 32 bit */
			/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag TCDWORD24_1; /* offset: 0x1038 size: 32 bit */
			/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag TCDWORD28_1; /* offset: 0x103C size: 32 bit */
			/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_2; /* offset: 0x1040 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_2; /* offset: 0x1044 size: 32 bit */
																						 /* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_2; /* offset: 0x1048 size: 32 bit */
																						 /* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag TCDWORD12_2; /* offset: 0x104C size: 32 bit */
																							 /* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag TCDWORD16_2; /* offset: 0x1050 size: 32 bit */
			/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag TCDWORD20_2; /* offset: 0x1054 size: 32 bit */
			/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag TCDWORD24_2; /* offset: 0x1058 size: 32 bit */
			/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag TCDWORD28_2; /* offset: 0x105C size: 32 bit */
			/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_3; /* offset: 0x1060 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_3; /* offset: 0x1064 size: 32 bit */
																						 /* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_3; /* offset: 0x1068 size: 32 bit */
																						 /* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag TCDWORD12_3; /* offset: 0x106C size: 32 bit */
																							 /* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag TCDWORD16_3; /* offset: 0x1070 size: 32 bit */
			/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag TCDWORD20_3; /* offset: 0x1074 size: 32 bit */
			/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag TCDWORD24_3; /* offset: 0x1078 size: 32 bit */
			/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag TCDWORD28_3; /* offset: 0x107C size: 32 bit */
			/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_4; /* offset: 0x1080 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_4; /* offset: 0x1084 size: 32 bit */
																						 /* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_4; /* offset: 0x1088 size: 32 bit */
																						 /* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag TCDWORD12_4; /* offset: 0x108C size: 32 bit */
																							 /* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag TCDWORD16_4; /* offset: 0x1090 size: 32 bit */
			/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag TCDWORD20_4; /* offset: 0x1094 size: 32 bit */
			/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag TCDWORD24_4; /* offset: 0x1098 size: 32 bit */
			/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag TCDWORD28_4; /* offset: 0x109C size: 32 bit */
			/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_5; /* offset: 0x10A0 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_5; /* offset: 0x10A4 size: 32 bit */
																						 /* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_5; /* offset: 0x10A8 size: 32 bit */
																						 /* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag TCDWORD12_5; /* offset: 0x10AC size: 32 bit */
																							 /* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag TCDWORD16_5; /* offset: 0x10B0 size: 32 bit */
			/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag TCDWORD20_5; /* offset: 0x10B4 size: 32 bit */
			/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag TCDWORD24_5; /* offset: 0x10B8 size: 32 bit */
			/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag TCDWORD28_5; /* offset: 0x10BC size: 32 bit */
			/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_6; /* offset: 0x10C0 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_6; /* offset: 0x10C4 size: 32 bit */
																						 /* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_6; /* offset: 0x10C8 size: 32 bit */
																						 /* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag TCDWORD12_6; /* offset: 0x10CC size: 32 bit */
																							 /* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag TCDWORD16_6; /* offset: 0x10D0 size: 32 bit */
			/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag TCDWORD20_6; /* offset: 0x10D4 size: 32 bit */
			/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag TCDWORD24_6; /* offset: 0x10D8 size: 32 bit */
			/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag TCDWORD28_6; /* offset: 0x10DC size: 32 bit */
			/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_7; /* offset: 0x10E0 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_7; /* offset: 0x10E4 size: 32 bit */
																						 /* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_7; /* offset: 0x10E8 size: 32 bit */
																						 /* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag TCDWORD12_7; /* offset: 0x10EC size: 32 bit */
																							 /* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag TCDWORD16_7; /* offset: 0x10F0 size: 32 bit */
			/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag TCDWORD20_7; /* offset: 0x10F4 size: 32 bit */
			/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag TCDWORD24_7; /* offset: 0x10F8 size: 32 bit */
			/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag TCDWORD28_7; /* offset: 0x10FC size: 32 bit */
			/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_8; /* offset: 0x1100 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_8; /* offset: 0x1104 size: 32 bit */
																						 /* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_8; /* offset: 0x1108 size: 32 bit */
																						 /* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag TCDWORD12_8; /* offset: 0x110C size: 32 bit */
																							 /* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag TCDWORD16_8; /* offset: 0x1110 size: 32 bit */
			/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag TCDWORD20_8; /* offset: 0x1114 size: 32 bit */
			/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag TCDWORD24_8; /* offset: 0x1118 size: 32 bit */
			/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag TCDWORD28_8; /* offset: 0x111C size: 32 bit */
			/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_9; /* offset: 0x1120 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_9; /* offset: 0x1124 size: 32 bit */
																						 /* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_9; /* offset: 0x1128 size: 32 bit */
																						 /* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag TCDWORD12_9; /* offset: 0x112C size: 32 bit */
																							 /* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag TCDWORD16_9; /* offset: 0x1130 size: 32 bit */
			/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag TCDWORD20_9; /* offset: 0x1134 size: 32 bit */
			/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag TCDWORD24_9; /* offset: 0x1138 size: 32 bit */
			/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag TCDWORD28_9; /* offset: 0x113C size: 32 bit */
			/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_10; /* offset: 0x1140 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_10; /* offset: 0x1144 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_10; /* offset: 0x1148 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_10; /* offset: 0x114C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_10; /* offset: 0x1150 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_10; /* offset: 0x1154 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_10; /* offset: 0x1158 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_10; /* offset: 0x115C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_11; /* offset: 0x1160 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_11; /* offset: 0x1164 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_11; /* offset: 0x1168 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_11; /* offset: 0x116C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_11; /* offset: 0x1170 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_11; /* offset: 0x1174 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_11; /* offset: 0x1178 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_11; /* offset: 0x117C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_12; /* offset: 0x1180 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_12; /* offset: 0x1184 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_12; /* offset: 0x1188 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_12; /* offset: 0x118C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_12; /* offset: 0x1190 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_12; /* offset: 0x1194 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_12; /* offset: 0x1198 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_12; /* offset: 0x119C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_13; /* offset: 0x11A0 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_13; /* offset: 0x11A4 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_13; /* offset: 0x11A8 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_13; /* offset: 0x11AC size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_13; /* offset: 0x11B0 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_13; /* offset: 0x11B4 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_13; /* offset: 0x11B8 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_13; /* offset: 0x11BC size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_14; /* offset: 0x11C0 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_14; /* offset: 0x11C4 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_14; /* offset: 0x11C8 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_14; /* offset: 0x11CC size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_14; /* offset: 0x11D0 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_14; /* offset: 0x11D4 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_14; /* offset: 0x11D8 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_14; /* offset: 0x11DC size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_15; /* offset: 0x11E0 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_15; /* offset: 0x11E4 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_15; /* offset: 0x11E8 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_15; /* offset: 0x11EC size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_15; /* offset: 0x11F0 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_15; /* offset: 0x11F4 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_15; /* offset: 0x11F8 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_15; /* offset: 0x11FC size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_16; /* offset: 0x1200 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_16; /* offset: 0x1204 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_16; /* offset: 0x1208 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_16; /* offset: 0x120C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_16; /* offset: 0x1210 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_16; /* offset: 0x1214 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_16; /* offset: 0x1218 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_16; /* offset: 0x121C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_17; /* offset: 0x1220 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_17; /* offset: 0x1224 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_17; /* offset: 0x1228 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_17; /* offset: 0x122C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_17; /* offset: 0x1230 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_17; /* offset: 0x1234 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_17; /* offset: 0x1238 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_17; /* offset: 0x123C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_18; /* offset: 0x1240 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_18; /* offset: 0x1244 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_18; /* offset: 0x1248 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_18; /* offset: 0x124C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_18; /* offset: 0x1250 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_18; /* offset: 0x1254 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_18; /* offset: 0x1258 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_18; /* offset: 0x125C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_19; /* offset: 0x1260 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_19; /* offset: 0x1264 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_19; /* offset: 0x1268 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_19; /* offset: 0x126C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_19; /* offset: 0x1270 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_19; /* offset: 0x1274 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_19; /* offset: 0x1278 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_19; /* offset: 0x127C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_20; /* offset: 0x1280 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_20; /* offset: 0x1284 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_20; /* offset: 0x1288 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_20; /* offset: 0x128C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_20; /* offset: 0x1290 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_20; /* offset: 0x1294 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_20; /* offset: 0x1298 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_20; /* offset: 0x129C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_21; /* offset: 0x12A0 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_21; /* offset: 0x12A4 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_21; /* offset: 0x12A8 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_21; /* offset: 0x12AC size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_21; /* offset: 0x12B0 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_21; /* offset: 0x12B4 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_21; /* offset: 0x12B8 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_21; /* offset: 0x12BC size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_22; /* offset: 0x12C0 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_22; /* offset: 0x12C4 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_22; /* offset: 0x12C8 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_22; /* offset: 0x12CC size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_22; /* offset: 0x12D0 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_22; /* offset: 0x12D4 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_22; /* offset: 0x12D8 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_22; /* offset: 0x12DC size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_23; /* offset: 0x12E0 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_23; /* offset: 0x12E4 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_23; /* offset: 0x12E8 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_23; /* offset: 0x12EC size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_23; /* offset: 0x12F0 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_23; /* offset: 0x12F4 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_23; /* offset: 0x12F8 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_23; /* offset: 0x12FC size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_24; /* offset: 0x1300 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_24; /* offset: 0x1304 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_24; /* offset: 0x1308 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_24; /* offset: 0x130C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_24; /* offset: 0x1310 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_24; /* offset: 0x1314 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_24; /* offset: 0x1318 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_24; /* offset: 0x131C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_25; /* offset: 0x1320 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_25; /* offset: 0x1324 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_25; /* offset: 0x1328 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_25; /* offset: 0x132C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_25; /* offset: 0x1330 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_25; /* offset: 0x1334 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_25; /* offset: 0x1338 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_25; /* offset: 0x133C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_26; /* offset: 0x1340 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_26; /* offset: 0x1344 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_26; /* offset: 0x1348 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_26; /* offset: 0x134C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_26; /* offset: 0x1350 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_26; /* offset: 0x1354 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_26; /* offset: 0x1358 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_26; /* offset: 0x135C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_27; /* offset: 0x1360 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_27; /* offset: 0x1364 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_27; /* offset: 0x1368 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_27; /* offset: 0x136C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_27; /* offset: 0x1370 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_27; /* offset: 0x1374 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_27; /* offset: 0x1378 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_27; /* offset: 0x137C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_28; /* offset: 0x1380 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_28; /* offset: 0x1384 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_28; /* offset: 0x1388 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_28; /* offset: 0x138C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_28; /* offset: 0x1390 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_28; /* offset: 0x1394 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_28; /* offset: 0x1398 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_28; /* offset: 0x139C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_29; /* offset: 0x13A0 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_29; /* offset: 0x13A4 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_29; /* offset: 0x13A8 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_29; /* offset: 0x13AC size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_29; /* offset: 0x13B0 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_29; /* offset: 0x13B4 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_29; /* offset: 0x13B8 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_29; /* offset: 0x13BC size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_30; /* offset: 0x13C0 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_30; /* offset: 0x13C4 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_30; /* offset: 0x13C8 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_30; /* offset: 0x13CC size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_30; /* offset: 0x13D0 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_30; /* offset: 0x13D4 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_30; /* offset: 0x13D8 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_30; /* offset: 0x13DC size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_31; /* offset: 0x13E0 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_31; /* offset: 0x13E4 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_31; /* offset: 0x13E8 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_31; /* offset: 0x13EC size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_31; /* offset: 0x13F0 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_31; /* offset: 0x13F4 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_31; /* offset: 0x13F8 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_31; /* offset: 0x13FC size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_32; /* offset: 0x1400 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_32; /* offset: 0x1404 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_32; /* offset: 0x1408 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_32; /* offset: 0x140C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_32; /* offset: 0x1410 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_32; /* offset: 0x1414 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_32; /* offset: 0x1418 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_32; /* offset: 0x141C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_33; /* offset: 0x1420 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_33; /* offset: 0x1424 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_33; /* offset: 0x1428 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_33; /* offset: 0x142C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_33; /* offset: 0x1430 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_33; /* offset: 0x1434 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_33; /* offset: 0x1438 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_33; /* offset: 0x143C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_34; /* offset: 0x1440 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_34; /* offset: 0x1444 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_34; /* offset: 0x1448 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_34; /* offset: 0x144C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_34; /* offset: 0x1450 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_34; /* offset: 0x1454 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_34; /* offset: 0x1458 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_34; /* offset: 0x145C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_35; /* offset: 0x1460 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_35; /* offset: 0x1464 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_35; /* offset: 0x1468 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_35; /* offset: 0x146C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_35; /* offset: 0x1470 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_35; /* offset: 0x1474 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_35; /* offset: 0x1478 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_35; /* offset: 0x147C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_36; /* offset: 0x1480 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_36; /* offset: 0x1484 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_36; /* offset: 0x1488 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_36; /* offset: 0x148C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_36; /* offset: 0x1490 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_36; /* offset: 0x1494 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_36; /* offset: 0x1498 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_36; /* offset: 0x149C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_37; /* offset: 0x14A0 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_37; /* offset: 0x14A4 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_37; /* offset: 0x14A8 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_37; /* offset: 0x14AC size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_37; /* offset: 0x14B0 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_37; /* offset: 0x14B4 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_37; /* offset: 0x14B8 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_37; /* offset: 0x14BC size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_38; /* offset: 0x14C0 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_38; /* offset: 0x14C4 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_38; /* offset: 0x14C8 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_38; /* offset: 0x14CC size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_38; /* offset: 0x14D0 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_38; /* offset: 0x14D4 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_38; /* offset: 0x14D8 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_38; /* offset: 0x14DC size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_39; /* offset: 0x14E0 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_39; /* offset: 0x14E4 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_39; /* offset: 0x14E8 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_39; /* offset: 0x14EC size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_39; /* offset: 0x14F0 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_39; /* offset: 0x14F4 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_39; /* offset: 0x14F8 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_39; /* offset: 0x14FC size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_40; /* offset: 0x1500 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_40; /* offset: 0x1504 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_40; /* offset: 0x1508 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_40; /* offset: 0x150C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_40; /* offset: 0x1510 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_40; /* offset: 0x1514 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_40; /* offset: 0x1518 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_40; /* offset: 0x151C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_41; /* offset: 0x1520 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_41; /* offset: 0x1524 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_41; /* offset: 0x1528 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_41; /* offset: 0x152C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_41; /* offset: 0x1530 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_41; /* offset: 0x1534 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_41; /* offset: 0x1538 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_41; /* offset: 0x153C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_42; /* offset: 0x1540 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_42; /* offset: 0x1544 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_42; /* offset: 0x1548 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_42; /* offset: 0x154C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_42; /* offset: 0x1550 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_42; /* offset: 0x1554 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_42; /* offset: 0x1558 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_42; /* offset: 0x155C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_43; /* offset: 0x1560 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_43; /* offset: 0x1564 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_43; /* offset: 0x1568 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_43; /* offset: 0x156C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_43; /* offset: 0x1570 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_43; /* offset: 0x1574 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_43; /* offset: 0x1578 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_43; /* offset: 0x157C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_44; /* offset: 0x1580 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_44; /* offset: 0x1584 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_44; /* offset: 0x1588 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_44; /* offset: 0x158C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_44; /* offset: 0x1590 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_44; /* offset: 0x1594 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_44; /* offset: 0x1598 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_44; /* offset: 0x159C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_45; /* offset: 0x15A0 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_45; /* offset: 0x15A4 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_45; /* offset: 0x15A8 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_45; /* offset: 0x15AC size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_45; /* offset: 0x15B0 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_45; /* offset: 0x15B4 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_45; /* offset: 0x15B8 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_45; /* offset: 0x15BC size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_46; /* offset: 0x15C0 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_46; /* offset: 0x15C4 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_46; /* offset: 0x15C8 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_46; /* offset: 0x15CC size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_46; /* offset: 0x15D0 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_46; /* offset: 0x15D4 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_46; /* offset: 0x15D8 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_46; /* offset: 0x15DC size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_47; /* offset: 0x15E0 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_47; /* offset: 0x15E4 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_47; /* offset: 0x15E8 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_47; /* offset: 0x15EC size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_47; /* offset: 0x15F0 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_47; /* offset: 0x15F4 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_47; /* offset: 0x15F8 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_47; /* offset: 0x15FC size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_48; /* offset: 0x1600 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_48; /* offset: 0x1604 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_48; /* offset: 0x1608 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_48; /* offset: 0x160C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_48; /* offset: 0x1610 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_48; /* offset: 0x1614 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_48; /* offset: 0x1618 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_48; /* offset: 0x161C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_49; /* offset: 0x1620 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_49; /* offset: 0x1624 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_49; /* offset: 0x1628 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_49; /* offset: 0x162C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_49; /* offset: 0x1630 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_49; /* offset: 0x1634 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_49; /* offset: 0x1638 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_49; /* offset: 0x163C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_50; /* offset: 0x1640 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_50; /* offset: 0x1644 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_50; /* offset: 0x1648 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_50; /* offset: 0x164C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_50; /* offset: 0x1650 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_50; /* offset: 0x1654 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_50; /* offset: 0x1658 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_50; /* offset: 0x165C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_51; /* offset: 0x1660 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_51; /* offset: 0x1664 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_51; /* offset: 0x1668 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_51; /* offset: 0x166C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_51; /* offset: 0x1670 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_51; /* offset: 0x1674 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_51; /* offset: 0x1678 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_51; /* offset: 0x167C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_52; /* offset: 0x1680 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_52; /* offset: 0x1684 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_52; /* offset: 0x1688 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_52; /* offset: 0x168C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_52; /* offset: 0x1690 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_52; /* offset: 0x1694 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_52; /* offset: 0x1698 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_52; /* offset: 0x169C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_53; /* offset: 0x16A0 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_53; /* offset: 0x16A4 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_53; /* offset: 0x16A8 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_53; /* offset: 0x16AC size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_53; /* offset: 0x16B0 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_53; /* offset: 0x16B4 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_53; /* offset: 0x16B8 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_53; /* offset: 0x16BC size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_54; /* offset: 0x16C0 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_54; /* offset: 0x16C4 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_54; /* offset: 0x16C8 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_54; /* offset: 0x16CC size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_54; /* offset: 0x16D0 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_54; /* offset: 0x16D4 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_54; /* offset: 0x16D8 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_54; /* offset: 0x16DC size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_55; /* offset: 0x16E0 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_55; /* offset: 0x16E4 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_55; /* offset: 0x16E8 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_55; /* offset: 0x16EC size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_55; /* offset: 0x16F0 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_55; /* offset: 0x16F4 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_55; /* offset: 0x16F8 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_55; /* offset: 0x16FC size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_56; /* offset: 0x1700 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_56; /* offset: 0x1704 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_56; /* offset: 0x1708 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_56; /* offset: 0x170C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_56; /* offset: 0x1710 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_56; /* offset: 0x1714 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_56; /* offset: 0x1718 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_56; /* offset: 0x171C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_57; /* offset: 0x1720 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_57; /* offset: 0x1724 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_57; /* offset: 0x1728 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_57; /* offset: 0x172C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_57; /* offset: 0x1730 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_57; /* offset: 0x1734 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_57; /* offset: 0x1738 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_57; /* offset: 0x173C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_58; /* offset: 0x1740 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_58; /* offset: 0x1744 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_58; /* offset: 0x1748 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_58; /* offset: 0x174C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_58; /* offset: 0x1750 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_58; /* offset: 0x1754 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_58; /* offset: 0x1758 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_58; /* offset: 0x175C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_59; /* offset: 0x1760 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_59; /* offset: 0x1764 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_59; /* offset: 0x1768 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_59; /* offset: 0x176C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_59; /* offset: 0x1770 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_59; /* offset: 0x1774 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_59; /* offset: 0x1778 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_59; /* offset: 0x177C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_60; /* offset: 0x1780 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_60; /* offset: 0x1784 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_60; /* offset: 0x1788 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_60; /* offset: 0x178C size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_60; /* offset: 0x1790 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_60; /* offset: 0x1794 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_60; /* offset: 0x1798 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_60; /* offset: 0x179C size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_61; /* offset: 0x17A0 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_61; /* offset: 0x17A4 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_61; /* offset: 0x17A8 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_61; /* offset: 0x17AC size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_61; /* offset: 0x17B0 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_61; /* offset: 0x17B4 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_61; /* offset: 0x17B8 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_61; /* offset: 0x17BC size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_62; /* offset: 0x17C0 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_62; /* offset: 0x17C4 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_62; /* offset: 0x17C8 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_62; /* offset: 0x17CC size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_62; /* offset: 0x17D0 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_62; /* offset: 0x17D4 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_62; /* offset: 0x17D8 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_62; /* offset: 0x17DC size: 32 bit */
												/* SPP_DMA2_TCDn Word0 - Source Address */
			SPP_DMA2_TCDWORD0__32B_tag TCDWORD0_63; /* offset: 0x17E0 size: 32 bit */
			/* SPP_DMA2_TCDn Word1 - smod, ssize, dmod, dsize, soff */
			SPP_DMA2_TCDWORD4__32B_tag TCDWORD4_63; /* offset: 0x17E4 size: 32 bit */
																							/* SPP_DMA2_TCDn Word2 - nbytes */
			SPP_DMA2_TCDWORD8__32B_tag TCDWORD8_63; /* offset: 0x17E8 size: 32 bit */
																							/* SPP_DMA2_TCDn Word3 - slast */
			SPP_DMA2_TCDWORD12__32B_tag
					TCDWORD12_63; /* offset: 0x17EC size: 32 bit */
												/* SPP_DMA2_TCDn Word4 - daddr */
			SPP_DMA2_TCDWORD16__32B_tag
					TCDWORD16_63; /* offset: 0x17F0 size: 32 bit */
												/* SPP_DMA2_TCDn Word5 -  citer.e_link, citer, doff */
			SPP_DMA2_TCDWORD20__32B_tag
					TCDWORD20_63; /* offset: 0x17F4 size: 32 bit */
												/* SPP_DMA2_TCDn Word6 -  dlast_sga */
			SPP_DMA2_TCDWORD24__32B_tag
					TCDWORD24_63; /* offset: 0x17F8 size: 32 bit */
												/* SPP_DMA2_TCDn Word7 -  biter, etc. */
			SPP_DMA2_TCDWORD28__32B_tag
					TCDWORD28_63; /* offset: 0x17FC size: 32 bit */
		};
	};
} SPP_DMA2_tag;

#define SPP_DMA2 (*(volatile SPP_DMA2_tag*)0xFFF44000UL)

/****************************************************************/
/*                                                              */
/* Module: INTC  */
/*                                                              */
/****************************************************************/

typedef union { /* BCR - Block Configuration Register */
	vuint32_t R;
	struct {
		vuint32_t : 18;
		vuint32_t VTES_PRC1 : 1; /* Vector Table Entry Size - Processor 1 */
		vuint32_t : 4;
		vuint32_t HVEN_PRC1 : 1; /* Hardware Vector Enable - Processor 1 */
		vuint32_t : 2;
#ifndef USE_FIELD_ALIASES_INTC
		vuint32_t VTES_PRC0 : 1; /* Vector Table Entry Size - Processor 0 */
#else
		vuint32_t VTES : 1;				 /* deprecated name - please avoid */
#endif
		vuint32_t : 4;
#ifndef USE_FIELD_ALIASES_INTC
		vuint32_t HVEN_PRC0 : 1; /* Hardware Vector Enable - Processor 0 */
#else
		vuint32_t HVEN : 1;				 /* deprecated name - please avoid */
#endif
	} B;
} INTC_BCR_32B_tag;

typedef union { /* CPR - Current Priority Register - Processor 0 */
	vuint32_t R;
	struct {
		vuint32_t : 28;
		vuint32_t PRI : 4; /* Priority Bits */
	} B;
} INTC_CPR_PRC0_32B_tag;

typedef union { /* CPR - Current Priority Register - Processor 1 */
	vuint32_t R;
	struct {
		vuint32_t : 28;
		vuint32_t PRI : 4; /* Priority Bits */
	} B;
} INTC_CPR_PRC1_32B_tag;

typedef union { /* IACKR- Interrupt Acknowledge Register - Processor 0 */
	vuint32_t R;
	struct {
#ifndef USE_FIELD_ALIASES_INTC
		vuint32_t VTBA_PRC0 : 21; /* Vector Table Base Address - Processor 0 */
#else
		vuint32_t VTBA : 21;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_INTC
		vuint32_t INTEC_PRC0 : 9; /* Interrupt Vector - Processor 0 */
#else
		vuint32_t INTVEC : 9;			 /* deprecated name - please avoid */
#endif
		vuint32_t : 2;
	} B;
} INTC_IACKR_PRC0_32B_tag;

typedef union { /* IACKR- Interrupt Acknowledge Register - Processor 1 */
	vuint32_t R;
	struct {
		vuint32_t VTBA_PRC1 : 21; /* Vector Table Base Address - Processor 1 */
		vuint32_t INTEC_PRC1 : 9; /* Interrupt Vector - Processor 1 */
		vuint32_t : 2;
	} B;
} INTC_IACKR_PRC1_32B_tag;

typedef union { /* EOIR- End of Interrupt Register - Processor 0 */
	vuint32_t R;
} INTC_EOIR_PRC0_32B_tag;

typedef union { /* EOIR- End of Interrupt Register - Processor 1 */
	vuint32_t R;
} INTC_EOIR_PRC1_32B_tag;

/* Register layout for all registers SSCIR... */

typedef union { /* SSCIR0-7 INTC Software Set/Clear Interrupt Registers */
	vuint8_t R;
	struct {
		vuint8_t : 6;
		vuint8_t SET : 1; /* Set Flag bit */
		vuint8_t CLR : 1; /* Clear Flag bit */
	} B;
} INTC_SSCIR_8B_tag;

typedef union { /* SSCIR0_3 - Software Set/Clear Interrupt Registers */
	vuint32_t R;
	struct {
		vuint32_t : 6;
		vuint32_t SET0 : 1; /* Set Flag 0 bit */
		vuint32_t CLR0 : 1; /* Clear Flag 0 bit */
		vuint32_t : 6;
		vuint32_t SET1 : 1; /* Set Flag 1 bit */
		vuint32_t CLR1 : 1; /* Clear Flag 1 bit */
		vuint32_t : 6;
		vuint32_t SET2 : 1; /* Set Flag 2 bit */
		vuint32_t CLR2 : 1; /* Clear Flag 2 bit */
		vuint32_t : 6;
		vuint32_t SET3 : 1; /* Set Flag 3 bit */
		vuint32_t CLR3 : 1; /* Clear Flag 3 bit */
	} B;
} INTC_SSCIR0_3_32B_tag;

typedef union { /* SSCIR4_7 - Software Set/Clear Interrupt Registers */
	vuint32_t R;
	struct {
		vuint32_t : 6;
		vuint32_t SET4 : 1; /* Set Flag 4 bit */
		vuint32_t CLR4 : 1; /* Clear Flag 4 bit */
		vuint32_t : 6;
		vuint32_t SET5 : 1; /* Set Flag 5 bit */
		vuint32_t CLR5 : 1; /* Clear Flag 5 bit */
		vuint32_t : 6;
		vuint32_t SET6 : 1; /* Set Flag 6 bit */
		vuint32_t CLR6 : 1; /* Clear Flag 6 bit */
		vuint32_t : 6;
		vuint32_t SET7 : 1; /* Set Flag 7 bit */
		vuint32_t CLR7 : 1; /* Clear Flag 7 bit */
	} B;
} INTC_SSCIR4_7_32B_tag;

/* Register layout for all registers PSR... */

typedef union { /* PSR0-511 - Priority Select Registers */
	vuint8_t R;
	struct {
		vuint8_t PRC_SEL : 2; /* Processor Select */
		vuint8_t : 2;
		vuint8_t PRI : 4; /* Priority Select */
	} B;
} INTC_PSR_8B_tag;

/* Register layout for all registers PSR... */

typedef union { /* PSR0_3 - 508_511 - Priority Select Registers */
	vuint32_t R;
	struct {
		vuint32_t PRC_SEL0 : 2; /* Processor Select - Entry 0 */
		vuint32_t : 2;
		vuint32_t PRI0 : 4;			/* Priority Select - Entry 0 */
		vuint32_t PRC_SEL1 : 2; /* Processor Select - Entry 1 */
		vuint32_t : 2;
		vuint32_t PRI1 : 4;			/* Priority Select - Entry 1 */
		vuint32_t PRC_SEL2 : 2; /* Processor Select - Entry 2 */
		vuint32_t : 2;
		vuint32_t PRI2 : 4;			/* Priority Select - Entry 2 */
		vuint32_t PRC_SEL3 : 2; /* Processor Select - Entry 3 */
		vuint32_t : 2;
		vuint32_t PRI3 : 4; /* Priority Select - Entry 3 */
	} B;
} INTC_PSR_32B_tag;

typedef struct INTC_struct_tag { /* start of INTC_tag */
	union {
		INTC_BCR_32B_tag MCR; /* deprecated - please avoid */

		/* BCR - Block Configuration Register */
		INTC_BCR_32B_tag BCR; /* offset: 0x0000 size: 32 bit */
	};
	int8_t INTC_reserved_0004_C[4];
	union {
		/* CPR - Current Priority Register - Processor 0 */
		INTC_CPR_PRC0_32B_tag CPR_PRC0; /* offset: 0x0008 size: 32 bit */

		INTC_CPR_PRC0_32B_tag CPR; /* deprecated - please avoid */
	};
	/* CPR - Current Priority Register - Processor 1 */
	INTC_CPR_PRC1_32B_tag CPR_PRC1; /* offset: 0x000C size: 32 bit */
	union {
		/* IACKR- Interrupt Acknowledge Register - Processor 0 */
		INTC_IACKR_PRC0_32B_tag IACKR_PRC0; /* offset: 0x0010 size: 32 bit */

		INTC_IACKR_PRC0_32B_tag IACKR; /* deprecated - please avoid */
	};
	/* IACKR- Interrupt Acknowledge Register - Processor 1 */
	INTC_IACKR_PRC1_32B_tag IACKR_PRC1; /* offset: 0x0014 size: 32 bit */
	union {
		/* EOIR- End of Interrupt Register - Processor 0 */
		INTC_EOIR_PRC0_32B_tag EOIR_PRC0; /* offset: 0x0018 size: 32 bit */

		INTC_EOIR_PRC0_32B_tag EOIR; /* deprecated - please avoid */
	};
	/* EOIR- End of Interrupt Register - Processor 1 */
	INTC_EOIR_PRC1_32B_tag EOIR_PRC1; /* offset: 0x001C size: 32 bit */
	union {
		/* SSCIR0-7 INTC Software Set/Clear Interrupt Registers */
		INTC_SSCIR_8B_tag SSCIR[8]; /* offset: 0x0020  (0x0001 x 8) */

		struct {
			/* SSCIR0-7 INTC Software Set/Clear Interrupt Registers */
			INTC_SSCIR_8B_tag SSCIR0; /* offset: 0x0020 size: 8 bit */
			INTC_SSCIR_8B_tag SSCIR1; /* offset: 0x0021 size: 8 bit */
			INTC_SSCIR_8B_tag SSCIR2; /* offset: 0x0022 size: 8 bit */
			INTC_SSCIR_8B_tag SSCIR3; /* offset: 0x0023 size: 8 bit */
			INTC_SSCIR_8B_tag SSCIR4; /* offset: 0x0024 size: 8 bit */
			INTC_SSCIR_8B_tag SSCIR5; /* offset: 0x0025 size: 8 bit */
			INTC_SSCIR_8B_tag SSCIR6; /* offset: 0x0026 size: 8 bit */
			INTC_SSCIR_8B_tag SSCIR7; /* offset: 0x0027 size: 8 bit */
		};

		struct {
			/* SSCIR0_3 - Software Set/Clear Interrupt Registers */
			INTC_SSCIR0_3_32B_tag SSCIR0_3; /* offset: 0x0020 size: 32 bit */
			/* SSCIR4_7 - Software Set/Clear Interrupt Registers */
			INTC_SSCIR4_7_32B_tag SSCIR4_7; /* offset: 0x0024 size: 32 bit */
		};
	};
	int8_t INTC_reserved_0028_C[24];
	union {
		/* PSR0_3 - 508_511 - Priority Select Registers */
		INTC_PSR_32B_tag PSR_32B[128]; /* offset: 0x0040  (0x0004 x 128) */

		/* PSR0-511 - Priority Select Registers */
		INTC_PSR_8B_tag PSR[512]; /* offset: 0x0040  (0x0001 x 512) */

		struct {
			/* PSR0_3 - 508_511 - Priority Select Registers */
			INTC_PSR_32B_tag PSR0_3;		 /* offset: 0x0040 size: 32 bit */
			INTC_PSR_32B_tag PSR4_7;		 /* offset: 0x0044 size: 32 bit */
			INTC_PSR_32B_tag PSR8_11;		 /* offset: 0x0048 size: 32 bit */
			INTC_PSR_32B_tag PSR12_15;	 /* offset: 0x004C size: 32 bit */
			INTC_PSR_32B_tag PSR16_19;	 /* offset: 0x0050 size: 32 bit */
			INTC_PSR_32B_tag PSR20_23;	 /* offset: 0x0054 size: 32 bit */
			INTC_PSR_32B_tag PSR24_27;	 /* offset: 0x0058 size: 32 bit */
			INTC_PSR_32B_tag PSR28_31;	 /* offset: 0x005C size: 32 bit */
			INTC_PSR_32B_tag PSR32_35;	 /* offset: 0x0060 size: 32 bit */
			INTC_PSR_32B_tag PSR36_39;	 /* offset: 0x0064 size: 32 bit */
			INTC_PSR_32B_tag PSR40_43;	 /* offset: 0x0068 size: 32 bit */
			INTC_PSR_32B_tag PSR44_47;	 /* offset: 0x006C size: 32 bit */
			INTC_PSR_32B_tag PSR48_51;	 /* offset: 0x0070 size: 32 bit */
			INTC_PSR_32B_tag PSR52_55;	 /* offset: 0x0074 size: 32 bit */
			INTC_PSR_32B_tag PSR56_59;	 /* offset: 0x0078 size: 32 bit */
			INTC_PSR_32B_tag PSR60_63;	 /* offset: 0x007C size: 32 bit */
			INTC_PSR_32B_tag PSR64_67;	 /* offset: 0x0080 size: 32 bit */
			INTC_PSR_32B_tag PSR68_71;	 /* offset: 0x0084 size: 32 bit */
			INTC_PSR_32B_tag PSR72_75;	 /* offset: 0x0088 size: 32 bit */
			INTC_PSR_32B_tag PSR76_79;	 /* offset: 0x008C size: 32 bit */
			INTC_PSR_32B_tag PSR80_83;	 /* offset: 0x0090 size: 32 bit */
			INTC_PSR_32B_tag PSR84_87;	 /* offset: 0x0094 size: 32 bit */
			INTC_PSR_32B_tag PSR88_91;	 /* offset: 0x0098 size: 32 bit */
			INTC_PSR_32B_tag PSR92_95;	 /* offset: 0x009C size: 32 bit */
			INTC_PSR_32B_tag PSR96_99;	 /* offset: 0x00A0 size: 32 bit */
			INTC_PSR_32B_tag PSR100_103; /* offset: 0x00A4 size: 32 bit */
			INTC_PSR_32B_tag PSR104_107; /* offset: 0x00A8 size: 32 bit */
			INTC_PSR_32B_tag PSR108_111; /* offset: 0x00AC size: 32 bit */
			INTC_PSR_32B_tag PSR112_115; /* offset: 0x00B0 size: 32 bit */
			INTC_PSR_32B_tag PSR116_119; /* offset: 0x00B4 size: 32 bit */
			INTC_PSR_32B_tag PSR120_123; /* offset: 0x00B8 size: 32 bit */
			INTC_PSR_32B_tag PSR124_127; /* offset: 0x00BC size: 32 bit */
			INTC_PSR_32B_tag PSR128_131; /* offset: 0x00C0 size: 32 bit */
			INTC_PSR_32B_tag PSR132_135; /* offset: 0x00C4 size: 32 bit */
			INTC_PSR_32B_tag PSR136_139; /* offset: 0x00C8 size: 32 bit */
			INTC_PSR_32B_tag PSR140_143; /* offset: 0x00CC size: 32 bit */
			INTC_PSR_32B_tag PSR144_147; /* offset: 0x00D0 size: 32 bit */
			INTC_PSR_32B_tag PSR148_151; /* offset: 0x00D4 size: 32 bit */
			INTC_PSR_32B_tag PSR152_155; /* offset: 0x00D8 size: 32 bit */
			INTC_PSR_32B_tag PSR156_159; /* offset: 0x00DC size: 32 bit */
			INTC_PSR_32B_tag PSR160_163; /* offset: 0x00E0 size: 32 bit */
			INTC_PSR_32B_tag PSR164_167; /* offset: 0x00E4 size: 32 bit */
			INTC_PSR_32B_tag PSR168_171; /* offset: 0x00E8 size: 32 bit */
			INTC_PSR_32B_tag PSR172_175; /* offset: 0x00EC size: 32 bit */
			INTC_PSR_32B_tag PSR176_179; /* offset: 0x00F0 size: 32 bit */
			INTC_PSR_32B_tag PSR180_183; /* offset: 0x00F4 size: 32 bit */
			INTC_PSR_32B_tag PSR184_187; /* offset: 0x00F8 size: 32 bit */
			INTC_PSR_32B_tag PSR188_191; /* offset: 0x00FC size: 32 bit */
			INTC_PSR_32B_tag PSR192_195; /* offset: 0x0100 size: 32 bit */
			INTC_PSR_32B_tag PSR196_199; /* offset: 0x0104 size: 32 bit */
			INTC_PSR_32B_tag PSR200_203; /* offset: 0x0108 size: 32 bit */
			INTC_PSR_32B_tag PSR204_207; /* offset: 0x010C size: 32 bit */
			INTC_PSR_32B_tag PSR208_211; /* offset: 0x0110 size: 32 bit */
			INTC_PSR_32B_tag PSR212_215; /* offset: 0x0114 size: 32 bit */
			INTC_PSR_32B_tag PSR216_219; /* offset: 0x0118 size: 32 bit */
			INTC_PSR_32B_tag PSR220_223; /* offset: 0x011C size: 32 bit */
			INTC_PSR_32B_tag PSR224_227; /* offset: 0x0120 size: 32 bit */
			INTC_PSR_32B_tag PSR228_231; /* offset: 0x0124 size: 32 bit */
			INTC_PSR_32B_tag PSR232_235; /* offset: 0x0128 size: 32 bit */
			INTC_PSR_32B_tag PSR236_239; /* offset: 0x012C size: 32 bit */
			INTC_PSR_32B_tag PSR240_243; /* offset: 0x0130 size: 32 bit */
			INTC_PSR_32B_tag PSR244_247; /* offset: 0x0134 size: 32 bit */
			INTC_PSR_32B_tag PSR248_251; /* offset: 0x0138 size: 32 bit */
			INTC_PSR_32B_tag PSR252_255; /* offset: 0x013C size: 32 bit */
			INTC_PSR_32B_tag PSR256_259; /* offset: 0x0140 size: 32 bit */
			INTC_PSR_32B_tag PSR260_263; /* offset: 0x0144 size: 32 bit */
			INTC_PSR_32B_tag PSR264_267; /* offset: 0x0148 size: 32 bit */
			INTC_PSR_32B_tag PSR268_271; /* offset: 0x014C size: 32 bit */
			INTC_PSR_32B_tag PSR272_275; /* offset: 0x0150 size: 32 bit */
			INTC_PSR_32B_tag PSR276_279; /* offset: 0x0154 size: 32 bit */
			INTC_PSR_32B_tag PSR280_283; /* offset: 0x0158 size: 32 bit */
			INTC_PSR_32B_tag PSR284_287; /* offset: 0x015C size: 32 bit */
			INTC_PSR_32B_tag PSR288_291; /* offset: 0x0160 size: 32 bit */
			INTC_PSR_32B_tag PSR292_295; /* offset: 0x0164 size: 32 bit */
			INTC_PSR_32B_tag PSR296_299; /* offset: 0x0168 size: 32 bit */
			INTC_PSR_32B_tag PSR300_303; /* offset: 0x016C size: 32 bit */
			INTC_PSR_32B_tag PSR304_307; /* offset: 0x0170 size: 32 bit */
			INTC_PSR_32B_tag PSR308_311; /* offset: 0x0174 size: 32 bit */
			INTC_PSR_32B_tag PSR312_315; /* offset: 0x0178 size: 32 bit */
			INTC_PSR_32B_tag PSR316_319; /* offset: 0x017C size: 32 bit */
			INTC_PSR_32B_tag PSR320_323; /* offset: 0x0180 size: 32 bit */
			INTC_PSR_32B_tag PSR324_327; /* offset: 0x0184 size: 32 bit */
			INTC_PSR_32B_tag PSR328_331; /* offset: 0x0188 size: 32 bit */
			INTC_PSR_32B_tag PSR332_335; /* offset: 0x018C size: 32 bit */
			INTC_PSR_32B_tag PSR336_339; /* offset: 0x0190 size: 32 bit */
			INTC_PSR_32B_tag PSR340_343; /* offset: 0x0194 size: 32 bit */
			INTC_PSR_32B_tag PSR344_347; /* offset: 0x0198 size: 32 bit */
			INTC_PSR_32B_tag PSR348_351; /* offset: 0x019C size: 32 bit */
			INTC_PSR_32B_tag PSR352_355; /* offset: 0x01A0 size: 32 bit */
			INTC_PSR_32B_tag PSR356_359; /* offset: 0x01A4 size: 32 bit */
			INTC_PSR_32B_tag PSR360_363; /* offset: 0x01A8 size: 32 bit */
			INTC_PSR_32B_tag PSR364_367; /* offset: 0x01AC size: 32 bit */
			INTC_PSR_32B_tag PSR368_371; /* offset: 0x01B0 size: 32 bit */
			INTC_PSR_32B_tag PSR372_375; /* offset: 0x01B4 size: 32 bit */
			INTC_PSR_32B_tag PSR376_379; /* offset: 0x01B8 size: 32 bit */
			INTC_PSR_32B_tag PSR380_383; /* offset: 0x01BC size: 32 bit */
			INTC_PSR_32B_tag PSR384_387; /* offset: 0x01C0 size: 32 bit */
			INTC_PSR_32B_tag PSR388_391; /* offset: 0x01C4 size: 32 bit */
			INTC_PSR_32B_tag PSR392_395; /* offset: 0x01C8 size: 32 bit */
			INTC_PSR_32B_tag PSR396_399; /* offset: 0x01CC size: 32 bit */
			INTC_PSR_32B_tag PSR400_403; /* offset: 0x01D0 size: 32 bit */
			INTC_PSR_32B_tag PSR404_407; /* offset: 0x01D4 size: 32 bit */
			INTC_PSR_32B_tag PSR408_411; /* offset: 0x01D8 size: 32 bit */
			INTC_PSR_32B_tag PSR412_415; /* offset: 0x01DC size: 32 bit */
			INTC_PSR_32B_tag PSR416_419; /* offset: 0x01E0 size: 32 bit */
			INTC_PSR_32B_tag PSR420_423; /* offset: 0x01E4 size: 32 bit */
			INTC_PSR_32B_tag PSR424_427; /* offset: 0x01E8 size: 32 bit */
			INTC_PSR_32B_tag PSR428_431; /* offset: 0x01EC size: 32 bit */
			INTC_PSR_32B_tag PSR432_435; /* offset: 0x01F0 size: 32 bit */
			INTC_PSR_32B_tag PSR436_439; /* offset: 0x01F4 size: 32 bit */
			INTC_PSR_32B_tag PSR440_443; /* offset: 0x01F8 size: 32 bit */
			INTC_PSR_32B_tag PSR444_447; /* offset: 0x01FC size: 32 bit */
			INTC_PSR_32B_tag PSR448_451; /* offset: 0x0200 size: 32 bit */
			INTC_PSR_32B_tag PSR452_455; /* offset: 0x0204 size: 32 bit */
			INTC_PSR_32B_tag PSR456_459; /* offset: 0x0208 size: 32 bit */
			INTC_PSR_32B_tag PSR460_463; /* offset: 0x020C size: 32 bit */
			INTC_PSR_32B_tag PSR464_467; /* offset: 0x0210 size: 32 bit */
			INTC_PSR_32B_tag PSR468_471; /* offset: 0x0214 size: 32 bit */
			INTC_PSR_32B_tag PSR472_475; /* offset: 0x0218 size: 32 bit */
			INTC_PSR_32B_tag PSR476_479; /* offset: 0x021C size: 32 bit */
			INTC_PSR_32B_tag PSR480_483; /* offset: 0x0220 size: 32 bit */
			INTC_PSR_32B_tag PSR484_487; /* offset: 0x0224 size: 32 bit */
			INTC_PSR_32B_tag PSR488_491; /* offset: 0x0228 size: 32 bit */
			INTC_PSR_32B_tag PSR492_495; /* offset: 0x022C size: 32 bit */
			INTC_PSR_32B_tag PSR496_499; /* offset: 0x0230 size: 32 bit */
			INTC_PSR_32B_tag PSR500_503; /* offset: 0x0234 size: 32 bit */
			INTC_PSR_32B_tag PSR504_507; /* offset: 0x0238 size: 32 bit */
			INTC_PSR_32B_tag PSR508_511; /* offset: 0x023C size: 32 bit */
		};

		struct {
			/* PSR0-511 - Priority Select Registers */
			INTC_PSR_8B_tag PSR0;		/* offset: 0x0040 size: 8 bit */
			INTC_PSR_8B_tag PSR1;		/* offset: 0x0041 size: 8 bit */
			INTC_PSR_8B_tag PSR2;		/* offset: 0x0042 size: 8 bit */
			INTC_PSR_8B_tag PSR3;		/* offset: 0x0043 size: 8 bit */
			INTC_PSR_8B_tag PSR4;		/* offset: 0x0044 size: 8 bit */
			INTC_PSR_8B_tag PSR5;		/* offset: 0x0045 size: 8 bit */
			INTC_PSR_8B_tag PSR6;		/* offset: 0x0046 size: 8 bit */
			INTC_PSR_8B_tag PSR7;		/* offset: 0x0047 size: 8 bit */
			INTC_PSR_8B_tag PSR8;		/* offset: 0x0048 size: 8 bit */
			INTC_PSR_8B_tag PSR9;		/* offset: 0x0049 size: 8 bit */
			INTC_PSR_8B_tag PSR10;	/* offset: 0x004A size: 8 bit */
			INTC_PSR_8B_tag PSR11;	/* offset: 0x004B size: 8 bit */
			INTC_PSR_8B_tag PSR12;	/* offset: 0x004C size: 8 bit */
			INTC_PSR_8B_tag PSR13;	/* offset: 0x004D size: 8 bit */
			INTC_PSR_8B_tag PSR14;	/* offset: 0x004E size: 8 bit */
			INTC_PSR_8B_tag PSR15;	/* offset: 0x004F size: 8 bit */
			INTC_PSR_8B_tag PSR16;	/* offset: 0x0050 size: 8 bit */
			INTC_PSR_8B_tag PSR17;	/* offset: 0x0051 size: 8 bit */
			INTC_PSR_8B_tag PSR18;	/* offset: 0x0052 size: 8 bit */
			INTC_PSR_8B_tag PSR19;	/* offset: 0x0053 size: 8 bit */
			INTC_PSR_8B_tag PSR20;	/* offset: 0x0054 size: 8 bit */
			INTC_PSR_8B_tag PSR21;	/* offset: 0x0055 size: 8 bit */
			INTC_PSR_8B_tag PSR22;	/* offset: 0x0056 size: 8 bit */
			INTC_PSR_8B_tag PSR23;	/* offset: 0x0057 size: 8 bit */
			INTC_PSR_8B_tag PSR24;	/* offset: 0x0058 size: 8 bit */
			INTC_PSR_8B_tag PSR25;	/* offset: 0x0059 size: 8 bit */
			INTC_PSR_8B_tag PSR26;	/* offset: 0x005A size: 8 bit */
			INTC_PSR_8B_tag PSR27;	/* offset: 0x005B size: 8 bit */
			INTC_PSR_8B_tag PSR28;	/* offset: 0x005C size: 8 bit */
			INTC_PSR_8B_tag PSR29;	/* offset: 0x005D size: 8 bit */
			INTC_PSR_8B_tag PSR30;	/* offset: 0x005E size: 8 bit */
			INTC_PSR_8B_tag PSR31;	/* offset: 0x005F size: 8 bit */
			INTC_PSR_8B_tag PSR32;	/* offset: 0x0060 size: 8 bit */
			INTC_PSR_8B_tag PSR33;	/* offset: 0x0061 size: 8 bit */
			INTC_PSR_8B_tag PSR34;	/* offset: 0x0062 size: 8 bit */
			INTC_PSR_8B_tag PSR35;	/* offset: 0x0063 size: 8 bit */
			INTC_PSR_8B_tag PSR36;	/* offset: 0x0064 size: 8 bit */
			INTC_PSR_8B_tag PSR37;	/* offset: 0x0065 size: 8 bit */
			INTC_PSR_8B_tag PSR38;	/* offset: 0x0066 size: 8 bit */
			INTC_PSR_8B_tag PSR39;	/* offset: 0x0067 size: 8 bit */
			INTC_PSR_8B_tag PSR40;	/* offset: 0x0068 size: 8 bit */
			INTC_PSR_8B_tag PSR41;	/* offset: 0x0069 size: 8 bit */
			INTC_PSR_8B_tag PSR42;	/* offset: 0x006A size: 8 bit */
			INTC_PSR_8B_tag PSR43;	/* offset: 0x006B size: 8 bit */
			INTC_PSR_8B_tag PSR44;	/* offset: 0x006C size: 8 bit */
			INTC_PSR_8B_tag PSR45;	/* offset: 0x006D size: 8 bit */
			INTC_PSR_8B_tag PSR46;	/* offset: 0x006E size: 8 bit */
			INTC_PSR_8B_tag PSR47;	/* offset: 0x006F size: 8 bit */
			INTC_PSR_8B_tag PSR48;	/* offset: 0x0070 size: 8 bit */
			INTC_PSR_8B_tag PSR49;	/* offset: 0x0071 size: 8 bit */
			INTC_PSR_8B_tag PSR50;	/* offset: 0x0072 size: 8 bit */
			INTC_PSR_8B_tag PSR51;	/* offset: 0x0073 size: 8 bit */
			INTC_PSR_8B_tag PSR52;	/* offset: 0x0074 size: 8 bit */
			INTC_PSR_8B_tag PSR53;	/* offset: 0x0075 size: 8 bit */
			INTC_PSR_8B_tag PSR54;	/* offset: 0x0076 size: 8 bit */
			INTC_PSR_8B_tag PSR55;	/* offset: 0x0077 size: 8 bit */
			INTC_PSR_8B_tag PSR56;	/* offset: 0x0078 size: 8 bit */
			INTC_PSR_8B_tag PSR57;	/* offset: 0x0079 size: 8 bit */
			INTC_PSR_8B_tag PSR58;	/* offset: 0x007A size: 8 bit */
			INTC_PSR_8B_tag PSR59;	/* offset: 0x007B size: 8 bit */
			INTC_PSR_8B_tag PSR60;	/* offset: 0x007C size: 8 bit */
			INTC_PSR_8B_tag PSR61;	/* offset: 0x007D size: 8 bit */
			INTC_PSR_8B_tag PSR62;	/* offset: 0x007E size: 8 bit */
			INTC_PSR_8B_tag PSR63;	/* offset: 0x007F size: 8 bit */
			INTC_PSR_8B_tag PSR64;	/* offset: 0x0080 size: 8 bit */
			INTC_PSR_8B_tag PSR65;	/* offset: 0x0081 size: 8 bit */
			INTC_PSR_8B_tag PSR66;	/* offset: 0x0082 size: 8 bit */
			INTC_PSR_8B_tag PSR67;	/* offset: 0x0083 size: 8 bit */
			INTC_PSR_8B_tag PSR68;	/* offset: 0x0084 size: 8 bit */
			INTC_PSR_8B_tag PSR69;	/* offset: 0x0085 size: 8 bit */
			INTC_PSR_8B_tag PSR70;	/* offset: 0x0086 size: 8 bit */
			INTC_PSR_8B_tag PSR71;	/* offset: 0x0087 size: 8 bit */
			INTC_PSR_8B_tag PSR72;	/* offset: 0x0088 size: 8 bit */
			INTC_PSR_8B_tag PSR73;	/* offset: 0x0089 size: 8 bit */
			INTC_PSR_8B_tag PSR74;	/* offset: 0x008A size: 8 bit */
			INTC_PSR_8B_tag PSR75;	/* offset: 0x008B size: 8 bit */
			INTC_PSR_8B_tag PSR76;	/* offset: 0x008C size: 8 bit */
			INTC_PSR_8B_tag PSR77;	/* offset: 0x008D size: 8 bit */
			INTC_PSR_8B_tag PSR78;	/* offset: 0x008E size: 8 bit */
			INTC_PSR_8B_tag PSR79;	/* offset: 0x008F size: 8 bit */
			INTC_PSR_8B_tag PSR80;	/* offset: 0x0090 size: 8 bit */
			INTC_PSR_8B_tag PSR81;	/* offset: 0x0091 size: 8 bit */
			INTC_PSR_8B_tag PSR82;	/* offset: 0x0092 size: 8 bit */
			INTC_PSR_8B_tag PSR83;	/* offset: 0x0093 size: 8 bit */
			INTC_PSR_8B_tag PSR84;	/* offset: 0x0094 size: 8 bit */
			INTC_PSR_8B_tag PSR85;	/* offset: 0x0095 size: 8 bit */
			INTC_PSR_8B_tag PSR86;	/* offset: 0x0096 size: 8 bit */
			INTC_PSR_8B_tag PSR87;	/* offset: 0x0097 size: 8 bit */
			INTC_PSR_8B_tag PSR88;	/* offset: 0x0098 size: 8 bit */
			INTC_PSR_8B_tag PSR89;	/* offset: 0x0099 size: 8 bit */
			INTC_PSR_8B_tag PSR90;	/* offset: 0x009A size: 8 bit */
			INTC_PSR_8B_tag PSR91;	/* offset: 0x009B size: 8 bit */
			INTC_PSR_8B_tag PSR92;	/* offset: 0x009C size: 8 bit */
			INTC_PSR_8B_tag PSR93;	/* offset: 0x009D size: 8 bit */
			INTC_PSR_8B_tag PSR94;	/* offset: 0x009E size: 8 bit */
			INTC_PSR_8B_tag PSR95;	/* offset: 0x009F size: 8 bit */
			INTC_PSR_8B_tag PSR96;	/* offset: 0x00A0 size: 8 bit */
			INTC_PSR_8B_tag PSR97;	/* offset: 0x00A1 size: 8 bit */
			INTC_PSR_8B_tag PSR98;	/* offset: 0x00A2 size: 8 bit */
			INTC_PSR_8B_tag PSR99;	/* offset: 0x00A3 size: 8 bit */
			INTC_PSR_8B_tag PSR100; /* offset: 0x00A4 size: 8 bit */
			INTC_PSR_8B_tag PSR101; /* offset: 0x00A5 size: 8 bit */
			INTC_PSR_8B_tag PSR102; /* offset: 0x00A6 size: 8 bit */
			INTC_PSR_8B_tag PSR103; /* offset: 0x00A7 size: 8 bit */
			INTC_PSR_8B_tag PSR104; /* offset: 0x00A8 size: 8 bit */
			INTC_PSR_8B_tag PSR105; /* offset: 0x00A9 size: 8 bit */
			INTC_PSR_8B_tag PSR106; /* offset: 0x00AA size: 8 bit */
			INTC_PSR_8B_tag PSR107; /* offset: 0x00AB size: 8 bit */
			INTC_PSR_8B_tag PSR108; /* offset: 0x00AC size: 8 bit */
			INTC_PSR_8B_tag PSR109; /* offset: 0x00AD size: 8 bit */
			INTC_PSR_8B_tag PSR110; /* offset: 0x00AE size: 8 bit */
			INTC_PSR_8B_tag PSR111; /* offset: 0x00AF size: 8 bit */
			INTC_PSR_8B_tag PSR112; /* offset: 0x00B0 size: 8 bit */
			INTC_PSR_8B_tag PSR113; /* offset: 0x00B1 size: 8 bit */
			INTC_PSR_8B_tag PSR114; /* offset: 0x00B2 size: 8 bit */
			INTC_PSR_8B_tag PSR115; /* offset: 0x00B3 size: 8 bit */
			INTC_PSR_8B_tag PSR116; /* offset: 0x00B4 size: 8 bit */
			INTC_PSR_8B_tag PSR117; /* offset: 0x00B5 size: 8 bit */
			INTC_PSR_8B_tag PSR118; /* offset: 0x00B6 size: 8 bit */
			INTC_PSR_8B_tag PSR119; /* offset: 0x00B7 size: 8 bit */
			INTC_PSR_8B_tag PSR120; /* offset: 0x00B8 size: 8 bit */
			INTC_PSR_8B_tag PSR121; /* offset: 0x00B9 size: 8 bit */
			INTC_PSR_8B_tag PSR122; /* offset: 0x00BA size: 8 bit */
			INTC_PSR_8B_tag PSR123; /* offset: 0x00BB size: 8 bit */
			INTC_PSR_8B_tag PSR124; /* offset: 0x00BC size: 8 bit */
			INTC_PSR_8B_tag PSR125; /* offset: 0x00BD size: 8 bit */
			INTC_PSR_8B_tag PSR126; /* offset: 0x00BE size: 8 bit */
			INTC_PSR_8B_tag PSR127; /* offset: 0x00BF size: 8 bit */
			INTC_PSR_8B_tag PSR128; /* offset: 0x00C0 size: 8 bit */
			INTC_PSR_8B_tag PSR129; /* offset: 0x00C1 size: 8 bit */
			INTC_PSR_8B_tag PSR130; /* offset: 0x00C2 size: 8 bit */
			INTC_PSR_8B_tag PSR131; /* offset: 0x00C3 size: 8 bit */
			INTC_PSR_8B_tag PSR132; /* offset: 0x00C4 size: 8 bit */
			INTC_PSR_8B_tag PSR133; /* offset: 0x00C5 size: 8 bit */
			INTC_PSR_8B_tag PSR134; /* offset: 0x00C6 size: 8 bit */
			INTC_PSR_8B_tag PSR135; /* offset: 0x00C7 size: 8 bit */
			INTC_PSR_8B_tag PSR136; /* offset: 0x00C8 size: 8 bit */
			INTC_PSR_8B_tag PSR137; /* offset: 0x00C9 size: 8 bit */
			INTC_PSR_8B_tag PSR138; /* offset: 0x00CA size: 8 bit */
			INTC_PSR_8B_tag PSR139; /* offset: 0x00CB size: 8 bit */
			INTC_PSR_8B_tag PSR140; /* offset: 0x00CC size: 8 bit */
			INTC_PSR_8B_tag PSR141; /* offset: 0x00CD size: 8 bit */
			INTC_PSR_8B_tag PSR142; /* offset: 0x00CE size: 8 bit */
			INTC_PSR_8B_tag PSR143; /* offset: 0x00CF size: 8 bit */
			INTC_PSR_8B_tag PSR144; /* offset: 0x00D0 size: 8 bit */
			INTC_PSR_8B_tag PSR145; /* offset: 0x00D1 size: 8 bit */
			INTC_PSR_8B_tag PSR146; /* offset: 0x00D2 size: 8 bit */
			INTC_PSR_8B_tag PSR147; /* offset: 0x00D3 size: 8 bit */
			INTC_PSR_8B_tag PSR148; /* offset: 0x00D4 size: 8 bit */
			INTC_PSR_8B_tag PSR149; /* offset: 0x00D5 size: 8 bit */
			INTC_PSR_8B_tag PSR150; /* offset: 0x00D6 size: 8 bit */
			INTC_PSR_8B_tag PSR151; /* offset: 0x00D7 size: 8 bit */
			INTC_PSR_8B_tag PSR152; /* offset: 0x00D8 size: 8 bit */
			INTC_PSR_8B_tag PSR153; /* offset: 0x00D9 size: 8 bit */
			INTC_PSR_8B_tag PSR154; /* offset: 0x00DA size: 8 bit */
			INTC_PSR_8B_tag PSR155; /* offset: 0x00DB size: 8 bit */
			INTC_PSR_8B_tag PSR156; /* offset: 0x00DC size: 8 bit */
			INTC_PSR_8B_tag PSR157; /* offset: 0x00DD size: 8 bit */
			INTC_PSR_8B_tag PSR158; /* offset: 0x00DE size: 8 bit */
			INTC_PSR_8B_tag PSR159; /* offset: 0x00DF size: 8 bit */
			INTC_PSR_8B_tag PSR160; /* offset: 0x00E0 size: 8 bit */
			INTC_PSR_8B_tag PSR161; /* offset: 0x00E1 size: 8 bit */
			INTC_PSR_8B_tag PSR162; /* offset: 0x00E2 size: 8 bit */
			INTC_PSR_8B_tag PSR163; /* offset: 0x00E3 size: 8 bit */
			INTC_PSR_8B_tag PSR164; /* offset: 0x00E4 size: 8 bit */
			INTC_PSR_8B_tag PSR165; /* offset: 0x00E5 size: 8 bit */
			INTC_PSR_8B_tag PSR166; /* offset: 0x00E6 size: 8 bit */
			INTC_PSR_8B_tag PSR167; /* offset: 0x00E7 size: 8 bit */
			INTC_PSR_8B_tag PSR168; /* offset: 0x00E8 size: 8 bit */
			INTC_PSR_8B_tag PSR169; /* offset: 0x00E9 size: 8 bit */
			INTC_PSR_8B_tag PSR170; /* offset: 0x00EA size: 8 bit */
			INTC_PSR_8B_tag PSR171; /* offset: 0x00EB size: 8 bit */
			INTC_PSR_8B_tag PSR172; /* offset: 0x00EC size: 8 bit */
			INTC_PSR_8B_tag PSR173; /* offset: 0x00ED size: 8 bit */
			INTC_PSR_8B_tag PSR174; /* offset: 0x00EE size: 8 bit */
			INTC_PSR_8B_tag PSR175; /* offset: 0x00EF size: 8 bit */
			INTC_PSR_8B_tag PSR176; /* offset: 0x00F0 size: 8 bit */
			INTC_PSR_8B_tag PSR177; /* offset: 0x00F1 size: 8 bit */
			INTC_PSR_8B_tag PSR178; /* offset: 0x00F2 size: 8 bit */
			INTC_PSR_8B_tag PSR179; /* offset: 0x00F3 size: 8 bit */
			INTC_PSR_8B_tag PSR180; /* offset: 0x00F4 size: 8 bit */
			INTC_PSR_8B_tag PSR181; /* offset: 0x00F5 size: 8 bit */
			INTC_PSR_8B_tag PSR182; /* offset: 0x00F6 size: 8 bit */
			INTC_PSR_8B_tag PSR183; /* offset: 0x00F7 size: 8 bit */
			INTC_PSR_8B_tag PSR184; /* offset: 0x00F8 size: 8 bit */
			INTC_PSR_8B_tag PSR185; /* offset: 0x00F9 size: 8 bit */
			INTC_PSR_8B_tag PSR186; /* offset: 0x00FA size: 8 bit */
			INTC_PSR_8B_tag PSR187; /* offset: 0x00FB size: 8 bit */
			INTC_PSR_8B_tag PSR188; /* offset: 0x00FC size: 8 bit */
			INTC_PSR_8B_tag PSR189; /* offset: 0x00FD size: 8 bit */
			INTC_PSR_8B_tag PSR190; /* offset: 0x00FE size: 8 bit */
			INTC_PSR_8B_tag PSR191; /* offset: 0x00FF size: 8 bit */
			INTC_PSR_8B_tag PSR192; /* offset: 0x0100 size: 8 bit */
			INTC_PSR_8B_tag PSR193; /* offset: 0x0101 size: 8 bit */
			INTC_PSR_8B_tag PSR194; /* offset: 0x0102 size: 8 bit */
			INTC_PSR_8B_tag PSR195; /* offset: 0x0103 size: 8 bit */
			INTC_PSR_8B_tag PSR196; /* offset: 0x0104 size: 8 bit */
			INTC_PSR_8B_tag PSR197; /* offset: 0x0105 size: 8 bit */
			INTC_PSR_8B_tag PSR198; /* offset: 0x0106 size: 8 bit */
			INTC_PSR_8B_tag PSR199; /* offset: 0x0107 size: 8 bit */
			INTC_PSR_8B_tag PSR200; /* offset: 0x0108 size: 8 bit */
			INTC_PSR_8B_tag PSR201; /* offset: 0x0109 size: 8 bit */
			INTC_PSR_8B_tag PSR202; /* offset: 0x010A size: 8 bit */
			INTC_PSR_8B_tag PSR203; /* offset: 0x010B size: 8 bit */
			INTC_PSR_8B_tag PSR204; /* offset: 0x010C size: 8 bit */
			INTC_PSR_8B_tag PSR205; /* offset: 0x010D size: 8 bit */
			INTC_PSR_8B_tag PSR206; /* offset: 0x010E size: 8 bit */
			INTC_PSR_8B_tag PSR207; /* offset: 0x010F size: 8 bit */
			INTC_PSR_8B_tag PSR208; /* offset: 0x0110 size: 8 bit */
			INTC_PSR_8B_tag PSR209; /* offset: 0x0111 size: 8 bit */
			INTC_PSR_8B_tag PSR210; /* offset: 0x0112 size: 8 bit */
			INTC_PSR_8B_tag PSR211; /* offset: 0x0113 size: 8 bit */
			INTC_PSR_8B_tag PSR212; /* offset: 0x0114 size: 8 bit */
			INTC_PSR_8B_tag PSR213; /* offset: 0x0115 size: 8 bit */
			INTC_PSR_8B_tag PSR214; /* offset: 0x0116 size: 8 bit */
			INTC_PSR_8B_tag PSR215; /* offset: 0x0117 size: 8 bit */
			INTC_PSR_8B_tag PSR216; /* offset: 0x0118 size: 8 bit */
			INTC_PSR_8B_tag PSR217; /* offset: 0x0119 size: 8 bit */
			INTC_PSR_8B_tag PSR218; /* offset: 0x011A size: 8 bit */
			INTC_PSR_8B_tag PSR219; /* offset: 0x011B size: 8 bit */
			INTC_PSR_8B_tag PSR220; /* offset: 0x011C size: 8 bit */
			INTC_PSR_8B_tag PSR221; /* offset: 0x011D size: 8 bit */
			INTC_PSR_8B_tag PSR222; /* offset: 0x011E size: 8 bit */
			INTC_PSR_8B_tag PSR223; /* offset: 0x011F size: 8 bit */
			INTC_PSR_8B_tag PSR224; /* offset: 0x0120 size: 8 bit */
			INTC_PSR_8B_tag PSR225; /* offset: 0x0121 size: 8 bit */
			INTC_PSR_8B_tag PSR226; /* offset: 0x0122 size: 8 bit */
			INTC_PSR_8B_tag PSR227; /* offset: 0x0123 size: 8 bit */
			INTC_PSR_8B_tag PSR228; /* offset: 0x0124 size: 8 bit */
			INTC_PSR_8B_tag PSR229; /* offset: 0x0125 size: 8 bit */
			INTC_PSR_8B_tag PSR230; /* offset: 0x0126 size: 8 bit */
			INTC_PSR_8B_tag PSR231; /* offset: 0x0127 size: 8 bit */
			INTC_PSR_8B_tag PSR232; /* offset: 0x0128 size: 8 bit */
			INTC_PSR_8B_tag PSR233; /* offset: 0x0129 size: 8 bit */
			INTC_PSR_8B_tag PSR234; /* offset: 0x012A size: 8 bit */
			INTC_PSR_8B_tag PSR235; /* offset: 0x012B size: 8 bit */
			INTC_PSR_8B_tag PSR236; /* offset: 0x012C size: 8 bit */
			INTC_PSR_8B_tag PSR237; /* offset: 0x012D size: 8 bit */
			INTC_PSR_8B_tag PSR238; /* offset: 0x012E size: 8 bit */
			INTC_PSR_8B_tag PSR239; /* offset: 0x012F size: 8 bit */
			INTC_PSR_8B_tag PSR240; /* offset: 0x0130 size: 8 bit */
			INTC_PSR_8B_tag PSR241; /* offset: 0x0131 size: 8 bit */
			INTC_PSR_8B_tag PSR242; /* offset: 0x0132 size: 8 bit */
			INTC_PSR_8B_tag PSR243; /* offset: 0x0133 size: 8 bit */
			INTC_PSR_8B_tag PSR244; /* offset: 0x0134 size: 8 bit */
			INTC_PSR_8B_tag PSR245; /* offset: 0x0135 size: 8 bit */
			INTC_PSR_8B_tag PSR246; /* offset: 0x0136 size: 8 bit */
			INTC_PSR_8B_tag PSR247; /* offset: 0x0137 size: 8 bit */
			INTC_PSR_8B_tag PSR248; /* offset: 0x0138 size: 8 bit */
			INTC_PSR_8B_tag PSR249; /* offset: 0x0139 size: 8 bit */
			INTC_PSR_8B_tag PSR250; /* offset: 0x013A size: 8 bit */
			INTC_PSR_8B_tag PSR251; /* offset: 0x013B size: 8 bit */
			INTC_PSR_8B_tag PSR252; /* offset: 0x013C size: 8 bit */
			INTC_PSR_8B_tag PSR253; /* offset: 0x013D size: 8 bit */
			INTC_PSR_8B_tag PSR254; /* offset: 0x013E size: 8 bit */
			INTC_PSR_8B_tag PSR255; /* offset: 0x013F size: 8 bit */
			INTC_PSR_8B_tag PSR256; /* offset: 0x0140 size: 8 bit */
			INTC_PSR_8B_tag PSR257; /* offset: 0x0141 size: 8 bit */
			INTC_PSR_8B_tag PSR258; /* offset: 0x0142 size: 8 bit */
			INTC_PSR_8B_tag PSR259; /* offset: 0x0143 size: 8 bit */
			INTC_PSR_8B_tag PSR260; /* offset: 0x0144 size: 8 bit */
			INTC_PSR_8B_tag PSR261; /* offset: 0x0145 size: 8 bit */
			INTC_PSR_8B_tag PSR262; /* offset: 0x0146 size: 8 bit */
			INTC_PSR_8B_tag PSR263; /* offset: 0x0147 size: 8 bit */
			INTC_PSR_8B_tag PSR264; /* offset: 0x0148 size: 8 bit */
			INTC_PSR_8B_tag PSR265; /* offset: 0x0149 size: 8 bit */
			INTC_PSR_8B_tag PSR266; /* offset: 0x014A size: 8 bit */
			INTC_PSR_8B_tag PSR267; /* offset: 0x014B size: 8 bit */
			INTC_PSR_8B_tag PSR268; /* offset: 0x014C size: 8 bit */
			INTC_PSR_8B_tag PSR269; /* offset: 0x014D size: 8 bit */
			INTC_PSR_8B_tag PSR270; /* offset: 0x014E size: 8 bit */
			INTC_PSR_8B_tag PSR271; /* offset: 0x014F size: 8 bit */
			INTC_PSR_8B_tag PSR272; /* offset: 0x0150 size: 8 bit */
			INTC_PSR_8B_tag PSR273; /* offset: 0x0151 size: 8 bit */
			INTC_PSR_8B_tag PSR274; /* offset: 0x0152 size: 8 bit */
			INTC_PSR_8B_tag PSR275; /* offset: 0x0153 size: 8 bit */
			INTC_PSR_8B_tag PSR276; /* offset: 0x0154 size: 8 bit */
			INTC_PSR_8B_tag PSR277; /* offset: 0x0155 size: 8 bit */
			INTC_PSR_8B_tag PSR278; /* offset: 0x0156 size: 8 bit */
			INTC_PSR_8B_tag PSR279; /* offset: 0x0157 size: 8 bit */
			INTC_PSR_8B_tag PSR280; /* offset: 0x0158 size: 8 bit */
			INTC_PSR_8B_tag PSR281; /* offset: 0x0159 size: 8 bit */
			INTC_PSR_8B_tag PSR282; /* offset: 0x015A size: 8 bit */
			INTC_PSR_8B_tag PSR283; /* offset: 0x015B size: 8 bit */
			INTC_PSR_8B_tag PSR284; /* offset: 0x015C size: 8 bit */
			INTC_PSR_8B_tag PSR285; /* offset: 0x015D size: 8 bit */
			INTC_PSR_8B_tag PSR286; /* offset: 0x015E size: 8 bit */
			INTC_PSR_8B_tag PSR287; /* offset: 0x015F size: 8 bit */
			INTC_PSR_8B_tag PSR288; /* offset: 0x0160 size: 8 bit */
			INTC_PSR_8B_tag PSR289; /* offset: 0x0161 size: 8 bit */
			INTC_PSR_8B_tag PSR290; /* offset: 0x0162 size: 8 bit */
			INTC_PSR_8B_tag PSR291; /* offset: 0x0163 size: 8 bit */
			INTC_PSR_8B_tag PSR292; /* offset: 0x0164 size: 8 bit */
			INTC_PSR_8B_tag PSR293; /* offset: 0x0165 size: 8 bit */
			INTC_PSR_8B_tag PSR294; /* offset: 0x0166 size: 8 bit */
			INTC_PSR_8B_tag PSR295; /* offset: 0x0167 size: 8 bit */
			INTC_PSR_8B_tag PSR296; /* offset: 0x0168 size: 8 bit */
			INTC_PSR_8B_tag PSR297; /* offset: 0x0169 size: 8 bit */
			INTC_PSR_8B_tag PSR298; /* offset: 0x016A size: 8 bit */
			INTC_PSR_8B_tag PSR299; /* offset: 0x016B size: 8 bit */
			INTC_PSR_8B_tag PSR300; /* offset: 0x016C size: 8 bit */
			INTC_PSR_8B_tag PSR301; /* offset: 0x016D size: 8 bit */
			INTC_PSR_8B_tag PSR302; /* offset: 0x016E size: 8 bit */
			INTC_PSR_8B_tag PSR303; /* offset: 0x016F size: 8 bit */
			INTC_PSR_8B_tag PSR304; /* offset: 0x0170 size: 8 bit */
			INTC_PSR_8B_tag PSR305; /* offset: 0x0171 size: 8 bit */
			INTC_PSR_8B_tag PSR306; /* offset: 0x0172 size: 8 bit */
			INTC_PSR_8B_tag PSR307; /* offset: 0x0173 size: 8 bit */
			INTC_PSR_8B_tag PSR308; /* offset: 0x0174 size: 8 bit */
			INTC_PSR_8B_tag PSR309; /* offset: 0x0175 size: 8 bit */
			INTC_PSR_8B_tag PSR310; /* offset: 0x0176 size: 8 bit */
			INTC_PSR_8B_tag PSR311; /* offset: 0x0177 size: 8 bit */
			INTC_PSR_8B_tag PSR312; /* offset: 0x0178 size: 8 bit */
			INTC_PSR_8B_tag PSR313; /* offset: 0x0179 size: 8 bit */
			INTC_PSR_8B_tag PSR314; /* offset: 0x017A size: 8 bit */
			INTC_PSR_8B_tag PSR315; /* offset: 0x017B size: 8 bit */
			INTC_PSR_8B_tag PSR316; /* offset: 0x017C size: 8 bit */
			INTC_PSR_8B_tag PSR317; /* offset: 0x017D size: 8 bit */
			INTC_PSR_8B_tag PSR318; /* offset: 0x017E size: 8 bit */
			INTC_PSR_8B_tag PSR319; /* offset: 0x017F size: 8 bit */
			INTC_PSR_8B_tag PSR320; /* offset: 0x0180 size: 8 bit */
			INTC_PSR_8B_tag PSR321; /* offset: 0x0181 size: 8 bit */
			INTC_PSR_8B_tag PSR322; /* offset: 0x0182 size: 8 bit */
			INTC_PSR_8B_tag PSR323; /* offset: 0x0183 size: 8 bit */
			INTC_PSR_8B_tag PSR324; /* offset: 0x0184 size: 8 bit */
			INTC_PSR_8B_tag PSR325; /* offset: 0x0185 size: 8 bit */
			INTC_PSR_8B_tag PSR326; /* offset: 0x0186 size: 8 bit */
			INTC_PSR_8B_tag PSR327; /* offset: 0x0187 size: 8 bit */
			INTC_PSR_8B_tag PSR328; /* offset: 0x0188 size: 8 bit */
			INTC_PSR_8B_tag PSR329; /* offset: 0x0189 size: 8 bit */
			INTC_PSR_8B_tag PSR330; /* offset: 0x018A size: 8 bit */
			INTC_PSR_8B_tag PSR331; /* offset: 0x018B size: 8 bit */
			INTC_PSR_8B_tag PSR332; /* offset: 0x018C size: 8 bit */
			INTC_PSR_8B_tag PSR333; /* offset: 0x018D size: 8 bit */
			INTC_PSR_8B_tag PSR334; /* offset: 0x018E size: 8 bit */
			INTC_PSR_8B_tag PSR335; /* offset: 0x018F size: 8 bit */
			INTC_PSR_8B_tag PSR336; /* offset: 0x0190 size: 8 bit */
			INTC_PSR_8B_tag PSR337; /* offset: 0x0191 size: 8 bit */
			INTC_PSR_8B_tag PSR338; /* offset: 0x0192 size: 8 bit */
			INTC_PSR_8B_tag PSR339; /* offset: 0x0193 size: 8 bit */
			INTC_PSR_8B_tag PSR340; /* offset: 0x0194 size: 8 bit */
			INTC_PSR_8B_tag PSR341; /* offset: 0x0195 size: 8 bit */
			INTC_PSR_8B_tag PSR342; /* offset: 0x0196 size: 8 bit */
			INTC_PSR_8B_tag PSR343; /* offset: 0x0197 size: 8 bit */
			INTC_PSR_8B_tag PSR344; /* offset: 0x0198 size: 8 bit */
			INTC_PSR_8B_tag PSR345; /* offset: 0x0199 size: 8 bit */
			INTC_PSR_8B_tag PSR346; /* offset: 0x019A size: 8 bit */
			INTC_PSR_8B_tag PSR347; /* offset: 0x019B size: 8 bit */
			INTC_PSR_8B_tag PSR348; /* offset: 0x019C size: 8 bit */
			INTC_PSR_8B_tag PSR349; /* offset: 0x019D size: 8 bit */
			INTC_PSR_8B_tag PSR350; /* offset: 0x019E size: 8 bit */
			INTC_PSR_8B_tag PSR351; /* offset: 0x019F size: 8 bit */
			INTC_PSR_8B_tag PSR352; /* offset: 0x01A0 size: 8 bit */
			INTC_PSR_8B_tag PSR353; /* offset: 0x01A1 size: 8 bit */
			INTC_PSR_8B_tag PSR354; /* offset: 0x01A2 size: 8 bit */
			INTC_PSR_8B_tag PSR355; /* offset: 0x01A3 size: 8 bit */
			INTC_PSR_8B_tag PSR356; /* offset: 0x01A4 size: 8 bit */
			INTC_PSR_8B_tag PSR357; /* offset: 0x01A5 size: 8 bit */
			INTC_PSR_8B_tag PSR358; /* offset: 0x01A6 size: 8 bit */
			INTC_PSR_8B_tag PSR359; /* offset: 0x01A7 size: 8 bit */
			INTC_PSR_8B_tag PSR360; /* offset: 0x01A8 size: 8 bit */
			INTC_PSR_8B_tag PSR361; /* offset: 0x01A9 size: 8 bit */
			INTC_PSR_8B_tag PSR362; /* offset: 0x01AA size: 8 bit */
			INTC_PSR_8B_tag PSR363; /* offset: 0x01AB size: 8 bit */
			INTC_PSR_8B_tag PSR364; /* offset: 0x01AC size: 8 bit */
			INTC_PSR_8B_tag PSR365; /* offset: 0x01AD size: 8 bit */
			INTC_PSR_8B_tag PSR366; /* offset: 0x01AE size: 8 bit */
			INTC_PSR_8B_tag PSR367; /* offset: 0x01AF size: 8 bit */
			INTC_PSR_8B_tag PSR368; /* offset: 0x01B0 size: 8 bit */
			INTC_PSR_8B_tag PSR369; /* offset: 0x01B1 size: 8 bit */
			INTC_PSR_8B_tag PSR370; /* offset: 0x01B2 size: 8 bit */
			INTC_PSR_8B_tag PSR371; /* offset: 0x01B3 size: 8 bit */
			INTC_PSR_8B_tag PSR372; /* offset: 0x01B4 size: 8 bit */
			INTC_PSR_8B_tag PSR373; /* offset: 0x01B5 size: 8 bit */
			INTC_PSR_8B_tag PSR374; /* offset: 0x01B6 size: 8 bit */
			INTC_PSR_8B_tag PSR375; /* offset: 0x01B7 size: 8 bit */
			INTC_PSR_8B_tag PSR376; /* offset: 0x01B8 size: 8 bit */
			INTC_PSR_8B_tag PSR377; /* offset: 0x01B9 size: 8 bit */
			INTC_PSR_8B_tag PSR378; /* offset: 0x01BA size: 8 bit */
			INTC_PSR_8B_tag PSR379; /* offset: 0x01BB size: 8 bit */
			INTC_PSR_8B_tag PSR380; /* offset: 0x01BC size: 8 bit */
			INTC_PSR_8B_tag PSR381; /* offset: 0x01BD size: 8 bit */
			INTC_PSR_8B_tag PSR382; /* offset: 0x01BE size: 8 bit */
			INTC_PSR_8B_tag PSR383; /* offset: 0x01BF size: 8 bit */
			INTC_PSR_8B_tag PSR384; /* offset: 0x01C0 size: 8 bit */
			INTC_PSR_8B_tag PSR385; /* offset: 0x01C1 size: 8 bit */
			INTC_PSR_8B_tag PSR386; /* offset: 0x01C2 size: 8 bit */
			INTC_PSR_8B_tag PSR387; /* offset: 0x01C3 size: 8 bit */
			INTC_PSR_8B_tag PSR388; /* offset: 0x01C4 size: 8 bit */
			INTC_PSR_8B_tag PSR389; /* offset: 0x01C5 size: 8 bit */
			INTC_PSR_8B_tag PSR390; /* offset: 0x01C6 size: 8 bit */
			INTC_PSR_8B_tag PSR391; /* offset: 0x01C7 size: 8 bit */
			INTC_PSR_8B_tag PSR392; /* offset: 0x01C8 size: 8 bit */
			INTC_PSR_8B_tag PSR393; /* offset: 0x01C9 size: 8 bit */
			INTC_PSR_8B_tag PSR394; /* offset: 0x01CA size: 8 bit */
			INTC_PSR_8B_tag PSR395; /* offset: 0x01CB size: 8 bit */
			INTC_PSR_8B_tag PSR396; /* offset: 0x01CC size: 8 bit */
			INTC_PSR_8B_tag PSR397; /* offset: 0x01CD size: 8 bit */
			INTC_PSR_8B_tag PSR398; /* offset: 0x01CE size: 8 bit */
			INTC_PSR_8B_tag PSR399; /* offset: 0x01CF size: 8 bit */
			INTC_PSR_8B_tag PSR400; /* offset: 0x01D0 size: 8 bit */
			INTC_PSR_8B_tag PSR401; /* offset: 0x01D1 size: 8 bit */
			INTC_PSR_8B_tag PSR402; /* offset: 0x01D2 size: 8 bit */
			INTC_PSR_8B_tag PSR403; /* offset: 0x01D3 size: 8 bit */
			INTC_PSR_8B_tag PSR404; /* offset: 0x01D4 size: 8 bit */
			INTC_PSR_8B_tag PSR405; /* offset: 0x01D5 size: 8 bit */
			INTC_PSR_8B_tag PSR406; /* offset: 0x01D6 size: 8 bit */
			INTC_PSR_8B_tag PSR407; /* offset: 0x01D7 size: 8 bit */
			INTC_PSR_8B_tag PSR408; /* offset: 0x01D8 size: 8 bit */
			INTC_PSR_8B_tag PSR409; /* offset: 0x01D9 size: 8 bit */
			INTC_PSR_8B_tag PSR410; /* offset: 0x01DA size: 8 bit */
			INTC_PSR_8B_tag PSR411; /* offset: 0x01DB size: 8 bit */
			INTC_PSR_8B_tag PSR412; /* offset: 0x01DC size: 8 bit */
			INTC_PSR_8B_tag PSR413; /* offset: 0x01DD size: 8 bit */
			INTC_PSR_8B_tag PSR414; /* offset: 0x01DE size: 8 bit */
			INTC_PSR_8B_tag PSR415; /* offset: 0x01DF size: 8 bit */
			INTC_PSR_8B_tag PSR416; /* offset: 0x01E0 size: 8 bit */
			INTC_PSR_8B_tag PSR417; /* offset: 0x01E1 size: 8 bit */
			INTC_PSR_8B_tag PSR418; /* offset: 0x01E2 size: 8 bit */
			INTC_PSR_8B_tag PSR419; /* offset: 0x01E3 size: 8 bit */
			INTC_PSR_8B_tag PSR420; /* offset: 0x01E4 size: 8 bit */
			INTC_PSR_8B_tag PSR421; /* offset: 0x01E5 size: 8 bit */
			INTC_PSR_8B_tag PSR422; /* offset: 0x01E6 size: 8 bit */
			INTC_PSR_8B_tag PSR423; /* offset: 0x01E7 size: 8 bit */
			INTC_PSR_8B_tag PSR424; /* offset: 0x01E8 size: 8 bit */
			INTC_PSR_8B_tag PSR425; /* offset: 0x01E9 size: 8 bit */
			INTC_PSR_8B_tag PSR426; /* offset: 0x01EA size: 8 bit */
			INTC_PSR_8B_tag PSR427; /* offset: 0x01EB size: 8 bit */
			INTC_PSR_8B_tag PSR428; /* offset: 0x01EC size: 8 bit */
			INTC_PSR_8B_tag PSR429; /* offset: 0x01ED size: 8 bit */
			INTC_PSR_8B_tag PSR430; /* offset: 0x01EE size: 8 bit */
			INTC_PSR_8B_tag PSR431; /* offset: 0x01EF size: 8 bit */
			INTC_PSR_8B_tag PSR432; /* offset: 0x01F0 size: 8 bit */
			INTC_PSR_8B_tag PSR433; /* offset: 0x01F1 size: 8 bit */
			INTC_PSR_8B_tag PSR434; /* offset: 0x01F2 size: 8 bit */
			INTC_PSR_8B_tag PSR435; /* offset: 0x01F3 size: 8 bit */
			INTC_PSR_8B_tag PSR436; /* offset: 0x01F4 size: 8 bit */
			INTC_PSR_8B_tag PSR437; /* offset: 0x01F5 size: 8 bit */
			INTC_PSR_8B_tag PSR438; /* offset: 0x01F6 size: 8 bit */
			INTC_PSR_8B_tag PSR439; /* offset: 0x01F7 size: 8 bit */
			INTC_PSR_8B_tag PSR440; /* offset: 0x01F8 size: 8 bit */
			INTC_PSR_8B_tag PSR441; /* offset: 0x01F9 size: 8 bit */
			INTC_PSR_8B_tag PSR442; /* offset: 0x01FA size: 8 bit */
			INTC_PSR_8B_tag PSR443; /* offset: 0x01FB size: 8 bit */
			INTC_PSR_8B_tag PSR444; /* offset: 0x01FC size: 8 bit */
			INTC_PSR_8B_tag PSR445; /* offset: 0x01FD size: 8 bit */
			INTC_PSR_8B_tag PSR446; /* offset: 0x01FE size: 8 bit */
			INTC_PSR_8B_tag PSR447; /* offset: 0x01FF size: 8 bit */
			INTC_PSR_8B_tag PSR448; /* offset: 0x0200 size: 8 bit */
			INTC_PSR_8B_tag PSR449; /* offset: 0x0201 size: 8 bit */
			INTC_PSR_8B_tag PSR450; /* offset: 0x0202 size: 8 bit */
			INTC_PSR_8B_tag PSR451; /* offset: 0x0203 size: 8 bit */
			INTC_PSR_8B_tag PSR452; /* offset: 0x0204 size: 8 bit */
			INTC_PSR_8B_tag PSR453; /* offset: 0x0205 size: 8 bit */
			INTC_PSR_8B_tag PSR454; /* offset: 0x0206 size: 8 bit */
			INTC_PSR_8B_tag PSR455; /* offset: 0x0207 size: 8 bit */
			INTC_PSR_8B_tag PSR456; /* offset: 0x0208 size: 8 bit */
			INTC_PSR_8B_tag PSR457; /* offset: 0x0209 size: 8 bit */
			INTC_PSR_8B_tag PSR458; /* offset: 0x020A size: 8 bit */
			INTC_PSR_8B_tag PSR459; /* offset: 0x020B size: 8 bit */
			INTC_PSR_8B_tag PSR460; /* offset: 0x020C size: 8 bit */
			INTC_PSR_8B_tag PSR461; /* offset: 0x020D size: 8 bit */
			INTC_PSR_8B_tag PSR462; /* offset: 0x020E size: 8 bit */
			INTC_PSR_8B_tag PSR463; /* offset: 0x020F size: 8 bit */
			INTC_PSR_8B_tag PSR464; /* offset: 0x0210 size: 8 bit */
			INTC_PSR_8B_tag PSR465; /* offset: 0x0211 size: 8 bit */
			INTC_PSR_8B_tag PSR466; /* offset: 0x0212 size: 8 bit */
			INTC_PSR_8B_tag PSR467; /* offset: 0x0213 size: 8 bit */
			INTC_PSR_8B_tag PSR468; /* offset: 0x0214 size: 8 bit */
			INTC_PSR_8B_tag PSR469; /* offset: 0x0215 size: 8 bit */
			INTC_PSR_8B_tag PSR470; /* offset: 0x0216 size: 8 bit */
			INTC_PSR_8B_tag PSR471; /* offset: 0x0217 size: 8 bit */
			INTC_PSR_8B_tag PSR472; /* offset: 0x0218 size: 8 bit */
			INTC_PSR_8B_tag PSR473; /* offset: 0x0219 size: 8 bit */
			INTC_PSR_8B_tag PSR474; /* offset: 0x021A size: 8 bit */
			INTC_PSR_8B_tag PSR475; /* offset: 0x021B size: 8 bit */
			INTC_PSR_8B_tag PSR476; /* offset: 0x021C size: 8 bit */
			INTC_PSR_8B_tag PSR477; /* offset: 0x021D size: 8 bit */
			INTC_PSR_8B_tag PSR478; /* offset: 0x021E size: 8 bit */
			INTC_PSR_8B_tag PSR479; /* offset: 0x021F size: 8 bit */
			INTC_PSR_8B_tag PSR480; /* offset: 0x0220 size: 8 bit */
			INTC_PSR_8B_tag PSR481; /* offset: 0x0221 size: 8 bit */
			INTC_PSR_8B_tag PSR482; /* offset: 0x0222 size: 8 bit */
			INTC_PSR_8B_tag PSR483; /* offset: 0x0223 size: 8 bit */
			INTC_PSR_8B_tag PSR484; /* offset: 0x0224 size: 8 bit */
			INTC_PSR_8B_tag PSR485; /* offset: 0x0225 size: 8 bit */
			INTC_PSR_8B_tag PSR486; /* offset: 0x0226 size: 8 bit */
			INTC_PSR_8B_tag PSR487; /* offset: 0x0227 size: 8 bit */
			INTC_PSR_8B_tag PSR488; /* offset: 0x0228 size: 8 bit */
			INTC_PSR_8B_tag PSR489; /* offset: 0x0229 size: 8 bit */
			INTC_PSR_8B_tag PSR490; /* offset: 0x022A size: 8 bit */
			INTC_PSR_8B_tag PSR491; /* offset: 0x022B size: 8 bit */
			INTC_PSR_8B_tag PSR492; /* offset: 0x022C size: 8 bit */
			INTC_PSR_8B_tag PSR493; /* offset: 0x022D size: 8 bit */
			INTC_PSR_8B_tag PSR494; /* offset: 0x022E size: 8 bit */
			INTC_PSR_8B_tag PSR495; /* offset: 0x022F size: 8 bit */
			INTC_PSR_8B_tag PSR496; /* offset: 0x0230 size: 8 bit */
			INTC_PSR_8B_tag PSR497; /* offset: 0x0231 size: 8 bit */
			INTC_PSR_8B_tag PSR498; /* offset: 0x0232 size: 8 bit */
			INTC_PSR_8B_tag PSR499; /* offset: 0x0233 size: 8 bit */
			INTC_PSR_8B_tag PSR500; /* offset: 0x0234 size: 8 bit */
			INTC_PSR_8B_tag PSR501; /* offset: 0x0235 size: 8 bit */
			INTC_PSR_8B_tag PSR502; /* offset: 0x0236 size: 8 bit */
			INTC_PSR_8B_tag PSR503; /* offset: 0x0237 size: 8 bit */
			INTC_PSR_8B_tag PSR504; /* offset: 0x0238 size: 8 bit */
			INTC_PSR_8B_tag PSR505; /* offset: 0x0239 size: 8 bit */
			INTC_PSR_8B_tag PSR506; /* offset: 0x023A size: 8 bit */
			INTC_PSR_8B_tag PSR507; /* offset: 0x023B size: 8 bit */
			INTC_PSR_8B_tag PSR508; /* offset: 0x023C size: 8 bit */
			INTC_PSR_8B_tag PSR509; /* offset: 0x023D size: 8 bit */
			INTC_PSR_8B_tag PSR510; /* offset: 0x023E size: 8 bit */
			INTC_PSR_8B_tag PSR511; /* offset: 0x023F size: 8 bit */
		};
	};
} INTC_tag;

#define INTC (*(volatile INTC_tag*)0xFFF48000UL)

/****************************************************************/
/*                                                              */
/* Module: DSPI  */
/*                                                              */
/****************************************************************/

typedef union { /* MCR - Module Configuration Register */
	vuint32_t R;
	struct {
		vuint32_t MSTR : 1;			 /* Master/Slave mode select */
		vuint32_t CONT_SCKE : 1; /* Continuous SCK Enable */
		vuint32_t DCONF : 2;		 /* DSPI Configuration */
		vuint32_t FRZ : 1;			 /* Freeze */
		vuint32_t MTFE : 1;			 /* Modified Timing Format Enable */
		vuint32_t PCSSE : 1;		 /* Peripheral Chip Select Strobe Enable */
		vuint32_t ROOE : 1;			 /* Receive FIFO Overflow Overwrite Enable */
		vuint32_t PCSIS7 : 1;		 /* Peripheral Chip Select 7 Inactive State */
		vuint32_t PCSIS6 : 1;		 /* Peripheral Chip Select 6 Inactive State */
		vuint32_t PCSIS5 : 1;		 /* Peripheral Chip Select 5 Inactive State */
		vuint32_t PCSIS4 : 1;		 /* Peripheral Chip Select 4 Inactive State */
		vuint32_t PCSIS3 : 1;		 /* Peripheral Chip Select 3 Inactive State */
		vuint32_t PCSIS2 : 1;		 /* Peripheral Chip Select 2 Inactive State */
		vuint32_t PCSIS1 : 1;		 /* Peripheral Chip Select 1 Inactive State */
		vuint32_t PCSIS0 : 1;		 /* Peripheral Chip Select 0 Inactive State */
		vuint32_t DOZE : 1;			 /* Doze Enable */
		vuint32_t MDIS : 1;			 /* Module Disable */
		vuint32_t DIS_TXF : 1;	 /* Disable Transmit FIFO */
		vuint32_t DIS_RXF : 1;	 /* Disable Receive FIFO */
		vuint32_t CLR_TXF : 1;	 /* Clear TX FIFO */
		vuint32_t CLR_RXF : 1;	 /* Clear RX FIFO */
		vuint32_t SMPL_PT : 2;	 /* Sample Point */
		vuint32_t : 7;
		vuint32_t HALT : 1; /* Halt */
	} B;
} DSPI_MCR_32B_tag;

typedef union { /* TCR - Transfer Count Register */
	vuint32_t R;
	struct {
#ifndef USE_FIELD_ALIASES_DSPI
		vuint32_t SPI_TCNT : 16; /* SPI Transfer Counter */
#else
		vuint32_t TCNT : 16;			 /* deprecated name - please avoid */
#endif
		vuint32_t : 16;
	} B;
} DSPI_TCR_32B_tag;

/* Register layout for all registers CTAR... */

typedef union { /* CTAR0-7 - Clock and Transfer Attribute Registers */
	vuint32_t R;
	struct {
		vuint32_t DBR : 1;		/* Double Baud Rate */
		vuint32_t FMSZ : 4;		/* Frame Size */
		vuint32_t CPOL : 1;		/* Clock Polarity */
		vuint32_t CPHA : 1;		/* Clock Phase */
		vuint32_t LSBFE : 1;	/* LSB First Enable */
		vuint32_t PCSSCK : 2; /* PCS to SCK Delay Prescaler */
		vuint32_t PASC : 2;		/* After SCK Delay Prescaler */
		vuint32_t PDT : 2;		/* Delay after Transfer Prescaler */
		vuint32_t PBR : 2;		/* Baud Rate Prescaler */
		vuint32_t CSSCK : 4;	/* PCS to SCK Delay Scaler */
		vuint32_t ASC : 4;		/* After SCK Delay Scaler */
		vuint32_t DT : 4;			/* Delay after Transfer Scaler */
		vuint32_t BR : 4;			/* Baud Rate Scaler */
	} B;
} DSPI_CTAR_32B_tag;

typedef union { /* SR - Status Register */
	vuint32_t R;
	struct {
		vuint32_t TCF : 1;	 /* Transfer Complete Flag */
		vuint32_t TXRXS : 1; /* TX & RX Status */
		vuint32_t : 1;
		vuint32_t EOQF : 1; /* End of queue Flag */
		vuint32_t TFUF : 1; /* Transmit FIFO Underflow Flag */
		vuint32_t : 1;
		vuint32_t TFFF : 1; /* Transmit FIFO FIll Flag */
		vuint32_t : 5;
		vuint32_t RFOF : 1; /* Receive FIFO Overflow Flag */
		vuint32_t : 1;
		vuint32_t RFDF : 1; /* Receive FIFO Drain Flag */
		vuint32_t : 1;
		vuint32_t TXCTR : 4;		 /* TX FIFO Counter */
		vuint32_t TXNXTPTR : 4;	 /* Transmit Next Pointer */
		vuint32_t RXCTR : 4;		 /* RX FIFO Counter */
		vuint32_t POPNXTPTR : 4; /* Pop Next Pointer */
	} B;
} DSPI_SR_32B_tag;

typedef union { /* RSER - DMA/Interrupt Request Register */
	vuint32_t R;
	struct {
#ifndef USE_FIELD_ALIASES_DSPI
		vuint32_t TCF_RE : 1; /* Transmission Complete Request Enable */
#else
		vuint32_t TCFRE : 1;			 /* deprecated name - please avoid */
#endif
		vuint32_t : 2;
#ifndef USE_FIELD_ALIASES_DSPI
		vuint32_t EOQF_RE : 1; /* DSPI Finished Request Enable */
#else
		vuint32_t EOQFRE : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_DSPI
		vuint32_t TFUF_RE : 1; /* Transmit FIFO Underflow Request Enable */
#else
		vuint32_t TFUFRE : 1;			 /* deprecated name - please avoid */
#endif
		vuint32_t : 1;
#ifndef USE_FIELD_ALIASES_DSPI
		vuint32_t TFFF_RE : 1; /* Transmit FIFO Fill Request Enable */
#else
		vuint32_t TFFFRE : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_DSPI
		vuint32_t
				TFFF_DIRS : 1; /* Transmit FIFO Fill DMA or Interrupt Request Select */
#else
		vuint32_t TFFFDIRS : 1;		 /* deprecated name - please avoid */
#endif
		vuint32_t : 4;
#ifndef USE_FIELD_ALIASES_DSPI
		vuint32_t RFOF_RE : 1; /* Receive FIFO overflow Request Enable */
#else
		vuint32_t RFOFRE : 1;			 /* deprecated name - please avoid */
#endif
		vuint32_t : 1;
#ifndef USE_FIELD_ALIASES_DSPI
		vuint32_t RFDF_RE : 1; /* Receive FIFO Drain Request Enable */
#else
		vuint32_t RFDFRE : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_DSPI
		vuint32_t
				RFDF_DIRS : 1; /* Receive FIFO Drain DMA or Interrupt Request Select */
#else
		vuint32_t RFDFDIRS : 1;		 /* deprecated name - please avoid */
#endif
		vuint32_t : 16;
	} B;
} DSPI_RSER_32B_tag;

typedef union { /* PUSHR - PUSH TX FIFO Register */
	vuint32_t R;
	struct {
		vuint32_t CONT : 1;	 /* Continuous Peripheral Chip Select Enable */
		vuint32_t CTAS : 3;	 /* Clock and Transfer Attributes Select */
		vuint32_t EOQ : 1;	 /* End of Queue */
		vuint32_t CTCNT : 1; /* Clear SPI_TCNT */
		vuint32_t : 2;
		vuint32_t PCS7 : 1;		 /* Peripheral Chip Select 7 */
		vuint32_t PCS6 : 1;		 /* Peripheral Chip Select 6 */
		vuint32_t PCS5 : 1;		 /* Peripheral Chip Select 5 */
		vuint32_t PCS4 : 1;		 /* Peripheral Chip Select 4 */
		vuint32_t PCS3 : 1;		 /* Peripheral Chip Select 3 */
		vuint32_t PCS2 : 1;		 /* Peripheral Chip Select 2 */
		vuint32_t PCS1 : 1;		 /* Peripheral Chip Select 1 */
		vuint32_t PCS0 : 1;		 /* Peripheral Chip Select 0 */
		vuint32_t TXDATA : 16; /* Transmit Data */
	} B;
} DSPI_PUSHR_32B_tag;

typedef union { /* POPR - POP RX FIFO Register */
	vuint32_t R;
	struct {
		vuint32_t : 16;
		vuint32_t RXDATA : 16; /* Receive Data */
	} B;
} DSPI_POPR_32B_tag;

/* Register layout for all registers TXFR... */

typedef union { /* Transmit FIFO Registers */
	vuint32_t R;
	struct {
#ifndef USE_FIELD_ALIASES_DSPI
		vuint32_t FIFO_TXCMD : 16; /* Transmit Command */
#else
		vuint32_t TXCMD : 16;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_DSPI
		vuint32_t FIFO_TXDATA : 16; /* Transmit Data */
#else
		vuint32_t TXDATA : 16;		 /* deprecated name - please avoid */
#endif
	} B;
} DSPI_TXFR_32B_tag;

/* Register layout for all registers RXFR... */

typedef union { /* Receive FIFO Registers */
	vuint32_t R;
	struct {
		vuint32_t : 16;
#ifndef USE_FIELD_ALIASES_DSPI
		vuint32_t FIFO_RXDATA : 16; /* Transmit Data */
#else
		vuint32_t RXDATA : 16;		 /* deprecated name - please avoid */
#endif
	} B;
} DSPI_RXFR_32B_tag;

typedef union { /* DSICR - DSI Configuration Register */
	vuint32_t R;
	struct {
		vuint32_t MTOE : 1; /* Multiple Transfer Operation Enable */
		vuint32_t : 1;
		vuint32_t MTOCNT : 6; /* Multiple Transfer Operation Count */
		vuint32_t : 4;
		vuint32_t TXSS : 1;		 /* Transmit Data Source Select */
		vuint32_t TPOL : 1;		 /* Trigger Polarity */
		vuint32_t TRRE : 1;		 /* Trigger Reception Enable */
		vuint32_t CID : 1;		 /* Change in Data Transfer Enable */
		vuint32_t DCONT : 1;	 /* DSI Continuous Peripheral Chip Select Enable */
		vuint32_t DSICTAS : 3; /* DSI CLock and Transfer Attributes Select */
		vuint32_t : 4;
		vuint32_t DPCS7 : 1; /* DSI Peripheral Chip Select 7 */
		vuint32_t DPCS6 : 1; /* DSI Peripheral Chip Select 6 */
		vuint32_t DPCS5 : 1; /* DSI Peripheral Chip Select 5 */
		vuint32_t DPCS4 : 1; /* DSI Peripheral Chip Select 4 */
		vuint32_t DPCS3 : 1; /* DSI Peripheral Chip Select 3 */
		vuint32_t DPCS2 : 1; /* DSI Peripheral Chip Select 2 */
		vuint32_t DPCS1 : 1; /* DSI Peripheral Chip Select 1 */
		vuint32_t DPCS0 : 1; /* DSI Peripheral Chip Select 0 */
	} B;
} DSPI_DSICR_32B_tag;

typedef union { /* SDR - DSI Serialization Data Register */
	vuint32_t R;
	struct {
		vuint32_t : 16;
		vuint32_t SER_DATA : 16; /* Serialized Data */
	} B;
} DSPI_SDR_32B_tag;

typedef union { /* ASDR - DSI Alternate Serialization Data Register */
	vuint32_t R;
	struct {
		vuint32_t : 16;
		vuint32_t ASER_DATA : 16; /* Alternate Serialized Data */
	} B;
} DSPI_ASDR_32B_tag;

typedef union { /* COMPR - DSI Transmit Comparison Register */
	vuint32_t R;
	struct {
		vuint32_t : 16;
		vuint32_t COMP_DATA : 16; /* Compare Data */
	} B;
} DSPI_COMPR_32B_tag;

typedef union { /* DDR - DSI Deserialization Data Register */
	vuint32_t R;
	struct {
		vuint32_t : 16;
		vuint32_t DESER_DATA : 16; /* Deserialized Data */
	} B;
} DSPI_DDR_32B_tag;

typedef union { /* DSICR1 - DSI Configuration Register 1 */
	vuint32_t R;
} DSPI_DSICR1_32B_tag;

typedef struct DSPI_struct_tag { /* start of DSPI_tag */
																 /* MCR - Module Configuration Register */
	DSPI_MCR_32B_tag MCR;					 /* offset: 0x0000 size: 32 bit */
	int8_t DSPI_reserved_0004[4];
	/* TCR - Transfer Count Register */
	DSPI_TCR_32B_tag TCR; /* offset: 0x0008 size: 32 bit */
	union {
		/* CTAR0-7 - Clock and Transfer Attribute Registers */
		DSPI_CTAR_32B_tag CTAR[8]; /* offset: 0x000C  (0x0004 x 8) */

		struct {
			/* CTAR0-7 - Clock and Transfer Attribute Registers */
			DSPI_CTAR_32B_tag CTAR0; /* offset: 0x000C size: 32 bit */
			DSPI_CTAR_32B_tag CTAR1; /* offset: 0x0010 size: 32 bit */
			DSPI_CTAR_32B_tag CTAR2; /* offset: 0x0014 size: 32 bit */
			DSPI_CTAR_32B_tag CTAR3; /* offset: 0x0018 size: 32 bit */
			DSPI_CTAR_32B_tag CTAR4; /* offset: 0x001C size: 32 bit */
			DSPI_CTAR_32B_tag CTAR5; /* offset: 0x0020 size: 32 bit */
			DSPI_CTAR_32B_tag CTAR6; /* offset: 0x0024 size: 32 bit */
			DSPI_CTAR_32B_tag CTAR7; /* offset: 0x0028 size: 32 bit */
		};
	};
	/* SR - Status Register */
	DSPI_SR_32B_tag SR;				/* offset: 0x002C size: 32 bit */
														/* RSER - DMA/Interrupt Request Register */
	DSPI_RSER_32B_tag RSER;		/* offset: 0x0030 size: 32 bit */
														/* PUSHR - PUSH TX FIFO Register */
	DSPI_PUSHR_32B_tag PUSHR; /* offset: 0x0034 size: 32 bit */
														/* POPR - POP RX FIFO Register */
	DSPI_POPR_32B_tag POPR;		/* offset: 0x0038 size: 32 bit */
	union {
		/* Transmit FIFO Registers */
		DSPI_TXFR_32B_tag TXFR[5]; /* offset: 0x003C  (0x0004 x 5) */

		struct {
			/* Transmit FIFO Registers */
			DSPI_TXFR_32B_tag TXFR0; /* offset: 0x003C size: 32 bit */
			DSPI_TXFR_32B_tag TXFR1; /* offset: 0x0040 size: 32 bit */
			DSPI_TXFR_32B_tag TXFR2; /* offset: 0x0044 size: 32 bit */
			DSPI_TXFR_32B_tag TXFR3; /* offset: 0x0048 size: 32 bit */
			DSPI_TXFR_32B_tag TXFR4; /* offset: 0x004C size: 32 bit */
		};
	};
	int8_t DSPI_reserved_0050_C[44];
	union {
		/* Receive FIFO Registers */
		DSPI_RXFR_32B_tag RXFR[5]; /* offset: 0x007C  (0x0004 x 5) */

		struct {
			/* Receive FIFO Registers */
			DSPI_RXFR_32B_tag RXFR0; /* offset: 0x007C size: 32 bit */
			DSPI_RXFR_32B_tag RXFR1; /* offset: 0x0080 size: 32 bit */
			DSPI_RXFR_32B_tag RXFR2; /* offset: 0x0084 size: 32 bit */
			DSPI_RXFR_32B_tag RXFR3; /* offset: 0x0088 size: 32 bit */
			DSPI_RXFR_32B_tag RXFR4; /* offset: 0x008C size: 32 bit */
		};
	};
	int8_t DSPI_reserved_0090[44];
	/* DSICR - DSI Configuration Register */
	DSPI_DSICR_32B_tag DSICR; /* offset: 0x00BC size: 32 bit */
														/* SDR - DSI Serialization Data Register */
	DSPI_SDR_32B_tag SDR;			/* offset: 0x00C0 size: 32 bit */
	/* ASDR - DSI Alternate Serialization Data Register */
	DSPI_ASDR_32B_tag ASDR;			/* offset: 0x00C4 size: 32 bit */
															/* COMPR - DSI Transmit Comparison Register */
	DSPI_COMPR_32B_tag COMPR;		/* offset: 0x00C8 size: 32 bit */
															/* DDR - DSI Deserialization Data Register */
	DSPI_DDR_32B_tag DDR;				/* offset: 0x00CC size: 32 bit */
															/* DSICR1 - DSI Configuration Register 1 */
	DSPI_DSICR1_32B_tag DSICR1; /* offset: 0x00D0 size: 32 bit */
} DSPI_tag;

#define DSPI_A (*(volatile DSPI_tag*)0xFFF90000UL)
#define DSPI_B (*(volatile DSPI_tag*)0xFFF94000UL)
#define DSPI_C (*(volatile DSPI_tag*)0xFFF98000UL)

/****************************************************************/
/*                                                              */
/* Module: FLEXCAN  */
/*                                                              */
/****************************************************************/

typedef union { /* MCR - Module Configuration Register */
	vuint32_t R;
	struct {
		vuint32_t MDIS : 1; /* Module Disable */
		vuint32_t FRZ : 1;	/* Freeze Enable */
		vuint32_t FEN : 1;	/* FIFO Enable */
		vuint32_t HALT : 1; /* Halt Flexcan */
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t NOT_RDY : 1; /* Flexcan Not Ready */
#else
		vuint32_t NOTRDY : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t WAK_MSK : 1; /* Wake Up Interrupt Mask */
#else
		vuint32_t WAKMSK : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t SOFT_RST : 1; /* Soft Reset */
#else
		vuint32_t SOFTRST : 1;		 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t FRZ_ACK : 1; /* Freeze Mode Acknowledge */
#else
		vuint32_t FRZACK : 1;			 /* deprecated name - please avoid */
#endif
		vuint32_t SUPV : 1; /* Supervisor Mode */
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t SLF_WAK : 1; /* Self Wake Up */
#else
		vuint32_t SLFWAK : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t WRN_EN : 1; /* Warning Interrupt Enable */
#else
		vuint32_t WRNEN : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t LPM_ACK : 1; /* Low Power Mode Acknowledge */
#else
		vuint32_t LPMACK : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t WAK_SRC : 1; /* Wake Up Source */
#else
		vuint32_t WAKSRC : 1;			 /* deprecated name - please avoid */
#endif
		vuint32_t DOZE : 1; /* Doze Mode Enable */
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t SRX_DIS : 1; /* Self Reception Disable */
#else
		vuint32_t SRXDIS : 1;			 /* deprecated name - please avoid */
#endif
		vuint32_t BCC : 1; /* Backwards Compatibility Configuration */
		vuint32_t : 2;
		vuint32_t LPRIO_EN : 1; /* Local Priority Enable */
		vuint32_t AEN : 1;			/* Abort Enable */
		vuint32_t : 2;
		vuint32_t IDAM : 2; /* ID Acceptance Mode */
		vuint32_t : 2;
		vuint32_t MAXMB : 6; /* Maximum Number of Message Buffers */
	} B;
} FLEXCAN_MCR_32B_tag;

typedef union { /* CTRL -  Control Register */
	vuint32_t R;
	struct {
		vuint32_t PRESDIV : 8; /* Prescaler Divsion Factor */
		vuint32_t RJW : 2;		 /* Resync Jump Width */
		vuint32_t PSEG1 : 3;	 /* Phase Segment 1 */
		vuint32_t PSEG2 : 3;	 /* Phase Segment 2 */
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t BOFF_MSK : 1; /* Bus Off Mask */
#else
		vuint32_t BOFFMSK : 1;		 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t ERR_MSK : 1; /* Error Mask */
#else
		vuint32_t ERRMSK : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t CLK_SRC : 1; /* CAN Engine Clock Source */
#else
		vuint32_t CLKSRC : 1;			 /* deprecated name - please avoid */
#endif
		vuint32_t LPB : 1; /* Loop Back */
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t TWRN_MSK : 1; /* Tx Warning Interrupt Mask */
#else
		vuint32_t TWRNMSK : 1;		 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t RWRN_MSK : 1; /* Rx Warning Interrupt Mask */
#else
		vuint32_t RWRNMSK : 1;		 /* deprecated name - please avoid */
#endif
		vuint32_t : 2;
		vuint32_t SMP : 1; /* Sampling Mode */
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t BOFF_REC : 1; /* Bus Off Recovery Mode */
#else
		vuint32_t BOFFREC : 1;		 /* deprecated name - please avoid */
#endif
		vuint32_t TSYN : 1;		 /* Timer Sync Mode */
		vuint32_t LBUF : 1;		 /* Lowest Buffer Transmitted First */
		vuint32_t LOM : 1;		 /* Listen-Only Mode */
		vuint32_t PROPSEG : 3; /* Propagation Segment */
	} B;
} FLEXCAN_CTRL_32B_tag;

typedef union { /* TIMER - Free Running Timer */
	vuint32_t R;
} FLEXCAN_TIMER_32B_tag;

typedef union { /* RXGMASK - Rx Global Mask Register */
	vuint32_t R;
#ifndef USE_FIELD_ALIASES_FLEXCAN
	struct {
		vuint32_t MI : 32; /* deprecated field -- do not use */
	} B;
#endif
} FLEXCAN_RXGMASK_32B_tag;

typedef union { /* RX14MASK - Rx 14 Mask Register */
	vuint32_t R;
#ifndef USE_FIELD_ALIASES_FLEXCAN
	struct {
		vuint32_t MI : 32; /* deprecated field -- do not use */
	} B;
#endif
} FLEXCAN_RX14MASK_32B_tag;

typedef union { /* RX15MASK - Rx 15 Mask Register */
	vuint32_t R;
#ifndef USE_FIELD_ALIASES_FLEXCAN
	struct {
		vuint32_t MI : 32; /* deprecated field -- do not use */
	} B;
#endif
} FLEXCAN_RX15MASK_32B_tag;

typedef union { /* ECR - Error Counter Register */
	vuint32_t R;
	struct {
		vuint32_t : 16;
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t RX_ERR_COUNTER : 8; /* Rx Error Counter */
#else
		vuint32_t RXECNT : 8;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t TX_ERR_COUNTER : 8; /* Tx Error Counter */
#else
		vuint32_t TXECNT : 8;			 /* deprecated name - please avoid */
#endif
	} B;
} FLEXCAN_ECR_32B_tag;

typedef union { /* ESR - Error and Status Register */
	vuint32_t R;
	struct {
		vuint32_t : 14;
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t TWRN_INT : 1; /* Tx Warning Interrupt Flag */
#else
		vuint32_t TWRNINT : 1;		 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t RWRN_INT : 1; /* Rx Warning Interrupt Flag */
#else
		vuint32_t RWRNINT : 1;		 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t BIT1_ERR : 1; /* Bit 1 Error */
#else
		vuint32_t BIT1ERR : 1;		 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t BIT0_ERR : 1; /* Bit 0 Error */
#else
		vuint32_t BIT0ERR : 1;		 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t ACK_ERR : 1; /* Acknowledge Error */
#else
		vuint32_t ACKERR : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t CRC_ERR : 1; /* Cyclic Redundancy Check Error */
#else
		vuint32_t CRCERR : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t FRM_ERR : 1; /* Form Error */
#else
		vuint32_t FRMERR : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t STF_ERR : 1; /* Stuffing Error */
#else
		vuint32_t STFERR : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t TX_WRN : 1; /* Tx Error Counter */
#else
		vuint32_t TXWRN : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t RX_WRN : 1; /* Rx Error Counter */
#else
		vuint32_t RXWRN : 1;			 /* deprecated name - please avoid */
#endif
		vuint32_t IDLE : 1; /* CAN bus Idle State */
		vuint32_t TXRX : 1; /* Current Flexcan Status */
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t FLT_CONF : 2; /* Fault Confinement State */
#else
		vuint32_t FLTCONF : 2;		 /* deprecated name - please avoid */
#endif
		vuint32_t : 1;
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t BOFF_INT : 1; /* Bus Off Interrupt */
#else
		vuint32_t BOFFINT : 1;		 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t ERR_INT : 1; /* Error Interrupt */
#else
		vuint32_t ERRINT : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t WAK_INT : 1; /* Wake-Up Interrupt */
#else
		vuint32_t WAKINT : 1;			 /* deprecated name - please avoid */
#endif
	} B;
} FLEXCAN_ESR_32B_tag;

typedef union { /* IMASK2 - Interrupt Masks 2 Register */
	vuint32_t R;
	struct {
		vuint32_t BUF63M : 1; /* Buffer MB Mask 63 Bit */
		vuint32_t BUF62M : 1; /* Buffer MB Mask 62 Bit */
		vuint32_t BUF61M : 1; /* Buffer MB Mask 61 Bit */
		vuint32_t BUF60M : 1; /* Buffer MB Mask 60 Bit */
		vuint32_t BUF59M : 1; /* Buffer MB Mask 59 Bit */
		vuint32_t BUF58M : 1; /* Buffer MB Mask 58 Bit */
		vuint32_t BUF57M : 1; /* Buffer MB Mask 57 Bit */
		vuint32_t BUF56M : 1; /* Buffer MB Mask 56 Bit */
		vuint32_t BUF55M : 1; /* Buffer MB Mask 55 Bit */
		vuint32_t BUF54M : 1; /* Buffer MB Mask 54 Bit */
		vuint32_t BUF53M : 1; /* Buffer MB Mask 53 Bit */
		vuint32_t BUF52M : 1; /* Buffer MB Mask 52 Bit */
		vuint32_t BUF51M : 1; /* Buffer MB Mask 51 Bit */
		vuint32_t BUF50M : 1; /* Buffer MB Mask 50 Bit */
		vuint32_t BUF49M : 1; /* Buffer MB Mask 49 Bit */
		vuint32_t BUF48M : 1; /* Buffer MB Mask 48 Bit */
		vuint32_t BUF47M : 1; /* Buffer MB Mask 47 Bit */
		vuint32_t BUF46M : 1; /* Buffer MB Mask 46 Bit */
		vuint32_t BUF45M : 1; /* Buffer MB Mask 45 Bit */
		vuint32_t BUF44M : 1; /* Buffer MB Mask 44 Bit */
		vuint32_t BUF43M : 1; /* Buffer MB Mask 43 Bit */
		vuint32_t BUF42M : 1; /* Buffer MB Mask 42 Bit */
		vuint32_t BUF41M : 1; /* Buffer MB Mask 41 Bit */
		vuint32_t BUF40M : 1; /* Buffer MB Mask 40 Bit */
		vuint32_t BUF39M : 1; /* Buffer MB Mask 39 Bit */
		vuint32_t BUF38M : 1; /* Buffer MB Mask 38 Bit */
		vuint32_t BUF37M : 1; /* Buffer MB Mask 37 Bit */
		vuint32_t BUF36M : 1; /* Buffer MB Mask 36 Bit */
		vuint32_t BUF35M : 1; /* Buffer MB Mask 35 Bit */
		vuint32_t BUF34M : 1; /* Buffer MB Mask 34 Bit */
		vuint32_t BUF33M : 1; /* Buffer MB Mask 33 Bit */
		vuint32_t BUF32M : 1; /* Buffer MB Mask 32 Bit */
	} B;
} FLEXCAN_IMASK2_32B_tag;

typedef union { /* IMASK1 - Interrupt Masks 1 Register */
	vuint32_t R;
	struct {
		vuint32_t BUF31M : 1; /* Buffer MB Mask 31 Bit */
		vuint32_t BUF30M : 1; /* Buffer MB Mask 30 Bit */
		vuint32_t BUF29M : 1; /* Buffer MB Mask 29 Bit */
		vuint32_t BUF28M : 1; /* Buffer MB Mask 28 Bit */
		vuint32_t BUF27M : 1; /* Buffer MB Mask 27 Bit */
		vuint32_t BUF26M : 1; /* Buffer MB Mask 26 Bit */
		vuint32_t BUF25M : 1; /* Buffer MB Mask 25 Bit */
		vuint32_t BUF24M : 1; /* Buffer MB Mask 24 Bit */
		vuint32_t BUF23M : 1; /* Buffer MB Mask 23 Bit */
		vuint32_t BUF22M : 1; /* Buffer MB Mask 22 Bit */
		vuint32_t BUF21M : 1; /* Buffer MB Mask 21 Bit */
		vuint32_t BUF20M : 1; /* Buffer MB Mask 20 Bit */
		vuint32_t BUF19M : 1; /* Buffer MB Mask 19 Bit */
		vuint32_t BUF18M : 1; /* Buffer MB Mask 18 Bit */
		vuint32_t BUF17M : 1; /* Buffer MB Mask 17 Bit */
		vuint32_t BUF16M : 1; /* Buffer MB Mask 16 Bit */
		vuint32_t BUF15M : 1; /* Buffer MB Mask 15 Bit */
		vuint32_t BUF14M : 1; /* Buffer MB Mask 14 Bit */
		vuint32_t BUF13M : 1; /* Buffer MB Mask 13 Bit */
		vuint32_t BUF12M : 1; /* Buffer MB Mask 12 Bit */
		vuint32_t BUF11M : 1; /* Buffer MB Mask 11 Bit */
		vuint32_t BUF10M : 1; /* Buffer MB Mask 10 Bit */
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t BUF9M : 1; /* Buffer MB Mask 9 Bit */
#else
		vuint32_t BUF09M : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t BUF8M : 1; /* Buffer MB Mask 8 Bit */
#else
		vuint32_t BUF08M : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t BUF7M : 1; /* Buffer MB Mask 7 Bit */
#else
		vuint32_t BUF07M : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t BUF6M : 1; /* Buffer MB Mask 6 Bit */
#else
		vuint32_t BUF06M : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t BUF5M : 1; /* Buffer MB Mask 5 Bit */
#else
		vuint32_t BUF05M : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t BUF4M : 1; /* Buffer MB Mask 4 Bit */
#else
		vuint32_t BUF04M : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t BUF3M : 1; /* Buffer MB Mask 3 Bit */
#else
		vuint32_t BUF03M : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t BUF2M : 1; /* Buffer MB Mask 2 Bit */
#else
		vuint32_t BUF02M : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t BUF1M : 1; /* Buffer MB Mask 1 Bit */
#else
		vuint32_t BUF01M : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t BUF0M : 1; /* Buffer MB Mask 0 Bit */
#else
		vuint32_t BUF00M : 1;			 /* deprecated name - please avoid */
#endif
	} B;
} FLEXCAN_IMASK1_32B_tag;

typedef union { /* IFLAG2 - Interrupt Flags 2 Register */
	vuint32_t R;
	struct {
		vuint32_t BUF63I : 1; /* Buffer MB Interrupt 63 Bit */
		vuint32_t BUF62I : 1; /* Buffer MB Interrupt 62 Bit */
		vuint32_t BUF61I : 1; /* Buffer MB Interrupt 61 Bit */
		vuint32_t BUF60I : 1; /* Buffer MB Interrupt 60 Bit */
		vuint32_t BUF59I : 1; /* Buffer MB Interrupt 59 Bit */
		vuint32_t BUF58I : 1; /* Buffer MB Interrupt 58 Bit */
		vuint32_t BUF57I : 1; /* Buffer MB Interrupt 57 Bit */
		vuint32_t BUF56I : 1; /* Buffer MB Interrupt 56 Bit */
		vuint32_t BUF55I : 1; /* Buffer MB Interrupt 55 Bit */
		vuint32_t BUF54I : 1; /* Buffer MB Interrupt 54 Bit */
		vuint32_t BUF53I : 1; /* Buffer MB Interrupt 53 Bit */
		vuint32_t BUF52I : 1; /* Buffer MB Interrupt 52 Bit */
		vuint32_t BUF51I : 1; /* Buffer MB Interrupt 51 Bit */
		vuint32_t BUF50I : 1; /* Buffer MB Interrupt 50 Bit */
		vuint32_t BUF49I : 1; /* Buffer MB Interrupt 49 Bit */
		vuint32_t BUF48I : 1; /* Buffer MB Interrupt 48 Bit */
		vuint32_t BUF47I : 1; /* Buffer MB Interrupt 47 Bit */
		vuint32_t BUF46I : 1; /* Buffer MB Interrupt 46 Bit */
		vuint32_t BUF45I : 1; /* Buffer MB Interrupt 45 Bit */
		vuint32_t BUF44I : 1; /* Buffer MB Interrupt 44 Bit */
		vuint32_t BUF43I : 1; /* Buffer MB Interrupt 43 Bit */
		vuint32_t BUF42I : 1; /* Buffer MB Interrupt 42 Bit */
		vuint32_t BUF41I : 1; /* Buffer MB Interrupt 41 Bit */
		vuint32_t BUF40I : 1; /* Buffer MB Interrupt 40 Bit */
		vuint32_t BUF39I : 1; /* Buffer MB Interrupt 39 Bit */
		vuint32_t BUF38I : 1; /* Buffer MB Interrupt 38 Bit */
		vuint32_t BUF37I : 1; /* Buffer MB Interrupt 37 Bit */
		vuint32_t BUF36I : 1; /* Buffer MB Interrupt 36 Bit */
		vuint32_t BUF35I : 1; /* Buffer MB Interrupt 35 Bit */
		vuint32_t BUF34I : 1; /* Buffer MB Interrupt 34 Bit */
		vuint32_t BUF33I : 1; /* Buffer MB Interrupt 33 Bit */
		vuint32_t BUF32I : 1; /* Buffer MB Interrupt 32 Bit */
	} B;
} FLEXCAN_IFLAG2_32B_tag;

typedef union { /* IFLAG1 - Interrupt Flags 1 Register */
	vuint32_t R;
	struct {
		vuint32_t BUF31I : 1; /* Buffer MB Interrupt 31 Bit */
		vuint32_t BUF30I : 1; /* Buffer MB Interrupt 30 Bit */
		vuint32_t BUF29I : 1; /* Buffer MB Interrupt 29 Bit */
		vuint32_t BUF28I : 1; /* Buffer MB Interrupt 28 Bit */
		vuint32_t BUF27I : 1; /* Buffer MB Interrupt 27 Bit */
		vuint32_t BUF26I : 1; /* Buffer MB Interrupt 26 Bit */
		vuint32_t BUF25I : 1; /* Buffer MB Interrupt 25 Bit */
		vuint32_t BUF24I : 1; /* Buffer MB Interrupt 24 Bit */
		vuint32_t BUF23I : 1; /* Buffer MB Interrupt 23 Bit */
		vuint32_t BUF22I : 1; /* Buffer MB Interrupt 22 Bit */
		vuint32_t BUF21I : 1; /* Buffer MB Interrupt 21 Bit */
		vuint32_t BUF20I : 1; /* Buffer MB Interrupt 20 Bit */
		vuint32_t BUF19I : 1; /* Buffer MB Interrupt 19 Bit */
		vuint32_t BUF18I : 1; /* Buffer MB Interrupt 18 Bit */
		vuint32_t BUF17I : 1; /* Buffer MB Interrupt 17 Bit */
		vuint32_t BUF16I : 1; /* Buffer MB Interrupt 16 Bit */
		vuint32_t BUF15I : 1; /* Buffer MB Interrupt 15 Bit */
		vuint32_t BUF14I : 1; /* Buffer MB Interrupt 14 Bit */
		vuint32_t BUF13I : 1; /* Buffer MB Interrupt 13 Bit */
		vuint32_t BUF12I : 1; /* Buffer MB Interrupt 12 Bit */
		vuint32_t BUF11I : 1; /* Buffer MB Interrupt 11 Bit */
		vuint32_t BUF10I : 1; /* Buffer MB Interrupt 10 Bit */
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t BUF9I : 1; /* Buffer MB Interrupt 9 Bit */
#else
		vuint32_t BUF09I : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t BUF8I : 1; /* Buffer MB Interrupt 8 Bit */
#else
		vuint32_t BUF08I : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t BUF7I : 1; /* Buffer MB Interrupt 7 Bit */
#else
		vuint32_t BUF07I : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t BUF6I : 1; /* Buffer MB Interrupt 6 Bit */
#else
		vuint32_t BUF06I : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t BUF5I : 1; /* Buffer MB Interrupt 5 Bit */
#else
		vuint32_t BUF05I : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t BUF4I : 1; /* Buffer MB Interrupt 4 Bit */
#else
		vuint32_t BUF04I : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t BUF3I : 1; /* Buffer MB Interrupt 3 Bit */
#else
		vuint32_t BUF03I : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t BUF2I : 1; /* Buffer MB Interrupt 2 Bit */
#else
		vuint32_t BUF02I : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t BUF1I : 1; /* Buffer MB Interrupt 1 Bit */
#else
		vuint32_t BUF01I : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FLEXCAN
		vuint32_t BUF0I : 1; /* Buffer MB Interrupt 0 Bit */
#else
		vuint32_t BUF00I : 1;			 /* deprecated name - please avoid */
#endif
	} B;
} FLEXCAN_IFLAG1_32B_tag;

/* Register layout for all registers MSG_CS... */

typedef union { /* Message Buffer Control and Status */
	vuint32_t R;
	struct {
		vuint32_t : 4;
		vuint32_t CODE : 4; /* Message Buffer Code */
		vuint32_t : 1;
		vuint32_t SRR : 1;				/* Substitute Remote Request */
		vuint32_t IDE : 1;				/* ID Extended Bit */
		vuint32_t RTR : 1;				/* Remote Transmission Request */
		vuint32_t LENGTH : 4;			/* Length of Data in Bytes */
		vuint32_t TIMESTAMP : 16; /* Free-Running Counter Time Stamp */
	} B;
} FLEXCAN_MSG_CS_32B_tag;

/* Register layout for all registers MSG_ID... */

typedef union { /* Message Buffer Identifier Field */
	vuint32_t R;
	struct {
		vuint32_t PRIO : 3; /* Local Priority */
		vuint32_t STD_ID : 11;
		vuint32_t EXT_ID : 18;
	} B;
} FLEXCAN_MSG_ID_32B_tag;

/* Register layout for all registers MSG_BYTE0_3... */

typedef union { /* Message Buffer Data Register */
	vuint32_t R;
	vuint8_t BYTE[4]; /* individual bytes can be accessed */
	vuint32_t WORD;		/* individual words can be accessed */
} FLEXCAN_MSG_DATA_32B_tag;

typedef union {
	vuint8_t B[8];	/* Data buffer in Bytes (8 bits) */
	vuint16_t H[4]; /* Data buffer in Half-words (16 bits) */
	vuint32_t W[2]; /* Data buffer in words (32 bits) */
	vuint32_t R[2]; /* Data buffer in words (32 bits) */
} FLEXCAN_MSG_DATA2_32B_tag;

/* Register layout for all registers MSG_BYTE4_7 matches xxx */

/* Register layout for all registers RXIMR... */

typedef union { /* FLEXCAN_RXIMR0 - FLEXCAN_RXIMR63 - RX Individual Mask
									 Registers */
	vuint32_t R;
} FLEXCAN_RXIMR_32B_tag;

typedef struct FLEXCAN_MB_struct_tag {
	union {
		/* Message Buffer Control and Status */
		FLEXCAN_MSG_CS_32B_tag MSG_CS; /* relative offset: 0x0000 */
		FLEXCAN_MSG_CS_32B_tag CS;		 /* deprecated - please avoid */
	};
	union {
		/* Message Buffer Identifier Field */
		FLEXCAN_MSG_ID_32B_tag MSG_ID; /* relative offset: 0x0004 */
		FLEXCAN_MSG_ID_32B_tag ID;		 /* deprecated - please avoid */
	};
	union { /* Message Buffer Data Register */

		struct {
			FLEXCAN_MSG_DATA_32B_tag MSG_BYTE0_3; /* relative offset: 0x0008 */
																						/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG_BYTE4_7; /* relative offset: 0x000C */
		};

		FLEXCAN_MSG_DATA2_32B_tag DATA; /* relative offset: 0x000C */
	};

} FLEXCAN_MB_tag;

typedef struct FLEXCAN_struct_tag { /* start of FLEXCAN_tag */
																		/* MCR - Module Configuration Register */
	FLEXCAN_MCR_32B_tag MCR;					/* offset: 0x0000 size: 32 bit */
	union {
		/* CTRL -  Control Register */
		FLEXCAN_CTRL_32B_tag CTRL; /* offset: 0x0004 size: 32 bit */

		FLEXCAN_CTRL_32B_tag CR; /* deprecated - please avoid */
	};
	/* TIMER - Free Running Timer */
	FLEXCAN_TIMER_32B_tag TIMER; /* offset: 0x0008 size: 32 bit */
	int8_t FLEXCAN_reserved_000C[4];
	/* RXGMASK - Rx Global Mask Register */
	FLEXCAN_RXGMASK_32B_tag RXGMASK;	 /* offset: 0x0010 size: 32 bit */
																		 /* RX14MASK - Rx 14 Mask Register */
	FLEXCAN_RX14MASK_32B_tag RX14MASK; /* offset: 0x0014 size: 32 bit */
																		 /* RX15MASK - Rx 15 Mask Register */
	FLEXCAN_RX15MASK_32B_tag RX15MASK; /* offset: 0x0018 size: 32 bit */
																		 /* ECR - Error Counter Register */
	FLEXCAN_ECR_32B_tag ECR;					 /* offset: 0x001C size: 32 bit */
																		 /* ESR - Error and Status Register */
	FLEXCAN_ESR_32B_tag ESR;					 /* offset: 0x0020 size: 32 bit */
	union {
		FLEXCAN_IMASK2_32B_tag IMRH; /* deprecated - please avoid */

		/* IMASK2 - Interrupt Masks 2 Register */
		FLEXCAN_IMASK2_32B_tag IMASK2; /* offset: 0x0024 size: 32 bit */
	};
	union {
		FLEXCAN_IMASK1_32B_tag IMRL; /* deprecated - please avoid */

		/* IMASK1 - Interrupt Masks 1 Register */
		FLEXCAN_IMASK1_32B_tag IMASK1; /* offset: 0x0028 size: 32 bit */
	};
	union {
		FLEXCAN_IFLAG2_32B_tag IFRH; /* deprecated - please avoid */

		/* IFLAG2 - Interrupt Flags 2 Register */
		FLEXCAN_IFLAG2_32B_tag IFLAG2; /* offset: 0x002C size: 32 bit */
	};
	union {
		FLEXCAN_IFLAG1_32B_tag IFRL; /* deprecated - please avoid */

		/* IFLAG1 - Interrupt Flags 1 Register */
		FLEXCAN_IFLAG1_32B_tag IFLAG1; /* offset: 0x0030 size: 32 bit */
	};
	int8_t FLEXCAN_reserved_0034_C[76];
	union {
		/*  Register set MB */
		FLEXCAN_MB_tag MB[64]; /* offset: 0x0080  (0x0010 x 64) */

		/*  Alias name for MB */
		FLEXCAN_MB_tag BUF[64]; /* deprecated - please avoid */

		struct {
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG0_CS; /* offset: 0x0080 size: 32 bit */
																			/* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG0_ID; /* offset: 0x0084 size: 32 bit */
																			/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG0_BYTE0_3; /* offset: 0x0088 size: 32 bit */
																						 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG0_BYTE4_7; /* offset: 0x008C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG1_CS; /* offset: 0x0090 size: 32 bit */
																			/* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG1_ID; /* offset: 0x0094 size: 32 bit */
																			/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG1_BYTE0_3; /* offset: 0x0098 size: 32 bit */
																						 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG1_BYTE4_7; /* offset: 0x009C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG2_CS; /* offset: 0x00A0 size: 32 bit */
																			/* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG2_ID; /* offset: 0x00A4 size: 32 bit */
																			/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG2_BYTE0_3; /* offset: 0x00A8 size: 32 bit */
																						 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG2_BYTE4_7; /* offset: 0x00AC size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG3_CS; /* offset: 0x00B0 size: 32 bit */
																			/* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG3_ID; /* offset: 0x00B4 size: 32 bit */
																			/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG3_BYTE0_3; /* offset: 0x00B8 size: 32 bit */
																						 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG3_BYTE4_7; /* offset: 0x00BC size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG4_CS; /* offset: 0x00C0 size: 32 bit */
																			/* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG4_ID; /* offset: 0x00C4 size: 32 bit */
																			/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG4_BYTE0_3; /* offset: 0x00C8 size: 32 bit */
																						 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG4_BYTE4_7; /* offset: 0x00CC size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG5_CS; /* offset: 0x00D0 size: 32 bit */
																			/* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG5_ID; /* offset: 0x00D4 size: 32 bit */
																			/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG5_BYTE0_3; /* offset: 0x00D8 size: 32 bit */
																						 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG5_BYTE4_7; /* offset: 0x00DC size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG6_CS; /* offset: 0x00E0 size: 32 bit */
																			/* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG6_ID; /* offset: 0x00E4 size: 32 bit */
																			/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG6_BYTE0_3; /* offset: 0x00E8 size: 32 bit */
																						 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG6_BYTE4_7; /* offset: 0x00EC size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG7_CS; /* offset: 0x00F0 size: 32 bit */
																			/* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG7_ID; /* offset: 0x00F4 size: 32 bit */
																			/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG7_BYTE0_3; /* offset: 0x00F8 size: 32 bit */
																						 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG7_BYTE4_7; /* offset: 0x00FC size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG8_CS; /* offset: 0x0100 size: 32 bit */
																			/* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG8_ID; /* offset: 0x0104 size: 32 bit */
																			/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG8_BYTE0_3; /* offset: 0x0108 size: 32 bit */
																						 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG8_BYTE4_7; /* offset: 0x010C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG9_CS; /* offset: 0x0110 size: 32 bit */
																			/* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG9_ID; /* offset: 0x0114 size: 32 bit */
																			/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG9_BYTE0_3; /* offset: 0x0118 size: 32 bit */
																						 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG9_BYTE4_7; /* offset: 0x011C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG10_CS; /* offset: 0x0120 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG10_ID; /* offset: 0x0124 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG10_BYTE0_3; /* offset: 0x0128 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG10_BYTE4_7; /* offset: 0x012C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG11_CS; /* offset: 0x0130 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG11_ID; /* offset: 0x0134 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG11_BYTE0_3; /* offset: 0x0138 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG11_BYTE4_7; /* offset: 0x013C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG12_CS; /* offset: 0x0140 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG12_ID; /* offset: 0x0144 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG12_BYTE0_3; /* offset: 0x0148 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG12_BYTE4_7; /* offset: 0x014C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG13_CS; /* offset: 0x0150 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG13_ID; /* offset: 0x0154 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG13_BYTE0_3; /* offset: 0x0158 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG13_BYTE4_7; /* offset: 0x015C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG14_CS; /* offset: 0x0160 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG14_ID; /* offset: 0x0164 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG14_BYTE0_3; /* offset: 0x0168 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG14_BYTE4_7; /* offset: 0x016C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG15_CS; /* offset: 0x0170 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG15_ID; /* offset: 0x0174 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG15_BYTE0_3; /* offset: 0x0178 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG15_BYTE4_7; /* offset: 0x017C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG16_CS; /* offset: 0x0180 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG16_ID; /* offset: 0x0184 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG16_BYTE0_3; /* offset: 0x0188 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG16_BYTE4_7; /* offset: 0x018C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG17_CS; /* offset: 0x0190 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG17_ID; /* offset: 0x0194 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG17_BYTE0_3; /* offset: 0x0198 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG17_BYTE4_7; /* offset: 0x019C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG18_CS; /* offset: 0x01A0 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG18_ID; /* offset: 0x01A4 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG18_BYTE0_3; /* offset: 0x01A8 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG18_BYTE4_7; /* offset: 0x01AC size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG19_CS; /* offset: 0x01B0 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG19_ID; /* offset: 0x01B4 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG19_BYTE0_3; /* offset: 0x01B8 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG19_BYTE4_7; /* offset: 0x01BC size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG20_CS; /* offset: 0x01C0 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG20_ID; /* offset: 0x01C4 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG20_BYTE0_3; /* offset: 0x01C8 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG20_BYTE4_7; /* offset: 0x01CC size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG21_CS; /* offset: 0x01D0 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG21_ID; /* offset: 0x01D4 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG21_BYTE0_3; /* offset: 0x01D8 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG21_BYTE4_7; /* offset: 0x01DC size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG22_CS; /* offset: 0x01E0 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG22_ID; /* offset: 0x01E4 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG22_BYTE0_3; /* offset: 0x01E8 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG22_BYTE4_7; /* offset: 0x01EC size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG23_CS; /* offset: 0x01F0 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG23_ID; /* offset: 0x01F4 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG23_BYTE0_3; /* offset: 0x01F8 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG23_BYTE4_7; /* offset: 0x01FC size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG24_CS; /* offset: 0x0200 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG24_ID; /* offset: 0x0204 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG24_BYTE0_3; /* offset: 0x0208 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG24_BYTE4_7; /* offset: 0x020C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG25_CS; /* offset: 0x0210 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG25_ID; /* offset: 0x0214 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG25_BYTE0_3; /* offset: 0x0218 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG25_BYTE4_7; /* offset: 0x021C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG26_CS; /* offset: 0x0220 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG26_ID; /* offset: 0x0224 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG26_BYTE0_3; /* offset: 0x0228 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG26_BYTE4_7; /* offset: 0x022C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG27_CS; /* offset: 0x0230 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG27_ID; /* offset: 0x0234 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG27_BYTE0_3; /* offset: 0x0238 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG27_BYTE4_7; /* offset: 0x023C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG28_CS; /* offset: 0x0240 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG28_ID; /* offset: 0x0244 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG28_BYTE0_3; /* offset: 0x0248 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG28_BYTE4_7; /* offset: 0x024C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG29_CS; /* offset: 0x0250 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG29_ID; /* offset: 0x0254 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG29_BYTE0_3; /* offset: 0x0258 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG29_BYTE4_7; /* offset: 0x025C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG30_CS; /* offset: 0x0260 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG30_ID; /* offset: 0x0264 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG30_BYTE0_3; /* offset: 0x0268 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG30_BYTE4_7; /* offset: 0x026C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG31_CS; /* offset: 0x0270 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG31_ID; /* offset: 0x0274 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG31_BYTE0_3; /* offset: 0x0278 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG31_BYTE4_7; /* offset: 0x027C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG32_CS; /* offset: 0x0280 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG32_ID; /* offset: 0x0284 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG32_BYTE0_3; /* offset: 0x0288 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG32_BYTE4_7; /* offset: 0x028C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG33_CS; /* offset: 0x0290 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG33_ID; /* offset: 0x0294 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG33_BYTE0_3; /* offset: 0x0298 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG33_BYTE4_7; /* offset: 0x029C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG34_CS; /* offset: 0x02A0 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG34_ID; /* offset: 0x02A4 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG34_BYTE0_3; /* offset: 0x02A8 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG34_BYTE4_7; /* offset: 0x02AC size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG35_CS; /* offset: 0x02B0 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG35_ID; /* offset: 0x02B4 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG35_BYTE0_3; /* offset: 0x02B8 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG35_BYTE4_7; /* offset: 0x02BC size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG36_CS; /* offset: 0x02C0 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG36_ID; /* offset: 0x02C4 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG36_BYTE0_3; /* offset: 0x02C8 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG36_BYTE4_7; /* offset: 0x02CC size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG37_CS; /* offset: 0x02D0 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG37_ID; /* offset: 0x02D4 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG37_BYTE0_3; /* offset: 0x02D8 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG37_BYTE4_7; /* offset: 0x02DC size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG38_CS; /* offset: 0x02E0 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG38_ID; /* offset: 0x02E4 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG38_BYTE0_3; /* offset: 0x02E8 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG38_BYTE4_7; /* offset: 0x02EC size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG39_CS; /* offset: 0x02F0 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG39_ID; /* offset: 0x02F4 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG39_BYTE0_3; /* offset: 0x02F8 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG39_BYTE4_7; /* offset: 0x02FC size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG40_CS; /* offset: 0x0300 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG40_ID; /* offset: 0x0304 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG40_BYTE0_3; /* offset: 0x0308 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG40_BYTE4_7; /* offset: 0x030C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG41_CS; /* offset: 0x0310 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG41_ID; /* offset: 0x0314 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG41_BYTE0_3; /* offset: 0x0318 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG41_BYTE4_7; /* offset: 0x031C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG42_CS; /* offset: 0x0320 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG42_ID; /* offset: 0x0324 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG42_BYTE0_3; /* offset: 0x0328 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG42_BYTE4_7; /* offset: 0x032C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG43_CS; /* offset: 0x0330 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG43_ID; /* offset: 0x0334 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG43_BYTE0_3; /* offset: 0x0338 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG43_BYTE4_7; /* offset: 0x033C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG44_CS; /* offset: 0x0340 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG44_ID; /* offset: 0x0344 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG44_BYTE0_3; /* offset: 0x0348 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG44_BYTE4_7; /* offset: 0x034C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG45_CS; /* offset: 0x0350 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG45_ID; /* offset: 0x0354 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG45_BYTE0_3; /* offset: 0x0358 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG45_BYTE4_7; /* offset: 0x035C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG46_CS; /* offset: 0x0360 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG46_ID; /* offset: 0x0364 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG46_BYTE0_3; /* offset: 0x0368 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG46_BYTE4_7; /* offset: 0x036C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG47_CS; /* offset: 0x0370 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG47_ID; /* offset: 0x0374 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG47_BYTE0_3; /* offset: 0x0378 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG47_BYTE4_7; /* offset: 0x037C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG48_CS; /* offset: 0x0380 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG48_ID; /* offset: 0x0384 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG48_BYTE0_3; /* offset: 0x0388 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG48_BYTE4_7; /* offset: 0x038C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG49_CS; /* offset: 0x0390 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG49_ID; /* offset: 0x0394 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG49_BYTE0_3; /* offset: 0x0398 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG49_BYTE4_7; /* offset: 0x039C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG50_CS; /* offset: 0x03A0 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG50_ID; /* offset: 0x03A4 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG50_BYTE0_3; /* offset: 0x03A8 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG50_BYTE4_7; /* offset: 0x03AC size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG51_CS; /* offset: 0x03B0 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG51_ID; /* offset: 0x03B4 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG51_BYTE0_3; /* offset: 0x03B8 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG51_BYTE4_7; /* offset: 0x03BC size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG52_CS; /* offset: 0x03C0 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG52_ID; /* offset: 0x03C4 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG52_BYTE0_3; /* offset: 0x03C8 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG52_BYTE4_7; /* offset: 0x03CC size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG53_CS; /* offset: 0x03D0 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG53_ID; /* offset: 0x03D4 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG53_BYTE0_3; /* offset: 0x03D8 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG53_BYTE4_7; /* offset: 0x03DC size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG54_CS; /* offset: 0x03E0 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG54_ID; /* offset: 0x03E4 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG54_BYTE0_3; /* offset: 0x03E8 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG54_BYTE4_7; /* offset: 0x03EC size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG55_CS; /* offset: 0x03F0 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG55_ID; /* offset: 0x03F4 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG55_BYTE0_3; /* offset: 0x03F8 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG55_BYTE4_7; /* offset: 0x03FC size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG56_CS; /* offset: 0x0400 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG56_ID; /* offset: 0x0404 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG56_BYTE0_3; /* offset: 0x0408 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG56_BYTE4_7; /* offset: 0x040C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG57_CS; /* offset: 0x0410 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG57_ID; /* offset: 0x0414 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG57_BYTE0_3; /* offset: 0x0418 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG57_BYTE4_7; /* offset: 0x041C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG58_CS; /* offset: 0x0420 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG58_ID; /* offset: 0x0424 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG58_BYTE0_3; /* offset: 0x0428 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG58_BYTE4_7; /* offset: 0x042C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG59_CS; /* offset: 0x0430 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG59_ID; /* offset: 0x0434 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG59_BYTE0_3; /* offset: 0x0438 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG59_BYTE4_7; /* offset: 0x043C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG60_CS; /* offset: 0x0440 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG60_ID; /* offset: 0x0444 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG60_BYTE0_3; /* offset: 0x0448 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG60_BYTE4_7; /* offset: 0x044C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG61_CS; /* offset: 0x0450 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG61_ID; /* offset: 0x0454 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG61_BYTE0_3; /* offset: 0x0458 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG61_BYTE4_7; /* offset: 0x045C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG62_CS; /* offset: 0x0460 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG62_ID; /* offset: 0x0464 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG62_BYTE0_3; /* offset: 0x0468 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG62_BYTE4_7; /* offset: 0x046C size: 32 bit */
			/* Message Buffer Control and Status */
			FLEXCAN_MSG_CS_32B_tag MSG63_CS; /* offset: 0x0470 size: 32 bit */
																			 /* Message Buffer Identifier Field */
			FLEXCAN_MSG_ID_32B_tag MSG63_ID; /* offset: 0x0474 size: 32 bit */
																			 /* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG63_BYTE0_3; /* offset: 0x0478 size: 32 bit */
																							/* Message Buffer Data Register */
			FLEXCAN_MSG_DATA_32B_tag MSG63_BYTE4_7; /* offset: 0x047C size: 32 bit */
		};
	};
	int8_t FLEXCAN_reserved_0480_C[1024];
	union {
		/* FLEXCAN_RXIMR0 - FLEXCAN_RXIMR63 - RX Individual Mask Registers */
		FLEXCAN_RXIMR_32B_tag RXIMR[64]; /* offset: 0x0880  (0x0004 x 64) */

		struct {
			/* FLEXCAN_RXIMR0 - FLEXCAN_RXIMR63 - RX Individual Mask Registers */
			FLEXCAN_RXIMR_32B_tag RXIMR0;	 /* offset: 0x0880 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR1;	 /* offset: 0x0884 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR2;	 /* offset: 0x0888 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR3;	 /* offset: 0x088C size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR4;	 /* offset: 0x0890 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR5;	 /* offset: 0x0894 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR6;	 /* offset: 0x0898 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR7;	 /* offset: 0x089C size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR8;	 /* offset: 0x08A0 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR9;	 /* offset: 0x08A4 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR10; /* offset: 0x08A8 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR11; /* offset: 0x08AC size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR12; /* offset: 0x08B0 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR13; /* offset: 0x08B4 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR14; /* offset: 0x08B8 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR15; /* offset: 0x08BC size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR16; /* offset: 0x08C0 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR17; /* offset: 0x08C4 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR18; /* offset: 0x08C8 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR19; /* offset: 0x08CC size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR20; /* offset: 0x08D0 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR21; /* offset: 0x08D4 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR22; /* offset: 0x08D8 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR23; /* offset: 0x08DC size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR24; /* offset: 0x08E0 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR25; /* offset: 0x08E4 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR26; /* offset: 0x08E8 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR27; /* offset: 0x08EC size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR28; /* offset: 0x08F0 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR29; /* offset: 0x08F4 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR30; /* offset: 0x08F8 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR31; /* offset: 0x08FC size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR32; /* offset: 0x0900 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR33; /* offset: 0x0904 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR34; /* offset: 0x0908 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR35; /* offset: 0x090C size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR36; /* offset: 0x0910 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR37; /* offset: 0x0914 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR38; /* offset: 0x0918 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR39; /* offset: 0x091C size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR40; /* offset: 0x0920 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR41; /* offset: 0x0924 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR42; /* offset: 0x0928 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR43; /* offset: 0x092C size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR44; /* offset: 0x0930 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR45; /* offset: 0x0934 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR46; /* offset: 0x0938 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR47; /* offset: 0x093C size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR48; /* offset: 0x0940 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR49; /* offset: 0x0944 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR50; /* offset: 0x0948 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR51; /* offset: 0x094C size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR52; /* offset: 0x0950 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR53; /* offset: 0x0954 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR54; /* offset: 0x0958 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR55; /* offset: 0x095C size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR56; /* offset: 0x0960 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR57; /* offset: 0x0964 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR58; /* offset: 0x0968 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR59; /* offset: 0x096C size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR60; /* offset: 0x0970 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR61; /* offset: 0x0974 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR62; /* offset: 0x0978 size: 32 bit */
			FLEXCAN_RXIMR_32B_tag RXIMR63; /* offset: 0x097C size: 32 bit */
		};
	};
} FLEXCAN_tag;

#define FLEXCAN_A (*(volatile FLEXCAN_tag*)0xFFFC0000UL)
#define FLEXCAN_B (*(volatile FLEXCAN_tag*)0xFFFC4000UL)

/****************************************************************/
/*                                                              */
/* Module: DMA_CH_MUX  */
/*                                                              */
/****************************************************************/

/* Register layout for all registers CHCONFIG... */

typedef union { /* CHCONFIG[0-15] - Channel Configuration Registers */
	vuint8_t R;
	struct {
		vuint8_t ENBL : 1;	 /* DMA Channel Enable */
		vuint8_t TRIG : 1;	 /* DMA Channel Trigger Enable */
		vuint8_t SOURCE : 6; /* DMA Channel Source */
	} B;
} DMA_CH_MUX_CHCONFIG_8B_tag;

typedef struct DMA_CH_MUX_struct_tag { /* start of DMA_CH_MUX_tag */
	union {
		/* CHCONFIG[0-15] - Channel Configuration Registers */
		DMA_CH_MUX_CHCONFIG_8B_tag CHCONFIG[16]; /* offset: 0x0000  (0x0001 x 16) */

		struct {
			/* CHCONFIG[0-15] - Channel Configuration Registers */
			DMA_CH_MUX_CHCONFIG_8B_tag CHCONFIG0;	 /* offset: 0x0000 size: 8 bit */
			DMA_CH_MUX_CHCONFIG_8B_tag CHCONFIG1;	 /* offset: 0x0001 size: 8 bit */
			DMA_CH_MUX_CHCONFIG_8B_tag CHCONFIG2;	 /* offset: 0x0002 size: 8 bit */
			DMA_CH_MUX_CHCONFIG_8B_tag CHCONFIG3;	 /* offset: 0x0003 size: 8 bit */
			DMA_CH_MUX_CHCONFIG_8B_tag CHCONFIG4;	 /* offset: 0x0004 size: 8 bit */
			DMA_CH_MUX_CHCONFIG_8B_tag CHCONFIG5;	 /* offset: 0x0005 size: 8 bit */
			DMA_CH_MUX_CHCONFIG_8B_tag CHCONFIG6;	 /* offset: 0x0006 size: 8 bit */
			DMA_CH_MUX_CHCONFIG_8B_tag CHCONFIG7;	 /* offset: 0x0007 size: 8 bit */
			DMA_CH_MUX_CHCONFIG_8B_tag CHCONFIG8;	 /* offset: 0x0008 size: 8 bit */
			DMA_CH_MUX_CHCONFIG_8B_tag CHCONFIG9;	 /* offset: 0x0009 size: 8 bit */
			DMA_CH_MUX_CHCONFIG_8B_tag CHCONFIG10; /* offset: 0x000A size: 8 bit */
			DMA_CH_MUX_CHCONFIG_8B_tag CHCONFIG11; /* offset: 0x000B size: 8 bit */
			DMA_CH_MUX_CHCONFIG_8B_tag CHCONFIG12; /* offset: 0x000C size: 8 bit */
			DMA_CH_MUX_CHCONFIG_8B_tag CHCONFIG13; /* offset: 0x000D size: 8 bit */
			DMA_CH_MUX_CHCONFIG_8B_tag CHCONFIG14; /* offset: 0x000E size: 8 bit */
			DMA_CH_MUX_CHCONFIG_8B_tag CHCONFIG15; /* offset: 0x000F size: 8 bit */
		};
	};
} DMA_CH_MUX_tag;

#define DMA_CH_MUX (*(volatile DMA_CH_MUX_tag*)0xFFFDC000UL)

/****************************************************************/
/*                                                              */
/* Module: FR  */
/*                                                              */
/****************************************************************/

typedef union { /* Module Version Number */
	vuint16_t R;
	struct {
		vuint16_t CHIVER : 8; /* VERSION NUMBER OF CHI */
		vuint16_t PEVER : 8;	/* VERSION NUMBER OF PE */
	} B;
} FR_MVR_16B_tag;

typedef union { /* Module Configuration Register */
	vuint16_t R;
	struct {
		vuint16_t MEN : 1;	/* Module Enable */
		vuint16_t SBFF : 1; /* System Bus Failure Freeze */
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t SCM : 1; /* single channel device mode */
#else
		vuint16_t SCMD : 1;				 /* deprecated name - please avoid */
#endif
		vuint16_t CHB : 1;		/* Channel B enable */
		vuint16_t CHA : 1;		/* channel A enable */
		vuint16_t SFFE : 1;		/* Sync. frame filter Enable */
		vuint16_t ECCE : 1;		/* ECC Functionlity Enable */
		vuint16_t TMODER : 1; /* Functional Test mode */
		vuint16_t FUM : 1;		/* FIFO Update Mode */
		vuint16_t FAM : 1;		/* FIFO Address Mode */
		vuint16_t : 1;
		vuint16_t CLKSEL : 1; /* Protocol Engine clock source select */
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t BITRATE : 3; /* Bus bit rate */
#else
		vuint16_t PRESCALE : 3;		 /* deprecated name - please avoid */
#endif
		vuint16_t : 1;
	} B;
} FR_MCR_16B_tag;

typedef union { /* SYSTEM MEMORY BASE ADD HIGH REG */
	vuint16_t R;
	struct {
		vuint16_t SMBA_31_16 : 16; /* SYS_MEM_BASE_ADDR[31:16] */
	} B;
} FR_SYMBADHR_16B_tag;

typedef union { /* SYSTEM MEMORY BASE ADD LOW  REG */
	vuint16_t R;
	struct {
		vuint16_t SMBA_15_4 : 12; /* SYS_MEM_BASE_ADDR[15:4] */
		vuint16_t : 4;
	} B;
} FR_SYMBADLR_16B_tag;

typedef union { /* STROBE SIGNAL CONTROL REGISTER */
	vuint16_t R;
	struct {
		vuint16_t WMD : 1; /* DEFINES WRITE MODE OF REG */
		vuint16_t : 3;
		vuint16_t SEL : 4; /* STROBE SIGNSL SELECT */
		vuint16_t : 3;
		vuint16_t ENB : 1; /* STROBE SIGNAL ENABLE */
		vuint16_t : 2;
		vuint16_t STBPSEL : 2; /* STROBE PORT SELECT */
	} B;
} FR_STBSCR_16B_tag;

typedef union { /* MESSAGE BUFFER DATA SIZE REGISTER */
	vuint16_t R;
	struct {
		vuint16_t : 1;
		vuint16_t MBSEG2DS : 7; /* MESSAGE BUFFER SEGMENT 2 DATA SIZE */
		vuint16_t : 1;
		vuint16_t MBSEG1DS : 7; /* MESSAGE BUFFER SEGMENT 1 DATA SIZE */
	} B;
} FR_MBDSR_16B_tag;

typedef union { /* MESS. BUFFER SEG. SIZE & UTILISATION REG */
	vuint16_t R;
	struct {
		vuint16_t : 2;
		vuint16_t LAST_MB_SEG1 : 6; /* LAST MESS BUFFER IN SEG 1 */
		vuint16_t : 2;
		vuint16_t LAST_MB_UTIL : 6; /* LAST MESSAGE BUFFER UTILISED */
	} B;
} FR_MBSSUTR_16B_tag;

typedef union { /* PE DRAM ACCESS REGISTER */
	vuint16_t R;
	struct {
		vuint16_t INST : 4;	 /* PE DRAM ACCESS INSTRUCTION */
		vuint16_t ADDR : 11; /* PE DRAM ACCESS ADDRESS */
		vuint16_t DAD : 1;	 /* PE DRAM ACCESS DONE */
	} B;
} FR_PEDRAR_16B_tag;

typedef union { /* PE DRAM DATA REGISTER */
	vuint16_t R;
	struct {
		vuint16_t DATA : 16; /* DATA TO BE READ OR WRITTEN */
	} B;
} FR_PEDRDR_16B_tag;

typedef union { /* PROTOCOL OPERATION CONTROL REG */
	vuint16_t R;
	struct {
		vuint16_t WME : 1; /* WRITE MODE EXTERNAL CORRECTION */
		vuint16_t : 3;
		vuint16_t EOC_AP : 2; /* EXTERNAL OFFSET CORRECTION APPLICATION */
		vuint16_t ERC_AP : 2; /* EXTERNAL RATE CORRECTION APPLICATION */
		vuint16_t BSY : 1;		/* PROTOCOL CONTROL COMMAND WRITE BUSY */
		vuint16_t : 3;
		vuint16_t POCCMD : 4; /* PROTOCOL CONTROL COMMAND */
	} B;
} FR_POCR_16B_tag;

typedef union { /* GLOBAL INTERRUPT FLAG & ENABLE REG */
	vuint16_t R;
	struct {
		vuint16_t MIF : 1;	/* MODULE INTERRUPT FLAG */
		vuint16_t PRIF : 1; /* PROTOCOL INTERRUPT FLAG */
		vuint16_t CHIF : 1; /* CHI INTERRUPT FLAG */
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t WUPIF : 1; /* WAKEUP INTERRUPT FLAG */
#else
		vuint16_t WKUPIF : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t
				FAFBIF : 1; /* RECEIVE FIFO CHANNEL B ALMOST FULL INTERRUPT FLAG */
#else
		vuint16_t FNEBIF : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t
				FAFAIF : 1; /* RECEIVE FIFO CHANNEL A ALMOST FULL INTERRUPT FLAG */
#else
		vuint16_t FNEAIF : 1;			 /* deprecated name - please avoid */
#endif
		vuint16_t RBIF : 1; /* RECEIVE MESSAGE BUFFER INTERRUPT FLAG */
		vuint16_t TBIF : 1; /* TRANSMIT BUFFER INTERRUPT FLAG */
		vuint16_t MIE : 1;	/* MODULE INTERRUPT ENABLE */
		vuint16_t PRIE : 1; /* PROTOCOL INTERRUPT ENABLE */
		vuint16_t CHIE : 1; /* CHI INTERRUPT ENABLE */
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t WUPIE : 1; /* WAKEUP INTERRUPT ENABLE */
#else
		vuint16_t WKUPIE : 1;			 /* deprecated name - please avoid */
#endif
		vuint16_t
				FNEBIE : 1; /* RECEIVE FIFO CHANNEL B NOT EMPTY INTERRUPT ENABLE */
		vuint16_t
				FNEAIE : 1;			/* RECEIVE FIFO CHANNEL A NOT EMPTY INTERRUPT ENABLE */
		vuint16_t RBIE : 1; /* RECEIVE BUFFER INTERRUPT ENABLE */
		vuint16_t TBIE : 1; /* TRANSMIT BUFFER INTERRUPT ENABLE */
	} B;
} FR_GIFER_16B_tag;

typedef union { /* PROTOCOL INTERRUPT FLAG REGISTER 0 */
	vuint16_t R;
	struct {
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t FATL_IF : 1; /* FATAL PROTOCOL ERROR INTERRUPT FLAG */
#else
		vuint16_t FATLIF : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t INTL_IF : 1; /* INTERNAL PROTOCOL ERROR INTERRUPT FLAG */
#else
		vuint16_t INTLIF : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t ILCF_IF : 1; /* ILLEGAL PROTOCOL CONFIGURATION INTERRUPT FLAG */
#else
		vuint16_t ILCFIF : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t CSA_IF : 1; /* COLDSTART ABORT INTERRUPT FLAG */
#else
		vuint16_t CSAIF : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t MRC_IF : 1; /* MISSING RATE CORRECTION INTERRUPT FLAG */
#else
		vuint16_t MRCIF : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t MOC_IF : 1; /* MISSING OFFSET CORRECTION INTERRUPT FLAG */
#else
		vuint16_t MOCIF : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t CCL_IF : 1; /* CLOCK CORRECTION LIMIT REACHED INTERRUPT FLAG */
#else
		vuint16_t CCLIF : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t MXS_IF : 1; /* MAX SYNC FRAMES DETECTED INTERRUPT FLAG */
#else
		vuint16_t MXSIF : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t MTX_IF : 1; /* MEDIA ACCESS TEST SYMBOL RECEIVED INTERRUPT FLAG */
#else
		vuint16_t MTXIF : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t LTXB_IF : 1; /* pLATESTTX VIOLATION ON CHANNEL B INTERRUPT FLAG */
#else
		vuint16_t LTXBIF : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t LTXA_IF : 1; /* pLATESTTX VIOLATION ON CHANNEL A INTERRUPT FLAG */
#else
		vuint16_t LTXAIF : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t TBVB_IF : 1; /* TRANSMISSION ACROSS BOUNDARY ON CHANNEL B
															INTERRUPT FLAG */
#else
		vuint16_t TBVBIF : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t TBVA_IF : 1; /* TRANSMISSION ACROSS BOUNDARY ON CHANNEL A
															INTERRUPT FLAG */
#else
		vuint16_t TBVAIF : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t TI2_IF : 1; /* TIMER 2 EXPIRED INTERRUPT FLAG */
#else
		vuint16_t TI2IF : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t TI1_IF : 1; /* TIMER 1 EXPIRED INTERRUPT FLAG */
#else
		vuint16_t TI1IF : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t CYS_IF : 1; /* CYCLE START INTERRUPT FLAG */
#else
		vuint16_t CYSIF : 1;			 /* deprecated name - please avoid */
#endif
	} B;
} FR_PIFR0_16B_tag;

typedef union { /* PROTOCOL INTERRUPT FLAG REGISTER 1 */
	vuint16_t R;
	struct {
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t EMC_IF : 1; /* ERROR MODE CHANGED INTERRUPT FLAG */
#else
		vuint16_t EMCIF : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t IPC_IF : 1; /* ILLEGAL PROTOCOL CONTROL COMMAND INTERRUPT FLAG */
#else
		vuint16_t IPCIF : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t
				PECF_IF : 1; /* PROTOCOL ENGINE COMMUNICATION FAILURE INTERRUPT FLAG */
#else
		vuint16_t PECFIF : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t PSC_IF : 1; /* PROTOCOL STATE CHANGED INTERRUPT FLAG */
#else
		vuint16_t PSCIF : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t
				SSI3_IF : 1; /* SLOT STATUS COUNTER 3 INCREMENTED INTERRUPT FLAG */
#else
		vuint16_t SSI3IF : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t
				SSI2_IF : 1; /* SLOT STATUS COUNTER 2 INCREMENTED INTERRUPT FLAG */
#else
		vuint16_t SSI2IF : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t
				SSI1_IF : 1; /* SLOT STATUS COUNTER 1 INCREMENTED INTERRUPT FLAG */
#else
		vuint16_t SSI1IF : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t
				SSI0_IF : 1; /* SLOT STATUS COUNTER 0 INCREMENTED INTERRUPT FLAG */
#else
		vuint16_t SSI0IF : 1;			 /* deprecated name - please avoid */
#endif
		vuint16_t : 2;
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t EVT_IF : 1; /* EVEN CYCLE TABLE WRITTEN INTERRUPT FLAG */
#else
		vuint16_t EVTIF : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t ODT_IF : 1; /* ODD CYCLE TABLE WRITTEN INTERRUPT FLAG */
#else
		vuint16_t ODTIF : 1;			 /* deprecated name - please avoid */
#endif
		vuint16_t : 4;
	} B;
} FR_PIFR1_16B_tag;

typedef union { /* PROTOCOL INTERRUPT ENABLE REGISTER 0 */
	vuint16_t R;
	struct {
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t FATL_IE : 1; /* FATAL PROTOCOL ERROR INTERRUPT ENABLE */
#else
		vuint16_t FATLIE : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t INTL_IE : 1; /* INTERNAL PROTOCOL ERROR INTERRUPT ENABLE */
#else
		vuint16_t INTLIE : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t ILCF_IE : 1; /* ILLEGAL PROTOCOL CONFIGURATION INTERRUPT ENABLE */
#else
		vuint16_t ILCFIE : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t CSA_IE : 1; /* COLDSTART ABORT INTERRUPT ENABLE */
#else
		vuint16_t CSAIE : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t MRC_IE : 1; /* MISSING RATE CORRECTION INTERRUPT ENABLE */
#else
		vuint16_t MRCIE : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t MOC_IE : 1; /* MISSING OFFSET CORRECTION INTERRUPT ENABLE */
#else
		vuint16_t MOCIE : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t CCL_IE : 1; /* CLOCK CORRECTION LIMIT REACHED */
#else
		vuint16_t CCLIE : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t MXS_IE : 1; /* MAX SYNC FRAMES DETECTED INTERRUPT ENABLE */
#else
		vuint16_t MXSIE : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t
				MTX_IE : 1; /* MEDIA ACCESS TEST SYMBOL RECEIVED INTERRUPT ENABLE */
#else
		vuint16_t MTXIE : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t
				LTXB_IE : 1; /* pLATESTTX VIOLATION ON CHANNEL B INTERRUPT ENABLE */
#else
		vuint16_t LTXBIE : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t
				LTXA_IE : 1; /* pLATESTTX VIOLATION ON CHANNEL A INTERRUPT ENABLE */
#else
		vuint16_t LTXAIE : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t TBVB_IE : 1; /* TRANSMISSION ACROSS BOUNDARY ON CHANNEL B
															INTERRUPT ENABLE */
#else
		vuint16_t TBVBIE : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t TBVA_IE : 1; /* TRANSMISSION ACROSS BOUNDARY ON CHANNEL A
															INTERRUPT ENABLE */
#else
		vuint16_t TBVAIE : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t TI2_IE : 1; /* TIMER 2 EXPIRED INTERRUPT ENABLE */
#else
		vuint16_t TI2IE : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t TI1_IE : 1; /* TIMER 1 EXPIRED INTERRUPT ENABLE */
#else
		vuint16_t TI1IE : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t CYS_IE : 1; /* CYCLE START INTERRUPT ENABLE */
#else
		vuint16_t CYSIE : 1;			 /* deprecated name - please avoid */
#endif
	} B;
} FR_PIER0_16B_tag;

typedef union { /* PROTOCOL INTERRUPT ENABLE REGISTER 1 */
	vuint16_t R;
	struct {
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t EMC_IE : 1; /* ERROR MODE CHANGED INTERRUPT Enable */
#else
		vuint16_t EMCIE : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t
				IPC_IE : 1; /* ILLEGAL PROTOCOL CONTROL COMMAND INTERRUPT Enable */
#else
		vuint16_t IPCIE : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t PECF_IE : 1; /* PROTOCOL ENGINE COMMUNICATION FAILURE INTERRUPT
															Enable */
#else
		vuint16_t PECFIE : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t PSC_IE : 1; /* PROTOCOL STATE CHANGED INTERRUPT Enable */
#else
		vuint16_t PSCIE : 1;			 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t
				SSI_3_0_IE : 4; /* SLOT STATUS COUNTER INCREMENTED INTERRUPT Enable */
#else
		vuint16_t SSI3IE : 1;
		vuint16_t SSI2IE : 1;
		vuint16_t SSI1IE : 1;
		vuint16_t SSI0IE : 1;
#endif

		vuint16_t : 2;
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t EVT_IE : 1; /* EVEN CYCLE TABLE WRITTEN INTERRUPT Enable */
#else
		vuint16_t EVTIE : 1;		/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t ODT_IE : 1; /* ODD CYCLE TABLE WRITTEN INTERRUPT Enable */
#else
		vuint16_t ODTIE : 1;		/* deprecated name - please avoid */
#endif
		vuint16_t : 4;
	} B;
} FR_PIER1_16B_tag;

typedef union { /* CHI ERROR FLAG REGISTER */
	vuint16_t R;
	struct {
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t FRLB_EF : 1; /* FRAME LOST CHANNEL B ERROR FLAG */
#else
		vuint16_t FRLBEF : 1;		/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t FRLA_EF : 1; /* FRAME LOST CHANNEL A ERROR FLAG */
#else
		vuint16_t FRLAEF : 1;		/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t PCMI_EF : 1; /* PROTOCOL COMMAND IGNORED ERROR FLAG */
#else
		vuint16_t PCMIEF : 1;		/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t FOVB_EF : 1; /* RECEIVE FIFO OVERRUN CHANNEL B ERROR FLAG */
#else
		vuint16_t FOVBEF : 1;		/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t FOVA_EF : 1; /* RECEIVE FIFO OVERRUN CHANNEL A ERROR FLAG */
#else
		vuint16_t FOVAEF : 1;		/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t MBS_EF : 1; /* MESSAGE BUFFER SEARCH ERROR FLAG */
#else
		vuint16_t MSBEF : 1;		/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t MBU_EF : 1; /* MESSAGE BUFFER UTILIZATION ERROR FLAG */
#else
		vuint16_t MBUEF : 1;		/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t LCK_EF : 1; /* LOCK ERROR FLAG */
#else
		vuint16_t LCKEF : 1;		/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t DBL_EF : 1; /* DOUBLE TRANSMIT MESSAGE BUFFER LOCK ERROR FLAG */
#else
		vuint16_t DBLEF : 1;		/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t SBCF_EF : 1; /* SYSTEM BUS COMMUNICATION FAILURE ERROR FLAG */
#else
		vuint16_t SBCFEF : 1;		/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t FID_EF : 1; /* FRAME ID ERROR FLAG */
#else
		vuint16_t FIDEF : 1;		/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t DPL_EF : 1; /* DYNAMIC PAYLOAD LENGTH ERROR FLAG */
#else
		vuint16_t DPLEF : 1;		/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t SPL_EF : 1; /* STATIC PAYLOAD LENGTH ERROR FLAG */
#else
		vuint16_t SPLEF : 1;		/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t NML_EF : 1; /* NETWORK MANAGEMENT LENGTH ERROR FLAG */
#else
		vuint16_t NMLEF : 1;		/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t NMF_EF : 1; /* NETWORK MANAGEMENT FRAME ERROR FLAG */
#else
		vuint16_t NMFEF : 1;		/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t ILSA_EF : 1; /* ILLEGAL SYSTEM MEMORY ACCESS ERROR FLAG */
#else
		vuint16_t ILSAEF : 1;		/* deprecated name - please avoid */
#endif
	} B;
} FR_CHIERFR_16B_tag;

typedef union { /* Message Buffer Interrupt Vector Register */
	vuint16_t R;
	struct {
		vuint16_t : 2;
		vuint16_t TBIVEC : 6; /* Transmit Buffer Interrupt Vector */
		vuint16_t : 2;
		vuint16_t RBIVEC : 6; /* Receive Buffer Interrupt Vector */
	} B;
} FR_MBIVEC_16B_tag;

typedef union { /* Channel A Status Error Counter Register */
	vuint16_t R;
	struct {
		vuint16_t STATUS_ERR_CNT : 16; /* Channel Status Error Counter */
	} B;
} FR_CASERCR_16B_tag;

typedef union { /* Channel B Status Error Counter Register */
	vuint16_t R;
	struct {
		vuint16_t STATUS_ERR_CNT : 16; /* Channel Status Error Counter */
	} B;
} FR_CBSERCR_16B_tag;

typedef union { /* Protocol Status Register 0 */
	vuint16_t R;
	struct {
		vuint16_t ERRMODE : 2;	/* Error Mode */
		vuint16_t SLOTMODE : 2; /* Slot Mode */
		vuint16_t : 1;
		vuint16_t PROTSTATE : 3; /* Protocol State */
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t STARTUPSTATE : 4; /* Startup State */
#else
		vuint16_t SUBSTATE : 4; /* deprecated name - please avoid */
#endif
		vuint16_t WAKEUPSTATE : 4; /* Wakeup Status */
	} B;
} FR_PSR0_16B_tag;

typedef union { /* Protocol Status Register 1 */
	vuint16_t R;
	struct {
		vuint16_t CSAA : 1; /* Coldstart Attempt Aborted Flag */
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t CSP : 1; /* Leading Coldstart Path */
#else
		vuint16_t SCP : 1;			/* deprecated name - please avoid */
#endif
		vuint16_t : 1;
		vuint16_t REMCSAT : 5; /* Remaining Coldstart Attempts */
		vuint16_t CPN : 1;		 /* Leading Coldstart Path Noise */
		vuint16_t HHR : 1;		 /* Host Halt Request Pending */
		vuint16_t FRZ : 1;		 /* Freeze Occurred */
		vuint16_t APTAC : 5;	 /* Allow Passive to Active Counter */
	} B;
} FR_PSR1_16B_tag;

typedef union { /* Protocol Status Register 2 */
	vuint16_t R;
	struct {
		vuint16_t NBVB : 1; /* NIT Boundary Violation on Channel B */
		vuint16_t NSEB : 1; /* NIT Syntax Error on Channel B */
		vuint16_t STCB : 1; /* Symbol Window Transmit Conflict on Channel B */
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t SSVB : 1; /* Symbol Window Boundary Violation on Channel B */
#else
		vuint16_t SBVB : 1;			/* deprecated name - please avoid */
#endif
		vuint16_t SSEB : 1; /* Symbol Window Syntax Error on Channel B */
		vuint16_t MTB : 1;	/* Media Access Test Symbol MTS Received on Channel B */
		vuint16_t NBVA : 1; /* NIT Boundary Violation on Channel A */
		vuint16_t NSEA : 1; /* NIT Syntax Error on Channel A */
		vuint16_t STCA : 1; /* Symbol Window Transmit Conflict on Channel A */
		vuint16_t SBVA : 1; /* Symbol Window Boundary Violation on Channel A */
		vuint16_t SSEA : 1; /* Symbol Window Syntax Error on Channel A */
		vuint16_t MTA : 1;	/* Media Access Test Symbol MTS Received on Channel A */
		vuint16_t CLKCORRFAILCNT : 4; /* Clock Correction Failed Counter */
	} B;
} FR_PSR2_16B_tag;

typedef union { /* Protocol Status Register 3 */
	vuint16_t R;
	struct {
		vuint16_t : 2;
		vuint16_t WUB : 1;	/* Wakeup Symbol Received on Channel B */
		vuint16_t ABVB : 1; /* Aggregated Boundary Violation on Channel B */
		vuint16_t AACB : 1; /* Aggregated Additional Communication on Channel B */
		vuint16_t ACEB : 1; /* Aggregated Content Error on Channel B */
		vuint16_t ASEB : 1; /* Aggregated Syntax Error on Channel B */
		vuint16_t AVFB : 1; /* Aggregated Valid Frame on Channel B */
		vuint16_t : 2;
		vuint16_t WUA : 1;	/* Wakeup Symbol Received on Channel A */
		vuint16_t ABVA : 1; /* Aggregated Boundary Violation on Channel A */
		vuint16_t AACA : 1; /* Aggregated Additional Communication on Channel A */
		vuint16_t ACEA : 1; /* Aggregated Content Error on Channel A */
		vuint16_t ASEA : 1; /* Aggregated Syntax Error on Channel A */
		vuint16_t AVFA : 1; /* Aggregated Valid Frame on Channel A */
	} B;
} FR_PSR3_16B_tag;

typedef union { /* Macrotick Counter Register */
	vuint16_t R;
	struct {
		vuint16_t : 2;
		vuint16_t MTCT : 14; /* Macrotick Counter */
	} B;
} FR_MTCTR_16B_tag;

typedef union { /* Cycle Counter Register */
	vuint16_t R;
	struct {
		vuint16_t : 10;
		vuint16_t CYCCNT : 6; /* Cycle Counter */
	} B;
} FR_CYCTR_16B_tag;

typedef union { /* Slot Counter Channel A Register */
	vuint16_t R;
	struct {
		vuint16_t : 5;
		vuint16_t SLOTCNTA : 11; /* Slot Counter Value for Channel A */
	} B;
} FR_SLTCTAR_16B_tag;

typedef union { /* Slot Counter Channel B Register */
	vuint16_t R;
	struct {
		vuint16_t : 5;
		vuint16_t SLOTCNTB : 11; /* Slot Counter Value for Channel B */
	} B;
} FR_SLTCTBR_16B_tag;

typedef union { /* Rate Correction Value Register */
	vuint16_t R;
	struct {
		vuint16_t RATECORR : 16; /* Rate Correction Value */
	} B;
} FR_RTCORVR_16B_tag;

typedef union { /* Offset Correction Value Register */
	vuint16_t R;
	struct {
		vuint16_t : 6;
		vuint16_t OFFSETCORR : 10; /* Offset Correction Value */
	} B;
} FR_OFCORVR_16B_tag;

typedef union { /* Combined Interrupt Flag Register */
	vuint16_t R;
	struct {
		vuint16_t : 8;
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t MIF : 1; /* Module Interrupt Flag */
#else
		vuint16_t MIFR : 1;			/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t PRIF : 1; /* Protocol Interrupt Flag */
#else
		vuint16_t PRIFR : 1;		/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t CHIF : 1; /* CHI Interrupt Flag */
#else
		vuint16_t CHIFR : 1;		/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t WUPIF : 1; /* Wakeup Interrupt Flag */
#else
		vuint16_t WUPIFR : 1;		/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t
				FAFBIF : 1; /* Receive FIFO channel B Almost Full Interrupt Flag */
#else
		vuint16_t FNEBIFR : 1;	/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t
				FAFAIF : 1; /* Receive FIFO channel A Almost Full Interrupt Flag */
#else
		vuint16_t FNEAIFR : 1;	/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t RBIF : 1; /* Receive Message Buffer Interrupt Flag */
#else
		vuint16_t RBIFR : 1;		/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t TBIF : 1; /* Transmit Message Buffer Interrupt Flag */
#else
		vuint16_t TBIFR : 1;		/* deprecated name - please avoid */
#endif
	} B;
} FR_CIFR_16B_tag;

typedef union { /* System Memory Access Time-Out Register */
	vuint16_t R;
	struct {
		vuint16_t : 8;
		vuint16_t TIMEOUT : 8; /* Time-Out */
	} B;
} FR_SYMATOR_16B_tag;

typedef union { /* Sync Frame Counter Register */
	vuint16_t R;
	struct {
		vuint16_t SFEVB : 4; /* Sync Frames Channel B, even cycle */
		vuint16_t SFEVA : 4; /* Sync Frames Channel A, even cycle */
		vuint16_t SFODB : 4; /* Sync Frames Channel B, odd cycle */
		vuint16_t SFODA : 4; /* Sync Frames Channel A, odd cycle */
	} B;
} FR_SFCNTR_16B_tag;

typedef union { /* Sync Frame Table Offset Register */
	vuint16_t R;
	struct {
		vuint16_t SFT_OFFSET_15_1 : 15; /* Sync Frame Table Offset */
		vuint16_t : 1;
	} B;
} FR_SFTOR_16B_tag;

typedef union { /* Sync Frame Table Configuration, Control, Status Register */
	vuint16_t R;
	struct {
		vuint16_t ELKT : 1;		/* Even Cycle Tables Lock/Unlock Trigger */
		vuint16_t OLKT : 1;		/* Odd Cycle Tables Lock/Unlock Trigger */
		vuint16_t CYCNUM : 6; /* Cycle Number */
		vuint16_t ELKS : 1;		/* Even Cycle Tables Lock Status */
		vuint16_t OLKS : 1;		/* Odd Cycle Tables Lock Status */
		vuint16_t EVAL : 1;		/* Even Cycle Tables Valid */
		vuint16_t OVAL : 1;		/* Odd Cycle Tables Valid */
		vuint16_t : 1;
		vuint16_t OPT : 1;	 /* One Pair Trigger */
		vuint16_t SDVEN : 1; /* Sync Frame Deviation Table Enable */
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t SIVEN : 1; /* Sync Frame ID Table Enable */
#else
		vuint16_t SIDEN : 1;		/* deprecated name - please avoid */
#endif
	} B;
} FR_SFTCCSR_16B_tag;

typedef union { /* Sync Frame ID Rejection Filter */
	vuint16_t R;
	struct {
		vuint16_t : 6;
		vuint16_t SYNFRID : 10; /* Sync Frame Rejection ID */
	} B;
} FR_SFIDRFR_16B_tag;

typedef union { /* Sync Frame ID Acceptance Filter Value Register */
	vuint16_t R;
	struct {
		vuint16_t : 6;
		vuint16_t FVAL : 10; /* Filter Value */
	} B;
} FR_SFIDAFVR_16B_tag;

typedef union { /* Sync Frame ID Acceptance Filter Mask Register */
	vuint16_t R;
	struct {
		vuint16_t : 6;
		vuint16_t FMSK : 10; /* Filter Mask */
	} B;
} FR_SFIDAFMR_16B_tag;

typedef union { /* Network Management Vector Register0 */
	vuint16_t R;
	struct {
		vuint16_t NMVP_15_8 : 8; /* Network Management Vector Part */
		vuint16_t NMVP_7_0 : 8;	 /* Network Management Vector Part */
	} B;
} FR_NMVR0_16B_tag;

typedef union { /* Network Management Vector Register1 */
	vuint16_t R;
	struct {
		vuint16_t NMVP_15_8 : 8; /* Network Management Vector Part */
		vuint16_t NMVP_7_0 : 8;	 /* Network Management Vector Part */
	} B;
} FR_NMVR1_16B_tag;

typedef union { /* Network Management Vector Register2 */
	vuint16_t R;
	struct {
		vuint16_t NMVP_15_8 : 8; /* Network Management Vector Part */
		vuint16_t NMVP_7_0 : 8;	 /* Network Management Vector Part */
	} B;
} FR_NMVR2_16B_tag;

typedef union { /* Network Management Vector Register3 */
	vuint16_t R;
	struct {
		vuint16_t NMVP_15_8 : 8; /* Network Management Vector Part */
		vuint16_t NMVP_7_0 : 8;	 /* Network Management Vector Part */
	} B;
} FR_NMVR3_16B_tag;

typedef union { /* Network Management Vector Register4 */
	vuint16_t R;
	struct {
		vuint16_t NMVP_15_8 : 8; /* Network Management Vector Part */
		vuint16_t NMVP_7_0 : 8;	 /* Network Management Vector Part */
	} B;
} FR_NMVR4_16B_tag;

typedef union { /* Network Management Vector Register5 */
	vuint16_t R;
	struct {
		vuint16_t NMVP_15_8 : 8; /* Network Management Vector Part */
		vuint16_t NMVP_7_0 : 8;	 /* Network Management Vector Part */
	} B;
} FR_NMVR5_16B_tag;

typedef union { /* Network Management Vector Length Register */
	vuint16_t R;
	struct {
		vuint16_t : 12;
		vuint16_t NMVL : 4; /* Network Management Vector Length */
	} B;
} FR_NMVLR_16B_tag;

typedef union { /* Timer Configuration and Control Register */
	vuint16_t R;
	struct {
		vuint16_t : 2;
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t T2_CFG : 1; /* Timer T2 Configuration */
#else
		vuint16_t T2CFG : 1;		/* Timer T2 Configuration */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t T2_REP : 1; /* Timer T2 Repetitive Mode */
#else
		vuint16_t T2REP : 1;		/* Timer T2 Configuration */
#endif
		vuint16_t : 1;
		vuint16_t T2SP : 1; /* Timer T2 Stop */
		vuint16_t T2TR : 1; /* Timer T2 Trigger */
		vuint16_t T2ST : 1; /* Timer T2 State */
		vuint16_t : 3;
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t T1_REP : 1; /* Timer T1 Repetitive Mode */
#else
		vuint16_t T1REP : 1;
#endif
		vuint16_t : 1;
		vuint16_t T1SP : 1; /* Timer T1 Stop */
		vuint16_t T1TR : 1; /* Timer T1 Trigger */
		vuint16_t T1ST : 1; /* Timer T1 State */
	} B;
} FR_TICCR_16B_tag;

typedef union { /* Timer 1 Cycle Set Register */
	vuint16_t R;
	struct {
		vuint16_t : 2;
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t T1_CYC_VAL : 6; /* Timer T1 Cycle Filter Value */
#else
		vuint16_t TI1CYCVAL : 1; /* Timer T1 Cycle Filter Value */
#endif
		vuint16_t : 2;
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t T1_CYC_MSK : 6; /* Timer T1 Cycle Filter Mask */
#else
		vuint16_t TI1CYCMSK : 1; /* Timer T1 Cycle Filter Mask */
#endif
	} B;
} FR_TI1CYSR_16B_tag;

typedef union { /* Timer 1 Macrotick Offset Register */
	vuint16_t R;
	struct {
		vuint16_t : 2;
		vuint16_t T1_MTOFFSET : 14; /* Timer 1 Macrotick Offset */
	} B;
} FR_TI1MTOR_16B_tag;

typedef union { /* Timer 2 Configuration Register 0 */
	vuint16_t R;
	struct {
		vuint16_t : 2;
		vuint16_t T2_CYC_VAL : 6; /* Timer T2 Cycle Filter Value */
		vuint16_t : 2;
		vuint16_t T2_CYC_MSK : 6; /* Timer T2 Cycle Filter Mask */
	} B;
} FR_TI2CR0_16B_tag;

typedef union { /* Timer 2 Configuration Register 1 */
	vuint16_t R;
	struct {
		vuint16_t T2_MTCNT : 16; /* Timer T2 Macrotick Offset */
	} B;
} FR_TI2CR1_16B_tag;

typedef union { /* Slot Status Selection Register */
	vuint16_t R;
	struct {
		vuint16_t WMD : 1; /* Write Mode */
		vuint16_t : 1;
		vuint16_t SEL : 2; /* Selector */
		vuint16_t : 1;
		vuint16_t SLOTNUMBER : 11; /* Slot Number */
	} B;
} FR_SSSR_16B_tag;

typedef union { /* Slot Status Counter Condition Register */
	vuint16_t R;
	struct {
		vuint16_t WMD : 1; /* Write Mode */
		vuint16_t : 1;
		vuint16_t SEL : 2; /* Selector */
		vuint16_t : 1;
		vuint16_t CNTCFG : 2;			/* Counter Configuration */
		vuint16_t MCY : 1;				/* Multi Cycle Selection */
		vuint16_t VFR : 1;				/* Valid Frame Restriction */
		vuint16_t SYF : 1;				/* Sync Frame Restriction */
		vuint16_t NUF : 1;				/* Null Frame Restriction */
		vuint16_t SUF : 1;				/* Startup Frame Restriction */
		vuint16_t STATUSMASK : 4; /* Slot Status Mask */
	} B;
} FR_SSCCR_16B_tag;

typedef union { /* Slot Status Register0 */
	vuint16_t R;
	struct {
		vuint16_t VFB : 1; /* Valid Frame on Channel B */
		vuint16_t SYB : 1; /* Sync Frame Indicator Channel B */
		vuint16_t NFB : 1; /* Null Frame Indicator Channel B */
		vuint16_t SUB : 1; /* Startup Frame Indicator Channel B */
		vuint16_t SEB : 1; /* Syntax Error on Channel B */
		vuint16_t CEB : 1; /* Content Error on Channel B */
		vuint16_t BVB : 1; /* Boundary Violation on Channel B */
		vuint16_t TCB : 1; /* Transmission Conflict on Channel B */
		vuint16_t VFA : 1; /* Valid Frame on Channel A */
		vuint16_t SYA : 1; /* Sync Frame Indicator Channel A */
		vuint16_t NFA : 1; /* Null Frame Indicator Channel A */
		vuint16_t SUA : 1; /* Startup Frame Indicator Channel A */
		vuint16_t SEA : 1; /* Syntax Error on Channel A */
		vuint16_t CEA : 1; /* Content Error on Channel A */
		vuint16_t BVA : 1; /* Boundary Violation on Channel A */
		vuint16_t TCA : 1; /* Transmission Conflict on Channel A */
	} B;
} FR_SSR_16B_tag;

typedef union { /* Slot Status Counter Register0 */
	vuint16_t R;
	struct {
		vuint16_t SLOTSTSTUSCNT : 16; /* Slot Status Counter */
	} B;
} FR_SSCR0_16B_tag;

typedef union { /* Slot Status Counter Register1 */
	vuint16_t R;
	struct {
		vuint16_t SLOTSTSTUSCNT : 16; /* Slot Status Counter */
	} B;
} FR_SSCR1_16B_tag;

typedef union { /* Slot Status Counter Register2 */
	vuint16_t R;
	struct {
		vuint16_t SLOTSTSTUSCNT : 16; /* Slot Status Counter */
	} B;
} FR_SSCR2_16B_tag;

typedef union { /* Slot Status Counter Register3 */
	vuint16_t R;
	struct {
		vuint16_t SLOTSTSTUSCNT : 16; /* Slot Status Counter */
	} B;
} FR_SSCR3_16B_tag;

typedef union { /* MTS A Configuration Register */
	vuint16_t R;
	struct {
		vuint16_t MTE : 1; /* Media Access Test Symbol Transmission Enable */
		vuint16_t : 1;
		vuint16_t CYCCNTMSK : 6; /* Cycle Counter Mask */
		vuint16_t : 2;
		vuint16_t CYCCNTVAL : 6; /* Cycle Counter Value */
	} B;
} FR_MTSACFR_16B_tag;

typedef union { /* MTS B Configuration Register */
	vuint16_t R;
	struct {
		vuint16_t MTE : 1; /* Media Access Test Symbol Transmission Enable */
		vuint16_t : 1;
		vuint16_t CYCCNTMSK : 6; /* Cycle Counter Mask */
		vuint16_t : 2;
		vuint16_t CYCCNTVAL : 6; /* Cycle Counter Value */
	} B;
} FR_MTSBCFR_16B_tag;

typedef union { /* Receive Shadow Buffer Index Register */
	vuint16_t R;
	struct {
		vuint16_t WMD : 1; /* Write Mode */
		vuint16_t : 1;
		vuint16_t SEL : 2; /* Selector */
		vuint16_t : 5;
		vuint16_t RSBIDX : 7; /* Receive Shadow Buffer Index */
	} B;
} FR_RSBIR_16B_tag;

typedef union { /* Receive FIFO Watermark and Selection Register */
	vuint16_t R;
	struct {
		vuint16_t WM : 8; /* Watermark Value */
		vuint16_t : 7;
		vuint16_t SEL : 1; /* Select */
	} B;
} FR_RFWMSR_16B_tag;

typedef union { /* Receive FIFO Start Index Register */
	vuint16_t R;
	struct {
		vuint16_t : 6;
		vuint16_t SIDX : 10; /* Start Index */
	} B;
} FR_RF_RFSIR_16B_tag;

typedef union { /* Receive FIFO Depth and Size Register */
	vuint16_t R;
	struct {
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t FIFO_DEPTH : 8; /* FIFO Depth */
#else
		vuint16_t FIFODEPTH : 8; /* deprecated name - please avoid */
#endif
		vuint16_t : 1;
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t ENTRY_SIZE : 7; /* Entry Size */
#else
		vuint16_t ENTRYSIZE : 7; /* deprecated name - please avoid */
#endif
	} B;
} FR_RFDSR_16B_tag;

typedef union { /* Receive FIFO A Read Index Register */
	vuint16_t R;
	struct {
		vuint16_t : 6;
		vuint16_t RDIDX : 10; /* Read Index */
	} B;
} FR_RFARIR_16B_tag;

typedef union { /* Receive FIFO B Read Index Register */
	vuint16_t R;
	struct {
		vuint16_t : 6;
		vuint16_t RDIDX : 10; /* Read Index */
	} B;
} FR_RFBRIR_16B_tag;

typedef union { /* Receive FIFO Message ID Acceptance Filter Value Register */
	vuint16_t R;
	struct {
		vuint16_t MIDAFVAL : 16; /* Message ID Acceptance Filter Value */
	} B;
} FR_RFMIDAFVR_16B_tag;

typedef union { /* Receive FIFO Message ID Acceptance Filter Mask Register */
	vuint16_t R;
	struct {
		vuint16_t MIDAFMSK : 16; /* Message ID Acceptance Filter Mask */
	} B;
} FR_RFMIDAFMR_16B_tag;

typedef union { /* Receive FIFO Frame ID Rejection Filter Value Register */
	vuint16_t R;
	struct {
		vuint16_t : 5;
		vuint16_t FIDRFVAL : 11; /* Frame ID Rejection Filter Value */
	} B;
} FR_RFFIDRFVR_16B_tag;

typedef union { /* Receive FIFO Frame ID Rejection Filter Mask Register */
	vuint16_t R;
	struct {
		vuint16_t : 5;
		vuint16_t FIDRFMSK : 11; /* Frame ID Rejection Filter Mask */
	} B;
} FR_RFFIDRFMR_16B_tag;

typedef union { /* Receive FIFO Range Filter Configuration Register */
	vuint16_t R;
	struct {
		vuint16_t WMD : 1; /* Write Mode */
		vuint16_t IBD : 1; /* Interval Boundary */
		vuint16_t SEL : 2; /* Filter Selector */
		vuint16_t : 1;
		vuint16_t SID : 11; /* Slot ID */
	} B;
} FR_RFRFCFR_16B_tag;

typedef union { /* Receive FIFO Range Filter Control Register */
	vuint16_t R;
	struct {
		vuint16_t : 4;
		vuint16_t F3MD : 1; /* Range Filter 3 Mode */
		vuint16_t F2MD : 1; /* Range Filter 2 Mode */
		vuint16_t F1MD : 1; /* Range Filter 1 Mode */
		vuint16_t F0MD : 1; /* Range Filter 0 Mode */
		vuint16_t : 4;
		vuint16_t F3EN : 1; /* Range Filter 3 Enable */
		vuint16_t F2EN : 1; /* Range Filter 2 Enable */
		vuint16_t F1EN : 1; /* Range Filter 1 Enable */
		vuint16_t F0EN : 1; /* Range Filter 0 Enable */
	} B;
} FR_RFRFCTR_16B_tag;

typedef union { /* Last Dynamic Transmit Slot Channel A Register */
	vuint16_t R;
	struct {
		vuint16_t : 5;
		vuint16_t
				LASTDYNTXSLOTA : 11; /* Last Dynamic Transmission Slot Channel A */
	} B;
} FR_LDTXSLAR_16B_tag;

typedef union { /* Last Dynamic Transmit Slot Channel B Register */
	vuint16_t R;
	struct {
		vuint16_t : 5;
		vuint16_t
				LASTDYNTXSLOTB : 11; /* Last Dynamic Transmission Slot Channel B */
	} B;
} FR_LDTXSLBR_16B_tag;

typedef union { /* Protocol Configuration Register 0 */
	vuint16_t R;
	struct {
		vuint16_t ACTION_POINT_OFFSET : 6; /* gdActionPointOffset - 1 */
		vuint16_t STATIC_SLOT_LENGTH : 10; /* gdStaticSlot */
	} B;
} FR_PCR0_16B_tag;

typedef union { /* Protocol Configuration Register 1 */
	vuint16_t R;
	struct {
		vuint16_t : 2;
		vuint16_t
				MACRO_AFTER_FIRST_STATIC_SLOT : 14; /* gMacroPerCycle - gdStaticSlot */
	} B;
} FR_PCR1_16B_tag;

typedef union { /* Protocol Configuration Register 2 */
	vuint16_t R;
	struct {
		vuint16_t
				MINISLOT_AFTER_ACTION_POINT : 6;	 /* gdMinislot -
																							gdMinislotActionPointOffset - 1 */
		vuint16_t NUMBER_OF_STATIC_SLOTS : 10; /* gNumberOfStaticSlots */
	} B;
} FR_PCR2_16B_tag;

typedef union { /* Protocol Configuration Register 3 */
	vuint16_t R;
	struct {
		vuint16_t WAKEUP_SYMBOL_RX_LOW : 6; /* gdWakeupSymbolRxLow */
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t MINISLOT_ACTION_POINT_OFFSET_4_0 : 5; /* gdMinislotActionPointOffset
																											 - 1 */
#else
		vuint16_t
				MINISLOT_ACTION_POINT_OFFSET : 5;	 /* deprecated name - please avoid */
#endif
		vuint16_t COLDSTART_ATTEMPTS : 5; /* gColdstartAttempts */
	} B;
} FR_PCR3_16B_tag;

typedef union { /* Protocol Configuration Register 4 */
	vuint16_t R;
	struct {
		vuint16_t CAS_RX_LOW_MAX : 7;					 /* gdCASRxLowMax - 1 */
		vuint16_t WAKEUP_SYMBOL_RX_WINDOW : 9; /* gdWakeupSymbolRxWindow */
	} B;
} FR_PCR4_16B_tag;

typedef union { /* Protocol Configuration Register 5 */
	vuint16_t R;
	struct {
		vuint16_t TSS_TRANSMITTER : 4;			 /* gdTSSTransmitter */
		vuint16_t WAKEUP_SYMBOL_TX_LOW : 6;	 /* gdWakeupSymbolTxLow */
		vuint16_t WAKEUP_SYMBOL_RX_IDLE : 6; /* gdWakeupSymbolRxIdle */
	} B;
} FR_PCR5_16B_tag;

typedef union { /* Protocol Configuration Register 6 */
	vuint16_t R;
	struct {
		vuint16_t : 1;
		vuint16_t
				SYMBOL_WINDOW_AFTER_ACTION_POINT : 8; /* gdSymbolWindow -
																								 gdActionPointOffset - 1 */
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t MACRO_INITIAL_OFFSET_A : 7; /* pMacroInitialOffset[A] */
#else
		vuint16_t MICRO_INITIAL_OFFSET_A : 7;	 /* deprecated name - please avoid */
#endif
	} B;
} FR_PCR6_16B_tag;

typedef union { /* Protocol Configuration Register 7 */
	vuint16_t R;
	struct {
		vuint16_t DECODING_CORRECTION_B : 9;		/* pDecodingCorrection +
																							 pDelayCompensation[B] + 2 */
		vuint16_t MICRO_PER_MACRO_NOM_HALF : 7; /* round(pMicroPerMacroNom / 2) */
	} B;
} FR_PCR7_16B_tag;

typedef union { /* Protocol Configuration Register 8 */
	vuint16_t R;
	struct {
		vuint16_t MAX_WITHOUT_CLOCK_CORRECTION_FATAL : 4; /* gMaxWithoutClockCorrectionFatal
																											 */
		vuint16_t MAX_WITHOUT_CLOCK_CORRECTION_PASSIVE : 4; /* gMaxWithoutClockCorrectionPassive
																												 */
		vuint16_t WAKEUP_SYMBOL_TX_IDLE : 8; /* gdWakeupSymbolTxIdle */
	} B;
} FR_PCR8_16B_tag;

typedef union { /* Protocol Configuration Register 9 */
	vuint16_t R;
	struct {
		vuint16_t MINISLOT_EXISTS : 1;				/* gNumberOfMinislots!=0 */
		vuint16_t SYMBOL_WINDOW_EXISTS : 1;		/* gdSymbolWindow!=0 */
		vuint16_t OFFSET_CORRECTION_OUT : 14; /* pOffsetCorrectionOut */
	} B;
} FR_PCR9_16B_tag;

typedef union { /* Protocol Configuration Register 10 */
	vuint16_t R;
	struct {
		vuint16_t SINGLE_SLOT_ENABLED : 1; /* pSingleSlotEnabled */
		vuint16_t WAKEUP_CHANNEL : 1;			 /* pWakeupChannel */
		vuint16_t MACRO_PER_CYCLE : 14;		 /* pMicroPerCycle */
	} B;
} FR_PCR10_16B_tag;

typedef union { /* Protocol Configuration Register 11 */
	vuint16_t R;
	struct {
		vuint16_t KEY_SLOT_USED_FOR_STARTUP : 1; /* pKeySlotUsedForStartup */
		vuint16_t KEY_SLOT_USED_FOR_SYNC : 1;		 /* pKeySlotUsedForSync */
		vuint16_t OFFSET_CORRECTION_START : 14;	 /* gOffsetCorrectionStart */
	} B;
} FR_PCR11_16B_tag;

typedef union { /* Protocol Configuration Register 12 */
	vuint16_t R;
	struct {
		vuint16_t ALLOW_PASSIVE_TO_ACTIVE : 5; /* pAllowPassiveToActive */
		vuint16_t KEY_SLOT_HEADER_CRC : 11;		 /* header CRC for key slot */
	} B;
} FR_PCR12_16B_tag;

typedef union { /* Protocol Configuration Register 13 */
	vuint16_t R;
	struct {
		vuint16_t FIRST_MINISLOT_ACTION_POINT_OFFSET : 6; /* max(gdActionPointOffset,gdMinislotActionPointOffset)
																												 - 1 */
		vuint16_t STATIC_SLOT_AFTER_ACTION_POINT : 10; /* gdStaticSlot -
																											gdActionPointOffset - 1 */
	} B;
} FR_PCR13_16B_tag;

typedef union { /* Protocol Configuration Register 14 */
	vuint16_t R;
	struct {
		vuint16_t RATE_CORRECTION_OUT : 11; /* pRateCorrectionOut */
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t LISTEN_TIMEOUT_20_16 : 5; /* pdListenTimeout - 1 */
#else
		vuint16_t LISTEN_TIMEOUT_H : 5;				 /* deprecated name - please avoid */
#endif
	} B;
} FR_PCR14_16B_tag;

typedef union { /* Protocol Configuration Register 15 */
	vuint16_t R;
	struct {
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t LISTEN_TIMEOUT_15_0 : 16; /* pdListenTimeout - 1 */
#else
		vuint16_t LISTEN_TIMEOUT_L : 16;			 /* deprecated name - please avoid */
#endif
	} B;
} FR_PCR15_16B_tag;

typedef union { /* Protocol Configuration Register 16 */
	vuint16_t R;
	struct {
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t MACRO_INITIAL_OFFSET_B : 7; /* pMacroInitialOffset[B] */
#else
		vuint16_t MICRO_INITIAL_OFFSET_B : 7;	 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t NOISE_LISTEN_TIMEOUT_24_16 : 9; /* (gListenNoise *
																								 pdListenTimeout) - 1 */
#else
		vuint16_t NOISE_LISTEN_TIMEOUT_H : 9;	 /* deprecated name - please avoid */
#endif
	} B;
} FR_PCR16_16B_tag;

typedef union { /* Protocol Configuration Register 17 */
	vuint16_t R;
	struct {
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t NOISE_LISTEN_TIMEOUT_15_0 : 16; /* (gListenNoise *
																								 pdListenTimeout) - 1 */
#else
		vuint16_t NOISE_LISTEN_TIMEOUT_L : 16; /* deprecated name - please avoid */
#endif
	} B;
} FR_PCR17_16B_tag;

typedef union { /* Protocol Configuration Register 18 */
	vuint16_t R;
	struct {
		vuint16_t WAKEUP_PATTERN : 6; /* pWakeupPattern */
		vuint16_t KEY_SLOT_ID : 10;		/* pKeySlotId */
	} B;
} FR_PCR18_16B_tag;

typedef union { /* Protocol Configuration Register 19 */
	vuint16_t R;
	struct {
		vuint16_t DECODING_CORRECTION_A : 9; /* pDecodingCorrection +
																						pDelayCompensation[A] + 2 */
		vuint16_t PAYLOAD_LENGTH_STATIC : 7; /* gPayloadLengthStatic */
	} B;
} FR_PCR19_16B_tag;

typedef union { /* Protocol Configuration Register 20 */
	vuint16_t R;
	struct {
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t MACRO_INITIAL_OFFSET_B : 8; /* pMicroInitialOffset[B] */
#else
		vuint16_t MICRO_INITIAL_OFFSET_B : 8;	 /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t MACRO_INITIAL_OFFSET_A : 8; /* pMicroInitialOffset[A] */
#else
		vuint16_t MICRO_INITIAL_OFFSET_A : 8;	 /* deprecated name - please avoid */
#endif
	} B;
} FR_PCR20_16B_tag;

typedef union { /* Protocol Configuration Register 21 */
	vuint16_t R;
	struct {
		vuint16_t EXTERN_RATE_CORRECTION : 3; /* pExternRateCorrection */
		vuint16_t LATEST_TX : 13;							/* gNumberOfMinislots - pLatestTx */
	} B;
} FR_PCR21_16B_tag;

typedef union { /* Protocol Configuration Register 22 */
	vuint16_t R;
	struct {
		vuint16_t R : 1; /* Reserved bit */
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t COMP_ACCEPTED_STARRUP_RANGE_A : 11; /* pdAcceptedStartupRange -
																										 pDelayCompensationChA */
#else
		vuint16_t
				COMP_ACCEPTED_STARTUP_RANGE_A : 11; /* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t MICRO_PER_CYCLE_19_16 : 4; /* gMicroPerCycle */
#else
		vuint16_t MICRO_PER_CYCLE_H : 4;			/* deprecated name - please avoid */
#endif
	} B;
} FR_PCR22_16B_tag;

typedef union { /* Protocol Configuration Register 23 */
	vuint16_t R;
	struct {
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t MICRO_PER_CYCLE_15_0 : 16; /* pMicroPerCycle */
#else
		vuint16_t micro_per_cycle_l : 16;			/* deprecated name - please avoid */
#endif
	} B;
} FR_PCR23_16B_tag;

typedef union { /* Protocol Configuration Register 24 */
	vuint16_t R;
	struct {
		vuint16_t CLUSTER_DRIFT_DAMPING : 5;			/* pClusterDriftDamping */
		vuint16_t MAX_PAYLOAD_LENGTH_DYNAMIC : 7; /* pPayloadLengthDynMax */
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t MICRO_PER_CYCLE_MIN_19_16 : 4; /* pMicroPerCycle - pdMaxDrift */
#else
		vuint16_t MICRO_PER_CYCLE_MIN_H : 4;	/* deprecated name - please avoid */
#endif
	} B;
} FR_PCR24_16B_tag;

typedef union { /* Protocol Configuration Register 25 */
	vuint16_t R;
	struct {
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t MICRO_PER_CYCLE_MIN_15_0 : 16; /* pMicroPerCycle - pdMaxDrift */
#else
		vuint16_t MICRO_PER_CYCLE_MIN_L : 16; /* deprecated name - please avoid */
#endif
	} B;
} FR_PCR25_16B_tag;

typedef union { /* Protocol Configuration Register 26 */
	vuint16_t R;
	struct {
		vuint16_t ALLOW_HALT_DUE_TO_CLOCK : 1;				/* pAllowHaltDueToClock */
		vuint16_t COMP_ACCEPTED_STARTUP_RANGE_B : 11; /* pdAcceptedStartupRange -
																										 pDelayCompensationChB */
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t MICRO_PER_CYCLE_MAX_19_16 : 4; /* pMicroPerCycle + pdMaxDrift */
#else
		vuint16_t MICRO_PER_CYCLE_MAX_H : 4;	/* deprecated name - please avoid */
#endif
	} B;
} FR_PCR26_16B_tag;

typedef union { /* Protocol Configuration Register 27 */
	vuint16_t R;
	struct {
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t MICRO_PER_CYCLE_MAX_15_0 : 16; /* pMicroPerCycle + pdMaxDrift */
#else
		vuint16_t MICRO_PER_CYCLE_MAX_L : 16; /* deprecated name - please avoid */
#endif
	} B;
} FR_PCR27_16B_tag;

typedef union { /* Protocol Configuration Register 28 */
	vuint16_t R;
	struct {
		vuint16_t DYNAMIC_SLOT_IDLE_PHASE : 2;				/* gdDynamicSlotIdlePhase */
		vuint16_t MACRO_AFTER_OFFSET_CORRECTION : 14; /* gMacroPerCycle -
																										 gOffsetCorrectionStart */
	} B;
} FR_PCR28_16B_tag;

typedef union { /* Protocol Configuration Register 29 */
	vuint16_t R;
	struct {
		vuint16_t EXTERN_OFFSET_CORRECTION : 3; /* pExternOffsetCorrection */
		vuint16_t MINISLOTS_MAX : 13;						/* gNumberOfMinislots - 1 */
	} B;
} FR_PCR29_16B_tag;

typedef union { /* Protocol Configuration Register 30 */
	vuint16_t R;
	struct {
		vuint16_t : 12;
		vuint16_t SYNC_NODE_MAX : 4; /* gSyncNodeMax */
	} B;
} FR_PCR30_16B_tag;

typedef union { /* Receive FIFO System Memory Base Address High Register */
	vuint16_t R;
	struct {
		vuint16_t SMBA_31_16 : 16; /* System Memory Base Address */
	} B;
} FR_RFSYMBHADR_16B_tag;

typedef union { /* Receive FIFO System Memory Base Address Low Register */
	vuint16_t R;
	struct {
		vuint16_t : 4;
		vuint16_t SMBA_15_4 : 12; /* System Memory Base Address */
	} B;
} FR_RFSYMBLADR_16B_tag;

typedef union { /* Receive FIFO Periodic Timer Register */
	vuint16_t R;
	struct {
		vuint16_t : 2;
		vuint16_t PTD : 14; /* Periodic Timer Duration */
	} B;
} FR_RFPTR_16B_tag;

typedef union { /* Receive FIFO Fill Level and Pop Count Register */
	vuint16_t R;
	struct {
		vuint16_t FLPCB : 8; /* Fill Level and Pop Count Channel B */
		vuint16_t FLPCA : 8; /* Fill Level and Pop Count Channel A */
	} B;
} FR_RFFLPCR_16B_tag;

typedef union { /* ECC Error Interrupt Flag and Enable Register */
	vuint16_t R;
	struct {
		vuint16_t LRNE_OF : 1; /* LRAM Non-Corrected Error Overflow Flag */
		vuint16_t LRCE_OF : 1; /* LRAM Corrected Error Overflow Flag */
		vuint16_t DRNE_OF : 1; /* DRAM Non-Corrected Error Overflow Flag */
		vuint16_t DRCE_OF : 1; /* DRAM Corrected Error Overflow Flag */
		vuint16_t LRNE_IF : 1; /* LRAM Non-Corrected Error Interrupt Flag */
		vuint16_t LRCE_IF : 1; /* LRAM Corrected Error Interrupt Flag */
		vuint16_t DRNE_IF : 1; /* DRAM Non-Corrected Error Interrupt Flag */
		vuint16_t DRCE_IF : 1; /* DRAM Corrected Error Interrupt Flag */
		vuint16_t : 4;
		vuint16_t LRNE_IE : 1; /* LRAM Non-Corrected Error Interrupt Enable */
		vuint16_t LRCE_IE : 1; /* LRAM Corrected Error Interrupt Enable */
		vuint16_t DRNE_IE : 1; /* DRAM Non-Corrected Error Interrupt Enable */
		vuint16_t DRCE_IE : 1; /* DRAM Corrected Error Interrupt Enable */
	} B;
} FR_EEIFER_16B_tag;

typedef union { /* ECC Error Report and Injection Control Register */
	vuint16_t R;
	struct {
		vuint16_t BSY : 1; /* Register Update Busy */
		vuint16_t : 5;
		vuint16_t ERS : 2; /* Error Report Select */
		vuint16_t : 3;
		vuint16_t ERM : 1; /* Error Report Mode */
		vuint16_t : 2;
		vuint16_t EIM : 1; /* Error Injection Mode */
		vuint16_t EIE : 1; /* Error Injection Enable */
	} B;
} FR_EERICR_16B_tag;

typedef union { /* ECC Error Report Adress Register */
	vuint16_t R;
	struct {
		vuint16_t MID : 1;	 /* Memory Identifier */
		vuint16_t BANK : 3;	 /* Memory Bank */
		vuint16_t ADDR : 12; /* Memory Address */
	} B;
} FR_EERAR_16B_tag;

typedef union { /* ECC Error Report Data Register */
	vuint16_t R;
	struct {
		vuint16_t DATA : 16; /* Data */
	} B;
} FR_EERDR_16B_tag;

typedef union { /* ECC Error Report Code Register */
	vuint16_t R;
	struct {
		vuint16_t : 11;
		vuint16_t CODE : 5; /* Code */
	} B;
} FR_EERCR_16B_tag;

typedef union { /* ECC Error Injection Address Register */
	vuint16_t R;
	struct {
		vuint16_t MID : 1;	 /* Memory Identifier */
		vuint16_t BANK : 3;	 /* Memory Bank */
		vuint16_t ADDR : 12; /* Memory Address */
	} B;
} FR_EEIAR_16B_tag;

typedef union { /* ECC Error Injection Data Register */
	vuint16_t R;
	struct {
		vuint16_t DATA : 16; /* Data */
	} B;
} FR_EEIDR_16B_tag;

typedef union { /* ECC Error Injection Code Register */
	vuint16_t R;
	struct {
		vuint16_t : 11;
		vuint16_t CODE : 5; /* Code */
	} B;
} FR_EEICR_16B_tag;

/* Register layout for all registers MBCCSR... */

typedef union { /* Message Buffer Configuration Control Status Register */
	vuint16_t R;
	struct {
		vuint16_t : 1;
		vuint16_t MCM : 1;	/* Message Buffer Commit Mode */
		vuint16_t MBT : 1;	/* Message Buffer Type */
		vuint16_t MTD : 1;	/* Message Buffer Transfer Direction */
		vuint16_t CMT : 1;	/* Commit for Transmission */
		vuint16_t EDT : 1;	/* Enable/Disable Trigger */
		vuint16_t LCKT : 1; /* Lock/Unlock Trigger */
		vuint16_t MBIE : 1; /* Message Buffer Interrupt Enable */
		vuint16_t : 3;
		vuint16_t DUP : 1;	/* Data Updated */
		vuint16_t DVAL : 1; /* DataValid */
		vuint16_t EDS : 1;	/* Enable/Disable Status */
		vuint16_t LCKS : 1; /* LockStatus */
		vuint16_t MBIF : 1; /* Message Buffer Interrupt Flag */
	} B;
} FR_MBCCSR_16B_tag;

/* Register layout for all registers MBCCFR... */

typedef union { /* Message Buffer Cycle Counter Filter Register */
	vuint16_t R;
	struct {
		vuint16_t MTM : 1; /* Message Buffer Transmission Mode */
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t CHA : 1; /* Channel Assignment */
#else
		vuint16_t CHNLA : 1;									/* deprecated name - please avoid */
#endif
#ifndef USE_FIELD_ALIASES_FR
		vuint16_t CHB : 1; /* Channel Assignment */
#else
		vuint16_t CHNLB : 1;									/* deprecated name - please avoid */
#endif
		vuint16_t CCFE : 1;		/* Cycle Counter Filtering Enable */
		vuint16_t CCFMSK : 6; /* Cycle Counter Filtering Mask */
		vuint16_t CCFVAL : 6; /* Cycle Counter Filtering Value */
	} B;
} FR_MBCCFR_16B_tag;

/* Register layout for all registers MBFIDR... */

typedef union { /* Message Buffer Frame ID Register */
	vuint16_t R;
	struct {
		vuint16_t : 5;
		vuint16_t FID : 11; /* Frame ID */
	} B;
} FR_MBFIDR_16B_tag;

/* Register layout for all registers MBIDXR... */

typedef union { /* Message Buffer Index Register */
	vuint16_t R;
	struct {
		vuint16_t : 9;
		vuint16_t MBIDX : 7; /* Message Buffer Index */
	} B;
} FR_MBIDXR_16B_tag;

/* Register layout for generated register(s) NMVR... */

typedef union { /*  */
	vuint16_t R;
} FR_NMVR_16B_tag;

/* Register layout for generated register(s) SSCR... */

typedef union { /*  */
	vuint16_t R;
} FR_SSCR_16B_tag;

typedef struct FR_MB_struct_tag {
	/* Message Buffer Configuration Control Status Register */
	FR_MBCCSR_16B_tag MBCCSR; /* relative offset: 0x0000 */
														/* Message Buffer Cycle Counter Filter Register */
	FR_MBCCFR_16B_tag MBCCFR; /* relative offset: 0x0002 */
														/* Message Buffer Frame ID Register */
	FR_MBFIDR_16B_tag MBFIDR; /* relative offset: 0x0004 */
														/* Message Buffer Index Register */
	FR_MBIDXR_16B_tag MBIDXR; /* relative offset: 0x0006 */

} FR_MB_tag;

typedef struct FR_struct_tag { /* start of FR_tag */
															 /* Module Version Number */
	FR_MVR_16B_tag MVR;					 /* offset: 0x0000 size: 16 bit */
															 /* Module Configuration Register */
	FR_MCR_16B_tag MCR;					 /* offset: 0x0002 size: 16 bit */
	union {
		FR_SYMBADHR_16B_tag SYSBADHR; /* deprecated - please avoid */

		/* SYSTEM MEMORY BASE ADD HIGH REG */
		FR_SYMBADHR_16B_tag SYMBADHR; /* offset: 0x0004 size: 16 bit */
	};
	union {
		FR_SYMBADLR_16B_tag SYSBADLR; /* deprecated - please avoid */

		/* SYSTEM MEMORY BASE ADD LOW  REG */
		FR_SYMBADLR_16B_tag SYMBADLR; /* offset: 0x0006 size: 16 bit */
	};
	/* STROBE SIGNAL CONTROL REGISTER */
	FR_STBSCR_16B_tag STBSCR; /* offset: 0x0008 size: 16 bit */
	int8_t FR_reserved_000A[2];
	/* MESSAGE BUFFER DATA SIZE REGISTER */
	FR_MBDSR_16B_tag MBDSR;			/* offset: 0x000C size: 16 bit */
															/* MESS. BUFFER SEG. SIZE & UTILISATION REG */
	FR_MBSSUTR_16B_tag MBSSUTR; /* offset: 0x000E size: 16 bit */
	union {
		/* PE DRAM ACCESS REGISTER */
		FR_PEDRAR_16B_tag PEDRAR; /* offset: 0x0010 size: 16 bit */

		FR_PEDRAR_16B_tag PADR; /* deprecated - please avoid */
	};
	union {
		/* PE DRAM DATA REGISTER */
		FR_PEDRDR_16B_tag PEDRDR; /* offset: 0x0012 size: 16 bit */

		FR_PEDRDR_16B_tag PDAR; /* deprecated - please avoid */
	};
	/* PROTOCOL OPERATION CONTROL REG */
	FR_POCR_16B_tag POCR;				/* offset: 0x0014 size: 16 bit */
															/* GLOBAL INTERRUPT FLAG & ENABLE REG */
	FR_GIFER_16B_tag GIFER;			/* offset: 0x0016 size: 16 bit */
															/* PROTOCOL INTERRUPT FLAG REGISTER 0 */
	FR_PIFR0_16B_tag PIFR0;			/* offset: 0x0018 size: 16 bit */
															/* PROTOCOL INTERRUPT FLAG REGISTER 1 */
	FR_PIFR1_16B_tag PIFR1;			/* offset: 0x001A size: 16 bit */
															/* PROTOCOL INTERRUPT ENABLE REGISTER 0 */
	FR_PIER0_16B_tag PIER0;			/* offset: 0x001C size: 16 bit */
															/* PROTOCOL INTERRUPT ENABLE REGISTER 1 */
	FR_PIER1_16B_tag PIER1;			/* offset: 0x001E size: 16 bit */
															/* CHI ERROR FLAG REGISTER */
	FR_CHIERFR_16B_tag CHIERFR; /* offset: 0x0020 size: 16 bit */
															/* Message Buffer Interrupt Vector Register */
	FR_MBIVEC_16B_tag MBIVEC;		/* offset: 0x0022 size: 16 bit */
															/* Channel A Status Error Counter Register */
	FR_CASERCR_16B_tag CASERCR; /* offset: 0x0024 size: 16 bit */
															/* Channel B Status Error Counter Register */
	FR_CBSERCR_16B_tag CBSERCR; /* offset: 0x0026 size: 16 bit */
															/* Protocol Status Register 0 */
	FR_PSR0_16B_tag PSR0;				/* offset: 0x0028 size: 16 bit */
															/* Protocol Status Register 1 */
	FR_PSR1_16B_tag PSR1;				/* offset: 0x002A size: 16 bit */
															/* Protocol Status Register 2 */
	FR_PSR2_16B_tag PSR2;				/* offset: 0x002C size: 16 bit */
															/* Protocol Status Register 3 */
	FR_PSR3_16B_tag PSR3;				/* offset: 0x002E size: 16 bit */
															/* Macrotick Counter Register */
	FR_MTCTR_16B_tag MTCTR;			/* offset: 0x0030 size: 16 bit */
															/* Cycle Counter Register */
	FR_CYCTR_16B_tag CYCTR;			/* offset: 0x0032 size: 16 bit */
															/* Slot Counter Channel A Register */
	FR_SLTCTAR_16B_tag SLTCTAR; /* offset: 0x0034 size: 16 bit */
															/* Slot Counter Channel B Register */
	FR_SLTCTBR_16B_tag SLTCTBR; /* offset: 0x0036 size: 16 bit */
															/* Rate Correction Value Register */
	FR_RTCORVR_16B_tag RTCORVR; /* offset: 0x0038 size: 16 bit */
															/* Offset Correction Value Register */
	FR_OFCORVR_16B_tag OFCORVR; /* offset: 0x003A size: 16 bit */
	union {
		FR_CIFR_16B_tag CIFRR; /* deprecated - please avoid */

		/* Combined Interrupt Flag Register */
		FR_CIFR_16B_tag CIFR; /* offset: 0x003C size: 16 bit */
	};
	/* System Memory Access Time-Out Register */
	FR_SYMATOR_16B_tag SYMATOR; /* offset: 0x003E size: 16 bit */
															/* Sync Frame Counter Register */
	FR_SFCNTR_16B_tag SFCNTR;		/* offset: 0x0040 size: 16 bit */
															/* Sync Frame Table Offset Register */
	FR_SFTOR_16B_tag SFTOR;			/* offset: 0x0042 size: 16 bit */
	/* Sync Frame Table Configuration, Control, Status Register */
	FR_SFTCCSR_16B_tag SFTCCSR; /* offset: 0x0044 size: 16 bit */
															/* Sync Frame ID Rejection Filter */
	FR_SFIDRFR_16B_tag SFIDRFR; /* offset: 0x0046 size: 16 bit */
	/* Sync Frame ID Acceptance Filter Value Register */
	FR_SFIDAFVR_16B_tag SFIDAFVR; /* offset: 0x0048 size: 16 bit */
	/* Sync Frame ID Acceptance Filter Mask Register */
	FR_SFIDAFMR_16B_tag SFIDAFMR; /* offset: 0x004A size: 16 bit */
	union {
		FR_NMVR_16B_tag NMVR[6]; /* offset: 0x004C  (0x0002 x 6) */

		struct {
			/* Network Management Vector Register0 */
			FR_NMVR0_16B_tag NMVR0; /* offset: 0x004C size: 16 bit */
															/* Network Management Vector Register1 */
			FR_NMVR1_16B_tag NMVR1; /* offset: 0x004E size: 16 bit */
															/* Network Management Vector Register2 */
			FR_NMVR2_16B_tag NMVR2; /* offset: 0x0050 size: 16 bit */
															/* Network Management Vector Register3 */
			FR_NMVR3_16B_tag NMVR3; /* offset: 0x0052 size: 16 bit */
															/* Network Management Vector Register4 */
			FR_NMVR4_16B_tag NMVR4; /* offset: 0x0054 size: 16 bit */
															/* Network Management Vector Register5 */
			FR_NMVR5_16B_tag NMVR5; /* offset: 0x0056 size: 16 bit */
		};
	};
	/* Network Management Vector Length Register */
	FR_NMVLR_16B_tag NMVLR;			/* offset: 0x0058 size: 16 bit */
															/* Timer Configuration and Control Register */
	FR_TICCR_16B_tag TICCR;			/* offset: 0x005A size: 16 bit */
															/* Timer 1 Cycle Set Register */
	FR_TI1CYSR_16B_tag TI1CYSR; /* offset: 0x005C size: 16 bit */
	union {
		/* Timer 1 Macrotick Offset Register */
		FR_TI1MTOR_16B_tag TI1MTOR; /* offset: 0x005E size: 16 bit */

		FR_TI1MTOR_16B_tag T1MTOR; /* deprecated - please avoid */
	};
	/* Timer 2 Configuration Register 0 */
	FR_TI2CR0_16B_tag TI2CR0; /* offset: 0x0060 size: 16 bit */
														/* Timer 2 Configuration Register 1 */
	FR_TI2CR1_16B_tag TI2CR1; /* offset: 0x0062 size: 16 bit */
														/* Slot Status Selection Register */
	FR_SSSR_16B_tag SSSR;			/* offset: 0x0064 size: 16 bit */
														/* Slot Status Counter Condition Register */
	FR_SSCCR_16B_tag SSCCR;		/* offset: 0x0066 size: 16 bit */
	union {
		FR_SSR_16B_tag SSR[8]; /* offset: 0x0068  (0x0002 x 8) */

		struct {
			/* Slot Status Register0 */
			FR_SSR_16B_tag SSR0; /* offset: 0x0068 size: 16 bit */
													 /* Slot Status Register1 */
			FR_SSR_16B_tag SSR1; /* offset: 0x006A size: 16 bit */
													 /* Slot Status Register2 */
			FR_SSR_16B_tag SSR2; /* offset: 0x006C size: 16 bit */
													 /* Slot Status Register3 */
			FR_SSR_16B_tag SSR3; /* offset: 0x006E size: 16 bit */
													 /* Slot Status Register4 */
			FR_SSR_16B_tag SSR4; /* offset: 0x0070 size: 16 bit */
													 /* Slot Status Register5 */
			FR_SSR_16B_tag SSR5; /* offset: 0x0072 size: 16 bit */
													 /* Slot Status Register6 */
			FR_SSR_16B_tag SSR6; /* offset: 0x0074 size: 16 bit */
													 /* Slot Status Register7 */
			FR_SSR_16B_tag SSR7; /* offset: 0x0076 size: 16 bit */
		};
	};
	union {
		FR_SSCR_16B_tag SSCR[4]; /* offset: 0x0078  (0x0002 x 4) */

		struct {
			/* Slot Status Counter Register0 */
			FR_SSCR0_16B_tag SSCR0; /* offset: 0x0078 size: 16 bit */
															/* Slot Status Counter Register1 */
			FR_SSCR1_16B_tag SSCR1; /* offset: 0x007A size: 16 bit */
															/* Slot Status Counter Register2 */
			FR_SSCR2_16B_tag SSCR2; /* offset: 0x007C size: 16 bit */
															/* Slot Status Counter Register3 */
			FR_SSCR3_16B_tag SSCR3; /* offset: 0x007E size: 16 bit */
		};
	};
	/* MTS A Configuration Register */
	FR_MTSACFR_16B_tag MTSACFR; /* offset: 0x0080 size: 16 bit */
															/* MTS B Configuration Register */
	FR_MTSBCFR_16B_tag MTSBCFR; /* offset: 0x0082 size: 16 bit */
															/* Receive Shadow Buffer Index Register */
	FR_RSBIR_16B_tag RSBIR;			/* offset: 0x0084 size: 16 bit */
	union {
		/* Receive FIFO Watermark and Selection Register */
		FR_RFWMSR_16B_tag RFWMSR; /* offset: 0x0086 size: 16 bit */

		FR_RFWMSR_16B_tag RFSR; /* deprecated - please avoid */
	};
	union {
		FR_RF_RFSIR_16B_tag RFSIR; /* deprecated - please avoid */

		/* Receive FIFO Start Index Register */
		FR_RF_RFSIR_16B_tag RF_RFSIR; /* offset: 0x0088 size: 16 bit */
	};
	/* Receive FIFO Depth and Size Register */
	FR_RFDSR_16B_tag RFDSR;		/* offset: 0x008A size: 16 bit */
														/* Receive FIFO A Read Index Register */
	FR_RFARIR_16B_tag RFARIR; /* offset: 0x008C size: 16 bit */
														/* Receive FIFO B Read Index Register */
	FR_RFBRIR_16B_tag RFBRIR; /* offset: 0x008E size: 16 bit */
	/* Receive FIFO Message ID Acceptance Filter Value Register */
	FR_RFMIDAFVR_16B_tag RFMIDAFVR; /* offset: 0x0090 size: 16 bit */
	union {
		/* Receive FIFO Message ID Acceptance Filter Mask Register */
		FR_RFMIDAFMR_16B_tag RFMIDAFMR; /* offset: 0x0092 size: 16 bit */

		FR_RFMIDAFMR_16B_tag RFMIAFMR; /* deprecated - please avoid */
	};
	/* Receive FIFO Frame ID Rejection Filter Value Register */
	FR_RFFIDRFVR_16B_tag RFFIDRFVR; /* offset: 0x0094 size: 16 bit */
	/* Receive FIFO Frame ID Rejection Filter Mask Register */
	FR_RFFIDRFMR_16B_tag RFFIDRFMR; /* offset: 0x0096 size: 16 bit */
	/* Receive FIFO Range Filter Configuration Register */
	FR_RFRFCFR_16B_tag RFRFCFR; /* offset: 0x0098 size: 16 bit */
															/* Receive FIFO Range Filter Control Register */
	FR_RFRFCTR_16B_tag RFRFCTR; /* offset: 0x009A size: 16 bit */
	/* Last Dynamic Transmit Slot Channel A Register */
	FR_LDTXSLAR_16B_tag LDTXSLAR; /* offset: 0x009C size: 16 bit */
	/* Last Dynamic Transmit Slot Channel B Register */
	FR_LDTXSLBR_16B_tag LDTXSLBR; /* offset: 0x009E size: 16 bit */
																/* Protocol Configuration Register 0 */
	FR_PCR0_16B_tag PCR0;					/* offset: 0x00A0 size: 16 bit */
																/* Protocol Configuration Register 1 */
	FR_PCR1_16B_tag PCR1;					/* offset: 0x00A2 size: 16 bit */
																/* Protocol Configuration Register 2 */
	FR_PCR2_16B_tag PCR2;					/* offset: 0x00A4 size: 16 bit */
																/* Protocol Configuration Register 3 */
	FR_PCR3_16B_tag PCR3;					/* offset: 0x00A6 size: 16 bit */
																/* Protocol Configuration Register 4 */
	FR_PCR4_16B_tag PCR4;					/* offset: 0x00A8 size: 16 bit */
																/* Protocol Configuration Register 5 */
	FR_PCR5_16B_tag PCR5;					/* offset: 0x00AA size: 16 bit */
																/* Protocol Configuration Register 6 */
	FR_PCR6_16B_tag PCR6;					/* offset: 0x00AC size: 16 bit */
																/* Protocol Configuration Register 7 */
	FR_PCR7_16B_tag PCR7;					/* offset: 0x00AE size: 16 bit */
																/* Protocol Configuration Register 8 */
	FR_PCR8_16B_tag PCR8;					/* offset: 0x00B0 size: 16 bit */
																/* Protocol Configuration Register 9 */
	FR_PCR9_16B_tag PCR9;					/* offset: 0x00B2 size: 16 bit */
																/* Protocol Configuration Register 10 */
	FR_PCR10_16B_tag PCR10;				/* offset: 0x00B4 size: 16 bit */
																/* Protocol Configuration Register 11 */
	FR_PCR11_16B_tag PCR11;				/* offset: 0x00B6 size: 16 bit */
																/* Protocol Configuration Register 12 */
	FR_PCR12_16B_tag PCR12;				/* offset: 0x00B8 size: 16 bit */
																/* Protocol Configuration Register 13 */
	FR_PCR13_16B_tag PCR13;				/* offset: 0x00BA size: 16 bit */
																/* Protocol Configuration Register 14 */
	FR_PCR14_16B_tag PCR14;				/* offset: 0x00BC size: 16 bit */
																/* Protocol Configuration Register 15 */
	FR_PCR15_16B_tag PCR15;				/* offset: 0x00BE size: 16 bit */
																/* Protocol Configuration Register 16 */
	FR_PCR16_16B_tag PCR16;				/* offset: 0x00C0 size: 16 bit */
																/* Protocol Configuration Register 17 */
	FR_PCR17_16B_tag PCR17;				/* offset: 0x00C2 size: 16 bit */
																/* Protocol Configuration Register 18 */
	FR_PCR18_16B_tag PCR18;				/* offset: 0x00C4 size: 16 bit */
																/* Protocol Configuration Register 19 */
	FR_PCR19_16B_tag PCR19;				/* offset: 0x00C6 size: 16 bit */
																/* Protocol Configuration Register 20 */
	FR_PCR20_16B_tag PCR20;				/* offset: 0x00C8 size: 16 bit */
																/* Protocol Configuration Register 21 */
	FR_PCR21_16B_tag PCR21;				/* offset: 0x00CA size: 16 bit */
																/* Protocol Configuration Register 22 */
	FR_PCR22_16B_tag PCR22;				/* offset: 0x00CC size: 16 bit */
																/* Protocol Configuration Register 23 */
	FR_PCR23_16B_tag PCR23;				/* offset: 0x00CE size: 16 bit */
																/* Protocol Configuration Register 24 */
	FR_PCR24_16B_tag PCR24;				/* offset: 0x00D0 size: 16 bit */
																/* Protocol Configuration Register 25 */
	FR_PCR25_16B_tag PCR25;				/* offset: 0x00D2 size: 16 bit */
																/* Protocol Configuration Register 26 */
	FR_PCR26_16B_tag PCR26;				/* offset: 0x00D4 size: 16 bit */
																/* Protocol Configuration Register 27 */
	FR_PCR27_16B_tag PCR27;				/* offset: 0x00D6 size: 16 bit */
																/* Protocol Configuration Register 28 */
	FR_PCR28_16B_tag PCR28;				/* offset: 0x00D8 size: 16 bit */
																/* Protocol Configuration Register 29 */
	FR_PCR29_16B_tag PCR29;				/* offset: 0x00DA size: 16 bit */
																/* Protocol Configuration Register 30 */
	FR_PCR30_16B_tag PCR30;				/* offset: 0x00DC size: 16 bit */
	int8_t FR_reserved_00DE[10];
	/* Receive FIFO System Memory Base Address High Register */
	FR_RFSYMBHADR_16B_tag RFSYMBHADR; /* offset: 0x00E8 size: 16 bit */
	/* Receive FIFO System Memory Base Address Low Register */
	FR_RFSYMBLADR_16B_tag RFSYMBLADR; /* offset: 0x00EA size: 16 bit */
																		/* Receive FIFO Periodic Timer Register */
	FR_RFPTR_16B_tag RFPTR;						/* offset: 0x00EC size: 16 bit */
	/* Receive FIFO Fill Level and Pop Count Register */
	FR_RFFLPCR_16B_tag RFFLPCR; /* offset: 0x00EE size: 16 bit */
															/* ECC Error Interrupt Flag and Enable Register */
	FR_EEIFER_16B_tag EEIFER;		/* offset: 0x00F0 size: 16 bit */
	/* ECC Error Report and Injection Control Register */
	FR_EERICR_16B_tag EERICR; /* offset: 0x00F2 size: 16 bit */
														/* ECC Error Report Adress Register */
	FR_EERAR_16B_tag EERAR;		/* offset: 0x00F4 size: 16 bit */
														/* ECC Error Report Data Register */
	FR_EERDR_16B_tag EERDR;		/* offset: 0x00F6 size: 16 bit */
														/* ECC Error Report Code Register */
	FR_EERCR_16B_tag EERCR;		/* offset: 0x00F8 size: 16 bit */
														/* ECC Error Injection Address Register */
	FR_EEIAR_16B_tag EEIAR;		/* offset: 0x00FA size: 16 bit */
														/* ECC Error Injection Data Register */
	FR_EEIDR_16B_tag EEIDR;		/* offset: 0x00FC size: 16 bit */
														/* ECC Error Injection Code Register */
	FR_EEICR_16B_tag EEICR;		/* offset: 0x00FE size: 16 bit */
	union {
		/*  Register set MB */
		FR_MB_tag MB[64]; /* offset: 0x0100  (0x0008 x 64) */

		struct {
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR0; /* offset: 0x0100 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR0; /* offset: 0x0102 size: 16 bit */
																 /* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR0; /* offset: 0x0104 size: 16 bit */
																 /* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR0; /* offset: 0x0106 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR1; /* offset: 0x0108 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR1; /* offset: 0x010A size: 16 bit */
																 /* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR1; /* offset: 0x010C size: 16 bit */
																 /* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR1; /* offset: 0x010E size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR2; /* offset: 0x0110 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR2; /* offset: 0x0112 size: 16 bit */
																 /* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR2; /* offset: 0x0114 size: 16 bit */
																 /* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR2; /* offset: 0x0116 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR3; /* offset: 0x0118 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR3; /* offset: 0x011A size: 16 bit */
																 /* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR3; /* offset: 0x011C size: 16 bit */
																 /* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR3; /* offset: 0x011E size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR4; /* offset: 0x0120 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR4; /* offset: 0x0122 size: 16 bit */
																 /* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR4; /* offset: 0x0124 size: 16 bit */
																 /* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR4; /* offset: 0x0126 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR5; /* offset: 0x0128 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR5; /* offset: 0x012A size: 16 bit */
																 /* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR5; /* offset: 0x012C size: 16 bit */
																 /* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR5; /* offset: 0x012E size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR6; /* offset: 0x0130 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR6; /* offset: 0x0132 size: 16 bit */
																 /* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR6; /* offset: 0x0134 size: 16 bit */
																 /* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR6; /* offset: 0x0136 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR7; /* offset: 0x0138 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR7; /* offset: 0x013A size: 16 bit */
																 /* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR7; /* offset: 0x013C size: 16 bit */
																 /* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR7; /* offset: 0x013E size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR8; /* offset: 0x0140 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR8; /* offset: 0x0142 size: 16 bit */
																 /* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR8; /* offset: 0x0144 size: 16 bit */
																 /* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR8; /* offset: 0x0146 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR9; /* offset: 0x0148 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR9; /* offset: 0x014A size: 16 bit */
																 /* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR9; /* offset: 0x014C size: 16 bit */
																 /* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR9; /* offset: 0x014E size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR10; /* offset: 0x0150 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR10; /* offset: 0x0152 size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR10; /* offset: 0x0154 size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR10; /* offset: 0x0156 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR11; /* offset: 0x0158 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR11; /* offset: 0x015A size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR11; /* offset: 0x015C size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR11; /* offset: 0x015E size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR12; /* offset: 0x0160 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR12; /* offset: 0x0162 size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR12; /* offset: 0x0164 size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR12; /* offset: 0x0166 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR13; /* offset: 0x0168 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR13; /* offset: 0x016A size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR13; /* offset: 0x016C size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR13; /* offset: 0x016E size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR14; /* offset: 0x0170 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR14; /* offset: 0x0172 size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR14; /* offset: 0x0174 size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR14; /* offset: 0x0176 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR15; /* offset: 0x0178 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR15; /* offset: 0x017A size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR15; /* offset: 0x017C size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR15; /* offset: 0x017E size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR16; /* offset: 0x0180 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR16; /* offset: 0x0182 size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR16; /* offset: 0x0184 size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR16; /* offset: 0x0186 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR17; /* offset: 0x0188 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR17; /* offset: 0x018A size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR17; /* offset: 0x018C size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR17; /* offset: 0x018E size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR18; /* offset: 0x0190 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR18; /* offset: 0x0192 size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR18; /* offset: 0x0194 size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR18; /* offset: 0x0196 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR19; /* offset: 0x0198 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR19; /* offset: 0x019A size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR19; /* offset: 0x019C size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR19; /* offset: 0x019E size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR20; /* offset: 0x01A0 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR20; /* offset: 0x01A2 size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR20; /* offset: 0x01A4 size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR20; /* offset: 0x01A6 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR21; /* offset: 0x01A8 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR21; /* offset: 0x01AA size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR21; /* offset: 0x01AC size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR21; /* offset: 0x01AE size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR22; /* offset: 0x01B0 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR22; /* offset: 0x01B2 size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR22; /* offset: 0x01B4 size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR22; /* offset: 0x01B6 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR23; /* offset: 0x01B8 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR23; /* offset: 0x01BA size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR23; /* offset: 0x01BC size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR23; /* offset: 0x01BE size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR24; /* offset: 0x01C0 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR24; /* offset: 0x01C2 size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR24; /* offset: 0x01C4 size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR24; /* offset: 0x01C6 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR25; /* offset: 0x01C8 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR25; /* offset: 0x01CA size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR25; /* offset: 0x01CC size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR25; /* offset: 0x01CE size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR26; /* offset: 0x01D0 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR26; /* offset: 0x01D2 size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR26; /* offset: 0x01D4 size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR26; /* offset: 0x01D6 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR27; /* offset: 0x01D8 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR27; /* offset: 0x01DA size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR27; /* offset: 0x01DC size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR27; /* offset: 0x01DE size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR28; /* offset: 0x01E0 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR28; /* offset: 0x01E2 size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR28; /* offset: 0x01E4 size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR28; /* offset: 0x01E6 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR29; /* offset: 0x01E8 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR29; /* offset: 0x01EA size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR29; /* offset: 0x01EC size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR29; /* offset: 0x01EE size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR30; /* offset: 0x01F0 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR30; /* offset: 0x01F2 size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR30; /* offset: 0x01F4 size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR30; /* offset: 0x01F6 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR31; /* offset: 0x01F8 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR31; /* offset: 0x01FA size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR31; /* offset: 0x01FC size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR31; /* offset: 0x01FE size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR32; /* offset: 0x0200 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR32; /* offset: 0x0202 size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR32; /* offset: 0x0204 size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR32; /* offset: 0x0206 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR33; /* offset: 0x0208 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR33; /* offset: 0x020A size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR33; /* offset: 0x020C size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR33; /* offset: 0x020E size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR34; /* offset: 0x0210 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR34; /* offset: 0x0212 size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR34; /* offset: 0x0214 size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR34; /* offset: 0x0216 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR35; /* offset: 0x0218 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR35; /* offset: 0x021A size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR35; /* offset: 0x021C size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR35; /* offset: 0x021E size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR36; /* offset: 0x0220 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR36; /* offset: 0x0222 size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR36; /* offset: 0x0224 size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR36; /* offset: 0x0226 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR37; /* offset: 0x0228 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR37; /* offset: 0x022A size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR37; /* offset: 0x022C size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR37; /* offset: 0x022E size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR38; /* offset: 0x0230 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR38; /* offset: 0x0232 size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR38; /* offset: 0x0234 size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR38; /* offset: 0x0236 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR39; /* offset: 0x0238 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR39; /* offset: 0x023A size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR39; /* offset: 0x023C size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR39; /* offset: 0x023E size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR40; /* offset: 0x0240 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR40; /* offset: 0x0242 size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR40; /* offset: 0x0244 size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR40; /* offset: 0x0246 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR41; /* offset: 0x0248 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR41; /* offset: 0x024A size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR41; /* offset: 0x024C size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR41; /* offset: 0x024E size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR42; /* offset: 0x0250 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR42; /* offset: 0x0252 size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR42; /* offset: 0x0254 size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR42; /* offset: 0x0256 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR43; /* offset: 0x0258 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR43; /* offset: 0x025A size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR43; /* offset: 0x025C size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR43; /* offset: 0x025E size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR44; /* offset: 0x0260 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR44; /* offset: 0x0262 size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR44; /* offset: 0x0264 size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR44; /* offset: 0x0266 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR45; /* offset: 0x0268 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR45; /* offset: 0x026A size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR45; /* offset: 0x026C size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR45; /* offset: 0x026E size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR46; /* offset: 0x0270 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR46; /* offset: 0x0272 size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR46; /* offset: 0x0274 size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR46; /* offset: 0x0276 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR47; /* offset: 0x0278 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR47; /* offset: 0x027A size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR47; /* offset: 0x027C size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR47; /* offset: 0x027E size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR48; /* offset: 0x0280 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR48; /* offset: 0x0282 size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR48; /* offset: 0x0284 size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR48; /* offset: 0x0286 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR49; /* offset: 0x0288 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR49; /* offset: 0x028A size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR49; /* offset: 0x028C size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR49; /* offset: 0x028E size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR50; /* offset: 0x0290 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR50; /* offset: 0x0292 size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR50; /* offset: 0x0294 size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR50; /* offset: 0x0296 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR51; /* offset: 0x0298 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR51; /* offset: 0x029A size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR51; /* offset: 0x029C size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR51; /* offset: 0x029E size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR52; /* offset: 0x02A0 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR52; /* offset: 0x02A2 size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR52; /* offset: 0x02A4 size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR52; /* offset: 0x02A6 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR53; /* offset: 0x02A8 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR53; /* offset: 0x02AA size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR53; /* offset: 0x02AC size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR53; /* offset: 0x02AE size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR54; /* offset: 0x02B0 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR54; /* offset: 0x02B2 size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR54; /* offset: 0x02B4 size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR54; /* offset: 0x02B6 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR55; /* offset: 0x02B8 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR55; /* offset: 0x02BA size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR55; /* offset: 0x02BC size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR55; /* offset: 0x02BE size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR56; /* offset: 0x02C0 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR56; /* offset: 0x02C2 size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR56; /* offset: 0x02C4 size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR56; /* offset: 0x02C6 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR57; /* offset: 0x02C8 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR57; /* offset: 0x02CA size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR57; /* offset: 0x02CC size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR57; /* offset: 0x02CE size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR58; /* offset: 0x02D0 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR58; /* offset: 0x02D2 size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR58; /* offset: 0x02D4 size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR58; /* offset: 0x02D6 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR59; /* offset: 0x02D8 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR59; /* offset: 0x02DA size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR59; /* offset: 0x02DC size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR59; /* offset: 0x02DE size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR60; /* offset: 0x02E0 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR60; /* offset: 0x02E2 size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR60; /* offset: 0x02E4 size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR60; /* offset: 0x02E6 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR61; /* offset: 0x02E8 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR61; /* offset: 0x02EA size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR61; /* offset: 0x02EC size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR61; /* offset: 0x02EE size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR62; /* offset: 0x02F0 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR62; /* offset: 0x02F2 size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR62; /* offset: 0x02F4 size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR62; /* offset: 0x02F6 size: 16 bit */
			/* Message Buffer Configuration Control Status Register */
			FR_MBCCSR_16B_tag MBCCSR63; /* offset: 0x02F8 size: 16 bit */
			/* Message Buffer Cycle Counter Filter Register */
			FR_MBCCFR_16B_tag MBCCFR63; /* offset: 0x02FA size: 16 bit */
																	/* Message Buffer Frame ID Register */
			FR_MBFIDR_16B_tag MBFIDR63; /* offset: 0x02FC size: 16 bit */
																	/* Message Buffer Index Register */
			FR_MBIDXR_16B_tag MBIDXR63; /* offset: 0x02FE size: 16 bit */
		};
	};
} FR_tag;

#define FR (*(volatile FR_tag*)0xFFFE0000UL)

#ifdef __MWERKS__
#pragma pop
#endif

#ifdef __cplusplus
}
#endif
#endif /* _leopard_H_*/
/* End of file */
