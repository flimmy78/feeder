/*************************************************************************/
// common.h                                       Version 1.00
//
// ���ļ���DTU2.0�豸�Ĺ�����Դ
// ��д��:R&N                        email:R&N1110@126.com
//
//  ��	   ��:2015.04.17
//  ע	   ��:
/*************************************************************************/
#ifndef    _COMMON_H
#define    _COMMON_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/rtc.h>


#include <ti/syslink/Std.h>     /* must be first */


#include "iec104.h"
#include "iec101.h"
#include "monitorpc.h"

#define TRUE 							1
#define FALSE 						0
//����״̬��־�ĺ궨��
#define   EMPTY    						0x00
#define   FULL     						0x01
#define   ERROR   						0x02
#define   NEW     						0x03  	//for SOEArray
#define   OLD     						0x04 	 //for SOEArray


#define DISENABLE					0xA0
#define ENABLE						0x05

#define SEND_FRAME					1
#define RECV_FRAME					0

//�������еĺ궨��
#define SHARED_REGION_0         		0x0
#define SHARED_REGION_1         		0x1
#define SHAREAREA_SIZE				0x4000
#define SystemCfg_LineId        			0
#define SystemCfg_EventId       			7

//MESSAGE\NOTIFY\SHAREAREA�еĺ궨��
#define APP_E_FAILURE           			0xF0000000
#define APP_E_OVERFLOW        			0xF1000000
#define APP_E_FAILURE            			0xF0000000
#define APP_SPTR_HADDR          		0x20000000  /* cc--hhhh */
#define APP_SPTR_LADDR          		0x21000000  /* cc--llll */
#define APP_SPTR_ADDR_ACK       		0x22000000  /* cc------ */
#define APP_CMD_SHUTDOWN_ACK    	    0x30000000  /* cc------ */
#define APP_CMD_SHUTDOWN        		0x31000000  /* cc------ */
#define APP_CMD_RESRDY          		0x09000000  /* cc-------*/
#define APP_CMD_READY           		0x0A000000  /* cc-------*/
#define APP_CMD_NOP             			0x10000000  /* cc------ */


#define App_MsgHeapName         		"MsgHeap:0"
#define App_MsgHeapSrId         			0
#define App_MsgHeapId            			0
#define App_HostMsgQueName       		"HOST:MsgQ:0"
#define App_VideoMsgQueName     		"VIDEO:MsgQ:0"


#define TYPE_UAB	   				1  //����ѹ
#define TYPE_UA						2  //�ߵ�ѹ
#define TYPE_O		 				3  //��ѹ���
#define TYPE_HZ						4  //��Ƶ
#define TYPE_IAB					5  //�����
#define TYPE_IA						6  //�ߵ���
#define TYPE_WA						7  //�й�
#define TYPE_WI						8  //�޹�
#define TYPE_Q						9  //��������
#define TYPE_E						10//���
#define TYPE_OAB					11//�������
#define TYPE_YK						12//ң��
#define TYPE_YX						13//ң��
#define TYPE_YC						14//ң��
#define TYPE_YC_B					15//ң�⹲��������һ��Int
#define TYPE_YX_STATUS_BIT          16//ң�Ź�����״̬λ
#define TYPE_YX_ENABLE_BIT          17//ң�Ź���ʹ��λ
#define TYPE_YX_COS_ENABLE_BIT      18//ң�Ź�����COSʹ��λ
#define TYPE_YX_SOE_ENABLE_BIT      19//ң�Ź�����SOEʹ��λ
#define TYPE_YX_RETURN_BIT          20//ң�Ź���תʹ��λ
#define TYPE_YC_CHECK_VALUE			21//ң��У׼ֵ
#define TYPE_YC_CHECK_PAR			22//ң��У׼����
//������ʱ��
#define NOTIME						0
#define WITHTIME					1

struct _CP56Time2a
{
	unsigned char bMsecL;
	unsigned char bMsecH;
	unsigned char bMinute;
	unsigned char bHour;
	unsigned char bDay;
	unsigned char bMonth;
	unsigned char bYear;
};
typedef	struct _CP56Time2a CP56Time2a;

//�ж���ң���ܵ�ַ�Ƿ�����
#define YC_NOTCONTINUE			(1<<0)
#define YK_NOTCONTINUE			(1<<1)
#define YX_NOTCONTINUE			(1<<2)

#define   YKLOCK_NONE           0
#define   YKLOCK_BYCSDA         1
#define   YKLOCK_BYPROTO1       2
#define   YKLOCK_BYPROTO2       3
#define   YKLOCK_BYPROTO3       4
#define   YKLOCK_BYPLC          5
#define   YKLOCK_BYLOCALPIN     6
//ң��0:����   1:����    2:����    3:û�������ַ
#define  YK_BISUO				0
#define  YK_NORMAL_CONTRL      	1
#define  YK_FUGUI				2
#define  YK_INVALID_ADDR		3
#define  YK_CANCEL_SEL 			4
#define  YK_ZHIXIN_FAIL			5
#define  YK_ZHIXIN_SUCC			6
#define  YK_SEL_FAIL            7

//��������ÿ�������Ϣ
struct _Point_Info
{
	UInt8   ucName[5];
	UInt8   ucType;					//��Ӧ1--����      2--��ѹ		3--���	4--����
	UInt16 usFlag;					//��Ӧ��ʹ�ܱ��(��4λ) �Լ�����(��12λ)  ����ң��bit[8]��բѡ��bit[9]��բѡ��
	UInt16 uiOffset;					//��Ӧʵ����ֵ�Ĵ洢��ַƫ����
	UInt16 uiAddr;					//��Ӧ�ĵ�ŵĵ�ַ
	struct _Point_Info * next;
};
typedef struct _Point_Info Point_Info;

//��������ȫ�ֵ�������Ϣ
typedef struct {
   UInt16  remoteProcId;

   UInt16  usYC_Num;					//ң������
   UInt16  usYX_Num;					//ң������  ������(����)
   UInt16  usYK_Num;					//ң������  �����(����)
   UInt8   Continuity;					//��ַ�Ƿ�����
   UInt8    reset;						//����豸�Ƿ�����

   Point_Info   * pYC_Addr_List_Head;
   Point_Info   * pYX_Addr_List_Head;
   Point_Info   * pYK_Addr_List_Head;

   IEC104Struct  *  Iec104_p;
   IEC101Struct  *  Iec101_p;
   int              pc_fd;
   UInt32           pc_flag;
} LOCAL_Module;


struct _YXInfoStruct
{
	unsigned char  ucStatus;
	unsigned char  ucYXValue;
	unsigned short usIndex;
	struct _YXInfoStruct * next;
};

typedef struct _YXInfoStruct YXInfoStruct;

struct _SOEInfoStruct
{
    unsigned char  ucYear;
 	unsigned char  ucMonth;
 	unsigned char  ucDay;
 	unsigned char  ucHour;
 	unsigned char  ucMin;
 	unsigned short usMSec;

	unsigned char  ucStatus;
    unsigned char  ucYXValue;
	unsigned short usIndex;
 	struct _SOEInfoStruct * next;
};
typedef struct _SOEInfoStruct SOEInfoStruct;


extern UInt8 print_buf[60];

#define DEBUG_SHAOYI

#ifdef DEBUG_SHAOYI
#define my_debug( format, ...)    do {    printf( format "\n",##__VA_ARGS__ );  } while (0)
#else
#define my_debug( format, ...)  \
    do {                        \
        sprintf((char *)print_buf,  format "\n",##__VA_ARGS__ );\
        PC_Send_Data(MSG_LOG, print_buf, strlen(print_buf));\
       } while (0)
#endif

extern int  Init_Logfile( char * file );
extern int  Init_SOEfile(char * file);

extern  void Log_Frame(int fd, unsigned char * buf, unsigned short recvcnt, unsigned short len, unsigned char dir );
extern unsigned int  Get_Rtc_Time(struct rtc_time  * rtctime);
extern unsigned char  Set_Rtc_Time( struct rtc_time  * rtctime, long microseconds);

#endif


