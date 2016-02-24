#ifndef __MY_TYPE_H
#define __MY_TYPE_H
//�Զ�������
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

//���岼������
//�����������
typedef enum{No=0,Yes=1}Qfault_Mark_type;
//����ָʾ������
typedef struct{
	UInt16 id;	//ָʾ����
	UInt16 normal_current;	//������·����
	UInt16 faule_current;	//���ϵ���
	UInt16 battery_voltage;	//��ص�ѹ
	UInt16 temp;	//��·�¶�
	UInt16 to_ground_voltage_ratio;//�Եص�ѹ����
	
	Qfault_Mark_type refresh_mark;//ָʾ������ˢ�±�־
	
	Qfault_Mark_type voltage_mark;//��·�Ƿ��е�ѹ��ʾ
	Qfault_Mark_type current_mark;//��·�Ƿ��е�����ʾ
	Qfault_Mark_type short_mark;//��·��ʾ
	Qfault_Mark_type earth_mark;//�ӵر�ʾ
	Qfault_Mark_type battery_mark;//��ر�ʾ
	Qfault_Mark_type Line_up_temp;//��·����
	Qfault_Mark_type self_mark;//ָʾ�������ʾ
}indicator_type;
//����ָʾ��������
//	indicator_type A;	//A��
//	indicator_type B;	//B��
//	indicator_type C;	//C��
typedef struct{
	indicator_type P[4];//����4�࣬��������û��ʹ��
} Indicator_Group_Type;
//������վͨ��������ն��е���������
typedef enum{gprs101,indicator}buff_type;
typedef struct{
	buff_type type;
	UInt8 *buff;
}QSem_ServerTask_ReceiveType;
//����433���ô��������������
typedef struct{
	UInt16 s_addr;//���ͷ���ַ
	UInt16 d_addr;//���շ���ַ
	UInt16 control_domain;//������
	UInt16 d_buffer[11];//����ֵ
}Receive433_Frame_Type;
//�����������ַ��������
typedef struct{
	UInt8 ip[4];
	UInt16 port;
}Server_Address_TypeDef;
//���崮�����ݽ������ݽṹ
typedef struct{
	UInt8 *p_buf;
	UInt32 cnt;
}Comm_TypeDef;
//��������������������
typedef enum{mark_and_value,just_mark}Sudden_Event_Type;
typedef struct{
	Sudden_Event_Type type;
	UInt16 mark_address;
	Qfault_Mark_type mark;
	UInt16 value_address;
	UInt16 value;
}Spont_Trans_type;

#endif

