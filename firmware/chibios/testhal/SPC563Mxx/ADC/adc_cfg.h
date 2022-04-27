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

#ifndef _ADC_CFG_H_
#define _ADC_CFG_H_

#define ADC_GRP1_NUM_CHANNELS   5
#define ADC_GRP1_BUF_DEPTH      8

#define ADC_GRP2_NUM_CHANNELS   8
#define ADC_GRP2_BUF_DEPTH      16

extern const ADCConversionGroup adcgrpcfg1;
extern const ADCConversionGroup adcgrpcfg2;

#ifdef __cplusplus
extern "C" {
#endif
  void adccallback(ADCDriver *adcp, adcsample_t *buffer, size_t n);
  void adcerrorcallback(ADCDriver *adcp, adcerror_t err);
#ifdef __cplusplus
}
#endif

#endif /* _ADC_CFG_H_ */
