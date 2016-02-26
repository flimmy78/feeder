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
//定义global为空
#define global
//******************************************宏定义常量************************************************************//
//定义101规约帧头帧尾
#define Fix_frame_top 0x10
#define Var_frame_top 0x68
#define Normal_frame_end 0x16
/*************************************101规约功能码*********************************************************/
//定义启动方向控制域功能码
#define Reset_link_code 0x00	//复位远方链路
#define Test_link_code 0x02		//链路测试，心跳包使用
#define Trans_data_code 0x03	//传送数据
#define Call_linkstate_code 0x09	//请求链路状态
#define Call_level1_data_code 0x0a	//召唤一级数据
#define Call_level2_data_code 0x0b	//召唤二级数据

//定义从动方向控制域功能码
#define Acknowledge_code 0x00	//确认
#define Negate_code 0x01			//否定确认
#define Respond_by_data_code 0x08	//以数据响应请求帧
#define No_request_data_code 0x09	//无所召唤的数据
#define Respond_by_link_code 0x0b	//以链路状态或访问请求来回答请求帧

//**************************************************定义ASDU数据单元类型标示************************************/
//名称参考国网DL634.5.101-2002中对类型标示的定义
#define M_SP_TB_1 0x1e		//带长时标的单点信息
//#define C_SC_NA_1 0x2d		//单点命令
#define C_SE_NA_1 0x30		//设定值命令，归一化值
#define C_SE_NB_1	0x31		//设定值命令，标度化值
//#define M_EI_NA_1 0x46		//初始化结束
#define C_CD_NA_1	0x6a		//延时获得
/************************************************101规约传送原因******************************************************/
//名称参考国网DL634.5.101-2002中对传送原因的定义
#define Spont_Cause 0x03		//突发
#define Reg_Cause	0x05			//请求或被请求
#define Act_Cause 0x06			//激活
#define Actcon_Cause 0x07		//激活确认
#define Actterm_Cause 0x0a	//激活终止
#define Introgen_Cause 0x14	//响应站总召唤
#define Intro1_Cause	0x15	//响应第1组召唤
#define Intro2_Cause	0x16	//响应第2组召唤
#define Intro9_Cause	0x1d	//响应第9组召唤
#define Intro10_Cause 0x1e	//响应第10组召唤
/************************************************101规约信息体地址相关*****************************************************/
#define State_StartAddress 0x65			//遥信起始地址
#define Value_StartAddress 0x4001			//遥测起始地址
#define Tele_RegulationAddress 0x6201	//遥调起始地址
#define Tele_ControlAddress 0x6001		//遥控起始地址

/********************************************101总召唤信息体内容*********************************************************/
#define TotalCall_Info 0x14	//总召唤

//******************************************宏定义外部变量********************************************************//
//临时调试变量
UInt8 Balance101_Mask_temp = 1;
UInt8 QLY_LinkAddress_Len_temp = 2;
//定义101通讯模式，平衡/非平衡
#define Balance101_Mask	1	//宏定义外部变量，将外部标志在此替换，0为非平衡，1为平衡
//宏定义101链路地址字节数
#define QLY_LinkAddress_Len QLY_LinkAddress_Len_temp 


//定义RTU终端号与链路地址
#define RTU_Numble 1//宏定义终端号
#define Link_Address 1	//宏定义链路地址
#define ASDU_Address 2 //ASDU公共地址
#define Wait101Frame_TimeOut 3//等待超时，单位：秒
#define Resend_Max_Frames 3 //最大重发次数
//*******************************定义缓冲区*********************************************************************//
//定义101两个发送缓冲区，启动方向与从动方向
//请求帧的方向为启动站到从动站，确认帧的方向为从动站到启动站
UInt8 QLY_Last_101_Request_Frame[256];//最后一个请求帧，用于超时重发
UInt8 QLY_Last_101_Answer_Frame[256];//最后一个确认帧，用于FCB判定重发
UInt8 QLY_ReciveFrame_temp[257];//用于存放等待回应期间收到的请求帧,第一个字节用来存放标志位

#else
#define global extern
#endif
typedef enum
{
    low=0,
    high
}Qconbit;
//定义flash启动区数据结构
struct _APP_Grade_Parameter_Type{
	UInt8 	status;	//程序运行状态，从外置Flash读取的状态值
	UInt8 	count;    //执行APP应用程序的次数
	UInt8 	flag;    //0--表示执行APP1; 1--表示执行APP2
	UInt8  bootver; //bootloader版本号。	
	UInt8  appver;	 //应用程序版本号。	
	UInt8 	dbuffer[10];//预留字节。
	UInt16 crc;			//对以上数据进行的16位CRC校验值。
};
typedef struct _APP_Grade_Parameter_Type APP_Grade_Parameter_Type;

//定义指示器上报类型
struct _Indicator_Spont_Trans_Type{
	UInt16 id;	//指示器号
	UInt16 fault_code;//故障码
	UInt16 fault_value;//故障值
};
typedef struct _Indicator_Spont_Trans_Type Indicator_Spont_Trans_Type;
//定义101规约控制标志与变量
//global struct{

#define BL101_PointCnt	255	//定义遥信点数
#define BL101_ValueCnt	255	//定义遥测点数
global UInt8 BL101_Point_Table[BL101_PointCnt];//遥信点偏移表，通过默认顺序访问偏移
global UInt8 BL101_Value_Table[BL101_ValueCnt];//遥测点偏移表
////定义收发缓冲区，上UCOSI系统后此处应为指针
//global UInt8 QLY_recive_101buff[256];
//global UInt8 QLY_send_101buff[256];

//声明外部调用接口函数
UInt8 QLY_101FrameProcess(LOCAL_Module *sys_mod_p,UInt8* p_buff1);//101规约报文处理函数，buff1接收帧
//UInt8 QLY_BL101_SpontTrans(Indicator_Spont_Trans_Type *p_buff1);//突发调用，用于平衡101突发传输//内部使用了一个队列来缓冲故障
UInt8 QLY_BL101_SpontTrans(void);//突发调用，用于平衡101突发传输//内部使用了一个队列来缓冲故障
UInt8 QLY_101_FrameSend(UInt8* p_buff);

UInt8 QLY_101Frame_1s_RsendCheck(int signo);//平衡101重发机制，1秒调用一次，且与报文处理函数互斥
extern unsigned char BL101_CheckSum(unsigned short getcnt,unsigned char length,  unsigned char flag);
extern UInt8 QLY_101FrameLen(UInt8* p_buff);

#endif

