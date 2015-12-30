/*************************************************************************/
/*
 * Copyright (c) 2015, 郑州瑞能电气 Runner All rights reserved.
 *
 * file       : collect.c
 * Design     : zlb
 * Email	  : longbiao831@qq.com
 * Data       : 2015-8-10
 * Version    : V1.0
 * Change     :
 * Description: 遥测、遥信数据采集 Collect_Task任务运行程序
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
#include "ad7606.h"				//添加 9.9
#include "sys.h"
#include "led.h"
#include "log.h"
#include "fa.h"


UChar FGflag = 0;

/***************************************************************************/
//函数:	Void ChangeLED(UInt16 *status)
//说明:	LED分合状态指示灯切换
//输入: status YX状态数据地址
//输出: 无
//编辑:
//时间:2015.8.18
/***************************************************************************/
Void ChangeLED(UInt32 *status)
{
	//分闸分位为合 
	if((*status >> (FWYX-1)) & 0x01)
	{
		LEDDATA |= 0x01<<LED_FZ1;
	}
	else
	{
		// 点亮分闸LED
		LEDDATA &= ~(0x01<<LED_FZ1);
	}
	//分闸合位为分
	if((*status >> (HWYX-1)) & 0x01)
	{
		LEDDATA |= 0x01<<LED_HZ1;
	}
	else
	{
		//点亮合闸LED
		LEDDATA &= ~(0x01<<LED_HZ1);
	}
	/* yk2 可以添加到这里 */
	/* fugui信号有效*/
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
//函数:	Void Collect_Task(UArg arg0, UArg arg1) 
//说明:	数据采集任务
//输入: arg0 arg1
//输出: 无
//编辑:
//时间:2015.4.24
/***************************************************************************/
Void Collect_Task(UArg arg0, UArg arg1) 
{
	UInt32 *yxdata = (UInt32 *)ShareRegionAddr.digitIn_addr;
	SYSPARAMSTR *sysparm = (SYSPARAMSTR *)ShareRegionAddr.sysconf_addr;
	UChar i;
	UInt32 channel;
	// 遥信默认值全部为1
	UInt32 newdata = 0xff;
	UInt32 comparedata = 0;
	UInt32 status = 0;
	unsigned long ticks[10]={0};
	/* 这里没有考虑溢出问题,系统时钟节拍是U32格式最小节拍1000us因此需要保证系统运行49天之内重启一次即可*/
//	UInt16 overflow = 0;
	/* 10表示硬件只有10个遥信节点 数组中分别为每个遥信的IO管脚 */
	UChar yxio_num[10] = {YX_1,YX_2,YX_3,YX_4,YX_5,YX_6,YX_7,YX_8,YX_9,YX_10};
	/* yx初始值为0 */
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
		// 更新数据
		newdata = yxdata[0];
		/* 遥信数据变位 */
		for(i=0;i<10;i++)
		{
			if((status>>i) & 0x01)
			{	
				/* 延时时间到 Clock_getTicks() */
				if(ticks[i] <= tickets)
				{
					comparedata = (newdata >> i) & 0x01;
					if(comparedata != GPIOPinRead(SOC_GPIO_0_REGS, yxio_num[i]))
					{
						newdata ^= (0x01<<i);
						//复制到共享区
						yxdata[0] = newdata;
						/* 判断是否取反使能 */
						if(dianbiaodata.yxnot & (0x01<<i))
						{
							// 使能取反则将数据再次异或
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
					/* 清除变位标志 */
					status &= ~(0x01<<i);
				}
			}
			/* 判断是否使能 */
			else if((dianbiaodata.yxen >> i) & 0x01)
			{	
				comparedata = (newdata>>i) & 0x01;
				if(comparedata != GPIOPinRead(SOC_GPIO_0_REGS, yxio_num[i]))
				{
					/* 设置flag标志 表示状态发生改变 */
					status |= (0x01<<i);
					/* sysparm->yxtime遥信去抖时间 */
					ticks[i] = tickets + sysparm->yxtime;
				}
			}
		}		
		/* 点亮相应的LED(分合闸LED) */
		ChangeLED(yxdata);
		/* 延时2ms */
		Task_sleep(2);
	}

}



