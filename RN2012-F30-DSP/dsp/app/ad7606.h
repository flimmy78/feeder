/*************************************************************************/
/*
 * Copyright (c) 2015, 郑州瑞能电气 Runner All rights reserved.
 *
 * file       : ad7606.h
 * Design     : zlb
 * Email	  : longbiao831@qq.com
 * Data       : 2015-7-28
 * Version    : V1.0
 * Change     :
 * Description: AD7606数据采集部分头文件
 */
/*************************************************************************/
#ifndef _AD7606_H_
#define _AD7606_H_

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/timers/timer64/Timer.h>
#include <ti/sysbios/knl/Semaphore.h>
#include "EMIFAPinmuxSetup.h"


/* 采样点数 */
#define TN  64					//每周期采集64点数据
/* set ad1 or ad2 */
//#define SETAD7606_1			1
#define SETAD7606_2  			1

/* AD1 I/O */
#define AD7606_BUSY_AD1			GPIO_TO_PIN(2, 13)
#define AD7606_CLK_AD1				GPIO_TO_PIN(2, 15)
#define AD7606_SELCLK_AD1			GPIO_TO_PIN(0, 5)
#define AD7606_CONVST_AD1			GPIO_TO_PIN(6, 8)
/* AD2 I/O */
#define AD7606_BUSY_AD2			GPIO_TO_PIN(2, 11)
#define AD7606_CLK_AD2 			GPIO_TO_PIN(2, 14)
#define AD7606_SELCLK_AD2			GPIO_TO_PIN(0, 6)
#define AD7606_CONVST_AD2			GPIO_TO_PIN(6, 9)

struct _ARRAY_
{
	float channel[2*TN];
};
typedef struct _ARRAY_ channalstr;

/* AD7606数据结构体 */
struct _AD7606_
{
	channalstr *databuff;		//数据缓存区
	UInt16 event_id;			//事件id
	UInt16 irq_event;			//中断向量号
	float analog_ad;			//AD采集值转换为mA或mV
	UInt32 freq;				//采集频率
	
	Timer_Handle clock;			//定时时钟
	UInt16 conter;				//采集点号计数
	UInt16 select_ad;			//ad频率输出选择
	UInt16 flagfreq;			//缓存存放位置标记bit[0] 频率采集标记bit[1] 频率输出方式标记bit[2] 输出时钟是否创建bit[3]
	Semaphore_Handle sem;		//信号量
};
typedef struct _AD7606_ AD7606Str;


void AD7606_Init(channalstr *buff, AD7606Str *ad);



/* function */
#endif/* _AD7606_H_ */
