/*************************************************************************/
/*
 * Copyright (c) 2015, ֣�����ܵ��� Runner All rights reserved.
 *
 * file       : ad7606.h
 * Design     : zlb
 * Email	  : longbiao831@qq.com
 * Data       : 2015-7-28
 * Version    : V1.0
 * Change     :
 * Description: AD7606���ݲɼ�����ͷ�ļ�
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


/* �������� */
#define TN  64					//ÿ���ڲɼ�64������
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

/* AD7606���ݽṹ�� */
struct _AD7606_
{
	channalstr *databuff;		//���ݻ�����
	UInt16 event_id;			//�¼�id
	UInt16 irq_event;			//�ж�������
	float analog_ad;			//AD�ɼ�ֵת��ΪmA��mV
	UInt32 freq;				//�ɼ�Ƶ��
	
	Timer_Handle clock;			//��ʱʱ��
	UInt16 conter;				//�ɼ���ż���
	UInt16 select_ad;			//adƵ�����ѡ��
	UInt16 flagfreq;			//������λ�ñ��bit[0] Ƶ�ʲɼ����bit[1] Ƶ�������ʽ���bit[2] ���ʱ���Ƿ񴴽�bit[3]
	Semaphore_Handle sem;		//�ź���
};
typedef struct _AD7606_ AD7606Str;


void AD7606_Init(channalstr *buff, AD7606Str *ad);



/* function */
#endif/* _AD7606_H_ */
