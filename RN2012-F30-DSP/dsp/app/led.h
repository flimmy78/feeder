/*************************************************************************/
/*
 * Copyright (c) 2015, ֣�����ܵ��� Runner All rights reserved.
 *
 * file       : led.h
 * Design     : zlb
 * Email	  : longbiao831@qq.com
 * Data       : 2015-8-3
 * Version    : V1.0
 * Change     :
 * Description: LEDָʾ��ͷ�ļ�
 */
/*************************************************************************/
#ifndef _LED_H_
#define _LED_H_

#include <xdc/std.h>
#include "EMIFAPinmuxSetup.h"


/* LED����ָʾ��IO�ܽ� */
#define LED_SCLK		GPIO_TO_PIN(2, 10)
#define LED_DATA		GPIO_TO_PIN(2, 12)

/* YK����ܽ� */
#define YK_YZ			GPIO_TO_PIN(0, 1)
#define YK_YZS			GPIO_TO_PIN(6, 3)
#define YK_FZ1			GPIO_TO_PIN(6, 4)
#define YK_HZ1			GPIO_TO_PIN(6, 2)
#define YK_FZ2			GPIO_TO_PIN(6, 7)
#define YK_HZ2			GPIO_TO_PIN(6, 6)

/* YX����ܽ� */
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

/* YX�ܽŶ�Ӧ����״̬��*/
#define FWYX			1					//��λң��
#define HWYX			2					//��λң��
#define WCNYX			3					//δ����ң��
#define MJYX			4					//�Ž�ң��
#define YFYX			5					//�͵�ң��
#define JDYX			6					//Զ��ң��
#define HHYX			7					//�״̬ң��
#define JLSD			8					//����ʧ��澯ң��
#define DCSY			9					//���ʧѹң��
#define FGYX			10					//����ң�� 

#define LED_RUN			0					//����ָʾ��
#define LED_PROB		1					//����ָʾ��
#define LED_PROT		2					//����ָʾ��
#define LED_HZ1			3					//��բ1ָʾ��
#define LED_FZ1			4					//��բ1ָʾ��
#define LED_HZ2			5					//��բ2ָʾ��
#define LED_FZ2			6					//��բ2ָʾ��

#define PIN_HIGH		1					//�ߵ�ƽ
#define PIN_LOW			0					//�͵�ƽ
/* YK struct */
struct _YK_DATA_
{
	UInt32    DoutFlag;						//ң��Ԥ�ü̵�����λ���
	UInt32	  DoutIndex;					//ң�ر��
};	
typedef struct _YK_DATA_ DigOutStr;

void Delay(volatile unsigned int n);		//��ʱN������
void LED_SENDOUT(UChar led_data);	//LED���
void LED_GPIO_Config(void);			//LED����
void YX_GPIO_Config(void);			//YX GPIO����
void YK_GPIO_Config(void);			//YK GPIO����
void YK_SendOut(UInt16 index, UInt16 value);//YK ���

#endif/* _LED_H_ */

