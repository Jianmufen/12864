/* Includes ------------------------------------------------------------------*/
#include "button.h"
//#include "cmsis_os.h"


void Button_Init(void)
{
  GPIO_InitTypeDef gpioinitstruct = {0};

  /* Enable the BUTTON Clock */
  __HAL_RCC_GPIOA_CLK_ENABLE();
 

  /* Configure Button pin as input with External interrupt  PA1外部中断 */
  gpioinitstruct.Pin   = GPIO_PIN_1;
  gpioinitstruct.Pull  = GPIO_NOPULL;
  gpioinitstruct.Speed = GPIO_SPEED_FREQ_HIGH;
  gpioinitstruct.Mode  = GPIO_MODE_IT_FALLING; 
  //gpioinitstruct.Mode  = GPIO_MODE_IT_RISING; 
  HAL_GPIO_Init(GPIOA, &gpioinitstruct);

  /* Enable and set Button EXTI Interrupt to the lowest priority PA1中断优先级*/
  HAL_NVIC_SetPriority((IRQn_Type)(EXTI1_IRQn), 0x0A, 0);
  HAL_NVIC_EnableIRQ((IRQn_Type)(EXTI1_IRQn));
	
	/*Configure GPIO pins :  PA4 PA5 PA6 
                           PA7 PA8 */
  gpioinitstruct.Pin = BUTTONs_PINS;
  gpioinitstruct.Mode = GPIO_MODE_INPUT;
  gpioinitstruct.Pull = GPIO_PULLUP;
	gpioinitstruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(BUTTONs_GPIO_PORT, &gpioinitstruct);
}

/**
  * @}
  */ 

/**
  * @}
  */ 


/**
  * @}
  */ 

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
