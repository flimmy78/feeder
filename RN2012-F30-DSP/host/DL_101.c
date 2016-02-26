/******************************************************************************
* File Name          : DL_101.c
* Author             : 
* Version            : V1.0
* Description        : 
* Date               : 2015/10/19
* ����޸����ڣ�
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

//��������
void QLY_Nb101_total_call_reset(void);//��ƽ��101���ٻ�״̬��λ����
//static UInt8 QLY_BL101_start_state(void);//ƽ��101��·��ʼ�жϺ���
static void QLY_BL101_state_Reset(void);//ƽ��101��λ����
static  void  BL101_YX_Summonall( UInt8 code );
static LOCAL_Module         * sys_mod_p;
static unsigned char BL101_AP_ALL_YX( short send_num ,UInt8 control_code);//��������ң������
static unsigned char       	save_recv_data[IEC101BUFNUM * IEC101BUFLEN];
static short BL101_AP_ALL_YC( short send_num ,UInt8 control_code);//��������ң������
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
//��������
static enum 
{
    Bl101_NoneCall_Type,
    Bl101_TotaCall_Type,
    Bl101_OneGroupCall_Type,
    Bl101_TwoGroupCall_Type,
    Bl101_NineGroupCall_Type,
    Bl101_TenGroupCall_Type
}Bl101_Call_Type = Bl101_NoneCall_Type;//�����ٻ�����
//static Spont_Trans_type *Sudden_fault; //ָ�����е�ͻ���¼�
static UInt16 T1_time;//��¼��ʱ����T1
//static UInt8 GPRS_Reset_Mark = 0;//GPRSģ��������־������ң������GPRS����������
//static UInt8 *Tele_Regulation_Frame;//��ʱָ�����е�ң��֡
struct _QLY_control_101{
	Qconbit SFCB;//����֡����λ��ÿ֡��λ��ƽ��ʽ101ʹ��
	Qconbit RFCB;//֡����λ��ÿ֡��λ
	UInt8 RFCB_cnt;//֡�����ۼӣ�����3��λ
	Qconbit ACD;//��վҪ����1������λ
	Qconbit DFC;//��վ�Ƿ���Խ��պ������ġ�0:yes  1:busy
	
	Qconbit Wait_Answer_mask;//�ȴ���Ӧ��־�����������˱�־λ��1������ʱʱ�䵽��ط����һ֡������
	UInt8 Wait101Frame_Times;//��ʱ�ȴ�ʱ��
	UInt8 Resend_Frame_cnt;//�ط���������
	UInt8 Send101Frame_Interval;//���ͼ��
};
struct _QLY_control_101 Q_control_101_data;

struct _QLY_control_101 *  Q_control_101 = &Q_control_101_data;


/******************************************************/
//�������ܣ���λ��·
//����ֵ����ȷ����0�����󷵻�1
//˵������101ͨ�ŵ�����״̬��ʼ������������վҪ��λ���߳�����·����ʱ����
//�޸����ڣ�2015/10/16
/******************************************************/
UInt8 QLY_101_link_Init(void)
{
    Q_control_101->SFCB= 0;
	Q_control_101->RFCB = 0;
	Q_control_101->RFCB_cnt = 0;
	Q_control_101->DFC = 0;
	QLY_BL101_state_Reset();//ƽ��101״̬��λ
	return 0;
}
/*******************************************************���ķ��ͺ���*******************************************************************************/
//	1�����ͺ���:	UInt8 QLY_101_FrameSend(UInt8* p_buff) 
//	2�����ͼ����	UInt8 QLY_101_FrameInterval(void)
//
/******************************************************/
//�������ܣ�101���ĳ��ȼ���
//����ֵ����ȷ���س��ȣ����󷵻�0
//�����������͵ı���ָ��p_buff
//˵�������ڻظ���վ�ٻ���·״̬
//�޸����ڣ�2015/11/10
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
//�������ܣ����ļ������
//����ֵ����ȷ����0�����󷵻�1
//������
//˵����������֡����֮��ļ���������վ��Ҫ��ʵ�ָú���
//�޸����ڣ�2015/11/05
/******************************************************/
/*
static UInt8 QLY_101_FrameInterval(void)
{
	//OSTimeDly(Global_Variable.SendFrame_Interval);
	return 0;
}*/
///********************************************************��·�ظ��ӳ���****************************************************************************/
	
//ƽ��
//		1���̶�֡�����ͺ�����	UInt8 QLY_BL101_FixFrame_Send(UInt8 Bl101_Function_Code,Qconbit PRM,Qconbit FCV,UInt8* p_buff)
//		2���ɱ�֡�����ͺ����� UInt8 QLY_BL101_VarFrame_Send(UInt8 Bl101_Function_Code,UInt8 Type_Identification,UInt8 Trans_Cause)
//		3��ƽ��101����ͻ��֡: UInt8 QLY_BL101_SpontTransData(Spont_Trans_type *Sudden_fault,UInt8 Type_Identification)
//����

/******************************************************/
//�������ܣ�����һ������
//����ֵ����ȷ����0�����󷵻�1
//���������ظ��ı���ָ�룬ACD,CAUSEԭ��
//˵���������ϴ����ݣ�����Ϊ�������󣬻������ٻ�
//�޸����ڣ�2015/10/19
/******************************************************/
UInt8 QLY_101send_one_level_Data(UInt8* p_buff,Qconbit acd,UInt8 cause)
{
	if(Balance101_Mask)
		;
	else
	Q_control_101->ACD = low;//ACD����
	return 0;
}
/******************************************************/
//�������ܣ����Ͷ�������
//����ֵ����ȷ����0�����󷵻�1
//���������ظ��ı���ָ�룬ACD,CAUSE
//˵���������ϴ��������ݣ�����Ϊ�������󣬻������ٻ�
//�޸����ڣ�2015/10/19
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
//�������ܣ�ƽ��101���͹̶�֡
//����ֵ����ȷ����0�����󷵻�1
//������������Bl101_Function_Code��PRM����FCB�Ƿ���Ч���������ݵ�ָ��p_buff
//˵�������ڷ��͹̶�֡��PRMΪ1Ϊ��������0Ϊ�Ӷ�����FCVΪ1ΪFCB��Ч��0Ϊ��Ч��p_buffΪ���ͻ�������ָ��
//�޸����ڣ�2015/11/04
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
	//���ͱ���
	return(QLY_101_FrameSend(p_buff));
    //write(pIEC101_Structd->fd_usart,p_temp,QLY_101FrameLen(p_buff));
}
/**************************************************************************/
//����˵��:�ϱ����ٻ�����������
//����:��
//���:��
//�༭:R&N
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
        if(numyc != sys_mod_p->usYC_Num)//û�з������
        {
            numyc = BL101_AP_ALL_YC(numyc,codes);
            return;
        }
     }
    numyc = 0;
}

/******************************************************/
//�������ܣ�ƽ��101���ͱ䳤֡
//����ֵ����ȷ����0�����󷵻�1
//������������Bl101_Function_Code�����ͱ�ʾType_Identification������ԭ��Trans_Cause���������ݵ�ָ��p_buff
//˵�������ڷ��ͱ䳤֡��ͨ�����ù����룬���ͱ�ʾ������ԭ�򣬽�p_buffָ��Ҫ���͵�����
//�޸����ڣ�2015/11/04
/******************************************************/
static UInt8 QLY_BL101_VarFrame_Send(UInt8 Bl101_Function_Code,UInt8 Type_Identification,UInt8 Trans_Cause,UInt8* p_buff,UInt8* p_buffer)
{
//	APP_Grade_Parameter_Type Boot_Struct;//����������Ϣ�ṹ��
	UInt8* p_temp;
//    int data=0;
//    Point_Info   * pTmp_Point=NULL;
	UInt8 APDU_len,i,sum_value=0;
	UInt8 control_code = Bl101_Function_Code | 0x50;//FCVλΪ1��PRMΪ1
//	UInt8 SQ,Inf_Cnt,Tele_Regulation_Shift;//����ƫ�Ƶ�ַ
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
	switch(Type_Identification)//�������ͱ�ʾ��䲻ͬ��ASDU����
	{
		case M_EI_NA_1://��ʼ������
			*p_temp++ = control_code;
			*p_temp++ = pIEC101_Struct->linkaddr;
			*p_temp++ = M_EI_NA_1;
			*p_temp++ = 0x01;//�ɱ�ṹ�޶���
			*p_temp++ = Trans_Cause;
            *p_temp++ = Trans_Cause>>8;
			*p_temp++ = ASDU_Address;
			*p_temp++ = ASDU_Address>>8;
			*p_temp++ = 00;
			*p_temp++ = 00;
			*p_temp++ = 00;
			*p_temp++ = 00;
			APDU_len = 3+9;//��Ϣ����� +�̶�����
			break;
            
		case C_IC_NA_1://��Ӧ���ٻ�
			*p_temp++ = control_code;
			*p_temp++ = pIEC101_Struct->linkaddr;
			*p_temp++ = C_IC_NA_1;
			*p_temp++ = 0x01;//�ɱ�ṹ�޶���
			*p_temp++ = Trans_Cause;
            *p_temp++ = Trans_Cause>>8;
			*p_temp++ = ASDU_Address;
			*p_temp++ = ASDU_Address>>8;
			*p_temp++ = 0x00;
			*p_temp++ = 0x00;
			*p_temp++ = 0x00;
			switch(Bl101_Call_Type)
			{
				case Bl101_TotaCall_Type://���ٻ���Ϣ��
					*p_temp++ = 0x14;
					break;
				default:
					*p_temp++ =0x00;//�������������
					break;
			}
			APDU_len = 8+3+1;//��Ϣ����� +�̶�����
			break;
         
		case C_CD_NA_1://��ʱ���
			*p_temp++ = control_code;
			*p_temp++ = pIEC101_Struct->linkaddr;
			*p_temp++ = C_CD_NA_1;
			*p_temp++ = 0x01;//�ɱ�ṹ�޶���
			*p_temp++ = Trans_Cause;
            *p_temp++ = Trans_Cause>>8;
			*p_temp++ = ASDU_Address;
			*p_temp++ = ASDU_Address>>8;
			*p_temp++ = 0x00;
			*p_temp++ = 0x00;
            *p_temp++ = 0x00;
			*p_temp++ = (UInt8)((T1_time) & 0xff);//��T3time������Ϣ��
			*p_temp++ |= (UInt8)((T1_time)>>8);
			APDU_len = 8+3+2;//��Ϣ����� +�̶�����
			break;
		default:
			return 1;
	}
	p_buff[1] = p_buff[2] = APDU_len;
	for(i=APDU_len;i>0;i--)//����У���
	{
		sum_value += p_buff[3+i];
	}
	*p_temp++ = sum_value;
	*p_temp = Normal_frame_end;
	//���ͱ���
	return(QLY_101_FrameSend(p_buff));
}
/**************************************************************************/
//����˵��: ������վʱ��
//����:֡����
//���::
//�༭:R&N1110	
//ʱ��:2014.10.20
/**************************************************************************/
static void BL101_SendTime(int length,UInt8 * p_buffs,UInt8 control_code)
{
	unsigned short  usMSec;
	long  usUSec;				//΢�뾫��
	unsigned char  ucSec;
	unsigned char len1=0, len2=0, len3=0, len4=0;

    unsigned short recvcnt = 0;
	struct rtc_time  * stime=malloc(sizeof(struct rtc_time));
    UInt8 * buf;
    buf =  p_buffs;//���յ�������
    UInt8 control_codes = control_code;
    
	 len1 = pIEC101_Struct->linkaddrlen;
	 len2 = len1 + pIEC101_Struct->cotlen;
	 len3 = len2 + pIEC101_Struct->conaddrlen;
	 len4 = len3  +pIEC101_Struct->infoaddlen;

	usMSec = (buf[recvcnt+length-8]<<8)|buf[recvcnt+length-9];

	ucSec = (unsigned char)(usMSec/1000);
	if(ucSec>59)
		return;
	usUSec = usMSec%1000; //�����ms
	usUSec = usUSec *1000; //�����us
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
	Set_Rtc_Time(stime,  usUSec);	//����RTCʱ��


	memset(pIEC101_Struct->pSendBuf, 0, length);
	 pIEC101_Struct->pSendBuf[0]= 0x68;
	 pIEC101_Struct->pSendBuf[1]= length-6;
	 pIEC101_Struct->pSendBuf[2]= pIEC101_Struct->pSendBuf[1];
	 pIEC101_Struct->pSendBuf[3]= 0x68;

	 pIEC101_Struct->pSendBuf[4]= control_codes;
	 pIEC101_Struct->pSendBuf[5]= pIEC101_Struct->linkaddr;
	 pIEC101_Struct->pSendBuf[len1+5]=C_CS_NA_1;
	 pIEC101_Struct->pSendBuf[len1+6]=0x01;		//�ɱ�ṹ���޶���
	 pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_ACTCON;//ԭ��
	 pIEC101_Struct->pSendBuf[len2+7]=pIEC101_Struct->linkaddr; //վ��ַ
	 pIEC101_Struct->pSendBuf[len3+7]=0;	//��Ϣ���ַ
	 pIEC101_Struct->pSendBuf[len4+7]=buf[recvcnt+length-9]; //��Ϣ����
	 pIEC101_Struct->pSendBuf[len4+8]=buf[recvcnt+length-8]; //��Ϣ����
	 pIEC101_Struct->pSendBuf[len4+9]=stime->tm_min; //��Ϣ����
	 pIEC101_Struct->pSendBuf[len4+10]=stime->tm_hour; //��Ϣ����
	 pIEC101_Struct->pSendBuf[len4+11]=stime->tm_mday; //��Ϣ����
	 pIEC101_Struct->pSendBuf[len4+12]=stime->tm_mon; //��Ϣ����
	 pIEC101_Struct->pSendBuf[len4+13]=stime->tm_year; //��Ϣ����
	 //pIEC101_Struct->pSendBuf[len4+14]=IEC101_CheckSum(4, length-2, IECFRAMESEND);//֡У��
	 	 pIEC101_Struct->pSendBuf[len4+14]=BL101_CheckSum(4, (len4+13-2), IECFRAMESEND);//֡У��

	 pIEC101_Struct->pSendBuf[len4+15]=0x16;
        write(pIEC101_Struct->fd_usart,  pIEC101_Struct->pSendBuf, len4+16);
	free(stime);
	Log_Frame(pIEC101_Struct->logfd,  pIEC101_Struct->pSendBuf,  0, len4+16, SEND_FRAME);
}

/**************************************************************************/
//��������:��������ң������
//����:��
//���:��
//�༭:R&N
/**************************************************************************/
static unsigned char BL101_AP_ALL_YX( short send_num ,UInt8 control_code)//��������ң������
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
	 
	 if(sys_mod_p->Continuity & YC_NOTCONTINUE) 	//��ַ������
		pIEC101_Struct->pSendBuf[len1+6]= 0;        //�ɱ�ṹ���޶���
     else
		pIEC101_Struct->pSendBuf[len1+6]= IEC104_VSQ_SQ_BIT;//�ɱ�ṹ���޶���

	 pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_INTROGEN;//ԭ��
	 pIEC101_Struct->pSendBuf[len1+len2+6]=(pIEC101_Struct->linkaddr); //վ��ַ

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

    	 pIEC101_Struct->pSendBuf[len3+7+len]=BL101_CheckSum(4, len3+7+len, IECFRAMESEND);//֡У��
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
//����:static short IEC101_AP_ALL_YC( short send_num )
//����:��������ң������,���͸�����
//����:�Ѿ�������ң��ĸ���
//���:�Ѿ�������ң��ĸ���
//�༭:R&N
//ʱ��:2015.5.27
/**************************************************************************/
static short BL101_AP_ALL_YC( short send_num ,UInt8 control_code)//����ȫ��ң������
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
	 if(sys_mod_p->Continuity & YC_NOTCONTINUE) 	//��ַ������
		pIEC101_Struct->pSendBuf[len1+6]= 0;        //�ɱ�ṹ���޶���
     else
		pIEC101_Struct->pSendBuf[len1+6]= IEC104_VSQ_SQ_BIT;//�ɱ�ṹ���޶���

	 pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_INTROGEN;//ԭ��
	 pIEC101_Struct->pSendBuf[len1+len2+6]=pIEC101_Struct->linkaddr; //վ��ַ

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

    	 pIEC101_Struct->pSendBuf[len3+7+len]=BL101_CheckSum(4, len3+7+len, IECFRAMESEND);//֡У��
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
//����˵��:У�麯��
//����:getcnt���ݴ洢�ĵط�    length  ���ٸ�У��   flag  ֡�ṹ
//��� :У����
//�༭:R&N1110			
//ʱ��:2014.10.22
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
//����:static void IEC101_NegativeCON(int frame_len)
//˵��:ȷ�ϻظ�֡
//����:֡��
//���:��
//�༭:R&N
/**************************************************************************/
static void BL101_NegativeCON(int frame_len,UInt8 * p_buffs,UInt8 code)
{
	//char * buf =(char *)  save_recv_data;//���յ�������
	
    UInt8 control_code=code;
	UInt8 * buf;
    buf =  p_buffs;//���յ�������
	//unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;//��ǰ֡���ݵ����λ��
	unsigned short recvcnt =0;
	unsigned char len1=pIEC101_Struct->linkaddrlen;
	unsigned short i=0;

     for(i=0; i<frame_len; i++)
        pIEC101_Struct->pSendBuf[i] = buf[recvcnt+i];
     pIEC101_Struct->pSendBuf[4]= control_code;
     //pIEC101_Struct->pSendBuf[4]= IEC_DIR_RTU2MAS_BIT;
     //pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_N_BIT | IEC_CAUSE_ACTCON;//ԭ��
     pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_ACTCON;//ԭ��
     pIEC101_Struct->pSendBuf[frame_len-2]=BL101_CheckSum(4,  frame_len-6, IECFRAMESEND);//֡У��
    
    write(pIEC101_Struct->fd_usart,  pIEC101_Struct->pSendBuf, frame_len);
	Log_Frame(pIEC101_Struct->logfd,  pIEC101_Struct->pSendBuf,  0, frame_len, SEND_FRAME);
    my_debug("BL101_NegativeCON!");
}
/**************************************************************************/
//����:static void IEC101_PositiveCON(int frame_len)
//˵��:ȷ�ϻظ�֡
//����:֡��
//���:��
//�༭:R&N
/**************************************************************************/
static void BL101_PositiveCON(int frame_len,UInt8 * p_buffs,UInt8 code)
{
	//char * buf =(char *)  save_recv_data;//���յ�������
	
    UInt8 control_code=code;
	UInt8 * buf;
    buf =  p_buffs;//���յ�������
	//unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;//��ǰ֡���ݵ����λ��
	unsigned short recvcnt =0;
	unsigned char len1=pIEC101_Struct->linkaddrlen;
	unsigned short i=0;

     for(i=0; i<frame_len; i++)
        pIEC101_Struct->pSendBuf[i] = buf[recvcnt+i];

     //pIEC101_Struct->pSendBuf[4]= IEC_DIR_RTU2MAS_BIT;
     pIEC101_Struct->pSendBuf[4]= control_code;
     pIEC101_Struct->pSendBuf[len1+7]= IEC_CAUSE_ACTCON;//ԭ��
     pIEC101_Struct->pSendBuf[frame_len-2]=BL101_CheckSum(4,  frame_len-6, IECFRAMESEND);//֡У��

    write(pIEC101_Struct->fd_usart,  pIEC101_Struct->pSendBuf, frame_len);
	Log_Frame(pIEC101_Struct->logfd,  pIEC101_Struct->pSendBuf,  0, frame_len, SEND_FRAME);
    
    my_debug("BL101_PositiveCON!");
}

/******************************************************/
//�������ܣ�ƽ��101����ͻ��֡
//������p_buff1ָʾ�����ݣ�Time_Mask�Ƿ��ʱ�꣺high����low������Type_Identification���ͱ�ʾ
//����ֵ����ȷ����0�����󷵻�1
//�޸����ڣ�2015/10/25
/******************************************************/

//static UInt8 QLY_BL101_SpontTransData(Spont_Trans_type *Sudden_fault,UInt8 Type_Identification)
static UInt8 QLY_BL101_SpontTransData(UInt8 Type_Identification)
{
//	UInt8 *p_temp = QLY_Last_101_Request_Frame;
//	UInt8 *p_buff = QLY_Last_101_Request_Frame;
	UInt8 control_code = Trans_data_code | 0x50;//FCVλΪ1��PRMΪ1;
	
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

/************************************************************101��Լ������֤����******************************************************************************/
//����:
//			1��У��ͳ���
//			2��FCBλ�ж�
/******************************************************/
//�������ܣ��̶���֡У��
//����ֵ����ȷ����0�����󷵻�1
//�޸����ڣ�2015/10/16
/******************************************************/
UInt8 QLY_Fix101frame_check(UInt8* p_buff)
{
	UInt8 CRC_value;
	CRC_value = (*(p_buff+2)) + (*(p_buff+1));
	if(CRC_value == (*(p_buff+3)))//�����·��ַΪ���ֽڣ�ƫ��3��˫�ֽ�ƫ��4
		return 0;
	else
	{
		my_debug("Fix101Frame Sum_value erro!");
		return 1;
	}
}
/******************************************************/
//�������ܣ��ɱ䳤֡У��
//����ֵ����ȷ����0�����󷵻�1,�������Ȳ�һ�·���2
//�޸����ڣ�2015/10/16
/******************************************************/
UInt8 QLY_Var101frame_check(UInt8* p_buff)
{
	UInt8 CRC_value=0,buff_len;
	UInt8* p_temp = p_buff+4;//�ɱ䳤ASDU���ݴӵ�����ֽڿ�ʼ
	//���γ��ȱ���һ��
	if(*(p_buff+1) != *(p_buff+2))
		return 2;
	
	buff_len = *(p_buff+1);
	for(;buff_len>0;buff_len--)//ASDU�����ۼ�
	{
		CRC_value += *p_temp++;
	}
	if(CRC_value == *p_temp)//��У��ֵ�Ƚ�
		return 0;
	else
	{
		my_debug("Var101Frame Sum_value erro!");
		return 1;
	}
}
/******************************************************/
//�������ܣ�101֡У��
//����ֵ����ȷ����0�����󷵻�1,�ɱ�֡�������Ȳ�һ�·���2
//˵����У��֡ͷ��У��ͣ��Լ�DTU��
//�޸����ڣ�2015/10/16
/******************************************************/
UInt8 QLY_101frame_check(UInt8* p_buff)
{
	UInt8 status = 0;
//	UInt16 Frame_RTUNumble;
	if(Fix_frame_top == *p_buff)//�̶�֡
	{
		status |= QLY_Fix101frame_check(p_buff);
	}
	else if(Var_frame_top == *p_buff)//�ɱ�֡
	{
		status |= QLY_Var101frame_check(p_buff);
	}
	else
	{
		my_debug("frame top erro !");//����ͷ����
		status = 1;
	}
	return status;
}
/******************************************************/
//�������ܣ�ȡ��101֡������
//����ֵ��������
//˵����erro����Ϊ1����ȷΪ0
//�޸����ڣ�2015/10/19
/******************************************************/
UInt8 QLY_get101frame_controltype(UInt8* p_buff)
{
	UInt8 control_type=0;
	if(Fix_frame_top == *p_buff)//����Ƕ������ģ��ڶ����ֽ�Ϊ������
	{
		control_type = *(p_buff+1);
        my_debug("Fix_control_type:0x%x",control_type);
	}
	else if(Var_frame_top == *p_buff)//����ǿɱ䳤���ģ�������ֽ��ǿ�����
	{
		control_type = *(p_buff+4);
        my_debug("Var_control_type:0x%x",control_type);
	}
	return control_type;
}
/******************************************************/
//�������ܣ�101������FCB�ж�
//������������
//����ֵ����ȷ����0�����󷵻�1
//˵����FCV��Ч��FCBÿ�η�תΪ��ȷ����ͬ��ʼ�������������Σ���λ��·
//�޸����ڣ�2015/10/19
/******************************************************/
UInt8 QLY_101frame_FCB_detect(UInt8 con_type)
{
	Qconbit status = low;
	if(!(con_type & (1<<4)))//���FCV��ЧλΪ0
		return 0;
	if(con_type&(1<<5))//ȡ��FCBλ
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
		//�ط����һ��ȷ��֡
		QLY_101_FrameSend(QLY_Last_101_Answer_Frame);
		return 1;//�ط����������Ĳ�����Ҫ����
	}
	
	//���FCB_cnt��������3����λ��·
	if(Q_control_101->RFCB_cnt > 3)
	{
		my_debug("FCB Erro Link Init!");
		QLY_101_link_Init();
		return 1;//��λ�����ٴ�����
	}
	return 0;
}
/*************************************************************ƽ��101���Ĵ������*********************************************************************/
//����
//*****************************ƽ��****************************
//������
//							1��ƽ��101���Ĵ���������		UInt8 QLY_BL101FrameProcess(UInt8 *p_buff1)
//							2��ƽ��101���Ĺ��ܽ�������	UInt8 QLY_BL101frame_switch(UInt8* p_buff1,UInt8 con_type,UInt8* p_buff2)
//
//���ڳ�ʱ�ط���
//							1������ȴ���Ӧ״̬��					static UInt8 BL101_Requst_On()
//							2���˳��ȴ���Ӧ״̬��					static UInt8 BL101_Requst_Off()
//							3��ƽ��101��ʱ�ط�������			UInt8 QLY_101Frame_1s_RsendCheck()
//��·���̿��ƣ�
//							1��ƽ��101��·���Ĵ���:				static UInt8 QLY_BL101_link_manage(UInt8* p_buff1,UInt8 con_type,UInt8* p_buff2)
//							2��ƽ��101���ٻ����̿���:			static UInt8 QLY_BL101_total_call(UInt8* p_buff1,UInt8 con_type,UInt8* p_buff2)
//
//
/******************************************************/
//�������ܣ�ƽ��101�򿪳�ʱ�ط�
//��������
//����ֵ��
//˵����
//�޸����ڣ�2015/11/3
/******************************************************/
static UInt8 BL101_Requst_On()
{
	Q_control_101->Wait_Answer_mask = high;
	Q_control_101->Wait101Frame_Times = Wait101Frame_TimeOut;
	return 0;
}
/******************************************************/
//�������ܣ�ƽ��101�رճ�ʱ�ط�
//��������
//����ֵ��
//˵����
//�޸����ڣ�2015/11/3
/******************************************************/
static UInt8 BL101_Requst_Off()
{
	Q_control_101->Wait_Answer_mask = low;//����ȴ���Ӧ��־
	Q_control_101->Resend_Frame_cnt = 0;//�ط���������
	return 0;
}
/******************************************************/
//�������ܣ�ƽ��101��ʱ�ط�����
//��������
//����ֵ���ط�����1�����ط�����0
//˵������Ҫ1S����һ�Σ����ȴ���ʱ���뱨�Ĵ���������
//�޸����ڣ�2015/11/3
/******************************************************/

UInt8 QLY_101Frame_1s_RsendCheck(int signo)
{
//	UInt8* p_buff;
//	static UInt8 link_test_delay = 0;
//	if(QLY_BL101_start_state())//�����·û�����ӣ�10�뷢��һ����·����
//	{
//		link_test_delay++;
//		if(link_test_delay > 10)
//		{
//			QLY_BL101_FixFrame_Send(Call_linkstate_code,high,low,p_buff);//����������·״̬
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
	if(!Q_control_101->Wait_Answer_mask)//������ڵȴ���Ӧ�ڼ䣬ֱ�ӷ���0
		return 0;
	if(--Q_control_101->Wait101Frame_Times)//����ȴ�ʱ��δ��������0
		return 0;
	else
	{
		if(++Q_control_101->Resend_Frame_cnt > Resend_Max_Frames)
		{	
			QLY_101_link_Init();//��·��λ
		}
		else
		{
			//�ط����һ֡������
			QLY_101_FrameSend(QLY_Last_101_Request_Frame);
			my_debug("Rsend 101LastFrame!");
			//�ȴ�ʱ�临λ
			Q_control_101->Wait101Frame_Times = Wait101Frame_TimeOut;
		}
	}
	return 1;
}
/******************************************************/
//�������ܣ�ƽ��101��·���Ĵ���
//������p_buff��������ָ�룬con_type�������Ŀ�����p_buff2�����ͱ���ָ��
//����ֵ����ȷ����0�����󷵻�1
//����˵������·������һ�������״̬��1����·�ȴ�״̬��2���ȴ���λ��3���ȴ�����ȷ�ϡ�4���ȴ���λȷ�ϡ�5����ʼ������ȷ�ϡ�6����·����״̬(��·����״̬�ظ�����ȷ��)
//�޸����ڣ�2015/11/3
/******************************************************/
static enum 
{
    Bl101_LinkWait,
    Bl101_LinkWait_Rest,
    Bl101_RequestLink,
    Bl101_RequestRest,
    Bl101_LinkInitOver,
    Bl101_LinkConnect
} BL101_Link_state = Bl101_LinkWait;//����ƽ��101��·״̬

static UInt8 QLY_BL101_link_manage(UInt8* p_buff1,UInt8 con_type,UInt8* p_buff2)
{
	UInt32 status;
	UInt8 PRM;//��������λ
	if(Fix_frame_top != *p_buff1)//�жϱ���ͷ
		return 1;
	status = con_type & 0x0f;
	PRM = con_type & 0x40;//��������λ
	switch(BL101_Link_state)
	{
		case Bl101_LinkWait: 
					if((PRM == 0x40) && (status == Call_linkstate_code))//�ȴ���·����
					{
						QLY_BL101_FixFrame_Send(Respond_by_link_code,low,low,p_buff2);//��Ӧ��·״̬
						BL101_Link_state = Bl101_LinkWait_Rest;
						return 0;
					}
					break;
                    
		case Bl101_LinkWait_Rest:
					if((PRM == 0x40) && (status == Reset_link_code))//�ȴ���·��λ
					{
					   // my_debug("Bl101_LinkConnect");
						QLY_101_link_Init();//��·��λ
						QLY_BL101_FixFrame_Send(Acknowledge_code,low,low,p_buff2);//�϶�ȷ��
						//set_timer();//��ʱ��С���ͼ��
                        set_timer();
						QLY_BL101_FixFrame_Send(Call_linkstate_code,high,low,p_buff2);//������·״̬
						BL101_Requst_On();//�򿪵ȴ���ʱ�ط�
						BL101_Link_state = Bl101_RequestLink;
						return 0;
					}
					break;
		case Bl101_RequestLink:
					if(status == Respond_by_link_code)//�ȴ���·��Ӧ
					{
					    //my_debug("Bl101_RequestLink");
						BL101_Requst_Off();//�رյȴ���ʱ�ط�
						QLY_BL101_FixFrame_Send(Reset_link_code,high,low,p_buff2);//������·��λ
						BL101_Requst_On();//�򿪵ȴ���ʱ�ط�
						BL101_Link_state = Bl101_RequestRest;
						return 0;
					}
					break;
		case Bl101_RequestRest:
					if(status == Acknowledge_code)//�ȴ���λ��Ӧ
					{
						BL101_Requst_Off();
						//set_timer();//��ʱ��С���ͼ��
                        set_timer();
						QLY_BL101_VarFrame_Send(Trans_data_code,M_EI_NA_1,Spont_Cause,p_buff2,p_buff1);//��ʼ������ͻ��
						//my_debug("Trans Bl101_LinkInitOver!");
						BL101_Requst_On();
						BL101_Link_state = Bl101_LinkInitOver;
						return 0;
					}
					break;
		case Bl101_LinkInitOver:
					if(status == Acknowledge_code)//�ȴ���վȷ�ϳ�ʼ������
					{
						//my_debug("Receive Bl101_LinkInitOverCon");
						BL101_Requst_Off();//�رճ�ʱ�ط�
						BL101_Link_state = Bl101_LinkConnect;
						return 0;
					}
					break;
		case Bl101_LinkConnect:
					if(status == Test_link_code)//�ȴ�������
					{
					    //my_debug("Bl101_LinkConnect");
						QLY_BL101_FixFrame_Send(Acknowledge_code,low,low,p_buff2);//����ȷ��
						return 0;
					}
					break;
		default:
					break;
	}
	if(status == Call_linkstate_code)//�������·�ȴ�״̬֮���յ�������·״̬��������·��λ
	{
        //my_debug("Call_linkstate_code");
		QLY_101_link_Init();//��λ��·
		QLY_BL101_FixFrame_Send(Respond_by_link_code,low,low,p_buff2);//��Ӧ��·״̬
		BL101_Link_state = Bl101_LinkWait_Rest;
		return 0;
	}
	return 1;//����δƥ�䣬����1
}
/******************************************************/
//�������ܣ�ƽ��101��·�Ƿ�ʼ
//������
//����ֵ����ʼ����0��û�з���1
//˵���������ж���·״̬
//�޸����ڣ�2015/12/25
/******************************************************/
//static UInt8 QLY_BL101_start_state(void)
//{
//	if(BL101_Link_state != Bl101_LinkWait)
//		return 0;
//	else
//		return 1;
//}
/******************************************************/
//�������ܣ�ƽ��101��Լ�ٻ����̿���
//������p_buff��������ָ�룬con_type�������Ŀ�����p_buff2�����ͱ���ָ��
//����ֵ����ȷ����0�����󷵻�1
//˵�����������ٻ������ٻ�
//�޸����ڣ�2015/11/09
/******************************************************/
static enum 
{
    Bl101_TotaCall_Wait_Act,
    Bl101_TotaCall_Wait_ActCon,
    Bl101_TotaCall_Wait_PointCon,
    Bl101_TotaCall_Wait_ValCon,
    Bl101_TotaCall_Wait_OverCon
} BL101_totacall_state = Bl101_TotaCall_Wait_Act;//����ƽ��101���ٻ�״̬

static UInt8 QLY_BL101_total_call(UInt8* p_buff1,UInt8 con_type,UInt8* p_buff2)
{
	UInt32 status = con_type & 0x0f;
	UInt32 type_id=0;
	UInt32 TotaCall_Cause=0;
	UInt32 Information;//������Ϣ��32�ֽ�
	//my_debug("QLY_BL101_total_call! status:%d",status);
	if(Var_frame_top == *p_buff1)//�жϱ���Ϊ�ɱ�֡
	{
		type_id = p_buff1[6];//ȡ�����ͱ�ʾ
		
		TotaCall_Cause = p_buff1[8];//ȡ������ԭ��
	}
	switch(BL101_totacall_state)
	{
		case Bl101_TotaCall_Wait_Act://�ȴ����ٻ�����
				if(Var_frame_top != *p_buff1)//�жϱ���ͷ
					break;
				if((type_id == C_IC_NA_1) && (Act_Cause == TotaCall_Cause))//���Ϊ���ͱ�ʾΪ���ٻ�������ԭ��Ϊ����
				{
					Information = p_buff1[15];
                    my_debug("Information:%d",Information);
					switch(Information)
					{
						case TotalCall_Info://���ٻ�
							Bl101_Call_Type = Bl101_TotaCall_Type;//�������ٻ�����
							QLY_BL101_FixFrame_Send(Acknowledge_code,low,low,p_buff2);//ȷ��
							set_timer();//��ʱ��С���ͼ��
							QLY_BL101_VarFrame_Send(Trans_data_code,C_IC_NA_1,Actcon_Cause,p_buff2,p_buff1);//����"����ȷ��"
							BL101_Requst_On();//�򿪵ȴ���ʱ�ط�
							BL101_totacall_state = Bl101_TotaCall_Wait_ActCon;//����ȴ�"����ȷ��"ȷ�ϻظ�֡
							return 0;
						default:
							break;
					}
				}
				break;
		case Bl101_TotaCall_Wait_ActCon:
				if(Fix_frame_top != *p_buff1)//�жϱ���ͷ
					break;
				if(status == Acknowledge_code)
				{
					switch(Bl101_Call_Type)
					{
						case Bl101_TotaCall_Type:
                            my_debug("M_SP_NA_1!");
							BL101_Requst_Off();//�رյȴ���ʱ�ط�
							QLY_BL101_VarFrame_Send(Trans_data_code,M_SP_NA_1,Introgen_Cause,p_buff2,p_buff1);//����Ӧ���ٻ�ԭ�򣬴��͵�����Ϣ
							BL101_Requst_On();//�򿪵ȴ���ʱ�ط�
							BL101_totacall_state = Bl101_TotaCall_Wait_PointCon;//����ȴ�"������Ϣȷ��"
							return 0;
						default:
							break;
					}
				}
				break;
		case Bl101_TotaCall_Wait_PointCon:
			if(Fix_frame_top != *p_buff1)//�жϱ���ͷ
					break;
			if(status == Acknowledge_code)
				{
					switch(Bl101_Call_Type)
					{
						case Bl101_TotaCall_Type:
							BL101_Requst_Off();//�رյȴ���ʱ�ط�
							QLY_BL101_VarFrame_Send(Trans_data_code,M_ME_NC_1,Introgen_Cause,p_buff2,p_buff1);//����Ӧ���ٻ�ԭ�򣬴��Ͳ���ֵ����һ��ֵ
							BL101_Requst_On();//�򿪵ȴ���ʱ�ط�
							BL101_totacall_state = Bl101_TotaCall_Wait_ValCon;//����ȴ�"����ֵȷ��"
							return 0;
						default:
							break;
					}
				}
			break;
		case Bl101_TotaCall_Wait_ValCon:
			if(Fix_frame_top != *p_buff1)//�жϱ���ͷ
					break;
			if(status == Acknowledge_code)
				{
				    
				    my_debug("Bl101_TotaCall_Wait_ValCon!");
					BL101_Requst_Off();//�رյȴ���ʱ�ط�
					QLY_BL101_VarFrame_Send(Trans_data_code,C_IC_NA_1,Actterm_Cause,p_buff2,p_buff1);//���ͼ������,��Ϣ�����ݲ�ͬ
					BL101_Requst_On();//�򿪵ȴ���ʱ�ط�
					BL101_totacall_state = Bl101_TotaCall_Wait_OverCon;//����ȴ�"���ٽ���"ȷ��
					return 0;
				}
			break;
		case Bl101_TotaCall_Wait_OverCon:
			if(Fix_frame_top != *p_buff1)//�жϱ���ͷ
					break;
			if(status == Acknowledge_code)//�յ����ٻ�����ȷ��
				{
				    my_debug("Bl101_TotaCall_Wait_OverCon!");
					BL101_Requst_Off();//�رյȴ���ʱ�ط�
					BL101_totacall_state = Bl101_TotaCall_Wait_Act;//�ٻ����̸�λ
					Bl101_Call_Type = Bl101_NoneCall_Type;//�ٻ����͸�λ
				}
			break;
		default:
			break;
	}
	return 1;
}
/******************************************************/
//�������ܣ�ƽ��101ʱ��ͬ�����̿���
//������p_buff��������ָ�룬con_type�������Ŀ�����p_buff2�����ͱ���ָ��
//����ֵ����ȷ����0�����󷵻�1
//����˵����������վ������ʱ��ͬ������
//�޸����ڣ�2015/10/19
/******************************************************/
static enum 
{
    BL101_GetDelay_Wait_Act,
    BL101_GetDelay_Wait_ActCon
}BL101_GetDelay_State = BL101_GetDelay_Wait_Act;//����ƽ��101��ʱ��ø�״̬

static UInt8 QLY_BL101_ClockSync_Control(UInt8* p_buff1,UInt8 con_type,UInt8* p_buff2)
{
	UInt32 status = con_type & 0x0f;
	UInt32 type_id=0;
	UInt32 Trans_Cause=0;
	my_debug("QLY_BL101_ClockSync_Control!");
	if(Var_frame_top == *p_buff1)//�жϱ���Ϊ�ɱ�֡
	{
		type_id = p_buff1[6];//ȡ�����ͱ�ʾ
		Trans_Cause = p_buff1[8];//ȡ������ԭ��
	}
	switch(BL101_GetDelay_State)
	{
		case BL101_GetDelay_Wait_Act:
			if(Var_frame_top != *p_buff1)//�жϱ���ͷ
					break;
			if((type_id == C_CD_NA_1) && (Act_Cause == Trans_Cause))//��ʱ��ã����
			{
				T1_time = p_buff1[15];//��¼����ʱ��
				T1_time |= p_buff1[16]<<8;
				QLY_BL101_FixFrame_Send(Acknowledge_code,low,low,p_buff2);//ȷ��
				set_timer();//��ʱ��С���ͼ��
				QLY_BL101_VarFrame_Send(Trans_data_code,C_CD_NA_1,Actcon_Cause,p_buff2,p_buff1);//������ʱ��ü���ȷ��
				BL101_Requst_On();//�򿪵ȴ���ʱ�ط�
				BL101_GetDelay_State = BL101_GetDelay_Wait_ActCon;
				return 0;
			}
			if((type_id == C_CD_NA_1) && (Spont_Cause == Trans_Cause))//��ʱ��ã�ͻ����
			{
				//Global_Variable.Link_Trans_Delay = p_buff1[15];//��¼��·��ʱ
				//Global_Variable.Link_Trans_Delay |= p_buff1[16] << 8;
				QLY_BL101_FixFrame_Send(Acknowledge_code,low,low,p_buff2);//����ȷ��
				return 0;
			}
            
			if((type_id == C_CS_NA_1) && (Act_Cause == Trans_Cause))//ʱ��ͬ�������
			{
//				DL101_SetTime(&p_buff1[14]);//�����ն�ʱ��
				//settimeofday(&p_buff1[15],(struct timezone *)0);//�����ն�ʱ��
				QLY_BL101_FixFrame_Send(Acknowledge_code,low,low,p_buff2);//ȷ��
				set_timer();//��ʱ��С���ͼ��
				QLY_BL101_VarFrame_Send(Trans_data_code,C_CS_NA_1,Actcon_Cause,p_buff2,p_buff1);//����ʱ��ͬ������ȷ��
				BL101_Requst_On();//�򿪵ȴ���ʱ�ط�
				BL101_GetDelay_State = BL101_GetDelay_Wait_ActCon;
				return 0;
			}
			break;
		case BL101_GetDelay_Wait_ActCon:
			if(Fix_frame_top != *p_buff1)//�жϱ���ͷ
					break;
			if(status == Acknowledge_code)
			{
				BL101_Requst_Off();//�رյȴ���ʱ�ط�
				BL101_GetDelay_State = BL101_GetDelay_Wait_Act;//״̬�ָ�
				return 0;
			}
			break;
		default:
			break;
	}
	return 1;
}
/******************************************************/
//�������ܣ�ƽ��101��Լң�ؿ�������
//������p_buff��������ָ�룬con_type�������Ŀ�����p_buff2�����ͱ���ָ��
//����ֵ����ȷ����0�����󷵻�1
//˵����
//�޸����ڣ�2015/12/04
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
	if(Var_frame_top == *p_buff1)//�жϱ���Ϊ�ɱ�֡
	{
		type_id = p_buff1[6];//ȡ�����ͱ�ʾ
		Trans_Cause = p_buff1[8];//ȡ������ԭ��
	}
	switch(BL101_TeleControl_State)
	{
		case BL101_TeleControl_Wait_Act:
			if(Var_frame_top != *p_buff1)//�жϱ���ͷ
					break;
			if((type_id == C_SC_NA_1) && (Act_Cause == Trans_Cause))//�ж�ң�ؼ���
			{
			/*
				if(0x00 != p_buff1[15])//���ң�������Ƿ���ȷ
					break;*/
				my_debug("QLY_BL101_TeleControl");
				QLY_BL101_FixFrame_Send(Acknowledge_code,low,low,p_buff2);//ȷ��
				set_timer();//��ʱ��С���ͼ��
				QLY_BL101_VarFrame_Send(Trans_data_code,C_SC_NA_1,Actcon_Cause,p_buff2,p_buff1);//����ң�ؼ���ȷ��
				BL101_Requst_On();//�򿪵ȴ���ʱ�ط�
				BL101_TeleControl_State = BL101_TeleControl_Wait_ActCon;
				return 0;
			}
			break;
		case BL101_TeleControl_Wait_ActCon:
			if(Fix_frame_top != *p_buff1)//�жϱ���ͷ
					break;
			if(status == Acknowledge_code)
			{
				BL101_Requst_Off();//�رյȴ���ʱ�ط�
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
//����:static void IEC101_Single_YK( void )
//˵��:����ң��
//����:��
//���:��
//�༭:R&N
/**************************************************************************/
static void BL101_Single_YK( int frame_len ,UInt8 * p_buffs,UInt8 code)
{
    
    UInt8 codes=code;
	UInt8 * buf;
    buf =  p_buffs;//���յ�������
	//unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;//��ǰ֡���ݵ����λ��
	unsigned short recvcnt = 0;
	unsigned char len1=0, len2=0, len3=0, len4=0;
	unsigned char type= buf[recvcnt+ pIEC101_Struct->linkaddrlen+5];//���ͱ�ʶ��

    if(pIEC101_Struct->fd_usart <=0)
        return ;
     my_debug("recvcnt1 is:%d type is:%d linkaddr:0x%x",recvcnt,type,buf[recvcnt+5]);
	 len1 = pIEC101_Struct->linkaddrlen;
	 len2 = len1+pIEC101_Struct->cotlen;
	 len3 = len2+pIEC101_Struct->conaddrlen;
     len4 = len3+pIEC101_Struct->infoaddlen;
     my_debug("recvcnt is:%d",buf[len4+7]);
     if(buf[len4+7]&0x80)//ң��ѡ��
     {
        my_debug("recvcnt2 is:%d",recvcnt);
        if(buf[len1+7] == IEC_CAUSE_ACT) //����
		{
		    
            my_debug("recvcnt3 is:%d",recvcnt);
			switch(BL101_YK_Sel( frame_len,buf))
			{
			case YK_INVALID_ADDR://ң�ر����� ���� ��ַ����
				BL101_NegativeCON(frame_len,buf,codes);
				break;
			case YK_NORMAL_CONTRL: //һ��ң��ѡ��
				BL101_PositiveCON(frame_len,buf,codes);
				break;
//    			case 2: //����ѡ��
//    				IEC_PositiveCON(ucPort,pTemp_ASDUFrame , ucTemp_Length);
//    				break;
			}
		}
		else if(buf[len1+7] == IEC_CAUSE_DEACT) //ֹͣ����ȷ��
		{
		
            my_debug("recvcnt4 is:%d",recvcnt);
			switch(BL101_YK_SelHalt(buf))
			{
			case YK_INVALID_ADDR:
				BL101_NegativeCON(frame_len,buf,codes);
				break;
			case YK_CANCEL_SEL://һ��ң��ѡ����
				BL101_PositiveCON(frame_len,buf,codes);
				break;
			}
		}
//		else
//            IEC101_NegativeCON(frame_len);
     }
     else//ң��ִ��
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
//����:static static void IEC101_YK_Sel( int frame_len )
//˵��:����ң��ѡ��
//����:��
//���:��
//�༭:R&N
/**************************************************************************/
static unsigned char BL101_YK_Sel(int frame_len,UInt8 * p_buffs)
{
	//char * buf =(char *)  save_recv_data;//���յ�������
	UInt8 * buf;
    buf =  p_buffs;//���յ�������
	//unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;//��ǰ֡���ݵ����λ��
	unsigned short recvcnt = 0;
    unsigned short sPointNo, j,len3,len4;
	Point_Info   *  Point_Info;
    
    len3 = pIEC101_Struct->conaddrlen+pIEC101_Struct->cotlen+pIEC101_Struct->linkaddrlen;
    len4 = len3+pIEC101_Struct->infoaddlen;
	if(sys_mod_p->usYK_Num ==0)
		return YK_INVALID_ADDR;
//    sPointNo = (buf[(recvcnt+len3+7)&0x3FF]<<8) + buf[(recvcnt+len3+7)&0x3FF];//���
    sPointNo = buf[recvcnt+len3+7];//���
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
    
	if((buf[recvcnt+len4+7]&0x01)==0X01)//�������ң�ط�բѡ��  ����ң�غ�բѡ��
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
static unsigned char BL101_YK_SelHalt(UInt8 * p_buffs)
{
	//char * buf =(char *)  save_recv_data;//���յ�������
	UInt8 * buf;
    buf =  p_buffs;//���յ�������
	//unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;//��ǰ֡���ݵ����λ��
	unsigned short recvcnt = 0;
	unsigned short sPointNo, i, len3, len4;
	Point_Info   *  Point_Info;

    len3 = pIEC101_Struct->conaddrlen+pIEC101_Struct->cotlen+pIEC101_Struct->linkaddrlen;
    len4 = len3+pIEC101_Struct->infoaddlen;

	if(sys_mod_p->usYK_Num ==0)
		return YK_INVALID_ADDR;

    //sPointNo = (buf[(recvcnt+len3+7)&0x3FF]<<8) | buf[(recvcnt+len3+7)&0x3FF];//���
    sPointNo = buf[recvcnt+len3+7];//���
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

	if((buf[recvcnt+len4+7]&0x01)==0X01)//�������ң�ط�բѡ��ȡ��  ����ң�غ�բѡ��ȡ��
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
//����˵��:ң��ִ��
//����:ASDU֡     �ͳ���
//�༭:R&N1110
//ʱ��:2015.6.1
//YK_ZHIXIN_FAIL			5
//YK_ZHIXIN_SUCC		6
/****************************************************************************/
static unsigned char BL101_AP_YKExec(int frame_len,UInt8 * p_buffs)
{
	//char * buf =(char *)  save_recv_data;//���յ�������
	UInt8 * buf;
    buf =  p_buffs;//���յ�������
	//unsigned short recvcnt = pIEC101_Struct->pBufGet&0x3FF;//��ǰ֡���ݵ����λ��
	unsigned short recvcnt = 0;
	unsigned short sPointNo, i,len3, len4;
	Point_Info   *  Point_Info;
//len3=5 len4=8
    len3 = pIEC101_Struct->conaddrlen+pIEC101_Struct->cotlen+pIEC101_Struct->linkaddrlen;
    len4 = len3+pIEC101_Struct->infoaddlen;
	if(sys_mod_p->usYK_Num ==0)
		return YK_INVALID_ADDR;
//    sPointNo = (buf[(recvcnt+len3+7)&0x3FF]<<8) | buf[(recvcnt+len3+7)&0x3FF];//���
    sPointNo = buf[recvcnt+len3+7];//���
        
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

	if((buf[len4+7]&0x01)==0X01)   //�������ң�ط�բ
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

/******************************************************/
//�������ܣ�ƽ��101ͻ���������̹���
//������
//����ֵ����ȷ����0�����󷵻�1
//�޸����ڣ�2015/10/25
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
	if(Fix_frame_top != *p_buff1)//�жϱ���ͷ
			return 1;
	switch(BL101_SpontTrans_State)
	{
		case BL101_SpontTrans_Wait1_Con:
			if(status == Acknowledge_code)
			{
				BL101_Requst_Off();//�رյȴ���ʱ�ط�
//				QLY_BL101_SpontTransData(Sudden_fault,M_SP_TB_1);//���ʹ�ʱ���ͻ��֡��������Ϣ
				QLY_BL101_SpontTransData(M_SP_TB_1);//���ʹ�ʱ���ͻ��֡��������Ϣ
				BL101_Requst_On();//�򿪵ȴ���ʱ�ط�
				BL101_SpontTrans_State = BL101_SpontTrans_Wait3_Con;
				return 0;
			}
			break;
            /*
		case BL101_SpontTrans_Wait2_Con:
			if(status == Acknowledge_code)
			{
				BL101_Requst_Off();//�رյȴ���ʱ�ط�
                set_timer();//��ʱ��С���ͼ��
				
				if(Sudden_fault->type == just_mark)
				{
					BL101_SpontTrans_State = BL101_SpontTrans_None;//״̬�ָ�
					OSMemPut(Buff16_FreeList,Sudden_fault);//ɾ���ڴ�
					return 0;
				}
				QLY_BL101_SpontTransData(Sudden_fault,M_ME_NA_1);;//���Ͳ���ֵ����һ��ֵ��ͻ��
                
				BL101_Requst_On();//�򿪵ȴ���ʱ�ط�
				BL101_SpontTrans_State = BL101_SpontTrans_Wait3_Con;
				return 0;
			}
			break;*/
		case BL101_SpontTrans_Wait3_Con:
			if(status == Acknowledge_code)
			{
				BL101_Requst_Off();//�رյȴ���ʱ�ط�
				BL101_SpontTrans_State = BL101_SpontTrans_None;//״̬�ָ�
				//OSMemPut(Buff16_FreeList,Sudden_fault);//ɾ���ڴ�
				return 0;
			}
		default:
			break;
	}
	return 1;
}
/******************************************************/
//�������ܣ�ƽ��101��λ����
//������
//����ֵ��
//�޸����ڣ�2015/12/03
/******************************************************/
static void QLY_BL101_state_Reset(void)
{
	/*UInt8 *p_fault,err;
	//p_fault = OSQAccept(QSem_Fault_Trants,&err);
	
	while((void*)0 != p_fault)//����ϱ������е��ϱ�����
	{
		//OSMemPut(Buff16_FreeList,p_fault);//ɾ���ڴ�
		//p_fault = OSQAccept(QSem_Fault_Trants,&err);
	}
	*/
	my_debug("QLY_BL101_state_Reset");
	Q_control_101->Wait_Answer_mask = low;//����ȴ���Ӧ��־
	Q_control_101->Resend_Frame_cnt = 0;//�ط���������
	
	BL101_Link_state = Bl101_LinkWait;//��·״̬��λ
	BL101_totacall_state = Bl101_TotaCall_Wait_Act;//���ٻ�״̬��λ
	Bl101_Call_Type = Bl101_NoneCall_Type;//�ٻ�����״̬��λ
	BL101_GetDelay_State = BL101_GetDelay_Wait_Act;//ʱ��ͬ��״̬��λ
	BL101_SpontTrans_State = BL101_SpontTrans_None;//ͻ���ϴ�״̬��λ
	BL101_TeleControl_State = BL101_TeleControl_Wait_Act;//ң�ظ�λ
	QLY_ReciveFrame_temp[0] = 0;//�ݴ汨�ı�־����
	
}
/******************************************************/
//�������ܣ�ƽ��101���Ĺ��ܽ�������
//������p_buff��������ָ�룬con_type�������Ŀ�����p_buff2�����ͱ���ָ��
//����ֵ����ȷ����0�����󷵻�1
//�޸����ڣ�2015/10/19
/******************************************************/
//���崦����ָ������
static UInt8 (*p_QLY_BL101_function[4])(UInt8* p_buff1,UInt8 con_type,UInt8* p_buff2) = {
//ƽ��101���Ĵ�����
	QLY_BL101_link_manage,//��·����
	QLY_BL101_total_call,//���ٻ�
	QLY_BL101_ClockSync_Control,//ʱ��ͬ��
	QLY_BL101_SpontTrans_Control//ͻ���ϴ�
};//����˳�����ȼ�����

UInt8 QLY_BL101frame_switch(UInt8* p_buff1,UInt8 con_type,UInt8* p_buff2)
{
	UInt8 i;
	if(BL101_Link_state == Bl101_LinkConnect)//��·������
	{
	    my_debug("Bl101_LinkConnect is ok!");
		for(i=0;i<4;i++)
		{
			if(!(p_QLY_BL101_function[i](p_buff1,con_type,p_buff2)))//������ı���һ����������
				return 0;
		}
	}
	else
	{
		if(!QLY_BL101_link_manage(p_buff1,con_type,p_buff2))//��·δ���ӣ�������·����
			return 0;
	}
	//printf("unknow frame type !\r\n");//�������ǲ�ʶ��ı���
	return 1;
}
/******************************************************/
//�������ܣ�ƽ��101���Ĵ���������
//������
//����ֵ����ȷ����0�����󷵻�3FCB�ж�ʧ�ܣ��ط����һ�����Ļ��߸�λ������4δʶ��ı��ģ�5�����ݴ�
//�޸����ڣ�2015/10/19
/******************************************************/
UInt8 QLY_BL101FrameProcess(UInt8 *p_buff1)
{
	UInt8 con_type,Frame_length;
	UInt8* p_buff2;
	UInt32 i;
	con_type = QLY_get101frame_controltype(p_buff1);//��ȡ������
	
	if(Q_control_101->Wait_Answer_mask == high)//������ڵȴ���Ӧ�ڼ䣬��Ҫ���յ����������ݴ�
	{
		if((con_type & (1<<6)) && ((con_type & 0x0f) != Call_linkstate_code))//�ж��Ƿ��յ������������Ҳ���������·״̬
		{
			//�������ݴ�
			QLY_ReciveFrame_temp[0] = 1;
			Frame_length = QLY_101FrameLen(p_buff1);
			for(i=1;i<=Frame_length;i++)
			QLY_ReciveFrame_temp[i] = *p_buff1++;
			return 5;
		}
	}
    
    if(!QLY_BL101_TeleControl(p_buff1,con_type,p_buff2))//ң��
		return 0;
	//�����׼101����
	if(QLY_101frame_FCB_detect(con_type))//FCB�ж�
		return 3;
	if(QLY_BL101frame_switch(p_buff1,con_type,p_buff2))//������
		return 4;
    
	if(Q_control_101->Wait_Answer_mask == low)	//������ڵȴ���Ӧ�ڼ�
	{/*
		//�������ͻ���¼�������ͻ���¼�
		p_fault = OSQAccept(QSem_Fault_Trants,&err);
		if((void*)0 != p_fault)
		{
			Sudden_fault = (Spont_Trans_type*)p_fault;//��¼��Ҫ�����ͻ���¼�
			QLY_BL101_SpontTransData(Sudden_fault,M_SP_NA_1);//���Ͳ���ʱ���ͻ��֡��������Ϣ
			BL101_Requst_On();//�򿪵ȴ���ʱ�ط�
			BL101_SpontTrans_State = BL101_SpontTrans_Wait1_Con;//�л�ͻ���ϴ����̵��ȴ���Ӧ
			return 0;
		}		*/
		//�����ݴ汨��
		if(QLY_ReciveFrame_temp[0])//�����ݴ汨��
		{
			QLY_ReciveFrame_temp[0] = 0;//�ݴ汨�ı�־����
			con_type = QLY_get101frame_controltype(&QLY_ReciveFrame_temp[1]);//��ȡ������
			if(QLY_101frame_FCB_detect(con_type))//FCB�ж�
				return 6;
			//�����ݴ汨��
			my_debug("process save frame!");
			if(QLY_BL101frame_switch(&QLY_ReciveFrame_temp[1],con_type,p_buff2))//������
			return 4;
		}
	}
	return 0;
}
//*******************************************************101���Ĵ���������********************************************************************************//

/******************************************************/
//�������ܣ�ƽ��101ͻ���ϴ�������
//������
//����ֵ����ȷ����0�����󷵻�1
//�޸����ڣ�2015/10/25
/******************************************************/
UInt8 QLY_BL101_SpontTrans()
{
	if(BL101_Link_state != Bl101_LinkConnect)//�����·δ���ӣ�������
		return 1;
	if(Q_control_101->Wait_Answer_mask == low)//���ڵȴ���Ӧ�ڼ䣬ֱ��ͻ������
	{
		QLY_BL101_SpontTransData(M_SP_NA_1);//���Ͳ���ʱ���ͻ��֡��������Ϣ
		BL101_Requst_On();//�򿪵ȴ���ʱ�ط�
		BL101_SpontTrans_State = BL101_SpontTrans_Wait1_Con;//�л�ͻ���ϴ����̵��ȴ���Ӧ
	}
	return 0;
}

/******************************************************/
//�������ܣ�ƽ��101���Ĵ���������
//������
//����ֵ����ȷ����0�����󷵻�1֡У��δͨ����2�������ȡʧ�ܣ�3FCB�ж�ʧ�ܣ��ط����һ�����Ļ��߸�λ������4δʶ��ı��ģ�5�����ݴ棬6�ݴ汨��FCB�ж�ʧ�ܣ�
//�޸����ڣ�2015/10/19
/******************************************************/
//101���Ĵ���������
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
	if(QLY_101frame_check(p_buff1))//֡У��
		return 1;
	switch(Balance101_Mask)//101ͨѶ��Լģʽѡ��
	{
		case 0://��ƽ��ʽ
				break;
		case 1://ƽ��ʽ
			return QLY_BL101FrameProcess(p_buff1);//������
		default:
			break;
	}
	return 0;//�ɹ�����0
}

