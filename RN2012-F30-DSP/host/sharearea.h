/*************************************************************************/
// sharearea.h                                Version 1.00
// ����
//
// ���ļ���DTUͨѶ����װ�õ�CPU֮�乲���ڴ�
// ��д��:R&N
//
//  email		  :R&N@126.com
//  ��	   ��:2014.9.16
//  ע	   ��:
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
#define YX_STATUS_BASE  			3200//ÿ��bit��ʾһ��״̬λ
#define YX_ENABLE_BASE       		3220//ÿ��bit��ʾһ��ʹ��λ
#define YX_COS_BASE            		3240//ÿ��bit��ʾһ��COSʹ��
#define YX_SOE_BASE           		3260//ÿ��bit��ʾһ��SOEʹ��
#define YX_REVERSE_BASE        		3280//ÿ��bit��ʾһ����תʹ��


/*�·�����ڴ�ռ�*/
#define YK_CONFIG_BASE              3300//addң������ 2B+28B
#define YK_ENABLE_BASE              3300//ң�Ź���ʹ��λ
#define YK_COS_BASE                 3310//ÿ��bit��ʾһ��COSʹ��
#define YK_SOE_BASE                 3320//ÿ��bit��ʾһ��SOEʹ��
#define YC_CONFIG_BASE              3330//addң������ 4B+60B
#define SYS_CONFIG_BASE             3400//addϵͳ���� 14B+50B 3394
#define FA_CONFIG_BASE              3464//addFA����   43B+57B 3458


#define YC_ENABLE_BASE       		3400//ÿ��ʹ��λռ��4bits  bit[0]:����ʹ�ܣ�bit[1]:COS   bit[2]:SOE   bit[3]:Ԥ��

#define YC_CHECK_VALUE_BASE         3600//�洢У׼ֵ 1�ֽ�
#define YC_CHECK_PAR_BASE           4000//�洢ң�������У׼������float
#define PRINTF_BASE				    6000//����DSP�Ĵ�ӡ����
#define YK_STATE_ADDR_BASE          6100//ң�ط���״̬


#define YK_OPEN_SEL                 0x01
#define YK_CLOSE_SEL                0x02
#define YK_CANCEL_OPEN_SEL          0x03
#define YK_CANCEL_CLOSE_SEL         0x04
#define YK_OPEN_EXECUTIVE           0x05
#define YK_CLOSE_EXECUTIVE          0x06

/*ң��У׼����*/
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
	MSG_CURRENT,//У׼����
	MSG_VOLTAGE,//У׼��ѹ
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
