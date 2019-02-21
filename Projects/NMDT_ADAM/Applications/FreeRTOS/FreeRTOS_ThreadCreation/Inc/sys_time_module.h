/**
  ******************************************************************************
  * File Name          : sys_time_module.h
  * Description        : This file provides a module updating system data&time. 
  * 
  ******************************************************************************
  *
  * COPYRIGHT(c) 2015 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SYS_TIME_MODULE_H
#define __SYS_TIME_MODULE_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx_hal.h"
#include "main.h"
#include "rtc.h"
#include "pcf8563.h"
#include "time_related.h"
#include "time.h"

typedef struct
{
	char number[5];/*[001,*/
	char shijian[10];/*年月日时分*/
	char wendu[4];/*气温  +211=21.1*/
	char shidu[3];/*湿度 076=76%*/
	char qiya[5];/*气压 10171=1017.1*/
	char fengxiang_1[3];/*瞬时风向254*/
	char fengsu_1[3];/*瞬时风速010=1.0*/
	char fengxiang_2[3];/*2分风向*/
	char fengsu_2[3];/*2分风速*/
	char fengxiang_10[3];/*10分风向*/
	char fengsu_10[3];/*10分风速*/
	char diwen[4];/*地温  "///"*/
	char dianya[3];/*电压 080=8.0V*/
	char yuliang_m[2];/*分钟雨量*/
	char yuliang_h[4];/*小时雨量*/
	char yuliang_d[5];/*日雨量*/
}History_Data;
extern History_Data historydata;
/* Exported types ------------------------------------------------------------*/
extern uint8_t DATA_Number;
extern int number_N;
/* Exported constants --------------------------------------------------------*/


int32_t init_sys_time_module(void);
int32_t get_sys_time(RTC_DateTypeDef *sDate,RTC_TimeTypeDef *sTime);
int32_t get_sys_time_tm(struct tm *DateTime);
int32_t set_sys_time(RTC_DateTypeDef *sDate,RTC_TimeTypeDef *sTime);


#ifdef __cplusplus
}
#endif
#endif /*__SYS_TIME_MODULE_H */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
