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

#define YC_CHECK_PAR_BASE         4000		//存储遥测参数，校准参数是float
#define PRINTF_BASE				6000	   	//用于DSP的打印数据
#define YC_OFFSET					800			//100*8 100个参量 每个占8字节

#define YX_OFFSET      			3200u		//遥信编译地址
#define YK_OFFSET					3300u		//遥控配置地址
#define SYS_OFFSET					3400u		//系统配置信息地址
#define FA_OFFSET					3464u		//FA配置信息地址

#define YX_STEP						20			//遥信状态到遥信使能的偏移  20
#define YC_Byte						2			//8个字节 低四位数据 高四位预留 2 = 8/4 偏移是基于一个字节计算  但是基地址是UInt32* 型地址 
#define YX_Byte						1			// 1个字节 bit[0] 开入状态 bit[1] 使能 bit[2] cos使能 bit[3] soe使能 bit[4] 反转
#define YX_Num						96			// 遥信点个数

#define YX_Enable      			0x02		//bit[1] 为使能标志
#define YX_COS						0x04		//bit[2] 为COS使能
#define YX_SOE						0x08		//bit[3] 为SOE使能
#define YX_Not						0x10		//bit[4] 为反转

#define YK_OPEN_SEL                0x01  		//打开选中
#define YK_CLOSE_SEL               0x02		//关闭选中
#define YK_CANCEL_OPEN_SEL        0x03  		//取消打开选中
#define YK_CANCEL_CLOSE_SEL       0x04   		//取消关闭选中
#define YK_OPEN_EXECUTIVE         0x05  		//执行打开动作
#define YK_CLOSE_EXECUTIVE        0x06   		//执行关闭动作


/****************************** type define ************************************/
/* 基本参量偏移地址结构体 */
typedef struct _BaseValuePtr_
{
	UInt16 module;							//模值
	UInt16 angle;							//相角
}BaseValuePtr;
/* 遥测板参数结构体 */
typedef struct _Board_Param_
{
	BaseValuePtr Ua;							//a相电压 相角
	BaseValuePtr Ub;							//b相电压 相角
	BaseValuePtr Uc;							//c相电压 相角
	BaseValuePtr U0;							//零序电压 相角
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
/* 上传参数的基本存储结构 */
typedef struct _BaseStr_
{
	float Param;						//参数值
	float Reserve;						//预留参数位
}ParBaseStr;
/* 遥测参数列表 */
struct _Param_List_
{
	ParBaseStr Ua1;						//A相电压
	ParBaseStr Ub1;						//B相电压
	ParBaseStr Uc1;						//C相电压
	ParBaseStr U01;						//0序电压
	ParBaseStr Ia1;						//A线电流
	ParBaseStr Ib1;						//B线电流
	ParBaseStr Ic1;						//C线电流
	ParBaseStr I01;						//零序电流
	ParBaseStr Pa1;						//A相有功
	ParBaseStr Pb1;						//B相有功
	ParBaseStr Pc1;						//C相有功
	ParBaseStr Qa1;						//A相无功
	ParBaseStr Qb1;						//B相无功
	ParBaseStr Qc1;						//C相无功
	ParBaseStr P1;						//全相有功
	ParBaseStr Q1;						//全相无功
	ParBaseStr Cos1;					//功率因数
	ParBaseStr DC1;						//DC1(蓄电池电压)
	ParBaseStr DC2;						//DC2(装置内部温度)
	ParBaseStr F1;						//工频
	ParBaseStr H1;						//湿度 提供该功能 但并未使用
};
typedef struct  _Param_List_  CurrentPaStr;
/* 共享地址结构体 */
typedef struct _Share_Addr_
{
	UInt32* base_addr;					//共享区基地址，也是遥测数据共享区基地址
	UInt32* ykconf_addr;				//遥控配置信息地址
	UInt32* sysconf_addr;				//系统配置信息地址
	UInt32* faconf_addr;				//FA配置信息地址
	UInt32* digitIn_addr;				//遥信共享区地址
	UInt32* digitIn_EN_addr;			//遥信使能标志地址
	UInt32* digitIn_COS_addr;			//遥信COS标志地址
	UInt32* digitIn_SOE_addr;			//遥信SOE标志地址
	UInt32* digitIn_NOT_addr;			//遥信反转标志地址
	float*  anadjust_addr;				//遥测校准参数地址
	UChar*  printf_addr;				//打印地址
}ShareAddrBase;	
/* 点表配置使能参数结构体 */
typedef struct _DianBiao_Data_
{
	UInt32 yxen;						//yx使能
	UInt32 yxcos;						//yx cos
	UInt32 yxsoe;						//yx soe 
	UInt32 yxnot;						//yx 取反
	UInt32 yken;						//yk 使能
}DianBiaoStr;
/* YZ 数据 */
typedef struct _YZ_DATA_
{
	UInt8    YZFlag;						//遥控预置标志
	UInt32	 YZDelay;						//遥控预置延时
}yz_data;	


/******************************* declar data ***********************************/
extern ShareAddrBase ShareRegionAddr;	//共享区地址结构体 全局变量
//extern ParameterListStr* ParListPtr;	//遥测参数列表 电流电压值 功率因数 功率 能耗等
extern UInt16 findindex[8];			//索引表
extern DianBiaoStr dianbiaodata;		//点表数据
extern yz_data yzdata;					//预置数据
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
