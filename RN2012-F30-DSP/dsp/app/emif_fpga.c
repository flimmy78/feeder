/****************************************************************************/
/*
 * Copyright (c) 2015, 郑州瑞能电气 Runner
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
/* 添加的IPC模块头文件 */
#include <ti/ipc/Ipc.h>
#include <ti/ipc/MultiProc.h>
#include "EMIFAPinmuxSetup.h"
#include "emif_fpga.h"
#include "soc_C6748.h"			    // DSP C6748 外设寄存器
#include "emifa.h"					// EMIFA 头文件
#include "edma.h"

#include "mathlib.h" 				//DSP数学函数库
#include "dsplib.h"                 //DSP函数库
/* 配置外设需要用到的库 */
//#include "TL6748.h"
#include "hw_types.h"                // 宏命令
#include "hw_syscfg0_C6748.h"        // 系统配置模块寄存器
#include "soc_C6748.h"               // DSP C6748 外设寄存器
#include "psc.h"                     // 电源与睡眠控制宏及设备抽象层函数声明
#include "gpio.h"                    // 通用输入输出口宏及设备抽象层函数声明
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
unsigned int evtQ       = 0; 						// 使用的事件队列

volatile char *srcBuff;
volatile char *dstBuff;

//#pragma DATA_ALIGN(, 8);				      
Queue *AnaQueue;								  //遥测队列
UChar Logo[] = "RN1080";
volatile BaseStatu_Struct FPGAReadStatus;		   //FPGA状态信息
//volatile DIGRP_Struct DigInData;                  //开入量数据

DIGRP_Struct DigInData;
ANSMP_Stru ANSInData[8][64];             //遥测量原始数据 8*64 表示 8块AD 的64 组数据(8通道数据)
volatile ValidPartFlag ValidFlag;                 //数据上部分或下部分为最新数据(每次FPGA只采集32点的数据)
AnaQueueMessStru AnaQueueMess;					   //遥测队列信息 已准备好数据的通道
/***************************************************************************/
//函数:	Void EMIF_FGPA_Init(Void)
//说明:	EMIF CS2外设配置
//输入: 无
//输出: 无
//编辑:
//时间:2015.4.24
/***************************************************************************/
static Void EMIFA_FGPA_Init(Void)
{
	//Power on the EMIFA
	PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_EMIFA, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);

	/* 配置EMIFA相关复用引脚 */
	EMIFAPinMuxSetup();
	/* 配置数据总线16bit */
	EMIFAAsyncDevDataBusWidthSelect(SOC_EMIFA_0_REGS,EMIFA_CHIP_SELECT_2,
									EMIFA_DATA_BUSWITTH_16BIT);

	/* 选着Normal模式 */
	EMIFAAsyncDevOpModeSelect(SOC_EMIFA_0_REGS,EMIFA_CHIP_SELECT_2,
							   EMIFA_ASYNC_INTERFACE_NORMAL_MODE);

	/* 禁止WAIT引脚 */
	EMIFAExtendedWaitConfig(SOC_EMIFA_0_REGS,EMIFA_CHIP_SELECT_2,
							 EMIFA_EXTENDED_WAIT_DISABLE);

	/* 配置W_SETUP/R_SETUP   W_STROBE/R_STROBE    W_HOLD/R_HOLD	TA等参数 */
	EMIFAWaitTimingConfig(SOC_EMIFA_0_REGS,EMIFA_CHIP_SELECT_2,
//						   EMIFA_ASYNC_WAITTIME_CONFIG(0, 1, 0, 0, 1, 0, 0 ));
			 	 	 	   EMIFA_ASYNC_WAITTIME_CONFIG(1, 2, 1, 1, 2, 1, 0 ));
//						   EMIFA_ASYNC_WAITTIME_CONFIG(2, 3, 2, 2, 3, 2, 0 ));
}
/***************************************************************************/
//函数:	Void EMIFA_EDMA_Init()
//说明:	EMIF CS2外设配置
//输入: 无
//输出: 无
//编辑:
//时间:2015.4.24
/***************************************************************************/
static Void EMIFA_EDMA_Init()
{
}
/***************************************************************************/
//函数:	Void EMIFA_Task(UArg arg0, UArg arg1) 
//说明:	数据采集任务
//输入: arg0 arg1
//输出: 无
//编辑:
//时间:2015.4.24
/***************************************************************************/
Void EMIFA_Task(UArg arg0, UArg arg1) 
{
	Char status = 0;
	UChar anardytmp;
	UChar digrdytmp;
	DIGRP_Struct digintmp;						   //临时遥信量

	LOG_INFO("--> EMIFA_Task:");
	
	Init_GloalData();							   //初始化全局变量
	EMIFA_FGPA_Init();                           //初始化EMIFA
	EMIFA_EDMA_Init();							   //初始化EDMA3

	status = EMIFA_FPGA_Test();				   //测试FPGA数据读取是否正确
	if(status < 0)
	{
		//读取FPGA出错 暂不处理
		LOG_INFO("Test FPGA is err,EMIFA Task exit;");
		//临时屏蔽
		return ;
	}
	//初始化状态 
	EMIFA_ReadStatus((BaseStatu_Struct *)&FPGAReadStatus);
	anardytmp =  FPGAReadStatus.AnaRdy;
	digrdytmp = FPGAReadStatus.DinRdy;

	while(1)
	{
		EMIFA_ReadStatus((BaseStatu_Struct *)&FPGAReadStatus);

		//判断遥测状态是否改变
		digrdytmp ^= FPGAReadStatus.DinRdy;
		if(digrdytmp)
		{
			EMIFA_ReadDigIn((UInt16 *)&digintmp);
			//判断是否有开入变化
			if(DigInData.DIN1 ^ digintmp.DIN1 || DigInData.DIN2 ^ digintmp.DIN2 || DigInData.DIN3 ^ digintmp.DIN3)
			{
				//遥信数据放入共享区
				DigTurn2ShareRegion(&DigInData, &digintmp);	
				//更新数据
				DigInData = digintmp;
			}
		}
		//更新遥信状态数据
		digrdytmp = FPGAReadStatus.DinRdy;

		//判断遥测状态是否改变
		anardytmp ^= FPGAReadStatus.AnaRdy;
		if(anardytmp)
		{
			//读取遥测数据
			EMIFA_ReadAnaIn(FPGAReadStatus.AnaRdy);
			AnaQueueMess.AnaRdyStatus = anardytmp;
			//AD取上32组数据或下32组数据
			AnaQueueMess.Flag = *(UChar *)&ValidFlag;		
			//发送队列 传递遥测准备就绪组信息  这里用队列是为了防止数据更新影响数据处理部分
			queue_push(AnaQueue, (Queue_Elem *)&AnaQueueMess);
		}
		//更新遥测状态数据
		anardytmp = FPGAReadStatus.AnaRdy;
		
//		Task_sleep(10000);
	}
	
}
/***************************************************************************/
//函数:	Char EMIFA_FPGA_Test(Void)
//说明:	读取EMIF status 状态 测试FPGA是否正确
//输入: 无
//输出: status test成功与否 0 成功 -1 失败
//编辑:
//时间: 2015.4.24
/***************************************************************************/
static Char EMIFA_FPGA_Test(Void)
{
	UChar i;
	UChar len;
	Char status = 0;

	//除2 表示有多少16bit的数
	len = sizeof(BaseStatu_Struct) >> 1;  

	memset((char *)&FPGAReadStatus, 0 , sizeof(BaseStatu_Struct));

	//读取状态信息
	for(i=0;i<len;i++)                            
	{
		((UInt16 *)&FPGAReadStatus)[i] = ((UInt16 *)EMIF_BASEADDR)[i];
	}

	//读取Logo与默认设置不相等，FPGA版本出错
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
//函数:	Void EMIFA_ReadAnaIn(UChar analogNum)
//说明:	EMIFA 读取遥测量值
//输入: analogNum 模拟量准备就绪标志
//输出: 无
//编辑:
//时间:2015.4.25
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
				//无BOARD 准备好
				continue;
			case BOARD_1UP:
				//读取第一块板 上片AD数据
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
				//读取第一块板 下片AD数据
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
				//读取第二块板 上片AD数据
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
				//读取第二块板 下片AD数据
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
				//读取第三块板 上片AD数据
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
				//读取第三块板 上片AD数据
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
				//读取第四块板 上片AD数据
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
				//读取第四块板 下片AD数据
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
//函数:	Void EMIFA_ReadAnaInBoard(UInt32 baseaddr, UInt16* buffaddr)
//说明:	EMIF 读取遥测一个AD的8通道32组数据
//输入: baseaddr 读取EMIFA地址 buffaddr 数据存放缓存指针
//输出: 无
//编辑:
//时间:2015.4.25
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
			//交流电峰值为311=220*sqrtsp2  有效值为220
			buffaddr[i] = (Short)311*sinsp(PI*(i>>3)/32);		
		}
		else if(counter > 7 && counter < 16 )
		{
			//交流电峰值为311=220*sqrtsp2  有效值为220
			buffaddr[i] = (Short)311*sinsp(PI*(i>>3)/32 + PI);	
		}	
	}
	counter++;
	if(counter > 15)
		counter = 0;
#else
	for(i=0;i<256;i++)											
	{	
		//8*32 = 256 需要读取8通道的32组AD值 AD为16位 
		buffaddr[i] = ((Short *)baseaddr)[i];
	}
#endif

}
/***************************************************************************/
//函数:	Void EMIF_ReadStatus(BaseStatu_Struct *statusptr)
//说明:	EMIF 读取状态标志函数 遥控 遥信 准备就绪标志 继电器分合实时状态 时间
//输入: statusptr 状态结构体指针
//输出: 无
//编辑:
//时间:2015.4.24
/***************************************************************************/
Void EMIFA_ReadDigIn(UInt16* diginbuff)
{
	UChar i;
	UInt16 dignum = 0;
	UInt16 offsetaddr = 0;

	//根据调整 暂时不使用最新组号 dignum为自定义
//	dignum = ((UInt16 *)(EMIF_BASEADDR + OFFSET_NEWDIGRPNUM))[0];
	dignum = 0x01;
	
	//高8位暂时没有使用 低8位为最新遥信信息组号
	dignum &= 0x00FF;
	if(dignum > 32)
	{
		//越界 默认读取第一组数据
		dignum = 1;
	}
		
	offsetaddr = OFFSET_DIGRP_BASE + OFFSET_DIGRP_STEP*(dignum - 1);
	
	for(i=0;i<8;i++)
	{
#if DEBUG_NOFPGA
		static UInt16 tmp =0x01;
		//16位数据
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
//函数:	Void EMIF_ReadStatus(BaseStatu_Struct *statusptr)
//说明:	EMIF 读取状态标志函数 遥控 遥信 准备就绪标志 继电器分合实时状态 时间
//输入: statusptr 状态结构体指针
//输出: 无
//编辑:
//时间:2015.4.24
/***************************************************************************/
Void EMIFA_ReadStatus(BaseStatu_Struct *statusptr)
{
	UInt16 tmpdata;

	//读取状态数据
	tmpdata = ((UInt16 *)(EMIF_BASEADDR + OFFSET_DATAREADY))[0];
	statusptr->AnaRdy = tmpdata&0x00FF;
	statusptr->DinRdy = (tmpdata&0xFF00)>>8;
	//读取时间
	statusptr->Microsnd = ((UInt16 *)(EMIF_BASEADDR + OFFSET_MICROSND))[0];

	#if DEBUG_NOFPGA
		static UInt8 tmp = 0x01;
		if(0x80 == tmp)
		{
			tmp = 0x01;
			//一个循环产生一次遥信事件
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
//函数:	Void EMIFA_ReadDout(UInt16 *readoutbuff)
//说明:	EMIF 读取 继电器分合实时状态 此处是遥控的历史数据 用作反馈
//输入: readoutbuff 实时状态数组
//输出: 无
//编辑:
//时间:2015.4.24
/***************************************************************************/
Void EMIFA_ReadDout(UInt16 *readoutbuff)
{
	//读取继电器状态 Dout1 Dout2
	readoutbuff[0] = ((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRREADY1))[0];
	readoutbuff[1] = ((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRREADY1))[1];
	readoutbuff[2] = ((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRREADY2))[0];
	readoutbuff[3] = ((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRREADY2))[1];
}
/***************************************************************************/
//函数:	Void EMIFA_WriteCMD(DoutCMDStru *cmdout)
//说明:	EMIF 遥控继电器分合
//输入: cmdout 控制状态命令
//输出: 无
//编辑:
//时间:2015.4.24
/***************************************************************************/
Void EMIFA_WriteCMD(DoutCMDStru *cmdout)
{
	//写继电器状态 Dout1 Dout2
	((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRCMDNEW1))[0] = (UInt16)(cmdout->DoutCMD1 & 0xffff);
	((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRCMDNEW1))[1] = (UInt16)((cmdout->DoutCMD1 >> 16) & 0xffff);
	((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRCMDNEW1))[2] = (UInt16)(cmdout->DoutCMD2 & 0xffff);
	((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRCMDNEW1))[3] = (UInt16)((cmdout->DoutCMD2 >> 16) & 0xffff);
}
/***************************************************************************/
//函数:	Void EMIFA_WriteEnable(void)
//说明:	EMIF 使能输出控制
//输入: 无
//输出: 无
//编辑:
//时间:2015.4.29
/***************************************************************************/
Void EMIFA_WriteEnable(DoutCMDStru* cmdout)
{
#ifdef DEBUG_RESERVE
	DoutCMDStru cmdoutbuff;
	UInt32 *ptr = (UInt32 *)&cmdoutbuff;
	
	cmdoutbuff.Stuff   = 0xFFF1;					//65521 的整数倍
	cmdoutbuff.Stuff2  = 0xB;						//固定值1011 = 0xB  
	cmdoutbuff.RelayNo = 0xFF;						//使能控制输出
	cmdoutbuff.OnOff   = EMIF_CMDOUT_ON;			//闭合


	((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRCMD))[0] = (*ptr) & 0xffff;
	((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRCMD))[1] = ((*ptr) >> 16) & 0xffff;
#else
	cmdout->DoutFlag = 1;
#endif
	
}
/***************************************************************************/
//函数:	Void EMIFA_WriteDisable(void)
//说明:	EMIF 失能遥控输出控制命令
//输入: 无
//输出: 无
//编辑:
//时间:2015.4.29
/***************************************************************************/
Void EMIFA_WriteDisable(DoutCMDStru* cmdout)
{
#ifdef DEBUG_RESERVE
	DoutCMDStru cmdoutbuff;
	UInt32 *ptr = (UInt32 *)&cmdoutbuff;

	cmdoutbuff.Stuff   = 0xFFF1;					//65521 的整数倍
	cmdoutbuff.Stuff2  = 0xB;						//固定值1011 = 0xB  
	cmdoutbuff.RelayNo = 0x0;						//失能控制输出
	cmdoutbuff.OnOff   = EMIF_CMDOUT_OFF;			//打开

	((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRCMD))[0] = (*ptr) & 0xffff;
	((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRCMD))[1] = ((*ptr) >> 16) & 0xffff;
#else
	cmdout->DoutFlag = 0;
#endif
}
/***************************************************************************/
//函数:	UInt16 EMIFA_WriteDout(DoutCMDStru *cmdout)
//说明:	EMIF 遥控命令输出
//输入: cmdout 输出cmd结构体指针 包含输出点号与状态信息
//输出: status 0 成功 1 失败
//编辑:
//时间:2015.4.29
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
	
	cmdoutbuff.Stuff   = 0xFFF1;					//65521 的整数倍
	cmdoutbuff.Stuff2  = 0xB;						//固定值1011 = 0xB  
	cmdoutbuff.RelayNo = cmdout->RelayNo;
	cmdoutbuff.OnOff   = cmdout->OnOff ;

	((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRCMD))[0] = (*ptr) & 0xffff;
	((UInt16 *)(EMIF_BASEADDR + OFFSET_CONTRCMD))[1] = ((*ptr) >> 16) & 0xffff;
	
	EMIFA_ReadDout(outstatus);
	
	if(cmdout->RelayNo <= 16)						//点号为1-8 
	{
		step = (cmdout->RelayNo - 1)<<1;
		tmp = *(UInt32 *)outstatus;
		tmp = (tmp >> step)&0x0003;
		if(tmp ^ cmdout->OnOff)
			status = 1;							   //错误
	}
	else if(cmdout->RelayNo >= 17 && cmdout->RelayNo <= 32 )	//18 =2*9 32 = 2*16 及遥控点号为9-16
	{
		step = (cmdout->RelayNo - 17)<<1;
		tmp = *(UInt32 *)(outstatus+2);
		tmp = (tmp >> step )&0x0003;
		if(tmp ^ cmdout->OnOff)
			status = 1;							   //错误
	}

	return status;
}
#endif
/***************************************************************************/
//函数:	UInt16 EMIFA_WriteCMDDout(DoutCMDStru *cmdout)
//说明:	EMIF 遥控命令输出
//输入: cmdout 输出cmd结构体指针 包含输出点号与状态信息
//输出: status 0 成功 1 失败
//编辑:
//时间:2015.4.29
/***************************************************************************/
UInt16 EMIFA_WriteCMDDout(DoutCMDStru *cmdout)
{
	UChar i;
	UInt16 status = 0;
	UInt16 outstatus[4] = {0}; 
	UInt32 outcmd = cmdout->DoutCMD2;
	UInt8 index = (UInt8)cmdout->DoutCMD1;

	//读取当前遥控输出状态
	EMIFA_ReadDout(outstatus);

	if(cmdout->DoutFlag)
	{
		cmdout->DoutCMD1 = ((UInt32 *)outstatus)[0];
		cmdout->DoutCMD2 = ((UInt32 *)outstatus)[1];
		//点号为1-8 
		if(index <= 16 )					
		{
			index = (index- 1)<<1;
			cmdout->DoutCMD1 &= ~(0x03 << index);
			cmdout->DoutCMD1 |= outcmd << index;
		}
		//18 =2*9 32 = 2*16 及遥控点号为9-16
		else if(index >= 17 && index <= 32)
		{
			index = (index- 17)<<1;
			cmdout->DoutCMD2 &= ~(0x03 << index);
			cmdout->DoutCMD2 |= outcmd << index;
		}
		//输出控制命令
		EMIFA_WriteCMD(cmdout);
		//延时等待FPGA
		Task_sleep(100);
		//重复4次
		for(i=0;i<4;i++)
		{
			EMIFA_ReadDout(outstatus);
			if((cmdout->DoutCMD1 ^ ((UInt32*)outstatus)[0])|(cmdout->DoutCMD2 ^ ((UInt32*)outstatus)[1]))
			{
				EMIFA_WriteCMD(cmdout);
				Task_sleep(100);
			}	
		}
		//遥控失败
		if(i >= 4)
			status = 1;
			
	}
	
	return status;
}
/***************************************************************************/
//函数:	Void EMIF_ReadStatus(BaseStatu_Struct *statusptr)
//说明:	EMIF 读取 继电器分合实时状态 此处是遥控的历史数据 用作反馈
//输入: statusptr 状态结构体指针
//输出: 无
//编辑:
//时间:2015.4.24
/***************************************************************************/
static Void Init_GloalData(Void)
{
	
	*(UChar *)&ValidFlag = 0;
	memset((char *)&FPGAReadStatus, 0, sizeof(FPGAReadStatus));		//初始化fpga status buff
	memset((char *)&DigInData, 0, sizeof(DigInData));		            //初始化遥信量 buff
	memset((char *)ANSInData, 0, sizeof(ANSInData));		            //初始化遥测量 buff

	//测试添加
	extern float AdjustValue[8][7];									//初始化校准值
	UChar i,j;
	//添加测试校准值
	for(i=0;i<8;i++)
		for(j=0;j<7;j++)
			AdjustValue[i][j] = 1;
}
