/**
  ******************************************************************************
  * File Name          : display_module.h
  * Description        : This file provides a module displaying on lcd. 
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
#ifndef __DISPLAY_MODULE_H
#define __DISPLAY_MODULE_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx_hal.h"
#include "main.h"
#include "time.h"
   
#include "lcd.h"
#include "lcdfont.h"
#include "usart.h"
/* Exported types ------------------------------------------------------------*/
/*设置参数结构体*/
typedef struct Parameter
{
	int16_t M1;
	int16_t M2;
	int16_t M3;
	int16_t M4;
	int16_t M5;
	int16_t A1;
	int16_t A2;
	int16_t A3;
	int16_t A4;
	int16_t A5;
	int16_t A6;
	int16_t A7;
	int16_t B1;
	int16_t B2;
	int16_t B3;
	int16_t B4;
	int16_t B5;
	int16_t B6;
	int16_t B7;
	int16_t C1;
	int16_t C2;
	int16_t C3;
	int16_t C4;
	int16_t C5;
	int16_t C6;
	int16_t C7;
	int16_t D4;
	int16_t D6;
	int16_t E4;
	int16_t A8;
	int16_t B8;
	int16_t C8;
}ParameterTypeDef;
extern ParameterTypeDef parameter;
/* Exported constants --------------------------------------------------------*/
extern uint32_t rainzhi1,rainzhi2,rainzhi3;


int32_t init_display_module(void);


#ifdef __cplusplus
}
#endif
#endif /*__DISPLAY_MODULE_H */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
