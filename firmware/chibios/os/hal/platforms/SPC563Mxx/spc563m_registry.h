/*
    SPC5 HAL - Copyright (C) 2013 STMicroelectronics

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    SPC563Mxx/spc563m_registry.h
 * @brief   SPC563Mxx capabilities registry.
 *
 * @addtogroup HAL
 * @{
 */

#ifndef _SPC563M_REGISTRY_H_
#define _SPC563M_REGISTRY_H_

/*===========================================================================*/
/* Platform capabilities.                                                    */
/*===========================================================================*/

/**
 * @name    SPC563Mxx capabilities
 * @{
 */
/* DSPI attribures.*/
#define SPC5_HAS_DSPI0                      FALSE
#define SPC5_HAS_DSPI1                      TRUE
#define SPC5_HAS_DSPI2                      TRUE
#define SPC5_HAS_DSPI3                      FALSE
#define SPC5_DSPI_FIFO_DEPTH                16
#define SPC5_DSPI1_TX_DMA_DEV_ID            12
#define SPC5_DSPI1_RX_DMA_DEV_ID            13
#define SPC5_DSPI2_TX_DMA_DEV_ID            14
#define SPC5_DSPI2_RX_DMA_DEV_ID            15
#define SPC5_DSPI1_EOQF_HANDLER             vector132
#define SPC5_DSPI1_EOQF_NUMBER              132
#define SPC5_DSPI2_EOQF_HANDLER             vector137
#define SPC5_DSPI2_EOQF_NUMBER              137
#define SPC5_DSPI1_ENABLE_CLOCK()
#define SPC5_DSPI1_DISABLE_CLOCK()
#define SPC5_DSPI2_ENABLE_CLOCK()
#define SPC5_DSPI2_DISABLE_CLOCK()

/* eDMA attributes.*/
#define SPC5_HAS_EDMA                       TRUE
#define SPC5_EDMA_NCHANNELS                 32
#define SPC5_EDMA_HAS_MUX                   FALSE

/* eQADC attributes.*/
#define SPC5_HAS_EQADC                      TRUE

/* eSCI attributes.*/
#define SPC5_HAS_ESCIA                      TRUE
#define SPC5_ESCIA_HANDLER                  vector146
#define SPC5_ESCIA_NUMBER                   146

#define SPC5_HAS_ESCIB                      TRUE
#define SPC5_ESCIB_HANDLER                  vector149
#define SPC5_ESCIB_NUMBER                   149

#define SPC5_HAS_ESCIC                      FALSE

/* SIU attributes.*/
#define SPC5_HAS_SIU                        TRUE
#define SPC5_SIU_SUPPORTS_PORTS             FALSE
/** @} */

#endif /* _SPC563M_REGISTRY_H_ */

/** @} */
