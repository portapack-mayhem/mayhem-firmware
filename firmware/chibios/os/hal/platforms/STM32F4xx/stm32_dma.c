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
 * @file    STM32F4xx/stm32_dma.c
 * @brief   Enhanced DMA helper driver code.
 *
 * @addtogroup STM32F4xx_DMA
 * @details DMA sharing helper driver. In the STM32 the DMA streams are a
 *          shared resource, this driver allows to allocate and free DMA
 *          streams at runtime in order to allow all the other device
 *          drivers to coordinate the access to the resource.
 * @note    The DMA ISR handlers are all declared into this module because
 *          sharing, the various device drivers can associate a callback to
 *          ISRs when allocating streams.
 * @{
 */

#include "ch.h"
#include "hal.h"

/* The following macro is only defined if some driver requiring DMA services
   has been enabled.*/
#if defined(STM32_DMA_REQUIRED) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/**
 * @brief   Mask of the DMA1 streams in @p dma_streams_mask.
 */
#define STM32_DMA1_STREAMS_MASK     0x000000FF

/**
 * @brief   Mask of the DMA2 streams in @p dma_streams_mask.
 */
#define STM32_DMA2_STREAMS_MASK     0x0000FF00

/**
 * @brief   Post-reset value of the stream CR register.
 */
#define STM32_DMA_CR_RESET_VALUE    0x00000000

/**
 * @brief   Post-reset value of the stream FCR register.
 */
#define STM32_DMA_FCR_RESET_VALUE   0x00000021

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   DMA streams descriptors.
 * @details This table keeps the association between an unique stream
 *          identifier and the involved physical registers.
 * @note    Don't use this array directly, use the appropriate wrapper macros
 *          instead: @p STM32_DMA1_STREAM0, @p STM32_DMA1_STREAM1 etc.
 */
const stm32_dma_stream_t _stm32_dma_streams[STM32_DMA_STREAMS] = {
  {DMA1_Stream0, &DMA1->LIFCR, 0, 0, DMA1_Stream0_IRQn},
  {DMA1_Stream1, &DMA1->LIFCR, 6, 1, DMA1_Stream1_IRQn},
  {DMA1_Stream2, &DMA1->LIFCR, 16, 2, DMA1_Stream2_IRQn},
  {DMA1_Stream3, &DMA1->LIFCR, 22, 3, DMA1_Stream3_IRQn},
  {DMA1_Stream4, &DMA1->HIFCR, 0, 4, DMA1_Stream4_IRQn},
  {DMA1_Stream5, &DMA1->HIFCR, 6, 5, DMA1_Stream5_IRQn},
  {DMA1_Stream6, &DMA1->HIFCR, 16, 6, DMA1_Stream6_IRQn},
  {DMA1_Stream7, &DMA1->HIFCR, 22, 7, DMA1_Stream7_IRQn},
  {DMA2_Stream0, &DMA2->LIFCR, 0, 8, DMA2_Stream0_IRQn},
  {DMA2_Stream1, &DMA2->LIFCR, 6, 9, DMA2_Stream1_IRQn},
  {DMA2_Stream2, &DMA2->LIFCR, 16, 10, DMA2_Stream2_IRQn},
  {DMA2_Stream3, &DMA2->LIFCR, 22, 11, DMA2_Stream3_IRQn},
  {DMA2_Stream4, &DMA2->HIFCR, 0, 12, DMA2_Stream4_IRQn},
  {DMA2_Stream5, &DMA2->HIFCR, 6, 13, DMA2_Stream5_IRQn},
  {DMA2_Stream6, &DMA2->HIFCR, 16, 14, DMA2_Stream6_IRQn},
  {DMA2_Stream7, &DMA2->HIFCR, 22, 15, DMA2_Stream7_IRQn},
};

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   DMA ISR redirector type.
 */
typedef struct {
  stm32_dmaisr_t        dma_func;       /**< @brief DMA callback function.  */
  void                  *dma_param;     /**< @brief DMA callback parameter. */
} dma_isr_redir_t;

/**
 * @brief   Mask of the allocated streams.
 */
static uint32_t dma_streams_mask;

/**
 * @brief   DMA IRQ redirectors.
 */
static dma_isr_redir_t dma_isr_redir[STM32_DMA_STREAMS];

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/**
 * @brief   DMA1 stream 0 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(DMA1_Stream0_IRQHandler) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA1->LISR >> 0) & STM32_DMA_ISR_MASK;
  DMA1->LIFCR = flags << 0;
  if (dma_isr_redir[0].dma_func)
    dma_isr_redir[0].dma_func(dma_isr_redir[0].dma_param, flags);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   DMA1 stream 1 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(DMA1_Stream1_IRQHandler) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA1->LISR >> 6) & STM32_DMA_ISR_MASK;
  DMA1->LIFCR = flags << 6;
  if (dma_isr_redir[1].dma_func)
    dma_isr_redir[1].dma_func(dma_isr_redir[1].dma_param, flags);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   DMA1 stream 2 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(DMA1_Stream2_IRQHandler) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA1->LISR >> 16) & STM32_DMA_ISR_MASK;
  DMA1->LIFCR = flags << 16;
  if (dma_isr_redir[2].dma_func)
    dma_isr_redir[2].dma_func(dma_isr_redir[2].dma_param, flags);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   DMA1 stream 3 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(DMA1_Stream3_IRQHandler) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA1->LISR >> 22) & STM32_DMA_ISR_MASK;
  DMA1->LIFCR = flags << 22;
  if (dma_isr_redir[3].dma_func)
    dma_isr_redir[3].dma_func(dma_isr_redir[3].dma_param, flags);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   DMA1 stream 4 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(DMA1_Stream4_IRQHandler) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA1->HISR >> 0) & STM32_DMA_ISR_MASK;
  DMA1->HIFCR = flags << 0;
  if (dma_isr_redir[4].dma_func)
    dma_isr_redir[4].dma_func(dma_isr_redir[4].dma_param, flags);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   DMA1 stream 5 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(DMA1_Stream5_IRQHandler) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA1->HISR >> 6) & STM32_DMA_ISR_MASK;
  DMA1->HIFCR = flags << 6;
  if (dma_isr_redir[5].dma_func)
    dma_isr_redir[5].dma_func(dma_isr_redir[5].dma_param, flags);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   DMA1 stream 6 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(DMA1_Stream6_IRQHandler) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA1->HISR >> 16) & STM32_DMA_ISR_MASK;
  DMA1->HIFCR = flags << 16;
  if (dma_isr_redir[6].dma_func)
    dma_isr_redir[6].dma_func(dma_isr_redir[6].dma_param, flags);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   DMA1 stream 7 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(DMA1_Stream7_IRQHandler) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA1->HISR >> 22) & STM32_DMA_ISR_MASK;
  DMA1->HIFCR = flags << 22;
  if (dma_isr_redir[7].dma_func)
    dma_isr_redir[7].dma_func(dma_isr_redir[7].dma_param, flags);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   DMA2 stream 0 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(DMA2_Stream0_IRQHandler) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA2->LISR >> 0) & STM32_DMA_ISR_MASK;
  DMA2->LIFCR = flags << 0;
  if (dma_isr_redir[8].dma_func)
    dma_isr_redir[8].dma_func(dma_isr_redir[8].dma_param, flags);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   DMA2 stream 1 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(DMA2_Stream1_IRQHandler) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA2->LISR >> 6) & STM32_DMA_ISR_MASK;
  DMA2->LIFCR = flags << 6;
  if (dma_isr_redir[9].dma_func)
    dma_isr_redir[9].dma_func(dma_isr_redir[9].dma_param, flags);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   DMA2 stream 2 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(DMA2_Stream2_IRQHandler) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA2->LISR >> 16) & STM32_DMA_ISR_MASK;
  DMA2->LIFCR = flags << 16;
  if (dma_isr_redir[10].dma_func)
    dma_isr_redir[10].dma_func(dma_isr_redir[10].dma_param, flags);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   DMA2 stream 3 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(DMA2_Stream3_IRQHandler) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA2->LISR >> 22) & STM32_DMA_ISR_MASK;
  DMA2->LIFCR = flags << 22;
  if (dma_isr_redir[11].dma_func)
    dma_isr_redir[11].dma_func(dma_isr_redir[11].dma_param, flags);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   DMA2 stream 4 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(DMA2_Stream4_IRQHandler) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA2->HISR >> 0) & STM32_DMA_ISR_MASK;
  DMA2->HIFCR = flags << 0;
  if (dma_isr_redir[12].dma_func)
    dma_isr_redir[12].dma_func(dma_isr_redir[12].dma_param, flags);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   DMA2 stream 5 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(DMA2_Stream5_IRQHandler) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA2->HISR >> 6) & STM32_DMA_ISR_MASK;
  DMA2->HIFCR = flags << 6;
  if (dma_isr_redir[13].dma_func)
    dma_isr_redir[13].dma_func(dma_isr_redir[13].dma_param, flags);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   DMA2 stream 6 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(DMA2_Stream6_IRQHandler) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA2->HISR >> 16) & STM32_DMA_ISR_MASK;
  DMA2->HIFCR = flags << 16;
  if (dma_isr_redir[14].dma_func)
    dma_isr_redir[14].dma_func(dma_isr_redir[14].dma_param, flags);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   DMA2 stream 7 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(DMA2_Stream7_IRQHandler) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA2->HISR >> 22) & STM32_DMA_ISR_MASK;
  DMA2->HIFCR = flags << 22;
  if (dma_isr_redir[15].dma_func)
    dma_isr_redir[15].dma_func(dma_isr_redir[15].dma_param, flags);

  CH_IRQ_EPILOGUE();
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   STM32 DMA helper initialization.
 *
 * @init
 */
void dmaInit(void) {
  int i;

  dma_streams_mask = 0;
  for (i = 0; i < STM32_DMA_STREAMS; i++) {
    _stm32_dma_streams[i].stream->CR = 0;
    dma_isr_redir[i].dma_func = NULL;
  }
  DMA1->LIFCR = 0xFFFFFFFF;
  DMA1->HIFCR = 0xFFFFFFFF;
  DMA2->LIFCR = 0xFFFFFFFF;
  DMA2->HIFCR = 0xFFFFFFFF;
}

/**
 * @brief   Allocates a DMA stream.
 * @details The stream is allocated and, if required, the DMA clock enabled.
 *          The function also enables the IRQ vector associated to the stream
 *          and initializes its priority.
 * @pre     The stream must not be already in use or an error is returned.
 * @post    The stream is allocated and the default ISR handler redirected
 *          to the specified function.
 * @post    The stream ISR vector is enabled and its priority configured.
 * @post    The stream must be freed using @p dmaStreamRelease() before it can
 *          be reused with another peripheral.
 * @post    The stream is in its post-reset state.
 * @note    This function can be invoked in both ISR or thread context.
 *
 * @param[in] dmastp    pointer to a stm32_dma_stream_t structure
 * @param[in] priority  IRQ priority mask for the DMA stream
 * @param[in] func      handling function pointer, can be @p NULL
 * @param[in] param     a parameter to be passed to the handling function
 * @return              The operation status.
 * @retval FALSE        no error, stream taken.
 * @retval TRUE         error, stream already taken.
 *
 * @special
 */
bool_t dmaStreamAllocate(const stm32_dma_stream_t *dmastp,
                         uint32_t priority,
                         stm32_dmaisr_t func,
                         void *param) {

  chDbgCheck(dmastp != NULL, "dmaStreamAllocate");

  /* Checks if the stream is already taken.*/
  if ((dma_streams_mask & (1 << dmastp->selfindex)) != 0)
    return TRUE;

  /* Marks the stream as allocated.*/
  dma_isr_redir[dmastp->selfindex].dma_func  = func;
  dma_isr_redir[dmastp->selfindex].dma_param = param;
  dma_streams_mask |= (1 << dmastp->selfindex);

  /* Enabling DMA clocks required by the current streams set.*/
  if ((dma_streams_mask & STM32_DMA1_STREAMS_MASK) != 0)
    rccEnableDMA1(FALSE);
  if ((dma_streams_mask & STM32_DMA2_STREAMS_MASK) != 0)
    rccEnableDMA2(FALSE);

  /* Putting the stream in a safe state.*/
  dmaStreamDisable(dmastp);
  dmastp->stream->CR = STM32_DMA_CR_RESET_VALUE;
  dmastp->stream->FCR = STM32_DMA_FCR_RESET_VALUE;

  /* Enables the associated IRQ vector if a callback is defined.*/
  if (func != NULL)
    nvicEnableVector(dmastp->vector, CORTEX_PRIORITY_MASK(priority));

  return FALSE;
}

/**
 * @brief   Releases a DMA stream.
 * @details The stream is freed and, if required, the DMA clock disabled.
 *          Trying to release a unallocated stream is an illegal operation
 *          and is trapped if assertions are enabled.
 * @pre     The stream must have been allocated using @p dmaStreamAllocate().
 * @post    The stream is again available.
 * @note    This function can be invoked in both ISR or thread context.
 *
 * @param[in] dmastp    pointer to a stm32_dma_stream_t structure
 *
 * @special
 */
void dmaStreamRelease(const stm32_dma_stream_t *dmastp) {

  chDbgCheck(dmastp != NULL, "dmaStreamRelease");

  /* Check if the streams is not taken.*/
  chDbgAssert((dma_streams_mask & (1 << dmastp->selfindex)) != 0,
              "dmaStreamRelease(), #1", "not allocated");

  /* Disables the associated IRQ vector.*/
  nvicDisableVector(dmastp->vector);

  /* Marks the stream as not allocated.*/
  dma_streams_mask &= ~(1 << dmastp->selfindex);

  /* Shutting down clocks that are no more required, if any.*/
  if ((dma_streams_mask & STM32_DMA1_STREAMS_MASK) == 0)
    rccDisableDMA1(FALSE);
  if ((dma_streams_mask & STM32_DMA2_STREAMS_MASK) == 0)
    rccDisableDMA2(FALSE);
}

#endif /* STM32_DMA_REQUIRED */

/** @} */
