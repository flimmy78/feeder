#ifndef __MY_TYPE_H
#define __MY_TYPE_H
//自定义类型
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

//定义布尔类型
//定义故障类型
typedef enum{No=0,Yes=1}Qfault_Mark_type;
//定义指示器类型
typedef struct{
	UInt16 id;	//指示器号
	UInt16 normal_current;	//正常线路电流
	UInt16 faule_current;	//故障电流
	UInt16 battery_voltage;	//电池电压
	UInt16 temp;	//线路温度
	UInt16 to_ground_voltage_ratio;//对地电压比例
	
	Qfault_Mark_type refresh_mark;//指示器数据刷新标志
	
	Qfault_Mark_type voltage_mark;//线路是否有电压标示
	Qfault_Mark_type current_mark;//线路是否有电流标示
	Qfault_Mark_type short_mark;//短路标示
	Qfault_Mark_type earth_mark;//接地标示
	Qfault_Mark_type battery_mark;//电池标示
	Qfault_Mark_type Line_up_temp;//线路高温
	Qfault_Mark_type self_mark;//指示器自身标示
}indicator_type;
//定义指示器组类型
//	indicator_type A;	//A相
//	indicator_type B;	//B相
//	indicator_type C;	//C相
typedef struct{
	indicator_type P[4];//定义4相，其中零序没有使用
} Indicator_Group_Type;
//定义主站通信任务接收队列的数据类型
typedef enum{gprs101,indicator}buff_type;
typedef struct{
	buff_type type;
	UInt8 *buff;
}QSem_ServerTask_ReceiveType;
//定义433配置处理任务接收类型
typedef struct{
	UInt16 s_addr;//发送方地址
	UInt16 d_addr;//接收方地址
	UInt16 control_domain;//控制域
	UInt16 d_buffer[11];//故障值
}Receive433_Frame_Type;
//定义服务器地址数据类型
typedef struct{
	UInt8 ip[4];
	UInt16 port;
}Server_Address_TypeDef;
//定义串口数据接收数据结构
typedef struct{
	UInt8 *p_buf;
	UInt32 cnt;
}Comm_TypeDef;
//定义主动上送数据类型
typedef enum{mark_and_value,just_mark}Sudden_Event_Type;
typedef struct{
	Sudden_Event_Type type;
	UInt16 mark_address;
	Qfault_Mark_type mark;
	UInt16 value_address;
	UInt16 value;
}Spont_Trans_type;

#endif

