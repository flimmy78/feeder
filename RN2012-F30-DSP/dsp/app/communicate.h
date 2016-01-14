#ifndef _COMMUNICATE_H_
#define _COMMUNICATE_H_
#if defined (__cplusplus)
extern "C" {
#endif
/****************************** include ***.h ************************************/
#include "IPCServer.h"
/****************************** macro define ************************************/
#define MAX_INDEX					21
#define YZ_DELAY					30000		//30*1000 = 30s

#define YC_CHECK_PAR_BASE         4000		//�洢ң�������У׼������float
#define PRINTF_BASE				6000	   	//����DSP�Ĵ�ӡ����
#define YC_OFFSET					800			//100*8 100������ ÿ��ռ8�ֽ�

#define YX_OFFSET      			3200u		//ң�ű����ַ
#define YK_OFFSET					3300u		//ң�����õ�ַ
#define SYS_OFFSET					3400u		//ϵͳ������Ϣ��ַ
#define FA_OFFSET					3464u		//FA������Ϣ��ַ

#define YX_STEP						20			//ң��״̬��ң��ʹ�ܵ�ƫ��  20
#define YC_Byte						2			//8���ֽ� ����λ���� ����λԤ�� 2 = 8/4 ƫ���ǻ���һ���ֽڼ���  ���ǻ���ַ��UInt32* �͵�ַ 
#define YX_Byte						1			// 1���ֽ� bit[0] ����״̬ bit[1] ʹ�� bit[2] cosʹ�� bit[3] soeʹ�� bit[4] ��ת
#define YX_Num						96			// ң�ŵ����

#define YX_Enable      			0x02		//bit[1] Ϊʹ�ܱ�־
#define YX_COS						0x04		//bit[2] ΪCOSʹ��
#define YX_SOE						0x08		//bit[3] ΪSOEʹ��
#define YX_Not						0x10		//bit[4] Ϊ��ת

#define YK_OPEN_SEL                0x01  		//��ѡ��
#define YK_CLOSE_SEL               0x02		//�ر�ѡ��
#define YK_CANCEL_OPEN_SEL        0x03  		//ȡ����ѡ��
#define YK_CANCEL_CLOSE_SEL       0x04   		//ȡ���ر�ѡ��
#define YK_OPEN_EXECUTIVE         0x05  		//ִ�д򿪶���
#define YK_CLOSE_EXECUTIVE        0x06   		//ִ�йرն���


/****************************** type define ************************************/
/* ��������ƫ�Ƶ�ַ�ṹ�� */
typedef struct _BaseValuePtr_
{
	UInt16 module;							//ģֵ
	UInt16 angle;							//���
}BaseValuePtr;
/* ң�������ṹ�� */
typedef struct _Board_Param_
{
	BaseValuePtr Ua;							//a���ѹ ���
	BaseValuePtr Ub;							//b���ѹ ���
	BaseValuePtr Uc;							//c���ѹ ���
	BaseValuePtr U0;							//�����ѹ ���
	BaseValuePtr Ib1;
	BaseValuePtr Ic1;
	BaseValuePtr Ia2;
	BaseValuePtr Ib2;
	BaseValuePtr Ic2;
	BaseValuePtr Ia3;
	BaseValuePtr Ib3;
	BaseValuePtr Ic3;
	BaseValuePtr reserve1;
	BaseValuePtr reserve2;
}BoardParamStr;
/* �ϴ������Ļ����洢�ṹ */
typedef struct _BaseStr_
{
	float Param;						//����ֵ
	float Reserve;						//Ԥ������λ
}ParBaseStr;
/* ң������б� */
struct _Param_List_
{
	ParBaseStr Ua1;						//A���ѹ
	ParBaseStr Ub1;						//B���ѹ
	ParBaseStr Uc1;						//C���ѹ
	ParBaseStr U01;						//0���ѹ
	ParBaseStr Ia1;						//A�ߵ���
	ParBaseStr Ib1;						//B�ߵ���
	ParBaseStr Ic1;						//C�ߵ���
	ParBaseStr I01;						//�������
	ParBaseStr Pa1;						//A���й�
	ParBaseStr Pb1;						//B���й�
	ParBaseStr Pc1;						//C���й�
	ParBaseStr Qa1;						//A���޹�
	ParBaseStr Qb1;						//B���޹�
	ParBaseStr Qc1;						//C���޹�
	ParBaseStr P1;						//ȫ���й�
	ParBaseStr Q1;						//ȫ���޹�
	ParBaseStr Cos1;					//��������
	ParBaseStr DC1;						//DC1(���ص�ѹ)
	ParBaseStr DC2;						//DC2(װ���ڲ��¶�)
	ParBaseStr F1;						//��Ƶ
	ParBaseStr H1;						//ʪ�� �ṩ�ù��� ����δʹ��
};
typedef struct  _Param_List_  CurrentPaStr;
/* �����ַ�ṹ�� */
typedef struct _Share_Addr_
{
	UInt32* base_addr;					//����������ַ��Ҳ��ң�����ݹ���������ַ
	UInt32* ykconf_addr;				//ң��������Ϣ��ַ
	UInt32* sysconf_addr;				//ϵͳ������Ϣ��ַ
	UInt32* faconf_addr;				//FA������Ϣ��ַ
	UInt32* digitIn_addr;				//ң�Ź�������ַ
	UInt32* digitIn_EN_addr;			//ң��ʹ�ܱ�־��ַ
	UInt32* digitIn_COS_addr;			//ң��COS��־��ַ
	UInt32* digitIn_SOE_addr;			//ң��SOE��־��ַ
	UInt32* digitIn_NOT_addr;			//ң�ŷ�ת��־��ַ
	float*  anadjust_addr;				//ң��У׼������ַ
	UChar*  printf_addr;				//��ӡ��ַ
}ShareAddrBase;	
/* �������ʹ�ܲ����ṹ�� */
typedef struct _DianBiao_Data_
{
	UInt32 yxen;						//yxʹ��
	UInt32 yxcos;						//yx cos
	UInt32 yxsoe;						//yx soe 
	UInt32 yxnot;						//yx ȡ��
	UInt32 yken;						//yk ʹ��
}DianBiaoStr;
/* YZ ���� */
typedef struct _YZ_DATA_
{
	UInt8    YZFlag;						//ң��Ԥ�ñ�־
	UInt32	 YZDelay;						//ң��Ԥ����ʱ
}yz_data;	


/******************************* declar data ***********************************/
extern ShareAddrBase ShareRegionAddr;	//��������ַ�ṹ�� ȫ�ֱ���
//extern ParameterListStr* ParListPtr;	//ң������б� ������ѹֵ �������� ���� �ܺĵ�
extern UInt16 findindex[8];			//������
extern DianBiaoStr dianbiaodata;		//�������
extern yz_data yzdata;					//Ԥ������
/******************************* functions **************************************/
//Void Init_BaseParamAddr(BaseValueType output[][7]);
//Void DigTurn2ShareRegion(DIGRP_Struct* olddigbuff, DIGRP_Struct* newdigbuff);
Void Init_ShareAddr(UChar* ptr);
Int DigAdjust(App_Msg *msg, float *adjust);
Int16 DigCmdout(App_Msg *msg);
Void InitAnaInAddr(void);
Int ReadConfig(Void);


#if defined (__cplusplus)
}
#endif /* __COMMUNICATE_H__ */
#endif /* __COMMUNICATE_H__ */
