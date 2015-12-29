/*************************************************************************/
/*
 * Copyright (c) 2015, 郑州瑞能电气 Runner All rights reserved.
 *
 * file       : sys.c
 * Design     : zlb
 * Email	  : longbiao831@qq.com
 * Data       : 2015-8-6
 * Version    : V1.0
 * Change     :
 * Description: 系统函数文件
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



#define 	Period 	   1*1000  // 1s定时器

channalstr AD_Data[8];			//存放2组AD采集的原始数据 
channalstr FFT_In[8]; 			//存放整组采样值 2*64 64 表示数组
AD7606Str ad7606Str;			//AD采集所需参数	
UChar LEDDATA;					//LED指示灯数据
Timer_Handle Timer3;			//定时器句柄
Clock_Handle SecondClock;		//定时器句柄
volatile unsigned long tickets = 0;	//计数节拍

volatile UInt16 distance = 0;	//临时添加
volatile unsigned long towards = 0;

/***************************************************************************/
//函数:void Sys_Init(void)
//说明:系统初始化函数
//输入: 无
//输出: 无
//编辑:zlb
//时间:2015.8.6
/***************************************************************************/
void Sys_Init(void)
{
	/* 初始化LED */
	LED_GPIO_Config();
	/* 点亮运行指示灯 LEDDATA初始化为0 LED1 */
	LEDDATA = 0;
	LEDDATA |= ~(0x01<<LED_RUN);
	LED_SENDOUT(LEDDATA);
	/* 初始化AD7606 */
	AD7606_Init(AD_Data,&ad7606Str);
	/* 中断初始化 */
	/* SHT11温度传感器初始化 */
	TEMP_GPIO_Config();
	/* 定时器初始化 */
	
	/* YX初始化配置 */
	YX_GPIO_Config();
	/* YK初始化配置 */
	YK_GPIO_Config();
	Timer_Init(); 
	
}
/***************************************************************************/
//函数:Int Sys_Check(void)
//说明:系统初始化函数
//输入: 无
//输出: 1 系统正常 0 系统故障
//编辑:zlb
//时间:2015.8.6
/***************************************************************************/
Int Sys_Check(void)
{
	UChar err = 1;
	UChar status = 0;

	/* 遥控自检 预置是否有效*/
	// 使能预置继电器
	GPIOPinWrite(SOC_GPIO_0_REGS, YK_YZ, GPIO_PIN_LOW);
	Delay(50000);
	// 读取预置继电器状态是否为低电平
	status = GPIOPinRead(SOC_GPIO_0_REGS, YK_YZS);
	if(status)
	{
		err = 0;
	}

	GPIOPinWrite(SOC_GPIO_0_REGS, YK_YZ, GPIO_PIN_HIGH);
	/* 遥信自检 分合闸信号是否正常*/
	

	return err;
}
/***************************************************************************/
//函数:Int Sys_Check(void)
//说明:系统初始化函数
//输入: 无
//输出: 1 系统正常 0 系统故障
//编辑:zlb
//时间:2015.8.6
/***************************************************************************/
Void Timer_Init(void)
{
	Timer_Params timerParams;
	Error_Block eb;

	Error_init(&eb);
	Timer_Params_init(&timerParams);

	// 1ms定时器
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
//函数:Void Timer3Isr(UArg arg)
//说明:系统初始化函数
//输入: 无
//输出: 
//编辑:zlb
//时间:2015.8.6
/***************************************************************************/
Void Timer3Isr(UArg arg)
{
	tickets++;
}
/***************************************************************************/
//函数:Void TimerIsr(UArg arg)
//说明:系统初始化函数
//输入: 无
//输出: 1 系统正常 0 系统故障
//编辑:zlb
//时间:2015.8.6
/***************************************************************************/
Void TimerIsr(UArg arg)
{	
	CurrentPaStr *hz = (CurrentPaStr *)arg;

	/* 清除计数标志 */
	if(ad7606Str.flagfreq & 0x02)
	{	
//		ad7606Str.flagfreq &= ~0x02;
		// 频率>40hz <60hz 有效
		if(ad7606Str.freq > 2560 && ad7606Str.freq < 3840)
		{
			// 计算出工频 采样频率*100/64 乘100是将频率变为100倍 /
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
		// 清零计数值
		ad7606Str.freq = 0;
//		// 使能计数标志 
//		ad7606Str.flagfreq |= 0x02;
	}

	if(LEDDATA & (0x01<<LED_RUN))
		LEDDATA &= ~(0x01<<LED_RUN);
	else
		LEDDATA |= 0x01<<LED_RUN;
	
	LED_SENDOUT(LEDDATA);
}
/***************************************************************************/
//函数:Int Sys_Check(void)
//说明:系统初始化函数
//输入: 无
//输出: 1 系统正常 0 系统故障
//编辑:zlb
//时间:2015.8.6
/***************************************************************************/
Void StartTimer(AD7606Str *flag)
{	
	/* 使能频率计数 */
	flag->flagfreq |= 0x1<<1;
	flag->freq = 0;
	
	/* start timer */
	Timer_start(Timer3);
}
/***************************************************************************/
//函数:Int Sys_Check(void)
//说明:系统初始化函数
//输入: 无
//输出: 1 系统正常 0 系统故障
//编辑:zlb
//时间:2015.9.6
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
//函数:Int Sys_Check(void)
//说明:系统初始化函数
//输入: 无
//输出: 1 系统正常 0 系统故障
//编辑:zlb
//时间:2015.9.6
/***************************************************************************/
Void StartClock(AD7606Str *flag)
{	
	/* 使能频率计数 清零数据*/
	flag->flagfreq |= 0x1<<1;
	flag->freq = 0;

	/* start timer */
	Clock_start(SecondClock);
}
