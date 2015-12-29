/*************************************************************************/
/*
 * Copyright (c) 2015, 郑州瑞能电气 Runner All rights reserved.
 *
 * file       : dsp.c
 * Design     : zlb
 * Email	  : longbiao831@qq.com
 * Data       : 2015-7-28
 * Version    : V1.0
 * Change     :
 * Description: dsp主程序
 */
/*************************************************************************/
/* xdctools header files */
#include <xdc/std.h>
#include <xdc/runtime/Diags.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/Log.h>
#include <xdc/runtime/System.h>

/* package header files */
#include <ti/ipc/Ipc.h>
#include <ti/ipc/MultiProc.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

/* local header files */
#include "IPCServer.h"
#include "collect.h"
#include "queue.h"
#include "led.h"
#include "fft.h"
#include "sys.h"
#include "log.h"
#include "fa.h"


/****************************** macro define ************************************/

/******************************* functions **************************************/
Void Init_Conf(UArg arg0, UArg arg1);
/***************************************************************************/
//函数:Int main(Int argc, Char* argv[])
//说明:main()			
//输入:输入参数
//输出: 
//编辑:
//时间:2015.4.20
/***************************************************************************/
Int main(Int argc, Char* argv[])
{
	Error_Block  eb;
	Task_Params  taskParams;
	Error_init(&eb);    

	Task_Params_init(&taskParams);
	taskParams.instance->name = "Init_Conf";
	taskParams.arg0 = (UArg)argc;
	taskParams.arg1 = (UArg)argv;
	taskParams.stackSize = 0x1000;
	taskParams.priority = 9;
	Task_create(Init_Conf, &taskParams, &eb);

	if (Error_check(&eb)) 
	{
	       System_abort("main: failed to create application startup thread");
	}

	/* start scheduler, this never returns */
	BIOS_start();

	return (0);
}
/***************************************************************************/
//函数:Void Init_Conf(UArg arg0, UArg arg1)
//说明:Init_Conf ()初始化syslink任务	
//输入: 输入参数
//输出: 
//编辑:
//时间:2015.4.20
/***************************************************************************/
Void Init_Conf(UArg arg0, UArg arg1)
{
    Int status = 0;
    Error_Block  eb;
	App_Msg  *msg;
    UInt16  remoteProcId;
	Task_Params  taskParams;
	
	//共享内存区地址指针
	UChar* ShareRegionPtr = NULL;	
    Error_init(&eb);

	/*串口打印初始化*/
	LOG_INIT();
	/* 1.硬件初始化 */
	Sys_Init();

	/* 2.建立双核通信机制 */
	status = Syslink_Init(&remoteProcId, &ShareRegionPtr);
	if(status < 0)
	{
		/* 点亮故障指示灯 LED2 */
		LEDDATA &= ~(0x01<<LED_PROB);
		LED_SENDOUT(LEDDATA);
		LOG_INFO("Syslink_Init is err. ");
		return;
	}
	/* 3.系统自检 */
	status = Sys_Check();
	if(status < 0)
	{	
		/* 点亮故障指示灯 LED2 */
		LEDDATA &= ~(0x01<<LED_PROB);
		LED_SENDOUT(LEDDATA);
		LOG_INFO("Init_Conf Sys_Check is err. ");
		goto level;
	}
	
	/* 4.读取配置信息 */
	Init_ShareAddr(ShareRegionPtr);
	ReadConfig();
	LOG_INFO("ReadConfig get data yx;");
	
	Task_Params_init(&taskParams);
	/* 采集任务 */
    taskParams.instance->name = "Collect_Task";
	taskParams.arg0 = (UArg)arg0;
	taskParams.arg1 = (UArg)arg1;
	taskParams.stackSize = 0x1000;
	taskParams.priority = 6;
	Task_create(Collect_Task, &taskParams, &eb);
	/* 温度采集 */
	taskParams.instance->name = "Tempture";
	taskParams.arg0 = (UArg)arg0;
	taskParams.arg1 = (UArg)arg1;
	taskParams.stackSize = 0x1000;
	taskParams.priority = 7;
	Task_create(Tempture, &taskParams, &eb);
	/* 数据计算任务 */
	taskParams.instance->name = "FFT_Task";
	taskParams.arg0 = (UArg)arg0;
	taskParams.arg1 = (UArg)arg1;
	taskParams.stackSize = 0x1000;
	taskParams.priority = 8;
	Task_create(FFT_Task, &taskParams, &eb);
	/* FA保护任务 */
	/* 优先级最高 */
	taskParams.instance->name = "FA_Task";
	taskParams.arg0 = (UArg)arg0;
	taskParams.arg1 = (UArg)arg1;
	taskParams.stackSize = 0x1000;
	taskParams.priority = 10;
	Task_create(FA_Task, &taskParams, &eb);
	/* MessageQ task */
	while(1)
	{
		MessageQ_get(Module_Mes.videoQue, (MessageQ_Msg *)&msg, MessageQ_FOREVER);
		/* process the message */
		LOG_INFO("Message_Task: processed cmd=0x%x", msg->cmd);

		//process message
		switch(msg->cmd)
		{
			case MSG_YK:
				LOG_INFO("MSG_YK: msg->data = %x,msg->channel = %x", msg->data,msg->channel);
				status = DigCmdout(msg);
				break;
			case MSG_JIAOZHUN:
				LOG_INFO("MSG_JIAOZHUN: msg->data = %x,msg->channel = %x", msg->data,msg->channel);
				status = DigAdjust(msg, AdjustValue);
				//应答
				if(status)
					Message_Send(msg->cmd, msg->channel, msg->data);
				break;
			default:
				break;
		}
		MessageQ_free((MessageQ_Msg)msg);
	}

level:
	LOG_INFO("Resource_delete: delete creat Message resource");
	Message_del();
	LOG_INFO("Resource_delete: delete creat Notify resource");
	Notify_del();		
	LOG_INFO("Stop_IPC: stop IPC connect ");
	Stop_IPC(remoteProcId);
	/* finalize modules */
	Server_exit();
    LOG_INFO("<-- Init_Conf: %d", (IArg)status);
    return;	
}

