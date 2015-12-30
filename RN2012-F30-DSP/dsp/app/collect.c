/*************************************************************************/
/*
 * Copyright (c) 2015, ֣�����ܵ��� Runner All rights reserved.
 *
 * file       : collect.c
 * Design     : zlb
 * Email	  : longbiao831@qq.com
 * Data       : 2015-8-10
 * Version    : V1.0
 * Change     :
 * Description: ң�⡢ң�����ݲɼ� Collect_Task�������г���
 */
/*************************************************************************/
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>
#include "hw_gpio.h"
#include "collect.h"
#include "communicate.h"
#include "emif_fpga.h"
#include "IPCServer.h"
#include "ad7606.h"				//��� 9.9
#include "sys.h"
#include "led.h"
#include "log.h"
#include "fa.h"


UChar FGflag = 0;

/***************************************************************************/
//����:	Void ChangeLED(UInt16 *status)
//˵��:	LED�ֺ�״ָ̬ʾ���л�
//����: status YX״̬���ݵ�ַ
//���: ��
//�༭:
//ʱ��:2015.8.18
/***************************************************************************/
Void ChangeLED(UInt32 *status)
{
	//��բ��λΪ�� 
	if((*status >> (FWYX-1)) & 0x01)
	{
		LEDDATA |= 0x01<<LED_FZ1;
	}
	else
	{
		// ������բLED
		LEDDATA &= ~(0x01<<LED_FZ1);
	}
	//��բ��λΪ��
	if((*status >> (HWYX-1)) & 0x01)
	{
		LEDDATA |= 0x01<<LED_HZ1;
	}
	else
	{
		//������բLED
		LEDDATA &= ~(0x01<<LED_HZ1);
	}
	/* yk2 ������ӵ����� */
	/* fugui�ź���Ч*/
	if((*status >> (FGYX-1)) & 0x01)
	{
		FGflag = 0;
	}
	else
	{
		FGflag = 1;
	}

	LED_SENDOUT(LEDDATA);
	
}
/***************************************************************************/
//����:	Void Collect_Task(UArg arg0, UArg arg1) 
//˵��:	���ݲɼ�����
//����: arg0 arg1
//���: ��
//�༭:
//ʱ��:2015.4.24
/***************************************************************************/
Void Collect_Task(UArg arg0, UArg arg1) 
{
	UInt32 *yxdata = (UInt32 *)ShareRegionAddr.digitIn_addr;
	SYSPARAMSTR *sysparm = (SYSPARAMSTR *)ShareRegionAddr.sysconf_addr;
	UChar i;
	UInt32 channel;
	// ң��Ĭ��ֵȫ��Ϊ1
	UInt32 newdata = 0xff;
	UInt32 comparedata = 0;
	UInt32 status = 0;
	unsigned long ticks[10]={0};
	/* ����û�п����������,ϵͳʱ�ӽ�����U32��ʽ��С����1000us�����Ҫ��֤ϵͳ����49��֮������һ�μ���*/
//	UInt16 overflow = 0;
	/* 10��ʾӲ��ֻ��10��ң�Žڵ� �����зֱ�Ϊÿ��ң�ŵ�IO�ܽ� */
	UChar yxio_num[10] = {YX_1,YX_2,YX_3,YX_4,YX_5,YX_6,YX_7,YX_8,YX_9,YX_10};
	/* yx��ʼֵΪ0 */
	yxdata[0] = 0;
	for(i=0;i<10;i++)
	{
		if(GPIOPinRead(SOC_GPIO_0_REGS, yxio_num[i]) == 0)
		{
			newdata &= ~(0x01<<i);
		}
	}
	yxdata[0] = newdata;	
	
	while(1)
	{
		// ��������
		newdata = yxdata[0];
		/* ң�����ݱ�λ */
		for(i=0;i<10;i++)
		{
			if((status>>i) & 0x01)
			{	
				/* ��ʱʱ�䵽 Clock_getTicks() */
				if(ticks[i] <= tickets)
				{
					comparedata = (newdata >> i) & 0x01;
					if(comparedata != GPIOPinRead(SOC_GPIO_0_REGS, yxio_num[i]))
					{
						newdata ^= (0x01<<i);
						//���Ƶ�������
						yxdata[0] = newdata;
						/* �ж��Ƿ�ȡ��ʹ�� */
						if(dianbiaodata.yxnot & (0x01<<i))
						{
							// ʹ��ȡ���������ٴ����
							newdata ^= (0x01<<i);
						}
						if(dianbiaodata.yxcos & (0x01<<i))
						{
							//send cos msg
							channel = 0x01<<i;
							Message_Send(MSG_COS, channel, newdata);
						}
						if(dianbiaodata.yxsoe & (0x01<<i))
						{
							//send soe msg
							channel = 0x01<<i;
							Message_Send(MSG_SOE, channel, newdata);
						}
					}
					/* �����λ��־ */
					status &= ~(0x01<<i);
				}
			}
			/* �ж��Ƿ�ʹ�� */
			else if((dianbiaodata.yxen >> i) & 0x01)
			{	
				comparedata = (newdata>>i) & 0x01;
				if(comparedata != GPIOPinRead(SOC_GPIO_0_REGS, yxio_num[i]))
				{
					/* ����flag��־ ��ʾ״̬�����ı� */
					status |= (0x01<<i);
					/* sysparm->yxtimeң��ȥ��ʱ�� */
					ticks[i] = tickets + sysparm->yxtime;
				}
			}
		}		
		/* ������Ӧ��LED(�ֺ�բLED) */
		ChangeLED(yxdata);
		/* ��ʱ2ms */
		Task_sleep(2);
	}

}



