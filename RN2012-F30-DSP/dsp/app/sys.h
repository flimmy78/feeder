/*************************************************************************/
/*
 * Copyright (c) 2015, 郑州瑞能电气 Runner All rights reserved.
 *
 * file       : sys.h
 * Design     : zlb
 * Email	  : longbiao831@qq.com
 * Data       : 2015-8-7
 * Version    : V1.0
 * Change     :
 * Description: 系统配置头文件
 */
/*************************************************************************/
#ifndef _SYS_H_
#define _SYS_H_
#include <xdc/std.h>
#include "gpio.h"
#include "soc_C6748.h"
#include "communicate.h"
#include "ad7606.h"

#define SetIO_HIGH(PIN)				GPIOPinWrite(SOC_GPIO_0_REGS, PIN, GPIO_PIN_HIGH)
#define SetIO_LOW(PIN)					GPIOPinWrite(SOC_GPIO_0_REGS, PIN, GPIO_PIN_LOW)


extern channalstr FFT_In[8];			//存放整组采样值 2*64 64 表示数组
extern AD7606Str ad7606Str;			//AD采集所需参数		
extern UChar LEDDATA;					//LED指示灯数据
extern channalstr AD_Data[8];
extern volatile unsigned long tickets;	//定时器节拍
extern volatile UInt16 distance;		//临时添加
extern volatile unsigned long towards;
/* 函数声明 */
void Sys_Init(void);
Void Clock_Init(CurrentPaStr *freq);
Void StartClock(AD7606Str *flag);
Int Sys_Check(void);
Void TimerIsr(UArg arg);
Void Timer3Isr(UArg arg);
Void Timer_Init(void);

#endif /* _SYS_H_ */
