/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

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
 * @file    STM32F4xx/stm32_dma.h
 * @brief   Enhanced-DMA helper driver header.
 * @note    This file requires definitions from the ST STM32F4xx header file
 *          stm32f4xx.h.
 *
 * @addtogroup STM32F4xx_DMA
 * @{
 */

#ifndef _STM32_DMA_H_
#define _STM32_DMA_H_

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Total number of DMA streams.
 * @note    This is the total number of streams among all the DMA units.
 */
#define STM32_DMA_STREAMS           16

/**
 * @brief   Mask of the ISR bits passed to the DMA callback functions.
 */
#define STM32_DMA_ISR_MASK          0x3D

/**
 * @brief   Returns the channel associated to the specified stream.
 *
 * @param[in] id        the unique numeric stream identifier
 * @param[in] c         a stream/channel association word, one channel per
 *                      nibble
 * @return              Returns the channel associated to the stream.
 */
#define STM32_DMA_GETCHANNEL(id, c) (((c) >> (((id) & 7) * 4)) & 7)

/**
 * @brief   Checks if a DMA priority is within the valid range.
 * @param[in] prio      DMA priority
 *
 * @retval              The check result.
 * @retval FALSE        invalid DMA priority.
 * @retval TRUE         correct DMA priority.
 */
#define STM32_DMA_IS_VALID_PRIORITY(prio) (((prio) >= 0) && ((prio) <= 3))

/**
 * @brief   Returns an unique numeric identifier for a DMA stream.
 *
 * @param[in] dma       the DMA unit number
 * @param[in] stream    the stream number
 * @return              An unique numeric stream identifier.
 */
#define STM32_DMA_STREAM_ID(dma, stream) ((((dma) - 1) * 8) + (stream))

/**
 * @brief   Returns a DMA stream identifier mask.
 *
 *
 * @param[in] dma       the DMA unit number
 * @param[in] stream    the stream number
 * @return              A DMA stream identifier mask.
 */
#define STM32_DMA_STREAM_ID_MSK(dma, stream)                                \
  (1 << STM32_DMA_STREAM_ID(dma, stream))

/**
 * @brief   Checks if a DMA stream unique identifier belongs to a mask.
 * @param[in] id        the stream numeric identifier
 * @param[in] mask      the stream numeric identifiers mask
 *
 * @retval              The check result.
 * @retval FALSE        id does not belong to the mask.
 * @retval TRUE         id belongs to the mask.
 */
#define STM32_DMA_IS_VALID_ID(id, mask) (((1 << (id)) & (mask)))

/**
 * @name    DMA streams identifiers
 * @{
 */
/**
 * @brief   Returns a pointer to a stm32_dma_stream_t structure.
 *
 * @param[in] id        the stream numeric identifier
 * @return              A pointer to the stm32_dma_stream_t constant structure
 *                      associated to the DMA stream.
 */
#define STM32_DMA_STREAM(id)        (&_stm32_dma_streams[id])

#define STM32_DMA1_STREAM0          STM32_DMA_STREAM(0)
#define STM32_DMA1_STREAM1          STM32_DMA_STREAM(1)
#define STM32_DMA1_STREAM2          STM32_DMA_STREAM(2)
#define STM32_DMA1_STREAM3          STM32_DMA_STREAM(3)
#define STM32_DMA1_STREAM4          STM32_DMA_STREAM(4)
#define STM32_DMA1_STREAM5          STM32_DMA_STREAM(5)
#define STM32_DMA1_STREAM6          STM32_DMA_STREAM(6)
#define STM32_DMA1_STREAM7          STM32_DMA_STREAM(7)
#define STM32_DMA2_STREAM0          STM32_DMA_STREAM(8)
#define STM32_DMA2_STREAM1          STM32_DMA_STREAM(9)
#define STM32_DMA2_STREAM2          STM32_DMA_STREAM(10)
#define STM32_DMA2_STREAM3          STM32_DMA_STREAM(11)
#define STM32_DMA2_STREAM4          STM32_DMA_STREAM(12)
#define STM32_DMA2_STREAM5          STM32_DMA_STREAM(13)
#define STM32_DMA2_STREAM6          STM32_DMA_STREAM(14)
#define STM32_DMA2_STREAM7          STM32_DMA_STREAM(15)
/** @} */

/**
 * @name    CR register constants common to all DMA types
 * @{
 */
#define STM32_DMA_CR_EN             DMA_SxCR_EN
#define STM32_DMA_CR_TEIE           DMA_SxCR_TEIE
#define STM32_DMA_CR_HTIE           DMA_SxCR_HTIE
#define STM32_DMA_CR_TCIE           DMA_SxCR_TCIE
#define STM32_DMA_CR_DIR_MASK       DMA_SxCR_DIR
#define STM32_DMA_CR_DIR_P2M        0
#define STM32_DMA_CR_DIR_M2P        DMA_SxCR_DIR_0
#define STM32_DMA_CR_DIR_M2M        DMA_SxCR_DIR_1
#define STM32_DMA_CR_CIRC           DMA_SxCR_CIRC
#define STM32_DMA_CR_PINC           DMA_SxCR_PINC
#define STM32_DMA_CR_MINC           DMA_SxCR_MINC
#define STM32_DMA_CR_PSIZE_MASK     DMA_SxCR_PSIZE
#define STM32_DMA_CR_PSIZE_BYTE     0
#define STM32_DMA_CR_PSIZE_HWORD    DMA_SxCR_PSIZE_0
#define STM32_DMA_CR_PSIZE_WORD     DMA_SxCR_PSIZE_1
#define STM32_DMA_CR_MSIZE_MASK     DMA_SxCR_MSIZE
#define STM32_DMA_CR_MSIZE_BYTE     0
#define STM32_DMA_CR_MSIZE_HWORD    DMA_SxCR_MSIZE_0
#define STM32_DMA_CR_MSIZE_WORD     DMA_SxCR_MSIZE_1
#define STM32_DMA_CR_SIZE_MASK      (STM32_DMA_CR_PSIZE_MASK |              \
                                     STM32_DMA_CR_MSIZE_MASK)
#define STM32_DMA_CR_PL_MASK        DMA_SxCR_PL
#define STM32_DMA_CR_PL(n)          ((n) << 16)
/** @} */

/**
 * @name    CR register constants only found in STM32F2xx/STM32F4xx
 * @{
 */
#define STM32_DMA_CR_DMEIE          DMA_SxCR_DMEIE
#define STM32_DMA_CR_PFCTRL         DMA_SxCR_PFCTRL
#define STM32_DMA_CR_PINCOS         DMA_SxCR_PINCOS
#define STM32_DMA_CR_DBM            DMA_SxCR_DBM
#define STM32_DMA_CR_CT             DMA_SxCR_CT
#define STM32_DMA_CR_PBURST_MASK    DMA_SxCR_PBURST
#define STM32_DMA_CR_PBURST_SINGLE  0
#define STM32_DMA_CR_PBURST_INCR4   DMA_SxCR_PBURST_0
#define STM32_DMA_CR_PBURST_INCR8   DMA_SxCR_PBURST_1
#define STM32_DMA_CR_PBURST_INCR16  (DMA_SxCR_PBURST_0 | DMA_SxCR_PBURST_1)
#define STM32_DMA_CR_MBURST_MASK    DMA_SxCR_MBURST
#define STM32_DMA_CR_MBURST_SINGLE  0
#define STM32_DMA_CR_MBURST_INCR4   DMA_SxCR_MBURST_0
#define STM32_DMA_CR_MBURST_INCR8   DMA_SxCR_MBURST_1
#define STM32_DMA_CR_MBURST_INCR16  (DMA_SxCR_MBURST_0 | DMA_SxCR_MBURST_1)
#define STM32_DMA_CR_CHSEL_MASK     DMA_SxCR_CHSEL
#define STM32_DMA_CR_CHSEL(n)       ((n) << 25)
/** @} */

/**
 * @name    FCR register constants only found in STM32F2xx/STM32F4xx
 * @{
 */
#define STM32_DMA_FCR_FEIE          DMA_SxFCR_FEIE
#define STM32_DMA_FCR_FS_MASK       DMA_SxFCR_FS
#define STM32_DMA_FCR_DMDIS         DMA_SxFCR_DMDIS
#define STM32_DMA_FCR_FTH_MASK      DMA_SxFCR_FTH
#define STM32_DMA_FCR_FTH_1Q        0
#define STM32_DMA_FCR_FTH_HALF      DMA_SxFCR_FTH_0
#define STM32_DMA_FCR_FTH_3Q        DMA_SxFCR_FTH_1
#define STM32_DMA_FCR_FTH_FULL      (DMA_SxFCR_FTH_0 | DMA_SxFCR_FTH_1)
/** @} */

/**
 * @name    Status flags passed to the ISR callbacks
 */
#define STM32_DMA_ISR_FEIF          DMA_LISR_FEIF0
#define STM32_DMA_ISR_DMEIF         DMA_LISR_DMEIF0
#define STM32_DMA_ISR_TEIF          DMA_LISR_TEIF0
#define STM32_DMA_ISR_HTIF          DMA_LISR_HTIF0
#define STM32_DMA_ISR_TCIF          DMA_LISR_TCIF0
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   STM32 DMA stream descriptor structure.
 */
typedef struct {
  DMA_Stream_TypeDef    *stream;        /**< @brief Associated DMA stream.  */
  volatile uint32_t     *ifcr;          /**< @brief Associated IFCR reg.    */
  uint8_t               ishift;         /**< @brief Bits offset in xIFCR
                                             register.                      */
  uint8_t               selfindex;      /**< @brief Index to self in array. */
  uint8_t               vector;         /**< @brief Associated IRQ vector.  */
} stm32_dma_stream_t;

/**
 * @brief   STM32 DMA ISR function type.
 *
 * @param[in] p         parameter for the registered function
 * @param[in] flags     pre-shifted content of the xISR register, the bits
 *                      are aligned to bit zero
 */
typedef void (*stm32_dmaisr_t)(void *p, uint32_t flags);

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @name    Macro Functions
 * @{
 */
/**
 * @brief   Associates a peripheral data register to a DMA stream.
 * @note    This function can be invoked in both ISR or thread context.
 * @pre     The stream must have been allocated using @p dmaStreamAllocate().
 * @post    After use the stream can be released using @p dmaStreamRelease().
 *
 * @param[in] dmastp    pointer to a stm32_dma_stream_t structure
 * @param[in] addr      value to be written in the PAR register
 *
 * @special
 */
#define dmaStreamSetPeripheral(dmastp, addr) {                              \
  (dmastp)->stream->PAR  = (uint32_t)(addr);                                \
}

/**
 * @brief   Associates a memory destination to a DMA stream.
 * @note    This function can be invoked in both ISR or thread context.
 * @pre     The stream must have been allocated using @p dmaStreamAllocate().
 * @post    After use the stream can be released using @p dmaStreamRelease().
 *
 * @param[in] dmastp    pointer to a stm32_dma_stream_t structure
 * @param[in] addr      value to be written in the M0AR register
 *
 * @special
 */
#define dmaStreamSetMemory0(dmastp, addr) {                                 \
  (dmastp)->stream->M0AR  = (uint32_t)(addr);                               \
}

/**
 * @brief   Associates an alternate memory destination to a DMA stream.
 * @note    This function can be invoked in both ISR or thread context.
 *
 * @param[in] dmastp    pointer to a stm32_dma_stream_t structure
 * @param[in] addr      value to be written in the M1AR register
 *
 * @special
 */
#define dmaStreamSetMemory1(dmastp, addr) {                                 \
  (dmastp)->stream->M1AR  = (uint32_t)(addr);                               \
}

/**
 * @brief   Sets the number of transfers to be performed.
 * @note    This function can be invoked in both ISR or thread context.
 * @pre     The stream must have been allocated using @p dmaStreamAllocate().
 * @post    After use the stream can be released using @p dmaStreamRelease().
 *
 * @param[in] dmastp    pointer to a stm32_dma_stream_t structure
 * @param[in] size      value to be written in the CNDTR register
 *
 * @special
 */
#define dmaStreamSetTransactionSize(dmastp, size) {                         \
  (dmastp)->stream->NDTR  = (uint32_t)(size);                               \
}

/**
 * @brief   Returns the number of transfers to be performed.
 * @note    This function can be invoked in both ISR or thread context.
 * @pre     The stream must have been allocated using @p dmaStreamAllocate().
 * @post    After use the stream can be released using @p dmaStreamRelease().
 *
 * @param[in] dmastp    pointer to a stm32_dma_stream_t structure
 * @return              The number of transfers to be performed.
 *
 * @special
 */
#define dmaStreamGetTransactionSize(dmastp) ((size_t)((dmastp)->stream->NDTR))

/**
 * @brief   Programs the stream mode settings.
 * @note    This function can be invoked in both ISR or thread context.
 * @pre     The stream must have been allocated using @p dmaStreamAllocate().
 * @post    After use the stream can be released using @p dmaStreamRelease().
 *
 * @param[in] dmastp    pointer to a stm32_dma_stream_t structure
 * @param[in] mode      value to be written in the CR register
 *
 * @special
 */
#define dmaStreamSetMode(dmastp, mode) {                                    \
  (dmastp)->stream->CR  = (uint32_t)(mode);                                 \
}

/**
 * @brief   Programs the stream FIFO settings.
 * @note    This function can be invoked in both ISR or thread context.
 * @pre     The stream must have been allocated using @p dmaStreamAllocate().
 * @post    After use the stream can be released using @p dmaStreamRelease().
 *
 * @param[in] dmastp    pointer to a stm32_dma_stream_t structure
 * @param[in] mode      value to be written in the FCR register
 *
 * @special
 */
#define dmaStreamSetFIFO(dmastp, mode) {                                    \
  (dmastp)->stream->FCR = (uint32_t)(mode);                                 \
}

/**
 * @brief   DMA stream enable.
 * @note    This function can be invoked in both ISR or thread context.
 * @pre     The stream must have been allocated using @p dmaStreamAllocate().
 * @post    After use the stream can be released using @p dmaStreamRelease().
 *
 * @param[in] dmastp    pointer to a stm32_dma_stream_t structure
 *
 * @special
 */
#define dmaStreamEnable(dmastp) {                                           \
  (dmastp)->stream->CR |= STM32_DMA_CR_EN;                                  \
}

/**
 * @brief   DMA stream disable.
 * @details The function disables the specified stream, waits for the disable
 *          operation to complete and then clears any pending interrupt.
 * @note    This function can be invoked in both ISR or thread context.
 * @note    Interrupts enabling flags are set to zero after this call, see
 *          bug 3607518.
 * @pre     The stream must have been allocated using @p dmaStreamAllocate().
 * @post    After use the stream can be released using @p dmaStreamRelease().
 *
 * @param[in] dmastp    pointer to a stm32_dma_stream_t structure
 *
 * @special
 */
#define dmaStreamDisable(dmastp) {                                          \
  (dmastp)->stream->CR &= ~(STM32_DMA_CR_TCIE | STM32_DMA_CR_HTIE  |        \
                            STM32_DMA_CR_TEIE | STM32_DMA_CR_DMEIE |        \
                            STM32_DMA_CR_EN);                               \
  while (((dmastp)->stream->CR & STM32_DMA_CR_EN) != 0)                     \
    ;                                                                       \
  dmaStreamClearInterrupt(dmastp);                                          \
}

/**
 * @brief   DMA stream interrupt sources clear.
 * @note    This function can be invoked in both ISR or thread context.
 * @pre     The stream must have been allocated using @p dmaStreamAllocate().
 * @post    After use the stream can be released using @p dmaStreamRelease().
 *
 * @param[in] dmastp    pointer to a stm32_dma_stream_t structure
 *
 * @special
 */
#define dmaStreamClearInterrupt(dmastp) {                                   \
  *(dmastp)->ifcr = STM32_DMA_ISR_MASK << (dmastp)->ishift;                 \
}

/**
 * @brief   Starts a memory to memory operation using the specified stream.
 * @note    The default transfer data mode is "byte to byte" but it can be
 *          changed by specifying extra options in the @p mode parameter.
 * @pre     The stream must have been allocated using @p dmaStreamAllocate().
 * @post    After use the stream can be released using @p dmaStreamRelease().
 *
 * @param[in] dmastp    pointer to a stm32_dma_stream_t structure
 * @param[in] mode      value to be written in the CCR register, this value
 *                      is implicitly ORed with:
 *                      - @p STM32_DMA_CR_MINC
 *                      - @p STM32_DMA_CR_PINC
 *                      - @p STM32_DMA_CR_DIR_M2M
 *                      - @p STM32_DMA_CR_EN
 *                      .
 * @param[in] src       source address
 * @param[in] dst       destination address
 * @param[in] n         number of data units to copy
 */
#define dmaStartMemCopy(dmastp, mode, src, dst, n) {                        \
  dmaStreamSetPeripheral(dmastp, src);                                      \
  dmaStreamSetMemory0(dmastp, dst);                                         \
  dmaStreamSetTransactionSize(dmastp, n);                                   \
  dmaStreamSetMode(dmastp, (mode) |                                         \
                           STM32_DMA_CR_MINC | STM32_DMA_CR_PINC |          \
                           STM32_DMA_CR_DIR_M2M | STM32_DMA_CR_EN);         \
}

/**
 * @brief   Polled wait for DMA transfer end.
 * @pre     The stream must have been allocated using @p dmaStreamAllocate().
 * @post    After use the stream can be released using @p dmaStreamRelease().
 *
 * @param[in] dmastp    pointer to a stm32_dma_stream_t structure
 */
#define dmaWaitCompletion(dmastp) {                                         \
  while ((dmastp)->stream->NDTR > 0)                                        \
    ;                                                                       \
  dmaStreamDisable(dmastp);                                                 \
}
/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if !defined(__DOXYGEN__)
extern const stm32_dma_stream_t _stm32_dma_streams[STM32_DMA_STREAMS];
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void dmaInit(void);
  bool_t dmaStreamAllocate(const stm32_dma_stream_t *dmastp,
                           uint32_t priority,
                           stm32_dmaisr_t func,
                           void *param);
  void dmaStreamRelease(const stm32_dma_stream_t *dmastp);
#ifdef __cplusplus
}
#endif

#endif /* _STM32_DMA_H_ */

/** @} */
