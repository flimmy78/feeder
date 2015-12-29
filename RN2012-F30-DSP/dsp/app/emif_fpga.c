/****************************************************************************/
/*
 * Copyright (c) 2015, ֣�����ܵ��� Runner
 * All rights reserved.
 * file       : emif_fgpa.c
 * Design     : ZLB
 * Data       : 2015-4-24
 * Version    : V1.0
 * Change     :
 */
/****************************************************************************/
#include <string.h>
/* xdctools header files */
#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/knl/Event.h>
/* ��ӵ�IPCģ��ͷ�ļ� */
#include <ti/ipc/Ipc.h>
#include <ti/ipc/MultiProc.h>
#include "EMIFAPinmuxSetup.h"
#include "emif_fpga.h"
#include "soc_C6748.h"			    // DSP C6748 ����Ĵ���
#include "emifa.h"					// EMIFA ͷ�ļ�
#include "edma.h"

#include "mathlib.h" 				//DSP��ѧ������
#include "dsplib.h"                 //DSP������
/* ����������Ҫ�õ��Ŀ� */
//#include "TL6748.h"
#include "hw_types.h"                // ������
#include "hw_syscfg0_C6748.h"        // ϵͳ����ģ��Ĵ���
#include "soc_C6748.h"               // DSP C6748 ����Ĵ���
#include "psc.h"                     // ��Դ��˯�߿��ƺ꼰�豸����㺯������
#include "gpio.h"                    // ͨ����������ں꼰�豸����㺯������
#include "queue.h"
#include "IPCServer.h"
#include "communicate.h"
#include "fft.h"
#include "log.h"

/****************************** macro define ************************************/
//#define PI                 3.1415
/****************************** type define ************************************/

/****************************** module structure ********************************/

/******************************* functions **************************************/
static Void EMIFA_FGPA_Init(Void);
static Void Init_GloalData(Void);
static Char EMIFA_FPGA_Test(Void);
/******************************* private data ***********************************/
unsigned int chType     = EDMA3_CHANNEL_TYPE_DMA;  //for DMA channel
unsigned int chNum      = 0;   					    //channel num
unsigned int tccNum     = 0;  						//
unsigned int edmaTC     = 0;
unsigned int syncType   = EDMA3_SYNC_A;  			//A mode or AB mode
unsigned int trigMode   = EDMA3_TRIG_MODE_MANUAL;
unsigned int evtQ       = 0; 						// ʹ�õ��¼�����

volatile char *srcBuff;
volatile char *dstBuff;

//#pragma DATA_ALIGN(, 8);				      
Queue *AnaQueue;								  //ң�����
UChar Logo[] = "RN1080";
volatile BaseStatu_Struct FPGAReadStatus;		   //FPGA״̬��Ϣ
//volatile DIGRP_Struct DigInData;                  //����������

DIGRP_Struct DigInData;
ANSMP_Stru ANSInData[8][64];             //ң����ԭʼ���� 8*64 ��ʾ 8��AD ��64 ������(8ͨ������)
volatile ValidPartFlag ValidFlag;                 //�����ϲ��ֻ��²���Ϊ��������(ÿ��FPGAֻ�ɼ�32�������)
AnaQueueMessStru AnaQueueMess;					   //ң�������Ϣ ��׼�������ݵ�ͨ��
/***************************************************************************/
//����:	Void EMIF_FGPA_Init(Void)
//˵��:	EMIF CS2��������
//����: ��
//���: ��
//�༭:
//ʱ��:2015.4.24
/***************************************************************************/
static Void EMIFA_FGPA_Init(Void)
{
	//Power on the EMIFA
	PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_EMIFA, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);

	/* ����EMIFA��ظ������� */
	EMIFAPinMuxSetup();
	/* ������������16bit */
	EMIFAAsyncDevDataBusWidthSelect(SOC_EMIFA_0_REGS,EMIFA_CHIP_SELECT_2,
									EMIFA_DATA_BUSWITTH_16BIT);

	/* ѡ��Normalģʽ */
	EMIFAAsyncDevOpModeSelect(SOC_EMIFA_0_REGS,EMIFA_CHIP_SELECT_2,
							   EMIFA_ASYNC_INTERFACE_NORMAL_MODE);

	/* ��ֹWAIT���� */
	EMIFAExtendedWaitConfig(SOC_EMIFA_0_REGS,EMIFA_CHIP_SELECT_2,
							 EMIFA_EXTENDED_WAIT_DISABLE);

	/* ����W_SETUP/R_SETUP   W_STROBE/R_STROBE    W_HOLD/R_HOLD	TA�Ȳ��� */
	EMIFAWaitTimingConfig(SOC_EMIFA_0_REGS,EMIFA_CHIP_SELECT_2,
//						   EMIFA_ASYNC_WAITTIME_CONFIG(0, 1, 0, 0, 1, 0, 0 ));
			 	 	 	   EMIFA_ASYNC_WAITTIME_CONFIG(1, 2, 1, 1, 2, 1, 0 ));
//						   EMIFA_ASYNC_WAITTIME_CONFIG(2, 3, 2, 2, 3, 2, 0 ));
}
/***************************************************************************/
//����:	Void EMIFA_EDMA_Init()
//˵��:	EMIF CS2��������
//����: ��
//���: ��
//�༭:
//ʱ��:2015.4.24
/***************************************************************************/
static Void EMIFA_EDMA_Init()
{
}
/***************************************************************************/
//����:	Void EMIFA_Task(UArg arg0, UArg arg1) 
//˵��:	���ݲɼ�����
//����: arg0 arg1
//���: ��
//�༭:
//ʱ��:2015.4.24
/***************************************************************************/
Void EMIFA_Task(UArg arg0, UArg arg1) 
{
	Char status = 0;
	UChar anardytmp;
	UChar digrdytmp;
	DIGRP_Struct digintmp;						   //��ʱң����

	LOG_INFO("--> EMIFA_Task:");
	
	Init_GloalData();							   //��ʼ��ȫ�ֱ���
	EMIFA_FGPA_Init();                           //��ʼ��EMIFA
	EMIFA_EDMA_Init();							   //��ʼ��EDMA3

	status = EMIFA_FPGA_Test();				   //����FPGA���ݶ�ȡ�Ƿ���ȷ
	if(status < 0)
	{
		//��ȡFPGA���� �ݲ�����
		LOG_INFO("Test FPGA is err,EMIFA Task exit;");
		//��ʱ����
		return ;
	}
	//��ʼ��״̬ 
	EMIFA_ReadStatus((BaseStatu_Struct *)&FPGAReadStatus);
	anardytmp =  FPGAReadStatus.AnaRdy;
	digrdytmp = FPGAReadStatus.DinRdy;

	while(1)
	{
		EMIFA_ReadStatus((BaseStatu_Struct *)&FPGAReadStatus);

		//�ж�ң��״̬�Ƿ�ı�
		digrdytmp ^= FPGAReadStatus.DinRdy;
		if(digrdytmp)
		{
			EMIFA_ReadDigIn((UInt16 *)&digintmp);
			//�ж��Ƿ��п���仯
			if(DigInData.DIN1 ^ digintmp.DIN1 || DigInData.DIN2 ^ digintmp.DIN2 || DigInData.DIN3 ^ digintmp.DIN3)
			{
				//ң�����ݷ��빲����
				DigTurn2ShareRegion(&DigInData, &digintmp);	
				//��������
				DigInData = digintmp;
			}
		}
		//����ң��״̬����
		digrdytmp = FPGAReadStatus.DinRdy;

		//�ж�ң��״̬�Ƿ�ı�
		anardytmp ^= FPGAReadStatus.AnaRdy;
		if(anardytmp)
		{
			//��ȡң������
			EMIFA_ReadAnaIn(FPGAReadStatus.AnaRdy);
			AnaQueueMess.AnaRdyStatus = anardytmp;
			//ADȡ��32�����ݻ���32������
			AnaQueueMess.Flag = *(UChar *)&ValidFlag;		
			//���Ͷ��� ����ң��׼����������Ϣ  �����ö�����Ϊ�˷�ֹ���ݸ���Ӱ�����ݴ�����
			queue_push(AnaQueue, (Queue_Elem *)&AnaQueueMess);
		}
		//����ң��״̬����
		anardytmp = FPGAReadStatus.AnaRdy;
		
//		Task_sleep(10000);
	}
	
}
/***************************************************************************/
//����:	Char EMIFA_FPGA_Test(Void)
//˵��:	��ȡEMIF status ״̬ ����FPGA�Ƿ���ȷ
//����: ��
//���: status test�ɹ���� 0 �ɹ� -1 ʧ��
//�༭:
//ʱ��: 2015.4.24
/***************************************************************************/
static Char EMIFA_FPGA_Test(Void)
{
	UChar i;
	UChar len;
	Char status = 0;

	//��2 ��ʾ�ж���16bit����
	len = sizeof(BaseStatu_Struct) >> 1;  

	memset((char *)&FPGAReadStatus, 0 , sizeof(BaseStatu_Struct));

	//��ȡ״̬��Ϣ
	for(i=0;i<len;i++)                            
	{
		((UInt16 *)&FPGAReadStatus)[i] = ((UInt16 *)EMIF_BASEADDR)[i];
	}

	//��ȡLogo��Ĭ�����ò���ȣ�FPGA�汾����
	for(i=0;i<6;i++)
	{
		if(Logo[i] != FPGAReadStatus.VersionID[i])
		{
			status = -1;
			return status;
		}
	}

//	LOG_INFO("Creat EMIFA_FPGA_Test is ok;\n");
	return status; 
}
/***************************************************************************/
//����:	Void EMIFA_ReadAnaIn(UChar analogNum)
//˵��:	EMIFA ��ȡң����ֵ
//����: analogNum ģ����׼��������־
//���: ��
//�༭:
//ʱ��:2015.4.25
/***************************************************************************/
Void EMIFA_ReadAnaIn(UChar analogNum)
{
	UChar i;
    UChar casenum = 0;

	for(i=0;i<8;i++)
	{
		casenum = analogNum &(0x01 << i);
		switch(casenum)
		{	
			case NONEBOARD:
				//��BOARD ׼����
				continue;
			case BOARD_1UP:
				//��ȡ��һ��� ��ƬAD����
				if(ValidFlag.board_1u)
				{
					ValidFlag.board_1u  = 0;
					EMIFA_ReadAnaInBoard( EMIF_BASEADDR + OFFSET_ANGRP1, (Short *)&ANSInData[0][32]);
				}
				else
				{
					ValidFlag.board_1u  = 1;
					EMIFA_ReadAnaInBoard( EMIF_BASEADDR + OFFSET_ANGRP1, (Short *)&ANSInData[0][0]);
				}
				break;
			case BOARD_1DOWN:
				//��ȡ��һ��� ��ƬAD����
				if(ValidFlag.board_1d)
				{
					ValidFlag.board_1d  = 0;
					EMIFA_ReadAnaInBoard( EMIF_BASEADDR + OFFSET_ANGRP2, (Short *)&ANSInData[1][32]);
				}
				else
				{
					ValidFlag.board_1d  = 1;
					EMIFA_ReadAnaInBoard( EMIF_BASEADDR + OFFSET_ANGRP2, (Short *)&ANSInData[1][0]);
				}
				break;
			case BOARD_2UP:
				//��ȡ�ڶ���� ��ƬAD����
				if(ValidFlag.board_2u)
				{
					ValidFlag.board_2u = 0;
					EMIFA_ReadAnaInBoard( EMIF_BASEADDR + OFFSET_ANGRP3, (Short *)&ANSInData[2][32]);
				}
				else
				{
					ValidFlag.board_2u = 1;
					EMIFA_ReadAnaInBoard( EMIF_BASEADDR + OFFSET_ANGRP3, (Short *)&ANSInData[2][0]);
				}
				break;
			case BOARD_2DOWN:
				//��ȡ�ڶ���� ��ƬAD����
				if(ValidFlag.board_2d)
				{
					ValidFlag.board_2d = 0;
					EMIFA_ReadAnaInBoard( EMIF_BASEADDR + OFFSET_ANGRP4, (Short *)&ANSInData[3][32]);
				}
				else
				{
					ValidFlag.board_2d = 1;
					EMIFA_ReadAnaInBoard( EMIF_BASEADDR + OFFSET_ANGRP4, (Short *)&ANSInData[3][0]);
				}
				break;
			case BOARD_3UP:
				//��ȡ������� ��ƬAD����
				if(ValidFlag.board_3u)
				{
					ValidFlag.board_3u= 0;
					EMIFA_ReadAnaInBoard( EMIF_BASEADDR + OFFSET_ANGRP5, (Short *)&ANSInData[4][32]);
				}
				else
				{
					ValidFlag.board_3u= 1;
					EMIFA_ReadAnaInBoard( EMIF_BASEADDR + OFFSET_ANGRP5, (Short *)&ANSInData[4][0]);
				}
				break;
			case BOARD_3DOWN:
				//��ȡ������� ��ƬAD����
				if(ValidFlag.board_3d)
				{
					ValidFlag.board_3d= 0;
					EMIFA_ReadAnaInBoard( EMIF_BASEADDR + OFFSET_ANGRP6, (Short *)&ANSInData[5][32]);
				}
				else
				{
					ValidFlag.board_3d= 1;
					EMIFA_ReadAnaInBoard( EMIF_BASEADDR + OFFSET_ANGRP6, (Short *)&ANSInData[5][0]);
				}
				break;
			case BOARD_4UP:
				//��ȡ���Ŀ�� ��ƬAD����
				if(ValidFlag.board_4u)
				{
					ValidFlag.board_4u= 0;
					EMIFA_ReadAnaInBoard( EMIF_BASEADDR + OFFSET_ANGRP7, (Short *)&ANSInData[6][32]);
				}
				else
				{
					ValidFlag.board_4u= 1;
					EMIFA_ReadAnaInBoard( EMIF_BASEADDR + OFFSET_ANGRP7, (Short *)&ANSInData[6][0]);
				}
				break;
			case BOARD_4DOWN:
				//��ȡ���Ŀ�� ��ƬAD����
				if(ValidFlag.board_4d)
				{
					ValidFlag.board_4d = 0;
					EMIFA_ReadAnaInBoard( EMIF_BASEADDR + OFFSET_ANGRP8, (Short *)&ANSInData[7][32]);
				}
				else
				{
					ValidFlag.board_4d = 1;
					EMIFA_ReadAnaInBoard( EMIF_BASEADDR + OFFSET_ANGRP8, (Short *)&ANSInData[7][0]);
				}
				break;
			default:
				break;
		}
	}
}
/***************************************************************************/
//����:	Void EMIFA_ReadAnaInBoard(UInt32 baseaddr, UInt16* buffaddr)
//˵��:	EMIF ��ȡң��һ��AD��8ͨ��32������
//����: baseaddr ��ȡEMIFA��ַ buffaddr ���ݴ�Ż���ָ��
//���: ��
//�༭:
//ʱ��:2015.4.25
/***************************************************************************/
Void EMIFA_ReadAnaInBoard(UInt32 baseaddr, Short* buffaddr)
{
	UInt16 i;
#if DEBUG_NOFPGA
	static UInt8 counter = 0;
	for(i=0;i<256;i++)
	{
		if(counter < 8)
		{
			//�������ֵΪ311=220*sqrtsp2  ��ЧֵΪ220
			buffaddr[i] = (Short)311*sinsp(PI*(i>>3)/32);		
		}
		else if(counter > 7 && counter < 16 )
		{
			//�������ֵΪ311=220*sqrtsp2  ��ЧֵΪ220
			buffaddr[i] = (Short)311*sinsp(PI*(i>>3)/32 + PI);	
		}	
	}
	counter++;
	if(counter > 15)
		counter = 0;
#else
	for(i=0;i<256;i++)											
	{	
		//8*32 = 256 ��Ҫ��ȡ8ͨ����32��ADֵ ADΪ16λ 
		buffaddr[i] = ((Short *)baseaddr)[i];
	}
#endif

}
/***************************************************************************/
//����:	Void EMIF_ReadStatus(BaseStatu_Struct *statusptr)
//˵��:	EMIF ��ȡ״̬��־���� ң�� ң�� ׼��������־ �̵����ֺ�ʵʱ״̬ ʱ��
//����: statusptr ״̬�ṹ��ָ��
//���: ��
//�༭:
//ʱ��:2015.4.24
/***************************************************************************/
Void EMIFA_ReadDigIn(UInt16* diginbuff)
{
	UChar i;
	UInt16 dignum = 0;
	UInt16 offsetaddr = 0;

	//���ݵ��� ��ʱ��ʹ��������� dignumΪ�Զ���
//	dignum = ((UInt16 *)(EMIF_BASEADDR + OFFSET_NEWDIGRPNUM))[0];
	dignum = 0x01;
	
	//��8λ��ʱû��ʹ�� ��8λΪ����ң����Ϣ���
	dignum &= 0x00FF;
	if(dignum > 32)
	{
		//Խ�� Ĭ�϶�ȡ��һ������
		dignum = 1;
	}
		
	offsetaddr = OFFSET_DIGRP_BASE + OFFSET_DIGRP_STEP*(dignum - 1);
	
	for(i=0;i<8;i++)
	{
#if DEBUG_NOFPGA
		static UInt16 tmp =0x01;
		//16λ����
		if(tmp == 0x8000)
			tmp = 0x01;
		else
			tmp <<= 1;
		
		diginbuff[i] = tmp; 
#else
		diginbuff[i] = ((UInt16 *)(EMIF_BASEADDR + offsetaddr))[i];		
#endif
	}
}
/***************************************************************************/
//����:	Void EMIF_ReadStatus(BaseStatu_Struct *statusptr)
//˵��:	EMIF ��ȡ״̬��־���� ң�� ң�� ׼��������־ �̵����ֺ�ʵʱ״̬ ʱ��
//����: statusptr ״̬�ṹ��ָ��
//���: ��
//�༭:
//ʱ��:2015.4.24
/***************************************************************************/
Void EMIFA_ReadStatus(BaseStatu_Struct *statusptr)
{
	UInt16 tmpdata;

	//��ȡ״̬����
	tmpdata = ((UInt16 *)(EMIF_BASEADDR + OFFSET_DATAREADY))[0];
	statusptr->AnaRdy = tmpdata&0x00FF;
	statusptr->DinRdy = (tmpdata&0xFF00)>>8;
	//��ȡʱ��
	statusptr->Microsnd = ((UInt16 *)(EMIF_BASEADDR + OFFSET_MICROSND))[0];

	#if DEBUG_NOFPGA
		static UInt8 tmp = 0x01;
		if(0x80 == tmp)
		{
			tmp = 0x01;
			//һ��ѭ������һ��ң���¼�
			statusptr->DinRdy = 1;	
		}
		else
		{
			tmp <<= 1;
			statusptr->DinRdy = 0;
		}
		statusptr->AnaRdy = tmp;
	#else
		
	#endif

}
/***************************************************************************/
//����:	Void EMIFA_ReadDout(UInt16 *readoutbuff)
//˵��:	EMIF ��ȡ �̵����ֺ�ʵʱ״̬ �˴���ң�ص���ʷ���� ��������
//����: readoutbuff ʵʱ״̬����
//���: ��
//�༭:
//ʱ��:2015.4.24
/***************************************************************************/
Void EMIFA_ReadDout(UInt16 *readoutbuff)
{
	//��ȡ�̵���״̬ Dout1 Dout2
	readoutbuff[0] = ((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRREADY1))[0];
	readoutbuff[1] = ((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRREADY1))[1];
	readoutbuff[2] = ((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRREADY2))[0];
	readoutbuff[3] = ((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRREADY2))[1];
}
/***************************************************************************/
//����:	Void EMIFA_WriteCMD(DoutCMDStru *cmdout)
//˵��:	EMIF ң�ؼ̵����ֺ�
//����: cmdout ����״̬����
//���: ��
//�༭:
//ʱ��:2015.4.24
/***************************************************************************/
Void EMIFA_WriteCMD(DoutCMDStru *cmdout)
{
	//д�̵���״̬ Dout1 Dout2
	((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRCMDNEW1))[0] = (UInt16)(cmdout->DoutCMD1 & 0xffff);
	((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRCMDNEW1))[1] = (UInt16)((cmdout->DoutCMD1 >> 16) & 0xffff);
	((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRCMDNEW1))[2] = (UInt16)(cmdout->DoutCMD2 & 0xffff);
	((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRCMDNEW1))[3] = (UInt16)((cmdout->DoutCMD2 >> 16) & 0xffff);
}
/***************************************************************************/
//����:	Void EMIFA_WriteEnable(void)
//˵��:	EMIF ʹ���������
//����: ��
//���: ��
//�༭:
//ʱ��:2015.4.29
/***************************************************************************/
Void EMIFA_WriteEnable(DoutCMDStru* cmdout)
{
#ifdef DEBUG_RESERVE
	DoutCMDStru cmdoutbuff;
	UInt32 *ptr = (UInt32 *)&cmdoutbuff;
	
	cmdoutbuff.Stuff   = 0xFFF1;					//65521 ��������
	cmdoutbuff.Stuff2  = 0xB;						//�̶�ֵ1011 = 0xB  
	cmdoutbuff.RelayNo = 0xFF;						//ʹ�ܿ������
	cmdoutbuff.OnOff   = EMIF_CMDOUT_ON;			//�պ�


	((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRCMD))[0] = (*ptr) & 0xffff;
	((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRCMD))[1] = ((*ptr) >> 16) & 0xffff;
#else
	cmdout->DoutFlag = 1;
#endif
	
}
/***************************************************************************/
//����:	Void EMIFA_WriteDisable(void)
//˵��:	EMIF ʧ��ң�������������
//����: ��
//���: ��
//�༭:
//ʱ��:2015.4.29
/***************************************************************************/
Void EMIFA_WriteDisable(DoutCMDStru* cmdout)
{
#ifdef DEBUG_RESERVE
	DoutCMDStru cmdoutbuff;
	UInt32 *ptr = (UInt32 *)&cmdoutbuff;

	cmdoutbuff.Stuff   = 0xFFF1;					//65521 ��������
	cmdoutbuff.Stuff2  = 0xB;						//�̶�ֵ1011 = 0xB  
	cmdoutbuff.RelayNo = 0x0;						//ʧ�ܿ������
	cmdoutbuff.OnOff   = EMIF_CMDOUT_OFF;			//��

	((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRCMD))[0] = (*ptr) & 0xffff;
	((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRCMD))[1] = ((*ptr) >> 16) & 0xffff;
#else
	cmdout->DoutFlag = 0;
#endif
}
/***************************************************************************/
//����:	UInt16 EMIFA_WriteDout(DoutCMDStru *cmdout)
//˵��:	EMIF ң���������
//����: cmdout ���cmd�ṹ��ָ�� ������������״̬��Ϣ
//���: status 0 �ɹ� 1 ʧ��
//�༭:
//ʱ��:2015.4.29
/***************************************************************************/
#ifdef DEBUG_RESERVE
UInt16 EMIFA_WriteDout(DoutCMDStru *cmdout)
{
	UInt16 status = 0;
	UInt16 outstatus[4] = {0}; 
	UInt32 tmp;
	UInt16 step;
	DoutCMDStru cmdoutbuff;
	UInt32 *ptr = (UInt32 *)&cmdoutbuff;
	
	cmdoutbuff.Stuff   = 0xFFF1;					//65521 ��������
	cmdoutbuff.Stuff2  = 0xB;						//�̶�ֵ1011 = 0xB  
	cmdoutbuff.RelayNo = cmdout->RelayNo;
	cmdoutbuff.OnOff   = cmdout->OnOff ;

	((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRCMD))[0] = (*ptr) & 0xffff;
	((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRCMD))[1] = ((*ptr) >> 16) & 0xffff;
	
	EMIFA_ReadDout(outstatus);
	
	if(cmdout->RelayNo <= 16)						//���Ϊ1-8 
	{
		step = (cmdout->RelayNo - 1)<<1;
		tmp = *(UInt32 *)outstatus;
		tmp = (tmp >> step)&0x0003;
		if(tmp ^ cmdout->OnOff)
			status = 1;							   //����
	}
	else if(cmdout->RelayNo >= 17 && cmdout->RelayNo <= 32 )	//18 =2*9 32 = 2*16 ��ң�ص��Ϊ9-16
	{
		step = (cmdout->RelayNo - 17)<<1;
		tmp = *(UInt32 *)(outstatus+2);
		tmp = (tmp >> step )&0x0003;
		if(tmp ^ cmdout->OnOff)
			status = 1;							   //����
	}

	return status;
}
#endif
/***************************************************************************/
//����:	UInt16 EMIFA_WriteCMDDout(DoutCMDStru *cmdout)
//˵��:	EMIF ң���������
//����: cmdout ���cmd�ṹ��ָ�� ������������״̬��Ϣ
//���: status 0 �ɹ� 1 ʧ��
//�༭:
//ʱ��:2015.4.29
/***************************************************************************/
UInt16 EMIFA_WriteCMDDout(DoutCMDStru *cmdout)
{
	UChar i;
	UInt16 status = 0;
	UInt16 outstatus[4] = {0}; 
	UInt32 outcmd = cmdout->DoutCMD2;
	UInt8 index = (UInt8)cmdout->DoutCMD1;

	//��ȡ��ǰң�����״̬
	EMIFA_ReadDout(outstatus);

	if(cmdout->DoutFlag)
	{
		cmdout->DoutCMD1 = ((UInt32 *)outstatus)[0];
		cmdout->DoutCMD2 = ((UInt32 *)outstatus)[1];
		//���Ϊ1-8 
		if(index <= 16 )					
		{
			index = (index- 1)<<1;
			cmdout->DoutCMD1 &= ~(0x03 << index);
			cmdout->DoutCMD1 |= outcmd << index;
		}
		//18 =2*9 32 = 2*16 ��ң�ص��Ϊ9-16
		else if(index >= 17 && index <= 32)
		{
			index = (index- 17)<<1;
			cmdout->DoutCMD2 &= ~(0x03 << index);
			cmdout->DoutCMD2 |= outcmd << index;
		}
		//�����������
		EMIFA_WriteCMD(cmdout);
		//��ʱ�ȴ�FPGA
		Task_sleep(100);
		//�ظ�4��
		for(i=0;i<4;i++)
		{
			EMIFA_ReadDout(outstatus);
			if((cmdout->DoutCMD1 ^ ((UInt32*)outstatus)[0])|(cmdout->DoutCMD2 ^ ((UInt32*)outstatus)[1]))
			{
				EMIFA_WriteCMD(cmdout);
				Task_sleep(100);
			}	
		}
		//ң��ʧ��
		if(i >= 4)
			status = 1;
			
	}
	
	return status;
}
/***************************************************************************/
//����:	Void EMIF_ReadStatus(BaseStatu_Struct *statusptr)
//˵��:	EMIF ��ȡ �̵����ֺ�ʵʱ״̬ �˴���ң�ص���ʷ���� ��������
//����: statusptr ״̬�ṹ��ָ��
//���: ��
//�༭:
//ʱ��:2015.4.24
/***************************************************************************/
static Void Init_GloalData(Void)
{
	
	*(UChar *)&ValidFlag = 0;
	memset((char *)&FPGAReadStatus, 0, sizeof(FPGAReadStatus));		//��ʼ��fpga status buff
	memset((char *)&DigInData, 0, sizeof(DigInData));		            //��ʼ��ң���� buff
	memset((char *)ANSInData, 0, sizeof(ANSInData));		            //��ʼ��ң���� buff

	//�������
	extern float AdjustValue[8][7];									//��ʼ��У׼ֵ
	UChar i,j;
	//��Ӳ���У׼ֵ
	for(i=0;i<8;i++)
		for(j=0;j<7;j++)
			AdjustValue[i][j] = 1;
}
