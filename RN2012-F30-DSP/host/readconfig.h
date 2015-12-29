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

/* 遥测越限保护参数 */
struct _YCOVERPARAME_
{
	short ycindex;				//遥测编号
	short flag;					//越限标志 上限1  下限2
	short value;				//越限值
};
typedef struct _YCOVERPARAME_ YCOVERSTRM;

/* 分段式保护参数 */
struct _PROTECTPAME_
{
	int protectvalue;			//过流值A
	int delaytime;				//过流时限ms(最小20ms)
	//struct _PROTECTPARM_ * next;
};
typedef struct _PROTECTPAME_ PROTECTSTRM;


/* FA保护参数 */
struct _FAPRMETER_
{
    
	short  loop;				//选择回路2
	short  lowcur;				//失流限值A4
	short  lowvol;				//失压限值V6
	short  remotenum;			//遥控编号8
	short  overvol;				//过压限值V10
	short  problmeyx;			//故障遥信清除延时S12
	short  fastdelay;			//速断保护延时ms14
	short  nopower;				//无压无流确认时间ms16
	short  yl_t;				//励磁涌流屏蔽时间ms18
	short  recov_t;				//从过流态恢复时间20
	short  recloseok_t;		    //确认出口重合成功时间ms22
	short  lockreclose_t;		//确认出口跳闸闭锁时间ms24
	short  ykok_t;				//开关跳闸确认时间ms26
	short  gzlbq_t;				//故障前录波时间ms28
	short  gzlbh_t;				//故障后录波时间ms30
	short  reclose_n;			//重合闸次数(<=3)32
	short  chargetime;			//重合闸充电时间ms34
	short  fistreclose_t;		//一次重合闸时间ms36
	short  secondreclose_t;	    //二次重合闸时间ms38
	short  thirdreclose_t;		//三次重合闸时间ms40
	int    softenable;			//软压板
	short  cursection_n;		//分段过流次数42
	short  zerosection_n;		//零序过流次数44
	short  ycover_n;			//遥测越限保护个数46
	PROTECTSTRM cursection[3];   //分段过流参数
	PROTECTSTRM zerosection[3];  //零序过流参数
	YCOVERSTRM  ycover[3];       //遥测越限参数
};
typedef struct _FAPRMETER_ FAPARAM;

void   Write_Fa_Sharearea(struct _FAPRMETER_ * item);
extern void Read_Faconfig_Conf(char * fd,  struct  _FAPRMETER_  *  fa);
#endif
