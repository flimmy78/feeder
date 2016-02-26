/******************************************************************************
* File Name          : DL_101.c
* Author             : 
* Version            : V1.0
* Description        : 
* Date               : 2015/10/19
* 最后修改日期：
*******************************************************************************/
#define _DL_101_C
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
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <net/if.h>
#include<pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <linux/rtc.h>
#include <semaphore.h>
#include <termios.h>


#include <ti/syslink/Std.h>     /* must be first */
#include <ti/ipc/SharedRegion.h>
#include <ti/ipc/MessageQ.h>
#include <ti/syslink/utils/IHeap.h>

#include "common.h"
#include "iec101.h"
#include "DL_101.h"
#include "my_type.h"
#include "sharearea.h"

//函数声明
void QLY_Nb101_total_call_reset(void);//非平衡101总召唤状态复位函数
//static UInt8 QLY_BL101_start_state(void);//平衡101链路开始判断函数
static void QLY_BL101_state_Reset(void);//平衡101复位函数
static  void  BL101_YX_Summonall( UInt8 code );
static LOCAL_Module         * sys_mod_p;
static unsigned char BL101_AP_ALL_YX( short send_num ,UInt8 control_code);//报告所有遥信数据
static unsigned char       	save_recv_data[IEC101BUFNUM * IEC101BUFLEN];
static short BL101_AP_ALL_YC( short send_num ,UInt8 control_code);//报告所有遥信数据
static  void  BL101_YC_Summonall( UInt8 code);

static void BL101_Single_YK( int frame_len ,UInt8 * p_buffs,UInt8 code);
static unsigned char BL101_YK_Sel(int frame_len,UInt8 * p_buffs);
static unsigned char BL101_YK_SelHalt(UInt8 * p_buffs);
static unsigned char BL101_AP_YKExec(int frame_len,UInt8 * p_buffs);
static void BL101_NegativeCON(int frame_len,UInt8 * p_buffs,UInt8 code);
static void BL101_PositiveCON(int frame_len,UInt8 * p_buffs,UInt8 code);
static void BL101_SendTime(int length,UInt8 * p_buffs,UInt8 control_code);
static UInt8 QLY_BL101_SpontTransData(UInt8 Type_Identification);
int timercount=0;
//变量声明
static enum 
{
    Bl101_NoneCall_Type,
    Bl101_TotaCall_Type,
    Bl101_OneGroupCall_Type,
    Bl101_TwoGroupCall_Type,
    Bl101_NineGroupCall_Type,
    Bl101_TenGroupCall_Type
}Bl101_Call_Type = Bl101_NoneCall_Type;//定义召唤类型
//static Spont_Trans_type *Sudden_fault; //指向处理中的突发事件
static UInt16 T1_time;//记录延时传递T1
//static UInt8 GPRS_Reset_Mark = 0;//GPRS模块重启标志，用于遥调设置GPRS参数后重启
//static UInt8 *Tele_Regulation_Frame;//临时指向处理中的遥调帧
struct _QLY_control_101{
	Qconbit SFCB;//发送帧计数位，每帧变位，平衡式101使用
	Qconbit RFCB;//帧计数位，每帧变位
	UInt8 RFCB_cnt;//帧计数累加，超过3复位
	Qconbit ACD;//子站要求传送1级数据位
	Qconbit DFC;//子站是否可以接收后续报文。0:yes  1:busy
	
	Qconbit Wait_Answer_mask;//等待回应标志。发出请求后此标志位置1，若超时时间到达，重发最后一帧请求报文
	UInt8 Wait101Frame_Times;//超时等待时间
	UInt8 Resend_Frame_cnt;//重发次数计数
	UInt8 Send101Frame_Interval;//发送间隔
};
struct _QLY_control_101 Q_control_101_data;

struct _QLY_control_101 *  Q_control_101 = &Q_control_101_data;


/******************************************************/
//函数功能：复位链路
//返回值：正确返回0，错误返回1
//说明：将101通信的所有状态初始化，可以在主站要求复位或者出现链路错误时调用
//修改日期：2015/10/16
/******************************************************/
UInt8 QLY_101_link_Init(void)
{
    Q_control_101->SFCB= 0;
	Q_control_101->RFCB = 0;
	Q_control_101->RFCB_cnt = 0;
	Q_control_101->DFC = 0;
	QLY_BL101_state_Reset();//平衡101状态复位
	return 0;
}
/*******************************************************报文发送函数*******************************************************************************/
//	1、发送函数:	UInt8 QLY_101_FrameSend(UInt8* p_buff) 
//	2、发送间隔：	UInt8 QLY_101_FrameInterval(void)
//
/******************************************************/
//函数功能：101报文长度计算
//返回值：正确返回长度，错误返回0
//参数：待发送的报文指针p_buff
//说明：用于回复主站召唤链路状态
//修改日期：2015/11/10
/******************************************************/
UInt8 QLY_101FrameLen(UInt8* p_buff)
{
	UInt8 Frame_Len;
	if(Fix_frame_top == (*p_buff))
	{
		Frame_Len = 5;
	}
	else if(Var_frame_top == (*p_buff))
	{
		Frame_Len = p_buff[1] + 6;
	}
	else
		return 0;
    
    return Frame_Len;
    
}	
/******************************************************/
//函数功能：报文间隔函数
//返回值：正确返回0，错误返回1
//参数：
//说明：用于两帧报文之间的间隔，如果主站需要，实现该函数
//修改日期：2015/11/05
/******************************************************/
/*
static UInt8 QLY_101_FrameInterval(void)
{
	//OSTimeDly(Global_Variable.SendFrame_Interval);
	return 0;
}*/
///********************************************************链路回复子程序****************************************************************************/
	
//平衡
//		1、固定帧长发送函数：	UInt8 QLY_BL101_FixFrame_Send(UInt8 Bl101_Function_Code,Qconbit PRM,Qconbit FCV,UInt8* p_buff)
//		2、可变帧长发送函数： UInt8 QLY_BL101_VarFrame_Send(UInt8 Bl101_Function_Code,UInt8 Type_Identification,UInt8 Trans_Cause)
//		3、平衡101发送突发帧: UInt8 QLY_BL101_SpontTransData(Spont_Trans_type *Sudden_fault,UInt8 Type_Identification)
//共用

/******************************************************/
//函数功能：发送一级数据
//返回值：正确返回0，错误返回1
//参数：待回复的报文指针，ACD,CAUSE原因
//说明：用于上传数据，可能为单次请求，或者总召唤
//修改日期：2015/10/19
/******************************************************/
UInt8 QLY_101send_one_level_Data(UInt8* p_buff,Qconbit acd,UInt8 cause)
{
	if(Balance101_Mask)
		;
	else
	Q_control_101->ACD = low;//ACD清零
	return 0;
}
/******************************************************/
//函数功能：发送二级数据
//返回值：正确返回0，错误返回1
//参数：待回复的报文指针，ACD,CAUSE
//说明：用于上传二级数据，可能为单次请求，或者总召唤
//修改日期：2015/10/19
/******************************************************/
UInt8 QLY_101send_two_level_Data(UInt8* p_buff,Qconbit acd,UInt8 cause)
{
	if(Balance101_Mask)
		;
	else
		;
	return 0;
}
/******************************************************/
//函数功能：平衡101发送固定帧
//返回值：正确返回0，错误返回1
//参数：功能码Bl101_Function_Code，PRM方向，FCB是否有效，发送数据的指针p_buff
//说明：用于发送固定帧，PRM为1为启动方向，0为从动方向。FCV为1为FCB有效，0为无效。p_buff为发送缓冲区的指针
//修改日期：2015/11/04
/******************************************************/
static UInt8 QLY_BL101_FixFrame_Send(UInt8 Bl101_Function_Code,Qconbit PRM,Qconbit FCV,UInt8* p_buff)
{
	UInt8* p_temp;
	UInt8 control_code = Bl101_Function_Code;
    //my_debug("control_code:%d",control_code);
	if(PRM)
	{
		p_buff = QLY_Last_101_Request_Frame;
		control_code |= 1<<6;
	}
	else
	{
		p_buff = QLY_Last_101_Answer_Frame;
	}
	if(FCV)
	{
		control_code |= 1<<4;
		Q_control_101->SFCB = (Qconbit)!Q_control_101->SFCB;
		control_code |= (Q_control_101->SFCB)<<5;
	}
	p_temp = p_buff;
	*p_temp++ = Fix_frame_top;
	*p_temp++ = control_code;
	*p_temp++ = pIEC101_Struct->linkaddr;
	*p_temp++ = p_buff[1] + p_buff[2];
	*p_temp = Normal_frame_end;
	//发送报文
	return(QLY_101_FrameSend(p_buff));
    //write(pIEC101_Structd->fd_usart,p_temp,QLY_101FrameLen(p_buff));
}
/**************************************************************************/
//函数说明:上报总召唤的所有数据
//输入:无
//输出:无
//编辑:R&N
/**************************************************************************/
static  void  BL101_YX_Summonall( UInt8 code )
{
    static short numyx=0;
    UInt8 codes=code;
	if(sys_mod_p->usYX_Num>=1)
     {
        if(numyx != sys_mod_p->usYX_Num)
        {
            my_debug("come 2 usYX_Num:%d",15);
            numyx = BL101_AP_ALL_YX(numyx,codes);
            return ;
        }
     }
    numyx = 0;
}

static  void  BL101_YC_Summonall( UInt8 code)
{
    static short numyc=0;
    UInt8 codes=code;
	if(sys_mod_p->usYC_Num>=1)
     {
        if(numyc != sys_mod_p->usYC_Num)//没有发送完成
        {
            numyc = BL101_AP_ALL_YC(numyc,codes);
            return;
        }
     }
    numyc = 0;
}

/******************************************************/
//函数功能：平衡101发送变长帧
//返回值：正确返回0，错误返回1
//参数：功能码Bl101_Function_Code，类型标示Type_Identification，传送原因Trans_Cause，发送数据的指针p_buff
//说明：用于发送变长帧，通过设置功能码，类型标示，传送原因，将p_buff指向要发送的数据
//修改日期：2015/11/04
/******************************************************/
static UInt8 QLY_BL101_VarFrame_Send(UInt8 Bl101_Function_Code,UInt8 Type_Identification,UInt8 Trans_Cause,UInt8* p_buff,UInt8* p_buffer)
{
//	APP_Grade_Parameter_Type Boot_Struct;//定义启动信息结构体
	UInt8* p_temp;
//    int data=0;
//    Point_Info   * pTmp_Point=NULL;
	UInt8 APDU_len,i,sum_value=0;
	UInt8 control_code = Bl101_Function_Code | 0x50;//FCV位为1，PRM为1
//	UInt8 SQ,Inf_Cnt,Tele_Regulation_Shift;//定义偏移地址
	UInt8 frame_len;
	Q_control_101->SFCB = (Qconbit)!Q_control_101->SFCB;
	control_code |= (Q_control_101->SFCB)<<5;
	
	p_buff = QLY_Last_101_Request_Frame;
	p_temp = p_buff;
	*p_temp++ = Var_frame_top;
	p_temp += 2;
	*p_temp++ = Var_frame_top;
    if(Type_Identification==M_SP_NA_1)
    {
        BL101_YX_Summonall(control_code);
    }else if(Type_Identification==M_ME_NC_1)
    {
        
        BL101_YC_Summonall(control_code);
    }else if(Type_Identification==C_SC_NA_1)
    {
        frame_len = QLY_101FrameLen(p_buffer);
        my_debug("frame_len:%d",frame_len);
        BL101_Single_YK(frame_len,p_buffer,control_code);
    }else if(Type_Identification==C_CS_NA_1)
    {
        frame_len = QLY_101FrameLen(p_buffer);
        my_debug("frame_len:%d",frame_len);
        BL101_SendTime(frame_len,p_buffer,control_code);
    }
	switch(Type_Identification)//根据类型标示填充不同的ASDU数据
	{
		case M_EI_NA_1://初始化结束
			*p_temp++ = control_code;
			*p_temp++ = pIEC101_Struct->linkaddr;
			*p_temp++ = M_EI_NA_1;
			*p_temp++ = 0x01;//可变结构限定词
			*p_temp++ = Trans_Cause;
            *p_temp++ = Trans_Cause>>8;
			*p_temp++ = ASDU_Address;
			*p_temp++ = ASDU_Address>>8;
			*p_temp++ = 00;
			*p_temp++ = 00;
			*p_temp++ = 00;
			*p_temp++ = 00;
			APDU_len = 3+9;//信息体个数 +固定长度
			break;
            
		case C_IC_NA_1://响应总召唤
			*p_temp++ = control_code;
			*p_temp++ = pIEC101_Struct->linkaddr;
			*p_temp++ = C_IC_NA_1;
			*p_temp++ = 0x01;//可变结构限定词
			*p_temp++ = Trans_Cause;
            *p_temp++ = Trans_Cause>>8;
			*p_temp++ = ASDU_Address;
			*p_temp++ = ASDU_Address>>8;
			*p_temp++ = 0x00;
			*p_temp++ = 0x00;
			*p_temp++ = 0x00;
			switch(Bl101_Call_Type)
			{
				case Bl101_TotaCall_Type://总召唤信息体
					*p_temp++ = 0x14;
					break;
				default:
					*p_temp++ =0x00;//这种情况不存在
					break;
			}
			APDU_len = 8+3+1;//信息体个数 +固定长度
			break;
         
		case C_CD_NA_1://延时获得
			*p_temp++ = control_code;
			*p_temp++ = pIEC101_Struct->linkaddr;
			*p_temp++ = C_CD_NA_1;
			*p_temp++ = 0x01;//可变结构限定词
			*p_temp++ = Trans_Cause;
            *p_temp++ = Trans_Cause>>8;
			*p_temp++ = ASDU_Address;
			*p_temp++ = ASDU_Address>>8;
			*p_temp++ = 0x00;
			*p_temp++ = 0x00;
            *p_temp++ = 0x00;
			*p_temp++ = (UInt8)((T1_time) & 0xff);//将T3time放入信息体
			*p_temp++ |= (UInt8)((T1_time)>>8);
			APDU_len = 8+3+2;//信息体个数 +固定长度
			break;
		default:
			return 1;
	}
	p_buff[1] = p_buff[2] = APDU_len;
	for(i=APDU_len;i>0;i--)//计算校验和
	{
		sum_value += p_buff[3+i];
	}
	*p_temp++ = sum_value;
	*p_temp = Normal_frame_end;
	//发送报文
	return(QLY_101_FrameSend(p_buff));
}
/**************************************************************************/
//函数说明: 反馈子站时间
//输入:帧长度
//输出::
//编辑:R&N1110	
//时间:2014.10.20
/**************************************************************************/
static void BL101_SendTime(int length,UInt8 * p_buffs,UInt8 control_code)
{
	unsigned short  usMSec;
	long  usUSec;				//微秒精度
	unsigned char  ucSec;
	unsigned char len1=0, len2=0, len3=0, len4=0;

    unsigned short recvcnt = 0;
	struct rtc_time  * stime=malloc(sizeof(struct rtc_time));
    UInt8 * buf;
    buf =  p_buffs;//接收到的数据
    UInt8 control_codes = control_code;
    
	 len1 = pIEC101_Struct->linkaddrlen;
	 len2 = len1 + pIEC101_Struct->cotlen;
	 len3 = len2 + pIEC101_Struct->conaddrlen;
	 len4 = len3  +pIEC101_Struct->infoaddlen;

	usMSec = (buf[recvcnt+length-8]<<8)|buf[recvcnt+length-9];

	ucSec = (unsigned char)(usMSec/1000);
	if(ucSec>59)
		return;
	usUSec = usMSec%1000; //这个是ms
	usUSec = usUSec *1000; //这个是us
	stime->tm_min = buf[recvcnt+length-7]&0x3f;
	if(stime->tm_min>59)
		return;

	stime->tm_hour = buf[recvcnt+length-6]&0x1f;
	if(stime->tm_hour>23)
		return;

	stime->tm_mday = buf[recvcnt+length-5]&0x1f;
	if(stime->tm_mday>31)
		return;

	stime->tm_mon = buf[recvcnt+length-4]&0x0f;
	if(stime->tm_mon>12)
		return;

	stime->tm_year = buf[recvcnt+length-3]&0x7F;
	if(stime->tm_year>99)
		return;
	Set_Rtc_Time(stime,  usUSec);	//设置RTC时间


	memset(pIEC101_Struct->pSendBuf, 0, length);
	 pIEC101_Struct->pSendBuf[0]= 0x68;
	 pIEC101_Struct->pSendBuf[1]= length-6;
	 pIEC101_Struct->pSendBuf[2]= pIEC101_Struct->pSendBuf[1];
	 pIEC101_Struct->pSendBuf[3]= 0x68;

	 pIEC101_Struct->pSendBuf[4]= control_codes;
	 pIEC101_Struct->pSendBuf[5]= pIEC101_Struct->linkaddr;
	 pIEC101_Struct->pSendBuf[len1+5]=C_CS_NA_1;
	 pIEC101_Struct->pSendBuf[len1+6]=0x01;		//可变结构体限定词
	 pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_ACTCON;//原因
	 pIEC101_Struct->pSendBuf[len2+7]=pIEC101_Struct->linkaddr; //站地址
	 pIEC101_Struct->pSendBuf[len3+7]=0;	//信息体地址
	 pIEC101_Struct->pSendBuf[len4+7]=buf[recvcnt+length-9]; //信息内容
	 pIEC101_Struct->pSendBuf[len4+8]=buf[recvcnt+length-8]; //信息内容
	 pIEC101_Struct->pSendBuf[len4+9]=stime->tm_min; //信息内容
	 pIEC101_Struct->pSendBuf[len4+10]=stime->tm_hour; //信息内容
	 pIEC101_Struct->pSendBuf[len4+11]=stime->tm_mday; //信息内容
	 pIEC101_Struct->pSendBuf[len4+12]=stime->tm_mon; //信息内容
	 pIEC101_Struct->pSendBuf[len4+13]=stime->tm_year; //信息内容
	 //pIEC101_Struct->pSendBuf[len4+14]=IEC101_CheckSum(4, length-2, IECFRAMESEND);//帧校验
	 	 pIEC101_Struct->pSendBuf[len4+14]=BL101_CheckSum(4, (len4+13-2), IECFRAMESEND);//帧校验

	 pIEC101_Struct->pSendBuf[len4+15]=0x16;
        write(pIEC101_Struct->fd_usart,  pIEC101_Struct->pSendBuf, len4+16);
	free(stime);
	Log_Frame(pIEC101_Struct->logfd,  pIEC101_Struct->pSendBuf,  0, len4+16, SEND_FRAME);
}

/**************************************************************************/
//函数描述:报告所有遥信数据
//输入:无
//输出:无
//编辑:R&N
/**************************************************************************/
static unsigned char BL101_AP_ALL_YX( short send_num ,UInt8 control_code)//报告所有遥信数据
{
	unsigned char len1=0, len2=0, len3=0;
    short len,j, k,i, send_over=send_num;
    UInt8 control_codes = control_code;
    Point_Info   * pTmp_Point=NULL;
    int data=0;
    
    
    if(pIEC101_Struct->fd_usart <=0)
        return -1;

	 len1 = pIEC101_Struct->linkaddrlen;
	 len2 = len1+pIEC101_Struct->cotlen;
	 len3 = len2+pIEC101_Struct->conaddrlen;
    
	 memset( pIEC101_Struct->pSendBuf, 0, IEC101BUFLEN);

     pIEC101_Struct->pSendBuf[4]=control_codes;
	 pIEC101_Struct->pSendBuf[5]= pIEC101_Struct->linkaddr;
	 pIEC101_Struct->pSendBuf[len1+5]=M_SP_NA_1;   //type
	 
	 if(sys_mod_p->Continuity & YC_NOTCONTINUE) 	//地址不连续
		pIEC101_Struct->pSendBuf[len1+6]= 0;        //可变结构体限定词
     else
		pIEC101_Struct->pSendBuf[len1+6]= IEC104_VSQ_SQ_BIT;//可变结构体限定词

	 pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_INTROGEN;//原因
	 pIEC101_Struct->pSendBuf[len1+len2+6]=(pIEC101_Struct->linkaddr); //站地址

    pTmp_Point = sys_mod_p->pYX_Addr_List_Head;
     if(send_over != 0)
    {
        for(j=0; j<send_over; j++)
			if(pTmp_Point->next !=NULL)
				pTmp_Point = pTmp_Point->next;
    }
    for(k=0; k<(sys_mod_p->usYX_Num-send_over); )		//发送多少次
	{
		len = 0;
		if(!(sys_mod_p->Continuity & YX_NOTCONTINUE))   //地址连续
		{
            for(i=0; i<pIEC101_Struct->infoaddlen ; i++)
            {
                pIEC101_Struct->pSendBuf[len3+7+len] = (unsigned char)((pTmp_Point->uiAddr>>(i*8))&0xff);
                len ++;
            }
		}
		for(j=0; ((j<sys_mod_p->usYX_Num-send_over)&&( len<240)&&( j<(1<<5))); j++)
		{
			if(sys_mod_p->Continuity & YX_NOTCONTINUE) 	//不连续地址
			{
                for(i=0; i<pIEC101_Struct->infoaddlen ; i++)
                {
                    pIEC101_Struct->pSendBuf[len3+7+len] = (unsigned char)((pTmp_Point->uiAddr>>(i*8))&0xff);
                    len ++;
                }
			}
			data =(int)Read_From_Sharearea(pTmp_Point->uiOffset, TYPE_YX_STATUS_BIT);//遥信值
			if(data&(1<<(pTmp_Point->uiOffset&0x7)))
               pIEC101_Struct->pSendBuf[len3+7+len++] = 0x1;            //品质因数
            else
               pIEC101_Struct->pSendBuf[len3+7+len++] = 0x0;            //品质因数

//			pIEC101_Struct->pSendBuf[len3+7+len++] = 0x00;	//品质因数
			if(pTmp_Point->next !=NULL)
				pTmp_Point = pTmp_Point->next;
		}
		send_over = send_over +j;
		my_debug("usYX_Num:%d send_over:%d j:%d", sys_mod_p->usYX_Num, send_over, j);
		if(sys_mod_p->Continuity & YX_NOTCONTINUE)   //不连续
			pIEC101_Struct->pSendBuf[len1+6]  = j;
		else
			pIEC101_Struct->pSendBuf[len1+6]  =IEC104_VSQ_SQ_BIT|j;

    	 pIEC101_Struct->pSendBuf[len3+7+len]=BL101_CheckSum(4, len3+7+len, IECFRAMESEND);//帧校验
    	 pIEC101_Struct->pSendBuf[len3+8+len]=0x16;

    	 pIEC101_Struct->pSendBuf[0]= 0x68;
    	 pIEC101_Struct->pSendBuf[1]= len3+3+len;
    	 pIEC101_Struct->pSendBuf[2]= pIEC101_Struct->pSendBuf[1];
    	 pIEC101_Struct->pSendBuf[3]= 0x68;

        write(pIEC101_Struct->fd_usart,  pIEC101_Struct->pSendBuf, len3+9+len);
	    Log_Frame(pIEC101_Struct->logfd,  pIEC101_Struct->pSendBuf,  0, len3+9+len, SEND_FRAME);
        return send_over;
	}

return 0;

}
/**************************************************************************/
//函数:static short IEC101_AP_ALL_YC( short send_num )
//描述:报告所有遥测数据,发送浮点数
//输入:已经发生的遥测的个数
//输出:已经发生的遥测的个数
//编辑:R&N
//时间:2015.5.27
/**************************************************************************/
static short BL101_AP_ALL_YC( short send_num ,UInt8 control_code)//报告全部遥测数据
{
	unsigned char len1=0, len2=0, len3=0;
    short len,j, k,i, send_over=send_num;
    Point_Info   * pTmp_Point=NULL;
    float data=0;
    UInt8 control_codes = control_code;

    if(pIEC101_Struct->fd_usart <=0)
        return-1;

	 len1 = pIEC101_Struct->linkaddrlen;
	 len2 = len1+pIEC101_Struct->cotlen;
	 len3 = len2+pIEC101_Struct->conaddrlen;

	 memset( pIEC101_Struct->pSendBuf, 0, IEC101BUFLEN);

     pIEC101_Struct->pSendBuf[4]=control_codes;
	 pIEC101_Struct->pSendBuf[5]= pIEC101_Struct->linkaddr;
	 pIEC101_Struct->pSendBuf[len1+5]=M_ME_NC_1;   //type
	 if(sys_mod_p->Continuity & YC_NOTCONTINUE) 	//地址不连续
		pIEC101_Struct->pSendBuf[len1+6]= 0;        //可变结构体限定词
     else
		pIEC101_Struct->pSendBuf[len1+6]= IEC104_VSQ_SQ_BIT;//可变结构体限定词

	 pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_INTROGEN;//原因
	 pIEC101_Struct->pSendBuf[len1+len2+6]=pIEC101_Struct->linkaddr; //站地址

     pTmp_Point = sys_mod_p->pYC_Addr_List_Head;
     if(send_over != 0)
    {
        for(j=0; j<send_over; j++)
			if(pTmp_Point->next !=NULL)
				pTmp_Point = pTmp_Point->next;
    }

	for(k=0; k<(sys_mod_p->usYC_Num-send_over); )		//发送多少次
	{
		len = 0;
		if(!(sys_mod_p->Continuity & YC_NOTCONTINUE))   //地址连续
		{
            for(i=0; i<pIEC101_Struct->infoaddlen ; i++)
            {
                pIEC101_Struct->pSendBuf[len3+7+len] = (unsigned char)((pTmp_Point->uiAddr>>(i*8))&0xff);
                len ++;
            }
		}
		for(j=0; ((j<sys_mod_p->usYC_Num-send_over)&&( len<240)&&( j<(1<<5))); j++)
		{
			if(sys_mod_p->Continuity & YC_NOTCONTINUE) 	//不连续地址
			{
                for(i=0; i<pIEC101_Struct->infoaddlen ; i++)
                {
                    pIEC101_Struct->pSendBuf[len3+7+len] = (unsigned char)((pTmp_Point->uiAddr>>(i*8))&0xff);
                    len ++;
                }
			}
			data =(float)Read_From_Sharearea(pTmp_Point->uiOffset, TYPE_YC);//遥测值
            if(pIEC101_Struct->yctype == TYPE_FLOAT)//浮点值
            {
    			memcpy((UInt8 *)&pIEC101_Struct->pSendBuf[len3+7+len], (UInt8 *)&data,4);
    			len+=4;
            }
            else if(pIEC101_Struct->yctype == TYPE_GUIYI)//归一值
            {
            	pIEC101_Struct->pSendBuf[len1+5]= IEC101_M_ME_NA_1;
    			pIEC101_Struct->pSendBuf[len3+7+len++] = 0xff;
    			pIEC101_Struct->pSendBuf[len3+7+len++] = 0xff;
            }
			pIEC101_Struct->pSendBuf[len3+7+len++] = 0x00;	//品质因数
			if(pTmp_Point->next !=NULL)
				pTmp_Point = pTmp_Point->next;
		}
		send_over = send_over +j;
		my_debug("usYC_Num:%d send_over:%d j:%d", sys_mod_p->usYC_Num, send_over, j);
		if(sys_mod_p->Continuity & YC_NOTCONTINUE)   //不连续
			pIEC101_Struct->pSendBuf[len1+6]  = j;
		else
			pIEC101_Struct->pSendBuf[len1+6]  =IEC104_VSQ_SQ_BIT|j;

    	 pIEC101_Struct->pSendBuf[len3+7+len]=BL101_CheckSum(4, len3+7+len, IECFRAMESEND);//帧校验
    	 pIEC101_Struct->pSendBuf[len3+8+len]=0x16;

    	 pIEC101_Struct->pSendBuf[0]= 0x68;
    	 pIEC101_Struct->pSendBuf[1]= len3+3+len;
    	 pIEC101_Struct->pSendBuf[2]= pIEC101_Struct->pSendBuf[1];
    	 pIEC101_Struct->pSendBuf[3]= 0x68;

        write(pIEC101_Struct->fd_usart,  pIEC101_Struct->pSendBuf, len3+9+len);
	    Log_Frame(pIEC101_Struct->logfd,  pIEC101_Struct->pSendBuf,  0, len3+9+len, SEND_FRAME);
        return send_over;
	}

	return(0);
}

/**********************************************************************************/
//函数说明:校验函数
//输入:getcnt数据存储的地方    length  多少个校验   flag  帧结构
//输出 :校验结果
//编辑:R&N1110			
//时间:2014.10.22
/**********************************************************************************/
unsigned char BL101_CheckSum(unsigned short getcnt,unsigned char length,  unsigned char flag)
{
	unsigned char i, val=0;
	if(flag == IECFRAMEVAR)
	{
	  for(i=0; i<length; i++)
		val+=save_recv_data[(getcnt+i+4)&0x3FF];
	}
	else if(flag ==IECFRAMEFIX)
	{
	  for(i=0; i<length; i++)
		val+=save_recv_data[(getcnt+i+1)&0x3FF];
	}
	else if(flag ==IECFRAMESEND)
	{
	  for(i=0; i<length; i++)
		val += pIEC101_Struct->pSendBuf[i+getcnt];
	}

//	my_debug("jiaoyan:0x%x len:%d", val, length);
	return val;
};
/**************************************************************************/
//函数:static void IEC101_NegativeCON(int frame_len)
//说明:确认回复帧
//输入:帧长
//输出:无
//编辑:R&N
/**************************************************************************/
static void BL101_NegativeCON(int frame_len,UInt8 * p_buffs,UInt8 code)
{
	//char * buf =(char *)  save_recv_data;//接收到的数据
	
    UInt8 control_code=code;
	UInt8 * buf;
    buf =  p_buffs;//接收到的数据
	//unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;//当前帧数据的起点位置
	unsigned short recvcnt =0;
	unsigned char len1=pIEC101_Struct->linkaddrlen;
	unsigned short i=0;

     for(i=0; i<frame_len; i++)
        pIEC101_Struct->pSendBuf[i] = buf[recvcnt+i];
     pIEC101_Struct->pSendBuf[4]= control_code;
     //pIEC101_Struct->pSendBuf[4]= IEC_DIR_RTU2MAS_BIT;
     //pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_N_BIT | IEC_CAUSE_ACTCON;//原因
     pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_ACTCON;//原因
     pIEC101_Struct->pSendBuf[frame_len-2]=BL101_CheckSum(4,  frame_len-6, IECFRAMESEND);//帧校验
    
    write(pIEC101_Struct->fd_usart,  pIEC101_Struct->pSendBuf, frame_len);
	Log_Frame(pIEC101_Struct->logfd,  pIEC101_Struct->pSendBuf,  0, frame_len, SEND_FRAME);
    my_debug("BL101_NegativeCON!");
}
/**************************************************************************/
//函数:static void IEC101_PositiveCON(int frame_len)
//说明:确认回复帧
//输入:帧长
//输出:无
//编辑:R&N
/**************************************************************************/
static void BL101_PositiveCON(int frame_len,UInt8 * p_buffs,UInt8 code)
{
	//char * buf =(char *)  save_recv_data;//接收到的数据
	
    UInt8 control_code=code;
	UInt8 * buf;
    buf =  p_buffs;//接收到的数据
	//unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;//当前帧数据的起点位置
	unsigned short recvcnt =0;
	unsigned char len1=pIEC101_Struct->linkaddrlen;
	unsigned short i=0;

     for(i=0; i<frame_len; i++)
        pIEC101_Struct->pSendBuf[i] = buf[recvcnt+i];

     //pIEC101_Struct->pSendBuf[4]= IEC_DIR_RTU2MAS_BIT;
     pIEC101_Struct->pSendBuf[4]= control_code;
     pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_ACTCON;//原因
     pIEC101_Struct->pSendBuf[frame_len-2]=BL101_CheckSum(4,  frame_len-6, IECFRAMESEND);//帧校验

    write(pIEC101_Struct->fd_usart,  pIEC101_Struct->pSendBuf, frame_len);
	Log_Frame(pIEC101_Struct->logfd,  pIEC101_Struct->pSendBuf,  0, frame_len, SEND_FRAME);
    
    my_debug("BL101_PositiveCON!");
}

/******************************************************/
//函数功能：平衡101发送突发帧
//参数：p_buff1指示器数据，Time_Mask是否带时标：high带，low不带，Type_Identification类型标示
//返回值：正确返回0，错误返回1
//修改日期：2015/10/25
/******************************************************/

//static UInt8 QLY_BL101_SpontTransData(Spont_Trans_type *Sudden_fault,UInt8 Type_Identification)
static UInt8 QLY_BL101_SpontTransData(UInt8 Type_Identification)
{
//	UInt8 *p_temp = QLY_Last_101_Request_Frame;
//	UInt8 *p_buff = QLY_Last_101_Request_Frame;
	UInt8 control_code = Trans_data_code | 0x50;//FCV位为1，PRM为1;
	
	Q_control_101->SFCB = (Qconbit)!Q_control_101->SFCB;
	control_code |= (Q_control_101->SFCB)<<5;
	
    if(Type_Identification==M_SP_NA_1)
    {
        //my_debug("M_SP_NA_1 begin!");
        BL101_Process_COS(control_code);
    }else if(Type_Identification==M_SP_TB_1)
    {
        //my_debug("M_SP_TB_1 begin!");
        BL101_Process_SOE(control_code);
    }
    return 0;
}

/************************************************************101规约报文验证程序******************************************************************************/
//包括:
//			1、校验和程序
//			2、FCB位判定
/******************************************************/
//函数功能：固定长帧校验
//返回值：正确返回0，错误返回1
//修改日期：2015/10/16
/******************************************************/
UInt8 QLY_Fix101frame_check(UInt8* p_buff)
{
	UInt8 CRC_value;
	CRC_value = (*(p_buff+2)) + (*(p_buff+1));
	if(CRC_value == (*(p_buff+3)))//如果链路地址为单字节，偏移3，双字节偏移4
		return 0;
	else
	{
		my_debug("Fix101Frame Sum_value erro!");
		return 1;
	}
}
/******************************************************/
//函数功能：可变长帧校验
//返回值：正确返回0，错误返回1,两个长度不一致返回2
//修改日期：2015/10/16
/******************************************************/
UInt8 QLY_Var101frame_check(UInt8* p_buff)
{
	UInt8 CRC_value=0,buff_len;
	UInt8* p_temp = p_buff+4;//可变长ASDU数据从第五个字节开始
	//两次长度保持一致
	if(*(p_buff+1) != *(p_buff+2))
		return 2;
	
	buff_len = *(p_buff+1);
	for(;buff_len>0;buff_len--)//ASDU数据累加
	{
		CRC_value += *p_temp++;
	}
	if(CRC_value == *p_temp)//与校验值比较
		return 0;
	else
	{
		my_debug("Var101Frame Sum_value erro!");
		return 1;
	}
}
/******************************************************/
//函数功能：101帧校验
//返回值：正确返回0，错误返回1,可变帧两个长度不一致返回2
//说明：校验帧头，校验和，以及DTU号
//修改日期：2015/10/16
/******************************************************/
UInt8 QLY_101frame_check(UInt8* p_buff)
{
	UInt8 status = 0;
//	UInt16 Frame_RTUNumble;
	if(Fix_frame_top == *p_buff)//固定帧
	{
		status |= QLY_Fix101frame_check(p_buff);
	}
	else if(Var_frame_top == *p_buff)//可变帧
	{
		status |= QLY_Var101frame_check(p_buff);
	}
	else
	{
		my_debug("frame top erro !");//报文头错误
		status = 1;
	}
	return status;
}
/******************************************************/
//函数功能：取出101帧控制域
//返回值：控制域
//说明：erro错误为1，正确为0
//修改日期：2015/10/19
/******************************************************/
UInt8 QLY_get101frame_controltype(UInt8* p_buff)
{
	UInt8 control_type=0;
	if(Fix_frame_top == *p_buff)//如果是定长报文，第二个字节为控制域
	{
		control_type = *(p_buff+1);
        my_debug("Fix_control_type:0x%x",control_type);
	}
	else if(Var_frame_top == *p_buff)//如果是可变长报文，第五个字节是控制域
	{
		control_type = *(p_buff+4);
        my_debug("Var_control_type:0x%x",control_type);
	}
	return control_type;
}
/******************************************************/
//函数功能：101控制域FCB判定
//参数：控制域
//返回值：正确返回0，错误返回1
//说明：FCV有效，FCB每次翻转为正确，相同则开始计数，连续三次，复位链路
//修改日期：2015/10/19
/******************************************************/
UInt8 QLY_101frame_FCB_detect(UInt8 con_type)
{
	Qconbit status = low;
	if(!(con_type & (1<<4)))//如果FCV有效位为0
		return 0;
	if(con_type&(1<<5))//取出FCB位
		status = high;
	if(Q_control_101->RFCB != status)
	{
		Q_control_101->RFCB = status;
		Q_control_101->RFCB_cnt =0;
	}
	else
	{
		Q_control_101->RFCB_cnt++;//
		my_debug("FCB Erro Resend Last Frame!");
		//重发最后一条确认帧
		QLY_101_FrameSend(QLY_Last_101_Answer_Frame);
		return 1;//重发则这条报文不再需要处理
	}
	
	//如果FCB_cnt计数超过3，复位链路
	if(Q_control_101->RFCB_cnt > 3)
	{
		my_debug("FCB Erro Link Init!");
		QLY_101_link_Init();
		return 1;//复位后不用再处理报文
	}
	return 0;
}
/*************************************************************平衡101报文处理程序*********************************************************************/
//包括
//*****************************平衡****************************
//主程序
//							1、平衡101报文处理主程序：		UInt8 QLY_BL101FrameProcess(UInt8 *p_buff1)
//							2、平衡101报文功能解析程序：	UInt8 QLY_BL101frame_switch(UInt8* p_buff1,UInt8 con_type,UInt8* p_buff2)
//
//用于超时重发：
//							1、进入等待回应状态：					static UInt8 BL101_Requst_On()
//							2、退出等待回应状态：					static UInt8 BL101_Requst_Off()
//							3、平衡101超时重发函数：			UInt8 QLY_101Frame_1s_RsendCheck()
//链路流程控制：
//							1、平衡101链路报文处理:				static UInt8 QLY_BL101_link_manage(UInt8* p_buff1,UInt8 con_type,UInt8* p_buff2)
//							2、平衡101总召唤流程控制:			static UInt8 QLY_BL101_total_call(UInt8* p_buff1,UInt8 con_type,UInt8* p_buff2)
//
//
/******************************************************/
//函数功能：平衡101打开超时重发
//参数：无
//返回值：
//说明：
//修改日期：2015/11/3
/******************************************************/
static UInt8 BL101_Requst_On()
{
	Q_control_101->Wait_Answer_mask = high;
	Q_control_101->Wait101Frame_Times = Wait101Frame_TimeOut;
	return 0;
}
/******************************************************/
//函数功能：平衡101关闭超时重发
//参数：无
//返回值：
//说明：
//修改日期：2015/11/3
/******************************************************/
static UInt8 BL101_Requst_Off()
{
	Q_control_101->Wait_Answer_mask = low;//清除等待回应标志
	Q_control_101->Resend_Frame_cnt = 0;//重发计数清零
	return 0;
}
/******************************************************/
//函数功能：平衡101超时重发函数
//参数：无
//返回值：重发返回1，不重发返回0
//说明：需要1S调用一次，监测等待超时。与报文处理函数互斥
//修改日期：2015/11/3
/******************************************************/

UInt8 QLY_101Frame_1s_RsendCheck(int signo)
{
//	UInt8* p_buff;
//	static UInt8 link_test_delay = 0;
//	if(QLY_BL101_start_state())//如果链路没有连接，10秒发送一次链路请求
//	{
//		link_test_delay++;
//		if(link_test_delay > 10)
//		{
//			QLY_BL101_FixFrame_Send(Call_linkstate_code,high,low,p_buff);//发送请求链路状态
//			link_test_delay = 0;
//		}
//		return 0;
//	}
    timercount++;
    if(timercount%3==0)
    {
        Process_Signal(14);
        timercount=0;
    }
//    my_debug("QLY_101Frame_1s_RsendCheck");
	if(!Q_control_101->Wait_Answer_mask)//如果不在等待回应期间，直接返回0
		return 0;
	if(--Q_control_101->Wait101Frame_Times)//如果等待时间未到，返回0
		return 0;
	else
	{
		if(++Q_control_101->Resend_Frame_cnt > Resend_Max_Frames)
		{	
			QLY_101_link_Init();//链路复位
		}
		else
		{
			//重发最后一帧请求报文
			QLY_101_FrameSend(QLY_Last_101_Request_Frame);
			my_debug("Rsend 101LastFrame!");
			//等待时间复位
			Q_control_101->Wait101Frame_Times = Wait101Frame_TimeOut;
		}
	}
	return 1;
}
/******************************************************/
//函数功能：平衡101链路报文处理
//参数：p_buff待处理报文指针，con_type待处理报文控制域，p_buff2待发送报文指针
//返回值：正确返回0，错误返回1
//函数说明：链路处理函数一共分五个状态：1、链路等待状态。2、等待复位。3、等待请求确认。4、等待复位确认。5、初始化结束确认。6、链路正常状态(链路正常状态回复心跳确认)
//修改日期：2015/11/3
/******************************************************/
static enum 
{
    Bl101_LinkWait,
    Bl101_LinkWait_Rest,
    Bl101_RequestLink,
    Bl101_RequestRest,
    Bl101_LinkInitOver,
    Bl101_LinkConnect
} BL101_Link_state = Bl101_LinkWait;//定义平衡101链路状态

static UInt8 QLY_BL101_link_manage(UInt8* p_buff1,UInt8 con_type,UInt8* p_buff2)
{
	UInt32 status;
	UInt8 PRM;//启动方向位
	if(Fix_frame_top != *p_buff1)//判断报文头
		return 1;
	status = con_type & 0x0f;
	PRM = con_type & 0x40;//启动方向位
	switch(BL101_Link_state)
	{
		case Bl101_LinkWait: 
					if((PRM == 0x40) && (status == Call_linkstate_code))//等待链路请求
					{
						QLY_BL101_FixFrame_Send(Respond_by_link_code,low,low,p_buff2);//响应链路状态
						BL101_Link_state = Bl101_LinkWait_Rest;
						return 0;
					}
					break;
                    
		case Bl101_LinkWait_Rest:
					if((PRM == 0x40) && (status == Reset_link_code))//等待链路复位
					{
					   // my_debug("Bl101_LinkConnect");
						QLY_101_link_Init();//链路复位
						QLY_BL101_FixFrame_Send(Acknowledge_code,low,low,p_buff2);//肯定确认
						//set_timer();//延时最小发送间隔
                        set_timer();
						QLY_BL101_FixFrame_Send(Call_linkstate_code,high,low,p_buff2);//请求链路状态
						BL101_Requst_On();//打开等待超时重发
						BL101_Link_state = Bl101_RequestLink;
						return 0;
					}
					break;
		case Bl101_RequestLink:
					if(status == Respond_by_link_code)//等待链路回应
					{
					    //my_debug("Bl101_RequestLink");
						BL101_Requst_Off();//关闭等待超时重发
						QLY_BL101_FixFrame_Send(Reset_link_code,high,low,p_buff2);//请求链路复位
						BL101_Requst_On();//打开等待超时重发
						BL101_Link_state = Bl101_RequestRest;
						return 0;
					}
					break;
		case Bl101_RequestRest:
					if(status == Acknowledge_code)//等待复位回应
					{
						BL101_Requst_Off();
						//set_timer();//延时最小发送间隔
                        set_timer();
						QLY_BL101_VarFrame_Send(Trans_data_code,M_EI_NA_1,Spont_Cause,p_buff2,p_buff1);//初始化结束突发
						//my_debug("Trans Bl101_LinkInitOver!");
						BL101_Requst_On();
						BL101_Link_state = Bl101_LinkInitOver;
						return 0;
					}
					break;
		case Bl101_LinkInitOver:
					if(status == Acknowledge_code)//等待主站确认初始化结束
					{
						//my_debug("Receive Bl101_LinkInitOverCon");
						BL101_Requst_Off();//关闭超时重发
						BL101_Link_state = Bl101_LinkConnect;
						return 0;
					}
					break;
		case Bl101_LinkConnect:
					if(status == Test_link_code)//等待心跳包
					{
					    //my_debug("Bl101_LinkConnect");
						QLY_BL101_FixFrame_Send(Acknowledge_code,low,low,p_buff2);//发送确认
						return 0;
					}
					break;
		default:
					break;
	}
	if(status == Call_linkstate_code)//如果在链路等待状态之外收到请求链路状态，进入链路复位
	{
        //my_debug("Call_linkstate_code");
		QLY_101_link_Init();//复位链路
		QLY_BL101_FixFrame_Send(Respond_by_link_code,low,low,p_buff2);//响应链路状态
		BL101_Link_state = Bl101_LinkWait_Rest;
		return 0;
	}
	return 1;//报文未匹配，返回1
}
/******************************************************/
//函数功能：平衡101链路是否开始
//参数：
//返回值：开始返回0，没有返回1
//说明：用于判断链路状态
//修改日期：2015/12/25
/******************************************************/
//static UInt8 QLY_BL101_start_state(void)
//{
//	if(BL101_Link_state != Bl101_LinkWait)
//		return 0;
//	else
//		return 1;
//}
/******************************************************/
//函数功能：平衡101规约召唤流程控制
//参数：p_buff待处理报文指针，con_type待处理报文控制域，p_buff2待发送报文指针
//返回值：正确返回0，错误返回1
//说明：包括总召唤，组召唤
//修改日期：2015/11/09
/******************************************************/
static enum 
{
    Bl101_TotaCall_Wait_Act,
    Bl101_TotaCall_Wait_ActCon,
    Bl101_TotaCall_Wait_PointCon,
    Bl101_TotaCall_Wait_ValCon,
    Bl101_TotaCall_Wait_OverCon
} BL101_totacall_state = Bl101_TotaCall_Wait_Act;//定义平衡101总召唤状态

static UInt8 QLY_BL101_total_call(UInt8* p_buff1,UInt8 con_type,UInt8* p_buff2)
{
	UInt32 status = con_type & 0x0f;
	UInt32 type_id=0;
	UInt32 TotaCall_Cause=0;
	UInt32 Information;//定义信息体32字节
	//my_debug("QLY_BL101_total_call! status:%d",status);
	if(Var_frame_top == *p_buff1)//判断报文为可变帧
	{
		type_id = p_buff1[6];//取出类型标示
		
		TotaCall_Cause = p_buff1[8];//取出传送原因
	}
	switch(BL101_totacall_state)
	{
		case Bl101_TotaCall_Wait_Act://等待总召唤激活
				if(Var_frame_top != *p_buff1)//判断报文头
					break;
				if((type_id == C_IC_NA_1) && (Act_Cause == TotaCall_Cause))//如果为类型标示为总召唤，传送原因为激活
				{
					Information = p_buff1[15];
                    my_debug("Information:%d",Information);
					switch(Information)
					{
						case TotalCall_Info://总召唤
							Bl101_Call_Type = Bl101_TotaCall_Type;//进入总召唤流程
							QLY_BL101_FixFrame_Send(Acknowledge_code,low,low,p_buff2);//确认
							set_timer();//延时最小发送间隔
							QLY_BL101_VarFrame_Send(Trans_data_code,C_IC_NA_1,Actcon_Cause,p_buff2,p_buff1);//发送"激活确认"
							BL101_Requst_On();//打开等待超时重发
							BL101_totacall_state = Bl101_TotaCall_Wait_ActCon;//进入等待"激活确认"确认回复帧
							return 0;
						default:
							break;
					}
				}
				break;
		case Bl101_TotaCall_Wait_ActCon:
				if(Fix_frame_top != *p_buff1)//判断报文头
					break;
				if(status == Acknowledge_code)
				{
					switch(Bl101_Call_Type)
					{
						case Bl101_TotaCall_Type:
                            my_debug("M_SP_NA_1!");
							BL101_Requst_Off();//关闭等待超时重发
							QLY_BL101_VarFrame_Send(Trans_data_code,M_SP_NA_1,Introgen_Cause,p_buff2,p_buff1);//以响应总召唤原因，传送单点信息
							BL101_Requst_On();//打开等待超时重发
							BL101_totacall_state = Bl101_TotaCall_Wait_PointCon;//进入等待"单点信息确认"
							return 0;
						default:
							break;
					}
				}
				break;
		case Bl101_TotaCall_Wait_PointCon:
			if(Fix_frame_top != *p_buff1)//判断报文头
					break;
			if(status == Acknowledge_code)
				{
					switch(Bl101_Call_Type)
					{
						case Bl101_TotaCall_Type:
							BL101_Requst_Off();//关闭等待超时重发
							QLY_BL101_VarFrame_Send(Trans_data_code,M_ME_NC_1,Introgen_Cause,p_buff2,p_buff1);//以响应总召唤原因，传送测量值，归一化值
							BL101_Requst_On();//打开等待超时重发
							BL101_totacall_state = Bl101_TotaCall_Wait_ValCon;//进入等待"测量值确认"
							return 0;
						default:
							break;
					}
				}
			break;
		case Bl101_TotaCall_Wait_ValCon:
			if(Fix_frame_top != *p_buff1)//判断报文头
					break;
			if(status == Acknowledge_code)
				{
				    
				    my_debug("Bl101_TotaCall_Wait_ValCon!");
					BL101_Requst_Off();//关闭等待超时重发
					QLY_BL101_VarFrame_Send(Trans_data_code,C_IC_NA_1,Actterm_Cause,p_buff2,p_buff1);//发送激活结束,信息体内容不同
					BL101_Requst_On();//打开等待超时重发
					BL101_totacall_state = Bl101_TotaCall_Wait_OverCon;//进入等待"总召结束"确认
					return 0;
				}
			break;
		case Bl101_TotaCall_Wait_OverCon:
			if(Fix_frame_top != *p_buff1)//判断报文头
					break;
			if(status == Acknowledge_code)//收到总召唤结束确认
				{
				    my_debug("Bl101_TotaCall_Wait_OverCon!");
					BL101_Requst_Off();//关闭等待超时重发
					BL101_totacall_state = Bl101_TotaCall_Wait_Act;//召唤流程复位
					Bl101_Call_Type = Bl101_NoneCall_Type;//召唤类型复位
				}
			break;
		default:
			break;
	}
	return 1;
}
/******************************************************/
//函数功能：平衡101时钟同步流程控制
//参数：p_buff待处理报文指针，con_type待处理报文控制域，p_buff2待发送报文指针
//返回值：正确返回0，错误返回1
//函数说明：处理主站发来的时间同步报文
//修改日期：2015/10/19
/******************************************************/
static enum 
{
    BL101_GetDelay_Wait_Act,
    BL101_GetDelay_Wait_ActCon
}BL101_GetDelay_State = BL101_GetDelay_Wait_Act;//定义平衡101延时获得各状态

static UInt8 QLY_BL101_ClockSync_Control(UInt8* p_buff1,UInt8 con_type,UInt8* p_buff2)
{
	UInt32 status = con_type & 0x0f;
	UInt32 type_id=0;
	UInt32 Trans_Cause=0;
	my_debug("QLY_BL101_ClockSync_Control!");
	if(Var_frame_top == *p_buff1)//判断报文为可变帧
	{
		type_id = p_buff1[6];//取出类型标示
		Trans_Cause = p_buff1[8];//取出传送原因
	}
	switch(BL101_GetDelay_State)
	{
		case BL101_GetDelay_Wait_Act:
			if(Var_frame_top != *p_buff1)//判断报文头
					break;
			if((type_id == C_CD_NA_1) && (Act_Cause == Trans_Cause))//延时获得（激活）
			{
				T1_time = p_buff1[15];//记录毫秒时标
				T1_time |= p_buff1[16]<<8;
				QLY_BL101_FixFrame_Send(Acknowledge_code,low,low,p_buff2);//确认
				set_timer();//延时最小发送间隔
				QLY_BL101_VarFrame_Send(Trans_data_code,C_CD_NA_1,Actcon_Cause,p_buff2,p_buff1);//发送延时获得激活确认
				BL101_Requst_On();//打开等待超时重发
				BL101_GetDelay_State = BL101_GetDelay_Wait_ActCon;
				return 0;
			}
			if((type_id == C_CD_NA_1) && (Spont_Cause == Trans_Cause))//延时获得（突发）
			{
				//Global_Variable.Link_Trans_Delay = p_buff1[15];//记录链路延时
				//Global_Variable.Link_Trans_Delay |= p_buff1[16] << 8;
				QLY_BL101_FixFrame_Send(Acknowledge_code,low,low,p_buff2);//发送确认
				return 0;
			}
            
			if((type_id == C_CS_NA_1) && (Act_Cause == Trans_Cause))//时钟同步（激活）
			{
//				DL101_SetTime(&p_buff1[14]);//更新终端时间
				//settimeofday(&p_buff1[15],(struct timezone *)0);//更新终端时间
				QLY_BL101_FixFrame_Send(Acknowledge_code,low,low,p_buff2);//确认
				set_timer();//延时最小发送间隔
				QLY_BL101_VarFrame_Send(Trans_data_code,C_CS_NA_1,Actcon_Cause,p_buff2,p_buff1);//发送时钟同步激活确认
				BL101_Requst_On();//打开等待超时重发
				BL101_GetDelay_State = BL101_GetDelay_Wait_ActCon;
				return 0;
			}
			break;
		case BL101_GetDelay_Wait_ActCon:
			if(Fix_frame_top != *p_buff1)//判断报文头
					break;
			if(status == Acknowledge_code)
			{
				BL101_Requst_Off();//关闭等待超时重发
				BL101_GetDelay_State = BL101_GetDelay_Wait_Act;//状态恢复
				return 0;
			}
			break;
		default:
			break;
	}
	return 1;
}
/******************************************************/
//函数功能：平衡101规约遥控控制流程
//参数：p_buff待处理报文指针，con_type待处理报文控制域，p_buff2待发送报文指针
//返回值：正确返回0，错误返回1
//说明：
//修改日期：2015/12/04
/******************************************************/
static enum
{
    BL101_TeleControl_Wait_Act,
    BL101_TeleControl_Wait_ActCon
}BL101_TeleControl_State = BL101_TeleControl_Wait_Act;

static UInt8 QLY_BL101_TeleControl(UInt8* p_buff1,UInt8 con_type,UInt8* p_buff2)
{
//	UInt8 log_inf[] = "0:flag app for update! Reset!\r\n";
	UInt32 status = con_type & 0x0f;
	UInt32 type_id;
	UInt32 Trans_Cause;
//	APP_Grade_Parameter_Type Boot_Struct;
	if(Var_frame_top == *p_buff1)//判断报文为可变帧
	{
		type_id = p_buff1[6];//取出类型标示
		Trans_Cause = p_buff1[8];//取出传送原因
	}
	switch(BL101_TeleControl_State)
	{
		case BL101_TeleControl_Wait_Act:
			if(Var_frame_top != *p_buff1)//判断报文头
					break;
			if((type_id == C_SC_NA_1) && (Act_Cause == Trans_Cause))//判断遥控激活
			{
			/*
				if(0x00 != p_buff1[15])//检查遥控数据是否正确
					break;*/
				my_debug("QLY_BL101_TeleControl");
				QLY_BL101_FixFrame_Send(Acknowledge_code,low,low,p_buff2);//确认
				set_timer();//延时最小发送间隔
				QLY_BL101_VarFrame_Send(Trans_data_code,C_SC_NA_1,Actcon_Cause,p_buff2,p_buff1);//发送遥控激活确认
				BL101_Requst_On();//打开等待超时重发
				BL101_TeleControl_State = BL101_TeleControl_Wait_ActCon;
				return 0;
			}
			break;
		case BL101_TeleControl_Wait_ActCon:
			if(Fix_frame_top != *p_buff1)//判断报文头
					break;
			if(status == Acknowledge_code)
			{
				BL101_Requst_Off();//关闭等待超时重发
				BL101_TeleControl_State = BL101_TeleControl_Wait_Act;
				return 0;
			}
			break;
		default:
			break;
	}
	return 1;
}

/**************************************************************************/
//函数:static void IEC101_Single_YK( void )
//说明:单点遥控
//输入:无
//输出:无
//编辑:R&N
/**************************************************************************/
static void BL101_Single_YK( int frame_len ,UInt8 * p_buffs,UInt8 code)
{
    
    UInt8 codes=code;
	UInt8 * buf;
    buf =  p_buffs;//接收到的数据
	//unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;//当前帧数据的起点位置
	unsigned short recvcnt = 0;
	unsigned char len1=0, len2=0, len3=0, len4=0;
	unsigned char type= buf[recvcnt+ pIEC101_Struct->linkaddrlen+5];//类型标识符

    if(pIEC101_Struct->fd_usart <=0)
        return ;
     my_debug("recvcnt1 is:%d type is:%d linkaddr:0x%x",recvcnt,type,buf[recvcnt+5]);
	 len1 = pIEC101_Struct->linkaddrlen;
	 len2 = len1+pIEC101_Struct->cotlen;
	 len3 = len2+pIEC101_Struct->conaddrlen;
     len4 = len3+pIEC101_Struct->infoaddlen;
     my_debug("recvcnt is:%d",buf[len4+7]);
     if(buf[len4+7]&0x80)//遥控选择
     {
        my_debug("recvcnt2 is:%d",recvcnt);
        if(buf[len1+7] == IEC_CAUSE_ACT) //激活
		{
		    
            my_debug("recvcnt3 is:%d",recvcnt);
			switch(BL101_YK_Sel( frame_len,buf))
			{
			case YK_INVALID_ADDR://遥控被闭锁 或者 地址不对
				BL101_NegativeCON(frame_len,buf,codes);
				break;
			case YK_NORMAL_CONTRL: //一般遥控选择
				BL101_PositiveCON(frame_len,buf,codes);
				break;
//    			case 2: //复归选择
//    				IEC_PositiveCON(ucPort,pTemp_ASDUFrame , ucTemp_Length);
//    				break;
			}
		}
		else if(buf[len1+7] == IEC_CAUSE_DEACT) //停止激活确认
		{
		
            my_debug("recvcnt4 is:%d",recvcnt);
			switch(BL101_YK_SelHalt(buf))
			{
			case YK_INVALID_ADDR:
				BL101_NegativeCON(frame_len,buf,codes);
				break;
			case YK_CANCEL_SEL://一般遥控选择撤销
				BL101_PositiveCON(frame_len,buf,codes);
				break;
			}
		}
//		else
//            IEC101_NegativeCON(frame_len);
     }
     else//遥控执行
     {
        my_debug("recvcnt5");
		switch(BL101_AP_YKExec(frame_len,buf))
		{
		case YK_INVALID_ADDR:
			BL101_NegativeCON(frame_len,buf,codes);
			break;
		case YK_ZHIXIN_SUCC:
			BL101_PositiveCON(frame_len,buf,codes);
			break;
		}
     }
}
/**************************************************************************/
//函数:static static void IEC101_YK_Sel( int frame_len )
//说明:单点遥控选择
//输入:无
//输出:无
//编辑:R&N
/**************************************************************************/
static unsigned char BL101_YK_Sel(int frame_len,UInt8 * p_buffs)
{
	//char * buf =(char *)  save_recv_data;//接收到的数据
	UInt8 * buf;
    buf =  p_buffs;//接收到的数据
	//unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;//当前帧数据的起点位置
	unsigned short recvcnt = 0;
    unsigned short sPointNo, j,len3,len4;
	Point_Info   *  Point_Info;
    
    len3 = pIEC101_Struct->conaddrlen+pIEC101_Struct->cotlen+pIEC101_Struct->linkaddrlen;
    len4 = len3+pIEC101_Struct->infoaddlen;
	if(sys_mod_p->usYK_Num ==0)
		return YK_INVALID_ADDR;
//    sPointNo = (buf[(recvcnt+len3+7)&0x3FF]<<8) + buf[(recvcnt+len3+7)&0x3FF];//点号
    sPointNo = buf[recvcnt+len3+7];//点号
	Point_Info = sys_mod_p->pYK_Addr_List_Head;
    my_debug("sPointNo:%d %d",sPointNo,buf[recvcnt+len3+7]);
	for(j=0; j<sys_mod_p->usYK_Num; j++)
	{
		if(Point_Info->uiAddr==sPointNo)
		{
		    my_debug("sPointNo:%d,j=:%d",sPointNo,j);
			//Point_Info->usFlag = 1;
			break;
		}
		if(Point_Info->next !=NULL)
			Point_Info = Point_Info->next;
	}
    
	if(j==sys_mod_p->usYK_Num)
    {   
        my_debug("j=:%d",j);
		return YK_INVALID_ADDR;
       }
    
	if((buf[recvcnt+len4+7]&0x01)==0X01)//这里添加遥控分闸选中  还是遥控合闸选中
	{
		Point_Info->usFlag |=(1<<8);
        Send_Event_By_Message(MSG_YK, Point_Info->uiOffset, YK_OPEN_SEL);//最高位是1是表示选中，是0表示执行
    }
	else
    {
		Point_Info->usFlag |=(1<<9);
        Send_Event_By_Message(MSG_YK, Point_Info->uiOffset, YK_CLOSE_SEL);
    }
	return YK_NORMAL_CONTRL;
}
/****************************************************************************/
//函数说明:遥控选中撤销
//0:闭锁   1:正常    2:复归    3:没有这个地址
//YK_BISUO				0
//YK_NORMAL_CONTRL      	1
//YK_FUGUI				2
//YK_INVALID_ADDR		3
//YK_CANCEL_SEL			4
//编辑:R&N1110
//时间:2015.6.1
/****************************************************************************/
static unsigned char BL101_YK_SelHalt(UInt8 * p_buffs)
{
	//char * buf =(char *)  save_recv_data;//接收到的数据
	UInt8 * buf;
    buf =  p_buffs;//接收到的数据
	//unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;//当前帧数据的起点位置
	unsigned short recvcnt = 0;
	unsigned short sPointNo, i, len3, len4;
	Point_Info   *  Point_Info;

    len3 = pIEC101_Struct->conaddrlen+pIEC101_Struct->cotlen+pIEC101_Struct->linkaddrlen;
    len4 = len3+pIEC101_Struct->infoaddlen;

	if(sys_mod_p->usYK_Num ==0)
		return YK_INVALID_ADDR;

    //sPointNo = (buf[(recvcnt+len3+7)&0x3FF]<<8) | buf[(recvcnt+len3+7)&0x3FF];//点号
    sPointNo = buf[recvcnt+len3+7];//点号
	Point_Info = 	sys_mod_p->pYK_Addr_List_Head;

	for(i=0; i<sys_mod_p->usYK_Num; i++)
	{
		if(Point_Info->uiAddr==sPointNo)
		{
			//Point_Info->usFlag = 0;
			break;
		}
		if(Point_Info->next !=NULL)
			Point_Info = Point_Info->next;
	}
	if(i==sys_mod_p->usYK_Num)
		return YK_INVALID_ADDR;

	if((buf[recvcnt+len4+7]&0x01)==0X01)//这里添加遥控分闸选中取消  还是遥控合闸选中取消
	{
		Point_Info->usFlag &=~(1<<8);
        Send_Event_By_Message(MSG_YK, Point_Info->uiOffset, YK_CANCEL_OPEN_SEL);
    }
	else
    {
		Point_Info->usFlag &=~(1<<9);
        Send_Event_By_Message(MSG_YK, Point_Info->uiOffset, YK_CANCEL_CLOSE_SEL);
    }
	return YK_CANCEL_SEL;
}

/****************************************************************************/
//函数说明:遥控执行
//输入:ASDU帧     和长度
//编辑:R&N1110
//时间:2015.6.1
//YK_ZHIXIN_FAIL			5
//YK_ZHIXIN_SUCC		6
/****************************************************************************/
static unsigned char BL101_AP_YKExec(int frame_len,UInt8 * p_buffs)
{
	//char * buf =(char *)  save_recv_data;//接收到的数据
	UInt8 * buf;
    buf =  p_buffs;//接收到的数据
	//unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;//当前帧数据的起点位置
	unsigned short recvcnt = 0;
	unsigned short sPointNo, i,len3, len4;
	Point_Info   *  Point_Info;
//len3=5 len4=8
    len3 = pIEC101_Struct->conaddrlen+pIEC101_Struct->cotlen+pIEC101_Struct->linkaddrlen;
    len4 = len3+pIEC101_Struct->infoaddlen;
	if(sys_mod_p->usYK_Num ==0)
		return YK_INVALID_ADDR;
//    sPointNo = (buf[(recvcnt+len3+7)&0x3FF]<<8) | buf[(recvcnt+len3+7)&0x3FF];//点号
    sPointNo = buf[recvcnt+len3+7];//点号
        
    Point_Info = 	sys_mod_p->pYK_Addr_List_Head;
	for(i=0; i<sys_mod_p->usYK_Num; i++)
	{
		if(Point_Info->uiAddr==sPointNo)
		{
			//Point_Info->usFlag = 1;
			break;
		}
		if(Point_Info->next !=NULL)
			Point_Info = Point_Info->next;
	}
	if(i==sys_mod_p->usYK_Num)
		return YK_INVALID_ADDR;

	if((buf[len4+7]&0x01)==0X01)   //这里添加遥控分闸
	{
		if(Point_Info->usFlag &(1<<8))			           //已经选中
		{
				Point_Info->usFlag &=~(1<<8); 		   //合闸动作,并且解除选中
				//添加合闸动作
				Send_Event_By_Message(MSG_YK, Point_Info->uiOffset, YK_CLOSE_EXECUTIVE);
		}
	}
	else
	{
		if(Point_Info->usFlag &(1<<9))			           //已经选中
		{
				Point_Info->usFlag &=~(1<<9); 		   //合闸动作,并且解除选中
				//添加分闸动作
				Send_Event_By_Message(MSG_YK, Point_Info->uiOffset, YK_OPEN_EXECUTIVE);
		}
	}
	if(1)											   //动作成功 ?    执行的时候必须判断选中没有
		return YK_ZHIXIN_SUCC;
	else
		return YK_ZHIXIN_FAIL;
}

/******************************************************/
//函数功能：平衡101突发上送流程管理
//参数：
//返回值：正确返回0，错误返回1
//修改日期：2015/10/25
/******************************************************/
static enum 
{
    BL101_SpontTrans_None,
    BL101_SpontTrans_Wait1_Con,
    BL101_SpontTrans_Wait2_Con,
    BL101_SpontTrans_Wait3_Con
} BL101_SpontTrans_State = BL101_SpontTrans_None;

UInt8 QLY_BL101_SpontTrans_Control(UInt8* p_buff1,UInt8 con_type,UInt8* p_buff2)
{
	UInt32 status = con_type & 0x0f;
//	UInt32 type_id;
//	UInt32 Trans_Cause;
	if(Fix_frame_top != *p_buff1)//判断报文头
			return 1;
	switch(BL101_SpontTrans_State)
	{
		case BL101_SpontTrans_Wait1_Con:
			if(status == Acknowledge_code)
			{
				BL101_Requst_Off();//关闭等待超时重发
//				QLY_BL101_SpontTransData(Sudden_fault,M_SP_TB_1);//发送带时标的突发帧，单点信息
				QLY_BL101_SpontTransData(M_SP_TB_1);//发送带时标的突发帧，单点信息
				BL101_Requst_On();//打开等待超时重发
				BL101_SpontTrans_State = BL101_SpontTrans_Wait3_Con;
				return 0;
			}
			break;
            /*
		case BL101_SpontTrans_Wait2_Con:
			if(status == Acknowledge_code)
			{
				BL101_Requst_Off();//关闭等待超时重发
                set_timer();//延时最小发送间隔
				
				if(Sudden_fault->type == just_mark)
				{
					BL101_SpontTrans_State = BL101_SpontTrans_None;//状态恢复
					OSMemPut(Buff16_FreeList,Sudden_fault);//删除内存
					return 0;
				}
				QLY_BL101_SpontTransData(Sudden_fault,M_ME_NA_1);;//发送测量值，归一化值，突发
                
				BL101_Requst_On();//打开等待超时重发
				BL101_SpontTrans_State = BL101_SpontTrans_Wait3_Con;
				return 0;
			}
			break;*/
		case BL101_SpontTrans_Wait3_Con:
			if(status == Acknowledge_code)
			{
				BL101_Requst_Off();//关闭等待超时重发
				BL101_SpontTrans_State = BL101_SpontTrans_None;//状态恢复
				//OSMemPut(Buff16_FreeList,Sudden_fault);//删除内存
				return 0;
			}
		default:
			break;
	}
	return 1;
}
/******************************************************/
//函数功能：平衡101复位程序
//参数：
//返回值：
//修改日期：2015/12/03
/******************************************************/
static void QLY_BL101_state_Reset(void)
{
	/*UInt8 *p_fault,err;
	//p_fault = OSQAccept(QSem_Fault_Trants,&err);
	
	while((void*)0 != p_fault)//清除上报队列中的上报故障
	{
		//OSMemPut(Buff16_FreeList,p_fault);//删除内存
		//p_fault = OSQAccept(QSem_Fault_Trants,&err);
	}
	*/
	my_debug("QLY_BL101_state_Reset");
	Q_control_101->Wait_Answer_mask = low;//清除等待回应标志
	Q_control_101->Resend_Frame_cnt = 0;//重发计数清零
	
	BL101_Link_state = Bl101_LinkWait;//链路状态复位
	BL101_totacall_state = Bl101_TotaCall_Wait_Act;//总召唤状态复位
	Bl101_Call_Type = Bl101_NoneCall_Type;//召唤类型状态复位
	BL101_GetDelay_State = BL101_GetDelay_Wait_Act;//时钟同步状态复位
	BL101_SpontTrans_State = BL101_SpontTrans_None;//突发上传状态复位
	BL101_TeleControl_State = BL101_TeleControl_Wait_Act;//遥控复位
	QLY_ReciveFrame_temp[0] = 0;//暂存报文标志清零
	
}
/******************************************************/
//函数功能：平衡101报文功能解析程序
//参数：p_buff待处理报文指针，con_type待处理报文控制域，p_buff2待发送报文指针
//返回值：正确返回0，错误返回1
//修改日期：2015/10/19
/******************************************************/
//定义处理函数指针数组
static UInt8 (*p_QLY_BL101_function[4])(UInt8* p_buff1,UInt8 con_type,UInt8* p_buff2) = {
//平衡101报文处理函数
	QLY_BL101_link_manage,//链路管理
	QLY_BL101_total_call,//总召唤
	QLY_BL101_ClockSync_Control,//时钟同步
	QLY_BL101_SpontTrans_Control//突发上传
};//函数顺序按优先级排列

UInt8 QLY_BL101frame_switch(UInt8* p_buff1,UInt8 con_type,UInt8* p_buff2)
{
	UInt8 i;
	if(BL101_Link_state == Bl101_LinkConnect)//链路已连接
	{
	    my_debug("Bl101_LinkConnect is ok!");
		for(i=0;i<4;i++)
		{
			if(!(p_QLY_BL101_function[i](p_buff1,con_type,p_buff2)))//如果报文被任一处理函数处理
				return 0;
		}
	}
	else
	{
		if(!QLY_BL101_link_manage(p_buff1,con_type,p_buff2))//链路未连接，处理链路请求
			return 0;
	}
	//printf("unknow frame type !\r\n");//否则则是不识别的报文
	return 1;
}
/******************************************************/
//函数功能：平衡101报文处理主程序
//参数：
//返回值：正确返回0，错误返回3FCB判定失败（重发最后一条报文或者复位）），4未识别的报文，5报文暂存
//修改日期：2015/10/19
/******************************************************/
UInt8 QLY_BL101FrameProcess(UInt8 *p_buff1)
{
	UInt8 con_type,Frame_length;
	UInt8* p_buff2;
	UInt32 i;
	con_type = QLY_get101frame_controltype(p_buff1);//获取控制域
	
	if(Q_control_101->Wait_Answer_mask == high)//如果是在等待回应期间，需要将收到的请求报文暂存
	{
		if((con_type & (1<<6)) && ((con_type & 0x0f) != Call_linkstate_code))//判断是否收到启动方向报文且不是请求链路状态
		{
			//将报文暂存
			QLY_ReciveFrame_temp[0] = 1;
			Frame_length = QLY_101FrameLen(p_buff1);
			for(i=1;i<=Frame_length;i++)
			QLY_ReciveFrame_temp[i] = *p_buff1++;
			return 5;
		}
	}
    
    if(!QLY_BL101_TeleControl(p_buff1,con_type,p_buff2))//遥控
		return 0;
	//处理标准101报文
	if(QLY_101frame_FCB_detect(con_type))//FCB判定
		return 3;
	if(QLY_BL101frame_switch(p_buff1,con_type,p_buff2))//处理报文
		return 4;
    
	if(Q_control_101->Wait_Answer_mask == low)	//如果不在等待回应期间
	{/*
		//如果存在突发事件，处理突发事件
		p_fault = OSQAccept(QSem_Fault_Trants,&err);
		if((void*)0 != p_fault)
		{
			Sudden_fault = (Spont_Trans_type*)p_fault;//记录将要处理的突发事件
			QLY_BL101_SpontTransData(Sudden_fault,M_SP_NA_1);//上送不带时标的突发帧，单点信息
			BL101_Requst_On();//打开等待超时重发
			BL101_SpontTrans_State = BL101_SpontTrans_Wait1_Con;//切换突发上传流程到等待回应
			return 0;
		}		*/
		//处理暂存报文
		if(QLY_ReciveFrame_temp[0])//存在暂存报文
		{
			QLY_ReciveFrame_temp[0] = 0;//暂存报文标志清零
			con_type = QLY_get101frame_controltype(&QLY_ReciveFrame_temp[1]);//获取控制域
			if(QLY_101frame_FCB_detect(con_type))//FCB判定
				return 6;
			//处理暂存报文
			my_debug("process save frame!");
			if(QLY_BL101frame_switch(&QLY_ReciveFrame_temp[1],con_type,p_buff2))//处理报文
			return 4;
		}
	}
	return 0;
}
//*******************************************************101报文处理主程序********************************************************************************//

/******************************************************/
//函数功能：平衡101突发上传主程序
//参数：
//返回值：正确返回0，错误返回1
//修改日期：2015/10/25
/******************************************************/
UInt8 QLY_BL101_SpontTrans()
{
	if(BL101_Link_state != Bl101_LinkConnect)//如果链路未连接，不上送
		return 1;
	if(Q_control_101->Wait_Answer_mask == low)//不在等待回应期间，直接突发上送
	{
		QLY_BL101_SpontTransData(M_SP_NA_1);//上送不带时标的突发帧，单点信息
		BL101_Requst_On();//打开等待超时重发
		BL101_SpontTrans_State = BL101_SpontTrans_Wait1_Con;//切换突发上传流程到等待回应
	}
	return 0;
}

/******************************************************/
//函数功能：平衡101报文处理主程序
//参数：
//返回值：正确返回0，错误返回1帧校验未通过，2控制域获取失败，3FCB判定失败（重发最后一条报文或者复位）），4未识别的报文，5报文暂存，6暂存报文FCB判定失败，
//修改日期：2015/10/19
/******************************************************/
//101报文处理主程序
UInt8 QLY_101FrameProcess(LOCAL_Module *sys_mod_ps,UInt8* p_buff1)
{
 //       create();
 /*
        struct itimerval tick;
        
        signal(SIGALRM, QLY_101Frame_1s_RsendCheck);
        memset(&tick, 0, sizeof(tick));
        tick.it_value.tv_sec = 1;
        tick.it_value.tv_usec = 0;
    
        //After first, the Interval time for clock
        tick.it_interval.tv_sec = 1;
        tick.it_interval.tv_usec = 0;
    
        if(setitimer(ITIMER_REAL, &tick, NULL) < 0)
                printf("Set timer failed!\n");
    */    
    sys_mod_p = sys_mod_ps;
	if(QLY_101frame_check(p_buff1))//帧校验
		return 1;
	switch(Balance101_Mask)//101通讯规约模式选择
	{
		case 0://非平衡式
				break;
		case 1://平衡式
			return QLY_BL101FrameProcess(p_buff1);//处理报文
		default:
			break;
	}
	return 0;//成功返回0
}

