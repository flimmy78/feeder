/*************************************************************************/
// iec104.c                                        Version 1.00
// ÃèÊö
//
// ±¾ÎÄ¼þÊÇDTUÍ¨Ñ¶Íø¹Ø×°ÖÃµÄIEC60870-5-104¹æÔ¼´¦Àí³ÌÐò
// ±àÐ´ÈË:R&N
//email           :R&N
//  ÈÕ	   ÆÚ:2014.9.16
//  ×¢	   ÊÍ:
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
#include <net/if.h>
#include<pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <linux/rtc.h>
#include<semaphore.h>
#include <ctype.h>
#include <ti/syslink/Std.h>     /* must be first */
#include <ti/ipc/SharedRegion.h>
#include <ti/ipc/MessageQ.h>
#include <ti/syslink/utils/IHeap.h>
#include "common.h"
#include "sharearea.h"
#include "monitorpc.h"
#include "nand_rw.h"
#define MAXBSIZE 65536 
#include "iec104.h"
/******************È«¾Ö±äÁ¿£¬´æ´¢´ó²¿·Ö104¹æÔ¼ÖÐµÄ²ÎÊý******/
static UInt32               yk_select_flag=0;//Ò£¿ØÑ¡ÖÐ±ê¼Ç£¬¿ª¹ØÑ¡ÖÐ£¬Ã¿¸öindex¶ÔÓ¦2bit,×î´ó16¸ö
IEC104Struct  				* pIEC104_Struct;
static unsigned int cout_framer=0;
//static unsigned int cuntr=0; 
static unsigned int flag_kvalue=0;
static unsigned char cout_Iframe=0;
struct itimerval timer;
off_t currpos;

pthread_mutex_t mutex;
static unsigned char       	save_recv_data[IEC104BUFDOWNNUMBER * IEC104BUFDOWNLENGTH];
static unsigned char       	save_send_data[IEC104BUFDOWNLENGTH];
static BufStruct 			* pBufRecvRoot;		//¹æÔ¼»º´æ½á¹¹¸ùÖ¸Õë
static BufStruct 			* pBufRecvFill;		//¹æÔ¼»º´æ½á¹¹ÏÂÒ»¸öÒªÌî³äµÄ½á¹¹Ö¸Õë
static BufStruct 			* pBufRecvGet;		//¹æÔ¼»º´æ½á¹¹ÏÂÒ»¸öÒª¶ÁÈ¡µÄ½á¹¹Ö¸Õë
static int 		  		    new_fd=-1;
static int 		   		    sock_fd=-1;
//sem_t              		    sem_iec104;
static LOCAL_Module                * sys_mod_p;

static YXInfoStruct         * pYXInfoRoot;     //ÓÃÓÚÒ£ÐÅ±äÎ»¼ÇÂ¼
static YXInfoStruct         * pYXInfoFill;
static YXInfoStruct         * pYXInfoGet;

static SOEInfoStruct        * pSOERoot;         //ÓÃÓÚÒ£ÐÅSOEÊÂ¼þ
static SOEInfoStruct        * pSOEFill;
static SOEInfoStruct        * pSOEGet;
/***************************  LOCAL   FUNCTION**********************************/
static  void IEC104_Creat_connection( void );
static void IEC104_Read_Conf(char * fd,  struct _IEC104Struct   * iec104);
static int  Get_Parp_From_Line(char *line, IEC104Struct   * item);
static unsigned char IEC104_Init_Protocol(void);
static unsigned char IEC104_VerifyDLFrame(unsigned char * pTemp_Buf , unsigned short usTemp_Length);
static void IEC104_Protocol_Entry(  void  );
static void IEC104_APDU_Send(unsigned char ucTemp_FrameFormat,unsigned char ucTemp_Value);
static void IEC104_ProcessAPDU(unsigned char * pTemp_Buf, unsigned char ucTemp_Length) ;
static void IEC104_AP_RP(IEC104_ASDU_Frame *pRecv_ASDUFrame,unsigned char ucTemp_Length) ;//´Ë´¦³¤¶ÈÎªASDU³¤¶È
static void IEC104_AP_UnknownCommAddr(IEC104_ASDU_Frame *pRecv_ASDUFrame,unsigned char ucTemp_Length) ;//ASDU³¤¶È
static void IEC104_AP_UnknownCause(IEC104_ASDU_Frame *pRecv_ASDUFrame,unsigned char ucTemp_Length);
static void IEC104_AP_UnknownType(IEC104_ASDU_Frame *pRecv_ASDUFrame, unsigned char ucTemp_Length);
static void IEC104_UnsoSend( void );
static void IEC104_AP_INTERROALL_END( void );
static void IEC104_AP_INTERROALL( void );
static void IEC104_AP_YK(IEC104_ASDU_Frame *pRecv_ASDUFrame,unsigned char ucTemp_Length, unsigned char flag) ;//´Ë´¦³¤¶ÈÎªASDU³¤¶È
static void IEC104_AP_UnknownInfoAddr(IEC104_ASDU_Frame *pRecv_ASDUFrame,unsigned char ucTemp_Length);
static void IEC104_AP_SUCC(IEC104_ASDU_Frame *pRecv_ASDUFrame,unsigned char ucTemp_Length);
static void IEC104_AP_NegativeCON(IEC104_ASDU_Frame *pRecv_ASDUFrame,unsigned char ucTemp_Length);
static void IEC104_AP_PositiveCON(IEC104_ASDU_Frame *pRecv_ASDUFrame,unsigned char ucTemp_Length);
static unsigned char IEC104_AP_YKSel(IEC104_ASDU_Frame * pRecv_ASDUFrame);
static unsigned char IEC104_AP_YKSelHalt(IEC104_ASDU_Frame * pRecv_ASDUFrame);
static unsigned char IEC104_AP_YKExec(IEC104_ASDU_Frame * pRecv_ASDUFrame, unsigned char ucTemp_Length);
static void IEC104_AP_CS(unsigned char *pTemp_Buf,unsigned char ucTemp_Length)  ;
static void IEC104_AP_IC(IEC104_ASDU_Frame *pRecv_ASDUFrame , unsigned char ucTemp_Length); //´Ë´¦³¤¶ÈÎªASDU³¤¶È
static void IEC104_AP_RQALL_CON(IEC104_ASDU_Frame *pRecv_ASDUFrame);
static unsigned char IEC104_AP_ALL_YX( void );// ±¨¸æÈ«²¿Ò£²âÊý¾Ý
static unsigned char IEC104_Process_COS( void ) ;//Ó¦ÓÃ²ã´¦Àí
static unsigned char IEC104_Process_SOE( void ); //Ó¦ÓÃ²ã´¦Àí

/*******************************EXTERN  VARIOUS*****************************/

sem_t              			sem_soe104;

extern YK_RESULT_STRUCT * g_YK_State_p;

/***************************************************************************/
//º¯Êý: static   void IEC104_Creat_connection( void )
//ËµÃ÷:ÓÃÓÚÍøÂçÁ¬½Ó£¬¹©iec104¹æÔ¼Ê¹ÓÃ
//ÊäÈë:
//Êä³ö:
//±à¼­:R&N
//Ê±¼ä:2015.4.21
/***************************************************************************/
static   void IEC104_Creat_connection( void )
{
	struct ifreq ifr;
	socklen_t len;
	struct sockaddr_in  my_addr,  their_addr;
	unsigned int  lisnum=2;
	fd_set rfds;
	struct timeval tv;
	int retval, maxfd = -1;
	int on = 1;

	if ((sock_fd= socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		my_debug("socket");
		exit(1);
	}
	setsockopt( sock_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );//ÔÊÐíµØÖ·Á¢¼´Ê¹ÓÃ

	 strncpy(ifr.ifr_name, "eth0" , IFNAMSIZ);
	 if (ioctl(sock_fd, SIOCGIFADDR, &ifr) == 0)   //SIOCGIFADDR »ñÈ¡ip address
	 {
		bzero(&my_addr,  sizeof(my_addr));
		memcpy(&my_addr, &ifr.ifr_addr, sizeof(my_addr));
		my_debug("Server ip:%s port:2404",inet_ntoa(my_addr.sin_addr));
	 }
	my_addr.sin_family = PF_INET;
	my_addr.sin_port = htons(2404);
	if (bind(sock_fd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr))== -1) {
		my_debug("bind");
		exit(1);
	}
	if (listen(sock_fd, lisnum) == -1) {
		my_debug("listen");
		exit(1);
	}

	while (1)
	{
		len = sizeof(struct sockaddr);
		if ((new_fd =accept(sock_fd,  (struct sockaddr *) &their_addr, &len)) == -1)
		{
			my_debug("accept");
			exit(errno);
		}
		else
		{
			my_debug("server: got connection from %s, port %d, socket %d", inet_ntoa(their_addr.sin_addr), ntohs(their_addr.sin_port), new_fd);
		}
		while (1)
		{
			FD_ZERO(&rfds);		    // °Ñ¼¯ºÏÇå¿Õ
			FD_SET(0, &rfds);		// °Ñ±ê×¼ÊäÈë¾ä±ú 0 ¼ÓÈëµ½¼¯ºÏÖÐ
			FD_SET(new_fd, &rfds);  // °Ñµ±Ç°Á¬½Ó¾ä±ú new_fd ¼ÓÈëµ½¼¯ºÏÖÐ
			maxfd = 0;
			if (new_fd > maxfd)
				maxfd = new_fd;
			tv.tv_sec = 1;			// ÉèÖÃ×î´óµÈ´ýÊ±¼ä
			tv.tv_usec = 0;

			retval = select(maxfd +1, &rfds, NULL, NULL, &tv);// ¿ªÊ¼µÈ´ý

			if (retval == (-1))
			{
				my_debug("select err:%s", strerror(errno));
				break;
			}
			else if (retval == 0)  //³¬Ê±µÈ´ý
			{
				continue;
			}
			else 			//ÊÕµ½Êý¾Ý
			 {
				if (FD_ISSET(0, &rfds)) 					//ÖÐ¶ÏÊäÈëÊý¾Ý
				{
				}
				if (FD_ISSET(new_fd, &rfds))
				{
					bzero(pBufRecvFill->buf, IEC104BUFDOWNLENGTH);
					len = recv(new_fd, pBufRecvFill->buf, IEC104BUFDOWNLENGTH, 0);		// ½ÓÊÕ¿Í»§¶ËµÄÏûÏ¢
					if (len > 0)
					{
						if(*(pBufRecvFill->buf)==0x68)
						{
						    my_debug("recv data from station");
							//pIEC104_Struct->ucNoRecvCount = 0;
							pBufRecvFill->usLength = len;
							pBufRecvFill->usStatus = FULL;
							pBufRecvFill = pBufRecvFill->next;
//							sem_post(&sem_iec104);
						}
					}
					else
					{
						if (len < 0)
							my_debug("receive message fail, err code:%d err msg:%s\n",errno, strerror(errno));
						else
							my_debug("The client exit, stop the connection,new_fd:%d retval:%d\n",new_fd,retval);
                        break;
					}
				}
			}
		}
		close(new_fd);// ´¦ÀíÃ¿¸öÐÂÁ¬½ÓÉÏµÄÊý¾ÝÊÕ·¢½áÊø
		new_fd = -1;
	}
   close(sock_fd);
   sock_fd = -1;
   pthread_exit("Network_Thread finished!\n");
}



/***************************************************************************/
//º¯Êý: void Init_IEC104_Buf(void)
//ËµÃ÷:³õÊ¼»¯»º³å£¬°üÀ¨ÊÕµ½Êý¾Ý±£´æµÄ»º³å£¬²¢½«Á¬½Ó³ÉÁ´±í
//ÊäÈë:ÎÞ
//Êä³ö:ÎÞ
/***************************************************************************/
 int  IEC104_Init_Buf(void)
{
	void * pointer=NULL;
	unsigned short step;
    unsigned char * pBuf;
	BufStruct * pBufList, * pBufList1;
	YXInfoStruct * pYXinfoList, * pYXinfoList1;
	SOEInfoStruct * pSOEList, * pSOEList1;

	pIEC104_Struct = malloc(sizeof(IEC104Struct));
	if(pIEC104_Struct == NULL)
	{
		my_debug( "pIEC104_Struct malloc fail\n");
		goto leave1;
	}
	memset(pIEC104_Struct, 0, sizeof(IEC104Struct));
//´´½¨Ñ­»·Á´±íÓÃÀ´´¦Àí½ÓÊÕµ½µÄÊý¾Ý
	pointer = malloc(IEC104BUFDOWNNUMBER*sizeof(BufStruct));//    3*12
     	if (pointer ==NULL)
	{
		my_debug("pBufRecvRoot malloc fail\n");
		goto leave2;
	}
	memset(pointer, 0, IEC104BUFDOWNNUMBER*sizeof(BufStruct));
     	pBufRecvRoot = (BufStruct *)pointer;
     	pBufRecvFill = pBufRecvRoot;
     	pBufRecvGet  = pBufRecvRoot;

//	pBuf = (unsigned char *)malloc(IEC104BUFDOWNNUMBER * IEC104BUFDOWNLENGTH); //  4*256
//     	if (pBuf ==NULL)
//	{
//		my_debug("pBufRecvRoot malloc fail\n" );
//		goto leave3;
//	}
	pBuf = save_recv_data;
  	pBufList = (BufStruct * )pBufRecvRoot;
 	pBufList1 =(BufStruct * ) pBufRecvRoot;
 	step=0;
 	while (step < IEC104BUFDOWNNUMBER)
 	{
       	pBufList->usStatus = EMPTY;
       	pBufList->buf = pBuf;
       	pBufList->usLength = 0;
       	pBufList1++;
       	pBufList->next = pBufList1;
       	pBuf += IEC104BUFDOWNLENGTH ;
       	pBufList++;
       	step++;
 	}
 	pBufList--;
 	pBufList->next = (BufStruct * )pBufRecvRoot;

//	pointer = malloc (IEC104BUFUPLENGTH);	//¿ª±Ù¿Õ¼äÓÃÓÚ¹æÔ¼·¢ËÍÊý¾Ý£¬³¤¶È256
//     	if (pointer ==NULL)
//	{
//		my_debug("pBufRecvRoot malloc fail\n");
//		goto leave4;
//	}
//	pIEC104_Struct->pSendBuf=(unsigned char *)pointer;
	pIEC104_Struct->pSendBuf=(unsigned char *)save_send_data;

//´´½¨Ñ­»·Á´±íÓÃÀ´´¦ÀíÒ£ÐÅ±äÎ»
 	pYXInfoRoot = malloc(IEC104YXINFONUMBER*sizeof(YXInfoStruct));//4
     	if (pYXInfoRoot ==NULL)
	{
		my_debug("pYXInfoRoot malloc fail\n");
		goto leave3;
	}
	memset(pYXInfoRoot, 0, IEC104YXINFONUMBER);
 	pYXInfoFill = pYXInfoRoot;
 	pYXInfoGet  = pYXInfoRoot;
 	pYXinfoList = pYXInfoRoot;
 	pYXinfoList1 =pYXInfoRoot;
 	step=0;
 	while (step < IEC104YXINFONUMBER)
 	{
       	pYXinfoList->ucStatus = EMPTY;
       	pYXinfoList1++;
       	pYXinfoList->next = pYXinfoList1;
       	pYXinfoList++;
       	step++;
 	}
 	pYXinfoList--;
 	pYXinfoList->next =pYXInfoRoot;
//´´½¨Ñ­»·Á´±íÓÃÀ´´¦ÀíSOEÊÂ¼þ
 	pSOERoot = malloc(IEC104SOENUMBER*sizeof(SOEInfoStruct));//4
     	if (pSOERoot ==NULL)
	{
		my_debug("pSOERoot malloc fail\n");
		goto leave4;
	}
	memset(pSOERoot, 0, IEC104SOENUMBER);
 	pSOEFill = pSOERoot;
 	pSOEGet  = pSOERoot;
 	pSOEList = pSOERoot;
 	pSOEList1 =pSOERoot;
 	step=0;
 	while (step < IEC104SOENUMBER)
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
leave2:
	free(pointer);
leave1:
	free(pIEC104_Struct);
	my_debug("malloc fail");
	return -1;
}
static unsigned char Read_Soe_Count(int  file)
{
    unsigned char bufS[1] = {0};
    lseek(file,0,SEEK_SET);
    if(file>0)
    {
        read(file, bufS,1); //
        cout_framer=bufS[0];
        lseek(pIEC104_Struct->logsoEfd,(cout_framer*11),SEEK_SET);
        my_debug("count_framerS:%d",cout_framer);
    }
    return cout_framer;
}

/***************************************************************************/
//º¯Êý:void IEC104_Process_Message(msg->cmd, msg->buf, UInt32 buf)
//ËµÃ÷:´¦ÀíMESSAGEµÄº¯Êý
//ÊäÈë:messageµÄÃüÁîºÍbufÒÔ¼°data£¬ÆäÖÐ
//Êä³ö:
//±à¼­:R&N
//Ê±¼ä:2015.9.23
/***************************************************************************/
void IEC104_Process_Message(UInt32 cmd, UInt32 buf, UInt32 data)
{
    struct rtc_time  * stime;
    unsigned char buff[3] = {0};
    unsigned char buffs[11] = {0};
    unsigned char bufcunt[1]={0};
    UInt8   i;
    UInt8   j;
	unsigned int usec=0;
    //if(new_fd == -1)
     //   return ;
    
    my_debug("cmd104:0x%x buf:0x%x data:0x%x",cmd, buf, data);
    switch (cmd&0xFFFF)
    {
        case MSG_COS:
            for(i=0; i<32; i++)
            {
                if(buf&(0x01<<i))//±íÊ¾µÚ(cmd>>16)*32+iÍ¨µÀ±äÎ»
                {
                    if(pYXInfoFill->ucStatus == EMPTY)
                    {
                        //pYXInfoFill->usIndex = ((((cmd>>16)&0xFFFF)<<5)+i)&0xFFFF;
                        pYXInfoFill->usIndex = i;
                        pYXInfoFill->ucYXValue = (data>>i)&0x1;
                        pYXInfoFill->ucStatus = FULL;
                        if(sys_mod_p->pc_flag &(1<<MSG_EVENT_COS))
                        {
                            buff[0] = pYXInfoFill->usIndex;
                            buff[1] = 0;
                            buff[2] = pYXInfoFill->ucYXValue;
                            PC_Send_Data(MSG_EVENT_COS, buff, 3);
                        }
                        pYXInfoFill = pYXInfoFill->next;
                    }
                }
            }
            break;
            
        case MSG_SOE:
             stime=malloc(sizeof(struct rtc_time));
             //cout_framer++;
            for(i=0; i<32; i++)
            {
                if(buf&(1<<i))//±íÊ¾µÚ(cmd>>16)*32+iÍ¨µÀ±äÎ»
                {
                    //if(pSOEFill->ucStatus == EMPTY)
                    //{
                        my_debug("send SOE number :%d",cout_framer);
                        //pSOEFill->usIndex = ((((cmd>>16)&0xFFFF)<<5)+i)&0xFFFF;
                        pSOEFill->usIndex = i;
                        pSOEFill->ucYXValue = (data>>i)&0x1;
                        pSOEFill->ucStatus = FULL;
                        usec=Get_Rtc_Time(stime);
                        pSOEFill->usMSec = stime->tm_sec*1000+usec/1000;
                        pSOEFill->ucMin = stime->tm_min;
                        pSOEFill->ucHour =stime->tm_hour;
                        pSOEFill->ucDay = stime->tm_mday;
                        pSOEFill->ucMonth = stime->tm_mon;
                        pSOEFill->ucYear = stime->tm_year;
                        my_debug("%d-%02d-%02d %02d:%02d:%02d:%02d",pSOEFill->ucYear+1900,pSOEFill->ucMonth+1,pSOEFill->ucDay,pSOEFill->ucHour,pSOEFill->ucMin,stime->tm_sec,usec/1000);
                        buffs[0] = pSOEFill->usIndex;
                        buffs[1] = 0;
                        buffs[2] = pSOEFill->ucYXValue;
                        buffs[3] = (pSOEFill->ucYear-100);
                        buffs[4] = (pSOEFill->ucMonth+1);
                        buffs[5] = pSOEFill->ucDay;
                        buffs[6] = pSOEFill->ucHour;
                        buffs[7] = pSOEFill->ucMin;
                        buffs[8] = (unsigned char)( pSOEFill->usMSec & 0xff);
                        buffs[9] = (unsigned char)(pSOEFill->usMSec >> 8);
                        buffs[10] = '\n';
                        //sem_wait(&sem_soe104);//ÐÅºÅÁ¿ÖµÔö1
                        write(pIEC104_Struct->logsoEfd, buffs,11);
                        /*
                        if(flock(pIEC104_Struct->logsoEfd,LOCK_EX|LOCK_NB)==0)
                        {
                            write(pIEC104_Struct->logsoEfd, buffs,11);
                            my_debug(" file has been locked!");
                        }*/
                        if(sys_mod_p->pc_flag &(1<<MSG_EVENT_SOE))
                        {
                             PC_Send_Data(MSG_EVENT_SOE, buffs, 11); 
                        }
                        //free(buffs);
                       pSOEFill = pSOEFill->next;
                    //}
                    cout_framer++;
                    bufcunt[0] = cout_framer;
                    lseek(pIEC104_Struct->soEfile,0,SEEK_SET);
                    write(pIEC104_Struct->soEfile, bufcunt,1);
                }
            }
            if(cout_framer>=100)				//Ã¿¸ölogÀïÃæ×î¶à´æ´¢¶àÉÙ¸ö¼ÇÂ¼s
            {  
                cout_framer=0;
        		lseek(pIEC104_Struct->logsoEfd, 0, SEEK_SET);
                my_debug("file station reset to start!");
            }
            //currpos = lseek(pIEC104_Struct->logsoEfd, 0, SEEK_CUR);
            //my_debug("currpos:%d",currpos);
            free(stime);
            break;
        default:
            break;
    }
            
}

/***************************************************************************/
//º¯Êý:void Send_Soe_Data(int fd)
//ËµÃ÷:´¦ÀíALL_SOEµÄº¯Êý
//ÊäÈë:
//Êä³ö:
//±à¼­:R&N
//Ê±¼ä:2015.9.23
/***************************************************************************/
void Send_Soe_Data(int fd)
{
    unsigned char buffc[11] = {0};
    unsigned long row; 
    unsigned int cuntr=0;
    off_t SOEFile;
    //lseek(fd, 0, SEEK_SET);
    /*
    if(flock(pIEC104_Struct->logsoEfd,LOCK_UN)==0)
    {
      my_debug("file has been unlocked");
    }*/
      if(sys_mod_p->pc_flag & (1<<MSG_ALL_SOE))
        {
              sys_mod_p->pc_flag &=~(1<<MSG_ALL_SOE);
              my_debug("cout_framer:%d",cout_framer);
              int i=0,j=0;
              if(cout_framer>0)
              {
                  row= file_wc(fd);
                  cuntr=(unsigned int)row;
                  cuntr=cuntr+cout_framer;
                  //cuntr = cuntr/2;
                  my_debug("cuntr:%d",cuntr);
                  if(cuntr>=100)
                  {
                       cuntr=100;
                  }
                  SOEFile=lseek(fd, 0, SEEK_SET);
                  //my_debug("cuntr:%d SOEFile:%d",cuntr,SOEFile);
                  for(j=0;j<cuntr;j++)
                  { 
                       //sem_post(&sem_soe104);//ÐÅºÅÁ¿value¼õ1
                       read(fd, buffc,11); //
                       PC_Send_Data(MSG_ALL_SOE, buffc, 11);
                  }
                  //lseek(fd, currpos, SEEK_SET);
                  lseek(fd, (cout_framer*11), SEEK_SET);
                  my_debug("ALLcurrpos:%d",(cout_framer*11));
                  row=0;
              }
              else if(cout_framer==0)
              {     
                    sys_mod_p->pc_flag &=~(1<<MSG_ALL_SOE);
                    row= file_wc(fd);
                    //row=row/2;
                    my_debug("row:%d",row);
                    lseek(fd, 0, SEEK_SET);
                    for(i=0;i<=row;i++)
                    {
                        read(fd, buffc,11); //
                        PC_Send_Data(MSG_ALL_SOE, buffc, 11); 
                    }
                    //lseek(fd, currpos, SEEK_SET);
                    lseek(fd, (cout_framer*11), SEEK_SET);
                    my_debug("ALL0currpos:%d",(cout_framer*11));                    
                    row=0;
                    //sys_mod_p->pc_flag &=~(1<<MSG_ALL_SOE);
                    //lseek(fd, 0, SEEK_SET);
                    //close(fd);
              }
        }
      return 0;
       //fclose(pIEC104_Struct->logsoEfd);
}
unsigned long file_wc(int file)  
{  
     register unsigned char *p;  
     register short gotsp;  
     register int ch, len;  
     register unsigned long linect, charct;  
     unsigned char buf[MAXBSIZE];
     //lseek(file, 0, SEEK_SET);
     if (file) 
     {
        for (gotsp = 1; len = read(file, buf, MAXBSIZE);) 
        {  
             if (len == -1)  
                 return -1;  
             charct += len;
             for (p = buf; len--;) 
             {  
                 ch = *p++;  
                 if (ch == '\n')  
                     ++linect;  
                 if (isspace(ch))  
                     gotsp = 1;  
                 else if (gotsp) 
                    {  
                     gotsp = 0;  
                    }  
            }  
       }  
     }
     my_debug("linect:%d",linect);
     return linect;  
 }

/***************************************************************************/
//º¯Êý:  int  IEC104_Free_Buf(void)
//ËµÃ÷:ÊÍ·Åµô»º³å
//ÊäÈë:ÎÞ
//Êä³ö:ÎÞ
//±à¼­:R&N
//Ê±¼ä:2015.09.18
/***************************************************************************/
 int  IEC104_Free_Buf(void)
{
    uninit_time();    
//	free(pIEC104_Struct->pSendBuf);
	free(pSOERoot);
	free(pYXInfoRoot);
	free(pBufRecvRoot);
    
	if(pIEC104_Struct->logfd !=-1)
		close(pIEC104_Struct->logfd);
    
	if(pIEC104_Struct->logsoEfd!=-1)
		close(pIEC104_Struct->logsoEfd);
    /*
	if(pNand_Struct->nandFile!=-1)
		close(pNand_Struct->nandFile);

    
    if(pIEC104_Struct->logSOEfile!=-1)
		close(pIEC104_Struct->logSOEfile);
    */
	free(pIEC104_Struct);
	if(new_fd != -1)
	{
		close(new_fd);
		new_fd = -1;
	}
	if(sock_fd != -1)
	{
		close(sock_fd);
		sock_fd = -1;
	}
	sem_destroy(&sem_soe104);//Ïú»ÙSOEÐÅºÅÁ¿
//	sem_destroy(&sem_iec104);
	return 0;
}
/***************************************************************************/
//º¯Êý: void *Network_Thread(void * arg)
//ËµÃ÷:ÓÃÓÚÍøÂçÁ¬½Ó£¬¹©iec104¹æÔ¼Ê¹ÓÃ
//ÊäÈë:
//Êä³ö:
//±à¼­:R&N
//Ê±¼ä:2015.4.21
/***************************************************************************/
void  *Network_Thread( void * arg  )
{
	sys_mod_p = (LOCAL_Module *)arg;
	sem_init(&sem_soe104, 0, 0);//´´½¨Ò»¸öÎ´ÃüÃûµÄÐÅºÅÁ¿
	pIEC104_Struct->logfd =Init_Logfile(IEC104LOGFFILE);
    pIEC104_Struct->soEfile = Init_SOEfile(IEC104SOEFILE);
    pIEC104_Struct->logsoEfd = Init_SOEfile(IEC104SOELOG);
    Read_Soe_Count(pIEC104_Struct->soEfile);
	if(pIEC104_Struct->logfd == -1)
	{
		my_debug("Creat logfd fail\n");
		pthread_exit("Network_Thread finished!\n");
	}
    
    if(pIEC104_Struct->logsoEfd== -1)
	{
		my_debug("Creat logfd fail\n");
		pthread_exit("Network_Thread finished!\n");
	}
     
	IEC104_Read_Conf(IEC104CONFFILE, pIEC104_Struct);
	IEC104_Creat_connection();
	return 0;
}


/************************************************************************************/
//º¯Êý:static int  Get_Parp_From_Line(char *line, IEC104Struct   * item)
//ËµÃ÷: ½âÎöÅäÖÃº¯ÊýÖÐµÄÃ¿Ò»ÐÐ£¬½âÎö¹æÔ¼ÅäÖÃÎÄ¼þ
//ÊäÈë:line       Ã¿Ò»ÐÐ
//Êä³ö:: 1·µ»ØÕýÈ·½á¹û       0 ½âÎö½áÊø
/************************************************************************************/
static int  Get_Parp_From_Line(char *line, IEC104Struct   * item)
{
	char * tmp_line= NULL;
	char * token = NULL;

	tmp_line=(char * )strstr((char *)line, "mode=");
	if(tmp_line!=NULL)
	{
		token = strtok( tmp_line, "#");
		item->mode = atoi( token+strlen("mode="));
//		my_debug("item->mode=%d",item->mode);
		return 0;
	}
	tmp_line= NULL;
	tmp_line=strstr((const char *)line, "linkaddr=");
	if(tmp_line!=NULL)
	{
		token = strtok( tmp_line, "#");
		item->linkaddr= atoi(token+strlen("linkaddr="));
//		my_debug("item->linkaddr=%d",item->linkaddr);
		return 0;
	}
	tmp_line= NULL;
	tmp_line=strstr((const char *)line, "linkaddrlen=");
	if(tmp_line!=NULL)
	{
		token = strtok( tmp_line, "#");
		item->linkaddrlen = atoi(token+strlen("linkaddrlen="));
//		my_debug("item->linkaddrlen=%d",item->linkaddrlen);
		return 0;
	}
	tmp_line= NULL;
	tmp_line=strstr((const char *)line, "conaddrlen=");
	if(tmp_line!=NULL)
	{
		token = strtok( tmp_line, "#");
		item->conaddrlen = atoi(token+strlen("conaddrlen="));
//		my_debug("item->conaddrlen=%d",item->conaddrlen);
		return 0;
	}
	tmp_line= NULL;
	tmp_line=strstr((const char *)line, "COTlen=");
	if(tmp_line!=NULL)
	{
		token = strtok( tmp_line, "#");
		item->COTlen = atoi(token+strlen("COTlen="));
//		my_debug("item->conaddrlen=%d",item->conaddrlen);
		return 0;
	}
	tmp_line= NULL;
	tmp_line=strstr((const char *)line, "infoaddlen=");
	if(tmp_line!=NULL)
	{
		token = strtok( tmp_line, "#");
		item->infoaddlen = atoi(token+strlen("infoaddlen="));
//		my_debug("item->infoaddlen=%d",item->infoaddlen);
		return 0;
	}
	tmp_line= NULL;
	tmp_line=strstr((const char *)line, "yctype="); //´«µÝÖµµÃÀàÐÍ
	if(tmp_line!=NULL)
	{
		token = strtok( tmp_line, "#");
		item->yctype = atoi(token+strlen("yctype="));
//		my_debug("item->yctype=%d",item->yctype);
		return 0;
	}
	return 0;
}


/***************************************************************************/
//º¯ÊýËµÃ÷:¶ÁÈ¡104²ÎÊý    ¶ÁÈ¡¹æÔ¼Ïà¹ØµÄÅäÖÃ
//
//ÊäÈë:fd Â·¾¶Ãû³ÆÒÔ¼°ÎÄ¼þitem ²ÎÊýÖ¸Õë
//Êä³ö:ÎÞ
//±à¼­:R&N
//Ê±¼ä:2014.10.08
/***************************************************************************/
static void IEC104_Read_Conf(char * fd,  struct _IEC104Struct   * iec104)
{
 	FILE *fp=NULL;
 	char  line[31];

	 fp = fopen(fd,  "rw");
	 if(fp == NULL){
		my_debug("Fail to open\n");
		pthread_exit("IEC104_Read_Conf fail!\n");
		}
	 fseek(fp, 0, SEEK_SET); 			//ÖØÐÂ¶¨Òåµ½¿ªÊ¼Î»ÖÃ
	 while (fgets(line, 31, fp))
 	{
 		Get_Parp_From_Line(line,  iec104);
 	}
	 fclose(fp);
}

/***************************************************************************/
//º¯Êý: static unsigned char Init_Protocol_IEC104(void)
//ËµÃ÷:ÓÃÓÚ³õÊ¼»¯IEC104
//ÊäÈë:   ÎÞ
//Êä³ö:  ÎÞ
//±à¼­:R&N
//Ê±¼ä:2015.4.22
/***************************************************************************/

static unsigned char IEC104_Init_Protocol(void)
{
	pIEC104_Struct->ucYK_Limit_Time = 40; //40*500ms=20s
	pIEC104_Struct->ucTimeOut_t0 = 30;
	pIEC104_Struct->ucTimeOut_t1 = 15;
	pIEC104_Struct->ucTimeOut_t2 = 10;
	pIEC104_Struct->ucTimeOut_t3 = 20;

	pIEC104_Struct->ucDataSend_Flag = FALSE;
	pIEC104_Struct->ucInterro_First = 0x00;
	pIEC104_Struct->ucYK_TimeCount_Flag = FALSE;
	pIEC104_Struct->ucYK_TimeCount = 0x00;

	pIEC104_Struct->ucSendCountOverturn_Flag = FALSE;
	//pIEC104_Struct->ucRecvCountOverturn_Flag = FALSE;

	pIEC104_Struct->usRecvNum = 0;			//½ÓÊÕÐòÁÐºÅ
	pIEC104_Struct->usSendNum = 0;			//·¢ËÍÐòÁÐºÅ

	pIEC104_Struct->usServRecvNum = 0;
	pIEC104_Struct->usServSendNum = 0;

	pIEC104_Struct->w = 0;
	pIEC104_Struct->k = 0;
	return 0;
}


/***************************************************************************/
//º¯Êý: void *Iec104_Thread(void * arg)
//ËµÃ÷:ÓÃÓÚ´¦Àí´ÓÍøÂç½ÓÊÕµ½µÄÊý¾Ý
//ÊäÈë:
//Êä³ö:
//±à¼­:R&N1110@126.com
//Ê±¼ä:2015.4.21
/***************************************************************************/

void  *Iec104_Thread( void * data  )
{
//	sem_init(&sem_iec104, 0, 0);
	IEC104_Init_Protocol();
	while(1)
	{
		if(new_fd != (-1))
		{
			IEC104_Protocol_Entry();
		}
		else
		{
			usleep(500);
		}
	}
	return 0;
}


/**************************************************************************/
//º¯Êý:static void IEC104_Protocol_Entry(  void  )
//ËµÃ÷:104¹æÔ¼µÄÈë¿Ú
//ÊäÈë:ÎÞ
//Êä³ö:ÎÞ
//±à¼­:R&N
/**************************************************************************/
static void IEC104_Protocol_Entry(  void  )
{
	unsigned char * pTemp_Buf;
	unsigned short  usTemp_Length;
	unsigned char  	ucTemp_ReturnFlag;

	if(pBufRecvGet->usStatus == FULL)//ÊÕµ½Êý¾Ý
	{
	    //my_debug("usStatus:%d\n",pBufRecvGet->usStatus);
		pTemp_Buf = pBufRecvGet->buf;
		usTemp_Length = pBufRecvGet->usLength;
		Log_Frame(pIEC104_Struct->logfd, pTemp_Buf,  0, usTemp_Length, RECV_FRAME);
		//Log_Frame(pIEC104_Struct->logsoEfd, pTemp_Buf,  0, usTemp_Length, RECV_FRAME);
        
		ucTemp_ReturnFlag = IEC104_VerifyDLFrame(pTemp_Buf, usTemp_Length);   //ÑéÖ¤104ÖÐÖ¡ÊÇI  U  S¸ñÊ½
//		my_debug("ucTemp_ReturnFlag:%d\n", ucTemp_ReturnFlag);
		if( (ucTemp_ReturnFlag == IEC104FRAME_I) || (ucTemp_ReturnFlag == IEC104FRAME_U))
		{
			IEC104_ProcessAPDU(pTemp_Buf, usTemp_Length);					//´¦Àí104¹æÔ¼ÖÐµÄÊý¾Ý
			pBufRecvGet->usLength = 0;
			pBufRecvGet->usStatus = EMPTY;
			pBufRecvGet = pBufRecvGet->next;
			return;
		}
		if(ucTemp_ReturnFlag == IECFRAMENEEDACK)
		{
			IEC104_APDU_Send(FORMAT_S,0);
			pBufRecvGet->usLength = 0;
			pBufRecvGet->usStatus = EMPTY;
			pBufRecvGet = pBufRecvGet->next;
			return;
		}
		if(ucTemp_ReturnFlag == IEC104FRAMEERR)
		{
			pBufRecvGet->usLength = 0;
			pBufRecvGet->usStatus = EMPTY;
			pBufRecvGet = pBufRecvGet->next;
			return;
		}
		if( ucTemp_ReturnFlag == IEC104FRAMESEQERR)
		{
			//nTCP_ConnectFlag = 0;
			pBufRecvGet->usLength = 0;
			pBufRecvGet->usStatus = EMPTY;
			pBufRecvGet = pBufRecvGet->next;
			IEC104_Init_Protocol();
			return;
		}

		else
		{
			pBufRecvGet->usLength = 0;
			pBufRecvGet->usStatus = EMPTY;
			pBufRecvGet = pBufRecvGet->next;
		}
	}
	IEC104_UnsoSend(); //ÊÂ¼þÉÏ´«  SOE   COSµÈ
}


/****************************************************************************/
//º¯ÊýËµÃ÷:´¦Àí´øÊ±±êµÄÒ£¿ØÃüÁî   ´øÊ±±êµ¥µãÒ£¿Ø
//ÊäÈë:ASDUÖ¡     ºÍ³¤¶È
//±à¼­:R&N
//Ê±¼ä:2014.10.10
//YK_BISUO				0
//YK_NORMAL_CONTRL      	1
//YK_FUGUI				2
//YK_INVALID_ADDR		3
//YK_CANCEL_SEL			4
//YK_ZHIXIN_FAIL			5
//YK_ZHIXIN_SUCC			6
/****************************************************************************/

static void IEC104_AP_YK(IEC104_ASDU_Frame *pRecv_ASDUFrame,unsigned char ucTemp_Length, unsigned char flag) //´Ë´¦³¤¶ÈÎªASDU³¤¶È
{
	CP56Time2a *pRec_Time;
	time_t timenow,  timerecv;
	struct tm * stime;



	if(pRecv_ASDUFrame->ucCommonAddrL != pIEC104_Struct->linkaddr)
	{
		IEC104_AP_UnknownCommAddr(pRecv_ASDUFrame,ucTemp_Length);
		return;
	}
	if(flag==WITHTIME)
	{
		pRec_Time = (CP56Time2a *)(&pRecv_ASDUFrame->ucInfoUnit[4]);
		time(&timenow);
		stime = localtime(&timenow);
		stime->tm_sec = ((pRec_Time->bMsecH<< 8)  + pRec_Time->bMsecL) / 1000;
		stime->tm_min = pRec_Time->bMinute&0x3F;
		stime->tm_hour = pRec_Time->bHour&0x1F;
		stime->tm_mday = pRec_Time->bDay&0x1F;
		stime->tm_mon = (pRec_Time->bMonth&0x1F)-1; //9-1
		stime->tm_year =(pRec_Time->bYear&0x7F)+100;//2014-1900

		timerecv= mktime(stime);
		if(((timenow>timerecv)&&(timenow-timerecv>15))||((timerecv>timenow)&&(timerecv-timenow>15)))//Èç¹û³¬¹ý15Ãë¾Í²»ÄÜ·ÖºÏÕ¢
		{
			IEC104_AP_NegativeCON(pRecv_ASDUFrame , ucTemp_Length);
			return;
		}
	}

	if(pRecv_ASDUFrame->ucInfoUnit[3] & 0x80) //Ò£¿ØÑ¡Ôñ
	{
		if(pRecv_ASDUFrame->ucCauseL == IEC104_CAUSE_ACT) //¼¤»î
		{
			switch(IEC104_AP_YKSel(pRecv_ASDUFrame))
			{
				case YK_NORMAL_CONTRL: 			//Ò»°ãÒ£¿ØÑ¡Ôñ
					IEC104_AP_PositiveCON(pRecv_ASDUFrame , ucTemp_Length);
					break;
				case YK_INVALID_ADDR: 			//ÐÅÏ¢ÌåµØÖ·ÓÐÎó
				    my_debug("YK_INVALID_ADDR is wrong");
					IEC104_AP_UnknownInfoAddr(pRecv_ASDUFrame,ucTemp_Length);
					break;
			}
		}
		else if(pRecv_ASDUFrame->ucCauseL == IEC104_CAUSE_DEACT) //Í£Ö¹¼¤»îÈ·ÈÏ
		{
			switch(IEC104_AP_YKSelHalt(pRecv_ASDUFrame))
			{
				case YK_CANCEL_SEL:			 //Ò»°ãÒ£¿ØÑ¡Ôñ³·Ïú
					IEC104_AP_NegativeCON(pRecv_ASDUFrame , ucTemp_Length);
					break;
				case YK_INVALID_ADDR:		 //ÐÅÏ¢ÌåµØÖ·ÓÐÎó
					IEC104_AP_UnknownInfoAddr(pRecv_ASDUFrame,ucTemp_Length);
					break;
			}
		}
		else
		{
			IEC104_AP_UnknownCause(pRecv_ASDUFrame,ucTemp_Length);
			return;
		}
	}
	else    //Ò£¿ØÖ´ÐÐ
	{
		switch(IEC104_AP_YKExec(pRecv_ASDUFrame, ucTemp_Length))
		{
		case YK_ZHIXIN_FAIL:		//Ö´ÐÐÊ§°Ü
		    my_debug("failure");
			IEC104_AP_NegativeCON(pRecv_ASDUFrame , ucTemp_Length);
			break;
		case YK_INVALID_ADDR:
			IEC104_AP_UnknownInfoAddr(pRecv_ASDUFrame,ucTemp_Length);
			break;
		case YK_ZHIXIN_SUCC:		//Ö´ÐÐ³É¹¦
		    my_debug("success");
			IEC104_AP_SUCC(pRecv_ASDUFrame , ucTemp_Length);
			break;
		}
	}
}

/****************************************************************************/
//º¯ÊýËµÃ÷:Ò£¿ØÖ´ÐÐ
//ÊäÈë:ASDUÖ¡     ºÍ³¤¶È
//±à¼­:R&N
//Ê±¼ä:2014.10.10
//YK_ZHIXIN_FAIL			5
//YK_ZHIXIN_SUCC		6
/****************************************************************************/
static unsigned char IEC104_AP_YKExec(IEC104_ASDU_Frame * pRecv_ASDUFrame, unsigned char ucTemp_Length)
{
	unsigned short sPointNo, i;
	Point_Info   *  Point_Info;

	if(sys_mod_p->usYK_Num ==0)
		return YK_INVALID_ADDR;

	sPointNo = (pRecv_ASDUFrame->ucInfoUnit[1]<<8) + pRecv_ASDUFrame->ucInfoUnit[0];
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

    /*
	if((pRecv_ASDUFrame->ucInfoUnit[3]&0x01)==0X01)   //ÕâÀïÌí¼ÓÒ£¿Ø·ÖÕ¢
	{
	    if(yk_select_flag&(1<<(Point_Info->uiOffset<<1)))
//		if(Point_Info->usFlag &(1<<8))			           //ÒÑ¾­Ñ¡ÖÐ
		{
		   //Ìí¼ÓºÏÕ¢¶¯×÷
			Send_Event_By_Message(MSG_YK, Point_Info->uiOffset, YK_CLOSE_EXECUTIVE);
            g_YK_State_p->result &=~ (1<<Point_Info->uiOffset);//³É¹¦ÁË¹²ÏíÇøÖÃÎ»
            //sleep(2);
            if(g_YK_State_p->result&(1<<Point_Info->uiOffset))//¶ÁÈ¡¹²ÏíÇøµÄ×´Ì¬
            {
//			  Point_Info->usFlag &=~(1<<8); 		   //ºÏÕ¢¶¯×÷,²¢ÇÒ½â³ýÑ¡ÖÐ
              yk_select_flag &=~(1<<(Point_Info->uiOffset<<1));
              return YK_ZHIXIN_SUCC;
            }
            return  YK_ZHIXIN_FAIL;
		}
	}
	else
	{
		if(yk_select_flag&(2<<(Point_Info->uiOffset<<1)))			           //ÒÑ¾­Ñ¡ÖÐ
		{
				//Ìí¼Ó·ÖÕ¢¶¯×÷
			Send_Event_By_Message(MSG_YK, Point_Info->uiOffset, YK_OPEN_EXECUTIVE);
            g_YK_State_p->result &=~ (1<<Point_Info->uiOffset);//³É¹¦ÁË¹²ÏíÇøÖÃÎ»
            //sleep(2);
            if(g_YK_State_p->result&(1<<Point_Info->uiOffset))//¶ÁÈ¡¹²ÏíÇøµÄ×´Ì¬
            {
  	      	  yk_select_flag &=~(2<<(Point_Info->uiOffset<<1)); 		   //ºÏÕ¢¶¯×÷,²¢ÇÒ½â³ýÑ¡ÖÐ
              return YK_ZHIXIN_SUCC;
            }
            return  YK_ZHIXIN_FAIL;
		}
        else
            return  YK_ZHIXIN_FAIL;
	}
 return YK_INVALID_ADDR;
    */
	if((pRecv_ASDUFrame->ucInfoUnit[3]&0x01)==0X01)   //ÕâÀïÌí¼ÓÒ£¿Ø·ÖÕ¢
	{
		if(Point_Info->usFlag &(1<<8))			           //ÒÑ¾­Ñ¡ÖÐ
		{
				Point_Info->usFlag &=~(1<<8); 		   //ºÏÕ¢¶¯×÷,²¢ÇÒ½â³ýÑ¡ÖÐ
				//Ìí¼ÓºÏÕ¢¶¯×÷
				Send_Event_By_Message(MSG_YK, Point_Info->uiOffset, YK_CLOSE_EXECUTIVE);
		}
	}
	else
	{
		if(Point_Info->usFlag &(1<<9))			           //ÒÑ¾­Ñ¡ÖÐ
		{
				Point_Info->usFlag &=~(1<<9); 		   //·ÖÕ¢¶¯×÷,²¢ÇÒ½â³ýÑ¡ÖÐ
				//Ìí¼Ó·ÖÕ¢¶¯×÷
				Send_Event_By_Message(MSG_YK, Point_Info->uiOffset, YK_OPEN_EXECUTIVE);
		}
	}
	if(1)											   //¶¯×÷³É¹¦ ?    Ö´ÐÐµÄÊ±ºò±ØÐëÅÐ¶ÏÑ¡ÖÐÃ»ÓÐ
		return YK_ZHIXIN_SUCC;
	else
		return YK_ZHIXIN_FAIL;
		
}

/****************************************************************************/
//º¯ÊýËµÃ÷:Ò£¿ØÑ¡ÖÐ³·Ïú
//0:±ÕËø   1:Õý³£    2:¸´¹é    3:Ã»ÓÐÕâ¸öµØÖ·
//YK_BISUO				0
//YK_NORMAL_CONTRL      	1
//YK_FUGUI				2
//YK_INVALID_ADDR		3
//YK_CANCEL_SEL			4
//±à¼­:R&N
//Ê±¼ä:2014.10.10
/****************************************************************************/
static unsigned char IEC104_AP_YKSelHalt(IEC104_ASDU_Frame * pRecv_ASDUFrame)
{
	unsigned short sPointNo, i;
	Point_Info   *  Point_Info;

	if(sys_mod_p->usYK_Num ==0)
		return YK_INVALID_ADDR;

	sPointNo = (pRecv_ASDUFrame->ucInfoUnit[1]<<8) + pRecv_ASDUFrame->ucInfoUnit[0];
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
/*
	if((pRecv_ASDUFrame->ucInfoUnit[3]&0x01)==0X01)//ÕâÀïÌí¼ÓÒ£¿Ø·ÖÕ¢Ñ¡ÖÐÈ¡Ïû  »¹ÊÇÒ£¿ØºÏÕ¢Ñ¡ÖÐÈ¡Ïû
	{

        Send_Event_By_Message(MSG_YK, Point_Info->uiOffset, YK_CANCEL_OPEN_SEL);
        g_YK_State_p->result &=~ (1<<Point_Info->uiOffset);//³É¹¦ÁË¹²ÏíÇøÖÃÎ»
        //sleep(2);
        if(g_YK_State_p->result&(1<<Point_Info->uiOffset))//¶ÁÈ¡¹²ÏíÇøµÄ×´Ì¬
        {
          yk_select_flag &=~(1<<(Point_Info->uiOffset<<1));//ºÏÕ¢Ô¤ÖÃ³É¹¦
          return YK_CANCEL_SEL;
        }
        return  YK_SEL_FAIL;
    }
	else
    {
        Send_Event_By_Message(MSG_YK, Point_Info->uiOffset, YK_CANCEL_CLOSE_SEL);
        g_YK_State_p->result &=~ (1<<Point_Info->uiOffset);//³É¹¦ÁË¹²ÏíÇøÖÃÎ»
        //sleep(2);
        if(g_YK_State_p->result&(1<<Point_Info->uiOffset))//¶ÁÈ¡¹²ÏíÇøµÄ×´Ì¬
        {
          yk_select_flag &=~(2<<(Point_Info->uiOffset<<1));;

          return YK_CANCEL_SEL;
        }
        return  YK_SEL_FAIL;
    }
	return YK_CANCEL_SEL;
*/
	if((pRecv_ASDUFrame->ucInfoUnit[3]&0x01)==0X01)//ÕâÀïÌí¼ÓÒ£¿Ø·ÖÕ¢Ñ¡ÖÐÈ¡Ïû  »¹ÊÇÒ£¿ØºÏÕ¢Ñ¡ÖÐÈ¡Ïû
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
//º¯ÊýËµÃ÷:Ò£¿ØÑ¡ÖÐ £¬Ò£¿Ø¹ý³ÌÖÐÏÈÑ¡Ôñ²Å²Ù×÷
//0:±ÕËø   1:Õý³£    2:¸´¹é    3:Ã»ÓÐÕâ¸öµØÖ·
//YK_BISUO				0
//YK_NORMAL_CONTRL      	1
//YK_FUGUI				2
//YK_INVALID_ADDR		3
//±à¼­:R&N
//Ê±¼ä:2014.10.10
/****************************************************************************/
static unsigned char IEC104_AP_YKSel(IEC104_ASDU_Frame * pRecv_ASDUFrame)
{
	unsigned short sPointNo, i;
	Point_Info   *  Point_Info;

	if(sys_mod_p->usYK_Num ==0)
		return YK_INVALID_ADDR;

	sPointNo = (pRecv_ASDUFrame->ucInfoUnit[1]<<8) + pRecv_ASDUFrame->ucInfoUnit[0];
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
    /*
    if((pRecv_ASDUFrame->ucInfoUnit[3]&0x01)==0X01)//ÕâÀïÌí¼ÓÒ£¿Ø·ÖÕ¢Ñ¡ÖÐ  »¹ÊÇÒ£¿ØºÏÕ¢Ñ¡ÖÐ
	{
        Send_Event_By_Message(MSG_YK, Point_Info->uiOffset, YK_CLOSE_SEL);//×î¸ßÎ»ÊÇ1ÊÇ±íÊ¾Ñ¡ÖÐ£¬ÊÇ0±íÊ¾Ö´ÐÐ
        g_YK_State_p->result &=~ (1<<Point_Info->uiOffset);//³É¹¦ÁË¹²ÏíÇøÖÃÎ»
        //sleep(2);
        if(g_YK_State_p->result&(1<<Point_Info->uiOffset))//¶ÁÈ¡¹²ÏíÇøµÄ×´Ì¬
        {

          yk_select_flag |= (1<<(Point_Info->uiOffset<<1));//ºÏÕ¢Ô¤ÖÃ³É¹¦
          return YK_NORMAL_CONTRL;
        }
        return  YK_SEL_FAIL;
    }
	else
    {
        Send_Event_By_Message(MSG_YK, Point_Info->uiOffset, YK_OPEN_SEL);
        g_YK_State_p->result &=~ (1<<Point_Info->uiOffset);//³É¹¦ÁË¹²ÏíÇøÖÃÎ»
        //sleep(2);
        if(g_YK_State_p->result&(1<<Point_Info->uiOffset))//¶ÁÈ¡¹²ÏíÇøµÄ×´Ì¬
        {
          yk_select_flag |= (2<<(Point_Info->uiOffset<<1));//·ÖÕ¢Ô¤ÖÃ³É¹¦
          return YK_NORMAL_CONTRL;
        }
        return  YK_SEL_FAIL;
    }
    */
	if((pRecv_ASDUFrame->ucInfoUnit[3]&0x01)==0X01)//ÕâÀïÌí¼ÓÒ£¿Ø·ÖÕ¢Ñ¡ÖÐ  »¹ÊÇÒ£¿ØºÏÕ¢Ñ¡ÖÐ
	{
		Point_Info->usFlag |=(1<<8);
        Send_Event_By_Message(MSG_YK, Point_Info->uiOffset, YK_OPEN_SEL);//×î¸ßÎ»ÊÇ1ÊÇ±íÊ¾Ñ¡ÖÐ£¬ÊÇ0±íÊ¾Ö´ÐÐ

    }
	else
    {
		Point_Info->usFlag |=(1<<9);
        Send_Event_By_Message(MSG_YK, Point_Info->uiOffset, YK_CLOSE_SEL);
    }
	return YK_NORMAL_CONTRL;
}

/*************************************************************************/
//º¯ÊýËµÃ÷:¶ÔÓÚÒ£¿Ø½øÐÐÑ¡ÖÐ»Ø¸´
//ÊäÈë:ASDUÖ¡      ºÍ³¤¶È
//Êä³ö:ÎÞ
//±à¼­:R&N
//Ê±¼ä:2014.10.10
/*************************************************************************/
static void IEC104_AP_PositiveCON(IEC104_ASDU_Frame *pRecv_ASDUFrame,unsigned char ucTemp_Length)
{
	unsigned char i;
	IEC104_ASDU_Frame *pTemp_ASDUFrame;

	pTemp_ASDUFrame = (IEC104_ASDU_Frame *)(pIEC104_Struct->pSendBuf+6);

	pTemp_ASDUFrame->ucType = pRecv_ASDUFrame->ucType;
	pTemp_ASDUFrame->ucRep_Num = pRecv_ASDUFrame->ucRep_Num;
	pTemp_ASDUFrame->ucCauseL = IEC104_CAUSE_ACTCON;
	pTemp_ASDUFrame->ucCauseH = pRecv_ASDUFrame->ucCauseH;
	pTemp_ASDUFrame->ucCommonAddrL = pRecv_ASDUFrame->ucCommonAddrL;
	pTemp_ASDUFrame->ucCommonAddrH = pRecv_ASDUFrame->ucCommonAddrH;

	for(i=0;i<ucTemp_Length-6;i++)
	{
		pTemp_ASDUFrame->ucInfoUnit[i] = pRecv_ASDUFrame->ucInfoUnit[i];
	}
    my_debug("reply");
	IEC104_APDU_Send(FORMAT_I,ucTemp_Length);
}

/***********************************************************************************/
//º¯ÊýËµÃ÷:µ¥µãÒ£¿Ø³¬Ê±
//ÊäÈë:ÊÕµ½µÄASDUÖ¡    ºÍ ³¤¶È
//±à¼­:R&N
//Ê±¼ä:2014.10.10
/***********************************************************************************/
static void IEC104_AP_NegativeCON(IEC104_ASDU_Frame *pRecv_ASDUFrame,unsigned char ucTemp_Length)
{
	unsigned char i;
	IEC104_ASDU_Frame *pTemp_ASDUFrame;

	pTemp_ASDUFrame = (IEC104_ASDU_Frame *)(pIEC104_Struct->pSendBuf+6);

	pTemp_ASDUFrame->ucType = pRecv_ASDUFrame->ucType;
	pTemp_ASDUFrame->ucRep_Num = pRecv_ASDUFrame->ucRep_Num;
	pTemp_ASDUFrame->ucCauseL = IEC104_CAUSE_DEACTCON;
	pTemp_ASDUFrame->ucCauseH = pRecv_ASDUFrame->ucCauseH;
	pTemp_ASDUFrame->ucCommonAddrL = pRecv_ASDUFrame->ucCommonAddrL;
	pTemp_ASDUFrame->ucCommonAddrH = pRecv_ASDUFrame->ucCommonAddrH;

	for(i=0;i<ucTemp_Length-6;i++)
	{
		pTemp_ASDUFrame->ucInfoUnit[i] = pRecv_ASDUFrame->ucInfoUnit[i];
	}
	IEC104_APDU_Send(FORMAT_I, ucTemp_Length);
}

/**************************************************************************/
//º¯ÊýËµÃ÷:Ö´ÐÐ³É¹¦º¯Êý
//ÊäÈë:ASDUÖ¡ºÍ³¤¶È
//Êä³öå:ÎÞ
//±à¼­:R&N
//Ê±¼ä:2015.04.29
/**************************************************************************/
static void IEC104_AP_SUCC(IEC104_ASDU_Frame *pRecv_ASDUFrame,unsigned char ucTemp_Length)
{
	unsigned char i;
	IEC104_ASDU_Frame *pTemp_ASDUFrame;

	pTemp_ASDUFrame = (IEC104_ASDU_Frame *)(pIEC104_Struct->pSendBuf+6);

	pTemp_ASDUFrame->ucType = pRecv_ASDUFrame->ucType;
	pTemp_ASDUFrame->ucRep_Num = pRecv_ASDUFrame->ucRep_Num;
	pTemp_ASDUFrame->ucCauseL = IEC104_CAUSE_ACTTERM;
	pTemp_ASDUFrame->ucCauseH = pRecv_ASDUFrame->ucCauseH;
	pTemp_ASDUFrame->ucCommonAddrL = pRecv_ASDUFrame->ucCommonAddrL;
	pTemp_ASDUFrame->ucCommonAddrH = pRecv_ASDUFrame->ucCommonAddrH;

	for(i=0;i<ucTemp_Length-6;i++)
	{
		pTemp_ASDUFrame->ucInfoUnit[i] = pRecv_ASDUFrame->ucInfoUnit[i];
	}
	IEC104_APDU_Send(FORMAT_I, ucTemp_Length);
}

/**************************************************************************/
//º¯ÊýËµÃ÷:´¦ÀíÎ´ÖªÐÅÏ¢µØÖ·
//ÊäÈë:ASDUÖ¡ºÍ³¤¶È
//Êä³öå:ÎÞ
//±à¼­:R&N
/**************************************************************************/
static void IEC104_AP_UnknownInfoAddr(IEC104_ASDU_Frame *pRecv_ASDUFrame,unsigned char ucTemp_Length)
{
	unsigned char i;
	IEC104_ASDU_Frame *pTemp_ASDUFrame;

	pTemp_ASDUFrame = (IEC104_ASDU_Frame *)(pIEC104_Struct->pSendBuf+6);

	pTemp_ASDUFrame->ucType = pRecv_ASDUFrame->ucType;
	pTemp_ASDUFrame->ucRep_Num = pRecv_ASDUFrame->ucRep_Num;
	pTemp_ASDUFrame->ucCauseL = IEC104_CAUSE_N_BIT | IEC104_CAUSE_UNKNOWNINFOADDR;
	pTemp_ASDUFrame->ucCauseH = pRecv_ASDUFrame->ucCauseH;
	pTemp_ASDUFrame->ucCommonAddrL = pRecv_ASDUFrame->ucCommonAddrL;
	pTemp_ASDUFrame->ucCommonAddrH = pRecv_ASDUFrame->ucCommonAddrH;

	for(i=0;i<ucTemp_Length-6;i++)
	{
		pTemp_ASDUFrame->ucInfoUnit[i] = pRecv_ASDUFrame->ucInfoUnit[i];
	}
	IEC104_APDU_Send(FORMAT_I,ucTemp_Length);
}

/**************************************************************************/
//º¯ÊýËµÃ÷:ÓÃÓÚ´¦ÀíAPDUÊý¾Ý
//pTemp_Buf	 Ö¡»º³å
//ucTemp_Length		Ö¡³¤¶È
//Êä³ö:ÎÞ
//±à¼­:R&N
/**************************************************************************/
//´Ë´¦³¤¶ÈÎªÁ´Â·²ã³¤¶È£¬²»ÊÇAPDU³¤¶È
static void IEC104_ProcessAPDU(unsigned char * pTemp_Buf, unsigned char ucTemp_Length)
{
	IEC104_ASDU_Frame *pTemp_ASDUFrame;
  	unsigned char ucTemp_Type;

  	unsigned short usTemp_ServSendNum;
	unsigned short usTemp_ServRecvNum;


	usTemp_ServSendNum = (pTemp_Buf[3]<<8)|pTemp_Buf[2];
	usTemp_ServRecvNum = (pTemp_Buf[5]<<8)|pTemp_Buf[4];

	if( !(usTemp_ServSendNum & 0x0001) )  //FORMAT_I
	{
		my_debug("FORMAT_I");
		pTemp_ASDUFrame=(IEC104_ASDU_Frame *)(pTemp_Buf+6);
		ucTemp_Type=pTemp_ASDUFrame->ucType;
		switch(ucTemp_Type)
		{
		case IEC104_C_RD:	//¶ÁÃüÁî
//			IEC104_AP_RD(pTemp_ASDUFrame,ucTemp_Length-6);
			break;

		case IEC104_C_SC_NA_1://²»´øÊ±±êµÄµ¥µãÃüÁî
			IEC104_AP_YK(pTemp_ASDUFrame,ucTemp_Length-6, NOTIME);
			break;
		case IEC104_C_SC_TA_1://´øÊ±±êµÄµ¥µãÃüÁî
			IEC104_AP_YK(pTemp_ASDUFrame,ucTemp_Length-6, WITHTIME);
			break;

		case IEC104_C_IC_NA_1://×ÜÕÙ»½
			IEC104_AP_IC(pTemp_ASDUFrame,ucTemp_Length-6);
			break;

		case IEC104_C_CS_NA_1://Ê±ÖÓÍ¬²½
			IEC104_AP_CS(pTemp_Buf,ucTemp_Length);
			break;

		case IEC104_C_RP_NA_1://¸´Î»½ø³Ì
			IEC104_AP_RP(pTemp_ASDUFrame,ucTemp_Length-6);
			break;

		default:
			IEC104_AP_UnknownType(pTemp_ASDUFrame,ucTemp_Length-6);
			break;
		}
	}
	else if( (usTemp_ServSendNum & 0x0002) != 0 ) //FORMAT_U
	{
		if(usTemp_ServSendNum & 0x04)
		{
			IEC104_APDU_Send(FORMAT_U,0x0b);
			pIEC104_Struct->ucDataSend_Flag = TRUE;  //ÊÇ·ñÔÊÐí·¢ËÍµÄ±ê¼Ç
		}
		if(usTemp_ServSendNum & 0x10)
		{
			pIEC104_Struct->ucDataSend_Flag = TRUE;  //false
			IEC104_APDU_Send(FORMAT_U,0x23);
		}

		if(usTemp_ServSendNum & 0x40)					//Test Frame
		{
		    pIEC104_Struct->ucDataSend_Flag = TRUE;
			IEC104_APDU_Send(FORMAT_U,0x83);
		}
	}
	else   //FORMAT_S
	{
		my_debug("FORMAT_S");
		return; // FORMAT_S±¨ÎÄÔÚÓ¦ÓÃ²ãÉÏ²»ÐèÒª´¦Àí
	}
}

/**************************************************************************/
//ÕÙ»½Ó¦ÓÃ²ã´¦Àíº¯Êý£¬°üÀ¨×ÜÕÙ»½¡¢·Ö×éÕÙ»½
//ÊäÈë:ASDUÊý¾ÝºÍ³¤¶È
//Êä³ö:ÎÞ
//±à¼­:R&N
/**************************************************************************/
static void IEC104_AP_IC(IEC104_ASDU_Frame *pRecv_ASDUFrame , unsigned char ucTemp_Length) //´Ë´¦³¤¶ÈÎªASDU³¤¶È
{
	unsigned char ucTemp_GroupNo;

	if(pRecv_ASDUFrame->ucCommonAddrL != pIEC104_Struct->linkaddr)
	{
		IEC104_AP_UnknownCommAddr(pRecv_ASDUFrame,ucTemp_Length);
		return;
	}

	//×ÜÕÙ»½Îª¼¤»î£¬·Ö×éÕÙ»½ÎªÇëÇó£¬»¹ÓÐÍ£Ö¹¼¤»î£¬ÔÝÊ±²»¿¼ÂÇ
	if((pRecv_ASDUFrame->ucCauseL != IEC104_CAUSE_ACT) && (pRecv_ASDUFrame->ucCauseL != IEC104_CAUSE_REQ))
	{
		IEC104_AP_UnknownCause(pRecv_ASDUFrame,ucTemp_Length);
		return;
	}

	ucTemp_GroupNo = pRecv_ASDUFrame->ucInfoUnit[3];
	if(ucTemp_GroupNo == 20)  //×ÜÕÙ»½
	{
		pIEC104_Struct->ucInterro_First = 0x01; //»Ø´ð×ÜÕÙ»½È·ÈÏ±êÖ¾
		IEC104_AP_RQALL_CON(pRecv_ASDUFrame);
		return;
	}
//	else if(ucTemp_GroupNo >= 21 && ucTemp_GroupNo <= 28 )	// Ò£ÐÅ×é 21~28
//	{
//		pIEC104_Struct->ucDI_Group_No[rtu_id] = ucTemp_GroupNo;
//		pIEC104_Struct->usDI_Group_Seq[rtu_id] = (ucTemp_GroupNo - 21) * 0x80;
//
//		IEC104_AP_GROUP_DI(rtu_no,ucTemp_GroupNo);
//		return;
//	}
//	else if( ucTemp_GroupNo >= 29 && ucTemp_GroupNo <= 32 )		// Ò£²â×é 29~32
//	{
//		pIEC104_Struct->ucAI_Group_No[rtu_id] = ucTemp_GroupNo;
//		pIEC104_Struct->usAI_Group_Seq[rtu_id] = (ucTemp_GroupNo - 29) * 0x80;
//
//		IEC104_AP_GROUP_AI(rtu_no,ucTemp_GroupNo);
//		return;
//	}
	else
	{
		IEC104_AP_UnknownInfoAddr(pRecv_ASDUFrame,ucTemp_Length);
		return;
	}
}
/**************************************************************************/
//º¯ÊýÃèÊö:×ÜÕÙ»½µÄ»Ø¸´º¯Êý
//ÊäÈë:ASDUÖ¡
//Êä³ö:ÎÞ
//±à¼­:R&N
/**************************************************************************/

static void IEC104_AP_RQALL_CON(IEC104_ASDU_Frame *pRecv_ASDUFrame)
{

	IEC104_ASDU_Frame *pTemp_ASDUFrame;
	unsigned char * pTemp_InfoPointer;

	pTemp_ASDUFrame = (IEC104_ASDU_Frame *)(pIEC104_Struct->pSendBuf+6);
	pTemp_InfoPointer = pTemp_ASDUFrame->ucInfoUnit;

	pTemp_ASDUFrame->ucType = pRecv_ASDUFrame->ucType;
	pTemp_ASDUFrame->ucRep_Num = 0x01;  //Êý¾Ý¸öÊý³õÖµ
	pTemp_ASDUFrame->ucCauseH = 0x00;
	pTemp_ASDUFrame->ucCommonAddrL = pRecv_ASDUFrame->ucCommonAddrL;
	pTemp_ASDUFrame->ucCommonAddrH = pRecv_ASDUFrame->ucCommonAddrH;
	*pTemp_InfoPointer++ = pRecv_ASDUFrame->ucInfoUnit[0];
	*pTemp_InfoPointer++ = pRecv_ASDUFrame->ucInfoUnit[1];
	*pTemp_InfoPointer++ = pRecv_ASDUFrame->ucInfoUnit[2];
	*pTemp_InfoPointer = pRecv_ASDUFrame->ucInfoUnit[3];

	pTemp_ASDUFrame->ucCauseL=IEC104_CAUSE_ACTCON;  //07-¼¤»îÈ·ÈÏ

	IEC104_APDU_Send(FORMAT_I,10);
}

/**************************************************************************/
//º¯ÊýÃèÊö:Ê±ÖÓÍ¬²½ÃüÁî
//ÊäÈë:²»ÊÇASDUÖ¡ºÍ³¤¶È£¬ ÊÇ104Ð£Ê±ÃüÁîµÄÖ¡ºÍ³¤¶È
//Êä³ö:ÎÞ
//±à¼­:R&N
/**************************************************************************/
static void IEC104_AP_CS(unsigned char *pTemp_Buf,unsigned char ucTemp_Length)
{
	unsigned char i;
	unsigned short  usMSec;
	long  usUSec;				//Î¢Ãë¾«¶È
	unsigned char  ucSec,ucMin,ucHour,ucDay,ucMonth,ucYear;
	unsigned char *buf;
   	IEC104_ASDU_Frame *pTemp_ASDUFrame;
   	IEC104_ASDU_Frame *pRecv_ASDUFrame;
	struct rtc_time  * stime=malloc(sizeof(struct rtc_time));

	pTemp_ASDUFrame = (IEC104_ASDU_Frame *)(pIEC104_Struct->pSendBuf+6);
	pRecv_ASDUFrame=(IEC104_ASDU_Frame *)(pTemp_Buf+6);

	buf=(unsigned char*)(pTemp_Buf+15);
	usMSec = buf[1]*256+buf[0];

	ucSec = (unsigned char)(usMSec/1000);
	if(ucSec>59)
		return;
	usUSec = usMSec%1000; //Õâ¸öÊÇms
	usUSec = usUSec *1000; //Õâ¸öÊÇus
	ucMin = buf[2]&0x3f;
	if(ucMin>59)
		return;

	ucHour = buf[3]&0x1f;
	if(ucHour>23)
		return;

	ucDay = buf[4]&0x1f;
	if(ucDay>31)
		return;

	ucMonth = buf[5]&0x0f;
	if(ucMonth>12)
		return;

	ucYear = buf[6]&0x7f;
	if(ucYear>99)
		return;

	stime->tm_mday =ucDay;
	stime->tm_mon = ucMonth;
	stime->tm_year = ucYear;
	stime->tm_hour = ucHour;
	stime->tm_min = ucMin;
	stime->tm_sec = ucSec;
	my_debug("%d-%d-%d %d:%d:%d:%ld", ucYear, ucMonth, ucDay, ucHour, ucMin, ucSec, usUSec);
	Set_Rtc_Time(stime,  usUSec);	//ÉèÖÃRTCÊ±¼ä

	memset(stime, 0, sizeof(struct rtc_time));
	Get_Rtc_Time(stime);
//		my_debug("system time: %d-%02d-%02d %02d:%02d:%02d",stime->tm_year + 1900, stime->tm_mon + 1,
//			stime->tm_mday,stime->tm_hour,stime->tm_min,stime->tm_sec);

	free(stime);

    if(pRecv_ASDUFrame->ucCauseL == IEC104_CAUSE_ACT)  //??
	{
		pTemp_ASDUFrame->ucType = pRecv_ASDUFrame->ucType;
		pTemp_ASDUFrame->ucRep_Num = 0x01;  //??????
		pTemp_ASDUFrame->ucCauseH = 0x00;
		pTemp_ASDUFrame->ucCommonAddrL = pIEC104_Struct->linkaddr;
		pTemp_ASDUFrame->ucCommonAddrH = pRecv_ASDUFrame->ucCommonAddrH;

	      for(i=0;i<12;i++)
		{
			pTemp_ASDUFrame->ucInfoUnit[i] = pRecv_ASDUFrame->ucInfoUnit[i];
		}
			pTemp_ASDUFrame->ucCauseL = IEC104_CAUSE_ACTCON;;

			IEC104_APDU_Send(FORMAT_I, 16);
			return;
	}

}


/**************************************************************************/
//º¯ÊýÃèÊö:×ÜÕÙ»½´¦Àí½áÊø
//ÊäÈë:ÎÞ
//Êä³ö:ÎÞ
//±à¼­:R&N
/**************************************************************************/
static void IEC104_AP_INTERROALL_END( void )
{
	unsigned char len;

	IEC104_ASDU_Frame *pTemp_ASDUFrame;
	pTemp_ASDUFrame = (IEC104_ASDU_Frame *)(pIEC104_Struct->pSendBuf+6);

	pTemp_ASDUFrame->ucType = IEC104_C_IC_NA_1;
	pTemp_ASDUFrame->ucRep_Num = 0x1;
	pTemp_ASDUFrame->ucCauseL = IEC104_CAUSE_ACTTERM; 	//0x0a-¼¤»î½áÊø
	pTemp_ASDUFrame->ucCauseH = 0x00;

	pTemp_ASDUFrame->ucCommonAddrL = pIEC104_Struct->linkaddr;
	pTemp_ASDUFrame->ucCommonAddrH = 0x00;

	len = 0;
	pTemp_ASDUFrame->ucInfoUnit[len++]=0x00;
	pTemp_ASDUFrame->ucInfoUnit[len++]=0x00;
	pTemp_ASDUFrame->ucInfoUnit[len++]=0x00;

	pTemp_ASDUFrame->ucInfoUnit[len++]=IEC104_QOI_INTROGEN;

	IEC104_APDU_Send(FORMAT_I, len+6);					//len+6ÎªFORMAT_IµÄASDU³¤¶È
}
/**************************************************************************/
//º¯ÊýÃèÊö:±¨¸æËùÓÐÒ£²âÊý¾Ý,·¢ËÍ¸¡µãÊý
//ÊäÈë:ÎÞ
//Êä³ö:ÎÞ
//±à¼­:R&N
/**************************************************************************/
static unsigned char IEC104_AP_ALL_YC( void )// ±¨¸æÈ«²¿Ò£²âÊý¾Ý
{
	short len,j, k, send_over=0;
	IEC104_ASDU_Frame *pTemp_ASDUFrame;
	Point_Info   * pTmp_Point=NULL;
	float data=0;

	pTemp_ASDUFrame = (IEC104_ASDU_Frame *)(pIEC104_Struct->pSendBuf+6);

	pTemp_ASDUFrame->ucType = IEC104_M_ME_NC_1;
	if(sys_mod_p->Continuity & YC_NOTCONTINUE) 			 		//µØÖ·²»Á¬Ðø
		pTemp_ASDUFrame->ucRep_Num = 0;
	else
		pTemp_ASDUFrame->ucRep_Num = IEC104_VSQ_SQ_BIT;

	pTemp_ASDUFrame->ucCauseL = IEC104_CAUSE_INTROGEN;
	pTemp_ASDUFrame->ucCauseH = 0x00;
	pTemp_ASDUFrame->ucCommonAddrL = pIEC104_Struct->linkaddr;    //Õ¾µØÖ·
	pTemp_ASDUFrame->ucCommonAddrH = 0x00;

	len=0;
	pTmp_Point = sys_mod_p->pYC_Addr_List_Head;

	for(k=0; k<(sys_mod_p->usYC_Num-send_over); )		//·¢ËÍ¶àÉÙ´Î
	{
		len = 0;
		if(!(sys_mod_p->Continuity & YC_NOTCONTINUE))
		{
			pTemp_ASDUFrame->ucInfoUnit[len++] = (unsigned char)(pTmp_Point->uiAddr&0xff);
			pTemp_ASDUFrame->ucInfoUnit[len++] = (unsigned char)(pTmp_Point->uiAddr>>8);
			pTemp_ASDUFrame->ucInfoUnit[len++] =  (unsigned char)(pTmp_Point->uiAddr>>16);
		}
		for(j=0; ((j<sys_mod_p->usYC_Num-send_over)&&( len<240)&&( j<(1<<5))); j++)
		{
			if(sys_mod_p->Continuity & YC_NOTCONTINUE) 					//²»Á¬ÐøµØÖ·
			{
				pTemp_ASDUFrame->ucInfoUnit[len++] = (unsigned char)(pTmp_Point->uiAddr&0xff);
				pTemp_ASDUFrame->ucInfoUnit[len++] = (unsigned char)(pTmp_Point->uiAddr>>8);
				pTemp_ASDUFrame->ucInfoUnit[len++] =  (unsigned char)(pTmp_Point->uiAddr>>16);
			}
			data =(float)Read_From_Sharearea(pTmp_Point->uiOffset, TYPE_YC);	  //Ò£²âÖµ
			//my_debug("pTmp_Point->uiOffset:%d",pTmp_Point->uiOffset);
            if(pIEC104_Struct->yctype == TYPE_FLOAT)//¸¡µãÖµ
            {
    			memcpy((UInt8 *)(pTemp_ASDUFrame->ucInfoUnit+len), (UInt8 *)&data,4);
    			len+=4;
            }
            else if(pIEC104_Struct->yctype == TYPE_GUIYI)//¹éÒ»Öµ
            {
            	pTemp_ASDUFrame->ucType = IEC104_M_ME_NA_1;
    			pTemp_ASDUFrame->ucInfoUnit[len++] = 0xff;
    			pTemp_ASDUFrame->ucInfoUnit[len++] = 0xff;
            }
			pTemp_ASDUFrame->ucInfoUnit[len++] = 0x00;	//Æ·ÖÊÒòÊý
			if(pTmp_Point->next !=NULL)
				pTmp_Point = pTmp_Point->next;
		}
		send_over = send_over +j;
		my_debug("usYC_Num:%d send_over:%d j:%d", sys_mod_p->usYC_Num, send_over, j);
		if(sys_mod_p->Continuity & YC_NOTCONTINUE)   //²»Á¬Ðø
			pTemp_ASDUFrame->ucRep_Num  = j;
		else
			pTemp_ASDUFrame->ucRep_Num  =IEC104_VSQ_SQ_BIT|j;
		IEC104_APDU_Send(FORMAT_I, len+6);
	}
	return 0;
}

/**************************************************************************/
//º¯ÊýÃèÊö:±¨¸æËùÓÐÒ£ÐÅÊý¾Ý
//ÊäÈë:ÎÞ
//Êä³ö:ÎÞ
//±à¼­:R&N
/**************************************************************************/
static unsigned char IEC104_AP_ALL_YX( void )// ±¨¸æÈ«²¿Ò£ÐÅÊý¾Ý
{
	short len,j, k, send_over=0;
	IEC104_ASDU_Frame *pTemp_ASDUFrame;
	Point_Info   * pTmp_Point=NULL;
	UInt32 data=0;

	pTemp_ASDUFrame = (IEC104_ASDU_Frame *)(pIEC104_Struct->pSendBuf+6);

	pTemp_ASDUFrame->ucType = IEC104_M_SP_NA_1;					//µ¥µãÒ£ÐÅ
	if(sys_mod_p->Continuity & YC_NOTCONTINUE) 			 		//µØÖ·²»Á¬Ðø
		pTemp_ASDUFrame->ucRep_Num = 0;
	else
		pTemp_ASDUFrame->ucRep_Num = IEC104_VSQ_SQ_BIT;

	pTemp_ASDUFrame->ucCauseL = IEC104_CAUSE_INTROGEN;
	pTemp_ASDUFrame->ucCauseH = 0x00;
	pTemp_ASDUFrame->ucCommonAddrL = pIEC104_Struct->linkaddr;    //Õ¾µØÖ·
	pTemp_ASDUFrame->ucCommonAddrH = 0x00;

	pTmp_Point = sys_mod_p->pYX_Addr_List_Head;

	for(k=0; k<(sys_mod_p->usYX_Num-send_over); )		//·¢ËÍ¶àÉÙ´Î
	{
		len = 0;
		if(!(sys_mod_p->Continuity & YX_NOTCONTINUE))
		{
			pTemp_ASDUFrame->ucInfoUnit[len++] = (unsigned char)(pTmp_Point->uiAddr&0xff);
			pTemp_ASDUFrame->ucInfoUnit[len++] = (unsigned char)(pTmp_Point->uiAddr>>8);
			pTemp_ASDUFrame->ucInfoUnit[len++] =  (unsigned char)(pTmp_Point->uiAddr>>16);
		}
		for(j=0; ((j<sys_mod_p->usYX_Num-send_over)&&( len<240)&&( j<(1<<7))); j++)
		{
			if(sys_mod_p->Continuity & YX_NOTCONTINUE) 					//²»Á¬ÐøµØÖ·
			{
				pTemp_ASDUFrame->ucInfoUnit[len++] = (unsigned char)(pTmp_Point->uiAddr&0xff);
				pTemp_ASDUFrame->ucInfoUnit[len++] = (unsigned char)(pTmp_Point->uiAddr>>8);
				pTemp_ASDUFrame->ucInfoUnit[len++] =  (unsigned char)(pTmp_Point->uiAddr>>16);
			}
			data = Read_From_Sharearea(pTmp_Point->uiOffset, TYPE_YX_STATUS_BIT);	  //Ò£ÐÅÖµ
			if((data&(1<<(pTmp_Point->uiOffset&0x7))) !=0)
					pTemp_ASDUFrame->ucInfoUnit[len++] = 0x01;			//Æ·ÖÊÒòÊý
				else
					pTemp_ASDUFrame->ucInfoUnit[len++] = 0x0;				//Æ·ÖÊÒòÊý
			if(pTmp_Point->next !=NULL)
				pTmp_Point = pTmp_Point->next;
		}
		send_over = send_over +j;
		my_debug("usYX_Num:%d send_over:%d j:%d", sys_mod_p->usYX_Num, send_over, j);
		if(sys_mod_p->Continuity & YX_NOTCONTINUE)   //²»Á¬Ðø
			pTemp_ASDUFrame->ucRep_Num  = j;
		else
			pTemp_ASDUFrame->ucRep_Num  =IEC104_VSQ_SQ_BIT|j;
		IEC104_APDU_Send(FORMAT_I, len+6);
	}
	return 0;
}

/**************************************************************************/
//º¯ÊýÃèÊö:×ÜÕÙ»½´¦Àí
//ÊäÈë:ÎÞ
//Êä³ö:ÎÞ
//±à¼­:R&N
/**************************************************************************/
static void IEC104_AP_INTERROALL( void )
{
	if(sys_mod_p->usYC_Num>=1)
		IEC104_AP_ALL_YC();
	if(sys_mod_p->usYX_Num>=1)
		IEC104_AP_ALL_YX();
	pIEC104_Struct->ucInterro_First=0x00;
	IEC104_AP_INTERROALL_END();
}
/***********************************************************************/
//º¯Êý:static unsigned char IEC104_Chg_Search()
//ËµÃ÷:¼ì²éÊÇ·ñÓÐÏà¹ØµÄÊÂ¼þ
//ÊäÈë:ÎÞ
//Êä³ö:¸÷ÖÖÊÂ¼þ
//±à¼­:R&N
//Ê±¼ä:2015.05.9
/***********************************************************************/
static unsigned char IEC104_Chg_Search()
{
	if(pYXInfoGet->ucStatus== FULL)
    {
        my_debug("Receive MSG_COS");
		return MSG_COS;
    }
	if(pSOEGet->ucStatus== FULL)
     {
      my_debug("Receive MSG_SOE");
		return  MSG_SOE;
     }
	return(0);
}

/***********************************************************************/
//º¯Êý:static unsigned char IEC104_Process_SOE( void ) //Ó¦ÓÃ²ã´¦Àí
//ËµÃ÷:´¦Àíµ¥µãÒ£ÐÅSOE
//ÊäÈë:
//Êä³ö:
//±à¼­:R&N
//Ê±¼ä:2015.05.9
/***********************************************************************/
static unsigned char IEC104_Process_SOE( void ) //Ó¦ÓÃ²ã´¦Àí
{
    UInt8 len=0;
	IEC104_ASDU_Frame *pTemp_ASDUFrame;
	Point_Info   * pTmp_Point=NULL;
	static  unsigned  cout_pc=0;
    unsigned char  buffer[200];
    if(new_fd == (-1))
        return 0;

	pTemp_ASDUFrame = (IEC104_ASDU_Frame *)(pIEC104_Struct->pSendBuf+6);
	pTemp_ASDUFrame->ucType = IEC104_M_SP_TB_1;         // Êý¾ÝÀàÐÍ
	pTemp_ASDUFrame->ucRep_Num = 0x00;                  //Êý¾Ý¸öÊý³õÖµ
	pTemp_ASDUFrame->ucCauseL = IEC104_CAUSE_SPONT;     // ·¢ËÍÔ­Òò
	pTemp_ASDUFrame->ucCauseH = 0x00;
	pTemp_ASDUFrame->ucCommonAddrH = 0x00;

    while(pSOEGet->ucStatus == FULL)
    {
    	pTemp_ASDUFrame->ucCommonAddrL = pIEC104_Struct->linkaddr;    //Õ¾µØÖ·
    	pTemp_ASDUFrame->ucCommonAddrH = 0x00;
        //±éÀúÁ´±íÕÒµ½¶ÔÓ¦µÄÒ£ÐÅÏà
        pTmp_Point = sys_mod_p->pYX_Addr_List_Head;
        while(pTmp_Point->uiOffset != pSOEGet->usIndex)
        {
            pTmp_Point = pTmp_Point->next;
            if(pTmp_Point == NULL)
                break;
        }
        if(pTmp_Point != NULL)
        {
    		pTemp_ASDUFrame->ucInfoUnit[len++] = (unsigned char)(pTmp_Point->uiAddr& 0xff);
    		pTemp_ASDUFrame->ucInfoUnit[len++] = (unsigned char)((pTmp_Point->uiAddr>>8) & 0xff);
    		pTemp_ASDUFrame->ucInfoUnit[len++] = 0x00;
    		
            if(pSOEGet->ucYXValue)
      		    pTemp_ASDUFrame->ucInfoUnit[len++] = 1;   //Öµ
      		else
                pTemp_ASDUFrame->ucInfoUnit[len++] = 0;   //Öµ

			pTemp_ASDUFrame->ucInfoUnit[len++] = (unsigned char)( pSOEGet->usMSec & 0xff);
			pTemp_ASDUFrame->ucInfoUnit[len++] = (unsigned char)(pSOEGet->usMSec >> 8);
			pTemp_ASDUFrame->ucInfoUnit[len++]= pSOEGet->ucMin;
			pTemp_ASDUFrame->ucInfoUnit[len++]= pSOEGet->ucHour;
			pTemp_ASDUFrame->ucInfoUnit[len++] =pSOEGet->ucDay;
			pTemp_ASDUFrame->ucInfoUnit[len++]= (pSOEGet->ucMonth+1);
			pTemp_ASDUFrame->ucInfoUnit[len++] = pSOEGet->ucYear-100;
    		pTemp_ASDUFrame->ucRep_Num++;                 //¸öÊý
    	}
        /*
        if(sys_mod_p->pc_flag &(1<<MSG_EVENT_SOE))
        PC_Send_Data(MSG_EVENT_SOE, (unsigned char *)&pTemp_ASDUFrame->ucInfoUnit[0], 11); 
         
        if(sys_mod_p->pc_flag &(1<<MSG_ALL_SOE))
        PC_Send_Data(MSG_ALL_SOE, (unsigned char *)&pTemp_ASDUFrame->ucInfoUnit[0], 11); 
        */
		pSOEGet->ucStatus =  EMPTY;
        pSOEGet = pSOEGet->next;
        if(pTemp_ASDUFrame->ucRep_Num>=8)
            break;
    }
	if(pTemp_ASDUFrame->ucRep_Num>0)
	{
		IEC104_APDU_Send(FORMAT_I,len+6);//len+5ÎªFORMAT_IµÄASDU³¤¶È
		return(1);
	}
	else
		return(0);
}

/***********************************************************************/
//º¯Êý:static unsigned char IEC104_Process_COS( void ) //Ó¦ÓÃ²ã´¦Àí
//ËµÃ÷:´¦Àíµ¥µãÒ£ÐÅ±äÎ»
//ÊäÈë:
//Êä³ö:
//±à¼­:R&N
//Ê±¼ä:2015.05.9
/***********************************************************************/
static unsigned char IEC104_Process_COS( void ) //Ó¦ÓÃ²ã´¦Àí
{
    UInt8 len=0;
	IEC104_ASDU_Frame *pTemp_ASDUFrame;
	Point_Info   * pTmp_Point=NULL;

    if(new_fd == (-1))
        return 0;

	pTemp_ASDUFrame = (IEC104_ASDU_Frame *)(pIEC104_Struct->pSendBuf+6);
	pTemp_ASDUFrame->ucType = IEC104_M_SP_NA_1;         // Êý¾ÝÀàÐÍ
	pTemp_ASDUFrame->ucRep_Num = 0x00;                  //Êý¾Ý¸öÊý³õÖµ
	pTemp_ASDUFrame->ucCauseL = IEC104_CAUSE_SPONT;     // ·¢ËÍÔ­Òò
	pTemp_ASDUFrame->ucCauseH = 0x00;
	pTemp_ASDUFrame->ucCommonAddrH = 0x00;

    while(pYXInfoGet->ucStatus == FULL)
    {
    	pTemp_ASDUFrame->ucCommonAddrL = pIEC104_Struct->linkaddr;    //Õ¾µØÖ·
    	pTemp_ASDUFrame->ucCommonAddrH = 0x00;
        //±éÀúÁ´±íÕÒµ½¶ÔÓ¦µÄÒ£ÐÅÏà
        pTmp_Point = sys_mod_p->pYX_Addr_List_Head;
        while(pTmp_Point->uiOffset != pYXInfoGet->usIndex)
        {
            pTmp_Point = pTmp_Point->next;
            if(pTmp_Point == NULL)
                break;
        }
        if(pTmp_Point != NULL)
        {
    		pTemp_ASDUFrame->ucInfoUnit[len++] = (unsigned char)(pTmp_Point->uiAddr& 0xff);
    		pTemp_ASDUFrame->ucInfoUnit[len++] = (unsigned char)((pTmp_Point->uiAddr>>8) & 0xff);
    		pTemp_ASDUFrame->ucInfoUnit[len++] = 0x00;
            if(pYXInfoGet->ucYXValue)
      		    pTemp_ASDUFrame->ucInfoUnit[len++] = 1;   //Öµ
      		else
                pTemp_ASDUFrame->ucInfoUnit[len++] = 0;   //Öµ
    		pTemp_ASDUFrame->ucRep_Num++;                 //¸öÊý
    	}
        else
            my_debug("cannot find the index");
        /*
        if(sys_mod_p->pc_flag &(1<<MSG_EVENT_COS))
        PC_Send_Data(MSG_EVENT_COS, (unsigned char *)&pTemp_ASDUFrame->ucInfoUnit[0], 4);
        */
		pYXInfoGet->ucStatus =  EMPTY;
        pYXInfoGet = pYXInfoGet->next;
        if(pTemp_ASDUFrame->ucRep_Num>=8)
            break;
    }
	if(pTemp_ASDUFrame->ucRep_Num>0)
	{
		IEC104_APDU_Send(FORMAT_I,len+6);//len+5ÎªFORMAT_IµÄASDU³¤¶È
		return(1);
	}
	else
    {
       	my_debug("ucRep_Num=0");
		return(0);
     }
}

/**************************************************************************/
//º¯ÊýËµÃ÷:ÓÉÓÚ104È¡ÏûÁËÕÙ»½Ò»¼¶¶þ¼¶Êý¾Ý£¬ÔÝÊ±ÎÞ·ÇÆ½ºâ´«Êä·½Ê½ÏÂµÄÊÂ¼þÊÕ¼¯·½·¨£¬
//			      ËùÒÔ²ÉÓÃÆ½ºâ´«Êä·½Ê½ÏÂµÄÊÂ¼þÊÕ¼¯·½·¨£¬ÊÂ¼þÈ«²¿Ö÷¶¯ÉÏËÍ
//Êä³ö:ÎÞ
//±à¼­:R&N
/**************************************************************************/

static void IEC104_UnsoSend( void )
{
	unsigned char ucTemp_Chg;

	ucTemp_Chg = IEC104_Chg_Search();
	if(ucTemp_Chg==MSG_COS)           //YX change  		2---Ò£ÐÅ±äÎ»
	{
		if(IEC104_Process_COS())
			return;
	}
	if(pIEC104_Struct->ucYK_TimeCount_Flag)
	{
//		if(pIEC104_Struct->ucYK_TimeCount >= pIEC104_Struct->ucYK_Limit_Time)
//		{
//		}
//		if(pYKUpInfoGet[3]->usStatus == FULL)
//		{
//		}
//		pIEC104_Struct->ucYK_TimeCount++;
	}

	if(pIEC104_Struct->ucInterro_First)		//×ÜÕÙ»½±ê¼Ç
	{
		IEC104_AP_INTERROALL();
		return;
	}

//	for(i=0;i<pIEC104_Struct->ucRTU_Num;i++) //²éÕÒ¸÷RTUÊÇ·ñ´æÔÚÒ£²â×éÕÙ»½
//	{
//		if(pIEC104_Struct->ucAI_RQGroup_Flag[i])
//		{
//			IEC104_AP_GROUP_AI(pIEC104_Struct->ucRTU_Code[i],pIEC104_Struct->ucAI_Group_No[i]);
//			return;
//		}
//	}
//
//	for(i=0;i<pIEC104_Struct->ucRTU_Num;i++) //²éÕÒ¸÷RTUÊÇ·ñ´æÔÚÒ£ÐÅ×éÕÙ»½
//	{
//		if(pIEC104_Struct->ucDI_RQGroup_Flag[i])
//		{
//			IEC104_AP_GROUP_DI(pIEC104_Struct->ucRTU_Code[i],pIEC104_Struct->ucDI_Group_No[i]);
//			return;
//		}
//	}
//


	if(ucTemp_Chg == MSG_SOE)		 //	4---SOEÊÂ¼þ
	{
		if(IEC104_Process_SOE())
			return;
	}
	if(ucTemp_Chg == 1)          //YC Overstep   1---Ò£²âÊý¾Ý
	{
//		if(IEC104_AP_OVERSTEP_AI())
			return;
	}

}


/**************************************************************************/
//º¯ÊýËµÃ÷:´¦ÀíÎ´ÖªÃüÁîµØÖ·
//ÊäÈë:ASDUÖ¡ºÍ³¤¶È
//Êä³öå:ÎÞ
//±à¼­:R&N
/**************************************************************************/
static void IEC104_AP_UnknownCommAddr(IEC104_ASDU_Frame *pRecv_ASDUFrame,unsigned char ucTemp_Length) //ASDU³¤¶È
{
	unsigned char i;
	IEC104_ASDU_Frame *pTemp_ASDUFrame;
	pTemp_ASDUFrame = (IEC104_ASDU_Frame *)(pIEC104_Struct->pSendBuf+6);
	pTemp_ASDUFrame->ucType = pRecv_ASDUFrame->ucType;
	pTemp_ASDUFrame->ucRep_Num = pRecv_ASDUFrame->ucRep_Num;
	pTemp_ASDUFrame->ucCauseL = IEC104_CAUSE_N_BIT | IEC104_CAUSE_UNKNOWNCOMMADDR;
	pTemp_ASDUFrame->ucCauseH = pRecv_ASDUFrame->ucCauseH;
	pTemp_ASDUFrame->ucCommonAddrL = pRecv_ASDUFrame->ucCommonAddrL;
	pTemp_ASDUFrame->ucCommonAddrH = pRecv_ASDUFrame->ucCommonAddrH;

	for(i=0;i<ucTemp_Length-6;i++)
	{
		pTemp_ASDUFrame->ucInfoUnit[i] = pRecv_ASDUFrame->ucInfoUnit[i];
	}
	IEC104_APDU_Send(FORMAT_I,ucTemp_Length);
}

/**************************************************************************/
//º¯ÊýËµÃ÷:´¦ÀíÎ´ÖªÔ­Òò
//ÊäÈë:ASDUÖ¡ºÍ³¤¶È
//Êä³öå:ÎÞ
//±à¼­:R&N
/**************************************************************************/
static void IEC104_AP_UnknownCause(IEC104_ASDU_Frame *pRecv_ASDUFrame,unsigned char ucTemp_Length)
{
	unsigned char i;
	IEC104_ASDU_Frame *pTemp_ASDUFrame;

	pTemp_ASDUFrame = (IEC104_ASDU_Frame *)(pIEC104_Struct->pSendBuf+6);

	pTemp_ASDUFrame->ucType = pRecv_ASDUFrame->ucType;
	pTemp_ASDUFrame->ucRep_Num = pRecv_ASDUFrame->ucRep_Num;
	pTemp_ASDUFrame->ucCauseL = IEC104_CAUSE_N_BIT | IEC104_CAUSE_UNKNOWNCAUSE;
	pTemp_ASDUFrame->ucCauseH = pRecv_ASDUFrame->ucCauseH;
	pTemp_ASDUFrame->ucCommonAddrL = pRecv_ASDUFrame->ucCommonAddrL;
	pTemp_ASDUFrame->ucCommonAddrH = pRecv_ASDUFrame->ucCommonAddrH;

	for(i=0;i<ucTemp_Length-6;i++)
	{
		pTemp_ASDUFrame->ucInfoUnit[i] = pRecv_ASDUFrame->ucInfoUnit[i];
	}
	IEC104_APDU_Send(FORMAT_I,ucTemp_Length);
}

/**************************************************************************/
//º¯ÊýËµÃ÷:´¦ÀíÎ´ÖªÀàÐÍ
//ÊäÈë:ASDUÖ¡ºÍ³¤¶È
//Êä³öå:ÎÞ
//±à¼­:R&N
/**************************************************************************/
static void IEC104_AP_UnknownType(IEC104_ASDU_Frame *pRecv_ASDUFrame,unsigned char ucTemp_Length)
{
	unsigned char i;
	IEC104_ASDU_Frame *pTemp_ASDUFrame;

	pTemp_ASDUFrame = (IEC104_ASDU_Frame *)(pIEC104_Struct->pSendBuf+6);

	pTemp_ASDUFrame->ucType = pRecv_ASDUFrame->ucType;
	pTemp_ASDUFrame->ucRep_Num = pRecv_ASDUFrame->ucRep_Num;
	pTemp_ASDUFrame->ucCauseL = IEC104_CAUSE_N_BIT | IEC104_CAUSE_UNKNOWNTYPE;
	pTemp_ASDUFrame->ucCauseH = pRecv_ASDUFrame->ucCauseH;
	pTemp_ASDUFrame->ucCommonAddrL = pRecv_ASDUFrame->ucCommonAddrL;
	pTemp_ASDUFrame->ucCommonAddrH = pRecv_ASDUFrame->ucCommonAddrH;

	for(i=0;i<ucTemp_Length-6;i++)
	{
		pTemp_ASDUFrame->ucInfoUnit[i] = pRecv_ASDUFrame->ucInfoUnit[i];
	}
	IEC104_APDU_Send(FORMAT_I, ucTemp_Length);
}

/**************************************************************************/
//º¯ÊýÃèÊö:¸´Î»Ô¶·½Á´Â·
//ÊäÈë:ASDUÖ¡ºÍ³¤¶È
//Êä³ö:ÎÞ
//±à¼­:R&N
/**************************************************************************/

static void IEC104_AP_RP(IEC104_ASDU_Frame *pRecv_ASDUFrame,unsigned char ucTemp_Length) //´Ë´¦³¤¶ÈÎªASDU³¤¶È
{
	unsigned char i;
//	unsigned short usTemp_MSec;
//	unsigned char ucTemp_Sec,ucTemp_Min,ucTemp_Hour,ucTemp_Day,ucTemp_Month,ucTemp_Year;
	IEC104_ASDU_Frame *pTemp_ASDUFrame;

	pTemp_ASDUFrame = (IEC104_ASDU_Frame *)(pIEC104_Struct->pSendBuf+6);


	if(pRecv_ASDUFrame->ucCommonAddrL == 0xff)
	{
		IEC104_AP_UnknownCommAddr(pRecv_ASDUFrame,ucTemp_Length);
		return;
	}
	if(pRecv_ASDUFrame->ucCauseL == IEC104_CAUSE_ACT)  //¼¤»î
	{
		pTemp_ASDUFrame->ucType = pRecv_ASDUFrame->ucType;
		pTemp_ASDUFrame->ucRep_Num = 0x01;  			//Êý¾Ý¸öÊý³õÖµ
		pTemp_ASDUFrame->ucCauseL = IEC104_CAUSE_ACTCON;
		pTemp_ASDUFrame->ucCauseH = 0x00;
		pTemp_ASDUFrame->ucCommonAddrL = pRecv_ASDUFrame->ucCommonAddrL;
		pTemp_ASDUFrame->ucCommonAddrH = pRecv_ASDUFrame->ucCommonAddrH;
		for(i=0;i<ucTemp_Length-6;i++)
		{
			pTemp_ASDUFrame->ucInfoUnit[i] = pRecv_ASDUFrame->ucInfoUnit[i];
		}
		IEC104_APDU_Send(FORMAT_I,ucTemp_Length);
		return;
	}
	else
	{
		IEC104_AP_UnknownCause(pRecv_ASDUFrame,ucTemp_Length);
		return;
	}
}

/**************************************************************************/
//º¯Êý¹¦ÄÜ:¼ìÑé½ÓÊÕµ½µÄÊý¾ÝÊÇ²»ÊÇ104¹æÔ¼µÄ±êÖ¾Ö¡
//ÊäÈë:pTemp_Buf½ÓÊÕµ½µÄÊý¾Ý»º´æ         usTemp_Length½ÓÊÕµ½µÄ³¤¶È
//Êä³ö:
//		IEC104FRAMEERR         1	´íÎóÖ¡
//		IEC104FRAMESEQERR    2	Ð£Ñé´íÎó
//		IECFRAMENEEDACK       3	ÐèÒª»Ø¸´
//		IEC104FRAMEOK          0	OK
/**************************************************************************/
static unsigned char IEC104_VerifyDLFrame(unsigned char * pTemp_Buf , unsigned short usTemp_Length)
{
	unsigned short usTemp_ServSendNum;
	unsigned short usTemp_ServRecvNum;
	unsigned char ucTemp_Length;
    
	unsigned char *pTemp_SendBuf;
    my_debug("VerifyDLFrame");
	if( *pTemp_Buf != 0x68 )
		return IEC104FRAMEERR;

	if( usTemp_Length > 255 || usTemp_Length<6)
		return IEC104FRAMEERR;

	ucTemp_Length = usTemp_Length & 0xff;
	if( pTemp_Buf[1] != (ucTemp_Length - 2) )
		return IEC104FRAMEERR;
	pTemp_SendBuf = pIEC104_Struct->pSendBuf;
	usTemp_ServSendNum = (pTemp_Buf[3]<<8)|pTemp_Buf[2];  //·¢ËÍÐòÁÐºÅ
	usTemp_ServRecvNum = (pTemp_Buf[5]<<8)|pTemp_Buf[4];  //½ÓÊÕºÅ
	my_debug("usTemp_ServSendNum:%d usTemp_ServRecvNum:%d",usTemp_ServSendNum,usTemp_ServRecvNum);
	if( !(usTemp_ServSendNum & 0x0001) )   //FORMAT_I
	{
		if(ucTemp_Length < 12)              //ASDU I Ö¡Ó¦¸Ã´ïµ½Ò»¸öÆðÊ¼³¤¶È
		{
			return IEC104FRAMEERR;
		}
		usTemp_ServRecvNum >>= 1;
		pIEC104_Struct->usServRecvNum = usTemp_ServRecvNum;

		if(pIEC104_Struct->ucSendCountOverturn_Flag)//·¢ËÍI¸ñÊ½Ö¡´ïµ½0x7FFF£¬½øÐÐ·´×ª
		{
			if(usTemp_ServRecvNum >= 0x0000 && usTemp_ServRecvNum < 0x08)
			{
				pIEC104_Struct->ucSendCountOverturn_Flag = FALSE;
				pIEC104_Struct->k = pIEC104_Struct->usSendNum - usTemp_ServRecvNum;
                my_debug("I pIEC104_Struct->k:%d usSendNum:%d",pIEC104_Struct->k,pIEC104_Struct->usSendNum);
			}
			else
				pIEC104_Struct->k = 0x8000 + pIEC104_Struct->usSendNum - usTemp_ServRecvNum;

		}
		else
		{
			if(usTemp_ServRecvNum <= pIEC104_Struct->usSendNum )
			{
				pIEC104_Struct->k = pIEC104_Struct->usSendNum - usTemp_ServRecvNum;
                my_debug("I usTemp_ServRecvNum:%d pIEC104_Struct->usSendNum:%d",usTemp_ServRecvNum,pIEC104_Struct->usSendNum);
			}
			else if( usTemp_ServRecvNum > pIEC104_Struct->usSendNum );
				//return IEC104FRAMESEQERR;
		}

		usTemp_ServSendNum >>= 1;
		pIEC104_Struct->usServSendNum = usTemp_ServSendNum;
		if( usTemp_ServSendNum != pIEC104_Struct->usRecvNum )//Ö÷Õ¾·¢ËÍÐòÁÐºÅ  != ´ÓÕ¾½ÓÊÕÐòÁÐºÅ
			return IEC104FRAMESEQERR;
		else                                                  //Ö÷Õ¾·¢ËÍÐòÁÐºÅ  == ´ÓÕ¾½ÓÊÕÐòÁÐºÅ
		{
			pIEC104_Struct->w++;                                //½ÓÊÕ·½Á¬Ðø½ÓÊÕWÖ¡»Ø¸´È·ÈÏ
            //pIEC104_Struct->usRecvNum = (pIEC104_Struct->usRecvNum+1)&0x7FFF;//×î´ó0x7FFF
            if(usTemp_ServSendNum==0x7fff)
            {
                pIEC104_Struct->usRecvNum = 0; 
            }
            else 
                pIEC104_Struct->usRecvNum++; 
            my_debug("w:%d",pIEC104_Struct->w);
			my_debug("I usTemp_ServSendNum:%d pIEC104_Struct->usRecvNum:%d",usTemp_ServSendNum,pIEC104_Struct->usRecvNum);
			if(pIEC104_Struct->w >= IEC104_MAX_W)               //½ÓÊÕµ½IEC104_MAX_W¸öI±¨ÎÄºó¸ø³öÈ·ÈÏ
			{
    			//pIEC104_Struct->w = pIEC104_Struct->w%IEC104_MAX_W;
                my_debug("IECFRAMENEEDACK");
				return IECFRAMENEEDACK;
			}
		}
		return  IEC104FRAME_I;
	}
//	else if( (usTemp_ServSendNum & 0x03)==0x01 )   //FORMAT_S
    else if( !(usTemp_ServSendNum & 0x0002))   //FORMAT_S
	{
		if( ucTemp_Length != 6 )
			return IEC104FRAMEERR;
		usTemp_ServRecvNum >>= 1;
		if(pIEC104_Struct->ucSendCountOverturn_Flag)
		{
			if(usTemp_ServRecvNum >= 0x0000 && usTemp_ServRecvNum < 0x08)
			{
				pIEC104_Struct->ucSendCountOverturn_Flag = FALSE;
				pIEC104_Struct->k = pIEC104_Struct->usSendNum - usTemp_ServRecvNum;
                my_debug("S pIEC104_Struct->k",pIEC104_Struct->k);
			}
			else
				pIEC104_Struct->k = 0x8000 + pIEC104_Struct->usSendNum - usTemp_ServRecvNum;

		}
		else
		{
			if(usTemp_ServRecvNum <= pIEC104_Struct->usSendNum )
			{
				pIEC104_Struct->k = pIEC104_Struct->usSendNum - usTemp_ServRecvNum;
                my_debug("S pIEC104_Struct->k:%d",pIEC104_Struct->k);
                my_debug("S usTemp_ServRecvNum:%d pIEC104_Struct->usSendNum:%d",usTemp_ServRecvNum,pIEC104_Struct->usSendNum);
			}
			else if( usTemp_ServRecvNum > pIEC104_Struct->usSendNum );//Ö÷Õ¾
				//return IEC104FRAMESEQERR;
		}
		return IECFRAMENEEDACK;
	}
	else
	{
		if( ucTemp_Length != 6 )
			return IEC104FRAMEERR;
        //pIEC104_Struct->usSendNum=0;
		return IEC104FRAME_U;
	}
}

/**************************************************************************/
//º¯ÊýËµÃ÷:ÓÃÓÚ·¢ËÍ  I    S    UÖ¡
//ucTemp_FrameFormat   Ö¡¸ñÊ½
//ucTemp_ValueÖ¡³¤¶È
//Êä³ö:ÎÞ
//±à¼­:R&N
/**************************************************************************/
static void IEC104_APDU_Send(unsigned char ucTemp_FrameFormat,unsigned char ucTemp_Value)
{
	unsigned char *pTemp_SendBuf;
	unsigned short usTemp_Num;
	int ucTemp_nSendCount;

	pTemp_SendBuf = pIEC104_Struct->pSendBuf;
	if(new_fd == -1)//Ê×ÏÈÅÐ¶ÏÊÇ·ñ½¨Á¢Á¬½Ó
		return;
      /*´´½¨Ê±¼ä¼ä¸ôt2*/
    //unsigned int cout_Iframe=0;
	switch(ucTemp_FrameFormat)
	{
	   case FORMAT_I:
		if(pIEC104_Struct->ucDataSend_Flag)
		{
		    my_debug("FORMAT_I!");
			*pTemp_SendBuf++ = 0x68;
			*pTemp_SendBuf++ = ucTemp_Value+0x04;
			usTemp_Num = pIEC104_Struct->usSendNum<<1;
			*pTemp_SendBuf++ = usTemp_Num & 0xff;
			*pTemp_SendBuf++ = usTemp_Num >> 8;
			usTemp_Num = pIEC104_Struct->usRecvNum<<1;
            my_debug("I usSendNum:%d usTemp_Num_Recv:%d",(pIEC104_Struct->usSendNum<<1),usTemp_Num);
			*pTemp_SendBuf++ = usTemp_Num & 0xff;
			*pTemp_SendBuf = usTemp_Num >> 8;
            if((pIEC104_Struct->usSendNum-pIEC104_Struct->usRecvNum)>=12)
            {
                    my_debug("send - rece min 12!");
                    close(new_fd);
                   //return IECFRAMENEEDACK;
            }
            if(flag_kvalue>=1)
            {
                flag_kvalue=0;
                my_debug("I_frame flag:%d",flag_kvalue);
            }
            if(flag_kvalue==0)
            {
                int i;
                set_timer();
                signal(SIGVTALRM, sig_handler);
                if(cout_Iframe<20)
                {
                    if(pIEC104_Struct->k >= IEC104_MAX_K)//¸ãÇå³þ
            		{
                         my_debug("k max 12...");
                         close(new_fd);
                         //return IECFRAMENEEDACK;
                         //return ;
            		}
               }
            }
			ucTemp_nSendCount = send(new_fd,  (char*)pIEC104_Struct->pSendBuf, (ucTemp_Value+6), 0);
            if(ucTemp_nSendCount != ucTemp_Value+6)//Èç¹û·¢ËÍ²»³É¹¦
			{
			    my_debug("send failure");
				IEC104_Init_Protocol();
				return;
			}
			Log_Frame(pIEC104_Struct->logfd,  (unsigned char *)pIEC104_Struct->pSendBuf,  0,  ucTemp_Value+6, SEND_FRAME);
			//Log_Frame(pIEC104_Struct->logsoEfd,(unsigned char *)pIEC104_Struct->pSendBuf,  0,  ucTemp_Value+6, SEND_FRAME);
            //write(pIEC104_Struct->logsoEfd,(char *)&pIEC104_Struct->pSendBuf[12],ucTemp_Value-6);
/*
            if(sys_mod_p->pc_flag &(1<<MSG_EVENT_SOE))
            PC_Send_Data(MSG_EVENT_SOE, (unsigned char *)&pIEC104_Struct->pSendBuf[12], ucTemp_Value-6);
*/
            if(pIEC104_Struct->usSendNum == 0x7fff)
			{
				pIEC104_Struct->usSendNum = 0;
				pIEC104_Struct->ucSendCountOverturn_Flag = TRUE;
                my_debug("ucSendCountOverturn_Flag");
			}
			else
				pIEC104_Struct->usSendNum++;
			pIEC104_Struct->k++;
            my_debug("k:%d",pIEC104_Struct->k);
//			pIEC104_Struct->w = 0;
//			pIEC104_Struct->ucWaitServConCount_Flag=TRUE;
//			pIEC104_Struct->ucWaitServConCount=0;
		}
		break;

	  case FORMAT_S:
		*pTemp_SendBuf++ = 0x68;
		*pTemp_SendBuf++ = 0x04;
		*pTemp_SendBuf++ = 0x01;
		*pTemp_SendBuf++ = 0x00;
		usTemp_Num = pIEC104_Struct->usRecvNum<<1;
        my_debug("S usTemp_Num_Rcev:%d",usTemp_Num);
		*pTemp_SendBuf++ = usTemp_Num & 0xff;
		*pTemp_SendBuf = usTemp_Num >> 8;

		ucTemp_nSendCount = send(new_fd, (char*)pIEC104_Struct->pSendBuf, 6, 0);

		if(ucTemp_nSendCount!=6)
		{
		    my_debug("ucTemp_nSendCount!=6");
			IEC104_Init_Protocol();
			return;
		}
		Log_Frame(pIEC104_Struct->logfd, (unsigned char *)pIEC104_Struct->pSendBuf,0,  6, SEND_FRAME);
		pIEC104_Struct->w = 0;//½ÓÊÕ·½Á¬Ðø½ÓÊÕWÖ¡»Ø¸´È·ÈÏ
		pIEC104_Struct->k = 0;//½ÓÊÕ·½Á¬Ðø½ÓÊÕKÖ¡»Ø¸´È·ÈÏ
        //pIEC104_Struct->usSendNum=0;
        flag_kvalue++;
        my_debug("FORMAT_S! flag:%d",flag_kvalue);
        if(flag_kvalue>=1)
            cout_Iframe=0;
		break;

	  case FORMAT_U:
		*pTemp_SendBuf++ = 0x68;
		*pTemp_SendBuf++ = 0x04;
		*pTemp_SendBuf++ = ucTemp_Value;
		*pTemp_SendBuf++ = 0x00;
		*pTemp_SendBuf++ = 0x00;
		*pTemp_SendBuf++ = 0x00;
		ucTemp_nSendCount = send(new_fd, (char*)pIEC104_Struct->pSendBuf, 6, 0);
		if(ucTemp_nSendCount!=6)
		{
			IEC104_Init_Protocol();
			return;
		}
		Log_Frame(pIEC104_Struct->logfd, (unsigned char*)pIEC104_Struct->pSendBuf, 0,  6,  SEND_FRAME);
        flag_kvalue++;
		pIEC104_Struct->k = 0;//½ÓÊÕ·½Á¬Ðø½ÓÊÕKÖ¡»Ø¸´È·ÈÏ
        my_debug("format_u flag:%d",flag_kvalue);
        if(flag_kvalue>=1)
            cout_Iframe=0;
		break;
	}
}
void sig_handler(int signo)
{
      cout_Iframe++;
//      if(cout_Iframe==20)
//        cout_Iframe=0;
      my_debug("cout_Iframe:%d",cout_Iframe);
      //return IECFRAMENEEDACK;
      //return;
      //Èç¹ûÊÇÏÈ¸ø×Ó½ø³Ì·¢ÐÅºÅ½áÊøµôËüÃÇ£¬È»ºó×Ô¼ºexit(0)ÍË³ö
      //ÁíÒ»ÖÖ·½·¨¿ÉÒÔÉèÒ»¸öÈ«¾Ö±äÁ¿£¬Ã¿´Î½øÈëÐÅºÅ´¦Àíº¯ÊýÊ±¼Ó1
}
void set_timer()  
{
    //struct timeval interval;
    //struct itimerval timer;
    //ÉèÖÃÊ±¼ä¼ä¸ôÎª10Ãë
/*  interval.tv_sec = pIEC104_Struct->ucTimeOut_t2;//Ãë 
    interval.tv_usec=0; //Î¢Ãë
    timer.it_interval = interval;//¶¨Ê±Æ÷Æô¶¯ºó£¬Ã¿¸ôxÃë½«Ö´ÐÐÏàÓ¦µÄº¯Êý
    timer.it_value = interval;//Ê®ÃëÖÓºó½«Æô¶¯¶¨Ê±Æ÷
*/    
    timer.it_value.tv_sec = 0;  //
    timer.it_value.tv_usec = 300000;
    timer.it_interval.tv_sec  =0; //¶¨Ê±Æ÷Æô¶¯ºó£¬Ã¿¸ô0Ãë½«Ö´ÐÐÏàÓ¦µÄº¯Êý
    timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_VIRTUAL, &timer, 0);//ÈÃËü²úÉúSIGVTALRMÐÅºÅ
}
void uninit_time() 
{ 
    //struct itimerval timer;
    timer.it_value.tv_sec = 0; 
    timer.it_value.tv_usec = 0; 
    timer.it_interval = timer.it_value; 
    setitimer(ITIMER_REAL, &timer, NULL); 
    //my_debug("release timer");
} 


/**************************************************************************/
//º¯Êý:void IEC104_Senddata2pc(int fd, char *tmp_buf, UInt16 len)
//ËµÃ÷:ÓÃÓÚ·¢ËÍ¼à¿ØÊý¾Ýµ½PC
//ÊäÈë:fdÓÃÀ´ÅÐ¶ÏÊÇiec101»¹ÊÇiec104  tmp_bufÐèÒª·¢ËÍµÄÊý¾Ý  lenÐèÒª·¢ËÍµÄÊý¾Ý³¤¶È
//Êä³ö:ÎÞ
//±à¼­:R&N
/**************************************************************************/
void IEC104_Senddata2pc(int fd, char *tmp_buf, UInt8 len)
{

    if(fd == pIEC104_Struct->logfd)
    {
        if(sys_mod_p->pc_flag &(1<<MSG_GET_IEC104))
            PC_Send_Data(MSG_GET_IEC104, (UInt8 *)tmp_buf, len);
        
    }
}

/***************************************************************************/
//º¯Êý: ÓÃÓÚ·¢ËÍÒ£²âÊý¾Ýµ½ÉÏÎ»»ú
//ËµÃ÷:ÓÃÓÚÍøÂçÁ¬½Ó£¬¹©ÉÏÎ»»úÊ¹ÓÃ
//ÊäÈë:buf:½ÓÊÕµ½µÄÊý¾Ý£¬ len:½ÓÊÕµ½µÄ³¤¶È
//Êä³ö:
//±à¼­:R&N
//Ê±¼ä:2015.8.21
/***************************************************************************/
void PC_Send_YC(UInt8* buf, UInt8 len)
{
    Point_Info   * pTmp_Point=NULL;
    UInt8 i=0, k;
    float data=0;
    UInt8 * tmp_buf = malloc(75);
    UInt16 uindex;
    if((buf[4]== 0xFF)&&(buf[5] == 0xFF))//ÕÙ»½ËùÓÐµÄÒ£²â
    {
        k = 0;
        pTmp_Point = sys_mod_p->pYC_Addr_List_Head;
        while(pTmp_Point != NULL)
        {
            uindex = pTmp_Point->uiOffset;
            //uindex = uindex + 1;
            tmp_buf[k] = uindex&0xFF;
            tmp_buf[k+1] = (uindex>>8)&0xFF;
            //tmp_buf[k] =   pTmp_Point->uiOffset&0xFF;       //Õ¼ÓÃ2×Ö½Ú
            //tmp_buf[k+1] =   (pTmp_Point->uiOffset>>8)&0xFF;
            //tmp_buf[k+2] =   pTmp_Point->uiAddr&0xFF;       //Õ¼ÓÃ4¸ö×Ö½Ú
            //tmp_buf[k+3] =   (pTmp_Point->uiAddr>>8)&0xFF;
            //tmp_buf[k+4] =   0x00;
            //tmp_buf[k+5] =   0x00;
            data =(float)Read_From_Sharearea(pTmp_Point->uiOffset, TYPE_YC);//Ò£²âÖµ
//          memcpy((UInt8 *)&tmp_buf[k+6], (UInt8 *)&data,4);//Õ¼ÓÃ4¸ö×Ö½Ú
            memcpy((UInt8 *)&tmp_buf[k+2], (UInt8 *)&data,4);//Õ¼ÓÃ4¸ö×Ö½Ú
            pTmp_Point = pTmp_Point->next;
//            k = k +10;
            k = k +6;
//            if(k>55)
            if(k>3)
            {
                PC_Send_Data(MSG_DIANBIAO_YC, tmp_buf, k);
                //my_debug("tem_buf[k+6]:%f tem_buf[k+10]:%f",tem_buf[k+6],tem_buf[k+10])£»
                k = 0;
            }
        }
        if(k>9)
            PC_Send_Data(MSG_DIANBIAO_YC, tmp_buf, k);
    }
    else
    {
        k = 0;
        for(i=0; i<len-5; )
        {
           pTmp_Point = sys_mod_p->pYC_Addr_List_Head;
           while(pTmp_Point != NULL)
            {
                if(((buf[i+4]<<8)|buf[i+5]) == pTmp_Point->uiOffset)
                {
                    break;
                }
                pTmp_Point = pTmp_Point->next;
                //i= i+2;
            }
            i= i+2;//Ã¿¸öindexÕ¼ÓÃ2¸ö×Ö½Ú
            if(pTmp_Point != NULL)//ÕÒµ½ÁË¶ÔÓ¦µÄ½á¹¹Ìå
            {
                
                uindex = pTmp_Point->uiOffset;
                uindex = uindex + 1;
                tmp_buf[k] = uindex&0xFF;
                tmp_buf[k+1] = (uindex>>8)&0xFF;
            //tmp_buf[k] =   pTmp_Point->uiOffset&0xFF;       //Õ¼ÓÃ2×Ö½Ú
            //tmp_buf[k+1] =   (pTmp_Point->uiOffset>>8)&0xFF;
            //tmp_buf[k+2] =   pTmp_Point->uiAddr&0xFF;       //Õ¼ÓÃ4¸ö×Ö½Ú
            //tmp_buf[k+3] =   (pTmp_Point->uiAddr>>8)&0xFF;
            //tmp_buf[k+4] =   0x00;
            //tmp_buf[k+5] =   0x00;
            data =(float)Read_From_Sharearea(pTmp_Point->uiOffset, TYPE_YC);	  //Ò£²âÖµ
            //memcpy((UInt8 *)&tmp_buf[k+6], (UInt8 *)&data,4);
            memcpy((UInt8 *)&tmp_buf[k+2], (UInt8 *)&data,4);
            
//            k = k+10;
            k = k+6;
            
            }
//            if(k>55)
            if(k>6)
            {
                PC_Send_Data(MSG_DIANBIAO_YC, tmp_buf, k);
                k = 0;
            }
        }
        if(k>9)
            PC_Send_Data(MSG_DIANBIAO_YC, tmp_buf, k);
    }
    free(tmp_buf);
}
/***************************************************************************/
//º¯Êý: ÓÃÓÚ·¢ËÍÒ£ÐÅÊý¾Ýµ½ÉÏÎ»»ú
//ËµÃ÷:ÓÃÓÚÍøÂçÁ¬½Ó£¬¹©ÉÏÎ»»úÊ¹ÓÃ
//ÊäÈë:buf:½ÓÊÕµ½µÄÊý¾Ý£¬ len:½ÓÊÕµ½µÄ³¤¶È
//Êä³ö:
//±à¼­:R&N
//Ê±¼ä:2015.8.19
/***************************************************************************/
void PC_Send_YX(UInt8* buf,UInt8 len)
{
    Point_Info   * pTmp_Point=NULL;
    UInt8 i=0, k;
    int data=0;
    UInt8 * tmp_buf = malloc(60);
    UInt16 uindex;
    if(buf[4] == 0xFF)//ÕÙ»½ËùÓÐµÄÒ£ÐÅ
    {
        k = 0;
        pTmp_Point = sys_mod_p->pYX_Addr_List_Head;
        while(pTmp_Point != NULL)
        {
            uindex = pTmp_Point->uiOffset;
            //uindex = uindex +1;
            tmp_buf[k] = uindex&0xFF;
            tmp_buf[k+1] = (uindex>>8)&0xFF;
            //tmp_buf[k] = pTmp_Point->uiOffset;
            //tmp_buf[k+1] = pTmp_Point->uiAddr&0xFF;
            //tmp_buf[k+2] = (pTmp_Point->uiAddr>>8)&0xFF;
            //tmp_buf[k+3] = 0;
            //tmp_buf[k+4] = 0;
            data =Read_From_Sharearea((pTmp_Point->uiOffset), TYPE_YX_STATUS_BIT);	  //Ò£ÐÅÖµ
//            tmp_buf[k+5] = (data&(1<<(pTmp_Point->uiOffset&0x7)))?1:0;
            tmp_buf[k+2] = (data&(1<<((pTmp_Point->uiOffset)&0x7)))?1:0;
            pTmp_Point = pTmp_Point->next;
//            k = k +6;
            k = k +3;
//          if(k>54)
            if(k>3)
            {
                PC_Send_Data(MSG_DIANBIAO_YX, tmp_buf, k);
                //my_debug("send yx data!");
                k = 0;
            }
        }
        if(k>8)
            PC_Send_Data(MSG_DIANBIAO_YX, tmp_buf, k);
    }
    else
    {
        k = 0;
        for(i=0; i<len-5; i++)
        {
           pTmp_Point = sys_mod_p->pYX_Addr_List_Head;
           while(pTmp_Point != NULL)
            {
                if(buf[i+4] == pTmp_Point->uiOffset)
                    break;
                pTmp_Point = pTmp_Point->next;
            }
            if(pTmp_Point != NULL)//ÕÒµ½ÁË¶ÔÓ¦µÄ½á¹¹Ìå
            {
            uindex = pTmp_Point->uiOffset;
            uindex = uindex + 1;
            tmp_buf[k] = uindex&0xFF;
            tmp_buf[k+1] = (uindex>>8)&0xFF;
            //tmp_buf[k] = pTmp_Point->uiOffset;
            //tmp_buf[k+1] = pTmp_Point->uiAddr&0xFF;
            //tmp_buf[k+2] = (pTmp_Point->uiAddr>>8)&0xFF;
            //tmp_buf[k+3] = 0;
            //tmp_buf[k+4] = 0;
            data =Read_From_Sharearea(pTmp_Point->uiOffset, TYPE_YX_STATUS_BIT);  //Ò£ÐÅÁ¿
            //tmp_buf[k+5] = (data&(1<<(pTmp_Point->uiOffset&0x7)))?1:0;
            tmp_buf[k+2] = (data&(1<<(pTmp_Point->uiOffset&0x7)))?1:0;                 
            pTmp_Point = pTmp_Point->next;
            //k = k +6;
            k = k +3;
            }
//          if(k>51)
            if(k>3)
            {
                PC_Send_Data(MSG_DIANBIAO_YX, tmp_buf, k);
                k = 0;
            }
        }
        if(k>4)
            PC_Send_Data(MSG_DIANBIAO_YX, tmp_buf, k);
    }
    free(tmp_buf);
}
