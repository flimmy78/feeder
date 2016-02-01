/*************************************************************************/
/*
 * Copyright (c) 2015, 郑州瑞能电气 Runner
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
DianBiaoStr dianbiaodata;			//点表数据
yz_data yzdata;						//预置数据
//ParameterListStr* ParListPtr;		//power value 所有遥测数据回路参数地址指针
//查找通道对应的index值 0xff 表示不存在此通道对应index
UInt16 findindex[8] = {0,1,2,4,5,6,7,17};
/***************************************************************************/
//函数:	Void Init_ShareAddr(UInt32* ptr)
//说明: 初始化共享地址
//输入: ptr 共享区基地址
//输出: 无
//编辑:
//时间:2015.5.6
/***************************************************************************/
Void Init_ShareAddr(UChar* ptr)
{
	ShareRegionAddr.base_addr = (UInt32 *)ptr;										// 共享区基地址
	ShareRegionAddr.ykconf_addr = (UInt32 *)(ptr + YK_OFFSET);						// 遥控配置地址
	ShareRegionAddr.sysconf_addr = (UInt32 *)(ptr + SYS_OFFSET);						// 系统配置地址
	ShareRegionAddr.faconf_addr = (UInt32 *)(ptr + FA_OFFSET);						// 系统配置地址
	
	ShareRegionAddr.digitIn_addr = (UInt32 *)(ptr + YX_OFFSET);
	ShareRegionAddr.digitIn_EN_addr = (UInt32 *)(ptr + YX_OFFSET + YX_STEP ); 
	ShareRegionAddr.digitIn_COS_addr = (UInt32 *)(ptr + YX_OFFSET + 2*YX_STEP ); 
	ShareRegionAddr.digitIn_SOE_addr = (UInt32 *)(ptr + YX_OFFSET + 3*YX_STEP ); 
	ShareRegionAddr.digitIn_NOT_addr = (UInt32 *)(ptr + YX_OFFSET + 4*YX_STEP ); 
	
	ShareRegionAddr.printf_addr  = ptr + PRINTF_BASE;									//调试打印共享区地址
	ShareRegionAddr.anadjust_addr = (float *)(ptr + YC_CHECK_PAR_BASE);				//校准地址

}
/***************************************************************************/
//函数:	Int8 DigAdjust(App_Msg *msg)
//说明: 校准参数
//输入: msg 接收到的校准信息 校准index 与 校准值 adjust 校准值数组
//输出: status 状态 备用 0 校准成功 1 校准失败
//编辑:
//时间:2015.5.22
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

	if(msg->channel <= MAX_INDEX)			//判断是否越界
		index = msg->channel;				//索引值
	else
	{
		LOG_INFO("Channel is over the index!");
		status = 0;
		return status;
	}
	
	value = 1000 * msg->data;				//校准值 mA 或 mV
	//判断是否为0值校准
	if(value == 0)
	{
		// value值为0,不改变校准值
		LOG_INFO("Value is 0!");
		return status;
	}
//	value = msg->data;						//校准值A或V

	// base_addr是UInt32 ,index索引号对应的参数占用8个字节
	collectvalue = (float *)(ShareRegionAddr.base_addr + index*2);
	ShareAdjustaddr  = ShareRegionAddr.anadjust_addr;
	//计算校准参数
	if(0 == collectvalue[0])
		ShareAdjustaddr[index] = 1;
	else
		ShareAdjustaddr[index] = (value * ShareAdjustaddr[index]) / collectvalue[0];

	//参考findindex[]数组 查找index对应的通道号
	for(i=0;i<8;i++)
	{
		if(index == findindex[i])
		{
			channel = i;
			break;
		}
	}
	/* 更新本地校准 */
	adjust[channel] = ShareAdjustaddr[index];

	LOG_INFO("DigAdjust collectvalue is %d channel is %d, adjust is %d, collect is %d;",(int)collectvalue[0],index , (int)adjust[channel], (int)collectvalue[0]);
	return status;
}
/***************************************************************************/
//函数: Int16 DigCmdout(App_Msg *msg)
//说明: 遥控输出函数
//输入: msg  获得的msg信息 包含index 与cmd
//输出: 0 成功 1 失败
//编辑:
//时间:2015.5.25
/***************************************************************************/
Int16 DigCmdout(App_Msg *msg)
{
	Int16 status = 0;
	SYSPARAMSTR *sysparm = (SYSPARAMSTR *)ShareRegionAddr.sysconf_addr;
	UInt32 *yxdata = (UInt32 *)ShareRegionAddr.digitIn_addr;
	static DigOutStr cmdout;

	// 远方使能
	if((yxdata[0]>>(JDYX-1)) & 0x01 == 0)
		return 0;
	
	switch(msg->data)
	{	
		case YK_OPEN_SEL:								//预置打开选中
			cmdout.DoutFlag = 1;
			cmdout.DoutIndex = msg->channel;			//预置遥控index
			YK_SendOut(0, PIN_LOW);					//拉低导通
			break;
		case YK_CLOSE_SEL:								//预置关闭选中
			cmdout.DoutFlag = 1;
			cmdout.DoutIndex = msg->channel;			//预置遥控index
			YK_SendOut(0, PIN_LOW);					//拉低导通
			break;
		case YK_CANCEL_OPEN_SEL:						//取消打开选中
			cmdout.DoutFlag = 0;
			cmdout.DoutIndex = msg->channel;			//预置遥控index
			YK_SendOut(0, PIN_HIGH);					//关闭预置开关
			break;
		case YK_CANCEL_CLOSE_SEL:						//取消关闭选中
			cmdout.DoutFlag = 0;
			cmdout.DoutIndex = msg->channel;			//预置遥控index
			YK_SendOut(0, PIN_HIGH);					//关闭预置开关
			break;
		case YK_OPEN_EXECUTIVE:							//执行打开
			cmdout.DoutIndex = msg->channel;			//执行YK编号
			
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
					status = 1;							//发生错误,未正常分闸
				}
			}
			YK_SendOut(0, PIN_HIGH);					//关闭预置选中
			break;
		case YK_CLOSE_EXECUTIVE:						//执行关闭
			cmdout.DoutIndex = msg->channel;			//执行YK编号
			
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
					status = 1;							//错误,未正常合闸
				}
			}
			YK_SendOut(0, PIN_HIGH);					//关闭预置选中
			break;
		default:
			break;
	}

	return status;
}
/***************************************************************************/
//函数: Void InitAnaInAddr(void)
//说明: 初始化基本参量地址指针
//输入: 
//输出: 
//编辑:
//时间:2015.5.25
/***************************************************************************/
Void InitAnaInAddr(void)
{
//	UChar i,j;

	//赋值YC参数地址
//	ParListPtr = ShareRegionAddr.boardparlistptr;

//	memset((char *)ParListPtr, 0, 4*sizeof(ParameterListStr));
}
/***************************************************************************/
//函数: Void Log_Printf(Void)
//说明: 打印调试信息
//输入: 无
//输出: 无
//编辑:
//时间:2015.5.7
/***************************************************************************/
//Void Log_Printf(Void)
//{
//	App_Msg logmsg;

//	logmsg.cmd = MSG_PRINTF;
//	Message_Send(&logmsg);
//}
/***************************************************************************/
//函数: Int ReadConfig(Void)
//说明: 读取部分参数并检测参数是否正确，不另行存储参数，应用参数时直接调用共享区即可
//输入: 无
//输出: 0 正常 -1 错误
//编辑:
//时间:2015.8.7
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

	// sys配置文件参数读取
	/* 遥控输出时间过短时 */
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
	
	/* 共计20个参量 index max=20 但是需要校准的只有8个通道 */
	for(i=0;i<20;i++)
	{
		if(adjust[i] == 0.0)
		{	
			/* 默认校准值*/
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
	//根据具体值算出，暂时使用
//	adjust[17] = 0.30205;
	/* 校准值赋值 根据indx值索引 */
	for(i=0;i<8;i++)
	{
		AdjustValue[i] = *(adjust + findindex[i]);
	}
	/* fa参数判断是否正确 */
	/* 三次重合闸 */
	if(fapamptr->reclose_n > 3)
	{
		fapamptr->reclose_n = 3;
		LOG_INFO("ReadConfig reclose_n is over value. ");
	}
	/* 三段过流 */
	if(fapamptr->cursection_n > 3)
	{
		fapamptr->cursection_n = 3;
		LOG_INFO("ReadConfig cursection_n is over value. ");
	}
//	for(i=0;i<fapamptr->cursection_n;i++)
//	{
//		// 由于输入的是A 因此需要扩大1000倍 
//		fapamptr->cursection[i].protectvalue *= 1000; 
//	}
	
	/* 零序三段过流 */
	if(fapamptr->zerosection_n > 3)
	{
		fapamptr->zerosection_n = 3;
		LOG_INFO("ReadConfig zerosection_n is over value. ");
	}
//	for(i=0;i<fapamptr->zerosection_n;i++)
//	{
//		// 由于输入的是A 因此需要扩大1000倍 
//		fapamptr->zerosection[i].protectvalue *= 1000; 
//	}
	
	/* 越限保护最大支持8个通道的越限检测(3U4I1DC) 由于分为上下限值因此n值最大为16 */
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
