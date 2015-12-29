/*************************************************************************/
/*
 * Copyright (c) 2015, ֣�����ܵ��� Runner All rights reserved.
 *
 * file       : sys.c
 * Design     : zlb
 * Email	  : longbiao831@qq.com
 * Data       : 2015-8-6
 * Version    : V1.0
 * Change     :
 * Description: ϵͳ�����ļ�
 */
/*************************************************************************/
#include "stdio.h"
#include <xdc/std.h>

#include "sys.h"

#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/timers/timer64/Timer.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/sysbios/knl/Clock.h>
#include <xdc/runtime/Error.h>
#include "fft.h"
#include "sht11.h"
#include "led.h"
#include "log.h"



#define 	Period 	   1*1000  // 1s��ʱ��

channalstr AD_Data[8];			//���2��AD�ɼ���ԭʼ���� 
channalstr FFT_In[8]; 			//����������ֵ 2*64 64 ��ʾ����
AD7606Str ad7606Str;			//AD�ɼ��������	
UChar LEDDATA;					//LEDָʾ������
Timer_Handle Timer3;			//��ʱ�����
Clock_Handle SecondClock;		//��ʱ�����
volatile unsigned long tickets = 0;	//��������

volatile UInt16 distance = 0;	//��ʱ���
volatile unsigned long towards = 0;

/***************************************************************************/
//����:void Sys_Init(void)
//˵��:ϵͳ��ʼ������
//����: ��
//���: ��
//�༭:zlb
//ʱ��:2015.8.6
/***************************************************************************/
void Sys_Init(void)
{
	/* ��ʼ��LED */
	LED_GPIO_Config();
	/* ��������ָʾ�� LEDDATA��ʼ��Ϊ0 LED1 */
	LEDDATA = 0;
	LEDDATA |= ~(0x01<<LED_RUN);
	LED_SENDOUT(LEDDATA);
	/* ��ʼ��AD7606 */
	AD7606_Init(AD_Data,&ad7606Str);
	/* �жϳ�ʼ�� */
	/* SHT11�¶ȴ�������ʼ�� */
	TEMP_GPIO_Config();
	/* ��ʱ����ʼ�� */
	
	/* YX��ʼ������ */
	YX_GPIO_Config();
	/* YK��ʼ������ */
	YK_GPIO_Config();
	Timer_Init(); 
	
}
/***************************************************************************/
//����:Int Sys_Check(void)
//˵��:ϵͳ��ʼ������
//����: ��
//���: 1 ϵͳ���� 0 ϵͳ����
//�༭:zlb
//ʱ��:2015.8.6
/***************************************************************************/
Int Sys_Check(void)
{
	UChar err = 1;
	UChar status = 0;

	/* ң���Լ� Ԥ���Ƿ���Ч*/
	// ʹ��Ԥ�ü̵���
	GPIOPinWrite(SOC_GPIO_0_REGS, YK_YZ, GPIO_PIN_LOW);
	Delay(50000);
	// ��ȡԤ�ü̵���״̬�Ƿ�Ϊ�͵�ƽ
	status = GPIOPinRead(SOC_GPIO_0_REGS, YK_YZS);
	if(status)
	{
		err = 0;
	}

	GPIOPinWrite(SOC_GPIO_0_REGS, YK_YZ, GPIO_PIN_HIGH);
	/* ң���Լ� �ֺ�բ�ź��Ƿ�����*/
	

	return err;
}
/***************************************************************************/
//����:Int Sys_Check(void)
//˵��:ϵͳ��ʼ������
//����: ��
//���: 1 ϵͳ���� 0 ϵͳ����
//�༭:zlb
//ʱ��:2015.8.6
/***************************************************************************/
Void Timer_Init(void)
{
	Timer_Params timerParams;
	Error_Block eb;

	Error_init(&eb);
	Timer_Params_init(&timerParams);

	// 1ms��ʱ��
	timerParams.period = 1000;	
	timerParams.periodType = Timer_PeriodType_MICROSECS;
	timerParams.startMode = Timer_StartMode_AUTO;
 
	Timer3 = Timer_create(3, Timer3Isr, &timerParams, &eb);
	if(Timer3 == NULL)
    {
    	LOG_INFO("Timer3 create failed");
    }
	Timer_start(Timer3);
}
/***************************************************************************/
//����:Void Timer3Isr(UArg arg)
//˵��:ϵͳ��ʼ������
//����: ��
//���: 
//�༭:zlb
//ʱ��:2015.8.6
/***************************************************************************/
Void Timer3Isr(UArg arg)
{
	tickets++;
}
/***************************************************************************/
//����:Void TimerIsr(UArg arg)
//˵��:ϵͳ��ʼ������
//����: ��
//���: 1 ϵͳ���� 0 ϵͳ����
//�༭:zlb
//ʱ��:2015.8.6
/***************************************************************************/
Void TimerIsr(UArg arg)
{	
	CurrentPaStr *hz = (CurrentPaStr *)arg;

	/* ���������־ */
	if(ad7606Str.flagfreq & 0x02)
	{	
//		ad7606Str.flagfreq &= ~0x02;
		// Ƶ��>40hz <60hz ��Ч
		if(ad7606Str.freq > 2560 && ad7606Str.freq < 3840)
		{
			// �������Ƶ ����Ƶ��*100/64 ��100�ǽ�Ƶ�ʱ�Ϊ100�� /
//			hz->F1.Param = ad7606Str.freq >> 6 ;
			hz->F1.Param = (float)ad7606Str.freq / 64;
			if(hz->F1.Param > 50)
			{
				if((hz->F1.Param - 50) > 0.05)
				{
					distance++;
					LOG_INFO("distance is %d ",distance);
				}
					
			}
		}
		// �������ֵ
		ad7606Str.freq = 0;
//		// ʹ�ܼ�����־ 
//		ad7606Str.flagfreq |= 0x02;
	}

	if(LEDDATA & (0x01<<LED_RUN))
		LEDDATA &= ~(0x01<<LED_RUN);
	else
		LEDDATA |= 0x01<<LED_RUN;
	
	LED_SENDOUT(LEDDATA);
}
/***************************************************************************/
//����:Int Sys_Check(void)
//˵��:ϵͳ��ʼ������
//����: ��
//���: 1 ϵͳ���� 0 ϵͳ����
//�༭:zlb
//ʱ��:2015.8.6
/***************************************************************************/
Void StartTimer(AD7606Str *flag)
{	
	/* ʹ��Ƶ�ʼ��� */
	flag->flagfreq |= 0x1<<1;
	flag->freq = 0;
	
	/* start timer */
	Timer_start(Timer3);
}
/***************************************************************************/
//����:Int Sys_Check(void)
//˵��:ϵͳ��ʼ������
//����: ��
//���: 1 ϵͳ���� 0 ϵͳ����
//�༭:zlb
//ʱ��:2015.9.6
/***************************************************************************/
Void Clock_Init(CurrentPaStr *freq)
{
	Clock_Params clockParams;
	Error_Block eb;

	Error_init(&eb);
	Clock_Params_init(&clockParams);

	clockParams.period = Period;
	clockParams.startFlag = TRUE ;
	clockParams.arg = (UInt32)freq;
 
	SecondClock = Clock_create(TimerIsr, Period, &clockParams, &eb);
	if(SecondClock == NULL)
    {
    	LOG_INFO("Clock create failed");
    }
}
//����:Int Sys_Check(void)
//˵��:ϵͳ��ʼ������
//����: ��
//���: 1 ϵͳ���� 0 ϵͳ����
//�༭:zlb
//ʱ��:2015.9.6
/***************************************************************************/
Void StartClock(AD7606Str *flag)
{	
	/* ʹ��Ƶ�ʼ��� ��������*/
	flag->flagfreq |= 0x1<<1;
	flag->freq = 0;

	/* start timer */
	Clock_start(SecondClock);
}
