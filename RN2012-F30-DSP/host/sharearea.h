/*************************************************************************/
// sharearea.h                                Version 1.00
// 描述
//
// 本文件是DTU通讯网关装置的CPU之间共享内存
// 编写人:R&N
//
//  email		  :R&N@126.com
//  日	   期:2014.9.16
//  注	   释:
/*************************************************************************/
#ifndef    _SHAREAREA_H
#define    _SHAREAREA_H

/* host header files */
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>

/* package header files */
#include <ti/syslink/Std.h>     /* must be first */
#include <ti/syslink/utils/Memory.h>
#include <ti/ipc/Notify.h>
#include <ti/ipc/SharedRegion.h>

#include "common.h"


#define QUEUESIZE   				6
#define MESSAGE_QUEUESIZE 		    6

#define YC_BASE       				0
#define YX_BASE       				3200
#define YX_STATUS_BASE  			3200//每个bit表示一个状态位
#define YX_ENABLE_BASE       		3220//每个bit表示一个使能位
#define YX_COS_BASE            		3240//每个bit表示一个COS使能
#define YX_SOE_BASE           		3260//每个bit表示一个SOE使能
#define YX_REVERSE_BASE        		3280//每个bit表示一个反转使能


/*新分配的内存空间*/
#define YK_CONFIG_BASE              3300//add遥控配置 2B+28B
#define YK_ENABLE_BASE              3300//遥信共享使能位
#define YK_COS_BASE                 3310//每个bit表示一个COS使能
#define YK_SOE_BASE                 3320//每个bit表示一个SOE使能
#define YC_CONFIG_BASE              3330//add遥测配置 4B+60B
#define SYS_CONFIG_BASE             3400//add系统配置 14B+50B 3394
#define FA_CONFIG_BASE              3464//addFA配置   43B+57B 3458


#define YC_ENABLE_BASE       		3400//每个使能位占用4bits  bit[0]:采样使能，bit[1]:COS   bit[2]:SOE   bit[3]:预留

#define YC_CHECK_VALUE_BASE         3600//存储校准值 1字节
#define YC_CHECK_PAR_BASE           4000//存储遥测参数，校准参数是float
#define PRINTF_BASE				    6000//用于DSP的打印数据
#define YK_STATE_ADDR_BASE          6100//遥控返回状态


#define YK_OPEN_SEL                 0x01
#define YK_CLOSE_SEL                0x02
#define YK_CANCEL_OPEN_SEL          0x03
#define YK_CANCEL_CLOSE_SEL         0x04
#define YK_OPEN_EXECUTIVE           0x05
#define YK_CLOSE_EXECUTIVE          0x06

/*遥测校准参数*/
typedef struct _YC_CHECK_PARA_struct_
{
    float checkpara[20];//20
    UInt8 checksum;
}YC_CHECK_PARA_struct;
typedef struct _YK_RESULT_STRUCT_
{
   UInt32 result;
}YK_RESULT_STRUCT;

typedef enum
{
	MSG_GET_DB,
	MSG_GET_DB_OK,
	MSG_GET_DB_FAIL,
	MSG_EVENT,
	MSG_COS,
	MSG_SOE,
	MSG_JIAOSHI,
	MSG_JIAOZHUN,
	MSG_CURRENT,//校准电流
	MSG_VOLTAGE,//校准电压
	MSG_RECLOSE,
	MSG_YK,
	MSG_PRINTF,
	MSG_EXIT
}MsgCmd;

extern SharedRegion_SRPtr Sharearea_Creat(  LOCAL_Module * sysmod_p);
extern  void Sharearea_Destory(  LOCAL_Module * sysmod_p);
extern  void *Message_Thread(void * arg);
extern float Read_From_Sharearea( UInt16 index, UInt8 flag );
extern void   Write_To_Sharearea(UInt16 index, UInt8 flag, UInt32  data);

extern Int  Send_Event_By_Message( UInt32   cmd,  UInt32  buf, UInt32 data);


#endif
