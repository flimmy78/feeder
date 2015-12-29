/*************************************************************************/
// monitorpc.h                                        Version 1.00
//// ���ļ���DTU2.0����λ��ͨ�ŵĴ���ͷ�ļ�
// ��д��  :shaoyi
// email		  :shaoyi1110@126.com
//  ��	   ��:2015.05.20
//  ע	   ��:
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
	MSG_GET_IEC101,//�����ϴ�guiyue101.conf�ļ������ݣ���λ��ֻ��Ҫ������ʾ������Ҫ��������λ�����͵�armʱ����data��
	MSG_GET_IEC104,//guiyue104.conf���ļ����ݣ���λ��ֻ��Ҫ������ʾ������Ҫ����
	MSG_LOG,//��ʾmy_debug�Ĵ�ӡ��Ϣ����λ��ֻ��Ҫ������ʾ������Ҫ����
	MSG_CHECK_TIME,//Уʱ�����λ����Ҫ�ظ�ȷ�ϣ�һ����Ϣִ��һ��
	MSG_CHECK_I,//У׼��������λ����Ҫ�ظ�ȷ��,һ����Ϣִ��һ��
	MSG_CHECK_U,//У׼��ѹ����λ����Ҫ�ظ�ȷ�ϣ�һ����Ϣִ��һ��
	MSG_EVENT_COS,//�¼��ϱ�����λ��Ϣ,��Ҫ�򿪹ر�
	MSG_EVENT_SOE,//�¼��ϱ���SOE����COS��һ��TIME
    MSG_DIANBIAO_YC,//�������Ϣ���ϴ�����Զ��ر�
    MSG_DIANBIAO_YC_DQ,//�����ϴ�ң��ֵ����������ر�
    MSG_DIANBIAO_YX,//�������Ϣ���ϴ�����Զ��ر�
    MSG_DIANBIAO_YX_DQ,//�����ϴ�ң��ֵ����������ر�11 0x0b
    MSG_DIANBIAO_YK,//���ڷ���ң������
	MSG_DIANBIAO_YC_DQ_CLOSE,  //�����ϴ�ֹͣ����
	MSG_DIANBIAO_YX_DQ_CLOSE,   //�����ϴ�ֹͣ����14 0x0e
    MSG_GET_IEC101_CLOSE,       //��data��
    MSG_GET_IEC104_CLOSE,       //��data��
    MSG_LOG_CLOSE, //��data�� 0x11
    MSG_EVENT_SOE_CLOSE, //��data��0x12
    MSG_EVENT_COS_CLOSE, //��data��0x13
    MSG_ALL_CLOSE,//��data��
    MSG_ALL_SOE,//21 15
    MSG_RBOOT_BOARD,
    MSG_END
    
}PcMsgCmd;
extern void * PC_Creat_connection( void * arg  );
extern void PC_Send_Data(UInt8 funcode, UInt8 * buf, UInt8 len);
extern UInt8 PC_Check_Sum(UInt8 * buf, UInt8 len);


#endif
