/*************************************************************************/
/*
 * Copyright (c) 2015, ֣�����ܵ��� Runner All rights reserved.
 *
 * file       : sys.h
 * Design     : zlb
 * Email	  : longbiao831@qq.com
 * Data       : 2015-8-7
 * Version    : V1.0
 * Change     :
 * Description: ϵͳ����ͷ�ļ�
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


extern channalstr FFT_In[8];			//����������ֵ 2*64 64 ��ʾ����
extern AD7606Str ad7606Str;			//AD�ɼ��������		
extern UChar LEDDATA;					//LEDָʾ������
extern channalstr AD_Data[8];
extern volatile unsigned long tickets;	//��ʱ������
extern volatile UInt16 distance;		//��ʱ���
extern volatile unsigned long towards;
/* �������� */
void Sys_Init(void);
Void Clock_Init(CurrentPaStr *freq);
Void StartClock(AD7606Str *flag);
Int Sys_Check(void);
Void TimerIsr(UArg arg);
Void Timer3Isr(UArg arg);
Void Timer_Init(void);

#endif /* _SYS_H_ */
