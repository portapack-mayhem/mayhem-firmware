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
 * @file    memstreams.h
 * @brief   Memory streams structures and macros.
 
 * @addtogroup memory_streams
 * @{
 */

#ifndef _MEMSTREAMS_H_
#define _MEMSTREAMS_H_

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

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
 * @brief   @p RamStream specific data.
 */
#define _memory_stream_data                                                 \
  _base_sequential_stream_data                                              \
  /* Pointer to the stream buffer.*/                                        \
  uint8_t               *buffer;                                            \
  /* Size of the stream.*/                                                  \
  size_t                size;                                               \
  /* Current end of stream.*/                                               \
  size_t                eos;                                                \
  /* Current read offset.*/                                                 \
  size_t                offset;

/**
 * @brief   @p MemStream virtual methods table.
 */
struct MemStreamVMT {
  _base_sequential_stream_methods
};

/**
 * @extends BaseSequentialStream
 *
 * @brief Memory stream object.
 */
typedef struct {
  /** @brief Virtual Methods Table.*/
  const struct MemStreamVMT *vmt;
  _memory_stream_data
} MemoryStream;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void msObjectInit(MemoryStream *msp, uint8_t *buffer,
                    size_t size, size_t eos);
#ifdef __cplusplus
}
#endif

#endif /* _MEMSTREAMS_H_ */

/** @} */
