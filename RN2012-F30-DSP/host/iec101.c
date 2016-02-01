/*************************************************************************/
// iec101.c                                        Version 1.00
//
// ���ļ���DTU2.0ͨѶ����װ�õ�IEC60870-5-101��Լ�������
// ��д��:R&N
//
// ����ʦ:R&N
//  ��	   ��:2015.04.17
//  ע	   ��:
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


/******************ȫ�ֱ������洢�󲿷�101��Լ�еĲ���******/
IEC101Struct  * pIEC101_Struct;

unsigned char ucYKLock;/*ң�ر�����־
                            0-�ޱ���
                            1-�����ر�����
                            2-����Լ1����,
                            3-����Լ2������
                            4-����̫����Լ���� ,
                            5-PLCֱ��ң�ر���,
                            6-��Զ���͵ر���
                      */
static unsigned char       	save_recv_data[IEC101BUFNUM * IEC101BUFLEN];
static unsigned char       	save_send_data[IEC101BUFLEN];
static LOCAL_Module         * sys_mod_p;

static YXInfoStruct         * pYXInfoRoot;     //����ң�ű�λ��¼
static YXInfoStruct         * pYXInfoFill;
static YXInfoStruct         * pYXInfoGet;

static SOEInfoStruct        * pSOERoot;         //����ң��SOE�¼�
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
static void IEC101_Send10H(unsigned char ucCommand) ;      /* �ɱ䱨�ĳ��ȵ�RTU�ش� */
static unsigned char IEC101_VerifyDLFrame(int fd_usart , int * buf_len);
static void IEC101_Interroall_End(void);
static short IEC101_AP_ALL_YC( short send_over );//����ȫ��ң������
static unsigned char IEC101_AP_ALL_YX( short send_num );//��������ң������
static unsigned char IEC101_Process_COS( void ) ;//Ӧ�ò㴦��
static unsigned char IEC101_Process_SOE( void ) ;//Ӧ�ò㴦��
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
//����˵��:�ظ�0xE5
//����:����λ
//���::��
//�༭:R&N1110	QQ:402097953
//ʱ��:2014.10.20
/**************************************************************************************/
static void IEC101_SendE5H( void )
{
 if(pIEC101_Struct->flag&SUPPORTE5_BIT)    //֧��E5����
 {
 	 pIEC101_Struct->pSendBuf[0] = 0xE5;
        write(pIEC101_Struct->fd_usart,  pIEC101_Struct->pSendBuf, 1);
    	 Log_Frame(pIEC101_Struct->logfd,  pIEC101_Struct->pSendBuf,  0, 1, SEND_FRAME);
 }
  else
 	IEC101_Send10H(IEC_CAUSE_DEACTCON);
}

/**************************************************************************************/
//����˵��:�ظ��̶�֡��ʽ
//����:����λ
//���::��
//�༭:R&N1110	QQ:402097953
//ʱ��:2014.10.20
/**************************************************************************************/
static void IEC101_Send10H(unsigned char ucCommand)       /* �ɱ䱨�ĳ��ȵ�RTU�ش� */
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
//����:void IEC104_Interroall_End(void)
//˵��:�������ٻ�
//����:��
//���:��
//�༭:R&N
//ʱ��:2015.05.25
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
	 pIEC101_Struct->pSendBuf[len1+6]=0x01;		//�ɱ�ṹ���޶���
	 pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_ACTTERM;//ԭ��
	 pIEC101_Struct->pSendBuf[len2+7]=pIEC101_Struct->linkaddr; //վ��ַ

	 pIEC101_Struct->pSendBuf[len3+7]=0;	//��Ϣ���ַ
	 pIEC101_Struct->pSendBuf[len4+7]=IEC_QOI_INTROGEN; //Ʒ������
	 pIEC101_Struct->pSendBuf[len4+8]=IEC101_CheckSum(4,  (len4+8-3), IECFRAMESEND);//֡У��
	 pIEC101_Struct->pSendBuf[len4+9]=0x16;

    nsend = write(pIEC101_Struct->fd_usart,  pIEC101_Struct->pSendBuf, len4+10);
	Log_Frame(pIEC101_Struct->logfd,  pIEC101_Struct->pSendBuf,  0, len4+10, SEND_FRAME);
}
/**************************************************************************/
//����:static short IEC101_AP_ALL_YC( short send_num )
//����:��������ң������,���͸�����
//����:�Ѿ�������ң��ĸ���
//���:�Ѿ�������ң��ĸ���
//�༭:R&N
//ʱ��:2015.5.27
/**************************************************************************/
static short IEC101_AP_ALL_YC( short send_num )//����ȫ��ң������
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
		ucCommand |=IEC_ACD_BIT;//er���û�������ͣ����Ҫ����һ���û�����
	 pIEC101_Struct->pSendBuf[4]= ucCommand;
	 pIEC101_Struct->pSendBuf[5]= pIEC101_Struct->linkaddr;
	 pIEC101_Struct->pSendBuf[len1+5]=M_ME_NC_1;   //type
	 if(sys_mod_p->Continuity & YC_NOTCONTINUE) 	//��ַ������
		pIEC101_Struct->pSendBuf[len1+6]= 0;        //�ɱ�ṹ���޶���
     else
		pIEC101_Struct->pSendBuf[len1+6]= IEC104_VSQ_SQ_BIT;//�ɱ�ṹ���޶���

	 pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_INTROGEN;//ԭ��
	 pIEC101_Struct->pSendBuf[len1+len2+7]=pIEC101_Struct->linkaddr; //վ��ַ

    	pTmp_Point = sys_mod_p->pYC_Addr_List_Head;
     if(send_over != 0)
    {
        for(j=0; j<send_over; j++)
			if(pTmp_Point->next !=NULL)
				pTmp_Point = pTmp_Point->next;
    }

	for(k=0; k<(sys_mod_p->usYC_Num-send_over); )		//���Ͷ��ٴ�
	{
		len = 0;
		if(!(sys_mod_p->Continuity & YC_NOTCONTINUE))   //��ַ����
		{
            for(i=0; i<pIEC101_Struct->infoaddlen ; i++)
            {
                pIEC101_Struct->pSendBuf[len3+7+len] = (unsigned char)((pTmp_Point->uiAddr>>(i*8))&0xff);
                len ++;
            }
		}
		for(j=0; ((j<sys_mod_p->usYC_Num-send_over)&&( len<240)&&( j<(1<<5))); j++)
		{
			if(sys_mod_p->Continuity & YC_NOTCONTINUE) 	//��������ַ
			{
                for(i=0; i<pIEC101_Struct->infoaddlen ; i++)
                {
                    pIEC101_Struct->pSendBuf[len3+7+len] = (unsigned char)((pTmp_Point->uiAddr>>(i*8))&0xff);
                    len ++;
                }
			}
			data =(float)Read_From_Sharearea(pTmp_Point->uiOffset, TYPE_YC);//ң��ֵ
            if(pIEC101_Struct->yctype == TYPE_FLOAT)//����ֵ
            {
    			memcpy((UInt8 *)&pIEC101_Struct->pSendBuf[len3+7+len], (UInt8 *)&data,4);
    			len+=4;
            }
            else if(pIEC101_Struct->yctype == TYPE_GUIYI)//��һֵ
            {
            	pIEC101_Struct->pSendBuf[len1+5]= IEC101_M_ME_NA_1;
    			pIEC101_Struct->pSendBuf[len3+7+len++] = 0xff;
    			pIEC101_Struct->pSendBuf[len3+7+len++] = 0xff;
            }
			pIEC101_Struct->pSendBuf[len3+7+len++] = 0x00;	//Ʒ������
			if(pTmp_Point->next !=NULL)
				pTmp_Point = pTmp_Point->next;
		}
		send_over = send_over +j;
		my_debug("usYC_Num:%d send_over:%d j:%d", sys_mod_p->usYC_Num, send_over, j);
		if(sys_mod_p->Continuity & YC_NOTCONTINUE)   //������
			pIEC101_Struct->pSendBuf[len1+6]  = j;
		else
			pIEC101_Struct->pSendBuf[len1+6]  =IEC104_VSQ_SQ_BIT|j;

    	 pIEC101_Struct->pSendBuf[len3+7+len]=IEC101_CheckSum(4, len3+7+len, IECFRAMESEND);//֡У��
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
//��������:��������ң������
//����:��
//���:��
//�༭:R&N
/**************************************************************************/
static unsigned char IEC101_AP_ALL_YX( short send_num )//��������ң������
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
		ucCommand |=IEC_ACD_BIT;//er���û�������ͣ����Ҫ����һ���û�����
	 pIEC101_Struct->pSendBuf[4]= ucCommand;
	 pIEC101_Struct->pSendBuf[5]= pIEC101_Struct->linkaddr;
	 pIEC101_Struct->pSendBuf[len1+5]=M_SP_NA_1;   //type
	 if(sys_mod_p->Continuity & YC_NOTCONTINUE) 	//��ַ������
		pIEC101_Struct->pSendBuf[len1+6]= 0;        //�ɱ�ṹ���޶���
     else
		pIEC101_Struct->pSendBuf[len1+6]= IEC104_VSQ_SQ_BIT;//�ɱ�ṹ���޶���

	 pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_INTROGEN;//ԭ��
	 pIEC101_Struct->pSendBuf[len1+len2+7]=pIEC101_Struct->linkaddr; //վ��ַ

    pTmp_Point = sys_mod_p->pYX_Addr_List_Head;
     if(send_over != 0)
    {
        for(j=0; j<send_over; j++)
			if(pTmp_Point->next !=NULL)
				pTmp_Point = pTmp_Point->next;
    }
    for(k=0; k<(sys_mod_p->usYX_Num-send_over); )		//���Ͷ��ٴ�
	{
		len = 0;
		if(!(sys_mod_p->Continuity & YX_NOTCONTINUE))   //��ַ����
		{
            for(i=0; i<pIEC101_Struct->infoaddlen ; i++)
            {
                pIEC101_Struct->pSendBuf[len3+7+len] = (unsigned char)((pTmp_Point->uiAddr>>(i*8))&0xff);
                len ++;
            }
		}
		for(j=0; ((j<sys_mod_p->usYX_Num-send_over)&&( len<240)&&( j<(1<<5))); j++)
		{
			if(sys_mod_p->Continuity & YX_NOTCONTINUE) 	//��������ַ
			{
                for(i=0; i<pIEC101_Struct->infoaddlen ; i++)
                {
                    pIEC101_Struct->pSendBuf[len3+7+len] = (unsigned char)((pTmp_Point->uiAddr>>(i*8))&0xff);
                    len ++;
                }
			}
			data =(int)Read_From_Sharearea(pTmp_Point->uiOffset, TYPE_YX_STATUS_BIT);//ң��ֵ
			if(data&(1<<(pTmp_Point->uiOffset&0x7)))
               pIEC101_Struct->pSendBuf[len3+7+len++] = 0x1;            //Ʒ������
            else
               pIEC101_Struct->pSendBuf[len3+7+len++] = 0x0;            //Ʒ������

//			pIEC101_Struct->pSendBuf[len3+7+len++] = 0x00;	//Ʒ������
			if(pTmp_Point->next !=NULL)
				pTmp_Point = pTmp_Point->next;
		}
		send_over = send_over +j;
		my_debug("usYX_Num:%d send_over:%d j:%d", sys_mod_p->usYX_Num, send_over, j);
		if(sys_mod_p->Continuity & YX_NOTCONTINUE)   //������
			pIEC101_Struct->pSendBuf[len1+6]  = j;
		else
			pIEC101_Struct->pSendBuf[len1+6]  =IEC104_VSQ_SQ_BIT|j;

    	 pIEC101_Struct->pSendBuf[len3+7+len]=IEC101_CheckSum(4, len3+7+len, IECFRAMESEND);//֡У��
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
//����:static void IEC101_PositiveCON(int frame_len)
//˵��:ȷ�ϻظ�֡
//����:֡��
//���:��
//�༭:R&N
/**************************************************************************/
static void IEC101_PositiveCON(int frame_len)
{
	char * buf =(char *)  save_recv_data;//���յ�������
	unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;//��ǰ֡���ݵ����λ��
	unsigned char len1=pIEC101_Struct->linkaddrlen;
	unsigned short i=0;

     for(i=0; i<frame_len; i++)
        pIEC101_Struct->pSendBuf[i] = buf[(recvcnt+i)&0x3FF];

     pIEC101_Struct->pSendBuf[4]= IEC_DIR_RTU2MAS_BIT;
     pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_ACTCON;//ԭ��
     pIEC101_Struct->pSendBuf[frame_len-2]=IEC101_CheckSum(4,  frame_len-6, IECFRAMESEND);//֡У��

    write(pIEC101_Struct->fd_usart,  pIEC101_Struct->pSendBuf, frame_len);
	Log_Frame(pIEC101_Struct->logfd,  pIEC101_Struct->pSendBuf,  0, frame_len, SEND_FRAME);
}
/**************************************************************************/
//����:static void IEC101_NegativeCON(int frame_len)
//˵��:ȷ�ϻظ�֡
//����:֡��
//���:��
//�༭:R&N
/**************************************************************************/
static void IEC101_NegativeCON(int frame_len)
{
	char * buf =(char *)  save_recv_data;//���յ�������
	unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;//��ǰ֡���ݵ����λ��
	unsigned char len1=pIEC101_Struct->linkaddrlen;
	unsigned short i=0;

     for(i=0; i<frame_len; i++)
        pIEC101_Struct->pSendBuf[i] = buf[(recvcnt+i)&0x3FF];

     pIEC101_Struct->pSendBuf[4]= IEC_DIR_RTU2MAS_BIT;
     pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_N_BIT | IEC_CAUSE_ACTCON;//ԭ��
     pIEC101_Struct->pSendBuf[frame_len-2]=IEC101_CheckSum(4,  frame_len-6, IECFRAMESEND);//֡У��

    write(pIEC101_Struct->fd_usart,  pIEC101_Struct->pSendBuf, frame_len);
	Log_Frame(pIEC101_Struct->logfd,  pIEC101_Struct->pSendBuf,  0, frame_len, SEND_FRAME);
}
/****************************************************************************/
//����˵��:ң��ѡ�г���
//0:����   1:����    2:����    3:û�������ַ
//YK_BISUO				0
//YK_NORMAL_CONTRL      	1
//YK_FUGUI				2
//YK_INVALID_ADDR		3
//YK_CANCEL_SEL			4
//�༭:R&N1110
//ʱ��:2015.6.1
/****************************************************************************/
static unsigned char IEC101_YK_SelHalt(void)
{
	char * buf =(char *)  save_recv_data;//���յ�������
	unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;//��ǰ֡���ݵ����λ��
	unsigned short sPointNo, i, len3, len4;
	Point_Info   *  Point_Info;

    len3 = pIEC101_Struct->conaddrlen+pIEC101_Struct->cotlen+pIEC101_Struct->linkaddrlen;
    len4 = len3+pIEC101_Struct->infoaddlen;

	if(sys_mod_p->usYK_Num ==0)
		return YK_INVALID_ADDR;

    //sPointNo = (buf[(recvcnt+len3+7)&0x3FF]<<8) | buf[(recvcnt+len3+7)&0x3FF];//���
    sPointNo = buf[(recvcnt+len3+7)&0x3FF];//���
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

	if((buf[(recvcnt+len4+7)&0x3FF]&0x01)==0X01)//�������ң�ط�բѡ��ȡ��  ����ң�غ�բѡ��ȡ��
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
//����:static static void IEC101_YK_Sel( int frame_len )
//˵��:����ң��ѡ��
//����:��
//���:��
//�༭:R&N
/**************************************************************************/
static unsigned char IEC101_YK_Sel( int frame_len )
{
	char * buf =(char *)  save_recv_data;//���յ�������
	unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;//��ǰ֡���ݵ����λ��
    unsigned short sPointNo, j,len3,len4;
	Point_Info   *  Point_Info;
    
    len3 = pIEC101_Struct->conaddrlen+pIEC101_Struct->cotlen+pIEC101_Struct->linkaddrlen;
    len4 = len3+pIEC101_Struct->infoaddlen;
	if(sys_mod_p->usYK_Num ==0)
		return YK_INVALID_ADDR;
//    sPointNo = (buf[(recvcnt+len3+7)&0x3FF]<<8) + buf[(recvcnt+len3+7)&0x3FF];//���
    sPointNo = buf[(recvcnt+len3+7)&0x3FF];//���
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
    
	if((buf[(recvcnt+len4+7)&0x3FF]&0x01)==0X01)//�������ң�ط�բѡ��  ����ң�غ�բѡ��
	{
		Point_Info->usFlag |=(1<<8);
        Send_Event_By_Message(MSG_YK, Point_Info->uiOffset, YK_OPEN_SEL);//���λ��1�Ǳ�ʾѡ�У���0��ʾִ��
    }
	else
    {
		Point_Info->usFlag |=(1<<9);
        Send_Event_By_Message(MSG_YK, Point_Info->uiOffset, YK_CLOSE_SEL);
    }
	return YK_NORMAL_CONTRL;
}

/****************************************************************************/
//����˵��:ң��ִ��
//����:ASDU֡     �ͳ���
//�༭:R&N1110
//ʱ��:2015.6.1
//YK_ZHIXIN_FAIL			5
//YK_ZHIXIN_SUCC		6
/****************************************************************************/
static unsigned char IEC101_AP_YKExec(int frame_len)
{
	char * buf =(char *)  save_recv_data;//���յ�������
	unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;//��ǰ֡���ݵ����λ��
	unsigned short sPointNo, i,len3, len4;
	Point_Info   *  Point_Info;

    len3 = pIEC101_Struct->conaddrlen+pIEC101_Struct->cotlen+pIEC101_Struct->linkaddrlen;
    len4 = len3+pIEC101_Struct->infoaddlen;
	if(sys_mod_p->usYK_Num ==0)
		return YK_INVALID_ADDR;
//    sPointNo = (buf[(recvcnt+len3+7)&0x3FF]<<8) | buf[(recvcnt+len3+7)&0x3FF];//���
    sPointNo = buf[(recvcnt+len3+7)&0x3FF];//���
        
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

	if((buf[(recvcnt+len4+7)&0x3FF]&0x01)==0X01)   //�������ң�ط�բ
	{
		if(Point_Info->usFlag &(1<<8))			           //�Ѿ�ѡ��
		{
				Point_Info->usFlag &=~(1<<8); 		   //��բ����,���ҽ��ѡ��
				//��Ӻ�բ����
				Send_Event_By_Message(MSG_YK, Point_Info->uiOffset, YK_CLOSE_EXECUTIVE);
		}
	}
	else
	{
		if(Point_Info->usFlag &(1<<9))			           //�Ѿ�ѡ��
		{
				Point_Info->usFlag &=~(1<<9); 		   //��բ����,���ҽ��ѡ��
				//��ӷ�բ����
				Send_Event_By_Message(MSG_YK, Point_Info->uiOffset, YK_OPEN_EXECUTIVE);
		}
	}
	if(1)											   //�����ɹ� ?    ִ�е�ʱ������ж�ѡ��û��
		return YK_ZHIXIN_SUCC;
	else
		return YK_ZHIXIN_FAIL;
}

/**************************************************************************/
//����:static void IEC101_Single_YK( void )
//˵��:����ң��
//����:��
//���:��
//�༭:R&N
/**************************************************************************/
static void IEC101_Single_YK( int frame_len )
{
	char * buf =(char *)  save_recv_data;//���յ�������
	unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;//��ǰ֡���ݵ����λ��
	unsigned char len1=0, len2=0, len3=0, len4=0;

    if(pIEC101_Struct->fd_usart <=0)
        return ;

	 len1 = pIEC101_Struct->linkaddrlen;
	 len2 = len1+pIEC101_Struct->cotlen;
	 len3 = len2+pIEC101_Struct->conaddrlen;
     len4 = len3+pIEC101_Struct->infoaddlen;

     if(buf[(recvcnt+len4+7)&0x3FF]&0x80)//ң��ѡ��
     {
        if(buf[(recvcnt+len1+7)&0x3FF] == IEC_CAUSE_ACT) //����
		{
			switch(IEC101_YK_Sel( frame_len))
			{
			case YK_INVALID_ADDR://ң�ر����� ���� ��ַ����
				IEC101_NegativeCON(frame_len);
				break;
			case YK_NORMAL_CONTRL: //һ��ң��ѡ��
				IEC101_PositiveCON(frame_len);
				break;
//    			case 2: //����ѡ��
//    				IEC_PositiveCON(ucPort,pTemp_ASDUFrame , ucTemp_Length);
//    				break;
			}
		}
		else if(buf[(recvcnt+len1+7)&0x3FF] == IEC_CAUSE_DEACT) //ֹͣ����ȷ��
		{
			switch(IEC101_YK_SelHalt())
			{
			case YK_INVALID_ADDR:
				IEC101_NegativeCON(frame_len);
				break;
			case YK_CANCEL_SEL://һ��ң��ѡ����
				IEC101_PositiveCON(frame_len);
				break;
			}
		}
//		else
//            IEC101_NegativeCON(frame_len);
     }
     else//ң��ִ��
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
//����˵��:�ϱ����ٻ�����������
//����:��
//���:��
//�༭:R&N
/**************************************************************************/
static  void  IEC101_Ap_Summonall( void )
{
    static short numyc=0, numyx=0;

	if(sys_mod_p->usYC_Num>=1)
     {
        if(numyc != sys_mod_p->usYC_Num)//û�з������
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
//����˵��:У�麯��
//����:getcnt���ݴ洢�ĵط�    length  ���ٸ�У��   flag  ֡�ṹ
//��� :У����
//�༭:R&N1110			QQ:402097953
//ʱ��:2014.10.22
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
//����˵��:�������ٻ�
//����:��
//���::
//�༭:R&N1110	QQ:402097953
//ʱ��:2014.10.22
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
	 pIEC101_Struct->pSendBuf[len1+6]=0x01;		//�ɱ�ṹ���޶���
	 pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_ACTCON;//ԭ��
	 pIEC101_Struct->pSendBuf[len2+7]=pIEC101_Struct->linkaddr; //վ��ַ

	 pIEC101_Struct->pSendBuf[len3+7]=0;	//��Ϣ���ַ
	 pIEC101_Struct->pSendBuf[len4+7]=IEC_QOI_INTROGEN; //Ʒ������
	 pIEC101_Struct->pSendBuf[len4+8]=IEC101_CheckSum(4,  (len4+8-3), IECFRAMESEND);//֡У��
	 pIEC101_Struct->pSendBuf[len4+9]=0x16;

       nsend = write(pIEC101_Struct->fd_usart,  pIEC101_Struct->pSendBuf, len4+10);
	 if(nsend == (len4+10))
	 	pIEC101_Struct->flag |=SUMMON_CONFIRM_BIT;
	Log_Frame(pIEC101_Struct->logfd,  pIEC101_Struct->pSendBuf,  0, len4+10, SEND_FRAME);
}

/**************************************************************************************/
//����˵��:�鿴�Ƿ����¼��ϱ�
//����:��
//���::
//�༭:R&N1110	QQ:402097953
//ʱ��:2014.10.20
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
//����˵��:������ʼ��
//����:��
//���壺�?
//�༭:R&N1110	QQ402097953
//ʱ��;2014.10.20
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
	 pIEC101_Struct->pSendBuf[len1+5]=M_EI_NA_1;//��ʼ���������
	 pIEC101_Struct->pSendBuf[len1+6]=0x01;		//�ɱ�ṹ���޶���
	 pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_INIT;//ԭ��
	 pIEC101_Struct->pSendBuf[len2+7]=pIEC101_Struct->linkaddr; //վ��ַ

	 pIEC101_Struct->pSendBuf[len3+7]=0;	//��Ϣ���ַ
	 pIEC101_Struct->pSendBuf[len4+7]=IEC_COI_RMRESET; //Ʒ������
	 pIEC101_Struct->pSendBuf[len4+8]=IEC101_CheckSum(4,  (len4+8-3), IECFRAMESEND);//֡У��
	 pIEC101_Struct->pSendBuf[len4+9]=0x16;

       write(pIEC101_Struct->fd_usart,  pIEC101_Struct->pSendBuf, len4+10);
       Log_Frame(pIEC101_Struct->logfd,  pIEC101_Struct->pSendBuf,  0, len4+10, SEND_FRAME);

     pIEC101_Struct->flag &=~ END_INIT_BIT;
}
/**************************************************************************/
//��վ��Ӧ��վ������·״̬����˵��: ����δ�յ���λԶ����·����֮ǰ,
//��������·��������. ������յ���λԶ����·����(��λ��־��1),
//��ʱ������·״̬����ACD��־,��ʾ��һ�����ݲ���,�����帴λ��־,
//�ó�ʼ��������־Ϊ1,Ϊ��һ���ٻ�һ��������׼��. �ڴ�֮�����յ�
//������·״̬����, ��� ������·��������, �ο�101��Լpage39.
/**************************************************************************/
static void IEC101_Rqlink_Res(void)
{
	unsigned char ucCommand;

	if(pIEC101_Struct->flag & RST_LINK_BIT)	//�Ѿ��յ�Զ����·��λ����
	{
		ucCommand =M_RQ_NA_1_LNKOK | IEC_DIR_RTU2MAS_BIT | IEC_ACD_BIT;
		pIEC101_Struct->flag &=~RST_LINK_BIT;
		pIEC101_Struct->flag |= END_INIT_BIT;
	}
	else
	{
		ucCommand =M_RQ_NA_1_LNKOK | IEC_DIR_RTU2MAS_BIT;
		if(IEC101_Chg_Search())
			ucCommand |=IEC_ACD_BIT;	//�������ϴ�
	}
	IEC101_Send10H(ucCommand);
}
/**************************************************************************/
//����˵��:��λԶ����·
//����:��
//���壺�?
//�༭	:R&N1110	QQ:402097953
//ʱ��:2014.10.20
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
//����˵��:����̶�֡�Ĵ�����
//����:֡����
//���::
//�༭:R&N1110	QQ:402097953
//ʱ��:2015.04.23
/**************************************************************************/
static void IEC101_Process_Fixframe( int frame_len)
{
	unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;
	unsigned char control_area=save_recv_data[(recvcnt+1)&0x3FF]& IEC_FUNC_BIT;

	Log_Frame(pIEC101_Struct->logfd,  save_recv_data,  pIEC101_Struct->pBufGet&0x3FF, frame_len, RECV_FRAME);

	switch (control_area)
	{
		case C_RQ_NA_1:              //0x09:�ٻ���·״̬
			IEC101_Rqlink_Res();
			break;
		case C_RL_NA_1:              //0x00:��λԶ����·
			IEC101_Rstlink_Con();
			break;
		case C_P1_NA_1:            	// 0x0a:�ٻ�1���û�����
			if(pIEC101_Struct->flag & END_INIT_BIT)  //��ʼ��������־,��ʾ�г�ʼ������һ������
			{
				IEC101_EndInit();
				break;
			}
            /*
			if(IEC101_Chg_Search()==MSG_COS)   //����ң�ű�λ
			{
			    IEC101_Process_COS();
				break;
			}
			if(IEC101_Chg_Search()==MSG_SOE)   //����ң�ű�λ
			{
			    IEC101_Process_SOE();
				break;
			}*/
//			if(pIEC_Struct[ucPort]->ucYK_Exec_Result) //ң�ؽ��
//			{
//				IEC_YKExecSend(ucPort,pIEC_Struct[ucPort]->ucYK_Exec_Result);
//				pIEC_Struct[ucPort]->ucYK_Exec_Result = 0;
//				break;
//			}
			IEC101_SendE5H(); 			    //����û�����ݿ��Է���
			break;
  	  	case C_P2_NA_1: 					//0x0b   �ٻ�2���û�����

			if(SUMMON_BIT&pIEC101_Struct->flag)
			{
				if(pIEC101_Struct->flag &SUMMON_CONFIRM_BIT)  //�����Ѿ�ȷ��
					IEC101_Ap_Summonall();
                    if(IEC101_Chg_Search()==MSG_COS)   //����ң�ű�λ
                    			{
                    			    IEC101_Process_COS();
                    				break;
                    			}
                    			if(IEC101_Chg_Search()==MSG_SOE)   //����ң�ű�λ
                    			{
                    			    IEC101_Process_SOE();
                    				break;
                    			}
				else											//����û��ȷ��
					IEC101_Rqall_Con();
			}
//����ΪABB��ʽ,��֧��ƽ�ⷽʽ���䣬����ң�����ֵ�ķ�֡Ӧ�����ٻ�
//�������ݵ�ʱ���ѯ�Ƿ���ڲ����͡����ٻ��ͷ����ٻ�������ˡ�
//				if(pIEC_Struct[ucPort]->ucYK_Exec_Result) //ң�ؽ��
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
//����:static unsigned char IEC101_Process_COS( void ) //Ӧ�ò㴦��
//˵��:������ң�ű�λ
//����:
//���:
//�༭:R&N1110@126.com
//ʱ��:2015.05.9
/***********************************************************************/
static unsigned char IEC101_Process_COS( void ) //Ӧ�ò㴦��
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
     pIEC101_Struct->pSendBuf[len1+6]= 0;          //�ɱ�ṹ���޶���
	 pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_SPONT;//ԭ��
	 pIEC101_Struct->pSendBuf[len1+len2+7]=pIEC101_Struct->linkaddr; //վ��ַ

    len = 0;
    while(pYXInfoGet->ucStatus == FULL)
    {
        //���������ҵ���Ӧ��ң����
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
      		    pIEC101_Struct->pSendBuf[len3+7+len++] = 0x1;            //Ʒ������
      		else
                pIEC101_Struct->pSendBuf[len3+7+len++] = 0x0;            //Ʒ������
    		pIEC101_Struct->pSendBuf[len1+6]=pIEC101_Struct->pSendBuf[len1+6]+1 ;                         //����
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
    	 pIEC101_Struct->pSendBuf[len3+7+len]=IEC101_CheckSum(4, len3+7+len, IECFRAMESEND);//֡У��
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
//����:static unsigned char IEC101_Process_SOE( void ) //Ӧ�ò㴦��
//˵��:������ң�ű�λ
//����:��
//���:
//�༭:R&N1110@126.com
//ʱ��:2015.05.9
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
     pIEC101_Struct->pSendBuf[len1+6]= 0;          //�ɱ�ṹ���޶���
	 pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_SPONT;//ԭ��
	 pIEC101_Struct->pSendBuf[len1+len2+7]=pIEC101_Struct->linkaddr; //վ��ַ

    len = 0;
    while(pSOEGet->ucStatus == FULL)
    {
        //���������ҵ���Ӧ��ң����
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
      		    pIEC101_Struct->pSendBuf[len3+7+len++] = 0x1;            //Ʒ��������������
      		else
                pIEC101_Struct->pSendBuf[len3+7+len++] = 0x0;            //Ʒ������

            pIEC101_Struct->pSendBuf[len3+7+len++] = (unsigned char)( pSOEGet->usMSec & 0xff);
            pIEC101_Struct->pSendBuf[len3+7+len++] = (unsigned char)(pSOEGet->usMSec >> 8);
            pIEC101_Struct->pSendBuf[len3+7+len++]=   pSOEGet->ucMin;
            pIEC101_Struct->pSendBuf[len3+7+len++]=   pSOEGet->ucHour;
            pIEC101_Struct->pSendBuf[len3+7+len++] =  pSOEGet->ucDay;
            pIEC101_Struct->pSendBuf[len3+7+len++]=   (pSOEGet->ucMonth+1);
            pIEC101_Struct->pSendBuf[len3+7+len++] =  pSOEGet->ucYear-100;

    		pIEC101_Struct->pSendBuf[len1+6]=pIEC101_Struct->pSendBuf[len1+6]+1 ;                         //����
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
    	 pIEC101_Struct->pSendBuf[len3+7+len]=IEC101_CheckSum(4, len3+7+len, IECFRAMESEND);//֡У��
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
//����˵��: ������վʱ��
//����:֡����
//���::
//�༭:R&N1110	QQ:402097953
//ʱ��:2014.10.20
/**************************************************************************/
static void IEC101_SendTime(int length)
{
	unsigned short  usMSec;
	long  usUSec;				//΢�뾫��
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
	usUSec = usMSec%1000; //�����ms
	usUSec = usUSec *1000; //�����us
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
	Set_Rtc_Time(stime,  usUSec);	//����RTCʱ��


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
	 pIEC101_Struct->pSendBuf[len1+6]=0x01;		//�ɱ�ṹ���޶���
	 pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_ACTCON;//ԭ��
	 pIEC101_Struct->pSendBuf[len2+7]=pIEC101_Struct->linkaddr; //վ��ַ
	 pIEC101_Struct->pSendBuf[len3+7]=0;	//��Ϣ���ַ
	 pIEC101_Struct->pSendBuf[len4+7]=buf[(recvcnt+length-9)&0x3FF]; //��Ϣ����
	 pIEC101_Struct->pSendBuf[len4+8]=buf[(recvcnt+length-8)&0x3FF]; //��Ϣ����
	 pIEC101_Struct->pSendBuf[len4+9]=stime->tm_min; //��Ϣ����
	 pIEC101_Struct->pSendBuf[len4+10]=stime->tm_hour; //��Ϣ����
	 pIEC101_Struct->pSendBuf[len4+11]=stime->tm_mday; //��Ϣ����
	 pIEC101_Struct->pSendBuf[len4+12]=stime->tm_mon; //��Ϣ����
	 pIEC101_Struct->pSendBuf[len4+13]=stime->tm_year; //��Ϣ����
	 //pIEC101_Struct->pSendBuf[len4+14]=IEC101_CheckSum(4, length-2, IECFRAMESEND);//֡У��
	 	 pIEC101_Struct->pSendBuf[len4+14]=IEC101_CheckSum(4, (len4+13-2), IECFRAMESEND);//֡У��

	 pIEC101_Struct->pSendBuf[len4+15]=0x16;
        write(pIEC101_Struct->fd_usart,  pIEC101_Struct->pSendBuf, len4+16);
	free(stime);
	Log_Frame(pIEC101_Struct->logfd,  pIEC101_Struct->pSendBuf,  0, len4+16, SEND_FRAME);
}

/**************************************************************************/
//����˵��:����ɱ�֡�Ĵ�����
//����:֡����
//���::
//�༭:R&N1110	QQ:402097953
//ʱ��:2014.10.20
/**************************************************************************/
static void IEC101_Process_Varframe( int frame_len)
{
	char * buf =(char *)  save_recv_data;
	unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;
	unsigned char control_area=buf[(recvcnt+4)&0x3FF]& IEC_FUNC_BIT;
	unsigned char type= buf[(recvcnt+ pIEC101_Struct->linkaddrlen+5)&0x3FF];

 	if(buf[recvcnt+5] !=pIEC101_Struct->linkaddr)				//��ַ����
		return;

	Log_Frame(pIEC101_Struct->logfd,  save_recv_data,  recvcnt, frame_len, RECV_FRAME);

	switch (control_area)
	{
	 case 0x03:
		switch(type)
		{
			case C_SC_NA_1:	//0x2d ����ң��
			case C_DC_NA_1:   //0x2e ˫��ң��
				IEC101_Single_YK(frame_len);
				break;
			case C_CS_NA_1:   //0x67-��ʱ
				IEC101_SendTime(frame_len);
				break;
			case C_IC_NA_1:      //0x64�����ٻ�
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
//����˵��:�����Ƿ��ǹ�Լ101��֡
//����:int fd_usart , unsigned short * buf_len(�����֡�ĳ���)
//���::
//		IECFRAMEERR֡����
//		IECFRAMEVAR�ɱ�֡
//		IECFRAMEFIX�̶�֡
//�༭:R&N1110		QQ:402097953
//ʱ��?:2014.10.22
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
			sem_wait(&sem_iec101);//�ź���value��1
			while(pIEC101_Struct->usRecvCont < (len+6))//������յ������ݲ������͵ȴ�
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

			*buf_len = len+6; 		//�ܳ���
			return IECFRAMEVAR; 		//�ɱ�֡
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
//����˵��:101��Լ�����
//����:��
//���:��
//�༭:R&N
/**************************************************************************/
static void IEC101_Protocol_Entry(  void  )
{
	int  frame_len;
	unsigned char   return_flag;

	if(pIEC101_Struct->usRecvCont>0)	//������
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
//����˵��:������Ҫ���͵����ݣ�����SOE�ϱ��¼���
//����:��
//���:��
//�༭:R&N
/**************************************************************************/
static void IEC101_UnsoSend( void )
{
    
}
/***************************************************************************/
//����: void *Iec101_Thread(void * arg)
//˵��:���ڴ���Ӵ��ڽ��յ�������
//����:
//���:
//�༭:R&N1110@126.com
//ʱ��:2015.4.21
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
					IEC101_Protocol_Entry();		//������յ�������
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
//����:  int  IEC101_Free_Buf(void)
//˵��:�ͷŵ�����
//����:��
//���:��
//�༭:R&N1110
//ʱ��:2015.04.21
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

	sem_destroy(&sem_iec101);//�����ź���
	sem_destroy(&sem_iec101_recv);

	return 0;
}


/***************************************************************************/
//����: void IEC101_Init_Buf(void)
//˵��:��ʼ�����壬�����յ����ݱ���Ļ��壬�������ӳ�����
//����:��
//���:��
//�༭:R&N1110@126.com
//ʱ��:2015.4.23
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

 //����ѭ��������������ң�ű�λ
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
//����ѭ��������������SOE�¼�
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
//����˵��:��ȡ101��Լ�еĴ�������
//����:line�����ļ��е�ÿһ��
//		  struct _IEC101Struct   * iec_parpȫ�ֱ���pIEC101_Struct
//���:��
//�༭:R&N1110		qq:402097953
//ʱ��:2014.10.20
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
//����: static void IEC101_Read_Conf(char * fd,  struct _IEC104Struct   * iec104)
//˵��:��ȡ���
//����:��
//���:��
//�༭:R&N1110@126.com
//ʱ��:2015.4.23
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
	 fseek(fp, 0, SEEK_SET); 			//���¶��嵽��ʼλ��
	 while (fgets(line, 31, fp))
 	{
 		Get_Dianbiao_From_Line(line,  iec101);
 	}
	 fclose(fp);
}

/**************************************************************************/
//����˵��:101��Լ�ĳ�ʼ��
//����:��
//���:��
//�༭:R&N
//ʱ��:2015.04.23
/**************************************************************************/
static  void IEC101_InitAllFlag( void )
{
	pIEC101_Struct->usRecvCont = 0;
	pIEC101_Struct->pBufFill = 0;
	pIEC101_Struct->pBufGet = 0;
    pIEC101_Struct->flag = 0;
}

/***************************************************************************/
//����:void IEC101_Process_Message(msg->cmd, msg->buf, msg->data)
//˵��:����MESSAGE�ĺ���
//����:message�������buf
//���:
//�༭:R&N1110@126.com
//ʱ��:2015.05.09
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
                if(buf&(1<<i))//��ʾ��(cmd>>16)*32+iͨ����λ
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
                if(buf&(1<<i))//��ʾ��(cmd>>16)*32+iͨ����λ
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
//Function:   ���ò�����
//Input:      ��Return:     FALSE TRUE
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
//Name:       set_parityFunction:   ���ô�������λ��ֹͣλ��Ч��λ
//Input:      1.fd          �򿪵Ĵ����ļ����
//		 2.databits    ����λ  ȡֵ  Ϊ 7  ���� 8
//		 3.stopbits    ֹͣλ  ȡֵΪ 1 ����2
//		 4.parity      Ч������  ȡֵΪ N,E,O,S
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

	/* ��������λ��*/
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

    /*  ����ֹͣλ*/
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
	options.c_cc[VTIME] = 150;							// ���ó�ʱ 15 seconds
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
//Function:   �򿪴����豸
//Input:      ��Return:     FALSE TRUE
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
//����: int  Uart_Thread( void )
//˵��:���ڽ����̣߳�����Ϊiec101����
//����:��
//���:��
//�༭:R&N1110@126.com
//ʱ��:2015.4.23
/***************************************************************************/
int  Uart_Thread( void )
{
	int nrecvdata = 0 , i;
	char  buf_rec[20];

	sem_init(&sem_iec101, 0, 0);//����һ��δ�������ź���
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
	   if(pIEC101_Struct->fd_usart >0)//�˿�����
	   	{
			if(pIEC101_Struct->pBufFill<=1003)
			{
				nrecvdata = read(pIEC101_Struct->fd_usart,  save_recv_data+pIEC101_Struct->pBufFill ,  20);

				pIEC101_Struct->pBufFill = pIEC101_Struct->pBufFill + nrecvdata;
				pIEC101_Struct->usRecvCont = pIEC101_Struct->usRecvCont + nrecvdata;
				if(nrecvdata >0)
				{
//				    my_debug("sem_post\n");
					sem_post(&sem_iec101);//�ź���ֵ��1
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
//����:void IEC101_Senddata2pc(int fd, char *tmp_buf, UInt16 len)
//˵��:���ڷ��ͼ�����ݵ�PC
//����:fd�����ж���iec101����iec104  tmp_buf��Ҫ���͵�����  len��Ҫ���͵����ݳ���
//���:��
//�༭:R&N
/**************************************************************************/
void IEC101_Senddata2pc(int fd, char *tmp_buf, UInt8 len)
{
    if(fd == pIEC101_Struct->logfd)
    {
        if(sys_mod_p->pc_flag &(1<<MSG_GET_IEC101))
            PC_Send_Data(MSG_GET_IEC101, (UInt8 *)tmp_buf, len);
    }
}
