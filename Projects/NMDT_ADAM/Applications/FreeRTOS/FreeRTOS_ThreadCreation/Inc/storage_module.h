/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STORAGE_MODULE_H
#define __STORAGE_MODULE_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx_hal.h"
#include "main.h"
#include "time.h"

extern char data_flag;
extern uint32_t debug_usart ;
extern uint32_t humi,windirect,twodirect,tendirect;    //湿度   风向  2风向  10风向
extern float voltage,temperature,airpressure,wspeed,twospeed,tenspeed;//电压	 温度   气压  风速 2风速  10风速
extern uint8_t space;
extern uint8_t USART2_FLAG;//串口2连接标志
extern	uint8_t DATA_FLAG ;
extern int32_t button1,space1,Tcorrect,Hcorrect,Wcorrect,Airpressure,Aconstant,Soil;
extern uint32_t Aircon,zhanhao;
//	 extern int16_t button1,space1,Tcorrect,Hcorrect,Wcorrect,Aconstant;
//   extern uint32_t Aircon,complex;
int32_t init_storage_module(void);
	 
#ifdef __cplusplus
}
#endif
#endif /*__STORAGE_MODULE_H */
