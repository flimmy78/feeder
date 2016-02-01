/*************************************************************************/
// iec101.c                                        Version 1.00
//
// 本文件是DTU2.0通讯网关装置的IEC60870-5-101规约处理程序
// 编写人:R&N
//
// 工程师:R&N
//  日	   期:2015.04.17
//  注	   释:
/*************************************************************************/
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
#include "monitorpc.h"
#include "sharearea.h"


/******************全局变量，存储大部分101规约中的参数******/
IEC101Struct  * pIEC101_Struct;

unsigned char ucYKLock;/*遥控闭锁标志
                            0-无闭锁
                            1-被当地闭锁，
                            2-被规约1闭锁,
                            3-被规约2闭锁，
                            4-被以太网规约闭锁 ,
                            5-PLC直接遥控闭锁,
                            6-被远方就地闭锁
                      */
static unsigned char       	save_recv_data[IEC101BUFNUM * IEC101BUFLEN];
static unsigned char       	save_send_data[IEC101BUFLEN];
static LOCAL_Module         * sys_mod_p;

static YXInfoStruct         * pYXInfoRoot;     //用于遥信变位记录
static YXInfoStruct         * pYXInfoFill;
static YXInfoStruct         * pYXInfoGet;

static SOEInfoStruct        * pSOERoot;         //用于遥信SOE事件
static SOEInfoStruct        * pSOEFill;
static SOEInfoStruct        * pSOEGet;

sem_t              			sem_iec101;
sem_t              			sem_iec101_recv;



/****************************static local  funtion****************************/
static unsigned char  Get_Dianbiao_From_Line(char * line, struct _IEC101Struct  * item);
static  void IEC101_InitAllFlag( void );
static  int IEC101_Open_Console(void);
static int set_speed(int fd,  int speed);
static int set_parity(int fd, int databits, int stopbits, int parity) ;
static void IEC101_UnsoSend( void );
static void IEC101_Protocol_Entry(  void  );
static void IEC101_SendE5H( void );
static  void  IEC101_Ap_Summonall( void );
static void IEC101_Rqall_Con(void);
static unsigned char IEC101_CheckSum(unsigned short getcnt,unsigned char length,  unsigned char flag);
static unsigned char IEC101_Chg_Search(void);
static void IEC101_EndInit( void );
static void IEC101_Rqlink_Res(void);
static void IEC101_Rstlink_Con( void );
static void IEC101_Process_Varframe( int frame_len);
static void IEC101_SendTime(int length) ;
static void IEC101_Send10H(unsigned char ucCommand) ;      /* 可变报文长度的RTU回答 */
static unsigned char IEC101_VerifyDLFrame(int fd_usart , int * buf_len);
static void IEC101_Interroall_End(void);
static short IEC101_AP_ALL_YC( short send_over );//报告全部遥测数据
static unsigned char IEC101_AP_ALL_YX( short send_num );//报告所有遥信数据
static unsigned char IEC101_Process_COS( void ) ;//应用层处理
static unsigned char IEC101_Process_SOE( void ) ;//应用层处理
static void IEC101_Single_YK( int frame_len );
static void IEC101_NegativeCON(int frame_len);
static void IEC101_PositiveCON(int frame_len);
static unsigned char IEC101_YK_Sel( int frame_len );
static unsigned char IEC101_YK_SelHalt(void);
static unsigned char IEC101_AP_YKExec(int frame_len);
static void IEC101_AP_INTERROALL(void);



/****************************static local  various****************************/
static int speed_arr[] = {B230400, B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1800, B1200, B600, B300};
static int name_arr[]  = {230400,  115200,  57600,  38400, 19200,  9600,  4800,  2400,  1800,  1200,  600,  300};


/**************************************************************************************/
//函数说明:回复0xE5
//输入:命令位
//输出::无
//编辑:R&N1110	QQ:402097953
//时间:2014.10.20
/**************************************************************************************/
static void IEC101_SendE5H( void )
{
 if(pIEC101_Struct->flag&SUPPORTE5_BIT)    //支持E5命令
 {
 	 pIEC101_Struct->pSendBuf[0] = 0xE5;
        write(pIEC101_Struct->fd_usart,  pIEC101_Struct->pSendBuf, 1);
    	 Log_Frame(pIEC101_Struct->logfd,  pIEC101_Struct->pSendBuf,  0, 1, SEND_FRAME);
 }
  else
 	IEC101_Send10H(IEC_CAUSE_DEACTCON);
}

/**************************************************************************************/
//函数说明:回复固定帧格式
//输入:命令位
//输出::无
//编辑:R&N1110	QQ:402097953
//时间:2014.10.20
/**************************************************************************************/
static void IEC101_Send10H(unsigned char ucCommand)       /* 可变报文长度的RTU回答 */
{
    memset(pIEC101_Struct->pSendBuf, 0 , pIEC101_Struct->linkaddrlen+4);
    pIEC101_Struct->pSendBuf[0]=0x10;
    pIEC101_Struct->pSendBuf[1]=ucCommand;
    pIEC101_Struct->pSendBuf[2]=pIEC101_Struct->linkaddr;
    pIEC101_Struct->pSendBuf[pIEC101_Struct->linkaddrlen+2]=ucCommand+pIEC101_Struct->linkaddr;
    pIEC101_Struct->pSendBuf[pIEC101_Struct->linkaddrlen+3]=0x16;
    write(pIEC101_Struct->fd_usart,  pIEC101_Struct->pSendBuf, pIEC101_Struct->linkaddrlen+4);
    Log_Frame(pIEC101_Struct->logfd,  pIEC101_Struct->pSendBuf,  0, pIEC101_Struct->linkaddrlen+4, SEND_FRAME);
}

/**************************************************************************/
//函数:void IEC104_Interroall_End(void)
//说明:结束总召唤
//输入:无
//输出:无
//编辑:R&N
//时间:2015.05.25
/**************************************************************************/
static void IEC101_Interroall_End(void)
{
	unsigned char len1=0, len2=0, len3=0, len4=0;
	unsigned char ucCommand;
	unsigned short nsend=0;

    if(pIEC101_Struct->fd_usart <=0)
        return ;

	 len1 = pIEC101_Struct->linkaddrlen;
	 len2 = len1+pIEC101_Struct->cotlen;
	 len3 = len2+pIEC101_Struct->conaddrlen;
	 len4 = len3+pIEC101_Struct->infoaddlen;

	 memset( pIEC101_Struct->pSendBuf, 0, len4+10);
	 pIEC101_Struct->pSendBuf[0]= 0x68;
	 pIEC101_Struct->pSendBuf[1]= len4+4;
	 pIEC101_Struct->pSendBuf[2]= pIEC101_Struct->pSendBuf[1];
	 pIEC101_Struct->pSendBuf[3]= 0x68;

	 ucCommand = 0x08 | IEC_DIR_RTU2MAS_BIT;
	//if(IEC101_Chg_Search())
		ucCommand |=IEC_ACD_BIT;
	 pIEC101_Struct->pSendBuf[4]= ucCommand;
	 pIEC101_Struct->pSendBuf[5]= pIEC101_Struct->linkaddr;
	 pIEC101_Struct->pSendBuf[len1+5]=C_IC_NA_1;   //type
	 pIEC101_Struct->pSendBuf[len1+6]=0x01;		//可变结构体限定词
	 pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_ACTTERM;//原因
	 pIEC101_Struct->pSendBuf[len2+7]=pIEC101_Struct->linkaddr; //站地址

	 pIEC101_Struct->pSendBuf[len3+7]=0;	//信息体地址
	 pIEC101_Struct->pSendBuf[len4+7]=IEC_QOI_INTROGEN; //品质因数
	 pIEC101_Struct->pSendBuf[len4+8]=IEC101_CheckSum(4,  (len4+8-3), IECFRAMESEND);//帧校验
	 pIEC101_Struct->pSendBuf[len4+9]=0x16;

    nsend = write(pIEC101_Struct->fd_usart,  pIEC101_Struct->pSendBuf, len4+10);
	Log_Frame(pIEC101_Struct->logfd,  pIEC101_Struct->pSendBuf,  0, len4+10, SEND_FRAME);
}
/**************************************************************************/
//函数:static short IEC101_AP_ALL_YC( short send_num )
//描述:报告所有遥测数据,发送浮点数
//输入:已经发生的遥测的个数
//输出:已经发生的遥测的个数
//编辑:R&N
//时间:2015.5.27
/**************************************************************************/
static short IEC101_AP_ALL_YC( short send_num )//报告全部遥测数据
{
	unsigned char len1=0, len2=0, len3=0;
	unsigned char ucCommand;
    short len,j, k,i, send_over=send_num;
    Point_Info   * pTmp_Point=NULL;
    float data=0;

    if(pIEC101_Struct->fd_usart <=0)
        return-1;

	 len1 = pIEC101_Struct->linkaddrlen;
	 len2 = len1+pIEC101_Struct->cotlen;
	 len3 = len2+pIEC101_Struct->conaddrlen;

	 memset( pIEC101_Struct->pSendBuf, 0, IEC101BUFLEN);

	 ucCommand = 0x08 | IEC_DIR_RTU2MAS_BIT;
	if(IEC101_Chg_Search())
		ucCommand |=IEC_ACD_BIT;//er级用户数据暂停，需要传递一级用户数据
	 pIEC101_Struct->pSendBuf[4]= ucCommand;
	 pIEC101_Struct->pSendBuf[5]= pIEC101_Struct->linkaddr;
	 pIEC101_Struct->pSendBuf[len1+5]=M_ME_NC_1;   //type
	 if(sys_mod_p->Continuity & YC_NOTCONTINUE) 	//地址不连续
		pIEC101_Struct->pSendBuf[len1+6]= 0;        //可变结构体限定词
     else
		pIEC101_Struct->pSendBuf[len1+6]= IEC104_VSQ_SQ_BIT;//可变结构体限定词

	 pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_INTROGEN;//原因
	 pIEC101_Struct->pSendBuf[len1+len2+7]=pIEC101_Struct->linkaddr; //站地址

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

    	 pIEC101_Struct->pSendBuf[len3+7+len]=IEC101_CheckSum(4, len3+7+len, IECFRAMESEND);//帧校验
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

/**************************************************************************/
//函数描述:报告所有遥信数据
//输入:无
//输出:无
//编辑:R&N
/**************************************************************************/
static unsigned char IEC101_AP_ALL_YX( short send_num )//报告所有遥信数据
{
	unsigned char len1=0, len2=0, len3=0;
	unsigned char ucCommand;
    short len,j, k,i, send_over=send_num;
    Point_Info   * pTmp_Point=NULL;
    int data=0;

    if(pIEC101_Struct->fd_usart <=0)
        return -1;

	 len1 = pIEC101_Struct->linkaddrlen;
	 len2 = len1+pIEC101_Struct->cotlen;
	 len3 = len2+pIEC101_Struct->conaddrlen;

	 memset( pIEC101_Struct->pSendBuf, 0, IEC101BUFLEN);

	 ucCommand = 0x08 | IEC_DIR_RTU2MAS_BIT;
	if(IEC101_Chg_Search())
		ucCommand |=IEC_ACD_BIT;//er级用户数据暂停，需要传递一级用户数据
	 pIEC101_Struct->pSendBuf[4]= ucCommand;
	 pIEC101_Struct->pSendBuf[5]= pIEC101_Struct->linkaddr;
	 pIEC101_Struct->pSendBuf[len1+5]=M_SP_NA_1;   //type
	 if(sys_mod_p->Continuity & YC_NOTCONTINUE) 	//地址不连续
		pIEC101_Struct->pSendBuf[len1+6]= 0;        //可变结构体限定词
     else
		pIEC101_Struct->pSendBuf[len1+6]= IEC104_VSQ_SQ_BIT;//可变结构体限定词

	 pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_INTROGEN;//原因
	 pIEC101_Struct->pSendBuf[len1+len2+7]=pIEC101_Struct->linkaddr; //站地址

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
			data =(int)Read_From_Sharearea(pTmp_Point->uiOffset, TYPE_YX_STATUS_BIT);//遥测值
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

    	 pIEC101_Struct->pSendBuf[len3+7+len]=IEC101_CheckSum(4, len3+7+len, IECFRAMESEND);//帧校验
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



//	len=0;
//	if(pIEC_Struct[ucPort]->usDI_Seq[rtu_id]>=pIEC_Struct[ucPort]->usRTU_DI_Num[rtu_id])
//		return(0);

//	usTemp_PointNo = pIEC_Struct[ucPort]->usSPI_Start_Addr[rtu_id] + pIEC_Struct[ucPort]->usDI_Seq[rtu_id];

//	pTemp_ASDUFrame->ucInfoUnit[len++] = (unsigned char)(usTemp_PointNo & 0xff);
//	pTemp_ASDUFrame->ucInfoUnit[len++] = (unsigned char)(usTemp_PointNo>>8);
//	if(pIEC_Struct[ucPort]->ucInfoByteNum == 3)
//	{
//		pTemp_ASDUFrame->ucInfoUnit[len++] = 0x00;
//	}
//	for(j=0;j<64;j++)
//	{
//		if(pIEC_Struct[ucPort]->usDI_Seq[rtu_id]>=pIEC_Struct[ucPort]->usRTU_DI_Num[rtu_id])
//			break;

//		usTemp_DBNo = pIEC_Struct[ucPort]->usDBYXNO[rtu_id][pIEC_Struct[ucPort]->usDI_Seq[rtu_id]];

//		if(pYXDBRoot[usTemp_DBNo].ucUnionFlag)
//			ucTemp_YXValue = GetUnionYXValue(pYXDBRoot[usTemp_DBNo].ucUnionFlag);
//		else
//			ucTemp_YXValue = pYXDBRoot[usTemp_DBNo].ucYXValue;

//		if(pYXDBRoot[usTemp_DBNo].ucStatus & (NOW_REFRESH_BIT|PRE_REFRESH_BIT))
//			ucTemp_YXValue &= ~IEC_SIQ_NT_BIT;
//		else
//			ucTemp_YXValue |= IEC_SIQ_NT_BIT;

//		pTemp_ASDUFrame->ucInfoUnit[len++] = ucTemp_YXValue;

//		pTemp_ASDUFrame->ucRep_Num++;
//		pIEC_Struct[ucPort]->usDI_Seq[rtu_id]++;
//	}

//	if(pTemp_ASDUFrame->ucRep_Num > 0)
//	{
//		pTemp_ASDUFrame->ucRep_Num |= IEC_VSQ_SQ_BIT;
//		IEC_DL_Send68H(ucPort,len+12);
//		return(1);
//	}
//	else
//		return 0;
//return 0;
}
/**************************************************************************/
//函数:static void IEC101_PositiveCON(int frame_len)
//说明:确认回复帧
//输入:帧长
//输出:无
//编辑:R&N
/**************************************************************************/
static void IEC101_PositiveCON(int frame_len)
{
	char * buf =(char *)  save_recv_data;//接收到的数据
	unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;//当前帧数据的起点位置
	unsigned char len1=pIEC101_Struct->linkaddrlen;
	unsigned short i=0;

     for(i=0; i<frame_len; i++)
        pIEC101_Struct->pSendBuf[i] = buf[(recvcnt+i)&0x3FF];

     pIEC101_Struct->pSendBuf[4]= IEC_DIR_RTU2MAS_BIT;
     pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_ACTCON;//原因
     pIEC101_Struct->pSendBuf[frame_len-2]=IEC101_CheckSum(4,  frame_len-6, IECFRAMESEND);//帧校验

    write(pIEC101_Struct->fd_usart,  pIEC101_Struct->pSendBuf, frame_len);
	Log_Frame(pIEC101_Struct->logfd,  pIEC101_Struct->pSendBuf,  0, frame_len, SEND_FRAME);
}
/**************************************************************************/
//函数:static void IEC101_NegativeCON(int frame_len)
//说明:确认回复帧
//输入:帧长
//输出:无
//编辑:R&N
/**************************************************************************/
static void IEC101_NegativeCON(int frame_len)
{
	char * buf =(char *)  save_recv_data;//接收到的数据
	unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;//当前帧数据的起点位置
	unsigned char len1=pIEC101_Struct->linkaddrlen;
	unsigned short i=0;

     for(i=0; i<frame_len; i++)
        pIEC101_Struct->pSendBuf[i] = buf[(recvcnt+i)&0x3FF];

     pIEC101_Struct->pSendBuf[4]= IEC_DIR_RTU2MAS_BIT;
     pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_N_BIT | IEC_CAUSE_ACTCON;//原因
     pIEC101_Struct->pSendBuf[frame_len-2]=IEC101_CheckSum(4,  frame_len-6, IECFRAMESEND);//帧校验

    write(pIEC101_Struct->fd_usart,  pIEC101_Struct->pSendBuf, frame_len);
	Log_Frame(pIEC101_Struct->logfd,  pIEC101_Struct->pSendBuf,  0, frame_len, SEND_FRAME);
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
static unsigned char IEC101_YK_SelHalt(void)
{
	char * buf =(char *)  save_recv_data;//接收到的数据
	unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;//当前帧数据的起点位置
	unsigned short sPointNo, i, len3, len4;
	Point_Info   *  Point_Info;

    len3 = pIEC101_Struct->conaddrlen+pIEC101_Struct->cotlen+pIEC101_Struct->linkaddrlen;
    len4 = len3+pIEC101_Struct->infoaddlen;

	if(sys_mod_p->usYK_Num ==0)
		return YK_INVALID_ADDR;

    //sPointNo = (buf[(recvcnt+len3+7)&0x3FF]<<8) | buf[(recvcnt+len3+7)&0x3FF];//点号
    sPointNo = buf[(recvcnt+len3+7)&0x3FF];//点号
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

	if((buf[(recvcnt+len4+7)&0x3FF]&0x01)==0X01)//这里添加遥控分闸选中取消  还是遥控合闸选中取消
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
/**************************************************************************/
//函数:static static void IEC101_YK_Sel( int frame_len )
//说明:单点遥控选择
//输入:无
//输出:无
//编辑:R&N
/**************************************************************************/
static unsigned char IEC101_YK_Sel( int frame_len )
{
	char * buf =(char *)  save_recv_data;//接收到的数据
	unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;//当前帧数据的起点位置
    unsigned short sPointNo, j,len3,len4;
	Point_Info   *  Point_Info;
    
    len3 = pIEC101_Struct->conaddrlen+pIEC101_Struct->cotlen+pIEC101_Struct->linkaddrlen;
    len4 = len3+pIEC101_Struct->infoaddlen;
	if(sys_mod_p->usYK_Num ==0)
		return YK_INVALID_ADDR;
//    sPointNo = (buf[(recvcnt+len3+7)&0x3FF]<<8) + buf[(recvcnt+len3+7)&0x3FF];//点号
    sPointNo = buf[(recvcnt+len3+7)&0x3FF];//点号
	Point_Info = sys_mod_p->pYK_Addr_List_Head;
    my_debug("sPointNo:%d %d",sPointNo,buf[(recvcnt+len3+7)&0x3FF]);
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
    
	if((buf[(recvcnt+len4+7)&0x3FF]&0x01)==0X01)//这里添加遥控分闸选中  还是遥控合闸选中
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
//函数说明:遥控执行
//输入:ASDU帧     和长度
//编辑:R&N1110
//时间:2015.6.1
//YK_ZHIXIN_FAIL			5
//YK_ZHIXIN_SUCC		6
/****************************************************************************/
static unsigned char IEC101_AP_YKExec(int frame_len)
{
	char * buf =(char *)  save_recv_data;//接收到的数据
	unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;//当前帧数据的起点位置
	unsigned short sPointNo, i,len3, len4;
	Point_Info   *  Point_Info;

    len3 = pIEC101_Struct->conaddrlen+pIEC101_Struct->cotlen+pIEC101_Struct->linkaddrlen;
    len4 = len3+pIEC101_Struct->infoaddlen;
	if(sys_mod_p->usYK_Num ==0)
		return YK_INVALID_ADDR;
//    sPointNo = (buf[(recvcnt+len3+7)&0x3FF]<<8) | buf[(recvcnt+len3+7)&0x3FF];//点号
    sPointNo = buf[(recvcnt+len3+7)&0x3FF];//点号
        
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

	if((buf[(recvcnt+len4+7)&0x3FF]&0x01)==0X01)   //这里添加遥控分闸
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

/**************************************************************************/
//函数:static void IEC101_Single_YK( void )
//说明:单点遥控
//输入:无
//输出:无
//编辑:R&N
/**************************************************************************/
static void IEC101_Single_YK( int frame_len )
{
	char * buf =(char *)  save_recv_data;//接收到的数据
	unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;//当前帧数据的起点位置
	unsigned char len1=0, len2=0, len3=0, len4=0;

    if(pIEC101_Struct->fd_usart <=0)
        return ;

	 len1 = pIEC101_Struct->linkaddrlen;
	 len2 = len1+pIEC101_Struct->cotlen;
	 len3 = len2+pIEC101_Struct->conaddrlen;
     len4 = len3+pIEC101_Struct->infoaddlen;

     if(buf[(recvcnt+len4+7)&0x3FF]&0x80)//遥控选择
     {
        if(buf[(recvcnt+len1+7)&0x3FF] == IEC_CAUSE_ACT) //激活
		{
			switch(IEC101_YK_Sel( frame_len))
			{
			case YK_INVALID_ADDR://遥控被闭锁 或者 地址不对
				IEC101_NegativeCON(frame_len);
				break;
			case YK_NORMAL_CONTRL: //一般遥控选择
				IEC101_PositiveCON(frame_len);
				break;
//    			case 2: //复归选择
//    				IEC_PositiveCON(ucPort,pTemp_ASDUFrame , ucTemp_Length);
//    				break;
			}
		}
		else if(buf[(recvcnt+len1+7)&0x3FF] == IEC_CAUSE_DEACT) //停止激活确认
		{
			switch(IEC101_YK_SelHalt())
			{
			case YK_INVALID_ADDR:
				IEC101_NegativeCON(frame_len);
				break;
			case YK_CANCEL_SEL://一般遥控选择撤销
				IEC101_PositiveCON(frame_len);
				break;
			}
		}
//		else
//            IEC101_NegativeCON(frame_len);
     }
     else//遥控执行
     {
		switch(IEC101_AP_YKExec(frame_len))
		{
		case YK_INVALID_ADDR:
			IEC101_NegativeCON(frame_len);
			break;
		case YK_ZHIXIN_SUCC:
			IEC101_PositiveCON(frame_len);
			break;
		}
     }
}
/**************************************************************************/
//函数说明:上报总召唤的所有数据
//输入:无
//输出:无
//编辑:R&N
/**************************************************************************/
static  void  IEC101_Ap_Summonall( void )
{
    static short numyc=0, numyx=0;

	if(sys_mod_p->usYC_Num>=1)
     {
        if(numyc != sys_mod_p->usYC_Num)//没有发送完成
        {
            numyc = IEC101_AP_ALL_YC(numyc);
            return;
        }
     }
	if(sys_mod_p->usYX_Num>=1)
     {
        if(numyx != sys_mod_p->usYX_Num)
        {
            numyx = IEC101_AP_ALL_YX(numyx);
            return;
        }
     }
	pIEC101_Struct->flag &=~SUMMON_BIT;
 	pIEC101_Struct->flag &=~SUMMON_CONFIRM_BIT;
    numyc = 0;
    numyx = 0;
	IEC101_Interroall_End();
}
/**********************************************************************************/
//函数说明:校验函数
//输入:getcnt数据存储的地方    length  多少个校验   flag  帧结构
//输出 :校验结果
//编辑:R&N1110			QQ:402097953
//时间:2014.10.22
/**********************************************************************************/
static unsigned char IEC101_CheckSum(unsigned short getcnt,unsigned char length,  unsigned char flag)
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
//函数说明:处理总召唤
//输入:无
//输出::
//编辑:R&N1110	QQ:402097953
//时间:2014.10.22
/**************************************************************************/
static void IEC101_Rqall_Con(void)
{
	unsigned char len1=0, len2=0, len3=0, len4=0;
	unsigned char ucCommand;
	unsigned short nsend=0;

	 len1 = pIEC101_Struct->linkaddrlen;
	 len2 = len1+pIEC101_Struct->cotlen;
	 len3 = len2+pIEC101_Struct->conaddrlen;
	 len4 = len3+pIEC101_Struct->infoaddlen;

	 memset( pIEC101_Struct->pSendBuf, 0, len4+10);
	 pIEC101_Struct->pSendBuf[0]= 0x68;
	 pIEC101_Struct->pSendBuf[1]= len4+4;
	 pIEC101_Struct->pSendBuf[2]= pIEC101_Struct->pSendBuf[1];
	 pIEC101_Struct->pSendBuf[3]= 0x68;

	 ucCommand = 0x0 | IEC_DIR_RTU2MAS_BIT;
	//if(IEC101_Chg_Search())
		ucCommand |=IEC_ACD_BIT;
	 pIEC101_Struct->pSendBuf[4]= ucCommand;
	 pIEC101_Struct->pSendBuf[5]= pIEC101_Struct->linkaddr;
	 pIEC101_Struct->pSendBuf[len1+5]=C_IC_NA_1;   //type
	 pIEC101_Struct->pSendBuf[len1+6]=0x01;		//可变结构体限定词
	 pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_ACTCON;//原因
	 pIEC101_Struct->pSendBuf[len2+7]=pIEC101_Struct->linkaddr; //站地址

	 pIEC101_Struct->pSendBuf[len3+7]=0;	//信息体地址
	 pIEC101_Struct->pSendBuf[len4+7]=IEC_QOI_INTROGEN; //品质因数
	 pIEC101_Struct->pSendBuf[len4+8]=IEC101_CheckSum(4,  (len4+8-3), IECFRAMESEND);//帧校验
	 pIEC101_Struct->pSendBuf[len4+9]=0x16;

       nsend = write(pIEC101_Struct->fd_usart,  pIEC101_Struct->pSendBuf, len4+10);
	 if(nsend == (len4+10))
	 	pIEC101_Struct->flag |=SUMMON_CONFIRM_BIT;
	Log_Frame(pIEC101_Struct->logfd,  pIEC101_Struct->pSendBuf,  0, len4+10, SEND_FRAME);
}

/**************************************************************************************/
//函数说明:查看是否有事件上报
//输入:无
//输出::
//编辑:R&N1110	QQ:402097953
//时间:2014.10.20
/**************************************************************************************/
static unsigned char IEC101_Chg_Search(void)
{
	if(pYXInfoGet->ucStatus== FULL)
    {
        my_debug("MSG_COS");
		return MSG_COS;
    }
	if(pSOEGet->ucStatus== FULL)
     {
      my_debug("MSG_SOE");
		return  MSG_SOE;
     }
	return(0);
}
/****************************************************************************/
//函数说明:结束初始化
//输入:无
//输出澹何?
//编辑:R&N1110	QQ402097953
//时间;2014.10.20
/****************************************************************************/
static void IEC101_EndInit( void )
{
	unsigned char len1=0, len2=0, len3=0, len4=0;
	unsigned char ucCommand;

	 len1 = pIEC101_Struct->linkaddrlen;
	 len2 = len1+pIEC101_Struct->cotlen;
	 len3 = len2+pIEC101_Struct->conaddrlen;
	 len4 = len3+pIEC101_Struct->infoaddlen;

	 memset( pIEC101_Struct->pSendBuf, 0, len4+10);
	 pIEC101_Struct->pSendBuf[0]= 0x68;
	 pIEC101_Struct->pSendBuf[1]= len4+4;
	 pIEC101_Struct->pSendBuf[2]= pIEC101_Struct->pSendBuf[1];
	 pIEC101_Struct->pSendBuf[3]= 0x68;

	 ucCommand = 0x08 | IEC_DIR_RTU2MAS_BIT;
     if(IEC101_Chg_Search())
        ucCommand |=IEC_ACD_BIT;
	 pIEC101_Struct->pSendBuf[4]= ucCommand;
	 pIEC101_Struct->pSendBuf[5]= pIEC101_Struct->linkaddr;
	 pIEC101_Struct->pSendBuf[len1+5]=M_EI_NA_1;//初始化结束标记
	 pIEC101_Struct->pSendBuf[len1+6]=0x01;		//可变结构体限定词
	 pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_INIT;//原因
	 pIEC101_Struct->pSendBuf[len2+7]=pIEC101_Struct->linkaddr; //站地址

	 pIEC101_Struct->pSendBuf[len3+7]=0;	//信息体地址
	 pIEC101_Struct->pSendBuf[len4+7]=IEC_COI_RMRESET; //品质因数
	 pIEC101_Struct->pSendBuf[len4+8]=IEC101_CheckSum(4,  (len4+8-3), IECFRAMESEND);//帧校验
	 pIEC101_Struct->pSendBuf[len4+9]=0x16;

       write(pIEC101_Struct->fd_usart,  pIEC101_Struct->pSendBuf, len4+10);
       Log_Frame(pIEC101_Struct->logfd,  pIEC101_Struct->pSendBuf,  0, len4+10, SEND_FRAME);

     pIEC101_Struct->flag &=~ END_INIT_BIT;
}
/**************************************************************************/
//子站响应主站请求链路状态函数说明: 在尚未收到复位远方链路命令之前,
//仅返回链路正常报文. 如果已收到复位远方链路命令(复位标志置1),
//此时请求链路状态将置ACD标志,表示有一级数据产生,并且清复位标志,
//置初始化结束标志为1,为第一个召唤一级数据作准备. 在此之后再收到
//请求链路状态命令, 则仅 返回链路正常报文, 参看101规约page39.
/**************************************************************************/
static void IEC101_Rqlink_Res(void)
{
	unsigned char ucCommand;

	if(pIEC101_Struct->flag & RST_LINK_BIT)	//已经收到远方链路复位命令
	{
		ucCommand =M_RQ_NA_1_LNKOK | IEC_DIR_RTU2MAS_BIT | IEC_ACD_BIT;
		pIEC101_Struct->flag &=~RST_LINK_BIT;
		pIEC101_Struct->flag |= END_INIT_BIT;
	}
	else
	{
		ucCommand =M_RQ_NA_1_LNKOK | IEC_DIR_RTU2MAS_BIT;
		if(IEC101_Chg_Search())
			ucCommand |=IEC_ACD_BIT;	//有数据上传
	}
	IEC101_Send10H(ucCommand);
}
/**************************************************************************/
//函数说明:复位远方链路
//输入:无
//输出澹何?
//编辑	:R&N1110	QQ:402097953
//时间:2014.10.20
/**************************************************************************/
static void IEC101_Rstlink_Con( void )
{
	unsigned char ucCommand;

	pIEC101_Struct->flag |=RST_LINK_BIT;
	ucCommand =0x00 | IEC_DIR_RTU2MAS_BIT;

	if(IEC101_Chg_Search())
		ucCommand |=IEC_ACD_BIT;
	IEC101_Send10H(ucCommand);
}

/**************************************************************************/
//函数说明:处理固定帧的处理函数
//输入:帧长度
//输出::
//编辑:R&N1110	QQ:402097953
//时间:2015.04.23
/**************************************************************************/
static void IEC101_Process_Fixframe( int frame_len)
{
	unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;
	unsigned char control_area=save_recv_data[(recvcnt+1)&0x3FF]& IEC_FUNC_BIT;

	Log_Frame(pIEC101_Struct->logfd,  save_recv_data,  pIEC101_Struct->pBufGet&0x3FF, frame_len, RECV_FRAME);

	switch (control_area)
	{
		case C_RQ_NA_1:              //0x09:召唤链路状态
			IEC101_Rqlink_Res();
			break;
		case C_RL_NA_1:              //0x00:复位远方链路
			IEC101_Rstlink_Con();
			break;
		case C_P1_NA_1:            	// 0x0a:召唤1级用户数据
			if(pIEC101_Struct->flag & END_INIT_BIT)  //初始化结束标志,表示有初始化结束一级数据
			{
				IEC101_EndInit();
				break;
			}
            /*
			if(IEC101_Chg_Search()==MSG_COS)   //发生遥信变位
			{
			    IEC101_Process_COS();
				break;
			}
			if(IEC101_Chg_Search()==MSG_SOE)   //发生遥信变位
			{
			    IEC101_Process_SOE();
				break;
			}*/
//			if(pIEC_Struct[ucPort]->ucYK_Exec_Result) //遥控结果
//			{
//				IEC_YKExecSend(ucPort,pIEC_Struct[ucPort]->ucYK_Exec_Result);
//				pIEC_Struct[ucPort]->ucYK_Exec_Result = 0;
//				break;
//			}
			IEC101_SendE5H(); 			    //发送没有数据可以发送
			break;
  	  	case C_P2_NA_1: 					//0x0b   召唤2级用户数据

			if(SUMMON_BIT&pIEC101_Struct->flag)
			{
				if(pIEC101_Struct->flag &SUMMON_CONFIRM_BIT)  //总招已经确认
					IEC101_Ap_Summonall();
                    if(IEC101_Chg_Search()==MSG_COS)   //发生遥信变位
                    			{
                    			    IEC101_Process_COS();
                    				break;
                    			}
                    			if(IEC101_Chg_Search()==MSG_SOE)   //发生遥信变位
                    			{
                    			    IEC101_Process_SOE();
                    				break;
                    			}
				else											//总招没有确认
					IEC101_Rqall_Con();
			}
//以下为ABB方式,不支持平衡方式传输，所以遥测分组值的分帧应该在召唤
//二级数据的时候查询是否存在并上送。总召唤和分组召唤都是如此。
//				if(pIEC_Struct[ucPort]->ucYK_Exec_Result) //遥控结果
//				{
//					IEC_YKExecSend(ucPort,pIEC_Struct[ucPort]->ucYK_Exec_Result);
//					pIEC_Struct[ucPort]->ucYK_Exec_Result = 0;
//					break;
//				}


//
//			if(pIEC_Struct[ucPort]->ucACD_Flag)
//			{
//				if(IEC_SPI(ucPort))
//					break;
//			}
//			if(IEC_SOE_Search(ucPort))
//			{
//				if(IEC_SOE(ucPort))
//					break;
//			}
//
//			if(pYCInfoGet[ucPort]->usStatus == FULL)
//			{
//				if(IEC_OVERSTEP_AI(ucPort))
//					break;
//			}
			IEC101_SendE5H();
			break;
		default:
			IEC101_SendE5H();
			break;

	}
}

/***********************************************************************/
//函数:static unsigned char IEC101_Process_COS( void ) //应用层处理
//说明:处理单点遥信变位
//输入:
//输出:
//编辑:R&N1110@126.com
//时间:2015.05.9
/***********************************************************************/
static unsigned char IEC101_Process_COS( void ) //应用层处理
{
    short len,i;
    Point_Info   * pTmp_Point=NULL;
	unsigned char len1=0, len2=0, len3=0;


    if(pIEC101_Struct->fd_usart <=0)
        return -1;

	 len1 = pIEC101_Struct->linkaddrlen;
	 len2 = len1+pIEC101_Struct->cotlen;
	 len3 = len2+pIEC101_Struct->conaddrlen;

	 memset( pIEC101_Struct->pSendBuf, 0, IEC101BUFLEN);
	 pIEC101_Struct->pSendBuf[4]= 0x08 | IEC_DIR_RTU2MAS_BIT;
	 pIEC101_Struct->pSendBuf[5]= pIEC101_Struct->linkaddr;
	 pIEC101_Struct->pSendBuf[len1+5]=M_SP_NA_1;   //type
     pIEC101_Struct->pSendBuf[len1+6]= 0;          //可变结构体限定词
	 pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_SPONT;//原因
	 pIEC101_Struct->pSendBuf[len1+len2+7]=pIEC101_Struct->linkaddr; //站地址

    len = 0;
    while(pYXInfoGet->ucStatus == FULL)
    {
        //遍历链表找到对应的遥信相
        pTmp_Point = sys_mod_p->pYX_Addr_List_Head;
        while(pTmp_Point->uiOffset != pYXInfoGet->usIndex)
        {
            pTmp_Point = pTmp_Point->next;
            if(pTmp_Point == NULL)
                break;
        }
        if(pTmp_Point != NULL)
        {
            for(i=0; i<pIEC101_Struct->infoaddlen ; i++)
            {
                pIEC101_Struct->pSendBuf[len3+7+len] = (unsigned char)((pTmp_Point->uiAddr>>(i*8))&0xff);
                len ++;
            }
            if(pYXInfoGet->ucYXValue)
      		    pIEC101_Struct->pSendBuf[len3+7+len++] = 0x1;            //品质因数
      		else
                pIEC101_Struct->pSendBuf[len3+7+len++] = 0x0;            //品质因数
    		pIEC101_Struct->pSendBuf[len1+6]=pIEC101_Struct->pSendBuf[len1+6]+1 ;                         //个数
    	}
        else
            my_debug("iec101 cos cannot find the index");
		pYXInfoGet->ucStatus =  EMPTY;
        pYXInfoGet = pYXInfoGet->next;
        if(pYXInfoGet == NULL)
            break;
        if(pIEC101_Struct->pSendBuf[len1+6]>=30)
            break;
    }
	if(pIEC101_Struct->pSendBuf[len1+6]>0)
	{
    	 pIEC101_Struct->pSendBuf[len3+7+len]=IEC101_CheckSum(4, len3+7+len, IECFRAMESEND);//帧校验
    	 pIEC101_Struct->pSendBuf[len3+8+len]=0x16;

    	 pIEC101_Struct->pSendBuf[0]= 0x68;
    	 pIEC101_Struct->pSendBuf[1]= len3+3+len;
    	 pIEC101_Struct->pSendBuf[2]= pIEC101_Struct->pSendBuf[1];
    	 pIEC101_Struct->pSendBuf[3]= 0x68;

        write(pIEC101_Struct->fd_usart,  pIEC101_Struct->pSendBuf, len3+9+len);
	    Log_Frame(pIEC101_Struct->logfd,  pIEC101_Struct->pSendBuf,  0, len3+9+len, SEND_FRAME);
        return(0);
	}
	else
    {
       	my_debug("cos num=0");
		return(0);
    }
 return -1;
}
/***********************************************************************/
//函数:static unsigned char IEC101_Process_SOE( void ) //应用层处理
//说明:处理单点遥信变位
//输入:无
//输出:
//编辑:R&N1110@126.com
//时间:2015.05.9
/***********************************************************************/
unsigned char IEC101_Process_SOE( void )
{
    short len,i;
    Point_Info   * pTmp_Point=NULL;
	unsigned char len1=0, len2=0, len3=0;


    if(pIEC101_Struct->fd_usart <=0)
        return -1;

	 len1 = pIEC101_Struct->linkaddrlen;
	 len2 = len1+pIEC101_Struct->cotlen;
	 len3 = len2+pIEC101_Struct->conaddrlen;

	 memset( pIEC101_Struct->pSendBuf, 0, IEC101BUFLEN);
	 pIEC101_Struct->pSendBuf[4]= 0x08 | IEC_DIR_RTU2MAS_BIT;
	 pIEC101_Struct->pSendBuf[5]= pIEC101_Struct->linkaddr;
	 pIEC101_Struct->pSendBuf[len1+5]=M_SP_TA_1;   //type
     pIEC101_Struct->pSendBuf[len1+6]= 0;          //可变结构体限定词
	 pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_SPONT;//原因
	 pIEC101_Struct->pSendBuf[len1+len2+7]=pIEC101_Struct->linkaddr; //站地址

    len = 0;
    while(pSOEGet->ucStatus == FULL)
    {
        //遍历链表找到对应的遥信相
        pTmp_Point = sys_mod_p->pYX_Addr_List_Head;
        while(pTmp_Point->uiOffset != pSOEGet->usIndex)
        {
            pTmp_Point = pTmp_Point->next;
            if(pTmp_Point == NULL)
                break;
        }
        if(pTmp_Point != NULL)
        {
            for(i=0; i<pIEC101_Struct->infoaddlen ; i++)
            {
                pIEC101_Struct->pSendBuf[len3+7+len] = (unsigned char)((pTmp_Point->uiAddr>>(i*8))&0xff);
                len ++;
            }
            if(pSOEGet->ucYXValue)
      		    pIEC101_Struct->pSendBuf[len3+7+len++] = 0x1;            //品质因数兼任数据
      		else
                pIEC101_Struct->pSendBuf[len3+7+len++] = 0x0;            //品质因数

            pIEC101_Struct->pSendBuf[len3+7+len++] = (unsigned char)( pSOEGet->usMSec & 0xff);
            pIEC101_Struct->pSendBuf[len3+7+len++] = (unsigned char)(pSOEGet->usMSec >> 8);
            pIEC101_Struct->pSendBuf[len3+7+len++]=   pSOEGet->ucMin;
            pIEC101_Struct->pSendBuf[len3+7+len++]=   pSOEGet->ucHour;
            pIEC101_Struct->pSendBuf[len3+7+len++] =  pSOEGet->ucDay;
            pIEC101_Struct->pSendBuf[len3+7+len++]=   (pSOEGet->ucMonth+1);
            pIEC101_Struct->pSendBuf[len3+7+len++] =  pSOEGet->ucYear-100;

    		pIEC101_Struct->pSendBuf[len1+6]=pIEC101_Struct->pSendBuf[len1+6]+1 ;                         //个数
    	}
        else
            my_debug("iec101 soe cannot find the index");
		pSOEGet->ucStatus =  EMPTY;
        pSOEGet = pSOEGet->next;
        if(pSOEGet == NULL)
            break;
        if(pIEC101_Struct->pSendBuf[len1+6]>=20)
            break;
    }
	if(pIEC101_Struct->pSendBuf[len1+6]>0)
	{
    	 pIEC101_Struct->pSendBuf[len3+7+len]=IEC101_CheckSum(4, len3+7+len, IECFRAMESEND);//帧校验
    	 pIEC101_Struct->pSendBuf[len3+8+len]=0x16;

    	 pIEC101_Struct->pSendBuf[0]= 0x68;
    	 pIEC101_Struct->pSendBuf[1]= len3+3+len;
    	 pIEC101_Struct->pSendBuf[2]= pIEC101_Struct->pSendBuf[1];
    	 pIEC101_Struct->pSendBuf[3]= 0x68;

        write(pIEC101_Struct->fd_usart,  pIEC101_Struct->pSendBuf, len3+9+len);
	    Log_Frame(pIEC101_Struct->logfd,  pIEC101_Struct->pSendBuf,  0, len3+9+len, SEND_FRAME);
        return(0);
	}
	else
    {
       	my_debug("cos num=0");
		return(0);
    }
 return -1;
}
/**************************************************************************/
//函数说明: 反馈子站时间
//输入:帧长度
//输出::
//编辑:R&N1110	QQ:402097953
//时间:2014.10.20
/**************************************************************************/
static void IEC101_SendTime(int length)
{
	unsigned short  usMSec;
	long  usUSec;				//微秒精度
	unsigned char  ucSec;
	unsigned char len1=0, len2=0, len3=0, len4=0;
	unsigned char ucCommand;
	char * buf = (char * )save_recv_data;
	unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;
	struct rtc_time  * stime=malloc(sizeof(struct rtc_time));

	 len1 = pIEC101_Struct->linkaddrlen;
	 len2 = len1 + pIEC101_Struct->cotlen;
	 len3 = len2 + pIEC101_Struct->conaddrlen;
	 len4 = len3  +pIEC101_Struct->infoaddlen;

	usMSec = (buf[(recvcnt+length-8)&0x3FF]<<8)|buf[(recvcnt+length-9)&0x3FF];

	ucSec = (unsigned char)(usMSec/1000);
	if(ucSec>59)
		return;
	usUSec = usMSec%1000; //这个是ms
	usUSec = usUSec *1000; //这个是us
	stime->tm_min = buf[(recvcnt+length-7)&0x3FF]&0x3f;
	if(stime->tm_min>59)
		return;

	stime->tm_hour = buf[(recvcnt+length-6)&0x3FF]&0x1f;
	if(stime->tm_hour>23)
		return;

	stime->tm_mday = buf[(recvcnt+length-5)&0x3FF]&0x1f;
	if(stime->tm_mday>31)
		return;

	stime->tm_mon = buf[(recvcnt+length-4)&0x3FF]&0x0f;
	if(stime->tm_mon>12)
		return;

	stime->tm_year = buf[(recvcnt+length-3)&0x3FF]&0x7F;
	if(stime->tm_year>99)
		return;
	Set_Rtc_Time(stime,  usUSec);	//设置RTC时间


	memset(pIEC101_Struct->pSendBuf, 0, length);
	 pIEC101_Struct->pSendBuf[0]= 0x68;
	 pIEC101_Struct->pSendBuf[1]= length-6;
	 pIEC101_Struct->pSendBuf[2]= pIEC101_Struct->pSendBuf[1];
	 pIEC101_Struct->pSendBuf[3]= 0x68;

	ucCommand = 0x08 | IEC_DIR_RTU2MAS_BIT;
	if(IEC101_Chg_Search())
		ucCommand |=IEC_ACD_BIT;
	 pIEC101_Struct->pSendBuf[4]= ucCommand;
	 pIEC101_Struct->pSendBuf[5]= pIEC101_Struct->linkaddr;
	 pIEC101_Struct->pSendBuf[len1+5]=C_CS_NA_1;
	 pIEC101_Struct->pSendBuf[len1+6]=0x01;		//可变结构体限定词
	 pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_ACTCON;//原因
	 pIEC101_Struct->pSendBuf[len2+7]=pIEC101_Struct->linkaddr; //站地址
	 pIEC101_Struct->pSendBuf[len3+7]=0;	//信息体地址
	 pIEC101_Struct->pSendBuf[len4+7]=buf[(recvcnt+length-9)&0x3FF]; //信息内容
	 pIEC101_Struct->pSendBuf[len4+8]=buf[(recvcnt+length-8)&0x3FF]; //信息内容
	 pIEC101_Struct->pSendBuf[len4+9]=stime->tm_min; //信息内容
	 pIEC101_Struct->pSendBuf[len4+10]=stime->tm_hour; //信息内容
	 pIEC101_Struct->pSendBuf[len4+11]=stime->tm_mday; //信息内容
	 pIEC101_Struct->pSendBuf[len4+12]=stime->tm_mon; //信息内容
	 pIEC101_Struct->pSendBuf[len4+13]=stime->tm_year; //信息内容
	 //pIEC101_Struct->pSendBuf[len4+14]=IEC101_CheckSum(4, length-2, IECFRAMESEND);//帧校验
	 	 pIEC101_Struct->pSendBuf[len4+14]=IEC101_CheckSum(4, (len4+13-2), IECFRAMESEND);//帧校验

	 pIEC101_Struct->pSendBuf[len4+15]=0x16;
        write(pIEC101_Struct->fd_usart,  pIEC101_Struct->pSendBuf, len4+16);
	free(stime);
	Log_Frame(pIEC101_Struct->logfd,  pIEC101_Struct->pSendBuf,  0, len4+16, SEND_FRAME);
}

/**************************************************************************/
//函数说明:处理可变帧的处理函数
//输入:帧长度
//输出::
//编辑:R&N1110	QQ:402097953
//时间:2014.10.20
/**************************************************************************/
static void IEC101_Process_Varframe( int frame_len)
{
	char * buf =(char *)  save_recv_data;
	unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;
	unsigned char control_area=buf[(recvcnt+4)&0x3FF]& IEC_FUNC_BIT;
	unsigned char type= buf[(recvcnt+ pIEC101_Struct->linkaddrlen+5)&0x3FF];

 	if(buf[recvcnt+5] !=pIEC101_Struct->linkaddr)				//地址错误
		return;

	Log_Frame(pIEC101_Struct->logfd,  save_recv_data,  recvcnt, frame_len, RECV_FRAME);

	switch (control_area)
	{
	 case 0x03:
		switch(type)
		{
			case C_SC_NA_1:	//0x2d 单点遥控
			case C_DC_NA_1:   //0x2e 双点遥控
				IEC101_Single_YK(frame_len);
				break;
			case C_CS_NA_1:   //0x67-对时
				IEC101_SendTime(frame_len);
				break;
			case C_IC_NA_1:      //0x64－总召唤
				pIEC101_Struct->flag |=SUMMON_BIT;
				IEC101_Rqall_Con();
				break;
			default:
				break;
		}
		break;
	 default:
		break;
	}
}
/**********************************************************************************/
//函数说明:检验是否是规约101的帧
//输入:int fd_usart , unsigned short * buf_len(计算的帧的长度)
//输出::
//		IECFRAMEERR帧错误
//		IECFRAMEVAR可变帧
//		IECFRAMEFIX固定帧
//编辑:R&N1110		QQ:402097953
//时间?:2014.10.22
/**********************************************************************************/
static unsigned char IEC101_VerifyDLFrame(int fd_usart , int * buf_len)
{
	char * buf = (char *)save_recv_data;
	unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;
	unsigned char  len ;
	unsigned int  cnt=0;

	*buf_len = 0;


	if((buf[recvcnt]==0x68)&&(pIEC101_Struct->usRecvCont>=3))
	{
		len = buf[(recvcnt+1)&0x3FF];
		if((len==buf[(recvcnt+2)&0x3FF])&&(buf[(recvcnt+3)&0x3FF]==0x68))
		{
		    
            my_debug("sem_wait1\n");
			sem_wait(&sem_iec101);//信号量value减1
			while(pIEC101_Struct->usRecvCont < (len+6))//如果接收到的数据不够，就等待
			{
			  my_debug("sem_wait2\n");
			  sem_wait(&sem_iec101);
			  if(cnt++>16)
				return IECFRAMEERROVERTIME;
			}
			if(buf[(recvcnt+len+5)&0x3FF]!=0x16)
				return IECFRAMEERREND;
			if(buf[(recvcnt+len+4)&0x3FF]!=IEC101_CheckSum(recvcnt,  len, IECFRAMEVAR))
				return IECFRAMEERRSUM;

			*buf_len = len+6; 		//总长度
			return IECFRAMEVAR; 		//可变帧
		}
		*buf_len  = 0;
		return IECFRAMEERR;
	}
	else if(buf[recvcnt]==0x10)
	{
		sem_wait(&sem_iec101);
		while(pIEC101_Struct->usRecvCont < 5)
		{
			if(cnt++>10)
				return IECFRAMEERROVERTIME;
		}
		if(buf[(recvcnt+3+pIEC101_Struct->linkaddrlen)&0x3FF]!=0x16)
			return IECFRAMEERREND;

		if(buf[(recvcnt+2+pIEC101_Struct->linkaddrlen)&0x3FF]!=IEC101_CheckSum(recvcnt,  pIEC101_Struct->linkaddrlen+1, IECFRAMEFIX))
			return IECFRAMEERRSUM;
		*buf_len = pIEC101_Struct->linkaddrlen+4;
		return IECFRAMEFIX;
	}
	else if(buf[recvcnt]==0xE5)
	{
		*buf_len = 1;
		return IECFRAMEFIXE5;
	}
	return IECFRAMEERR;
}

/**************************************************************************/
//函数说明:101规约的入口
//输入:无
//输出:无
//编辑:R&N
/**************************************************************************/
static void IEC101_Protocol_Entry(  void  )
{
	int  frame_len;
	unsigned char   return_flag;

	if(pIEC101_Struct->usRecvCont>0)	//有数据
	{
		return_flag = IEC101_VerifyDLFrame(pIEC101_Struct->fd_usart, &frame_len);

		 if(return_flag == IECFRAMEVAR)
		{
			my_debug("VAR len:%d, %d, %d", frame_len, pIEC101_Struct->pBufGet, pIEC101_Struct->usRecvCont);
			IEC101_Process_Varframe(frame_len);
			pIEC101_Struct->usRecvCont  = pIEC101_Struct->usRecvCont- frame_len;
			pIEC101_Struct->pBufGet = (pIEC101_Struct->pBufGet+frame_len)&0x3FF;
		}
		else if(return_flag == IECFRAMEFIX)
		{
			my_debug("FIX len:%d %d %d", frame_len,  pIEC101_Struct->pBufGet, pIEC101_Struct->usRecvCont);
			IEC101_Process_Fixframe(frame_len);
			pIEC101_Struct->usRecvCont  = pIEC101_Struct->usRecvCont- frame_len;
			pIEC101_Struct->pBufGet = (pIEC101_Struct->pBufGet+frame_len)&0x3FF;
		}
		else if(return_flag == IECFRAMEFIXE5)
		{
			my_debug("FIXE5 len:%d %d %d", frame_len,  pIEC101_Struct->pBufGet, pIEC101_Struct->usRecvCont);
			pIEC101_Struct->usRecvCont = pIEC101_Struct->usRecvCont-1;
			pIEC101_Struct->pBufGet = (pIEC101_Struct->pBufGet+1)&0x3FF;
		}
		else
		{
			if(return_flag ==IECFRAMEERREND)
				my_debug("ERRfremeEND");
			else if(return_flag ==IECFRAMEERRSUM)
				my_debug("ERRfremeSUM");
			else if(return_flag ==IECFRAMEERROVERTIME)
				my_debug("IECFRAMEERROVERTIME");
			pIEC101_Struct->usRecvCont = pIEC101_Struct->usRecvCont-1;
			pIEC101_Struct->pBufGet = (pIEC101_Struct->pBufGet+1)&0x3FF;
			my_debug("pBUFget:%d", pIEC101_Struct->pBufGet);
			return;
		}
	}
}

/**************************************************************************/
//函数说明:处理需要发送的数据，或者SOE上报事件等
//输入:无
//输出:无
//编辑:R&N
/**************************************************************************/
static void IEC101_UnsoSend( void )
{
    
}
/***************************************************************************/
//函数: void *Iec101_Thread(void * arg)
//说明:用于处理从串口接收到的数据
//输入:
//输出:
//编辑:R&N1110@126.com
//时间:2015.4.21
/***************************************************************************/

void  *Iec101_Thread( void * data  )
{
    sys_mod_p = (LOCAL_Module *)data;
	while(1)
	{
		if(pIEC101_Struct->fd_usart >0)
		{
			if(pIEC101_Struct->usRecvCont>0)
			{
				if((save_recv_data[pIEC101_Struct->pBufGet&0x3FF]==0x68)
				||(save_recv_data[pIEC101_Struct->pBufGet&0x3FF]==0x10)
				||(save_recv_data[pIEC101_Struct->pBufGet&0x3FF]==0xE5))
					IEC101_Protocol_Entry();		//处理接收到的数据
				else
				{
					pIEC101_Struct->usRecvCont --;
					pIEC101_Struct->pBufGet = (pIEC101_Struct->pBufGet+1)&0x3FF;
				}
			}
			else
				 sem_wait(&sem_iec101_recv);

			IEC101_UnsoSend();
		}
		else
			usleep(500);
	}
	return 0;
}




/***************************************************************************/
//函数:  int  IEC101_Free_Buf(void)
//说明:释放掉缓冲
//输入:无
//输出:无
//编辑:R&N1110
//时间:2015.04.21
/***************************************************************************/
 int  IEC101_Free_Buf(void)
{
	free(pSOERoot);
	free(pYXInfoRoot);
	if(pIEC101_Struct->logfd >0)
		close(pIEC101_Struct->logfd);
	if(pIEC101_Struct->fd_usart>0)
		close(pIEC101_Struct->fd_usart);

	free(pIEC101_Struct);

	sem_destroy(&sem_iec101);//销毁信号量
	sem_destroy(&sem_iec101_recv);

	return 0;
}


/***************************************************************************/
//函数: void IEC101_Init_Buf(void)
//说明:初始化缓冲，包括收到数据保存的缓冲，并将连接成链表
//输入:无
//输出:无
//编辑:R&N1110@126.com
//时间:2015.4.23
/***************************************************************************/
 int  IEC101_Init_Buf(void)
{
	YXInfoStruct * pYXinfoList, * pYXinfoList1;
	SOEInfoStruct * pSOEList, * pSOEList1;
	unsigned short step;

	pIEC101_Struct = malloc(sizeof(IEC101Struct));
	if(pIEC101_Struct == NULL)
	{
		my_debug( "pIEC101_Struct malloc fail\n");
		goto leave1;
	}
	memset(pIEC101_Struct, 0, sizeof(IEC101Struct));

	pIEC101_Struct->pBufGet = 0;
	pIEC101_Struct->pBufFill  = 0;

	pIEC101_Struct->pSendBuf=save_send_data;

 //创建循环链表用来处理遥信变位
 	pYXInfoRoot = malloc(IEC101YXINFONUMBER*sizeof(YXInfoStruct));//4
     	if (pYXInfoRoot ==NULL)
	{
		my_debug("pYXInfoRoot malloc fail\n");
		goto leave3;
	}
	memset(pYXInfoRoot, 0, IEC101YXINFONUMBER);
 	pYXInfoFill = pYXInfoRoot;
 	pYXInfoGet  = pYXInfoRoot;
 	pYXinfoList = pYXInfoRoot;
 	pYXinfoList1 =pYXInfoRoot;
 	step=0;
 	while (step < IEC101YXINFONUMBER)
 	{
       	pYXinfoList->ucStatus = EMPTY;
       	pYXinfoList1++;
       	pYXinfoList->next = pYXinfoList1;
       	pYXinfoList++;
       	step++;
 	}
 	pYXinfoList--;
 	pYXinfoList->next =pYXInfoRoot;
//创建循环链表用来处理SOE事件
 	pSOERoot = malloc(IEC101SOENUMBER*sizeof(SOEInfoStruct));//4
     	if (pSOERoot ==NULL)
	{
		my_debug("pSOERoot malloc fail\n");
		goto leave4;
	}
	memset(pSOERoot, 0, IEC101SOENUMBER);
 	pSOEFill = pSOERoot;
 	pSOEGet  = pSOERoot;
 	pSOEList = pSOERoot;
 	pSOEList1 =pSOERoot;
 	step=0;
 	while (step < IEC101SOENUMBER)
 	{
       	pSOEList->ucStatus = EMPTY;
       	pSOEList1++;
       	pSOEList->next = pSOEList1;
       	pSOEList++;
       	step++;
 	}
     	pSOEList--;
     	pSOEList->next =pSOERoot;
	return 0;

leave4:
	free(pSOERoot);
leave3:
	free(pYXInfoRoot);
//leave2:
//	free(pIEC101_Struct->pSendBuf);
leave1:
	free(pIEC101_Struct);
	my_debug("malloc fail");
    return -1;
}
/********************************************************************************/
//函数说明:读取101规约中的串口配置
//输入:line配置文件中的每一行
//		  struct _IEC101Struct   * iec_parp全局变量pIEC101_Struct
//输出:无
//编辑:R&N1110		qq:402097953
//时间:2014.10.20
/********************************************************************************/
static unsigned char  Get_Dianbiao_From_Line(char * line, struct _IEC101Struct  * item)
{

	char * tmp_line=NULL;
	char * token = NULL;
	tmp_line=(char * )strstr((char *)line, "mode=");
	if(tmp_line!=NULL)
		{
			token = strtok( tmp_line, "#");
			item->mode = atoi( token+strlen("mode="));
			my_debug("item->mode:%d",item->mode);
			return 1;
		}
	tmp_line=(char * )strstr((char *)line, "linkaddr=");
	if(tmp_line!=NULL)
		{
			token = strtok( tmp_line, "#");
			item->linkaddr= atoi(token+strlen("linkaddr="));
			my_debug("item->linkaddr:%d",item->linkaddr);
			return 1;
		}
	tmp_line=(char * )strstr((char *)line, "linkaddrlen=");
	if(tmp_line!=NULL)
		{
			token = strtok( tmp_line, "#");
			item->linkaddrlen = atoi(token+strlen("linkaddrlen="));
			my_debug("item->linkaddrlen:%d",item->linkaddrlen);
			return 1;
		}
	tmp_line=(char * )strstr((char *)line, "conaddrlen=");
	if(tmp_line!=NULL)
		{
			token = strtok( tmp_line, "#");
			item->conaddrlen = atoi(token+strlen("conaddrlen="));
			my_debug("item->conaddrlen:%d",item->conaddrlen);
			return 1;
		}
	tmp_line=(char * )strstr((char *)line, "cotlen=");
	if(tmp_line!=NULL)
		{
			token = strtok( tmp_line, "#");
			item->cotlen = atoi(token+strlen("cotlen="));
			my_debug("item->cotlen:%d",item->cotlen);
			return 1;
		}
	tmp_line=(char * )strstr((char *)line, "infoaddlen=");
	if(tmp_line!=NULL)
		{
			token = strtok( tmp_line, "#");
			item->infoaddlen = atoi(token+strlen("infoaddlen="));
			my_debug("item->infoaddlen:%d",item->infoaddlen);
			return 1;
		}
	tmp_line=(char * )strstr((char *)line, "yctype=");
	if(tmp_line!=NULL)
		{
		token = strtok( tmp_line, "#");
		item->yctype = atoi(token+strlen("yctype="));
		my_debug("item->yctype:%d",item->yctype);
		return 1;
		}
	tmp_line=(char * )strstr((char *)line, "usart=");
	if(tmp_line!=NULL)
		{
		token = strtok( tmp_line, "#");
		item->usart = atoi(token+strlen("usart="));
//		my_debug("item->usart:%d",item->usart);
		return 1;
		}
	tmp_line=(char * )strstr((char *)line, "bitrate=");
	if(tmp_line!=NULL)
		{
		token = strtok( tmp_line, "#");
		item->bitrate = atoi(token+strlen("bitrate="));
//		my_debug("usart:%d bitrate:%d ",  item->usart, item->bitrate);
		return 1;
	}
	tmp_line=(char * )strstr((char *)line, "supporte5=");
	if(tmp_line!=NULL)		{
		token = strtok( tmp_line, "#");
		item->flag  = 0;
		if(atoi(token+strlen("supporte5=")) !=0)
			item->flag |=SUPPORTE5_BIT;
		return 1;
	}

	return 0;
}

/***************************************************************************/
//函数: static void IEC101_Read_Conf(char * fd,  struct _IEC104Struct   * iec104)
//说明:读取点表
//输入:无
//输出:无
//编辑:R&N1110@126.com
//时间:2015.4.23
/***************************************************************************/
static void IEC101_Read_Conf(char * fd,  struct _IEC101Struct   * iec101)
{
 	FILE *fp=NULL;
 	char  line[31];

	 fp = fopen(fd,  "rw");
	 if(fp == NULL){
		my_debug("Fail to open\n");
		pthread_exit("IEC101_Read_Conf fail!\n");
		}
	 fseek(fp, 0, SEEK_SET); 			//重新定义到开始位置
	 while (fgets(line, 31, fp))
 	{
 		Get_Dianbiao_From_Line(line,  iec101);
 	}
	 fclose(fp);
}

/**************************************************************************/
//函数说明:101规约的初始化
//输入:无
//输出:无
//编辑:R&N
//时间:2015.04.23
/**************************************************************************/
static  void IEC101_InitAllFlag( void )
{
	pIEC101_Struct->usRecvCont = 0;
	pIEC101_Struct->pBufFill = 0;
	pIEC101_Struct->pBufGet = 0;
    pIEC101_Struct->flag = 0;
}

/***************************************************************************/
//函数:void IEC101_Process_Message(msg->cmd, msg->buf, msg->data)
//说明:处理MESSAGE的函数
//输入:message的命令和buf
//输出:
//编辑:R&N1110@126.com
//时间:2015.05.09
/***************************************************************************/
void IEC101_Process_Message(UInt32 cmd, UInt32 buf, UInt32 data)
{
    struct rtc_time  * stime;
    UInt8   i;

    //my_debug("cmd101:0x%x buf:0x%x data:0x%x", cmd, buf, data);
    switch (cmd&0xFFFF)
    {
        case MSG_COS:
            for(i=0; i<32; i++)
            {
                if(buf&(1<<i))//表示第(cmd>>16)*32+i通道变位
                {
                    if(pYXInfoFill->ucStatus == EMPTY)
                    {
                        pYXInfoFill->usIndex = ((((cmd>>16)&0xFFFF)<<5)+i)&0xFFFF;
                        pYXInfoFill->ucYXValue = (data>>i)&0x1;
                        pYXInfoFill->ucStatus = FULL;
                        pYXInfoFill = pYXInfoFill->next;
                    }
                }
            }
            break;
        case MSG_SOE:
             stime=malloc(sizeof(struct rtc_time));
            for(i=0; i<32; i++)
            {
                if(buf&(1<<i))//表示第(cmd>>16)*32+i通道变位
                {
                    if(pSOEFill->ucStatus == EMPTY)
                    {
                        pSOEFill->usIndex = ((((cmd>>16)&0xFFFF)<<5)+i)&0xFFFF;
                        pSOEFill->ucYXValue = (data>>i)&0x1;
                        pSOEFill->ucStatus = FULL;
                        pSOEFill->usMSec = Get_Rtc_Time(stime);
                        pSOEFill->ucMin = stime->tm_min;
                        pSOEFill->ucHour =stime->tm_hour;
                        pSOEFill->ucDay = stime->tm_mday;
                        pSOEFill->ucMonth = stime->tm_mon;
                        pSOEFill->ucYear = stime->tm_year;
//                        pSOEFill->ucStatus = FULL;
                        pSOEFill = pSOEFill->next;
                    }
                }
            }
            free(stime);
            break;
        default:
            break;
    }
}


/*****************************************************************/
//Name:       set_speed
//Function:   设置波特率
//Input:      无Return:     FALSE TRUE
/******************************************************************/
static int set_speed(int fd,  int speed)
{
	int i;
	int status;
	struct termios Opt;

	tcgetattr(fd, &Opt);

	for ( i= 0; i < sizeof(speed_arr) / sizeof(int); i++)
	{
		if (speed == name_arr[i])
		{
			tcflush(fd, TCIOFLUSH);
			cfsetispeed(&Opt, speed_arr[i]);
			cfsetospeed(&Opt, speed_arr[i]);

			status = tcsetattr(fd, TCSANOW, &Opt);
			if (status != 0)
			{
				perror("tcsetattr fd1");
				return -1;
			}

			tcflush(fd,TCIOFLUSH);
			return 0;
		}
    }
	return -1;
}

/***************************************************
//Name:       set_parityFunction:   设置串口数据位，停止位和效验位
//Input:      1.fd          打开的串口文件句柄
//		 2.databits    数据位  取值  为 7  或者 8
//		 3.stopbits    停止位  取值为 1 或者2
//		 4.parity      效验类型  取值为 N,E,O,S
//Return:
****************************************************/
static int set_parity(int fd, int databits, int stopbits, int parity)
{
	struct termios options;

	if ( tcgetattr( fd,&options) != 0)
	{
		my_debug("SetupSerial 1");
		return(-1);
    }
	options.c_cflag &= ~CSIZE;

	/* 设置数据位数*/
	switch (databits)
	{
		case 7:
			options.c_cflag |= CS7;
			break;

		case 8:
			options.c_cflag |= CS8;
			break;

		default:
			fprintf(stderr,"Unsupported data sizen");
			return (-1);
	}

	switch (parity)
	{
		case 'n':
		case 'N':
			options.c_cflag &= ~PARENB; 				/* Clear parity enable */
			options.c_iflag &= ~INPCK;					/* Enable parity checking */
			break;

		case 'o':
		case 'O':
			options.c_cflag |= (PARODD | PARENB);
			options.c_iflag |= INPCK;					/* Disnable parity checking */
			break;

		case 'e':
		case 'E':
			options.c_cflag |= PARENB;					/* Enable parity */
			options.c_cflag &= ~PARODD;
			options.c_iflag |= INPCK; 					/* Disnable parity checking */
			break;

		case 'S':
		case 's': 										/*as no parity*/
			options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;
			break;

		default:
			fprintf(stderr,"Unsupported parityn");
			return (-1);
	}

    /*  设置停止位*/
	switch (stopbits)
	{
		case 1:
			options.c_cflag &= ~CSTOPB;
			break;

		case 2:
			options.c_cflag |= CSTOPB;
			break;

		default:
			my_debug("Unsupported stop bitsn");
			return (-1);
	}

	/* Set input parity option */
	if (parity != 'n')
		options.c_iflag |= INPCK;

	tcflush(fd,TCIFLUSH);
	options.c_iflag = 0;
    	options.c_oflag = 0;
    	options.c_lflag = 0;
	options.c_cc[VTIME] = 150;							// 设置超时 15 seconds
	options.c_cc[VMIN] = 0;								// Update the options and do it NOW
	if (tcsetattr(fd,TCSANOW,&options) != 0)
	{
		perror("SetupSerial 3");
		return (-1);
	}
	return (0);
}

/*****************************************************************/
//Name:       open_dev
//Function:   打开串口设备
//Input:      无Return:     FALSE TRUE
//Author:R&N1110		qq:402097953
//Time:2014.10.20
/******************************************************************/
static int open_dev(char *dev)
{
	int fd= open(dev, O_RDWR );

	if (-1 == fd)
	{
		my_debug("Can't Open Serial Port:%s\n",dev);
		return -1;
	}
	else
		return fd;
}
static  int IEC101_Open_Console(void)
{
	char  file_name[10];
	 memset(file_name, 0 , 10);
	 sprintf(file_name, "/dev/ttyS%d",  pIEC101_Struct->usart);
	 pIEC101_Struct->fd_usart = open_dev( file_name );
	 if(pIEC101_Struct->fd_usart < 0)
	 	return -1;
 	set_speed(pIEC101_Struct->fd_usart, pIEC101_Struct->bitrate);
// 	set_speed(pIEC101_Struct->fd_usart, 115200);
	set_parity(pIEC101_Struct->fd_usart, 8, 1, 'N');
		return 0;
}

/***************************************************************************/
//函数: int  Uart_Thread( void )
//说明:串口接收线程，用于为iec101服务
//输入:无
//输出:无
//编辑:R&N1110@126.com
//时间:2015.4.23
/***************************************************************************/
int  Uart_Thread( void )
{
	int nrecvdata = 0 , i;
	char  buf_rec[20];

	sem_init(&sem_iec101, 0, 0);//创建一个未命名的信号量
	sem_init(&sem_iec101_recv, 0, 0);
	pIEC101_Struct->logfd =Init_Logfile(IEC101LOGFFILE);
	if(pIEC101_Struct->logfd == -1)
	{
		my_debug("Creat logfd fail\n");
		return -1;
	}
	IEC101_InitAllFlag();
	IEC101_Read_Conf(IEC101CONFFILE,  pIEC101_Struct);
	if(IEC101_Open_Console() < 0)
	{
		my_debug("IEC101_Open_Console fail\n");
		return -1;
	}
    /*
    if(Uart2_Open_Console() < 0)
	{
		my_debug("Uart2_Open_Console fail\n");
		return -1;
	}*/
	while(1)
	{
	   if(pIEC101_Struct->fd_usart >0)//端口连接
	   	{
			if(pIEC101_Struct->pBufFill<=1003)
			{
				nrecvdata = read(pIEC101_Struct->fd_usart,  save_recv_data+pIEC101_Struct->pBufFill ,  20);

				pIEC101_Struct->pBufFill = pIEC101_Struct->pBufFill + nrecvdata;
				pIEC101_Struct->usRecvCont = pIEC101_Struct->usRecvCont + nrecvdata;
				if(nrecvdata >0)
				{
//				    my_debug("sem_post\n");
					sem_post(&sem_iec101);//信号量值增1
					sem_post(&sem_iec101_recv);
				}
//				my_debug("rec:%d total:%d", nrecvdata, pIEC101_Struct->usRecvCont);
			}
			else
			{
				nrecvdata = read(pIEC101_Struct->fd_usart, buf_rec , 20);
				for(i=0; i<nrecvdata; i++)
				{
				  save_recv_data[pIEC101_Struct->pBufFill&0x3FF] = buf_rec[i];
				  pIEC101_Struct->pBufFill=(pIEC101_Struct->pBufFill+1)&0x3FF;
				}
				pIEC101_Struct->usRecvCont = pIEC101_Struct->usRecvCont + nrecvdata;
				if(nrecvdata >0)
				{
					sem_post(&sem_iec101);
					sem_post(&sem_iec101_recv);
				}
//				my_debug("rec:%d total:%d", nrecvdata, pIEC101_Struct->usRecvCont);
			}
			nrecvdata = 0;
	   	}
	   else
		break;
	}
	return -1;
}

/**************************************************************************/
//函数:void IEC101_Senddata2pc(int fd, char *tmp_buf, UInt16 len)
//说明:用于发送监控数据到PC
//输入:fd用来判断是iec101还是iec104  tmp_buf需要发送的数据  len需要发送的数据长度
//输出:无
//编辑:R&N
/**************************************************************************/
void IEC101_Senddata2pc(int fd, char *tmp_buf, UInt8 len)
{
    if(fd == pIEC101_Struct->logfd)
    {
        if(sys_mod_p->pc_flag &(1<<MSG_GET_IEC101))
            PC_Send_Data(MSG_GET_IEC101, (UInt8 *)tmp_buf, len);
    }
}
