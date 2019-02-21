/* Includes ------------------------------------------------------------------*/
#include "storage_module.h"
#include "cmsis_os.h"

#include "sys_time_module.h"
#include "display_module.h"
#include "usart.h"
#include "gpio.h"
#include "at24c512.h"
#include "at_iic.h"
#include "lcd.h"
#include "string.h"
#include "stdlib.h"
#include "eeprom.h"

/* Private define ------------------------------------------------------------*/
#define UART_RX_BUF_SIZE  (512) 	
#define storagePRIORITY     osPriorityNormal
#define storageSTACK_SIZE   configMINIMAL_STACK_SIZE

/* RTC Timeͨ����������ʱ��*/
static RTC_TimeTypeDef Usart_Time;
static RTC_DateTypeDef Usart_Date;

static char *p1;
static char rx1_buffer[UART_RX_BUF_SIZE]={0};  /* USART1 receiving buffer */
static char rx2_buffer[UART_RX_BUF_SIZE]={0};  /*USART2 receiving buffer */
static uint32_t rx1_count=0;     /* receiving counter */
static uint32_t rx2_count=0;
static uint8_t cr1_begin=false;        /* '\r'  received */ 
static uint8_t cr2_begin=false;        /* '\r'  received */ 
static uint8_t rx1_cplt=false;   /* received a frame of data ending with '\r'and'\n' */
static uint8_t rx2_cplt=false;   /* received a frame of data ending with '\r'and'\n' */
static uint32_t n=0;//�����ݴ��EEPROM�Ĵ���
static uint8_t read_buf[63];   //��EEPROM�����ݴ洢�Ļ���
static char Tbuf[13];
uint8_t USART2_FLAG=0;//����2���ӱ�־
uint8_t DATA_FLAG = false;//����2�����ݱ�־   false:û������    true:�����ݱ�־
uint8_t space=1;//��ʷ���ݴ洢���ʱ��   ��λ ����  Ĭ��Ϊ1���Ӵ洢һ������
uint32_t humi,windirect,twodirect,tendirect;
float voltage,temperature,airpressure,wspeed,twospeed,tenspeed;//��ѹ
int32_t button1,space1,Tcorrect,Hcorrect,Wcorrect,Airpressure,Aconstant,Soil;
uint32_t Aircon,zhanhao;

static int  group;
 uint32_t debug_usart = 0;

char data_flag = 0;		/*0û����������1��������*/
/* os relative */
static osThreadId Storage1ThreadHandle;
static osThreadId Storage2ThreadHandle;
static osSemaphoreId semaphore_usart1;
static osSemaphoreId semaphore_usart2;
static void Storage1_Thread(void const *argument);
static void Storage2_Thread(void const *argument);

/**
  * @brief  Init Storage Module. 
  * @retval 0:success;-1:failed
  */
int32_t init_storage_module(void)
{
	
	USART1_UART_Init(9600);
	USART2_UART_Init(9600);
	
	printf("Hello World!\r\n");
	
	OLED_Init();
	/*�����˵�*/
	OLED_OpenMenu();
	
	zhanhao=001;
	/* Define used semaphore ��������1���ź���*/
  osSemaphoreDef(SEM_USART1);
  /* Create the semaphore used by the two threads */
  semaphore_usart1=osSemaphoreCreate(osSemaphore(SEM_USART1), 1);
  if(semaphore_usart1 == NULL)
  {
    printf("Create Semaphore_USART1 failed!\r\n");
    return -1;
  }
	
	/* Define used semaphore ��������2���ź���*/
  osSemaphoreDef(SEM_USART2);
  /* Create the semaphore used by the two threads */
  semaphore_usart2=osSemaphoreCreate(osSemaphore(SEM_USART2), 1);
  if(semaphore_usart2 == NULL)
  {
    printf("Create Semaphore_USART2 failed!\r\n");
    return -1;
  }
	
	
 
	
	/* Create a thread to read historical data��������1����洢�������� */
  osThreadDef(Storage1, Storage1_Thread, storagePRIORITY, 0, storageSTACK_SIZE);
  Storage1ThreadHandle=osThreadCreate(osThread(Storage1), NULL);
  if(Storage1ThreadHandle == NULL)
  {
    printf("Create Storage1 Thread failed!\r\n");
    return -1;
  }
	
	/* Create a thread to read historical data��������2����洢�������� */
  osThreadDef(Storage2, Storage2_Thread, osPriorityNormal, 0, storageSTACK_SIZE);
  Storage2ThreadHandle=osThreadCreate(osThread(Storage2), NULL);
  if(Storage2ThreadHandle == NULL)
  {
    printf("Create Storage2 Thread failed!\r\n");
    return -1;
  }
  
  
  return 0;
	
	
}


/*����2������������*/
static void Storage2_Thread(void const *argument)
{
	uint8_t second,minute;
	uint8_t i=0;
	char buf[70];
	char flag_l = 0;
	
	parameter.M1=space/10;
	parameter.M2=space%10;	
	/* Init Display Module. 
   */
  if(init_display_module()<0)
  {
    printf("init display module failed!\r\n");
  }
  else
  {
    printf("init display module ok!\r\n");
  }
	
	if(init_sys_time_module()<0)
  {
    printf("init system time module failed!\r\n");
  }
  else
  {
    printf("init system time module ok!\r\n");
  }
	while(1)
	{
		if(osSemaphoreWait(semaphore_usart2,osWaitForever)==osOK)
    {
			//printf("����2���ݣ�%s\r\n",rx2_buffer);
			//printf("USART2:%d-%d-%d-%d-%d-%d-%d-%d-%d-%d\r\n",rx2_buffer[0],rx2_buffer[1],rx2_buffer[2],rx2_buffer[3],rx2_buffer[4],rx2_buffer[5],rx2_buffer[6],rx2_buffer[7],rx2_buffer[8],rx2_buffer[9]);
			if(strlen(rx2_buffer)==69)
			{
					//HAL_UART_Transmit(&huart1,(uint8_t *)rx2_buffer,sizeof(rx2_buffer),1000);
					//HAL_UART_Transmit(&huart1,(uint8_t *)"\r\n",2,1000);
					DATA_FLAG =true;
					DATA_Number = 0;//���㷢�Ͷ�ȡ�����������
				
					data_flag = 1;
				
					strncpy(buf, rx2_buffer, 69);
					if(debug_usart)
					{
							printf("buf=%s\r\n", buf);
					}
					
					p1 = strtok(buf, ",");
					
					
					temperature = atof(strtok(NULL, ","));
					
					
					humi = atoi(strtok(NULL, ","));
					
					
					airpressure = atof(strtok(NULL, ","));
					
					
					windirect = atoi(strtok(NULL, ","));
					
					
					wspeed = atof(strtok(NULL, ","));
					
					
					twodirect = atoi(strtok(NULL, ","));
					
					
					twospeed = atof(strtok(NULL, ","));
					
					
					tendirect = atoi(strtok(NULL, ","));
					
					
					tenspeed = atof(strtok(NULL, ","));
					
					strtok(NULL, ",");
					//printf("%s ", strtok(NULL, ","));
					
					voltage = atof(strtok(NULL, ","));
					
					
					if(debug_usart)
					{
							printf("%s ", p1);
							printf("temperature=%f ", temperature);
							printf("humi=%d ", humi);
							printf("airpressure=%f ", airpressure);
							printf("windirect=%d ", windirect);
							printf("wspeed=%f ", wspeed);
							printf("twodirect=%d ", twodirect);
							printf("twospeed=%f ", twospeed);
							printf("tendirect=%d ", tendirect);
							printf("tenspeed=%f ", tenspeed);
							printf("voltage=%f\r\n", voltage);
					}
					
					memset(buf, 0, 70);
				}
			else if(strlen(rx2_buffer)==29)
			{
				if(debug_usart)
				{
					printf("����������%s\r\n",rx2_buffer);
				}
			  USART2_FLAG = 1;//����2�Ѿ����ӱ�־
				//printf("USART2:%d-%d-%d-%d-%d-%d-%d-%d-%d-%d\r\n",rx2_buffer[0],rx2_buffer[1],rx2_buffer[2],rx2_buffer[3],rx2_buffer[4],rx2_buffer[5],rx2_buffer[6],rx2_buffer[7],rx2_buffer[8],rx2_buffer[9]);
				for(i=0;i<27;i++)
				{
					if((rx2_buffer[i+1] != 43) && ((rx2_buffer[i+1] > 57) || (rx2_buffer[i+1] < 48)) && (rx2_buffer[i+1] != 45))
					{
						flag_l = 1;
						//printf("������\r\n");
					}
				}
				if(flag_l == 0)
				{
						button1=rx2_buffer[1]-48;
						
						
						space1=(rx2_buffer[2]-48)*10+rx2_buffer[3]-48;
						
						
						/*����*/
						if(rx2_buffer[4]=='+')
						{
							Tcorrect=(rx2_buffer[5]-48)*10+rx2_buffer[6]-48;
						}
						else if(rx2_buffer[4]=='-')
						{
							Tcorrect=100+(rx2_buffer[5]-48)*10+rx2_buffer[6]-48;
						}
						else 
						{
							Tcorrect=(rx2_buffer[4]-48)*100+(rx2_buffer[5]-48)*10+rx2_buffer[6]-48;
						}
						
						//���¶���
						parameter.A1=Tcorrect/100;
						parameter.B1=(Tcorrect%100)/10;
						parameter.C1=Tcorrect%10;
						
						/*ʪ��*/
						if(rx2_buffer[7]=='+')
						{
							Hcorrect=(rx2_buffer[8]-48)*10+rx2_buffer[9]-48;
						}
						else if(rx2_buffer[7]=='-')
						{
							Hcorrect=100+(rx2_buffer[8]-48)*10+rx2_buffer[9]-48;
						}
						else 
						{
							Hcorrect = (rx2_buffer[7]-48)*100+(rx2_buffer[8]-48)*10 + (rx2_buffer[9]-48);
						}
						
						
						//ʪ�ȶ���
						parameter.A2=Hcorrect/100;
						parameter.B2=(Hcorrect%100)/10;
						parameter.C2=Hcorrect%10;
						
						/*����*/
						if(rx2_buffer[10]=='+')
						{
							Wcorrect=(rx2_buffer[11]-48)*10+rx2_buffer[12]-48;
						}
						else if(rx2_buffer[10]=='-')
						{
							Wcorrect=100+(rx2_buffer[11]-48)*10+rx2_buffer[12]-48;
						}
						else 
						{
							Wcorrect=(rx2_buffer[10]-48)*100+(rx2_buffer[11]-48)*10+rx2_buffer[12]-48;
						}
						
						//���ٶ���
						parameter.A3=Wcorrect/100;
						parameter.B3=(Wcorrect%100)/10;
						parameter.C3=Wcorrect%10;
						/*��ѹ*/
						if(rx2_buffer[13]=='+')
						{
							Airpressure=(rx2_buffer[14]-48)*10+rx2_buffer[15]-48;
						}
						else if(rx2_buffer[13]=='-')
						{
							Airpressure=100+(rx2_buffer[14]-48)*10+rx2_buffer[15]-48;
						}
						else 
						{
							Airpressure=(rx2_buffer[13]-48)*100+(rx2_buffer[14]-48)*10+rx2_buffer[15]-48;
						}
						
						//��ѹ����

						parameter.A5=Airpressure/100;
						parameter.B5=(Airpressure%100)/10;
						parameter.C5=Airpressure%10;
						
						Aircon=(rx2_buffer[19]-48)*10000+(rx2_buffer[20]-48)*1000+(rx2_buffer[21]-48)*100+(rx2_buffer[22]-48)*10+(rx2_buffer[23]-48);
						
						//��ѹ��������
						parameter.A4=Aircon/10000;
						parameter.B4=(Aircon%10000)/1000;
						parameter.C4=(Aircon%1000)/100;
						parameter.D4=(Aircon%100)/10;
						parameter.E4=Aircon%10;
						
						
						//��������
						if(rx2_buffer[24]=='+')
						{
							Aconstant=(rx2_buffer[25]-48)*100+(rx2_buffer[26]-48)*10+rx2_buffer[27]-48;
						}
						else if(rx2_buffer[13]=='-')
						{
							Aconstant=1000+(rx2_buffer[25]-48)*100+(rx2_buffer[26]-48)*10+rx2_buffer[27]-48;
						}
						else 
						{
							Aconstant=(rx2_buffer[24]-48)*1000+(rx2_buffer[25]-48)*100+(rx2_buffer[26]-48)*10+rx2_buffer[27]-48;
						}
						
						parameter.A6=Aconstant/1000;
						parameter.B6=(Aconstant%1000)/100;
						parameter.C6=(Aconstant%100)/10;
						parameter.D6=Aconstant%10;
						
						if(debug_usart)
						{
								printf("ʪ�ȶ�����%d\r\n",Hcorrect);
								printf("���أ�%d\r\n",button1);
								printf("�洢�����%d\r\n",space1);
								printf("���¶�����%d\r\n",Tcorrect);
								printf("ʪ�ȶ�����%d\r\n",Hcorrect);
								printf("���ٶ�����%d\r\n",Wcorrect);
								printf("��ѹ������%d\r\n",Airpressure);
								printf("��ѹ������%d\r\n",Aircon);
								printf("����������%d\r\n",Aconstant);
						}
				}
				
				
				else if(flag_l == 1)
				{
						flag_l = 0;
						printf("clear data\r\n");
						Tcorrect = 0;
						button1 = 0;
						space1 = 0;
						Hcorrect = 0;
						Wcorrect = 0;
						Airpressure = 0;
						Aircon = 0;
						Aconstant = 0;
						
						parameter.A1 = 0;
						parameter.A2 = 0;	
						parameter.A3 = 0;
						parameter.A4 = 0;
						parameter.A5 = 0;
						parameter.A6 = 0;
						parameter.A7 = 0;
						parameter.B1 = 0;
						parameter.B2 = 0;
						parameter.B3 = 0;
						parameter.B4 = 0;
						parameter.B5 = 0;
						parameter.B6 = 0;
						parameter.B7 = 0;
						
						parameter.C1 = 0;
						parameter.C2 = 0;
						parameter.C3 = 0;
						parameter.C4 = 0;
						parameter.C5 = 0;
						parameter.C6 = 0;
						parameter.C7 = 0;
						
						parameter.D4 = 0;
						parameter.D6 = 0;
						
						parameter.E4 = 0;
				}
			}
			else    //���յ��Ĳ�����Ҫ�����ݾ�ȫ��ͨ������1��ӡ���ڵ���������
			{
 	      //HAL_UART_Transmit(&huart1,(uint8_t *)rx2_buffer,sizeof(rx2_buffer),1000);
			}
			
			rx2_count=0;
			rx2_cplt=false;                                              /* clear the flag of receive */
			memset(rx2_buffer,0,sizeof(rx2_buffer));                      /* clear the register of receive */
			
		}
	}
}

/*����1������������*/
static void Storage1_Thread(void const *argument)
{
	char *p;
	uint32_t i;
	int group_i = 0;
	while(1)
	{
		if(osSemaphoreWait(semaphore_usart1,osWaitForever)==osOK)
		{
			if(strlen(rx1_buffer)==7)
				{
					if(memcmp("<INT01>",rx1_buffer,4)==0)//�洢���ʱ��  ��λ����
						{
							printf("%s\r\n",rx1_buffer);//��ӡ������������
							p=memchr(rx1_buffer,'T',4);
							space=atoi(p+1);
							//printf("�洢�����%d\r\n",space);
						}
						
//					else if(memcmp("<S0001>",rx1_buffer,7)==0)
//					{
//						if(number_N ==0)
//						{
//							AT24C512_Read(0xA0,0x00,read_buf,64);
//							HAL_UART_Transmit(&huart1,read_buf,sizeof(read_buf),1000);
//							HAL_UART_Transmit(&huart1,(uint8_t *)"}",1,1000);
//						}
//						else if(number_N<1025)
//						{
//							AT24C512_Read(0xA0,0x00+0x40*(number_N-1),read_buf,64);
//							HAL_UART_Transmit(&huart1,read_buf,sizeof(read_buf),1000);
//							HAL_UART_Transmit(&huart1,(uint8_t *)"}",1,1000);
//						}
//						else
//						{
//							AT24C512_Read(0xA4,0x00+0x40*(number_N-1025),read_buf,64);
//							HAL_UART_Transmit(&huart1,read_buf,sizeof(read_buf),1000);
//							HAL_UART_Transmit(&huart1,(uint8_t *)"}",1,1000);
//						}
//					}						
					else if(memcmp("<Sxxxx>",rx1_buffer,2)==0)//��ȡ�洢�����ݣ�xxxx�������������
					{
						p=memchr(rx1_buffer,'S',2);
						group=atoi(p+1);
						
						group_i = number_N;
						HAL_UART_Transmit(&huart1,(uint8_t *)"{",1,1000);
						for(i=0; i<group; i++)
						{
								if(group_i < 1025)
								{
										memset(read_buf, 0, sizeof(read_buf));
										AT24C512_Read(0xA0, 0x00+0x40 * group_i, read_buf, 63);
										if((read_buf[0] == '[') && (read_buf[62] == ']'))
										{
												HAL_UART_Transmit(&huart1,read_buf,sizeof(read_buf),1000);
										}
								}
								else
								{
									
										AT24C512_Read(0xA4, 0x00+(group_i - 1024) * 0x40, read_buf, 63);
										if((read_buf[0] == '[') && (read_buf[62] == ']'))
										{
												HAL_UART_Transmit(&huart1,read_buf,sizeof(read_buf),1000);
										}
								}
								group_i--;
								if(group_i <= 0)
								{
										group_i = 2040;
								}
						}
						HAL_UART_Transmit(&huart1,(uint8_t *)"}",1,1000);
						
						
						#if 0
            if(group <= number_N)
						{
              //osDelay(1000);							
							for(i=0;i<group;i++)
							{
								AT24C512_Read(0xA0,0x00+0x40*number_N-0x40*(i+1),read_buf,63);
								HAL_UART_Transmit(&huart1,read_buf,sizeof(read_buf),1000);
								if((i%200)==0)
								{
									HAL_UART_Transmit(&huart1,(uint8_t *)"{",1,1000);
									osDelay(1000);
								}
							}
							HAL_UART_Transmit(&huart1,(uint8_t *)"}",1,1000);
						}
						else if(group >number_N)
						{
							for(i=0;i<number_N;i++)
							{
								AT24C512_Read(0xA0,0x00+0x40*number_N-0x40*(i+1),read_buf,63);
								HAL_UART_Transmit(&huart1,read_buf,sizeof(read_buf),1000);
								if((i%200)==0)
								{
									HAL_UART_Transmit(&huart1,(uint8_t *)"{",1,1000);
									osDelay(1000);
								}
							}
							for(i=0;i<group-number_N;i++)//����Ҫ��ȡ����������������һ��AT24C512����������Ҫ���ڶ���AT24C512������
							{
								AT24C512_Read(0xA4,0x00+i*0x40,read_buf,63);
								HAL_UART_Transmit(&huart1,read_buf,sizeof(read_buf),1000);
								if((i%200)==0)
								{
									HAL_UART_Transmit(&huart1,(uint8_t *)"{",1,1000);
									osDelay(1000);
								}
							}
							HAL_UART_Transmit(&huart1,(uint8_t *)"}",1,1000);
						}
						#endif
			  	}
					else if(memcmp("<OOxxx>",rx1_buffer,3)==0)
					{
						printf("%s\r\n",rx1_buffer);//��ӡ������������
						zhanhao = atoi(rx1_buffer+3);
						printf("վ�ţ�%d\r\n",zhanhao);
					}
					else if(memcmp("<QKONG>",rx1_buffer,7)==0)
					{
						number_N = 0;
						for(i=0;i<65536;i++)
						{
							FM24C256_Write_NByte(0xA0,0x00+i,1,"255");	//д��ȥ��	  ���λΪ0��ʾд  oxa0�����һ��EEPROM��д��ַ
							FM24C256_Write_NByte(0xA4,0x00+i,1,"255");	//д��ȥ��	  ���λΪ0��ʾд  oxa0�����һ��EEPROM��д��ַ
						}
					}
					else if(memcmp("<GBACK>",rx1_buffer,7)==0)
					{
						HAL_NVIC_SystemReset();
					}
					else if(memcmp("<AUTOM>",rx1_buffer,7)==0)
					{
						HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_SET);
						HAL_UART_Transmit(&huart2,(uint8_t *)"<AUTOM>",10,1000);//������<Sxxxx>,<INTxx>֮�����������ȫ�����͸�����2
						HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_RESET);
					}
					else if(memcmp("<MANUA>",rx1_buffer,7)==0)
					{
						HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_SET);
						HAL_UART_Transmit(&huart2,(uint8_t *)"<MANUA>",10,1000);//������<Sxxxx>,<INTxx>֮�����������ȫ�����͸�����2
						HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_RESET);
					}
//					else if(memcmp("<Write>",rx1_buffer,7)==0)
//					{
//						FM24C256_Write_NByte(0xA0,0x00,30,"I Write Chinese!!!\r\n");	//д��ȥ��	  ���λΪ0��ʾд  oxa0�����һ��EEPROM��д��ַ
//						FM24C256_Write_NByte(0xA4,0x00,30,"I Write English!!!\r\n");	//д��ȥ��	  ���λΪ0��ʾд  oxa0�����һ��EEPROM��д��ַ
//						//HAL_UART_Transmit(&huart1,"д����\r\n",20,1000);
//					}
//					else if(memcmp("<Read1>",rx1_buffer,7)==0)
//					{
//						AT24C512_Read(0xA0,0x00,read_buf,30);
//						HAL_UART_Transmit(&huart1,read_buf,sizeof(read_buf),1000);
//						AT24C512_Read(0xA4,0x00,read_buf,30);
//						HAL_UART_Transmit(&huart1,read_buf,sizeof(read_buf),1000);
//						//HAL_UART_Transmit(&huart1,"������\r\n",20,1000);
//					}
					else if(memcmp("<DEBUG>",rx1_buffer,7)==0)			//2018-03-30 13:43��ӵ���������Բ鿴һЩ������Ϣ
					{
							if(debug_usart)
							{
								debug_usart = 0;
							}
							else
							{
								debug_usart = 1;
							}
							printf("T\r\n");
							
					}
					else
					{
						HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_SET);
						HAL_UART_Transmit(&huart2,(uint8_t *)rx1_buffer,sizeof(rx1_buffer),1000);//������<Sxxxx>,<INTxx>֮�����������ȫ�����͸�����2
						HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_RESET);
						printf("F\r\n");
					}
				}
		else if(strlen(rx1_buffer)==15)//���յ���ʱ����
		{
			printf("%s\r\n",rx1_buffer);
			HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_SET);//485оƬ��������Ϊ�ߵ�ƽʱ����������
			HAL_UART_Transmit(&huart2,(uint8_t *)rx1_buffer,sizeof(rx1_buffer),1000);//������ֱ�ӷ��͸�������
			HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_RESET);//485оƬ��������Ϊ�͵�ƽʱ����������
			
			p=memchr(rx1_buffer,'P',12);
			snprintf(Tbuf,sizeof(Tbuf),"%s",p+1);
			
			Usart_Time.Seconds     =   (Tbuf[10]-48)*10+Tbuf[11]-48;
			Usart_Time.Minutes     =   (Tbuf[8]-48)*10+Tbuf[9]-48;
			Usart_Time.Hours       =   (Tbuf[6]-48)*10+Tbuf[7]-48;
			Usart_Date.Date        =   (Tbuf[4]-48)*10+Tbuf[5]-48;
			Usart_Date.Month       =   (Tbuf[2]-48)*10+Tbuf[3]-48;
			Usart_Date.Year        =   (Tbuf[0]-48)*10+Tbuf[1]-48;
			/*������ò���*/
			Usart_Time.DayLightSaving=RTC_DAYLIGHTSAVING_NONE;
      Usart_Time.StoreOperation=RTC_STOREOPERATION_RESET;
      Usart_Time.SubSeconds=0;
      Usart_Time.TimeFormat=RTC_HOURFORMAT12_AM;
			if(set_sys_time(&Usart_Date,&Usart_Time)<0)
			{
				printf("USART SET TIME ERROR\r\n");
			}
//			else 
//			{
//				printf("USART SET TIME SUCCEED\r\n");
//			}
		}
		else if(strlen(rx1_buffer)==8)//���յ���ʱ����
		{
			if(memcmp("<BANBEN>",rx1_buffer,8)==0)
			{
				printf("BanBen:V1.2");
			}
		}
		#if 0
		else if(strlen(rx1_buffer)==69)
			{
				//HAL_UART_Transmit(&huart1,(uint8_t *)rx2_buffer,sizeof(rx2_buffer),1000);
				//HAL_UART_Transmit(&huart1,(uint8_t *)"\r\n",2,1000);
				DATA_FLAG =true;
				DATA_Number = 0;//���㷢�Ͷ�ȡ�����������
				//printf("���ݣ�%s\r\n",rx1_buffer);
				  p1=strchr(rx1_buffer,'<');
				  pointer_1=strtok(p1,"<,>");
          //printf("ʱ�䣺%s\r\n",pointer_1);//ʱ�� 
				
				  p2=strchr(p1,',');
				  pointer_2=strtok(p2,",>");
				  temperature=atof(pointer_2);
          //printf("���£�%f\r\n",temperature);//����

				  p3=strchr(p2,',');
					pointer_3=strtok(p3,",");
				  humi=atoi(pointer_3); 
          //printf("ʪ�ȣ�%d\r\n",humi);//ʪ��
				 
				  p4=strchr(p3,',');
					pointer_4=strtok(p4,",");
				  airpressure=atof(pointer_4);
          //printf("��ѹ��%f\r\n",airpressure);//��ѹ
				 
				  p5=strchr(p4,',');
					pointer_5=strtok(p5,",");
					windirect=atoi(pointer_5);
          //printf("����%d\r\n",windirect);//����
					
				  p6=strchr(p5,',');
					pointer_6=strtok(p6,",");
					wspeed=atof(pointer_6);
          //printf("���٣�%f\r\n",wspeed);//����
 
				  p7=strchr(p6,',');
					pointer_7=strtok(p7,",");
					twodirect=atoi(pointer_7);
          //printf("2�ַ���%d\r\n",twodirect);//2�ַ���
				 
				  p8=strchr(p7,',');
					pointer_8=strtok(p8,",");
					twospeed=atof(pointer_8);
          //printf("2�ַ��٣�%f\r\n",twospeed);//2�ַ���
  
				  p9=strchr(p8,',');
					pointer_9=strtok(p9,",");
					tendirect=atoi(pointer_9);
          //printf("10�ַ���%d\r\n",tendirect);//10�ַ���
					
					p10=strchr(p9,',');
					pointer_10=strtok(p10,",");
					tenspeed=atof(pointer_10);
          //printf("10�ַ��٣�%f\r\n",tenspeed);//10�ַ���
					
					p11=strchr(p10,',');
					pointer_11=strtok(p11,",");
          //printf("���£�%s\r\n",pointer_11);//����
					
					p12=strchr(p11,',');
					pointer_12=strtok(p12,",>");
					voltage=atof(pointer_12);
          //printf("��ѹ��%f\r\n",voltage);//��ѹ
				}
		#endif
//		else
//		{
//			printf("ERROR\r\n");
//		}
	rx1_count=0;
  rx1_cplt=false;                                              /* clear the flag of receive */
  memset(rx1_buffer,0,sizeof(rx1_buffer));                      /* clear the register of receive */
	}
	}
}



/**����1�жϺ���*/
void USART1_IRQHandler(void)
{
  UART_HandleTypeDef *huart=&huart1;
  uint32_t tmp_flag = 0, tmp_it_source = 0;

  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_PE);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_PE);  
  /* UART parity error interrupt occurred ------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  { 
    huart->ErrorCode |= HAL_UART_ERROR_PE;
  }
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_FE);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_ERR);
  /* UART frame error interrupt occurred -------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    huart->ErrorCode |= HAL_UART_ERROR_FE;
  }
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_NE);
  /* UART noise error interrupt occurred -------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    huart->ErrorCode |= HAL_UART_ERROR_NE;
  }
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_ORE);
  /* UART Over-Run interrupt occurred ----------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    huart->ErrorCode |= HAL_UART_ERROR_ORE;
  }
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_RXNE);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_RXNE);
  /* UART in mode Receiver ---------------------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    /*UART_Receive_IT(huart);*/
    uint8_t data=0;
  
    data=huart->Instance->DR;  /* the byte just received  */
    
		
		if(!rx1_cplt)
    {
      if(data=='<')
      {
        cr1_begin=true;
        rx1_count=0;
        rx1_buffer[rx1_count]=data;
        rx1_count++; 
      }
     
      else if(cr1_begin==true)
      {
        rx1_buffer[rx1_count]=data;
        rx1_count++; 
        if(rx1_count>UART_RX_BUF_SIZE-1)  /* rx buffer full */
        {
          /* Set transmission flag: trasfer complete*/
          rx1_cplt=true;
        }
        
        if(data=='>')
        {
          rx1_cplt=true;
          cr1_begin=false;
        }
      }
      else
      {
        rx1_count=0;
      }
    }

		
		
//    if(!rx1_cplt)
//    {
//      if(cr1_begin==true)  /* received '\r' */
//      {
//        cr1_begin=false;
//        if(data=='\n')  /* received '\r' and '\n' */
//        {
//          /* Set transmission flag: trasfer complete*/
//          rx1_cplt=true;
//        }
//        else
//        {
//          rx1_count=0;
//        }
//      }
//      else
//      {
//        if(data=='\r')  /* get '\r' */
//        {
//          cr1_begin=true;
//        }
//        else  /* continue saving data */
//        {
//          rx1_buffer[rx1_count]=data;
//          rx1_count++;
//          if(rx1_count>UART_RX_BUF_SIZE-1)  /* rx buffer full */
//          {
//            /* Set transmission flag: trasfer complete*/
//            rx1_cplt=true;
//          } 
//        }
//       }
//      }
    
  	 /* received a data frame ���ݽ�����ɾ��ͷŻ�����*/
    if(rx1_cplt==true)
    {
      if(semaphore_usart1!=NULL)
      {
         /* Release mutex */
        osSemaphoreRelease(semaphore_usart1);
				//printf("1���յ����ݲ��ͷ����ź���\r\n");
      }
    }

   
    }
  
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_TXE);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_TXE);
  /* UART in mode Transmitter ------------------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    /*UART_Transmit_IT(huart);*/
  }

  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_TC);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_TC);
  /* UART in mode Transmitter end --------------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    /*UART_EndTransmit_IT(huart);*/
  }  

  if(huart->ErrorCode != HAL_UART_ERROR_NONE)
  {
    /* Clear all the error flag at once */
    __HAL_UART_CLEAR_PEFLAG(huart);
    
    /* Set the UART state ready to be able to start again the process */
    huart->State = HAL_UART_STATE_READY;
    
    HAL_UART_ErrorCallback(huart);
  } 
}

/**����2�жϺ���*/
void USART2_IRQHandler(void)
{
  UART_HandleTypeDef *huart=&huart2;
  uint32_t tmp_flag = 0, tmp_it_source = 0;

  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_PE);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_PE);  
  /* UART parity error interrupt occurred ------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  { 
    huart->ErrorCode |= HAL_UART_ERROR_PE;
  }
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_FE);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_ERR);
  /* UART frame error interrupt occurred -------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    huart->ErrorCode |= HAL_UART_ERROR_FE;
  }
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_NE);
  /* UART noise error interrupt occurred -------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    huart->ErrorCode |= HAL_UART_ERROR_NE;
  }
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_ORE);
  /* UART Over-Run interrupt occurred ----------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    huart->ErrorCode |= HAL_UART_ERROR_ORE;
  }
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_RXNE);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_RXNE);
  /* UART in mode Receiver ---------------------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    /*UART_Receive_IT(huart);*/
    uint8_t data=0;
  
    data=huart->Instance->DR;  /* the byte just received  */
    
    
  
     
			if(!rx2_cplt)
    {
      if(data=='<')
      {
        cr2_begin=true;
        rx2_count=0;
        rx2_buffer[rx2_count]=data;
        rx2_count++; 
      }
     
      else if(cr2_begin==true)
      {
        rx2_buffer[rx2_count]=data;
        rx2_count++; 
        if(rx2_count>UART_RX_BUF_SIZE-1)  /* rx buffer full */
        {
          /* Set transmission flag: trasfer complete*/
          rx2_cplt=true;
        }
        
        if(data=='>')
        {
          rx2_cplt=true;
          cr2_begin=false;
        }
      }
      else
      {
        rx2_count=0;
      }

    }
		
		
		 /* received a data frame ���ݽ�����ɾ��ͷ��ź���*/
    if(rx2_cplt==true)
    {
			//printf("����2���յ�����\r\n");
      if(semaphore_usart2!=NULL)
      {
        /* Release the semaphore */
				//printf("�ź���2�����ɹ�\r\n");
        osSemaphoreRelease(semaphore_usart2);
				//printf("�ͷŴ���2���ź�\r\n");
      }
    }
		
		
    }
  
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_TXE);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_TXE);
  /* UART in mode Transmitter ------------------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    /*UART_Transmit_IT(huart);*/
  }

  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_TC);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_TC);
  /* UART in mode Transmitter end --------------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    /*UART_EndTransmit_IT(huart);*/
  }  

  if(huart->ErrorCode != HAL_UART_ERROR_NONE)
  {
    /* Clear all the error flag at once */
    __HAL_UART_CLEAR_PEFLAG(huart);
    
    /* Set the UART state ready to be able to start again the process */
    huart->State = HAL_UART_STATE_READY;
    
    HAL_UART_ErrorCallback(huart);
  } 
}




