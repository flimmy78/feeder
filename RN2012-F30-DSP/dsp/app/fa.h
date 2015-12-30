/*************************************************************************/
/*
 * Copyright (c) 2015, ֣�����ܵ��� Runner All rights reserved.
 *
 * file       : fa.h
 * Design     : zlb
 * Email	  : longbiao831@qq.com
 * Data       : 2015-7-28
 * Version    : V1.0
 * Change     :
 * Description: FA�����ṹ�����Ͷ����뺯����������ͷ�ļ�
 */
/*************************************************************************/
#ifndef _FA_H_
#define _FA_H_
#include <xdc/std.h>


//ʹ�ܺ궨��
#define Enable				1	//ʹ��
#define Disable				0   //ʧ��
//FTU״̬
#define NOPOWER				0	//��ѹ����
#define RUNNING				1	//����״̬
#define PROTECT				2	//����״̬
#define LOCKOUT				3	//����״̬

// ��ң���ź�
#define ACTION				10	//��������
#define ALARM				11	//����װ�ø澯
#define ALLERR				12	//�¹���
#define PTNOPOWER			13	//����ʧ��
#define CTNOPOWER			14	//CT�����¼�
#define YUEXBASE			15	//Խ�ޱ����¼�
#define RECLOSE				16	//�غ�բ
//
#define GLBASE				13	//����һ���¼� �����¼�
#define GLSECOND			14  //���������¼�
#define GLTHIRD				15  //���������¼�
#define LXGLBASE			16	//�������һ���¼� �����¼�
#define LXGLSECOND			17  //������������¼�
#define LXGLTHIRD			18  //������������¼�

/* ң��Խ�ޱ������� */
struct _YCOVERPARAM_
{
	UInt16 ycindex;				//ң����
	UInt16 flag;				//Խ�ޱ�־ ����1  ����2
	UInt16 value;				//Խ��ֵ
};
/* �ֶ�ʽ�������� */
struct _PROTECTPARM_
{
	UInt32 protectvalue;		//����ֵA
	UInt32 delaytime;			//����ʱ��ms(��С20ms)
};
/* sys.conf���ýṹ�� */
struct _SYSPARAM_
{
	UInt16 yc1_out;				//ң��1���ʱ��ms
	UInt16 yc2_out;				//ң��2���ʱ��ms
	UInt16 yxtime;				//ң��ȥ��ʱ��ms
	UInt16 ptrate;				//PT���
	UInt16 ctrate;				//CT���
	UInt16 pctrate;				//����CT���
	UInt16 battlecycle;		//���ػ���ڣ���λ��
	UInt16 battletime;			//���ػʱ�䣬��λs
}; 
/* FA�������� */
/* ��ѹ�屸ע:bit[0]A���ѹ��� bit[1]B���ѹ��� bit[2]C���ѹ���					*/
/* bit[3]����ӵؼ�� bit[4]A��������  bit[5]B��������  bit[6]C�������� 		*/
/* bit[7]����������  bit[8]С�ӵص���ϵϵͳ  bit[9]�����Դ����� bit[10]������բ	*/
/* bit[11]����ʧѹ��բ  bit[12]���վһ���غ�բʧ�ܺ���բ bit[13]�غ�բ����   		*/
/* bit[14]�����ϱ� bit[15]���վ��բ���ϱ� bit[16]���վһ���غ�բʧ�ܺ��ϱ�		*/
/* bit[17]��������COS��ʾ bit[18]��������SOE��ʾ 									*/
struct _FAPARAM_
{
	UInt16  loop;				//ѡ���·
	UInt16  lowcur;				//ʧ����ֵA
	UInt16  lowvol;				//ʧѹ��ֵV
	UInt16  remotenum;			//ң�ر��
	UInt16  overvol;			//��ѹ��ֵV
	UInt16  problmeyx;			//����ң�������ʱS
	UInt16  fastdelay;			//�ٶϱ�����ʱms
	UInt16  nopower;			//��ѹ����ȷ��ʱ��ms
	UInt16  yl_t;				//����ӿ������ʱ��ms
	UInt16  recov_t;			//�ӹ���̬�ָ�ʱ��
	UInt16  recloseok_t;		//ȷ�ϳ����غϳɹ�ʱ��ms
	UInt16  lockreclose_t;		//ȷ�ϳ�����բ����ʱ��ms
	UInt16  ykok_t;				//������բȷ��ʱ��ms
	UInt16  gzlbq_t;			//����ǰ¼��ʱ��ms
	UInt16  gzlbh_t;			//���Ϻ�¼��ʱ��ms
	UInt16  reclose_n;			//�غ�բ����(<=3)
	UInt16  chargetime;		//�غ�բ���ʱ��ms
	UInt16  fistreclose_t;		//һ���غ�բʱ��ms
	UInt16  secondreclose_t;	//�����غ�բʱ��ms
	UInt16  thirdreclose_t;	//�����غ�բʱ��ms
	UInt32  softenable;		//��ѹ��
	UInt16  cursection_n;		//�ֶι�������
	UInt16  zerosection_n;		//�����������
	UInt16  ycover_n;			//ң��Խ�ޱ�������
	/* ����Ĳ���ȫ���������������ٿռ䣬����������� */
	struct _PROTECTPARM_ cursection[3];
								//�ֶι������� ���ι��� �����Ҫ�������������������� 
	struct _PROTECTPARM_ zerosection[3];
								//�����������
	struct _YCOVERPARAM_ ycover[3];
								//ң��Խ�޲��� ���Ϊ16������
};

typedef struct _SYSPARAM_ SYSPARAMSTR;
typedef struct _FAPARAM_ FAPARAMSTR;
typedef struct _PROTECTPARM_ PROTECTSTR;
typedef struct _YCOVERPARAM_ YCOVERSTR;

Void FA_Task(UArg arg0, UArg arg1);
Void Tempture(UArg arg0, UArg arg1);
void Send_CS(UChar num,UChar status);



#endif/* _FA_H_ */

