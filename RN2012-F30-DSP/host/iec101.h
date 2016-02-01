/*************************************************************************/
// iec101.h                                        Version 1.00
//
// 本文件是DTU2.0通讯网关装置的IEC60870-5-101规约处理程序
// 编写人  :R&N
// email		  :R&N1110@126.com
//  日	   期:2015.04.17
//  注	   释:
/*************************************************************************/
#ifndef    _IEC101_H
#define    _IEC101_H

#include <stdio.h>
#include <stdlib.h>

#include <ti/syslink/Std.h>     /* must be first */

/*****************  Header Defination **********/
#define IEC_DIR_RTU2MAS_BIT	        0x00  //Direction
#define IEC_DIR_MAS2RTU_BIT	        0x80  //Direction
#define IEC_FUNC_BIT        		0x0F  //Function
#define IEC_PRM_BIT         		0x40  //PRM
#define IEC_ACD_BIT         		0x20  //ACD
#define IEC_FCB_BIT         		0x20  //FCB
#define IEC_FCV_BIT         		0x10  //FCV

#define IECFRAMEERRSUM       	2
#define IECFRAMEERROVERTIME    8
#define IECFRAMEERR          		1
#define IECFRAMEOK            		0
#define IECFRAMEVAR           		3
#define IECFRAMEFIX		    		4
#define IECFRAMEFIXE5        		6
#define IECFRAMEERREND         	5
#define IECFRAMESEND 	    		7

/*****************  Type Defination **********/
//define COI
#define IEC_COI_TURNON    		0x00
#define IEC_COI_LCRESET   		0x01
#define IEC_COI_RMRESET   		0x02

#define M_SP_NA_1				0x01 /* Single-point information */
#define M_SP_TA_1				0x02 /* Single-point information with time tag */
#define M_DP_NA_1				0x03 /* Double-point information */
#define M_DP_TA_1				0x04 /* Double-point information with time tag */
#define M_ST_NA_1				0x05 /* Step position information */
#define M_ST_TA_1				0x06 /* Step position information with time tag */
#define M_BO_NA_1				0x07 /* Bitstring of 32 bit */
#define M_BO_TA_1				0x08 /* Bitstring of 32 bit with time tag */
#define M_ME_NA_1				0x09 /* Measured value,normalized value */
#define M_ME_TA_1				0x0a /* Measured value,normalized value with time tag */
#define M_ME_NB_1				0x0b /* Measured value,scaled value */
#define M_ME_TB_1				0x0c /* Measured value,scaled value with time tag */
#define M_ME_NC_1				0x0d /* Measured value,short floating point value */
#define M_ME_TC_1				0x0e /* Measured value,short floating point value with time tag */
#define M_IT_NA_1				0x0f /* 电能脉冲计数量 */
#define M_IT_TA_1				0x10 /* Integrated totals with time tag */
#define M_EP_TA_1				0x11 /* Event of protection equipment with time tag */
#define M_EP_TB_1				0x12 /* Packed start events of protection equipment with time tag */
#define M_PS_NA_1				0x14 /* Packet single-point information with status change dection */
#define M_ME_ND_1				0x15 /* Measured value,normalized value without quality descriptor */
#define M_EI_NA_1				0x46 /* End of initialization COI parameters are not evaluted */
#define P_SETTASK				0x20 /* 召唤定值 PC->RTU */
#define P_SETTANS 				0x21	 /* 回答定值 RTU->PC */
//define QOI
#define IEC_QOI_INTROGEN  		0x14
#define IEC_QOI_INTRO1    		0x15
#define IEC_QOI_INTRO2    		0x16
#define IEC_QOI_INTRO3    		0x17
#define IEC_QOI_INTRO4    		0x18
#define IEC_QOI_INTRO5    		0x19
#define IEC_QOI_INTRO6    		0x1A
#define IEC_QOI_INTRO7    		0x1B
#define IEC_QOI_INTRO8    		0x1C
#define IEC_QOI_INTRO9    		0x1D
#define IEC_QOI_INTRO10   		0x1E
#define IEC_QOI_INTRO11   		0x1F
#define IEC_QOI_INTRO12   		0x20
#define IEC_QOI_INTRO13   		0x21
#define IEC_QOI_INTRO14   		0x22
#define IEC_QOI_INTRO15   		0x23
#define IEC_QOI_INTRO16   		0x24
//define SIQ_BIT
#define IEC_SIQ_IV_BIT    		0x80
#define IEC_SIQ_NT_BIT    		0x40
#define IEC_SIQ_SB_BIT    		0x20
#define IEC_SIQ_BL_BIT    		0x10

//define Cause
#define IEC_CAUSE_CYC       	0x01
#define IEC_CAUSE_BACK      	0x02
#define IEC_CAUSE_SPONT    		0x03
#define IEC_CAUSE_INIT      	0x04
#define IEC_CAUSE_REQ       	0x05
#define IEC_CAUSE_ACT       	0x06
#define IEC_CAUSE_ACTCON    	0x07
#define IEC_CAUSE_DEACT     	0x08
#define IEC_CAUSE_DEACTCON  	0x09
#define IEC_CAUSE_ACTTERM   	0x0A
#define IEC_CAUSE_RETREM    	0x0B
#define IEC_CAUSE_RETLOC    	0x0C
#define IEC_CAUSE_FILE      	0x0D
#define IEC_CAUSE_INTROGEN      0x14

#define IEC_CAUSE_N_BIT         0x40

#define IEC101_M_ME_NA_1        9	//测量值，归一化值

#define IEC101BUFNUM			4	//分配6个缓冲 6*256
#define IEC101BUFLEN			256
#define IEC101YXINFONUMBER		32	//分配4个缓冲
#define IEC101SOENUMBER		    32	//分配4个缓冲
#define IEC101CONFFILE			"/config/guiyue101.conf"
#define IEC101LOGFFILE			"/config/log101.txt"


/* IEC 870-5 System information in control Direction */
#define C_RL_NA_1           		0x00
#define C_QC_NA_1           		0x08
#define C_RQ_NA_1	    			0x09
#define M_RQ_NA_1_LNKBUSY   	0x01
#define M_RQ_NA_1_LNKNOWORK 0x0E
#define M_RQ_NA_1_LNKOK     	0x0B
#define M_RQ_NA_1_LNKNOEND  	0x0F

#define C_P1_NA_1           		0x0A
#define C_P2_NA_1           		0x0B
#define M_RQ_NA_1_RESPOND   	0x08

#define C_SC_NA_1           		0x2d     //Single-Point YK
#define C_DC_NA_1           		0x2e     //Double-Point YK

#define C_IC_NA_1 				100 //召唤
#define C_CI_NA_1 				101 /* Counter Interrogation command */
#define C_RD_NA_1 				102 /* Read command */
#define C_CS_NA_1 				103 /* Clock synchronization command */
#define C_TS_NA_1 				104 /* Test command */
#define C_RP_NA_1 				105 /* Reset process command */

#define RST_LINK_BIT			0x01
#define END_INIT_BIT			0x02
#define SUPPORTE5_BIT   		0x04
#define SUMMON_BIT       			0x08
#define SUMMON_CONFIRM_BIT     0x10


/**********************local struct**********************************/
struct _IEC101Struct
{
int    logfd;
int   fd_usart;				//打开串口对应的文件
unsigned int  bitrate;			//bitrate=115200#对应的串口波特率
short  usRecvCont;			//接收到的数据
unsigned short  pBufGet;		//指向接收缓冲区的下标
unsigned short  pBufFill;		//指向接收缓冲区的下标
unsigned char * pSendBuf;
unsigned char  usart;			//usart=1#使用哪个串口作为101规约进口

unsigned char  mode;			//mode=1#模式:不平衡(0),平衡(1)
unsigned char  linkaddr;		//linkaddr=2#链路地址
unsigned char  linkaddrlen;		//linkaddrlen=2#链路地址长度
unsigned char  conaddrlen;		//conaddrlen=2#公共地址长度
unsigned char  cotlen;			//COTlen=2#传送原因长度
unsigned char  infoaddlen;		//infoaddlen=2#信息体地址长度
unsigned char  yctype;			//yctype=9#遥测类型:规一值(9),标度值也就是浮点(11),无品质规一值(21)
unsigned char  dir;				//dir=1#平衡模式DIR位(0或1,缺省0)
unsigned char  flag;			//bit0 远方复位链路请求复位标记
							//bit1表示初始化结束标记
							//bit2表示支持E5指令
							//bit3表示总召唤
};
typedef	struct _IEC101Struct  IEC101Struct;



/**********************extern function**********************************/
extern  void  *Iec101_Thread( void * data  );
extern int IEC101_Init_Buf(void);
extern int  IEC101_Free_Buf(void);
extern int  Uart_Thread( void );
extern void IEC101_Process_Message(UInt32 cmd, UInt32 buf, UInt32 data);
extern void IEC101_Senddata2pc(int fd, char *tmp_buf, UInt8 len);



#endif




