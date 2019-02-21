/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __BUTTON_H
#define __BUTTON_H

#ifdef __cplusplus
 extern "C" {
#endif 

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx_hal.h"



typedef enum 
{
  BUTTON_ESCAPE = 0,
	BUTTON_DATA   = 1,
  BUTTON_ENTER  = 2,
  BUTTON_LEFT   = 3,
  BUTTON_RIGHT  = 4,
	BUTTON_SYSTEM = 5,
} ButtonType_TypeDef;


#define BUTTONn   1
/**
 * @brief USER push-button
 */
#define BUTTON_ESCAPE_PIN             GPIO_PIN_4
#define BUTTON_DATA_PIN               GPIO_PIN_8     
#define BUTTON_ENTER_PIN              GPIO_PIN_5
#define BUTTON_LEFT_PIN               GPIO_PIN_7    
#define BUTTON_RIGHT_PIN              GPIO_PIN_6   

#define BUTTONs_PINS                  (BUTTON_ESCAPE_PIN | BUTTON_DATA_PIN | BUTTON_ENTER_PIN | BUTTON_LEFT_PIN | BUTTON_RIGHT_PIN)          /* PA.12-PA.15 */
#define BUTTONs_GPIO_PORT             GPIOA
//#define BUTTONs_GPIO_CLK_ENABLE()     __HAL_RCC_GPIOA_CLK_ENABLE()
//#define BUTTONs_GPIO_CLK_DISABLE()    __HAL_RCC_GPIOA_CLK_DISABLE()


void   Button_Init(void);
  
#ifdef __cplusplus
}
#endif
  
#endif 
