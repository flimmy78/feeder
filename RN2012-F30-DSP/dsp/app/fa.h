/*************************************************************************/
/*
 * Copyright (c) 2015, 郑州瑞能电气 Runner All rights reserved.
 *
 * file       : fa.h
 * Design     : zlb
 * Email	  : longbiao831@qq.com
 * Data       : 2015-7-28
 * Version    : V1.0
 * Change     :
 * Description: FA保护结构体类型定义与函数部分声明头文件
 */
/*************************************************************************/
#ifndef _FA_H_
#define _FA_H_
#include <xdc/std.h>


//使能宏定义
#define Enable				1	//使能
#define Disable				0   //失能
//FTU状态
#define NOPOWER				0	//无压无流
#define RUNNING				1	//运行状态
#define PROTECT				2	//保护状态
#define LOCKOUT				3	//闭锁状态

// 软遥信信号
#define ACTION				10	//保护动作
#define ALARM				11	//保护装置告警
#define ALLERR				12	//事故总
#define PTNOPOWER			13	//交流失电
#define CTNOPOWER			14	//CT有流事件
#define YUEXBASE			15	//越限保护事件
#define RECLOSE				16	//重合闸
//
#define GLBASE				13	//过流一段事件 基础事件
#define GLSECOND			14  //过流二段事件
#define GLTHIRD				15  //过流三段事件
#define LXGLBASE			16	//零序过流一段事件 基础事件
#define LXGLSECOND			17  //零序过流二段事件
#define LXGLTHIRD			18  //零序过流三段事件

/* 遥测越限保护参数 */
struct _YCOVERPARAM_
{
	UInt16 ycindex;				//遥测编号
	UInt16 flag;				//越限标志 上限1  下限2
	UInt16 value;				//越限值
};
/* 分段式保护参数 */
struct _PROTECTPARM_
{
	UInt32 protectvalue;		//过流值A
	UInt32 delaytime;			//过流时限ms(最小20ms)
};
/* sys.conf配置结构体 */
struct _SYSPARAM_
{
	UInt16 yc1_out;				//遥控1输出时间ms
	UInt16 yc2_out;				//遥控2输出时间ms
	UInt16 yxtime;				//遥信去抖时间ms
	UInt16 ptrate;				//PT变比
	UInt16 ctrate;				//CT变比
	UInt16 pctrate;				//保护CT变比
	UInt16 battlecycle;		//蓄电池活化周期，单位日
	UInt16 battletime;			//蓄电池活化时间，单位s
}; 
/* FA保护参数 */
/* 软压板备注:bit[0]A项电压检测 bit[1]B项电压检测 bit[2]C项电压检测					*/
/* bit[3]单项接地检测 bit[4]A项过流检测  bit[5]B项过流检测  bit[6]C项过流检测 		*/
/* bit[7]零序过流检测  bit[8]小接地电流系系统  bit[9]开关自带保护 bit[10]过流跳闸	*/
/* bit[11]过流失压跳闸  bit[12]变电站一次重合闸失败后跳闸 bit[13]重合闸容许   		*/
/* bit[14]过流上报 bit[15]变电站跳闸后上报 bit[16]变电站一次重合闸失败后上报		*/
/* bit[17]故障事项COS表示 bit[18]故障事项SOE表示 									*/
struct _FAPARAM_
{
	UInt16  loop;				//选择回路
	UInt16  lowcur;				//失流限值A
	UInt16  lowvol;				//失压限值V
	UInt16  remotenum;			//遥控编号
	UInt16  overvol;			//过压限值V
	UInt16  problmeyx;			//故障遥信清除延时S
	UInt16  fastdelay;			//速断保护延时ms
	UInt16  nopower;			//无压无流确认时间ms
	UInt16  yl_t;				//励磁涌流屏蔽时间ms
	UInt16  recov_t;			//从过流态恢复时间
	UInt16  recloseok_t;		//确认出口重合成功时间ms
	UInt16  lockreclose_t;		//确认出口跳闸闭锁时间ms
	UInt16  ykok_t;				//开关跳闸确认时间ms
	UInt16  gzlbq_t;			//故障前录波时间ms
	UInt16  gzlbh_t;			//故障后录波时间ms
	UInt16  reclose_n;			//重合闸次数(<=3)
	UInt16  chargetime;		//重合闸充电时间ms
	UInt16  fistreclose_t;		//一次重合闸时间ms
	UInt16  secondreclose_t;	//二次重合闸时间ms
	UInt16  thirdreclose_t;	//三次重合闸时间ms
	UInt32  softenable;		//软压板
	UInt16  cursection_n;		//分段过流次数
	UInt16  zerosection_n;		//零序过流次数
	UInt16  ycover_n;			//遥测越限保护个数
	/* 这里的参数全部按照最大参数开辟空间，满足最大需求 */
	struct _PROTECTPARM_ cursection[3];
								//分段过流参数 三段过流 如果需要更多段数可以在这里添加 
	struct _PROTECTPARM_ zerosection[3];
								//零序过流参数
	struct _YCOVERPARAM_ ycover[3];
								//遥测越限参数 最大为16个数组
};

typedef struct _SYSPARAM_ SYSPARAMSTR;
typedef struct _FAPARAM_ FAPARAMSTR;
typedef struct _PROTECTPARM_ PROTECTSTR;
typedef struct _YCOVERPARAM_ YCOVERSTR;

Void FA_Task(UArg arg0, UArg arg1);
Void Tempture(UArg arg0, UArg arg1);
void Send_CS(UChar num,UChar status);



#endif/* _FA_H_ */

