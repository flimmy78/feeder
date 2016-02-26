#ifndef _DL_101_H
#define _DL_101_H
//#include "allconfig.h"
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
#include "common.h"

#include <ti/syslink/Std.h>     /* must be first */

#ifdef _DL_101_C
//����globalΪ��
#define global
//******************************************�궨�峣��************************************************************//
//����101��Լ֡ͷ֡β
#define Fix_frame_top 0x10
#define Var_frame_top 0x68
#define Normal_frame_end 0x16
/*************************************101��Լ������*********************************************************/
//���������������������
#define Reset_link_code 0x00	//��λԶ����·
#define Test_link_code 0x02		//��·���ԣ�������ʹ��
#define Trans_data_code 0x03	//��������
#define Call_linkstate_code 0x09	//������·״̬
#define Call_level1_data_code 0x0a	//�ٻ�һ������
#define Call_level2_data_code 0x0b	//�ٻ���������

//����Ӷ��������������
#define Acknowledge_code 0x00	//ȷ��
#define Negate_code 0x01			//��ȷ��
#define Respond_by_data_code 0x08	//��������Ӧ����֡
#define No_request_data_code 0x09	//�����ٻ�������
#define Respond_by_link_code 0x0b	//����·״̬������������ش�����֡

//**************************************************����ASDU���ݵ�Ԫ���ͱ�ʾ************************************/
//���Ʋο�����DL634.5.101-2002�ж����ͱ�ʾ�Ķ���
#define M_SP_TB_1 0x1e		//����ʱ��ĵ�����Ϣ
//#define C_SC_NA_1 0x2d		//��������
#define C_SE_NA_1 0x30		//�趨ֵ�����һ��ֵ
#define C_SE_NB_1	0x31		//�趨ֵ�����Ȼ�ֵ
//#define M_EI_NA_1 0x46		//��ʼ������
#define C_CD_NA_1	0x6a		//��ʱ���
/************************************************101��Լ����ԭ��******************************************************/
//���Ʋο�����DL634.5.101-2002�жԴ���ԭ��Ķ���
#define Spont_Cause 0x03		//ͻ��
#define Reg_Cause	0x05			//���������
#define Act_Cause 0x06			//����
#define Actcon_Cause 0x07		//����ȷ��
#define Actterm_Cause 0x0a	//������ֹ
#define Introgen_Cause 0x14	//��Ӧվ���ٻ�
#define Intro1_Cause	0x15	//��Ӧ��1���ٻ�
#define Intro2_Cause	0x16	//��Ӧ��2���ٻ�
#define Intro9_Cause	0x1d	//��Ӧ��9���ٻ�
#define Intro10_Cause 0x1e	//��Ӧ��10���ٻ�
/************************************************101��Լ��Ϣ���ַ���*****************************************************/
#define State_StartAddress 0x65			//ң����ʼ��ַ
#define Value_StartAddress 0x4001			//ң����ʼ��ַ
#define Tele_RegulationAddress 0x6201	//ң����ʼ��ַ
#define Tele_ControlAddress 0x6001		//ң����ʼ��ַ

/********************************************101���ٻ���Ϣ������*********************************************************/
#define TotalCall_Info 0x14	//���ٻ�

//******************************************�궨���ⲿ����********************************************************//
//��ʱ���Ա���
UInt8 Balance101_Mask_temp = 1;
UInt8 QLY_LinkAddress_Len_temp = 2;
//����101ͨѶģʽ��ƽ��/��ƽ��
#define Balance101_Mask	1	//�궨���ⲿ���������ⲿ��־�ڴ��滻��0Ϊ��ƽ�⣬1Ϊƽ��
//�궨��101��·��ַ�ֽ���
#define QLY_LinkAddress_Len QLY_LinkAddress_Len_temp 


//����RTU�ն˺�����·��ַ
#define RTU_Numble 1//�궨���ն˺�
#define Link_Address 1	//�궨����·��ַ
#define ASDU_Address 2 //ASDU������ַ
#define Wait101Frame_TimeOut 3//�ȴ���ʱ����λ����
#define Resend_Max_Frames 3 //����ط�����
//*******************************���建����*********************************************************************//
//����101�������ͻ�����������������Ӷ�����
//����֡�ķ���Ϊ����վ���Ӷ�վ��ȷ��֡�ķ���Ϊ�Ӷ�վ������վ
UInt8 QLY_Last_101_Request_Frame[256];//���һ������֡�����ڳ�ʱ�ط�
UInt8 QLY_Last_101_Answer_Frame[256];//���һ��ȷ��֡������FCB�ж��ط�
UInt8 QLY_ReciveFrame_temp[257];//���ڴ�ŵȴ���Ӧ�ڼ��յ�������֡,��һ���ֽ�������ű�־λ

#else
#define global extern
#endif
typedef enum
{
    low=0,
    high
}Qconbit;
//����flash���������ݽṹ
struct _APP_Grade_Parameter_Type{
	UInt8 	status;	//��������״̬��������Flash��ȡ��״ֵ̬
	UInt8 	count;    //ִ��APPӦ�ó���Ĵ���
	UInt8 	flag;    //0--��ʾִ��APP1; 1--��ʾִ��APP2
	UInt8  bootver; //bootloader�汾�š�	
	UInt8  appver;	 //Ӧ�ó���汾�š�	
	UInt8 	dbuffer[10];//Ԥ���ֽڡ�
	UInt16 crc;			//���������ݽ��е�16λCRCУ��ֵ��
};
typedef struct _APP_Grade_Parameter_Type APP_Grade_Parameter_Type;

//����ָʾ���ϱ�����
struct _Indicator_Spont_Trans_Type{
	UInt16 id;	//ָʾ����
	UInt16 fault_code;//������
	UInt16 fault_value;//����ֵ
};
typedef struct _Indicator_Spont_Trans_Type Indicator_Spont_Trans_Type;
//����101��Լ���Ʊ�־�����
//global struct{

#define BL101_PointCnt	255	//����ң�ŵ���
#define BL101_ValueCnt	255	//����ң�����
global UInt8 BL101_Point_Table[BL101_PointCnt];//ң�ŵ�ƫ�Ʊ�ͨ��Ĭ��˳�����ƫ��
global UInt8 BL101_Value_Table[BL101_ValueCnt];//ң���ƫ�Ʊ�
////�����շ�����������UCOSIϵͳ��˴�ӦΪָ��
//global UInt8 QLY_recive_101buff[256];
//global UInt8 QLY_send_101buff[256];

//�����ⲿ���ýӿں���
UInt8 QLY_101FrameProcess(LOCAL_Module *sys_mod_p,UInt8* p_buff1);//101��Լ���Ĵ�������buff1����֡
//UInt8 QLY_BL101_SpontTrans(Indicator_Spont_Trans_Type *p_buff1);//ͻ�����ã�����ƽ��101ͻ������//�ڲ�ʹ����һ���������������
UInt8 QLY_BL101_SpontTrans(void);//ͻ�����ã�����ƽ��101ͻ������//�ڲ�ʹ����һ���������������
UInt8 QLY_101_FrameSend(UInt8* p_buff);

UInt8 QLY_101Frame_1s_RsendCheck(int signo);//ƽ��101�ط����ƣ�1�����һ�Σ����뱨�Ĵ���������
extern unsigned char BL101_CheckSum(unsigned short getcnt,unsigned char length,  unsigned char flag);
extern UInt8 QLY_101FrameLen(UInt8* p_buff);

#endif

