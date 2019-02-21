/* Includes ------------------------------------------------------------------*/
#include "sys_time_module.h"
#include "cmsis_os.h"

#include "storage_module.h"
#include "display_module.h"
#include "iwdg.h"
#include "usart.h"
#include "gpio.h"
#include "at24c512.h"
#include "at_iic.h"
#include "button.h"
#include "eeprom.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define systimeSTACK_SIZE   configMINIMAL_STACK_SIZE
#define systimePRIORITY     osPriorityHigh

/* RTC Time*/
static RTC_TimeTypeDef sys_time;
static RTC_DateTypeDef sys_date;
int number_N = 0;

History_Data historydata;
uint8_t DATA_Number;
/* os relative */
static osThreadId SysTimeThreadHandle;
static osSemaphoreId semaphore;
static osMutexId mutex;
/* Private function prototypes -----------------------------------------------*/
static void SysTime_Thread(void const *argument);

/**
  * @brief  Init System Time Module. 
  * @retval 0:success;-1:failed
  */
int32_t init_sys_time_module(void)
{
	MX_RTC_Init();
	Button_Init();
	 /* Init extern RTC PCF8563 */
  if(IIC_Init() == HAL_ERROR)
  {
    printf("init pcf8563 failed!\r\n");
  }
  else
  {
    /* synchronize internal RTC with pcf8563 */
    sync_time();
  }
  /* Init RTC Internal */
  
	
  
 
 
  
  /* Define used semaphore */
  osSemaphoreDef(SEM);
  /* Create the semaphore used by the two threads */
  semaphore=osSemaphoreCreate(osSemaphore(SEM), 1);
  if(semaphore == NULL)
  {
    printf("Create Semaphore failed!\r\n");
    return -1;
  }
  
  /* Create the mutex */
  osMutexDef(Mutex);
  mutex=osMutexCreate(osMutex(Mutex));
  if(mutex == NULL)
  {
    printf("Create Mutex failed!\r\n");
    return -1;
  }
  
  /* Create a thread to update system date and time */
  osThreadDef(SysTime, SysTime_Thread, systimePRIORITY, 0, systimeSTACK_SIZE);
  SysTimeThreadHandle=osThreadCreate(osThread(SysTime), NULL);
  if(SysTimeThreadHandle == NULL)
  {
    printf("Create System Time Thread failed!\r\n");
    return -1;
  }
  return 0;
}

/**
  * @brief  get System Date and Time. 
  * @retval 0:success;-1:failed
  */
int32_t get_sys_time(RTC_DateTypeDef *sDate,RTC_TimeTypeDef *sTime)
{
  /* Wait until a Mutex becomes available */
  if(osMutexWait(mutex,500)==osOK)
  {
    if(sDate)
    {
      *sDate=sys_date;
    }
    if(sTime)
    {
      *sTime=sys_time;
    }
    
    /* Release mutex */
    osMutexRelease(mutex);
    
    return 0;
  }
  else
  {
    /* Time */
    if(sTime)
    {
      sTime->Seconds=0;
      sTime->Minutes=0;
      sTime->Hours=0;
    }
    /* Date */
    if(sDate)
    {
      sDate->Date=1;
      sDate->WeekDay=RTC_WEEKDAY_SUNDAY;
      sDate->Month=(uint8_t)RTC_Bcd2ToByte(RTC_MONTH_JANUARY);
      sDate->Year=0;
    }
    
    return -1;
  }
}

int32_t get_sys_time_tm(struct tm *DateTime)
{
  /* Wait until a Mutex becomes available */
  if(osMutexWait(mutex,500)==osOK)
  {
    if(DateTime)
    {
      DateTime->tm_year=sys_date.Year+2000;
      DateTime->tm_mon=sys_date.Month;
      DateTime->tm_mday=sys_date.Date;
      DateTime->tm_hour=sys_time.Hours;
      DateTime->tm_min=sys_time.Minutes;
      DateTime->tm_sec=sys_time.Seconds;
    }
    
    /* Release mutex */
    osMutexRelease(mutex);
    
    return 0;
  }
  else
  {
    if(DateTime)
    {
      DateTime->tm_year=2000;
      DateTime->tm_mon=0;
      DateTime->tm_mday=0;
      DateTime->tm_hour=0;
      DateTime->tm_min=0;
      DateTime->tm_sec=0;
    }
    
    return -1;
  }
}

int32_t set_sys_time(RTC_DateTypeDef *sDate,RTC_TimeTypeDef *sTime)
{
  int32_t res=0;
  
  /* Wait until a Mutex becomes available */
  if(osMutexWait(mutex,500)==osOK)
  {
    if(sDate)
    {
      sys_date=*sDate;
    }
    if(sTime)
    {
      sys_time=*sTime;
    }
    
    /* check param */
    if(IS_RTC_YEAR(sys_date.Year) && IS_RTC_MONTH(sys_date.Month) && IS_RTC_DATE(sys_date.Date) &&
       IS_RTC_HOUR24(sys_time.Hours) && IS_RTC_MINUTES(sys_time.Minutes) && IS_RTC_SECONDS(sys_time.Seconds))
    {
    
      if((HAL_RTC_SetDate(&hrtc,&sys_date,FORMAT_BIN)==HAL_OK)&&  /* internal RTC */
         (HAL_RTC_SetTime(&hrtc,&sys_time,FORMAT_BIN)==HAL_OK)&&
         (PCF8563_Set_Time(sys_date.Year,sys_date.Month,sys_date.Date,sys_time.Hours,sys_time.Minutes,sys_time.Seconds)==HAL_OK) )     /* PCF8563 */
      {
        res=0;
      }
      else
      {
        res=-1;
      }
    }
    else
    {
      res=-1;
    }
    
    /* Release mutex */
    osMutexRelease(mutex);
    
    return res;
  }
  else
  {
    return -1;
  }
}

/**
  * @brief  System sys_time update
  * @param  thread not used
  * @retval None
  */
static void SysTime_Thread(void const *argument)
{
  /* Init IWDG  */
  IWDG_Init();
  
  while(1)
  {
    /* Try to obtain the semaphore */
    if(osSemaphoreWait(semaphore,osWaitForever)==osOK)
    {
			//printf("系统任务成功！！！！\r\n");
      /* Wait until a Mutex becomes available */
      if(osMutexWait(mutex,500)==osOK)
      {
        HAL_RTC_GetTime(&hrtc,&sys_time,FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc,&sys_date,FORMAT_BIN);
        
//        printf("RTC:20%02u-%02u-%02u %02u:%02u:%02u\r\n",
//               sys_date.Year,sys_date.Month,sys_date.Date,
//               sys_time.Hours,sys_time.Minutes,sys_time.Seconds);
				if(debug_usart)
				{
						printf("USART2_FLAG=%d\r\n", USART2_FLAG);
				}
				
        /* synchronize internal RTC with pcf8563 */
        if((sys_time.Minutes==25) && (sys_time.Seconds==15))
        {
					//printf("对时成功\r\n");
          sync_time();
        }
		
				/*当不在设置订正值参数时，每隔10秒，主动读一次数据 并清除有数据标志*/
				if((USART2_FLAG == 0) && ((sys_time.Seconds == 4)||(sys_time.Seconds ==14)||(sys_time.Seconds ==24)||(sys_time.Seconds ==34)||(sys_time.Seconds ==44)||(sys_time.Seconds ==54)))
				{
						DATA_FLAG = false;//发送读取数据之前先清除有数据标志
						HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_SET);//485芯片控制引脚为高电平时，发送数据
						HAL_UART_Transmit(&huart2,(uint8_t *)"<RDATA>",strlen("<RDATA>"),0x0F);
						if(debug_usart)
						{
								HAL_UART_Transmit(&huart1,(uint8_t *)"<RDATA>",strlen("<RDATA>"),0x0F);
						}
						HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_RESET);//485芯片控制引脚为低电平时，接收数据
						DATA_Number++;//发送读取数据命令次数
				}
				
				/*检查是否连上传感器*/
				if((DATA_Number == 6)&&(DATA_FLAG == false))
				{
						temperature = 0;
						humi        = 0;
						airpressure = 0;
						windirect   = 0;
						wspeed      = 0;
						twodirect   = 0; 
						twospeed    = 0;
						tendirect   = 0;
						tenspeed    = 0;
						voltage     = 0;
				}
				
				/*每分钟01秒存储历史数据 同时清零分钟雨量*/
				uint16_t num=0;/*填充的数据长度*/
				if(sys_time.Seconds ==01)
				{
					num += sprintf(historydata.number,"[%03u,",zhanhao);
					num += sprintf(historydata.shijian ,"%02u%02u%02u%02u%02u",sys_date.Year ,sys_date.Month ,sys_date.Date ,sys_time.Hours ,sys_time.Minutes);
					if((airpressure!=0)||(humi!=0)||(temperature!=0)||(windirect!=0)||(wspeed!=0))
					{
						if(temperature<0)
							{
								num += sprintf(historydata.wendu,"-%03u",((uint32_t)(temperature*10)));
							}
	  				else if(temperature>=0)
							{
								num += sprintf(historydata.wendu,"+%03u",((uint32_t)(temperature*10)));
							}
						num += sprintf(historydata.shidu,"%03u",humi);
						num += sprintf(historydata.qiya,"%05u",((uint32_t)(airpressure*10)));
					  num += sprintf(historydata.fengxiang_1,"%03u",windirect);
					  num += sprintf(historydata.fengsu_1,"%03u",((uint32_t)(wspeed*10)));
					  num += sprintf(historydata.fengxiang_2,"%03u",twodirect);
					  num += sprintf(historydata.fengsu_2,"%03u",((uint32_t)(twospeed*10)));
					  num += sprintf(historydata.fengxiang_10,"%03u",tendirect);
					  num += sprintf(historydata.fengsu_10,"%03u",((uint32_t)(tenspeed*10)));
					  num += sprintf(historydata.diwen,"////");
					  num += sprintf(historydata.dianya,"%03u",((uint32_t)(voltage*10)));
					  num += sprintf(historydata.yuliang_m,"%02u",rainzhi1);
					  num += sprintf(historydata.yuliang_h,"%04u",rainzhi2);
					  num += sprintf(historydata.yuliang_d,"%04u]",rainzhi3);
					  /*清零*/
					  rainzhi1=0;
				}
				else
				{
					num += sprintf(historydata.wendu,"////");
					num += sprintf(historydata.shidu,"///");
					num += sprintf(historydata.qiya,"/////");
					num += sprintf(historydata.fengxiang_1,"///");
					num += sprintf(historydata.fengsu_1,"///");
					num += sprintf(historydata.fengxiang_2,"///");
					num += sprintf(historydata.fengsu_2,"///");
					num += sprintf(historydata.fengxiang_10,"///");
					num += sprintf(historydata.fengsu_10,"///");
					num += sprintf(historydata.diwen,"////");
					num += sprintf(historydata.dianya,"///");
					num += sprintf(historydata.yuliang_m,"//");
					num += sprintf(historydata.yuliang_h,"////");
					num += sprintf(historydata.yuliang_d,"////]");
				}					
				
					/*存储分钟历史数据*/
					if(space == 0)
					{}
					else if(space != 0)
					{
						if(sys_time.Minutes % space == 0) /*存储间隔*/
						{
								number_N++;
								if(number_N > 2040)
								{
										number_N = 0;
								}
								//printf("存储次数：%d\r\n",number_N);
								if(number_N < 1025)
								{
										FM24C256_Write_NByte(0xA0, 0x00+0x40*number_N, 64, (char *)&historydata);	//写进去了	  最低位为0表示写  oxa0代表第一个EEPROM的写地址
										FM24C256_Write_NByte(0xA4, 0xFF00, 4, (char *)&number_N);	//写进去了	  最低位为0表示写  oxa0代表第一个EEPROM的写地址
								}
								else
								{
										FM24C256_Write_NByte(0xA4, 0x00+0x40*(number_N-1024), 64, (char *)&historydata);	//写进去了	  最低位为0表示写  oxa0代表第一个EEPROM的写地址
										FM24C256_Write_NByte(0xA4, 0xFF00, 4, (char *)&number_N);	//写进去了	  最低位为0表示写  oxa0代表第一个EEPROM的写地址
								}
						}
					}
				}
				/*每小时的00分 01秒清零小时雨量*/
				if((sys_time.Seconds ==01)&&(sys_time.Minutes == 0))
				{
					rainzhi2 = 0;
				}
        /*每天的00时00分 01秒清零日雨量*/
				if((sys_time.Seconds ==01)&&(sys_time.Minutes == 0)&&(sys_time.Hours == 0))
				{
					rainzhi3 = 0;
				}
        /* Release mutex */
        osMutexRelease(mutex);
        
      }
      else
      {
        //printf("没有等到互斥信号量\r\n");
      }
    }
    
    
    if(hiwdg.Instance)
    {
      HAL_IWDG_Refresh(&hiwdg);  /* refresh the IWDG */
			//printf("喂狗了\r\n");
    }
    
  }
}

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
  
  /* Release the semaphore every 1 second */
   if(semaphore!=NULL)
  {
		//printf("闹钟A中断产生了\r\n");
    osSemaphoreRelease(semaphore);
  }
}


