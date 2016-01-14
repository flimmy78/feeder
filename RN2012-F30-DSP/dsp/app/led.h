/*************************************************************************/
/*
 * Copyright (c) 2015, 郑州瑞能电气 Runner All rights reserved.
 *
 * file       : led.h
 * Design     : zlb
 * Email	  : longbiao831@qq.com
 * Data       : 2015-8-3
 * Version    : V1.0
 * Change     :
 * Description: LED指示灯头文件
 */
/*************************************************************************/
#ifndef _LED_H_
#define _LED_H_

#include <xdc/std.h>
#include "EMIFAPinmuxSetup.h"


/* LED控制指示灯IO管脚 */
#define LED_SCLK		GPIO_TO_PIN(2, 10)
#define LED_DATA		GPIO_TO_PIN(2, 12)

/* YK输出管脚 */
#define YK_YZ			GPIO_TO_PIN(0, 1)
#define YK_YZS			GPIO_TO_PIN(6, 3)
#define YK_FZ1			GPIO_TO_PIN(6, 4)
#define YK_HZ1			GPIO_TO_PIN(6, 2)
#define YK_FZ2			GPIO_TO_PIN(6, 7)
#define YK_HZ2			GPIO_TO_PIN(6, 6)

/* YX输出管脚 */
#define YX_1			GPIO_TO_PIN(0, 2)
#define YX_2			GPIO_TO_PIN(0, 3)
#define YX_3			GPIO_TO_PIN(0, 4)
#define YX_4			GPIO_TO_PIN(0, 0)
#define YX_5			GPIO_TO_PIN(1, 15)
#define YX_6			GPIO_TO_PIN(0, 7)
#define YX_7			GPIO_TO_PIN(0, 12)
#define YX_8			GPIO_TO_PIN(0, 13)
#define YX_9			GPIO_TO_PIN(0, 14)
#define YX_10			GPIO_TO_PIN(0, 15)

/* YX管脚对应具体状态量*/
#define FWYX			1					//分位遥信
#define HWYX			2					//合位遥信
#define WCNYX			3					//未储能遥信
#define MJYX			4					//门禁遥信
#define YFYX			5					//就地遥信
#define JDYX			6					//远方遥信
#define HHYX			7					//活化状态遥信
#define JLSD			8					//交流失电告警遥信
#define DCSY			9					//电池失压遥信
#define FGYX			10					//复归遥信 

#define LED_RUN			0					//运行指示灯
#define LED_PROB		1					//故障指示灯
#define LED_PROT		2					//保护指示灯
#define LED_HZ1			3					//合闸1指示灯
#define LED_FZ1			4					//分闸1指示灯
#define LED_HZ2			5					//合闸2指示灯
#define LED_FZ2			6					//分闸2指示灯

#define PIN_HIGH		1					//高电平
#define PIN_LOW			0					//低电平
/* YK struct */
struct _YK_DATA_
{
	UInt32    DoutFlag;						//遥控预置继电器置位标记
	UInt32	  DoutIndex;					//遥控编号
};	
typedef struct _YK_DATA_ DigOutStr;

void Delay(volatile unsigned int n);		//延时N个节拍
void LED_SENDOUT(UChar led_data);	//LED输出
void LED_GPIO_Config(void);			//LED配置
void YX_GPIO_Config(void);			//YX GPIO配置
void YK_GPIO_Config(void);			//YK GPIO配置
void YK_SendOut(UInt16 index, UInt16 value);//YK 输出

#endif/* _LED_H_ */

