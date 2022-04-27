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
 * @file    STM32F30x/stm32_dma.c
 * @brief   DMA helper driver code.
 *
 * @addtogroup STM32F30x_DMA
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
#define STM32_DMA1_STREAMS_MASK     0x0000007F

/**
 * @brief   Mask of the DMA2 streams in @p dma_streams_mask.
 */
#define STM32_DMA2_STREAMS_MASK     0x00000F80

/**
 * @brief   Post-reset value of the stream CCR register.
 */
#define STM32_DMA_CCR_RESET_VALUE   0x00000000

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   DMA streams descriptors.
 * @details This table keeps the association between an unique stream
 *          identifier and the involved physical registers.
 * @note    Don't use this array directly, use the appropriate wrapper macros
 *          instead: @p STM32_DMA1_STREAM1, @p STM32_DMA1_STREAM2 etc.
 */
const stm32_dma_stream_t _stm32_dma_streams[STM32_DMA_STREAMS] = {
  {DMA1_Channel1, &DMA1->IFCR, 0, 0, DMA1_Channel1_IRQn},
  {DMA1_Channel2, &DMA1->IFCR, 4, 1, DMA1_Channel2_IRQn},
  {DMA1_Channel3, &DMA1->IFCR, 8, 2, DMA1_Channel3_IRQn},
  {DMA1_Channel4, &DMA1->IFCR, 12, 3, DMA1_Channel4_IRQn},
  {DMA1_Channel5, &DMA1->IFCR, 16, 4, DMA1_Channel5_IRQn},
  {DMA1_Channel6, &DMA1->IFCR, 20, 5, DMA1_Channel6_IRQn},
  {DMA1_Channel7, &DMA1->IFCR, 24, 6, DMA1_Channel7_IRQn},
  {DMA2_Channel1, &DMA2->IFCR, 0, 7, DMA2_Channel1_IRQn},
  {DMA2_Channel2, &DMA2->IFCR, 4, 8, DMA2_Channel2_IRQn},
  {DMA2_Channel3, &DMA2->IFCR, 8, 9, DMA2_Channel3_IRQn},
  {DMA2_Channel4, &DMA2->IFCR, 12, 10, DMA2_Channel4_IRQn},
  {DMA2_Channel5, &DMA2->IFCR, 16, 11, DMA2_Channel5_IRQn},
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
 * @brief   DMA1 stream 1 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector6C) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA1->ISR >> 0) & STM32_DMA_ISR_MASK;
  DMA1->IFCR = flags << 0;
  if (dma_isr_redir[0].dma_func)
    dma_isr_redir[0].dma_func(dma_isr_redir[0].dma_param, flags);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   DMA1 stream 2 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector70) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA1->ISR >> 4) & STM32_DMA_ISR_MASK;
  DMA1->IFCR = flags << 4;
  if (dma_isr_redir[1].dma_func)
    dma_isr_redir[1].dma_func(dma_isr_redir[1].dma_param, flags);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   DMA1 stream 3 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector74) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA1->ISR >> 8) & STM32_DMA_ISR_MASK;
  DMA1->IFCR = flags << 8;
  if (dma_isr_redir[2].dma_func)
    dma_isr_redir[2].dma_func(dma_isr_redir[2].dma_param, flags);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   DMA1 stream 4 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector78) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA1->ISR >> 12) & STM32_DMA_ISR_MASK;
  DMA1->IFCR = flags << 12;
  if (dma_isr_redir[3].dma_func)
    dma_isr_redir[3].dma_func(dma_isr_redir[3].dma_param, flags);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   DMA1 stream 5 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector7C) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA1->ISR >> 16) & STM32_DMA_ISR_MASK;
  DMA1->IFCR = flags << 16;
  if (dma_isr_redir[4].dma_func)
    dma_isr_redir[4].dma_func(dma_isr_redir[4].dma_param, flags);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   DMA1 stream 6 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector80) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA1->ISR >> 20) & STM32_DMA_ISR_MASK;
  DMA1->IFCR = flags << 20;
  if (dma_isr_redir[5].dma_func)
    dma_isr_redir[5].dma_func(dma_isr_redir[5].dma_param, flags);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   DMA1 stream 7 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector84) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA1->ISR >> 24) & STM32_DMA_ISR_MASK;
  DMA1->IFCR = flags << 24;
  if (dma_isr_redir[6].dma_func)
    dma_isr_redir[6].dma_func(dma_isr_redir[6].dma_param, flags);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   DMA2 stream 1 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector120) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA2->ISR >> 0) & STM32_DMA_ISR_MASK;
  DMA2->IFCR = flags << 0;
  if (dma_isr_redir[7].dma_func)
    dma_isr_redir[7].dma_func(dma_isr_redir[7].dma_param, flags);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   DMA2 stream 2 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector124) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA2->ISR >> 4) & STM32_DMA_ISR_MASK;
  DMA2->IFCR = flags << 4;
  if (dma_isr_redir[8].dma_func)
    dma_isr_redir[8].dma_func(dma_isr_redir[8].dma_param, flags);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   DMA2 stream 3 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector128) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA2->ISR >> 8) & STM32_DMA_ISR_MASK;
  DMA2->IFCR = flags << 8;
  if (dma_isr_redir[9].dma_func)
    dma_isr_redir[9].dma_func(dma_isr_redir[9].dma_param, flags);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   DMA2 stream 4 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector12C) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA2->ISR >> 12) & STM32_DMA_ISR_MASK;
  DMA2->IFCR = flags << 12;
  if (dma_isr_redir[10].dma_func)
    dma_isr_redir[10].dma_func(dma_isr_redir[10].dma_param, flags);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   DMA2 stream 5 shared interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector130) {
  uint32_t flags;

  CH_IRQ_PROLOGUE();

  flags = (DMA2->ISR >> 16) & STM32_DMA_ISR_MASK;
  DMA2->IFCR = flags << 16;
  if (dma_isr_redir[11].dma_func)
    dma_isr_redir[11].dma_func(dma_isr_redir[11].dma_param, flags);

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
    _stm32_dma_streams[i].channel->CCR = 0;
    dma_isr_redir[i].dma_func = NULL;
  }
  DMA1->IFCR = 0xFFFFFFFF;
#if STM32_HAS_DMA2
  DMA2->IFCR = 0xFFFFFFFF;
#endif
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
#if STM32_HAS_DMA2
  if ((dma_streams_mask & STM32_DMA2_STREAMS_MASK) != 0)
    rccEnableDMA2(FALSE);
#endif

  /* Putting the stream in a safe state.*/
  dmaStreamDisable(dmastp);
  dmastp->channel->CCR = STM32_DMA_CCR_RESET_VALUE;

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
#if STM32_HAS_DMA2
  if ((dma_streams_mask & STM32_DMA2_STREAMS_MASK) == 0)
    rccDisableDMA2(FALSE);
#endif
}

#endif /* STM32_DMA_REQUIRED */

/** @} */
