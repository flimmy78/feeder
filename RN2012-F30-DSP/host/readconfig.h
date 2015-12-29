#ifndef _READCONFIG_H
#define _READCONFIG_H

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


#define MODELFAFILE             "/config/fa.conf"

/* ң��Խ�ޱ������� */
struct _YCOVERPARAME_
{
	short ycindex;				//ң����
	short flag;					//Խ�ޱ�־ ����1  ����2
	short value;				//Խ��ֵ
};
typedef struct _YCOVERPARAME_ YCOVERSTRM;

/* �ֶ�ʽ�������� */
struct _PROTECTPAME_
{
	int protectvalue;			//����ֵA
	int delaytime;				//����ʱ��ms(��С20ms)
	//struct _PROTECTPARM_ * next;
};
typedef struct _PROTECTPAME_ PROTECTSTRM;


/* FA�������� */
struct _FAPRMETER_
{
    
	short  loop;				//ѡ���·2
	short  lowcur;				//ʧ����ֵA4
	short  lowvol;				//ʧѹ��ֵV6
	short  remotenum;			//ң�ر��8
	short  overvol;				//��ѹ��ֵV10
	short  problmeyx;			//����ң�������ʱS12
	short  fastdelay;			//�ٶϱ�����ʱms14
	short  nopower;				//��ѹ����ȷ��ʱ��ms16
	short  yl_t;				//����ӿ������ʱ��ms18
	short  recov_t;				//�ӹ���̬�ָ�ʱ��20
	short  recloseok_t;		    //ȷ�ϳ����غϳɹ�ʱ��ms22
	short  lockreclose_t;		//ȷ�ϳ�����բ����ʱ��ms24
	short  ykok_t;				//������բȷ��ʱ��ms26
	short  gzlbq_t;				//����ǰ¼��ʱ��ms28
	short  gzlbh_t;				//���Ϻ�¼��ʱ��ms30
	short  reclose_n;			//�غ�բ����(<=3)32
	short  chargetime;			//�غ�բ���ʱ��ms34
	short  fistreclose_t;		//һ���غ�բʱ��ms36
	short  secondreclose_t;	    //�����غ�բʱ��ms38
	short  thirdreclose_t;		//�����غ�բʱ��ms40
	int    softenable;			//��ѹ��
	short  cursection_n;		//�ֶι�������42
	short  zerosection_n;		//�����������44
	short  ycover_n;			//ң��Խ�ޱ�������46
	PROTECTSTRM cursection[3];   //�ֶι�������
	PROTECTSTRM zerosection[3];  //�����������
	YCOVERSTRM  ycover[3];       //ң��Խ�޲���
};
typedef struct _FAPRMETER_ FAPARAM;

void   Write_Fa_Sharearea(struct _FAPRMETER_ * item);
extern void Read_Faconfig_Conf(char * fd,  struct  _FAPRMETER_  *  fa);
#endif
