#ifndef _READSYSCONFIG_H
#define _READSYSCONFIG_H

/* host header files */
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdlib.h>

#include <ti/syslink/Std.h>     /* must be first */

/* must be first */
#include <ti/syslink/utils/Memory.h>
#include <ti/ipc/Notify.h>
#include <ti/ipc/SharedRegion.h>
#include "common.h"

#define MODELSYSFILE            "/config/sys.conf"

struct _SYSDSPREAD_
{
    
	short yc1_out;				//遥控1输出时间ms
	short yc2_out;				//遥控2输出时间ms
	short yxtime;				//遥信去抖时间ms
	short ptrate;				//PT变比
	short ctrate;				//CT变比
	short pctrate;				//保护CT变比
	short battlecycle;			//蓄电池活化周期，单位日
	short battletime;	        //蓄电池活化时间，单位s
};
typedef struct _SYSDSPREAD_ SYSDSPREAD; 
/* SYS系统保护参数 */
struct _SYSPARAME_
{
    
    int    logfd;
    int   fd_comtype;				//打开串口对应的文件
    unsigned int  bitrate;			//bitrate=115200#对应的串口波特率
    short  usRecvCont;			//接收到的数据
    unsigned short  pBufGet;		//指向接收缓冲区的下标
    unsigned short  pBufFill;		//指向接收缓冲区的下标
    unsigned char * pSendBuf;
    unsigned char  comtype;			//串口类型：2 RS232 4 RS485
    unsigned char  uart2;			//uart2=2#使用哪个串口作为调试进口
    SYSDSPREAD     DspReadData;
    /*
	short yc1_out;				//遥控1输出时间ms
	short yc2_out;				//遥控2输出时间ms
	short yxtime;				//遥信去抖时间ms
	short ptrate;				//PT变比
	short ctrate;				//CT变比
	short pctrate;				//保护CT变比
	short battlecycle;			//蓄电池活化周期，单位日
	short battletime;	        //蓄电池活化时间，单位s
	*/
}; 
typedef struct _SYSPARAME_ SYSPARAM;
/*
struct _SYS_CONFIG
{
    
    int    logfd;
    int   fd_uart;				//打开串口对应的文件
    unsigned int  bitrate;			//bitrate=115200#对应的串口波特率
    short  usRecvCont;			//接收到的数据
    unsigned short  pBufGet;		//指向接收缓冲区的下标
    unsigned short  pBufFill;		//指向接收缓冲区的下标
    unsigned char * pSendBuf;
    unsigned char  uart;			//usart=1#使用哪个串口作为101规约进口
};

typedef struct _SYS_CONFIG SYS_CONFIG;
*/

extern void Read_Sysconfig_Conf(char * fd,  struct _SYSPARAME_ *  sys);
void Write_Sys_Sharearea(struct _SYSPARAME_ * item);

extern int Uart2_Open_Console(void);


#endif
