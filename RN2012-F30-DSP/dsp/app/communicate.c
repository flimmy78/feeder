/*************************************************************************/
/*
 * Copyright (c) 2015, ֣�����ܵ��� Runner
 * All rights reserved.
 * file       : communicate.c
 * Design     : ZLB
 * Data       : 2015-4-29
 * Version    : V1.0
 * Change     :
 */
/*************************************************************************/
/* xdctools header files */
#include "string.h"
#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/knl/Event.h>

/* package header files */
#include <ti/ipc/SharedRegion.h>
#include <ti/ipc/HeapBufMP.h>
#include <ti/ipc/MessageQ.h>
#include <ti/ipc/MultiProc.h>
#include <ti/ipc/Notify.h>

#include "communicate.h"
#include "collect.h"
#include "IPCServer.h"
#include "queue.h"
#include "led.h"
#include "fft.h"
#include "log.h"
#include "fa.h"


/****************************** macro define ************************************/

/****************************** type define ************************************/

/****************************** module structure ********************************/

/******************************* functions **************************************/

/******************************* private data ***********************************/
ShareAddrBase ShareRegionAddr;
DianBiaoStr dianbiaodata;			//�������
yz_data yzdata;						//Ԥ������
//ParameterListStr* ParListPtr;		//power value ����ң�����ݻ�·������ַָ��
//����ͨ����Ӧ��indexֵ 0xff ��ʾ�����ڴ�ͨ����Ӧindex
UInt16 findindex[8] = {0,1,2,4,5,6,7,17};
/***************************************************************************/
//����:	Void Init_ShareAddr(UInt32* ptr)
//˵��: ��ʼ�������ַ
//����: ptr ����������ַ
//���: ��
//�༭:
//ʱ��:2015.5.6
/***************************************************************************/
Void Init_ShareAddr(UChar* ptr)
{
	ShareRegionAddr.base_addr = (UInt32 *)ptr;										// ����������ַ
	ShareRegionAddr.ykconf_addr = (UInt32 *)(ptr + YK_OFFSET);						// ң�����õ�ַ
	ShareRegionAddr.sysconf_addr = (UInt32 *)(ptr + SYS_OFFSET);						// ϵͳ���õ�ַ
	ShareRegionAddr.faconf_addr = (UInt32 *)(ptr + FA_OFFSET);						// ϵͳ���õ�ַ
	
	ShareRegionAddr.digitIn_addr = (UInt32 *)(ptr + YX_OFFSET);
	ShareRegionAddr.digitIn_EN_addr = (UInt32 *)(ptr + YX_OFFSET + YX_STEP ); 
	ShareRegionAddr.digitIn_COS_addr = (UInt32 *)(ptr + YX_OFFSET + 2*YX_STEP ); 
	ShareRegionAddr.digitIn_SOE_addr = (UInt32 *)(ptr + YX_OFFSET + 3*YX_STEP ); 
	ShareRegionAddr.digitIn_NOT_addr = (UInt32 *)(ptr + YX_OFFSET + 4*YX_STEP ); 
	
	ShareRegionAddr.printf_addr  = ptr + PRINTF_BASE;									//���Դ�ӡ��������ַ
	ShareRegionAddr.anadjust_addr = (float *)(ptr + YC_CHECK_PAR_BASE);				//У׼��ַ

}
/***************************************************************************/
//����:	Int8 DigAdjust(App_Msg *msg)
//˵��: У׼����
//����: msg ���յ���У׼��Ϣ У׼index �� У׼ֵ adjust У׼ֵ����
//���: status ״̬ ���� 0 У׼�ɹ� 1 У׼ʧ��
//�༭:
//ʱ��:2015.5.22
/***************************************************************************/
Int DigAdjust(App_Msg *msg, float *adjust)
{
	Int status = 1;	
	UChar channel = 0;
	UInt32 index = 0;
	float value = 0;
	UChar i;
	float *collectvalue;
	float *ShareAdjustaddr;

	if(msg->channel <= MAX_INDEX)			//�ж��Ƿ�Խ��
		index = msg->channel;				//����ֵ
	else
	{
		LOG_INFO("Channel is over the index!");
		status = 0;
		return status;
	}
	
	value = 1000 * msg->data;				//У׼ֵ mA �� mV
	//�ж��Ƿ�Ϊ0ֵУ׼
	if(value == 0)
	{
		// valueֵΪ0,���ı�У׼ֵ
		LOG_INFO("Value is 0!");
		return status;
	}
//	value = msg->data;						//У׼ֵA��V

	// base_addr��UInt32 ,index�����Ŷ�Ӧ�Ĳ���ռ��8���ֽ�
	collectvalue = (float *)(ShareRegionAddr.base_addr + index*2);
	ShareAdjustaddr  = ShareRegionAddr.anadjust_addr;
	//����У׼����
	if(0 == collectvalue[0])
		ShareAdjustaddr[index] = 1;
	else
		ShareAdjustaddr[index] = (value * ShareAdjustaddr[index]) / collectvalue[0];

	//�ο�findindex[]���� ����index��Ӧ��ͨ����
	for(i=0;i<8;i++)
	{
		if(index == findindex[i])
		{
			channel = i;
			break;
		}
	}
	/* ���±���У׼ */
	adjust[channel] = ShareAdjustaddr[index];

	LOG_INFO("DigAdjust collectvalue is %d channel is %d, adjust is %d, collect is %d;",(int)collectvalue[0],index , (int)adjust[channel], (int)collectvalue[0]);
	return status;
}
/***************************************************************************/
//����: Int16 DigCmdout(App_Msg *msg)
//˵��: ң���������
//����: msg  ��õ�msg��Ϣ ����index ��cmd
//���: 0 �ɹ� 1 ʧ��
//�༭:
//ʱ��:2015.5.25
/***************************************************************************/
Int16 DigCmdout(App_Msg *msg)
{
	Int16 status = 0;
	SYSPARAMSTR *sysparm = (SYSPARAMSTR *)ShareRegionAddr.sysconf_addr;
	UInt32 *yxdata = (UInt32 *)ShareRegionAddr.digitIn_addr;
	static DigOutStr cmdout;

	// Զ��ʹ��
	if((yxdata[0]>>(JDYX-1)) & 0x01 == 0)
		return 0;
	
	switch(msg->data)
	{	
		case YK_OPEN_SEL:								//Ԥ�ô�ѡ��
			cmdout.DoutFlag = 1;
			cmdout.DoutIndex = msg->channel;			//Ԥ��ң��index
			YK_SendOut(0, PIN_LOW);					//���͵�ͨ
			break;
		case YK_CLOSE_SEL:								//Ԥ�ùر�ѡ��
			cmdout.DoutFlag = 1;
			cmdout.DoutIndex = msg->channel;			//Ԥ��ң��index
			YK_SendOut(0, PIN_LOW);					//���͵�ͨ
			break;
		case YK_CANCEL_OPEN_SEL:						//ȡ����ѡ��
			cmdout.DoutFlag = 0;
			cmdout.DoutIndex = msg->channel;			//Ԥ��ң��index
			YK_SendOut(0, PIN_HIGH);					//�ر�Ԥ�ÿ���
			break;
		case YK_CANCEL_CLOSE_SEL:						//ȡ���ر�ѡ��
			cmdout.DoutFlag = 0;
			cmdout.DoutIndex = msg->channel;			//Ԥ��ң��index
			YK_SendOut(0, PIN_HIGH);					//�ر�Ԥ�ÿ���
			break;
		case YK_OPEN_EXECUTIVE:							//ִ�д�
			cmdout.DoutIndex = msg->channel;			//ִ��YK���
			
			if(cmdout.DoutFlag)
			{
				if((cmdout.DoutIndex == 1) && ((yxstatus >> FWYX) & 0x01))
				{
					YK_SendOut(2, PIN_LOW);
					Task_sleep(sysparm->yc1_out);		//
//					Task_sleep(200);
					YK_SendOut(2, PIN_HIGH);
				}
				else if(cmdout.DoutIndex == 2)
				{
					YK_SendOut(4, PIN_LOW);
					Task_sleep(sysparm->yc2_out);		//
//					Task_sleep(200);
					YK_SendOut(4, PIN_HIGH);
				}
				else
				{
					status = 1;							//��������,δ������բ
				}
			}
			YK_SendOut(0, PIN_HIGH);					//�ر�Ԥ��ѡ��
			break;
		case YK_CLOSE_EXECUTIVE:						//ִ�йر�
			cmdout.DoutIndex = msg->channel;			//ִ��YK���
			
			if(cmdout.DoutFlag)
			{
				if((cmdout.DoutIndex == 1) && ((yxstatus >> HWYX) & 0x01))
				{
					YK_SendOut(1, PIN_LOW);
					Task_sleep(sysparm->yc1_out);
//					Task_sleep(200);				
					YK_SendOut(1, PIN_HIGH);
				}
				else if(cmdout.DoutIndex == 2)
				{
					YK_SendOut(3, PIN_LOW);
					Task_sleep(sysparm->yc2_out);
//					Task_sleep(200);
					YK_SendOut(3, PIN_HIGH);
				}
				else
				{
					status = 1;							//����,δ������բ
				}
			}
			YK_SendOut(0, PIN_HIGH);					//�ر�Ԥ��ѡ��
			break;
		default:
			break;
	}

	return status;
}
/***************************************************************************/
//����: Void InitAnaInAddr(void)
//˵��: ��ʼ������������ַָ��
//����: 
//���: 
//�༭:
//ʱ��:2015.5.25
/***************************************************************************/
Void InitAnaInAddr(void)
{
//	UChar i,j;

	//��ֵYC������ַ
//	ParListPtr = ShareRegionAddr.boardparlistptr;

//	memset((char *)ParListPtr, 0, 4*sizeof(ParameterListStr));
}
/***************************************************************************/
//����: Void Log_Printf(Void)
//˵��: ��ӡ������Ϣ
//����: ��
//���: ��
//�༭:
//ʱ��:2015.5.7
/***************************************************************************/
//Void Log_Printf(Void)
//{
//	App_Msg logmsg;

//	logmsg.cmd = MSG_PRINTF;
//	Message_Send(&logmsg);
//}
/***************************************************************************/
//����: Int ReadConfig(Void)
//˵��: ��ȡ���ֲ������������Ƿ���ȷ�������д洢������Ӧ�ò���ʱֱ�ӵ��ù���������
//����: ��
//���: 0 ���� -1 ����
//�༭:
//ʱ��:2015.8.7
/***************************************************************************/
Int ReadConfig(Void)
{
	UChar i;
	SYSPARAMSTR *syspamptr = (SYSPARAMSTR *)ShareRegionAddr.sysconf_addr;
	float *adjust = ShareRegionAddr.anadjust_addr;
	FAPARAMSTR *fapamptr = (FAPARAMSTR *)ShareRegionAddr.faconf_addr;

	dianbiaodata.yxen = *ShareRegionAddr.digitIn_EN_addr;
	dianbiaodata.yxcos = *ShareRegionAddr.digitIn_COS_addr;
	dianbiaodata.yxsoe = *ShareRegionAddr.digitIn_SOE_addr;
	dianbiaodata.yxnot = *ShareRegionAddr.digitIn_NOT_addr;

	// sys�����ļ�������ȡ
	/* ң�����ʱ�����ʱ */
	if(syspamptr->yc1_out < 20)
	{
		LOG_INFO("ReadConfig yc1_out is %x too low. ",syspamptr->yc1_out);
		syspamptr->yc1_out = 100;
		
	}
	if(syspamptr->yc2_out < 20)
	{
		LOG_INFO("ReadConfig yc2_out is %x too low. ",syspamptr->yc2_out);
		syspamptr->yc2_out = 100;
	}
	
	/* ����20������ index max=20 ������ҪУ׼��ֻ��8��ͨ�� */
	for(i=0;i<20;i++)
	{
		if(adjust[i] == 0.0)
		{	
			/* Ĭ��У׼ֵ*/
			switch(i)
			{
				case 0:
				case 1:
				case 2:
				adjust[i] = 5.2120;
				break;
				case 4:
				case 5:
				case 6:
				case 7:
				adjust[i] = 4.3359;
				case 17:
				adjust[i] = 2.9079;
				default:
				adjust[i] = 1;
			}
		}
	}
	//���ݾ���ֵ�������ʱʹ��
//	adjust[17] = 0.30205;
	/* У׼ֵ��ֵ ����indxֵ���� */
	for(i=0;i<8;i++)
	{
		AdjustValue[i] = *(adjust + findindex[i]);
	}
	/* fa�����ж��Ƿ���ȷ */
	/* �����غ�բ */
	if(fapamptr->reclose_n > 3)
	{
		fapamptr->reclose_n = 3;
		LOG_INFO("ReadConfig reclose_n is over value. ");
	}
	/* ���ι��� */
	if(fapamptr->cursection_n > 3)
	{
		fapamptr->cursection_n = 3;
		LOG_INFO("ReadConfig cursection_n is over value. ");
	}
//	for(i=0;i<fapamptr->cursection_n;i++)
//	{
//		// �����������A �����Ҫ����1000�� 
//		fapamptr->cursection[i].protectvalue *= 1000; 
//	}
	
	/* �������ι��� */
	if(fapamptr->zerosection_n > 3)
	{
		fapamptr->zerosection_n = 3;
		LOG_INFO("ReadConfig zerosection_n is over value. ");
	}
//	for(i=0;i<fapamptr->zerosection_n;i++)
//	{
//		// �����������A �����Ҫ����1000�� 
//		fapamptr->zerosection[i].protectvalue *= 1000; 
//	}
	
	/* Խ�ޱ������֧��8��ͨ����Խ�޼��(3U4I1DC) ���ڷ�Ϊ������ֵ���nֵ���Ϊ16 */
	if(fapamptr->ycover_n > 16)
	{
		fapamptr->ycover_n = 16;
		LOG_INFO("ReadConfig ycover_n is over value. ");
	}
///*	
//	LOG_INFO("ReadConfig fistreclose_t = %d, secondreclose_t = %d, thirdreclose_t = %d, softenable = %x",
//		fapamptr->fistreclose_t,fapamptr->secondreclose_t,fapamptr->thirdreclose_t,fapamptr->softenable);
//	
//	for(i=0;i<fapamptr->cursection_n;i++)
//	{
//		LOG_INFO("ReadConfig protectvalue = %d, delaytime = %d ",fapamptr->cursection[i].protectvalue,fapamptr->cursection[i].delaytime);
//	}
	for(i=0;i<fapamptr->ycover_n;i++)
	{
		LOG_INFO("ReadConfig ycindex = %d, flag = %d, value = %d ",fapamptr->ycover[i].ycindex,fapamptr->ycover[i].flag,fapamptr->ycover[i].value);
	}
//*/	
	return 0;
}
