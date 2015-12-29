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
    
	short yc1_out;				//ң��1���ʱ��ms
	short yc2_out;				//ң��2���ʱ��ms
	short yxtime;				//ң��ȥ��ʱ��ms
	short ptrate;				//PT���
	short ctrate;				//CT���
	short pctrate;				//����CT���
	short battlecycle;			//���ػ���ڣ���λ��
	short battletime;	        //���ػʱ�䣬��λs
};
typedef struct _SYSDSPREAD_ SYSDSPREAD; 
/* SYSϵͳ�������� */
struct _SYSPARAME_
{
    
    int    logfd;
    int   fd_comtype;				//�򿪴��ڶ�Ӧ���ļ�
    unsigned int  bitrate;			//bitrate=115200#��Ӧ�Ĵ��ڲ�����
    short  usRecvCont;			//���յ�������
    unsigned short  pBufGet;		//ָ����ջ��������±�
    unsigned short  pBufFill;		//ָ����ջ��������±�
    unsigned char * pSendBuf;
    unsigned char  comtype;			//�������ͣ�2 RS232 4 RS485
    unsigned char  uart2;			//uart2=2#ʹ���ĸ�������Ϊ���Խ���
    SYSDSPREAD     DspReadData;
    /*
	short yc1_out;				//ң��1���ʱ��ms
	short yc2_out;				//ң��2���ʱ��ms
	short yxtime;				//ң��ȥ��ʱ��ms
	short ptrate;				//PT���
	short ctrate;				//CT���
	short pctrate;				//����CT���
	short battlecycle;			//���ػ���ڣ���λ��
	short battletime;	        //���ػʱ�䣬��λs
	*/
}; 
typedef struct _SYSPARAME_ SYSPARAM;
/*
struct _SYS_CONFIG
{
    
    int    logfd;
    int   fd_uart;				//�򿪴��ڶ�Ӧ���ļ�
    unsigned int  bitrate;			//bitrate=115200#��Ӧ�Ĵ��ڲ�����
    short  usRecvCont;			//���յ�������
    unsigned short  pBufGet;		//ָ����ջ��������±�
    unsigned short  pBufFill;		//ָ����ջ��������±�
    unsigned char * pSendBuf;
    unsigned char  uart;			//usart=1#ʹ���ĸ�������Ϊ101��Լ����
};

typedef struct _SYS_CONFIG SYS_CONFIG;
*/

extern void Read_Sysconfig_Conf(char * fd,  struct _SYSPARAME_ *  sys);
void Write_Sys_Sharearea(struct _SYSPARAME_ * item);

extern int Uart2_Open_Console(void);


#endif
