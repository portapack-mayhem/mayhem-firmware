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
 * @file    SPC560Pxx/spc560p_registry.h
 * @brief   SPC560Pxx capabilities registry.
 *
 * @addtogroup HAL
 * @{
 */

#ifndef _SPC560P_REGISTRY_H_
#define _SPC560P_REGISTRY_H_

/*===========================================================================*/
/* Platform capabilities.                                                    */
/*===========================================================================*/

/**
 * @name    SPC560Pxx capabilities
 * @{
 */
/* eDMA attributes.*/
#define SPC5_HAS_EDMA                       TRUE
#define SPC5_EDMA_NCHANNELS                 16
#define SPC5_EDMA_HAS_MUX                   TRUE

/* LINFlex attributes.*/
#define SPC5_HAS_LINFLEX0                   TRUE
#define SPC5_LINFLEX0_PCTL                  48
#define SPC5_LINFLEX0_RXI_HANDLER           vector79
#define SPC5_LINFLEX0_TXI_HANDLER           vector80
#define SPC5_LINFLEX0_ERR_HANDLER           vector81
#define SPC5_LINFLEX0_RXI_NUMBER            79
#define SPC5_LINFLEX0_TXI_NUMBER            80
#define SPC5_LINFLEX0_ERR_NUMBER            81
#define SPC5_LINFLEX0_CLK                   (halSPCGetSystemClock() / 1)

#define SPC5_HAS_LINFLEX1                   TRUE
#define SPC5_LINFLEX1_PCTL                  49
#define SPC5_LINFLEX1_RXI_HANDLER           vector99
#define SPC5_LINFLEX1_TXI_HANDLER           vector100
#define SPC5_LINFLEX1_ERR_HANDLER           vector101
#define SPC5_LINFLEX1_RXI_NUMBER            99
#define SPC5_LINFLEX1_TXI_NUMBER            100
#define SPC5_LINFLEX1_ERR_NUMBER            101
#define SPC5_LINFLEX1_CLK                   (halSPCGetSystemClock() / 1)

#define SPC5_HAS_LINFLEX2                   FALSE

#define SPC5_HAS_LINFLEX3                   FALSE

/* SIUL attributes.*/
#define SPC5_HAS_SIUL                       TRUE
#define SPC5_SIUL_NUM_PORTS                 8
#define SPC5_SIUL_NUM_PCRS                  108
#define SPC5_SIUL_NUM_PADSELS               36

/* FlexPWM attributes.*/
#define SPC5_HAS_FLEXPWM0                   TRUE
#define SPC5_FLEXPWM0_PCTL                  41
#define SPC5_FLEXPWM0_RF0_HANDLER           vector179
#define SPC5_FLEXPWM0_COF0_HANDLER          vector180
#define SPC5_FLEXPWM0_CAF0_HANDLER          vector181
#define SPC5_FLEXPWM0_RF1_HANDLER           vector182
#define SPC5_FLEXPWM0_COF1_HANDLER          vector183
#define SPC5_FLEXPWM0_CAF1_HANDLER          vector184
#define SPC5_FLEXPWM0_RF2_HANDLER           vector185
#define SPC5_FLEXPWM0_COF2_HANDLER          vector186
#define SPC5_FLEXPWM0_CAF2_HANDLER          vector187
#define SPC5_FLEXPWM0_RF3_HANDLER           vector188
#define SPC5_FLEXPWM0_COF3_HANDLER          vector189
#define SPC5_FLEXPWM0_CAF3_HANDLER          vector190
#define SPC5_FLEXPWM0_FFLAG_HANDLER         vector191
#define SPC5_FLEXPWM0_REF_HANDLER           vector192
#define SPC5_FLEXPWM0_RF0_NUMBER            179
#define SPC5_FLEXPWM0_COF0_NUMBER           180
#define SPC5_FLEXPWM0_CAF0_NUMBER           181
#define SPC5_FLEXPWM0_RF1_NUMBER            182
#define SPC5_FLEXPWM0_COF1_NUMBER           183
#define SPC5_FLEXPWM0_CAF1_NUMBER           184
#define SPC5_FLEXPWM0_RF2_NUMBER            185
#define SPC5_FLEXPWM0_COF2_NUMBER           186
#define SPC5_FLEXPWM0_CAF2_NUMBER           187
#define SPC5_FLEXPWM0_RF3_NUMBER            188
#define SPC5_FLEXPWM0_COF3_NUMBER           189
#define SPC5_FLEXPWM0_CAF3_NUMBER           190
#define SPC5_FLEXPWM0_FFLAG_NUMBER          191
#define SPC5_FLEXPWM0_REF_NUMBER            192
#define SPC5_FLEXPWM0_CLK                   SPC5_MCONTROL_CLK

#define SPC5_HAS_FLEXPWM1                   FALSE

/* eTimer attributes.*/
#define SPC5_HAS_ETIMER0                    TRUE
#define SPC5_ETIMER0_PCTL                   38
#define SPC5_ETIMER0_TC0IR_HANDLER          vector157
#define SPC5_ETIMER0_TC1IR_HANDLER          vector158
#define SPC5_ETIMER0_TC2IR_HANDLER          vector159
#define SPC5_ETIMER0_TC3IR_HANDLER          vector160
#define SPC5_ETIMER0_TC4IR_HANDLER          vector161
#define SPC5_ETIMER0_TC5IR_HANDLER          vector162
#define SPC5_ETIMER0_WTIF_HANDLER           vector165
#define SPC5_ETIMER0_RCF_HANDLER            vector167
#define SPC5_ETIMER0_TC0IR_NUMBER           157
#define SPC5_ETIMER0_TC1IR_NUMBER           158
#define SPC5_ETIMER0_TC2IR_NUMBER           159
#define SPC5_ETIMER0_TC3IR_NUMBER           160
#define SPC5_ETIMER0_TC4IR_NUMBER           161
#define SPC5_ETIMER0_TC5IR_NUMBER           162
#define SPC5_ETIMER0_WTIF_NUMBER            165
#define SPC5_ETIMER0_RCF_NUMBER             167
#define SPC5_ETIMER0_CLK                    SPC5_MCONTROL_CLK

#define SPC5_HAS_ETIMER1                    TRUE
#define SPC5_ETIMER1_PCTL                   39
#define SPC5_ETIMER1_TC0IR_HANDLER          vector168
#define SPC5_ETIMER1_TC1IR_HANDLER          vector169
#define SPC5_ETIMER1_TC2IR_HANDLER          vector170
#define SPC5_ETIMER1_TC3IR_HANDLER          vector171
#define SPC5_ETIMER1_TC4IR_HANDLER          vector172
#define SPC5_ETIMER1_TC5IR_HANDLER          vector173
#define SPC5_ETIMER1_RCF_HANDLER            vector178
#define SPC5_ETIMER1_TC0IR_NUMBER           168
#define SPC5_ETIMER1_TC1IR_NUMBER           169
#define SPC5_ETIMER1_TC2IR_NUMBER           170
#define SPC5_ETIMER1_TC3IR_NUMBER           171
#define SPC5_ETIMER1_TC4IR_NUMBER           172
#define SPC5_ETIMER1_TC5IR_NUMBER           173
#define SPC5_ETIMER1_RCF_NUMBER             178
#define SPC5_ETIMER1_CLK                    SPC5_MCONTROL_CLK

#define SPC5_HAS_ETIMER2                    FALSE
/** @} */

#endif /* _SPC560P_REGISTRY_H_ */

/** @} */
