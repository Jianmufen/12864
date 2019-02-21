/* Includes ------------------------------------------------------------------*/
#include "display_module.h"
#include "cmsis_os.h"

#include "button.h"
#include "time.h"
#include "sys_time_module.h"
#include "storage_module.h"
#include "at_iic.h"
#include "at24c512.h"
/* Private typedef -----------------------------------------------------------*/
/** 
  * @brief  Button Callback Function Definition
  */
typedef void (*ButtonCallbackFunc)(void *,int16_t);
/** 
  * @brief  LCD Display Function Definition
  */
typedef void (*DisplayFunc)(void *);

/** 
  * @brief  LCD Display Screen Structure
  */
typedef struct DisplayScreen
{
  int16_t selection;         /* current selection */
  int16_t screen_number;     /* current screen number */
	int16_t screen_leftright;
  
  ButtonCallbackFunc button_func;      /* button callback function */
  DisplayFunc        display_func;     /* display callback function */
} DisplayScreenTypeDef;
/** 
  * @brief  LCD Display Menu Structure
  */
typedef struct DisplayMenu
{
  DisplayScreenTypeDef Screen;
  
  struct DisplayMenu *prev;    /* previous menu */
  struct DisplayMenu *next;    /* next menu */
} DisplayMenuTypeDef;



/* Private define ------------------------------------------------------------*/
#define displaySTACK_SIZE   configMINIMAL_STACK_SIZE
#define displayPRIORITY     osPriorityNormal
#define QUEUE_SIZE ((uint32_t)1)

/* display */
#define MAX_DISPLAY_ON_TIME              (60)   /* display on time , unit:second */
#define DATA_SCREEN_NUMBER               (3)    /* data screen number */
#define TIME_SCREEN_SELECTION_NUMBER     (6)    /* time screen selection number */
#define MAIN_SCREEN_SELECTION_NUMBER     (3)    /* main screen selection number */
#define COMMON_SCREEN_SELECTION_NUMBER   (4)    /*�������Ĺ������*/
#define CORRECT_SCREEN_SELECTION_NUMBER  (8)    /*�������Ĺ������*/
//#define PRESURE_SCREEN_SELECTION_NUMBER  (4)    /*��ѹ�����������Ĺ������*/
/* Private variables ---------------------------------------------------------*/

/*ϵͳ����*/
ParameterTypeDef parameter;
/* RTC Time*/
static RTC_TimeTypeDef xTime;
static RTC_DateTypeDef xDate;
/* Set Time */
static RTC_TimeTypeDef setTime;
static RTC_DateTypeDef setDate;

/* display */
static uint8_t nian,yue,ri,shi,fen,miao;//����ʱ�����
//static int16_t disp_on_count=MAX_DISPLAY_ON_TIME;  /* ��ʾ������ʱ��display on time counter */
uint32_t rainzhi1,rainzhi2,rainzhi3;
/* Display Menus and Screens -------------------------------------------------*/
/* Menus */
static DisplayMenuTypeDef *CurrentDisplay;  /* Current Display Menu */
static DisplayMenuTypeDef MainMenu;  /* Main Menu */
static DisplayMenuTypeDef DataMenu;  /* Data Menu */
static DisplayMenuTypeDef CommMenu;  /* Communication Param Set Menu */
static DisplayMenuTypeDef TimeMenu;  /* Time Set Menu */
static DisplayMenuTypeDef AverageMenu;  /* ƽ�� Menu */
static DisplayMenuTypeDef PamsMenu;  /* PAMS Set Menu */
static DisplayMenuTypeDef HardwareMenu;  /* Ӳ�� Set Menu */
static DisplayMenuTypeDef CorrectMenu;  /* ���� Set Menu */

/* os relative��ʾ���� */
static osThreadId DisplayThreadHandle;
/*static osSemaphoreId semaphore;*/
static osMessageQId ButtonQueue;  /* ��������button queue */
/* Private function prototypes -----------------------------------------------*/
static void Display_Thread(void const *argument);
static void init_display_menus(void);

static void DataScreen_ButtonHandler(void *Menu,int16_t button);
static void DataScreen_Display(void *Menu);//������ 
static void TimeScreen_ButtonHandler(void *Menu,int16_t button);
static void TimeScreen_Display(void *Menu);//ʱ��������
static void CommScreen_ButtonHandler(void *Menu,int16_t button);
static void CommScreen_Display(void *Menu);//��������
static void MainScreen_ButtonHandler(void *Menu,int16_t button);
static void MainScreen_Display(void *Menu);//���˵���
static void AverageScreen_ButtonHandler(void *Menu,int16_t button);
static void AverageScreen_Display(void *Menu);//ƽ����
static void PamsScreen_ButtonHandler(void *Menu,int16_t button);
static void PamsScreen_Display(void *Menu);//PAMS��
static void HardwareScreen_ButtonHandler(void *Menu,int16_t button);
static void HardwareScreen_Display(void *Menu);//Ӳ����
static void CorrectScreen_ButtonHandler(void *Menu,int16_t button);
static void CorrectScreen_Display(void *Menu);//������

__STATIC_INLINE void short_delay(void);
//__STATIC_INLINE void turn_off_display(void);
//__STATIC_INLINE void turn_on_display(void);

/**��ʼ����ʾ����
  * @brief  Init Display Module. 
  * @retval 0:success;-1:failed
  */
int32_t init_display_module(void)
{
	FM_IIC_Init();
	if(FM24C256_Check()<0)
	{
		printf("AT24C512  ERROR!\r\n");
		return -1;
	}
	AT24C512_Read(0xA4,0xff00,(uint8_t *)&number_N, 4);
	printf("�������Ĵ洢������%d\r\n",number_N);
	if((number_N < 0) || (number_N > 2040))
	{
			number_N = 0;
	}
	
  /* init display menus */
  init_display_menus();
 
  
  
  
  /* Create the queue used by the button interrupt to pass the button value.����һ�����䰴��ֵ�ö��� */
  osMessageQDef(button_queue,QUEUE_SIZE,uint16_t);
  ButtonQueue=osMessageCreate(osMessageQ(button_queue),NULL);
  if(ButtonQueue == NULL)
  {
    printf("Create Button Queue failed!\r\n");
    return -1;
  }
  
  /* Create a thread to update system date and time������ʾ���� */
  osThreadDef(Display, Display_Thread, displayPRIORITY, 0, displaySTACK_SIZE);
  DisplayThreadHandle=osThreadCreate(osThread(Display), NULL);
  if(DisplayThreadHandle == NULL)
  {
    printf("Create Display Thread failed!\r\n");
    return -1;
  }
  
  
  return 0;
}


/**
  * @brief  EXTI line detection callbacks.
  * @param  GPIO_Pin: Specifies the port pin connected to corresponding EXTI line.
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  /* Disable Interrupt */
  HAL_NVIC_DisableIRQ((IRQn_Type)(EXTI1_IRQn));
	HAL_NVIC_DisableIRQ((IRQn_Type)(EXTI15_10_IRQn));
  
  /* eliminate key jitter���� */
  short_delay();
	//osDelay(10);
	//HAL_Delay(10);
  if(HAL_GPIO_ReadPin(GPIOA,GPIO_Pin)==GPIO_PIN_RESET)
  {
    /* Put the Button Value to the Message Queue */
    if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_4)==GPIO_PIN_RESET)  /* ESCAPE button */
    {
      if(ButtonQueue)
      {
        osMessagePut(ButtonQueue, BUTTON_ESCAPE, 100);
      }
    }
    
    if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_5)==GPIO_PIN_RESET)  /* ENTER button */
    {
      if(ButtonQueue)
      {
        osMessagePut(ButtonQueue, BUTTON_ENTER, 100);
      }
    }
    
    if((HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_6)==GPIO_PIN_RESET)&(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_7)==GPIO_PIN_SET))  /* LEFT button */
    {
      if(ButtonQueue)
      {
        osMessagePut(ButtonQueue, BUTTON_LEFT, 100);
      }
    }
    
    if((HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_7)==GPIO_PIN_RESET)&(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_6)==GPIO_PIN_SET))  /* RIGHT button */
    {
      if(ButtonQueue)
      {
        osMessagePut(ButtonQueue, BUTTON_RIGHT, 100);
      }
    }
		if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_8)==GPIO_PIN_RESET)  /* DATA button */
    {
      if(ButtonQueue)
      {
        osMessagePut(ButtonQueue, BUTTON_DATA, 100);
      }
    }
		//short_delay();
		if((HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_7)==GPIO_PIN_RESET)&(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_6)==GPIO_PIN_RESET))
		{
				if(ButtonQueue)
					{
						osMessagePut(ButtonQueue, BUTTON_SYSTEM, 100);     //ϵͳ����Enter��
					}
		}
  
  }
	
	if(GPIO_Pin==GPIO_PIN_11)  /* ������������ */
    {
			if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_11)==0)
			{
				rainzhi1++;/*��������*/
				rainzhi2++;/*Сʱ����*/
				rainzhi3++;/*������*/
			}
			else if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_11)==1)
			{
				//printf("�޼�����ֵ\r\n");
			}
			//__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_11);
    }
  /* Enable Interrupt */
  HAL_NVIC_EnableIRQ((IRQn_Type)(EXTI1_IRQn));
	HAL_NVIC_EnableIRQ((IRQn_Type)(EXTI15_10_IRQn));
}



/**
  * @brief  Display on LCD
  * @param  thread not used
  * @retval None
  */
static void Display_Thread(void const *argument)
{
  osEvent event;
  int16_t button_value=0;
  struct tm datetime={0};
  
  (void)get_sys_time_tm(&datetime);
  
  
  while(1)
  {
    /* Get the message from the queue */
    event = osMessageGet(ButtonQueue, 10);		/*�����ȴ�10msʱ��*/
    /*�а������¾͵�����ʾ��*/
    if (event.status == osEventMessage)//�ж��в���
    {
				data_flag = 0;
      /* get button value */
      button_value=event.value.v;
      
      /* button handler */
      if( CurrentDisplay->Screen.button_func)
      {
        (*CurrentDisplay->Screen.button_func)(CurrentDisplay,button_value);
      }
    }
    
    /* get data&time */
    (void)get_sys_time(&xDate,&xTime);
  
      /* display function ��ʾ��ǰ����*/
      if(CurrentDisplay->Screen.display_func)
      {
        (*CurrentDisplay->Screen.display_func)(CurrentDisplay);
				if(CurrentDisplay->Screen.display_func != CorrectScreen_Display)
				{
						USART2_FLAG = 0;		/*ֻҪ��ʾ���治�Ƕ������������ñ�־λ*/
				}
      }
  }
}

/**
  * @brief  init display menus.
  * @param  None
  * @retval None
  */
static void init_display_menus(void)
{
  /* Data Menu������ */
  DataMenu.prev=&MainMenu;
  DataMenu.next=NULL;
  DataMenu.Screen.screen_number=0;
  DataMenu.Screen.selection=0;
  DataMenu.Screen.button_func=DataScreen_ButtonHandler;
  DataMenu.Screen.display_func=DataScreen_Display;
  
  /* Time Menuʱ�������� */
  TimeMenu.prev=&CommMenu;
  TimeMenu.next=NULL;
  TimeMenu.Screen.screen_number=0;
  TimeMenu.Screen.selection=0;
  TimeMenu.Screen.button_func=TimeScreen_ButtonHandler;
  TimeMenu.Screen.display_func=TimeScreen_Display;
  
  /* Comm Menu ������*/
  CommMenu.prev=&MainMenu;
  CommMenu.next=&PamsMenu;
  CommMenu.Screen.screen_number=0;
  CommMenu.Screen.selection=0;
  CommMenu.Screen.button_func=CommScreen_ButtonHandler;
  CommMenu.Screen.display_func=CommScreen_Display;
  
  /* Main Menu ���˵���*/
  MainMenu.prev=NULL;
  MainMenu.next=&DataMenu;
  MainMenu.Screen.screen_number=0;
  MainMenu.Screen.selection=0;
  MainMenu.Screen.button_func=MainScreen_ButtonHandler;
  MainMenu.Screen.display_func=MainScreen_Display;
  
	/* Average Menuƽ���� */
  AverageMenu.prev=&MainMenu;
  AverageMenu.next=NULL;
  AverageMenu.Screen.screen_number=0;
  AverageMenu.Screen.selection=0;
  AverageMenu.Screen.button_func=AverageScreen_ButtonHandler;
  AverageMenu.Screen.display_func=AverageScreen_Display;
	
	/* PAMS Menu  PAMS�� */
  PamsMenu.prev=&CommMenu;
  PamsMenu.next=NULL;
  PamsMenu.Screen.screen_number=0;
  PamsMenu.Screen.selection=0;
  PamsMenu.Screen.button_func=PamsScreen_ButtonHandler;
  PamsMenu.Screen.display_func=PamsScreen_Display;
	
	
	/* Hardware MenuӲ���� */
  HardwareMenu.prev=&CommMenu;
  HardwareMenu.next=NULL;
  HardwareMenu.Screen.screen_number=0;
  HardwareMenu.Screen.selection=3;
  HardwareMenu.Screen.button_func=HardwareScreen_ButtonHandler;
  HardwareMenu.Screen.display_func=HardwareScreen_Display;
	
	/* Correct Menu������ */
  CorrectMenu.prev=&CommMenu;
  CorrectMenu.next=NULL;
  CorrectMenu.Screen.screen_number=0;
  CorrectMenu.Screen.selection=0;
  CorrectMenu.Screen.button_func=CorrectScreen_ButtonHandler;
  CorrectMenu.Screen.display_func=CorrectScreen_Display;
	
	
  /* Current Menu */
  /*CurrentDisplay=&DataMenu;   //just use Data Menu for now*/
  CurrentDisplay=&MainMenu;  /* display main menu */
}


/**
  * Data Screen
  */

/**
  * @brief  Data Screen Button Handle Function.
  * @param  None
  * @retval None
  */
static void DataScreen_ButtonHandler(void *Menu,int16_t button)
{
  DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
  
  switch(button)
  {
  case BUTTON_ESCAPE:
    if(menu->prev)
    {
      CurrentDisplay=menu->prev;
      CurrentDisplay->Screen.screen_number=0;
      CurrentDisplay->Screen.selection=0;
      OLED_Clear();  /* clear screen */
    }
    break;
  case BUTTON_ENTER:
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_SET);//485оƬ��������Ϊ�ߵ�ƽʱ����������
 	  HAL_UART_Transmit(&huart2,(uint8_t *)"<RDATA>",strlen("<RDATA>"),0x0F);
	  HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_RESET);//485оƬ��������Ϊ�͵�ƽʱ����������
    break;
  case BUTTON_LEFT:
    menu->Screen.screen_number++;
    break;
  case BUTTON_RIGHT:
    menu->Screen.screen_number--;
    break;
	case BUTTON_DATA:
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_SET);//485оƬ��������Ϊ�ߵ�ƽʱ����������
 	  HAL_UART_Transmit(&huart2,(uint8_t *)"<RDATA>",strlen("<RDATA>"),0x0F);
	  HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_RESET);//485оƬ��������Ϊ�͵�ƽʱ����������
    break;
  default:
    break;
  }
  
  if(menu->Screen.screen_number<0)
  {
    menu->Screen.screen_number=DATA_SCREEN_NUMBER-1;
  }
  else if(menu->Screen.screen_number>DATA_SCREEN_NUMBER-1)
  {
    menu->Screen.screen_number=0;
  }
  
}

/**
  * @brief  Data Screen Display Function.
  * @param  None
  * @retval None
  */
static void DataScreen_Display(void *Menu)
{
  DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
	/*��ʾʱ��*/
	/*2017-11-04 ������disp_buf����*/
	memset(disp_buf,0,128);
	snprintf(disp_buf,128,"  %02u:%02u:%02u      ",xTime.Hours,xTime.Minutes,xTime.Seconds);/*2017-11-04 ��ʱ�������ʾ�ո�*/
	OLED_ShowString(0,0,disp_buf,false);//ʱ��
	/*��ʾ����*/
	if(voltage==0)
	{
		OLED_ShowString(104,0,"///",false);
	}
	else if((voltage<7.0)&(voltage!=0))
	{
		OLED_Huatu(104,0,3);//������
	}
	else if((voltage<7.2)&(voltage>=7.0))
	{
		OLED_Huatu(104,0,2);//����֮һ����
	}
	else if((voltage>=7.2)&(voltage<7.4))
	{
		OLED_Huatu(104,0,1);//����֮������
	}
	else if(voltage>=7.4)
	{
		OLED_Huatu(104,0,0);//������
  }
	if(menu->Screen.screen_number==0)
	{
		OLED_China(1,16,17,false);//��
	  OLED_China(17,16,18,false);//��
	  OLED_Char(33,16,':',false);
//		if(temperature == 0)
//		{
			OLED_ShowString(40,16,"     //./",false);
//		}
	  OLED_China(112,16,61,false);//��
	
	  OLED_China(1,32,19,false);//ʪ
	  OLED_China(17,32,20,false);//��
	  OLED_Char(33,32,':',false);
//		if(humi == 0)
//		{
			OLED_ShowString(40,32,"     ///%RH",false);
//		}
	  
		
	  OLED_China(1,48,17,false);//��
	  OLED_China(17,48,21,false);//ѹ
	  OLED_Char(33,48,':',false);
//		if(airpressure == 0)
//		{
			OLED_ShowString(40,48,"  ////./hpa",false);
//		}

		if((temperature!=0)||(humi!=0)||(airpressure!=0))
		{
			/*��ʾ����ʪ����ѹ����*/
			snprintf(disp_buf,sizeof(disp_buf),"%5.1f",temperature);
			OLED_ShowString(72,16,disp_buf,false);//����
			snprintf(disp_buf,sizeof(disp_buf),"%3d",humi);
			OLED_ShowString(80,32,disp_buf,false);//ʪ��
//			OLED_ShowString(104,32,"%RH",false);
			snprintf(disp_buf,sizeof(disp_buf),"%6.1f",airpressure);
			OLED_ShowString(56,48,disp_buf,false);//��ѹ
//			OLED_ShowString(104,48,"hpa",false);
		}
		
	}
	else if(menu->Screen.screen_number==1)
	{
		OLED_China(1,16,22,false);//��
	  OLED_China(17,16,23,false);//��
	  OLED_China(33,16,23,false);//��
	  OLED_China(49,16,24,false);//��
	  OLED_Char(65,16,':',false);
//		if(windirect ==0)
//		{
			OLED_ShowString(73,16,"  ///",false);
//		}
	  OLED_China(112,16,62,false);//��
	
    OLED_China(1,32,22,false);//��
	  OLED_China(17,32,23,false);//��
	  OLED_China(33,32,23,false);//��
	  OLED_China(49,32,25,false);//��
	  OLED_Char(65,32,':',false);
//		if(wspeed==0)
//		{
			OLED_ShowString(72,32,"//./m/s",false);
//		}
	
	  OLED_China(1,48,26,false);//��
	  OLED_ShowString(17,48,"    ",false);
	  OLED_China(49,48,18,false);//��
	  OLED_Char(65,48,':',false);
	  OLED_ShowString(73,48," //./",false);
	  OLED_China(112,48,61,false);//��
		
		if((windirect!=0)||(wspeed!=0))
		{
			/*��ʾ�����������*/
			snprintf(disp_buf,sizeof(disp_buf),"%3d",windirect);
			OLED_ShowString(88,16,disp_buf,false);//����
			snprintf(disp_buf,sizeof(disp_buf),"%4.1f",wspeed);
			OLED_ShowString(72,32,disp_buf,false);//����
//			OLED_ShowString(104,32,"m/s",false);
		}
		
	}
	else if(menu->Screen.screen_number==2)
	{
		OLED_China(1,16,27,false);//��
	  OLED_China(17,16,28,false);//��
	  OLED_China(33,16,29,false);//��
	  OLED_Char(49,16,':',false);
	  OLED_ShowString(56,16,"    0.0mm",false);

    OLED_China(1,32,14,false);//ʱ
	  OLED_China(17,32,28,false);//��
	  OLED_China(33,32,29,false);//��
	  OLED_Char(49,32,':',false);
	  OLED_ShowString(56,32,"    0.0mm",false);
	
	  OLED_China(1,48,10,false);//��
	  OLED_China(17,48,28,false);//��
	  OLED_China(33,48,29,false);//��
	  OLED_Char(49,48,':',false);
	  OLED_ShowString(56,48,"    0.0mm",false);
		
		snprintf(disp_buf,sizeof(disp_buf),"%1u.%01u",rainzhi1/10,rainzhi1%10);//������
		//printf("��������%s\r\n",disp_buf);//����
		OLED_ShowString(88,16,disp_buf,false);//������
		snprintf(disp_buf,sizeof(disp_buf),"%3u.%01u",rainzhi2/10,rainzhi2%10);//ʱ����
		//printf("ʱ������%s\r\n",disp_buf);//����
		OLED_ShowString(72,32,disp_buf,false);//ʱ����
		snprintf(disp_buf,sizeof(disp_buf),"%3u.%01u",rainzhi3/10,rainzhi3%10);//������
		//printf("��������%s\r\n",disp_buf);//����
		OLED_ShowString(72,48,disp_buf,false);//������
	}
	OLED_Refresh_Gram();//ˢ����Ļ
}	


/**
  * Time Screen
  */

/**
  * @brief  Data Screen Button Handle Function.
  * @param  None
  * @retval None
  */
static void TimeScreen_ButtonHandler(void *Menu,int16_t button)
{
  DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
  int16_t selected_timevalue=0,min_value=0,max_value=0;
  uint8_t *selected=NULL;
	
  switch(menu->Screen.selection)
  {
  case 0:  /* year */
    selected=&setDate.Year;
    selected_timevalue=setDate.Year;
    min_value=0;
    max_value=99;
    break;
  case 1:  /* month */
    selected=&setDate.Month;
    selected_timevalue=setDate.Month;
    min_value=1;
    max_value=12;
    break;
  case 2:  /* day */
    selected=&setDate.Date;
    selected_timevalue=setDate.Date;
    min_value=1;
    max_value=31;
    break;
  case 3:  /* hour */
    selected=&setTime.Hours;
    selected_timevalue=setTime.Hours;
    min_value=0;
    max_value=23/*59*/;   /* 16.3.23 hour:0-23 */
    break;
  case 4:  /* minute */
    selected=&setTime.Minutes;
    selected_timevalue=setTime.Minutes;
    min_value=0;
    max_value=59;
    break;
  case 5:  /* second */
    selected=&setTime.Seconds;
    selected_timevalue=setTime.Seconds;
    min_value=0;
    max_value=59;
    break;
  default:
    menu->Screen.selection=5;
    selected=&setTime.Seconds;
    selected_timevalue=setTime.Seconds;
    min_value=0;
    max_value=59;
    break;
  }
  
  switch(button)
  {
  case BUTTON_ESCAPE:
		if((setDate.Year!=nian)||(setDate.Month!=yue)||(setDate.Date!=ri)||(setTime.Hours!=shi)||(setTime.Minutes!=fen)||(setTime.Seconds!=miao))
		{
			/* set data&time */
			/* fill unused value */
			setTime.DayLightSaving=RTC_DAYLIGHTSAVING_NONE;
			setTime.StoreOperation=RTC_STOREOPERATION_RESET;
			setTime.SubSeconds=0;
			setTime.TimeFormat=RTC_HOURFORMAT12_AM;
			if(set_sys_time(&setDate,&setTime)<0)
				{
					OLED_Clear();
					OLED_ShowString(0,24,"set time failed!",1);
					OLED_Refresh_Gram();
				}
				else
					{
						OLED_Clear();
						OLED_ShowString(16,24,"set time ok!",1);
						OLED_Refresh_Gram();
					}
					osDelay(500);
		}
    if(menu->prev)
			{
				CurrentDisplay=menu->prev;
				CurrentDisplay->Screen.screen_number=0;
				CurrentDisplay->Screen.selection=1;
				OLED_Clear();  /* clear screen */
			}
    break;
  case BUTTON_ENTER:
    menu->Screen.selection++;
    break;
  case BUTTON_LEFT:
    selected_timevalue++;
    break;
  case BUTTON_RIGHT:
    selected_timevalue--;
	  break;
	case BUTTON_DATA:
		CurrentDisplay=&DataMenu;
    CurrentDisplay->Screen.screen_number=0;
    CurrentDisplay->Screen.selection=0;
	  OLED_Clear();  /* clear screen */
    break;
  default:
    break;
  }
  
  if(menu->Screen.selection<0)
  {
    menu->Screen.selection=TIME_SCREEN_SELECTION_NUMBER-1;
  }
  else if(menu->Screen.selection>TIME_SCREEN_SELECTION_NUMBER-1)
  {
    menu->Screen.selection=0;
  }
  
  if(selected_timevalue<min_value)
  {
    selected_timevalue=max_value;
  }
  else if(selected_timevalue>max_value)
  {
    selected_timevalue=min_value;
  }
  /* set selected value */
  *selected=selected_timevalue;
}
	
/**
  * @brief  Time Screen Display Function.
  * @param  None
  * @retval None
  */
static void TimeScreen_Display(void *Menu)
{
  DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
  uint8_t highlight=false;
 
	/* year */
	OLED_ShowString(33,0,"20",false);//��20��
	snprintf(disp_buf,sizeof(disp_buf),"%02u",setDate.Year);
	highlight=(menu->Screen.selection==0);
	OLED_ShowString(49,0,disp_buf,highlight);//��17�ꡱ
	OLED_China(73,0,8,false);//��
	
	/* month */
	snprintf(disp_buf,sizeof(disp_buf),"%02u",setDate.Month);
	highlight=(menu->Screen.selection==1);
	OLED_ShowString(25,24,disp_buf,highlight);//��06�¡�
	OLED_China(49,24,9,false);//��
	
	/* day */
	snprintf(disp_buf,sizeof(disp_buf),"%02u",setDate.Date);
	highlight=(menu->Screen.selection==2);
	OLED_ShowString(65,24,disp_buf,highlight);//��22�ա�
	OLED_China(89,24,10,false);//��
	
	/* hour */
	snprintf(disp_buf,sizeof(disp_buf),"%02u",setTime.Hours);
	highlight=(menu->Screen.selection==3);
	OLED_ShowString(9,48,disp_buf,highlight);//��10�㡱
	OLED_China(33,48,14,false);//ʱ
	
	/* minute */
	snprintf(disp_buf,sizeof(disp_buf),"%02u",setTime.Minutes);
	highlight=(menu->Screen.selection==4);
	OLED_ShowString(49,48,disp_buf,highlight);//��10�֡�
	OLED_China(73,48,27,false);//��
	
	/* second */
	snprintf(disp_buf,sizeof(disp_buf),"%02u",setTime.Seconds);
	highlight=(menu->Screen.selection==5);
	OLED_ShowString(89,48,disp_buf,highlight);//��10�롱
	OLED_China(108,48,41,false);//��
	
	OLED_Refresh_Gram();
}	
	
/**
  * Communication Parameter Screen
  */

/**
  * @brief  Communication Screen Button Handle Function.
  * @param  None
  * @retval None
  */
static void CommScreen_ButtonHandler(void *Menu,int16_t button)
{
  DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
  
  switch(button)
  {
  case BUTTON_ESCAPE:
    if(menu->prev)
    {
      CurrentDisplay=menu->prev;
      CurrentDisplay->Screen.screen_number=0;
      CurrentDisplay->Screen.selection=0;
      OLED_Clear();  /* clear screen */
    }
    break;
  case BUTTON_ENTER:
		if(menu->Screen.selection!=3)
		{
		 if(menu->next)
     {
       CurrentDisplay=menu->next;
       CurrentDisplay->Screen.screen_number=0;
       CurrentDisplay->Screen.selection=0;
			 CurrentDisplay->Screen.screen_leftright=0;
       OLED_Clear();  /* clear screen */
			 
			 /*�õ�������ʾʱ��*/
			 get_sys_time(&setDate,&setTime);
			 nian   =  setDate.Year;
			 yue    =  setDate.Month;
			 ri     =  setDate.Date;
			 shi    =  setTime.Hours;
			 fen    =  setTime.Minutes;
			 miao   =  setTime.Seconds;
      }
	   }
    break;
  case BUTTON_LEFT:
    menu->Screen.selection++;
    break;
  case BUTTON_RIGHT:
    menu->Screen.selection--;
    break;
	case BUTTON_DATA:
		CurrentDisplay=&DataMenu;
    CurrentDisplay->Screen.screen_number=0;
    CurrentDisplay->Screen.selection=0;
	  OLED_Clear();  /* clear screen */
	  break;
	case BUTTON_SYSTEM:
		if(menu->Screen.selection==3)
		{
			if(menu->next)
     {
       CurrentDisplay=menu->next;
       CurrentDisplay->Screen.screen_number=0;
       CurrentDisplay->Screen.selection=0;
       OLED_Clear();  /* clear screen */
			 
			 /*ÿ�ν���ϵͳ�������� ����ȡһ��ϵͳ���� ��������<FINDE>*/
			 HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_SET);
			 HAL_UART_Transmit(&huart2,(uint8_t *)"<FINDE>",8,1000);
			 HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_RESET);
		 }
		}
  default:
    break;
  }
  
  if(menu->Screen.selection<0)
  {
    menu->Screen.selection=COMMON_SCREEN_SELECTION_NUMBER-1;
  }
  else if(menu->Screen.selection>COMMON_SCREEN_SELECTION_NUMBER-1)
  {
    menu->Screen.selection=0;
  }
}	
	
/**
  * @brief  Communication Screen Display Function.
  * @param  None
  * @retval None
  */
static void CommScreen_Display(void *Menu)
{
  DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
  uint8_t highlight=false;
  
  /*������ʽ*/
	highlight=(menu->Screen.selection==0);
	OLED_China(1,0,30,highlight);//��
	OLED_China(17,0,31,highlight);//��
	OLED_China(33,0,32,highlight);//��
	OLED_China(49,0,2,highlight);//ʽ
	OLED_ShowString(65,0,"...",false);
	
	/*ʱ������*/
	highlight=(menu->Screen.selection==1);
	OLED_China(1,16,14,highlight);//ʱ
	OLED_China(17,16,40,highlight);//��
	OLED_China(33,16,15,highlight);//��
	OLED_China(49,16,16,highlight);//��
	OLED_ShowString(65,16,"...",false);
	
	/*Ӳ������*/
	highlight=(menu->Screen.selection==2);
	OLED_China(1,32,42,highlight);//Ӳ
	OLED_China(17,32,43,highlight);//��
	OLED_China(33,32,15,highlight);//��
	OLED_China(49,32,16,highlight);//��
	OLED_ShowString(65,32,"...",false);
	
	/*ϵͳ����*/
	highlight=(menu->Screen.selection==3);
	OLED_China(1,48,55,highlight);//ϵ
	OLED_China(17,48,56,highlight);//ͳ
	OLED_China(33,48,57,highlight);//��
	OLED_China(49,48,58,highlight);//��
	OLED_ShowString(65,48," ***",false);
	
	/* Determine the next Menu */
  switch(menu->Screen.selection)
  {
  case 0:   /* PAMS Menu */
    menu->next=&PamsMenu;
    break;
  case 1:   /* TIME Menu */
    menu->next=&TimeMenu;
    break;
  case 2:   /* Hardware Menu */
    menu->next=&HardwareMenu;
    break;
	case 3:   /* Correct Menu */
    menu->next=&CorrectMenu;
    break;
  default:
    //menu->next=&PamsMenu;
    break;
  }
	OLED_Refresh_Gram();
}	
	
/**
  * Main Screen
  */

/**
  * @brief  Main Screen Button Handle Function.
  * @param  None
  * @retval None
  */
static void MainScreen_ButtonHandler(void *Menu,int16_t button)
{
  DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
  
  switch(button)
  {
  case BUTTON_ESCAPE:
    break;
  case BUTTON_ENTER:   /* enter next menu */
    if(menu->next)
    {
      CurrentDisplay=menu->next;
      CurrentDisplay->Screen.screen_number=0;
      CurrentDisplay->Screen.selection=0;
      OLED_Clear();  /* clear screen */
  
//      /* get system time */
//      get_sys_time(&setDate,&setTime);
    }
    break;
  case BUTTON_LEFT:
    menu->Screen.selection++;
    break;
  case BUTTON_RIGHT:
    menu->Screen.selection--;
    break;
	case BUTTON_DATA:
		CurrentDisplay=&DataMenu;
    CurrentDisplay->Screen.screen_number=0;
    CurrentDisplay->Screen.selection=0;
	  OLED_Clear();  /* clear screen */
	  break;
  default:
    break;
  }
  
  
  if(menu->Screen.selection<0)
  {
    menu->Screen.selection=MAIN_SCREEN_SELECTION_NUMBER-1;
  }
  else if(menu->Screen.selection>MAIN_SCREEN_SELECTION_NUMBER-1)
  {
    menu->Screen.selection=0;
  }
}
	
/**
  * @brief  Main Screen Display Function.
  * @param  None
  * @retval None
  */
static void MainScreen_Display(void *Menu)
{
  DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
  uint8_t highlight=false;
	uint8_t i=0;
	
	/*Date*/
	snprintf(disp_buf,sizeof(disp_buf),"%02u",xDate.Year);
	OLED_ShowString(4,0,disp_buf,false);
	OLED_China(20,0,8,false);//��
	snprintf(disp_buf,sizeof(disp_buf),"%02u",xDate.Month);
	OLED_ShowString(36,0,disp_buf,false);
  OLED_China(52,0,9,false);//��
	snprintf(disp_buf,sizeof(disp_buf),"%02u",xDate.Date);
	OLED_ShowString(68,0,disp_buf,false);
  OLED_China(84,0,10,false);//��
	OLED_ShowString(0,48,"  ",false);
	OLED_ShowString(48,48,"    ",false);
	OLED_ShowString(112,48,"  ",false);
	OLED_ShowString(0,32,"      ",false);
	OLED_ShowString(80,32,"      ",false);
		
	 /* Time */
  snprintf(disp_buf,sizeof(disp_buf)," %02u:%02u:%02u",
           xTime.Hours,xTime.Minutes,xTime.Seconds);
	OLED_ShowString(0,16,disp_buf,false);//ʱ����
//	OLED_ShowString(8,16,disp_buf,false);//ʱ����
	
	OLED_ShowString(72,16,"   S00M",false);
//	OLED_ShowString(80,16,"  S00M",false);
	snprintf(disp_buf,sizeof(disp_buf),"%02u",space);
	OLED_ShowString(104,16,disp_buf,false);

	/*������־*/
  if(voltage==0.0)
		{
			OLED_ShowString(104,0,"///",false);
		}
	else if((voltage<7.0)&(voltage!=0.0))
		{
			OLED_Huatu(104,0,3);//������
		}
	else if((voltage<7.2)&(voltage>=7.0))
		{
			OLED_Huatu(104,0,2);//����֮һ����
		}
	else if((voltage>=7.2)&(voltage<7.4))
		{
			OLED_Huatu(104,0,1);//����֮������
		}
	else if(voltage>=7.4)
		{
  		OLED_Huatu(104,0,0);//������
		}

  /* ˲ʱ */
  highlight=(menu->Screen.selection==0);
  OLED_China(16,48,13,highlight);//˲ʱ
	OLED_China(32,48,14,highlight);
		
		/* ƽ�� */
  highlight=(menu->Screen.selection==1);
  OLED_China(48,32,11,highlight);//ƽ��
	OLED_China(64,32,12,highlight);
		
			/* ���� */
  highlight=(menu->Screen.selection==2);
  OLED_China(80,48,15,highlight);//����
	OLED_China(96,48,16,highlight);

  /* Determine the next Menu */
  switch(menu->Screen.selection)
  {
  case 0:   /* Data Menu */
    menu->next=&DataMenu;
    break;
  case 1:   /* Communication Menu */
    menu->next=&AverageMenu;
    break;
  case 2:   /* Average Menu */
    menu->next=&CommMenu;
    break;
  default:
    menu->next=&DataMenu;
    break;
  }
	
	/*��������*/
	for(i=0;i<16;i++)
	{
		OLED_DrawPoint(100,i,0);
		OLED_DrawPoint(101,i,0);
		OLED_DrawPoint(102,i,0);
		OLED_DrawPoint(103,i,0);
	}
	
	OLED_Refresh_Gram();
}

  /*
     *ƽ���˵�
               **/
/**
  * @brief  Average Screen Button Handle Function.
  * @param  None
  * @retval None
  */
static void AverageScreen_ButtonHandler(void *Menu,int16_t button)
{
  DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
  
  switch(button)
  {
  case BUTTON_ESCAPE:
		if(menu->prev)
    {
      CurrentDisplay=menu->prev;
      CurrentDisplay->Screen.screen_number=0;
      CurrentDisplay->Screen.selection=0;
      OLED_Clear();  /* clear screen */
    }
    break;
  case BUTTON_ENTER:   /* ��ȡƽ������ */
    break;
  case BUTTON_LEFT:
    menu->Screen.selection++;
    break;
  case BUTTON_RIGHT:
    menu->Screen.selection--;
    break;
	case BUTTON_DATA:
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_SET);//485оƬ��������Ϊ�ߵ�ƽʱ����������
 	  HAL_UART_Transmit(&huart2,(uint8_t *)"<RDATA>",strlen("<RDATA>"),1000);
	  HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_RESET);//485оƬ��������Ϊ�͵�ƽʱ����������
	  break;
  default:
    break;
  }
  
  
  if(menu->Screen.selection!=0)
  {
    menu->Screen.selection=0;
  }
}

/**
  * @brief  Sverage Screen Display Function.
  * @param  None
  * @retval None
  */
static void AverageScreen_Display(void *Menu)
{
	DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
	
	if(menu->Screen.screen_number==0)
	{
	OLED_China(1,0,59,false);//��
	OLED_China(17,0,27,false);//��
	OLED_China(33,0,23,false);//��
	OLED_China(49,0,24,false);//��
	OLED_Char(65,0,':',false);
	OLED_ShowString(88,0,"///",false);
	OLED_China(112,0,62,false);//��

	
  OLED_China(1,16,59,false);//��
	OLED_China(17,16,27,false);//��
	OLED_China(33,16,23,false);//��
	OLED_China(49,16,25,false);//��
	OLED_Char(65,16,':',false);
	OLED_ShowString(80,16,"///m/s",false);
	
	
	OLED_China(1,32,60,false);//ʮ
	OLED_China(17,32,27,false);//��
	OLED_China(33,32,23,false);//��
	OLED_China(49,32,24,false);//��
	OLED_Char(65,32,':',false);
	OLED_ShowString(88,32,"///",false);
	OLED_China(112,32,62,false);//��
	
	OLED_China(1,48,60,false);//ʮ
	OLED_China(17,48,27,false);//��
	OLED_China(33,48,23,false);//��
	OLED_China(49,48,25,false);//��
	OLED_Char(65,48,':',false);
	OLED_ShowString(80,48,"///m/s",false);
	
	if((twodirect!=0)||(twospeed!=0)||(tendirect!=0)||(tenspeed!=0))
	{
		/*���ַ���*/
		snprintf(disp_buf,sizeof(disp_buf),"%3d",twodirect);
		OLED_ShowString(88,0,disp_buf,false);
		/*���ַ���*/
		snprintf(disp_buf,sizeof(disp_buf),"%4.1f",twospeed);
		OLED_ShowString(72,16,disp_buf,false);
		/*ʮ�ַ���*/
		snprintf(disp_buf,sizeof(disp_buf),"%3d",tendirect);
		OLED_ShowString(88,32,disp_buf,false);
		/*ʮ�ַ���*/
		snprintf(disp_buf,sizeof(disp_buf),"%4.1f",tenspeed);
		OLED_ShowString(72,48,disp_buf,false);
	}
	
		OLED_Refresh_Gram();//����
}
}

/*Hardware  Ӳ�����ò˵�*/
/**
  * @brief  Hardware Screen Button Handle Function.
  * @param  None
  * @retval None
  */
static void HardwareScreen_ButtonHandler(void *Menu,int16_t button)
{
  DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
  
  switch(button)
  {
  case BUTTON_ESCAPE:
		if(menu->prev)
    {
      CurrentDisplay=menu->prev;
      CurrentDisplay->Screen.screen_number=0;
      CurrentDisplay->Screen.selection=2;
      OLED_Clear();  /* clear screen */
    }
    break;
  case BUTTON_ENTER:   /* ����2��������������� */
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_SET);//485оƬ��������Ϊ�ߵ�ƽʱ����������
	  if(menu->Screen.selection==0)
		{
 	    HAL_UART_Transmit(&huart2,(uint8_t *)"<BEGIN>",strlen("<BEGIN>"),1000);
			HAL_UART_Transmit(&huart1,(uint8_t *)"<BEGIN>",strlen("<BEGIN>"),1000);
		}
		else if(menu->Screen.selection==1)
		{
 	    HAL_UART_Transmit(&huart2,(uint8_t *)"<CLOSE>",strlen("<CLOSE>"),1000);
			HAL_UART_Transmit(&huart1,(uint8_t *)"<CLOSE>",strlen("<CLOSE>"),1000);
		}
		else if(menu->Screen.selection==2)
		{
 	    HAL_UART_Transmit(&huart2,(uint8_t *)"<N1113>",strlen("<N1113>"),1000);
			HAL_UART_Transmit(&huart1,(uint8_t *)"<N1113>",strlen("<N1113>"),1000);
		}
		else if(menu->Screen.selection==3)
		{
 	    HAL_UART_Transmit(&huart2,(uint8_t *)"<QKONG>",strlen("<QKONG>"),1000);
			HAL_UART_Transmit(&huart1,(uint8_t *)"<QKONG>",strlen("<QKONG>"),1000);
		}
	  HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_RESET);//485оƬ��������Ϊ�͵�ƽʱ����������
		if((menu->Screen.selection==0)|(menu->Screen.selection==1))
		{
			menu->Screen.selection++;
		}
		
    break;
  case BUTTON_LEFT:
    menu->Screen.selection++;
    break;
  case BUTTON_RIGHT:
    menu->Screen.selection--;
    break;
	case BUTTON_DATA:
		CurrentDisplay=&DataMenu;
    CurrentDisplay->Screen.screen_number=0;
    CurrentDisplay->Screen.selection=0;
	  OLED_Clear();  /* clear screen */
	  break;
  default:
    break;
  }
  
  
  if(menu->Screen.selection<0)
  {
    menu->Screen.selection=COMMON_SCREEN_SELECTION_NUMBER-1;
  }
	else if(menu->Screen.selection>COMMON_SCREEN_SELECTION_NUMBER-1)
	{
		menu->Screen.selection=0;
	}
}

/**
  * @brief  Hardware Screen Display Function.
  * @param  None
  * @retval None
  */
static void HardwareScreen_Display(void *Menu)
{
  DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
  uint8_t highlight=false;

  highlight=(menu->Screen.selection==0);
	OLED_China(1,0,44,highlight);//��
	OLED_China(17,0,45,highlight);//ʼ
	OLED_China(33,0,42,highlight);//Ӳ
	OLED_China(49,0,46,highlight);//��
	OLED_China(65,0,47,highlight);//��
	OLED_China(81,0,48,highlight);//��
	
	highlight=(menu->Screen.selection==1);
	OLED_China(1,16,49,highlight);//��
	OLED_China(17,16,50,highlight);//��
	OLED_China(33,16,42,highlight);//Ӳ
	OLED_China(49,16,46,highlight);//��
	OLED_China(65,16,47,highlight);//��
	OLED_China(81,16,48,highlight);//��
	
	highlight=(menu->Screen.selection==2);
	OLED_China(1,32,51,highlight);//��
	OLED_China(17,32,52,highlight);//��
	OLED_China(33,32,53,highlight);//У
	OLED_China(49,32,51,highlight);//��
	OLED_ShowString(65,32," ***",false);
	
	highlight=(menu->Screen.selection==3);
	OLED_China(1,48,54,highlight);//��
	OLED_ShowString(17,48,"PAMS",highlight);
	OLED_China(49,48,35,highlight);//��
	OLED_China(65,48,36,highlight);//��
	OLED_China(81,48,37,highlight);//��
	OLED_ShowString(96,48," ***",false);
	
	OLED_Refresh_Gram();
}

/*PAMS menu�����洢�˵�*/
/**
  * @brief  PAMS Screen Button Handle Function.
  * @param  None
  * @retval None
  */
static void PamsScreen_ButtonHandler(void *Menu,int16_t button)
{
  DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
	int16_t selected_timevalue=0,min_value=0,max_value=0;
  int16_t *selected=NULL;
	
	switch(menu->Screen.screen_leftright)
	{
		case 0:
			break;
		case 1:
			if(menu->Screen.selection==0)
			{
			selected=&parameter.M1;
      selected_timevalue=parameter.M1;
      min_value=0;
      max_value=9;
			}
			else if(menu->Screen.selection==1)
			{
			selected=&parameter.M3;
      selected_timevalue=parameter.M3;
      min_value=0;
      max_value=9;
			}
			else if(menu->Screen.selection==2)
			{
			selected=&parameter.M5;
      selected_timevalue=parameter.M5;
      min_value=0;
      max_value=1;
			}
		  break;
		case 2:
			if(menu->Screen.selection==0)
			{
			selected=&parameter.M2;
      selected_timevalue=parameter.M2;
      min_value=0;
      max_value=9;
			}
			else if(menu->Screen.selection==1)
			{
			selected=&parameter.M4;
      selected_timevalue=parameter.M4;
      min_value=0;
      max_value=9;
			}
			break;
		default:
			break;
	}
	
	switch(button)
  {
  case BUTTON_ESCAPE:
		if(USART2_FLAG==1)
		{
			if(button1==0)
				{
					HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_SET);
					snprintf(disp_buf,sizeof(disp_buf),"<MANUA>");
					HAL_UART_Transmit(&huart2,(uint8_t *)disp_buf,7,1000);
					HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_RESET);
				}
			else if(button1==1)
				{
					HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_SET);
					snprintf(disp_buf,sizeof(disp_buf),"<AUTOM>");
					HAL_UART_Transmit(&huart2,(uint8_t *)disp_buf,7,1000);
					HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_RESET);
				}
		
			HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_SET);
			snprintf(disp_buf,sizeof(disp_buf),"<INT%02u>",space1);
			HAL_UART_Transmit(&huart2,(uint8_t *)disp_buf,7,1000);
			HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_RESET);
		}
		if(menu->prev)
    {
      CurrentDisplay=menu->prev;
      CurrentDisplay->Screen.screen_number=0;
      CurrentDisplay->Screen.selection=0;
      OLED_Clear();  /* clear screen */
    }
    break;
  case BUTTON_ENTER:
		if(USART2_FLAG==1)
		{
			menu->Screen.screen_leftright++;
		}
		else if((USART2_FLAG==0)&(menu->Screen.selection==0))
		{
			menu->Screen.screen_leftright++;
		}
	
    break;
  case BUTTON_LEFT:
		if(menu->Screen.screen_leftright==0)
		{
    menu->Screen.selection++;
		}
		else
		{
      selected_timevalue++;
		}
    break;
  case BUTTON_RIGHT:
		if(menu->Screen.screen_leftright==0)
		{
    menu->Screen.selection--;
		}
		else
		{
			selected_timevalue--;
		}
    break;
	case BUTTON_DATA:
		CurrentDisplay=&DataMenu;
    CurrentDisplay->Screen.screen_number=0;
    CurrentDisplay->Screen.selection=0;
	  OLED_Clear();  /* clear screen */
	  break;
  default:
    break;
  }
	
  parameter.M1=space/10;
	parameter.M2=space%10;
	parameter.M3=space1/10;
	parameter.M4=space1%10;
	parameter.M5=button1;
//	space  = parameter.M1*10+parameter.M2;
//	space1 = parameter.M3*10+parameter.M4;
//	button1=parameter.M5;
	
	if(menu->Screen.selection<0)
  {
    menu->Screen.selection=MAIN_SCREEN_SELECTION_NUMBER-1;
  }
	else if(menu->Screen.selection>MAIN_SCREEN_SELECTION_NUMBER-1)
	{
		menu->Screen.selection=0;
	}
	if(menu->Screen.selection!=2)
	{
		if(menu->Screen.screen_leftright<0)
			menu->Screen.screen_leftright=2;
		else if(menu->Screen.screen_leftright>2)
			menu->Screen.screen_leftright=0;
	}
	else if(menu->Screen.selection==2)
	{
		if(menu->Screen.screen_leftright<0)
			menu->Screen.screen_leftright=1;
		else if(menu->Screen.screen_leftright>1)
			menu->Screen.screen_leftright=0;
	}
	if(selected_timevalue<min_value)
  {
    selected_timevalue=max_value;
  }
  else if(selected_timevalue>max_value)
  {
    selected_timevalue=min_value;
  }
	/* set selected value */
  *selected=selected_timevalue;
}
/**
  * @brief  Communication Screen Display Function.
  * @param  None
  * @retval None
  */
static void PamsScreen_Display(void *Menu)
{
  DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
  uint8_t highlight=false;
  
	if(USART2_FLAG==0)
	{
		space  = parameter.M1*10+parameter.M2;
		
		OLED_ShowString(64,0,":    //M",false);
		OLED_ShowString(64,16,":    //M",false);
		OLED_ShowString(96,32,": //",false);
		
		highlight=((menu->Screen.screen_leftright==0)&(menu->Screen.selection==0));
		OLED_China(1,0,33,highlight);//��
	  OLED_China(17,0,34,highlight);//��
	  OLED_China(33,0,35,highlight);//��
	  OLED_China(49,0,36,highlight);//��
		
		highlight=((menu->Screen.screen_leftright==1)&(menu->Screen.selection==0));
		snprintf(disp_buf,sizeof(disp_buf),"%01u",parameter.M1);
	  OLED_ShowString(104,0,disp_buf,highlight);
		
		highlight=((menu->Screen.screen_leftright==2)&(menu->Screen.selection==0));
		snprintf(disp_buf,sizeof(disp_buf),"%01u",parameter.M2);
	  OLED_ShowString(112,0,disp_buf,highlight);
		
		highlight=(menu->Screen.selection==1);
		OLED_ShowString(1,16,"PAMS",highlight);
  	OLED_China(33,16,35,highlight);//��
  	OLED_China(49,16,36,highlight);//��
		
		highlight=(menu->Screen.selection==2);
		OLED_ShowString(1,32,"PAMS",highlight);
		OLED_China(33,32,3,highlight);//��
  	OLED_China(49,32,4,highlight);//��
	  OLED_China(65,32,38,highlight);//��
	  OLED_China(81,32,39,highlight);//��
	}
	
	else if(USART2_FLAG==1)
	{
		space  = parameter.M1*10+parameter.M2;
	  space1 = parameter.M3*10+parameter.M4;
	  button1=parameter.M5;
	
	OLED_Char(65,0,':',false);
	OLED_ShowString(120,0,"M",false);
	snprintf(disp_buf,sizeof(disp_buf),"%02u",space);
	//snprintf(disp_buf,sizeof(disp_buf),"%01u%01u",parameter.M1,parameter.M2);
	OLED_ShowString(104,0,disp_buf,false);
	
	OLED_Char(65,16,':',false);
	OLED_ShowString(120,16,"M",false);
	snprintf(disp_buf,sizeof(disp_buf),"%02u",space1);
	//snprintf(disp_buf,sizeof(disp_buf),"%01u%01u",parameter.M3,parameter.M4);
	OLED_ShowString(104,16,disp_buf,false);
	
	OLED_Char(97,32,':',false);
	
		highlight=((menu->Screen.screen_leftright==0)&(menu->Screen.selection==0));
		OLED_China(1,0,33,highlight);//��
	  OLED_China(17,0,34,highlight);//��
	  OLED_China(33,0,35,highlight);//��
	  OLED_China(49,0,36,highlight);//��
		
		highlight=((menu->Screen.screen_leftright==1)&(menu->Screen.selection==0));
		snprintf(disp_buf,sizeof(disp_buf),"%01u",parameter.M1);
	  OLED_ShowString(104,0,disp_buf,highlight);
		
		highlight=((menu->Screen.screen_leftright==2)&(menu->Screen.selection==0));
		snprintf(disp_buf,sizeof(disp_buf),"%01u",parameter.M2);
	  OLED_ShowString(112,0,disp_buf,highlight);
	

		highlight=((menu->Screen.screen_leftright==0)&(menu->Screen.selection==1));
		OLED_ShowString(1,16,"PAMS",highlight);
  	OLED_China(33,16,35,highlight);//��
  	OLED_China(49,16,36,highlight);//��
		
		highlight=((menu->Screen.screen_leftright==1)&(menu->Screen.selection==1));
		snprintf(disp_buf,sizeof(disp_buf),"%01u",parameter.M3);
	  OLED_ShowString(104,16,disp_buf,highlight);
		
		highlight=((menu->Screen.screen_leftright==2)&(menu->Screen.selection==1));
		snprintf(disp_buf,sizeof(disp_buf),"%01u",parameter.M4);
	  OLED_ShowString(112,16,disp_buf,highlight);

		highlight=((menu->Screen.screen_leftright==0)&(menu->Screen.selection==2));
		OLED_ShowString(1,32,"PAMS",highlight);
		OLED_China(33,32,3,highlight);//��
  	OLED_China(49,32,4,highlight);//��
	  OLED_China(65,32,38,highlight);//��
	  OLED_China(81,32,39,highlight);//��
		
		highlight=((menu->Screen.screen_leftright==1)&(menu->Screen.selection==2));
		if(parameter.M5==0)
	    OLED_China(112,32,65,highlight);//��
		else if(parameter.M5==1)
			OLED_China(112,32,44,highlight);//��
	}
	
	OLED_Refresh_Gram();
	
}

/*�����˵�*/
/**
  * @brief  Correct Screen Button Handle Function.
  * @param  None
  * @retval None
  */
static void CorrectScreen_ButtonHandler(void *Menu,int16_t button)
{
  DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
	int16_t selected_timevalue=0,min_value=0,max_value=0;
  int16_t *selected=NULL;
  
		switch(menu->Screen.screen_leftright)
		{
				case 0:
					break;
				case 1:
					if(menu->Screen.selection==0)
					{
							selected=&parameter.A1;
							selected_timevalue=parameter.A1;
							min_value=0;
							max_value=1;
					}
					else if(menu->Screen.selection==1)
					{
							selected=&parameter.A2;
							selected_timevalue=parameter.A2;
							min_value=0;
							max_value=1;
					}
					else if(menu->Screen.selection==2)
					{
							selected=&parameter.A3;
							selected_timevalue=parameter.A3;
							min_value=0;
							max_value=1;
					}
					else if(menu->Screen.selection==3)
					{
							selected=&parameter.A8;
							selected_timevalue=parameter.A8;
							min_value=0;
							max_value=1;
					}
					else if(menu->Screen.selection==4)
					{
							selected=&parameter.A4;
							selected_timevalue=parameter.A4;
							min_value=0;
							max_value=9;
					}
					else if(menu->Screen.selection==5)
					{
							selected=&parameter.A5;
							selected_timevalue=parameter.A5;
							min_value=0;
							max_value=1;
					}
					else if(menu->Screen.selection==6)
					{
							selected=&parameter.A6;
							selected_timevalue=parameter.A6;
							min_value=0;
							max_value=1;
					}
					else if(menu->Screen.selection==7)
					{
							selected=&parameter.A7;
							selected_timevalue=parameter.A7;
							min_value=0;
							max_value=9;
					}
					break;
				case 2:
					if(menu->Screen.selection==0)
					{
							selected=&parameter.B1;
							selected_timevalue=parameter.B1;
							min_value=0;
							max_value=9;
					}
					else if(menu->Screen.selection==1)
					{
							selected=&parameter.B2;
							selected_timevalue=parameter.B2;
							min_value=0;
							max_value=9;
					}
					else if(menu->Screen.selection==2)
					{
							selected=&parameter.B3;
							selected_timevalue=parameter.B3;
							min_value=0;
							max_value=9;
					}
					else if(menu->Screen.selection==3)
					{
							selected=&parameter.B8;
							selected_timevalue=parameter.B8;
							min_value=0;
							max_value=9;
					}
					else if(menu->Screen.selection==4)
					{
							selected=&parameter.B4;
							selected_timevalue=parameter.B4;
							min_value=0;
							max_value=9;
					}
					else if(menu->Screen.selection==5)
					{
							selected=&parameter.B5;
							selected_timevalue=parameter.B5;
							min_value=0;
							max_value=9;
					}
					else if(menu->Screen.selection==6)
					{
							selected=&parameter.B6;
							selected_timevalue=parameter.B6;
							min_value=0;
							max_value=9;
					}
					else if(menu->Screen.selection==7)
					{
							selected=&parameter.B7;
							selected_timevalue=parameter.B7;
							min_value=0;
							max_value=9;
					}
					break;
				case 3:
					if(menu->Screen.selection==0)
					{
							selected=&parameter.C1;
							selected_timevalue=parameter.C1;
							min_value=0;
							max_value=9;
					}
					else if(menu->Screen.selection==1)
					{
							selected=&parameter.C2;
							selected_timevalue=parameter.C2;
							min_value=0;
							max_value=9;
					}
					else if(menu->Screen.selection==2)
					{
							selected=&parameter.C3;
							selected_timevalue=parameter.C3;
							min_value=0;
							max_value=9;
					}
					else if(menu->Screen.selection==3)
					{
							selected=&parameter.C8;
							selected_timevalue=parameter.C8;
							min_value=0;
							max_value=9;
					}
					else if(menu->Screen.selection==4)
					{
							selected=&parameter.C4;
							selected_timevalue=parameter.C4;
							min_value=0;
							max_value=9;
					}
					else if(menu->Screen.selection==5)
					{
							selected=&parameter.C5;
							selected_timevalue=parameter.C5;
							min_value=0;
							max_value=9;
					}
					else if(menu->Screen.selection==6)
					{
							selected=&parameter.C6;
							selected_timevalue=parameter.C6;
							min_value=0;
							max_value=9;
					}
					else if(menu->Screen.selection==7)
					{
							selected=&parameter.C7;
							selected_timevalue=parameter.C7;
							min_value=0;
							max_value=9;
					}
					break;
				case 4:
					if(menu->Screen.selection==4)
					{
							selected=&parameter.D4;
							selected_timevalue=parameter.D4;
							min_value=0;
							max_value=9;
					}
					else if(menu->Screen.selection==6)
					{
							selected=&parameter.D6;
							selected_timevalue=parameter.D6;
							min_value=0;
							max_value=9;
					}
					break;
				case 5:
					if(menu->Screen.selection==4)
					{
							selected=&parameter.E4;
							selected_timevalue=parameter.E4;
							min_value=0;
							max_value=9;
					}
					break;
				default:
					break;
		}
	//��ѹ��������
	parameter.A4=Aircon/10000;
	parameter.B4=(Aircon%10000)/1000;
	parameter.C4=(Aircon%1000)/100;
	parameter.D4=(Aircon%100)/10;
	parameter.E4=Aircon%10;
	//���¶���
	parameter.A1=Tcorrect/100;
	parameter.B1=(Tcorrect%100)/10;
	parameter.C1=Tcorrect%10;
	//ʪ�ȶ���
	parameter.A2=Hcorrect/100;
	parameter.B2=(Hcorrect%100)/10;
	parameter.C2=Hcorrect%10;
	//���ٶ���
	parameter.A3=Wcorrect/100;
	parameter.B3=(Wcorrect%100)/10;
	parameter.C3=Wcorrect%10;
	//���¶���
	parameter.A8=Soil/100;
	parameter.B8=(Soil%100)/10;
	parameter.C8=Soil%10;
	//��ѹ����
	parameter.A5=Airpressure/100;
	parameter.B5=(Airpressure%100)/10;
	parameter.C5=Airpressure%10;
	//��������
	parameter.A6=Aconstant/1000;
	parameter.B6=(Aconstant%1000)/100;
	parameter.C6=(Aconstant%100)/10;
	parameter.D6=Aconstant%10;
//	Tcorrect=parameter.A1*100+parameter.B1*10+parameter.C1;
//	Hcorrect=parameter.A2*100+parameter.B2*10+parameter.C2;
//	Wcorrect=parameter.A3*100+parameter.B3*10+parameter.C3;
//	Aircon=parameter.A4*10000+parameter.B4*1000+parameter.C4*100+parameter.D4*10+parameter.E4;
//	Aconstant=parameter.A6*100+parameter.B6*10+parameter.C6;
	
	 switch(button)
  {
			case BUTTON_ESCAPE:
				USART2_FLAG = 0;
				if(menu->prev)
				{
					menu->Screen.screen_leftright = 0;
					USART2_FLAG = 0;
					CurrentDisplay=menu->prev;
					CurrentDisplay->Screen.screen_number=0;
					CurrentDisplay->Screen.selection=3;
					OLED_Clear();  /* clear screen */
				}
				break;
				
			case BUTTON_ENTER:
				if( USART2_FLAG == 1)
				{
						menu->Screen.screen_leftright++;
						if(menu->Screen.selection == 0)
						{
							/*���¶���*/
							if(menu->Screen.screen_leftright >3)
							{
								HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_SET);
								snprintf(disp_buf,sizeof(disp_buf),"<ZG%03u>",Tcorrect);
								HAL_UART_Transmit(&huart2,(uint8_t *)disp_buf,7,1000);
								HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_RESET);
								
								if(debug_usart)
								{
										printf("%s\r\n", disp_buf);
								}
							}
						}
						else if(menu->Screen.selection == 1)
						{
							/*ʪ�ȶ���*/
							if(menu->Screen.screen_leftright >3)
							{
								HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_SET);
								snprintf(disp_buf,sizeof(disp_buf),"<HG%03u>",Hcorrect);
								HAL_UART_Transmit(&huart2,(uint8_t *)disp_buf,7,1000);
								HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_RESET);
								
								if(debug_usart)
								{
										printf("%s\r\n", disp_buf);
								}
							}
						}
						else if(menu->Screen.selection == 2)
						{
							/*���ٶ���*/
							if(menu->Screen.screen_leftright >3)
							{
								HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_SET);
								snprintf(disp_buf,sizeof(disp_buf),"<XG%03u>",Wcorrect);
								HAL_UART_Transmit(&huart2,(uint8_t *)disp_buf,7,1000);
								HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_RESET);
								
								if(debug_usart)
								{
										printf("%s\r\n", disp_buf);
								}
							}
						}
						else if(menu->Screen.selection == 3)
						{
							/*���¶���*/
							if(menu->Screen.screen_leftright >3)
							{
								HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_SET);
								snprintf(disp_buf,sizeof(disp_buf),"<WG%01u%01u%01u>",parameter.A8 ,parameter.B8 ,parameter.C8);
								HAL_UART_Transmit(&huart2,(uint8_t *)disp_buf,7,1000);
								HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_RESET);
							}
						}
						else if(menu->Screen.selection == 4)
						{
							/*��ѹ������������ѹ�����ȶ���*/
							if(menu->Screen.screen_leftright >5)
							{
								HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_SET);
								snprintf(disp_buf,sizeof(disp_buf),"<%05u>",Aircon);
								HAL_UART_Transmit(&huart2,(uint8_t *)disp_buf,7,1000);
								HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_RESET);
								
								if(debug_usart)
								{
										printf("%s\r\n", disp_buf);
								}
							}
						}
						else if(menu->Screen.selection == 5)
						{
							/*��ѹ����*/
							if(menu->Screen.screen_leftright >3)
							{
								HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_SET);
								snprintf(disp_buf,sizeof(disp_buf),"<YG%01u%01u%01u>",parameter.A5 ,parameter.B5,parameter.C5);
								HAL_UART_Transmit(&huart2,(uint8_t *)disp_buf,7,1000);
								HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_RESET);
								
								if(debug_usart)
								{
										printf("%s\r\n", disp_buf);
								}
							}
						}
						else if(menu->Screen.selection == 6)
						{
							/*��������*/
							if(menu->Screen.screen_leftright >3)
							{
								HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_SET);
								if(parameter.A6 ==0)
								{
									snprintf(disp_buf,sizeof(disp_buf),"<L+%03u>",Aconstant);
								}
								else if(parameter.A6 ==1)
								{
									snprintf(disp_buf,sizeof(disp_buf),"<L-%03u>",(Aconstant- 1000));
								}
								HAL_UART_Transmit(&huart2,(uint8_t *)disp_buf,7,1000);
								HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_RESET);
								
								if(debug_usart)
								{
										printf("%s\r\n", disp_buf);
								}
							}
						}
						else if(menu->Screen.selection == 7)
						{
							/*�¶�ϵ������*/
							if(menu->Screen.screen_leftright >3)
							{
		//							HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_SET);
		//							snprintf(disp_buf,sizeof(disp_buf),"<V+0%01u%01u>",parameter.B7,parameter.C7);
		//							HAL_UART_Transmit(&huart2,(uint8_t *)disp_buf,7,1000);
		//							HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_RESET);
							}
						}
			}
				break;
			
			
			case BUTTON_LEFT:
			{
				if(menu->Screen.screen_leftright==0)
				{
					menu->Screen.selection++;
				}
				else
				{
					selected_timevalue++;
				}
			}
				break;
			
			case BUTTON_RIGHT:
				if(menu->Screen.screen_leftright==0)
				{
					menu->Screen.selection--;
//					if((menu->Screen.selection<0)|(menu->Screen.selection>4))
//					{
//						menu->Screen.selection=4;
//					}
				}
				else
				{
					selected_timevalue--;
				}
				break;
			case BUTTON_DATA:
				CurrentDisplay=&DataMenu;
				CurrentDisplay->Screen.screen_number=0;
				CurrentDisplay->Screen.selection=0;
				OLED_Clear();  /* clear screen */
			default:
				break;
  }
	

  if(menu->Screen.selection<0)
  {
    menu->Screen.selection=7;				//selection������һ������
  }
  else if(menu->Screen.selection>7)
  {
    menu->Screen.selection=0;
  }
  if(menu->Screen.selection==4)
	{
		if(menu->Screen.screen_leftright<0)
			menu->Screen.screen_leftright=5;
		else if(menu->Screen.screen_leftright>5)			//screen_leftright��ÿһ�еĹ��λ�ã��еĶ� �е���
			menu->Screen.screen_leftright=0;
	}
	else if(menu->Screen.selection==6)
	{
		if(menu->Screen.screen_leftright<0)
			menu->Screen.screen_leftright=4;
		else if(menu->Screen.screen_leftright>4)
			menu->Screen.screen_leftright=0;
	}
	else if(menu->Screen.selection!=4)
	{
		if(menu->Screen.screen_leftright<0)
			menu->Screen.screen_leftright=3;
		else if(menu->Screen.screen_leftright>3)
			menu->Screen.screen_leftright=0;
	}
	
	if(selected_timevalue<min_value)
  {
    selected_timevalue=max_value;
  }
  else if(selected_timevalue>max_value)
  {
    selected_timevalue=min_value;
  }
	
  /* set selected value */
  *selected=selected_timevalue;
}
/**
  * @brief  Correct Screen Button Handle Function.
  * @param  None
  * @retval None
  */
void CorrectScreen_Display(void *Menu)
{
  DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
	uint8_t highlight=false;

  if(menu->Screen.selection<4)
	{
		menu->Screen.screen_number=0;
	}
	else if(menu->Screen.selection>3)
	{
		menu->Screen.screen_number=1;
		
	}

	
	 if(menu->Screen.screen_number==0)
  {
		if(USART2_FLAG == 1)
		{
			Tcorrect=parameter.A1*100+parameter.B1*10+parameter.C1;//���¶���
	    Hcorrect=parameter.A2*100+parameter.B2*10+parameter.C2;//ʪ�ȶ���
	    Wcorrect=parameter.A3*100+parameter.B3*10+parameter.C3;//���ٶ���
			Soil    =parameter.A8*100+parameter.B8*10+parameter.C8;//���¶���
		/* ���¶��� */
		//printf("���¶�����%03u\r\n",Tcorrect);
			if(Tcorrect<100)
			{
				OLED_Char(80,0,'+',highlight);
				snprintf(disp_buf,sizeof(disp_buf),"%01u.%01u",Tcorrect/10,Tcorrect%10);
				OLED_ShowString(88,0,disp_buf,false);
			}
			else 
			{
				OLED_Char(80,0,'-',highlight);
			  snprintf(disp_buf,sizeof(disp_buf),"%01u.%01u",(Tcorrect-100)/10,(Tcorrect-100)%10);
				OLED_ShowString(88,0,disp_buf,false);
			}
		OLED_ShowString(64,0,": ",false);
		//OLED_Char(96,0,'.',false);
		highlight=((menu->Screen.selection==0)&(menu->Screen.screen_leftright==0));
	  OLED_China(1,0,17,highlight);//���¶���
	  OLED_China(17,0,18,highlight);
	  OLED_China(33,0,64,highlight);
	  OLED_China(49,0,51,highlight);
	  OLED_China(112,0,61,false);//��
		
		highlight=((menu->Screen.selection==0)&(menu->Screen.screen_leftright==1));
		if(parameter.A1==0)
			OLED_Char(80,0,'+',highlight);
		else if(parameter.A1==1)
			OLED_Char(80,0,'-',highlight);
		
		highlight=((menu->Screen.selection==0)&(menu->Screen.screen_leftright==2));
		//snprintf(disp_buf,sizeof(disp_buf),"%01u",(Tcorrect%100)/10);
		snprintf(disp_buf,sizeof(disp_buf),"%01u",parameter.B1);
		OLED_ShowString(88,0,disp_buf,highlight);
		
		highlight=((menu->Screen.selection==0)&(menu->Screen.screen_leftright==3));
		//snprintf(disp_buf,sizeof(disp_buf),"%01u",Tcorrect%10);
		snprintf(disp_buf,sizeof(disp_buf),"%01u",parameter.C1);
		OLED_ShowString(104,0,disp_buf,highlight);
		
		/* ʪ�ȶ��� */
	  if(Hcorrect<100)
			{
				OLED_Char(80,16,'+',highlight);
				snprintf(disp_buf,sizeof(disp_buf),"%02u",Hcorrect);
				OLED_ShowString(88,16,disp_buf,false);
			}
			else 
			{
				OLED_Char(80,16,'-',highlight);
			  snprintf(disp_buf,sizeof(disp_buf),"%02u",Hcorrect);
				OLED_ShowString(88,16,disp_buf,false);
			}
		OLED_ShowString(64,16,": ",false);
		highlight=((menu->Screen.selection==1)&(menu->Screen.screen_leftright==0));	
	  OLED_China(1,16,19,highlight);//ʪ�ȶ���
	  OLED_China(17,16,20,highlight);
	  OLED_China(33,16,64,highlight);
	  OLED_China(49,16,51,highlight);
		OLED_ShowString(104,16,"%RH",false);
		
		highlight=((menu->Screen.selection==1)&(menu->Screen.screen_leftright==1));	
		if(parameter.A2==0)
			OLED_Char(80,16,'+',highlight);
		else if(parameter.A2==1)
			OLED_Char(80,16,'-',highlight);
		
		highlight=((menu->Screen.selection==1)&(menu->Screen.screen_leftright==2));
		//snprintf(disp_buf,sizeof(disp_buf),"%01u",(Hcorrect%100)/10);
		snprintf(disp_buf,sizeof(disp_buf),"%01u",parameter.B2);
		OLED_ShowString(88,16,disp_buf,highlight);
		
		highlight=((menu->Screen.selection==1)&(menu->Screen.screen_leftright==3));
		//snprintf(disp_buf,sizeof(disp_buf),"%01u",Hcorrect%10);
		snprintf(disp_buf,sizeof(disp_buf),"%01u",parameter.C2);
		OLED_ShowString(96,16,disp_buf,highlight);
		
		/* ���ٶ��� */
		if(Wcorrect<100)
			{
				OLED_Char(72,32,'+',highlight);
				snprintf(disp_buf,sizeof(disp_buf),"%01u.%01u",Wcorrect/10,Wcorrect%10);
				OLED_ShowString(80,32,disp_buf,false);
			}
			else 
			{
				OLED_Char(72,32,'-',highlight);
			  snprintf(disp_buf,sizeof(disp_buf),"%01u.%01u",(Wcorrect-100)/10,(Wcorrect-100)%10);
				OLED_ShowString(80,32,disp_buf,false);
			}
		OLED_Char(64,32,':',false);
		//OLED_Char(88,32,'.',false);
		highlight=((menu->Screen.selection==2)&(menu->Screen.screen_leftright==0));		
	  OLED_China(1,32,23,highlight);//���ٶ���
	  OLED_China(17,32,25,highlight);
	  OLED_China(33,32,64,highlight);
	  OLED_China(49,32,51,highlight);
		OLED_ShowString(104,32,"m/s",false);
		
		highlight=((menu->Screen.selection==2)&(menu->Screen.screen_leftright==1));	
		if(parameter.A3==0)
			OLED_Char(72,32,'+',highlight);
		else if(parameter.A3==1)
			OLED_Char(72,32,'-',highlight);
		
		highlight=((menu->Screen.selection==2)&(menu->Screen.screen_leftright==2));
		//snprintf(disp_buf,sizeof(disp_buf),"%01u",(Wcorrect%100)/10);
		snprintf(disp_buf,sizeof(disp_buf),"%01u",parameter.B3);
		OLED_ShowString(80,32,disp_buf,highlight);
		
		highlight=((menu->Screen.selection==2)&(menu->Screen.screen_leftright==3));
		//snprintf(disp_buf,sizeof(disp_buf),"%01u",Wcorrect%10);
		snprintf(disp_buf,sizeof(disp_buf),"%01u",parameter.C3);
		OLED_ShowString(96,32,disp_buf,highlight);
		
		/* ���¶��� */
		OLED_ShowString(64,48,": ",false);
		OLED_Char(96,48,'.',false);
		highlight=((menu->Screen.selection==3)&(menu->Screen.screen_leftright==0));
	  OLED_China(1,48,26,highlight);//���¶���
	  OLED_China(17,48,18,highlight);
	  OLED_China(33,48,64,highlight);
	  OLED_China(49,48,51,highlight);
		OLED_China(112,48,61,false);//��
		
		highlight=((menu->Screen.selection==3)&(menu->Screen.screen_leftright==1));
		if(parameter.A8==0)
		{
				OLED_Char(80,48,'+',highlight);
		}
		else if(parameter.A8==1)
		{
				OLED_Char(80,48,'-',highlight);
		}
		
		highlight=((menu->Screen.selection==3)&(menu->Screen.screen_leftright==2));
		snprintf(disp_buf,sizeof(disp_buf),"%01u",parameter.B8);
		OLED_ShowString(88,48,disp_buf,highlight);
		
		highlight=((menu->Screen.selection==3)&(menu->Screen.screen_leftright==3));
		snprintf(disp_buf,sizeof(disp_buf),"%01u",parameter.C8);
		OLED_ShowString(104,48,disp_buf,highlight);
	}
	else if(USART2_FLAG == 0)
		{
			highlight=(menu->Screen.selection==0);
			OLED_China(1,0,17,highlight);//���¶���
			OLED_China(17,0,18,highlight);
			OLED_China(33,0,64,highlight);
			OLED_China(49,0,51,highlight);
			OLED_China(112,0,61,false);//��
			OLED_ShowString(64,0,": */./",false);
			
			highlight=(menu->Screen.selection==1);	
			OLED_China(1,16,19,highlight);//ʪ�ȶ���
			OLED_China(17,16,20,highlight);
			OLED_China(33,16,64,highlight);
			OLED_China(49,16,51,highlight);
			OLED_ShowString(64,16,": *//%RH",false);
			
			highlight=(menu->Screen.selection==2);		
			OLED_China(1,32,23,highlight);//���ٶ���
			OLED_China(17,32,25,highlight);
			OLED_China(33,32,64,highlight);
			OLED_China(49,32,51,highlight);
			OLED_ShowString(64,32,":*/./m/s",false);
			
			highlight=(menu->Screen.selection==3);
			OLED_China(1,48,26,highlight);//���¶���
			OLED_China(17,48,18,highlight);
			OLED_China(33,48,64,highlight);
			OLED_China(49,48,51,highlight);
			OLED_China(112,48,61,false);//��
			OLED_ShowString(64,48,": */./",false);
		}
  }
	else if(menu->Screen.screen_number==1)
	{
		if(USART2_FLAG == 1)
		{
				Aconstant=parameter.A6*1000+parameter.B6*100+parameter.C6*10+parameter.D6;//��������
				Aircon=parameter.A4*10000+parameter.B4*1000+parameter.C4*100+parameter.D4*10+parameter.E4;//��ѹ��������
				Airpressure=parameter.A5*100 +parameter.B5*10 +parameter.C5;
			
				//snprintf(disp_buf,sizeof(disp_buf),"%02u.%03u",Aircon/1000,Aircon%1000);
				//OLED_ShowString(80,0,disp_buf,false);
				OLED_ShowString(64,0,": ",false);
				OLED_Char(96,0,'.',false);
				highlight=((menu->Screen.selection==4)&(menu->Screen.screen_leftright==0));
				OLED_China(1,0,17,highlight);//��ѹ����
				OLED_China(17,0,21,highlight);
				OLED_China(33,0,63,highlight);
				OLED_China(49,0,58,highlight);
				
				highlight=((menu->Screen.selection==4)&(menu->Screen.screen_leftright==1));
				//snprintf(disp_buf,sizeof(disp_buf),"%01u",Aircon/10000);
				snprintf(disp_buf,sizeof(disp_buf),"%01u",parameter.A4);
				OLED_ShowString(80,0,disp_buf,highlight);
				
				highlight=((menu->Screen.selection==4)&(menu->Screen.screen_leftright==2));
				//snprintf(disp_buf,sizeof(disp_buf),"%01u",(Aircon%10000)/1000);
				snprintf(disp_buf,sizeof(disp_buf),"%01u",parameter.B4);
				OLED_ShowString(88,0,disp_buf,highlight);
				
				highlight=((menu->Screen.selection==4)&(menu->Screen.screen_leftright==3));
				//snprintf(disp_buf,sizeof(disp_buf),"%01u",(Aircon%1000)/100);
				snprintf(disp_buf,sizeof(disp_buf),"%01u",parameter.C4);
				OLED_ShowString(104,0,disp_buf,highlight);
				
				highlight=((menu->Screen.selection==4)&(menu->Screen.screen_leftright==4));
				//snprintf(disp_buf,sizeof(disp_buf),"%01u",(Aircon%100)/10);
				snprintf(disp_buf,sizeof(disp_buf),"%01u",parameter.D4);
				OLED_ShowString(112,0,disp_buf,highlight);
				
				highlight=((menu->Screen.selection==4)&(menu->Screen.screen_leftright==5));
				//snprintf(disp_buf,sizeof(disp_buf),"%01u",Aircon%10);
				snprintf(disp_buf,sizeof(disp_buf),"%01u",parameter.E4);
				OLED_ShowString(120,0,disp_buf,highlight);
				
				/* ��ѹ���� */	
				OLED_Char(64,16,':',false);
				OLED_ShowString(104,16,"hpa",false);
				OLED_Char(88,16,'.',false);
				highlight=((menu->Screen.selection==5)&(menu->Screen.screen_leftright==0));
				OLED_China(1,16,17,highlight);//��ѹ����
				OLED_China(17,16,21,highlight);
				OLED_China(33,16,64,highlight);
				OLED_China(49,16,51,highlight);
				
				highlight=((menu->Screen.selection==5)&(menu->Screen.screen_leftright==1));
				if(parameter.A5==0)
					OLED_Char(72,16,'+',highlight);
				else if(parameter.A5==1)
					OLED_Char(72,16,'-',highlight);
				
				highlight=((menu->Screen.selection==5)&(menu->Screen.screen_leftright==2));
				snprintf(disp_buf,sizeof(disp_buf),"%01u",parameter.B5);
				OLED_ShowString(80,16,disp_buf,highlight);
				
				highlight=((menu->Screen.selection==5)&(menu->Screen.screen_leftright==3));
				snprintf(disp_buf,sizeof(disp_buf),"%01u",parameter.C5);
				OLED_ShowString(96,16,disp_buf,highlight);
				
				/* �������� */
				snprintf(disp_buf,sizeof(disp_buf),"%02u.%01u", (Aconstant%1000)/10,Aconstant%10);
				OLED_ShowString(72,32,disp_buf,false);
				//OLED_Char(64,32,':',false);
				//OLED_Char(88,32,'.',false);
				OLED_ShowString(104,32,"hpa",false);
				highlight=((menu->Screen.selection==6)&(menu->Screen.screen_leftright==0));
				OLED_China(1,32,63,highlight);//��������
				OLED_China(17,32,58,highlight);
				OLED_China(33,32,64,highlight);
				OLED_China(49,32,51,highlight);
				
				highlight=((menu->Screen.selection==6)&(menu->Screen.screen_leftright==1));
				if(parameter.A6==0)
					OLED_Char(64,32,'+',highlight);
				else if(parameter.A6==1)
					OLED_Char(64,32,'-',highlight);
				
				highlight=((menu->Screen.selection==6)&(menu->Screen.screen_leftright==2));
				//snprintf(disp_buf,sizeof(disp_buf),"%01u",Aconstant/100);
				snprintf(disp_buf,sizeof(disp_buf),"%01u",parameter.B6);
				OLED_ShowString(72,32,disp_buf,highlight);
				
				highlight=((menu->Screen.selection==6)&(menu->Screen.screen_leftright==3));
				//snprintf(disp_buf,sizeof(disp_buf),"%01u",(Aconstant%100)/10);
				snprintf(disp_buf,sizeof(disp_buf),"%01u",parameter.C6);
				OLED_ShowString(80,32,disp_buf,highlight);
				
				highlight=((menu->Screen.selection==6)&(menu->Screen.screen_leftright==4));
				//snprintf(disp_buf,sizeof(disp_buf),"%01u",Aconstant%10);
				snprintf(disp_buf,sizeof(disp_buf),"%01u",parameter.D6);
				OLED_ShowString(96,32,disp_buf,highlight);
				
				/* �¶�ϵ������ */
				OLED_Char(64,48,':',false);
				OLED_Char(80,48,'.',false);
				OLED_ShowString(104,48,"hpa",false);
				highlight=((menu->Screen.selection==7)&(menu->Screen.screen_leftright==0));
				OLED_China(1,48,18,highlight);//�¶�ϵ��
				OLED_China(17,48,20,highlight);
				OLED_China(33,48,55,highlight);
				OLED_China(49,48,58,highlight);
				
				highlight=((menu->Screen.selection==7)&(menu->Screen.screen_leftright==1));
				snprintf(disp_buf,sizeof(disp_buf),"%01u",parameter.A7);
				OLED_ShowString(72,48,disp_buf,highlight);
				
				highlight=((menu->Screen.selection==7)&(menu->Screen.screen_leftright==2));
				snprintf(disp_buf,sizeof(disp_buf),"%01u",parameter.B7);
				OLED_ShowString(88,48,disp_buf,highlight);
				
				highlight=((menu->Screen.selection==7)&(menu->Screen.screen_leftright==3));
				snprintf(disp_buf,sizeof(disp_buf),"%01u",parameter.C7);
				OLED_ShowString(96,48,disp_buf,highlight);
	 
		}
		else if(USART2_FLAG == 0)
		{
			highlight=(menu->Screen.selection==4);
			OLED_China(1,0,17,highlight);//��ѹ����
			OLED_China(17,0,21,highlight);
			OLED_China(33,0,63,highlight);
			OLED_China(49,0,58,highlight);
			OLED_ShowString(64,0,": //.///",false);
			
			highlight=(menu->Screen.selection==5);
			OLED_China(1,16,17,highlight);//��ѹ����
			OLED_China(17,16,21,highlight);
			OLED_China(33,16,64,highlight);
			OLED_China(49,16,51,highlight);
			OLED_ShowString(64,16,":*/./hpa",false);
			
			highlight=(menu->Screen.selection==6);
			OLED_China(1,32,63,highlight);//��������
			OLED_China(17,32,58,highlight);
			OLED_China(33,32,64,highlight);
			OLED_China(49,32,51,highlight);
			OLED_ShowString(64,32,"*//./hpa",false);
			
			highlight=(menu->Screen.selection==7);
			OLED_China(1,48,18,highlight);//�¶�ϵ��
			OLED_China(17,48,20,highlight);
			OLED_China(33,48,55,highlight);
			OLED_China(49,48,58,highlight);
			OLED_ShowString(64,48,":/.//hpa",false);
		}
	}
	OLED_Refresh_Gram();
}

	

/**
  * @brief  shrot delay to eliminate button jitter.
  * @param  None
  * @retval None
  */
__STATIC_INLINE void short_delay(void)
{
  volatile uint32_t i=0;
	
	for(i=0;i<0x1FFFF;i++);
  
//  while(i--)
//  {
//    asm("nop");
//  }
}

/**
  * @brief  turn off display
  * @param  None
  * @retval None
  */
//__STATIC_INLINE void turn_off_display(void)
//{
//  /* turn off display */
//  disp_on=false;
//  OLED_Clear();  /* clear screen */
//  
//  CurrentDisplay=&MainMenu;  /* display main menu */
//  CurrentDisplay->Screen.screen_number=0;
//  CurrentDisplay->Screen.selection=0;
//}

/**
  * @brief  turn on display
  * @param  None
  * @retval None
  */
//__STATIC_INLINE void turn_on_display(void)
//{
//  /* turn on display */
//  disp_on=true;  /* display on */
//  disp_on_count=MAX_DISPLAY_ON_TIME;
//}
