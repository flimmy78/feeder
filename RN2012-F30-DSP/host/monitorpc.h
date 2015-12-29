/*************************************************************************/
// monitorpc.h                                        Version 1.00
//// 本文件是DTU2.0与上位机通信的代码头文件
// 编写人  :shaoyi
// email		  :shaoyi1110@126.com
//  日	   期:2015.05.20
//  注	   释:
/*************************************************************************/

#ifndef    _MONITORPC_H
#define    _MONITORPC_H

/* host header files */
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>

/* package header files */
#include <ti/syslink/Std.h>

/* must be first */
#include <ti/syslink/utils/Memory.h>
#include <ti/ipc/Notify.h>
#include <ti/ipc/SharedRegion.h>
#include "common.h"

typedef enum
{
	MSG_GET_IEC101,//用于上传guiyue101.conf文件的内容，上位机只需要串口显示，不需要解析，上位机发送到arm时候无data区
	MSG_GET_IEC104,//guiyue104.conf的文件内容，上位机只需要串口显示，不需要解析
	MSG_LOG,//显示my_debug的打印信息，上位机只需要串口显示，不需要解析
	MSG_CHECK_TIME,//校时命令，上位机需要回复确认，一次消息执行一次
	MSG_CHECK_I,//校准电流，上位机需要回复确认,一次消息执行一次
	MSG_CHECK_U,//校准电压，上位机需要回复确认，一次消息执行一次
	MSG_EVENT_COS,//事件上报，变位信息,需要打开关闭
	MSG_EVENT_SOE,//事件上报，SOE，比COS多一个TIME
    MSG_DIANBIAO_YC,//检查点表信息，上传完毕自动关闭
    MSG_DIANBIAO_YC_DQ,//定期上传遥测值，必须命令关闭
    MSG_DIANBIAO_YX,//检查点表信息，上传完毕自动关闭
    MSG_DIANBIAO_YX_DQ,//定期上传遥信值，必须命令关闭11 0x0b
    MSG_DIANBIAO_YK,//用于发送遥控命令
	MSG_DIANBIAO_YC_DQ_CLOSE,  //定期上传停止命令
	MSG_DIANBIAO_YX_DQ_CLOSE,   //定期上传停止命令14 0x0e
    MSG_GET_IEC101_CLOSE,       //无data区
    MSG_GET_IEC104_CLOSE,       //无data区
    MSG_LOG_CLOSE, //无data区 0x11
    MSG_EVENT_SOE_CLOSE, //无data区0x12
    MSG_EVENT_COS_CLOSE, //无data区0x13
    MSG_ALL_CLOSE,//无data区
    MSG_ALL_SOE,//21 15
    MSG_RBOOT_BOARD,
    MSG_END
    
}PcMsgCmd;
extern void * PC_Creat_connection( void * arg  );
extern void PC_Send_Data(UInt8 funcode, UInt8 * buf, UInt8 len);
extern UInt8 PC_Check_Sum(UInt8 * buf, UInt8 len);


#endif
