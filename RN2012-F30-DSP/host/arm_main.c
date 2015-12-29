/*************************************************************************/
// arm_main.c                                       Version 1.00
//
// ���ļ���DTU2.0ͨѶ����װ�õ�������
//ʵ�ֹ���: 4��ң��塢ÿ������3U9I�����128��ң����
//��128��ң����ʵ����Щ��Դ�����úͱ���
// ��д��:shaoyi       email:shaoyi1110@126.com
//  ��	   ��:2015.04.17
//  ע	   ��:
/*************************************************************************/
/* cstdlib header files */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
/*Linux header files*/
#include <sys/ioctl.h>
#include <net/if.h>
#include <pthread.h>
#include   <sys/time.h>
#include <signal.h>
#include <semaphore.h>

/* package header files */
#include <ti/syslink/Std.h>     /* must be first */
#include <ti/syslink/IpcHost.h>
#include <ti/syslink/SysLink.h>
#include <ti/ipc/MultiProc.h>
#include <ti/ipc/SharedRegion.h>

/* local header files */
#include "common.h"
#include "sharearea.h"
#include "iec104.h"
#include "iec101.h"
#include "monitorpc.h"
#include "readconfig.h"
#include "readsysconfig.h"
#include "nand_rw.h"
FAPARAM  pFAPARAM;
SYSPARAM pSYSPARAM;
//RWDATA   pRWDATA;
#define MODELCONFFILE			"/config/dianbiao.conf"
#define MODELLOGFFILE			"/config/model.txt"
/*******************extern function***************************************/

/*******************extern various***************************************/

/*******************local   struct***************************************/

/*******************local   various***************************************/
pthread_t  iec104_id, network_id, message_id, iec101_id, pc_id;
LOCAL_Module sys_mod;
Point_Info   *pYCPoint;			//ң��۰�����Ӧ��Ϣ
Point_Info   *pYXPoint;			//ң�Ų۰�����Ӧ��Ϣ
Point_Info   *pYKPoint;			//ң�ز۰�����Ӧ��Ϣ
/*******************local function***************************************/
static void Free_All_Mem( void );
static void Model_Read_Conf(char * fd,   LOCAL_Module  *  mod);
static void Model_Get_Dianbiao(char *  line,  LOCAL_Module  *  mod_p);
void Read_Faconfig_Conf(char * fd,  struct  _FAPRMETER_  *  fa);
//void Read_Sysconfig_Conf(char * fd,  struct  _SYSPARAME_ *  sys);


/***************************************************************************/
//����: void ipc_init( void )
//˵��:���ڴ���IPC
//����:
//���:
//�༭:shaoyi1110@126.com
//ʱ��:2015.4.17
/***************************************************************************/
UInt16 Ipc_Init( void )
{
  Int 	    status=0;

   SysLink_setup();   				    /* SysLink������ʼ��*/

    sys_mod.remoteProcId= MultiProc_getId("DSP");
    status = Ipc_control(sys_mod.remoteProcId, Ipc_CONTROLCMD_LOADCALLBACK, NULL); /* invoke the SysLink load callback */
   if (status < 0) {
        my_debug("Main_main: load callback failed\n");
        goto leave;
    }

    status = Ipc_control(sys_mod.remoteProcId, Ipc_CONTROLCMD_STARTCALLBACK, NULL); /* invoke the SysLink start callback */
    if (status < 0) {
        my_debug("Main_main: start callback failed\n");
        goto leave;
    	}

   return  0;

 leave:
 	Ipc_control(sys_mod.remoteProcId, Ipc_CONTROLCMD_STOPCALLBACK, NULL);
 	SysLink_destroy();

	return  -1;
}
/***************************************************************************/
//����:void ipc_exit( void )
//˵��:����destory IPC
//����: �ź��ж�ID
//���:
//�༭:shaoyi1110@126.com
//ʱ��:2015.4.17
/***************************************************************************/
void Ipc_Exit( void )
{
	Ipc_control(sys_mod.remoteProcId, Ipc_CONTROLCMD_STOPCALLBACK, NULL);
	SysLink_destroy();				    /* SysLink finalization */
}

/***************************************************************************/
//����˵��: �����ͷŵ����е���Դ
//
//����: ��
//���:
//�༭:shaoyi1110@126.com
//ʱ��:2015.4.23
/***************************************************************************/
static void Free_All_Mem( void )
{
	IEC104_Free_Buf();
	pthread_cancel(network_id);//�ص��߳�
	pthread_join(network_id, NULL);
	my_debug("network_id exit\n ");

	pthread_cancel(iec104_id);//�ص��߳�
	pthread_join(iec104_id, NULL);
	my_debug("iec104_id exit\n ");

	Sharearea_Destory(&sys_mod);
	pthread_cancel(message_id);//�ص��߳�
	pthread_join(message_id, NULL);
	my_debug("message_id exit\n ");


	IEC101_Free_Buf();
	pthread_cancel(iec101_id);//�ص��߳�
	pthread_join(iec101_id, NULL);
	my_debug("iec101_Thread exit\n ");

 	pthread_cancel(pc_id);//�ص��߳�
	pthread_join(pc_id, NULL);

	Ipc_Exit();
	my_debug("Main_Thread3 exit\n ");
	pthread_exit("Main_Thread exit\n");
}
/***************************************************************************/
//����˵��: ���ڷ�Ӧ�źŴ�����
//
//����: �ź��ж�ID
//���:
//�༭:shaoyi1110@126.com
//ʱ��:2015.4.17
/***************************************************************************/
static void Process_Signal(int type) /* �źŴ������̣�����dunno����õ��źŵ�ֵ */
{
    UInt8 buf[6];
	switch (type)
	{
		case 1:
			my_debug("Get a signal -- SIGHUP ");
		    break;
		case 2:
			my_debug("Get a signal -- SIGINT ");
			sys_mod.reset = DISENABLE;
			Free_All_Mem();
		    break;
		case 3:
			my_debug("Get a signal -- SIGQUIT ");
            break;
        case 14://�������ڼ�ʱ3S��
            buf[4] = 0xFF;
            buf[5] = 0xFF;
            if(sys_mod.pc_flag &(1<<MSG_DIANBIAO_YX_DQ))
                PC_Send_YX(buf,1);
            if(sys_mod.pc_flag &(1<<MSG_DIANBIAO_YC_DQ))
                PC_Send_YC(buf, 1);
            alarm(3);
		    break;
	}
return;
}

/***************************************************************************/
//����˵��: ϵͳ�ܺ������
//����:   argc   argv[]
//���:
//�༭:
//ʱ��:2015.8.17
/***************************************************************************/
Int main(Int argc, Char* argv[])
{
    Int status  = 0;

    SharedRegion_SRPtr sharearea_base;
    
    //�����û������������ź�
    signal(SIGINT, Process_Signal);
    signal(SIGALRM, Process_Signal);
    alarm(2);

    sys_mod.reset = ENABLE;

    //����IEC104��Ҫ�Ļ���
    status = IEC104_Init_Buf();
    if(status <0)
    {
		my_debug("Main_Thread exit\n");
		pthread_exit("Main_Thread exit\n");
    }
    //����IEC101��Ҫ�Ļ���
    status = IEC101_Init_Buf();
    if(status <0)
    {
    	IEC104_Free_Buf();
		my_debug("Main_Thread exit\n");
		pthread_exit("Main_Thread exit\n");
    }

    //������DSPͨѶ�Ļ���
    status = Ipc_Init();
    if(status <0)
	 goto leave;
    sharearea_base = Sharearea_Creat(&sys_mod);
    Model_Read_Conf(MODELCONFFILE,  &sys_mod);
    Read_Faconfig_Conf(MODELFAFILE, &pFAPARAM);
    Read_Sysconfig_Conf(MODELSYSFILE, &pSYSPARAM);
    Nand_Write_Config(MODELNANDFILE);
   //messageq�����߳�
    pthread_create (&message_id,  NULL,  Message_Thread,  &sys_mod);
   //��������߳�
    pthread_create (&network_id,  NULL,  Network_Thread, &sys_mod);
   //iec104�����߳�
    pthread_create (&iec104_id, NULL,  Iec104_Thread, &sys_mod);
   //iec101�����߳�
    pthread_create (&iec101_id, NULL,  Iec101_Thread, &sys_mod);
   //��λ�������߳�
    pthread_create (&pc_id, NULL,  PC_Creat_connection, &sys_mod);
    status =Uart_Thread();
    if(status<0)
		Free_All_Mem();
    exit(0);
    return 0;
 leave:

   return  -1;
}


/***************************************************************************/
//����: static void Model_Read_Conf(char * fd,  struct LOCAL_Module  *  mod)
//˵��:��ȡ���
//����:��
//���:��
//�༭:shaoyi1110@126.com
//ʱ��:2015.4.23
/***************************************************************************/
static void Model_Read_Conf(char * fd,   LOCAL_Module  *  mod)
{
 	FILE *fp=NULL;
 	char  line[51], i;
	unsigned  int  addr_tmp=0;
	Point_Info   * pTmp=NULL;

	mod->Continuity = 0;

	 fp = fopen(fd,  "rw");
	 if(fp == NULL){
			sys_mod.reset = DISENABLE;
			Free_All_Mem();
		}
	 fseek(fp, 0, SEEK_SET); 			//���¶��嵽��ʼλ��
	 while (fgets(line, 51, fp))
 	{
 		Model_Get_Dianbiao(line,  mod);
 	}
	  pTmp = mod->pYC_Addr_List_Head;
	 for(i=0; i<mod->usYC_Num-1; i++)
	 {
		//my_debug("type:%d addr:%d index:%d flag:0x%x name:%s", pTmp->ucType, pTmp->uiAddr, pTmp->uiOffset, pTmp->usFlag, pTmp->ucName);
		addr_tmp = pTmp->uiAddr;
	 	pTmp = pTmp->next;
		if(pTmp->uiAddr !=addr_tmp+1)
		{
			mod->Continuity |= YC_NOTCONTINUE;    //��������ַ���
		}
	 }

	  pTmp = mod->pYK_Addr_List_Head;
	 for(i=0; i<mod->usYK_Num-1; i++)
	 {
	 	addr_tmp = pTmp->uiAddr;
		//my_debug("type:%d addr:%d index:%d flag:0x%x name:%s", pTmp->ucType, pTmp->uiAddr, pTmp->uiOffset, pTmp->usFlag, pTmp->ucName);
	 	pTmp = pTmp->next;
		if(pTmp->uiAddr !=addr_tmp+1)
			mod->Continuity |= YK_NOTCONTINUE;    //��������ַ���
	 }


	  pTmp = mod->pYX_Addr_List_Head;
	 for(i=0; i<mod->usYX_Num-1; i++)
	 {
	 	addr_tmp = pTmp->uiAddr;
		//my_debug("type:%d addr:%d index:%d flag:0x%x name:%s", pTmp->ucType, pTmp->uiAddr, pTmp->uiOffset, pTmp->usFlag, pTmp->ucName);
	 	pTmp = pTmp->next;
		if(pTmp->uiAddr !=addr_tmp+1)
			mod->Continuity |= YX_NOTCONTINUE;    //��������ַ���
	 }
	 fclose(fp);
}
/***************************************************************************/
//����: void Model_Get_Dianbiao(char *  line, struct LOCAL_Module  *  mod_p)
//˵��:��ȡ����ÿһ��
//����:mod_p
//���:��
//�༭:shaoyi1110@126.com
//ʱ��:2015.4.23
/***************************************************************************/
static void Model_Get_Dianbiao(char *  line,  LOCAL_Module  *  mod_p)
{
	char * tmp_line=NULL;
	char i;
	static int yccnt=0, ykcnt=0, yxcnt=0;
	static int ycnum=0, yknum=0, yxnum=0;
       int type,addr,index,flag;
	char name[4]="";
	Point_Info   * pTmp=NULL;
	Point_Info   * pTmp_Pre=NULL;

	tmp_line=(char * )strstr((char *)line, "type:");
	if(tmp_line!=NULL)
	{
	    sscanf(tmp_line, "type:%d addr:%d index:%d  flag:%d name:%s", &type, &addr, &index, &flag ,name);
	    if((type>=TYPE_UAB)&&(type<=TYPE_OAB)&&(yccnt<ycnum))//ң��
	    {
		 (pYCPoint+yccnt)->ucType = type;
		 (pYCPoint+yccnt)->uiAddr = addr;
		 (pYCPoint+yccnt)->uiOffset = index;
		 (pYCPoint+yccnt)->usFlag = flag;
		 memcpy((pYCPoint+yccnt)->ucName, name, 4);
		    if(yccnt==0)
		    {
			mod_p->pYC_Addr_List_Head = pYCPoint+yccnt;
			(pYCPoint+yccnt)->next = NULL;
		    }
		    else
		    {
			pTmp = mod_p->pYC_Addr_List_Head;
			pTmp_Pre = pTmp;
			for(i=0; i<yccnt; i++)
			{
				if(pTmp->uiAddr>addr)
					break;
				else
					pTmp_Pre = pTmp;
				pTmp = pTmp_Pre->next;
			}
			if(i==0)			//���뵽����ͷ
			{
			 	(pYCPoint+yccnt)->next =  mod_p->pYC_Addr_List_Head;
				mod_p->pYC_Addr_List_Head = pYCPoint+yccnt;
			}
			else
			{
				pTmp_Pre->next = pYCPoint+yccnt;
				(pYCPoint+yccnt)->next = pTmp;
			}
		     }
		   yccnt++;
	      }
	      else if((type==TYPE_YK)&&(ykcnt<yknum))//ң��
	      {
		 (pYKPoint+ykcnt)->ucType = type;
		 (pYKPoint+ykcnt)->uiAddr = addr;
		 (pYKPoint+ykcnt)->uiOffset = index;					//**************************************//
		 (pYKPoint+ykcnt)->usFlag = flag;
		  memcpy((pYKPoint+ykcnt)->ucName, name, 4);
		    if(ykcnt==0)
		    {
			mod_p->pYK_Addr_List_Head = pYKPoint+ykcnt;
			(pYKPoint+ykcnt)->next = NULL;
		    }
		    else
		    {
			pTmp = mod_p->pYK_Addr_List_Head;
			pTmp_Pre = pTmp;
			for(i=0; i<ykcnt; i++)
			{
				if(pTmp->uiAddr>addr)
					break;
				else
					pTmp_Pre = pTmp;
				pTmp = pTmp_Pre->next;
			}
			if(i==0)			//���뵽����ͷ
			{
			 	(pYKPoint+ykcnt)->next =  mod_p->pYK_Addr_List_Head;
				mod_p->pYK_Addr_List_Head = pYKPoint+ykcnt;
			}
			else
			{
				pTmp_Pre->next = pYKPoint+ykcnt;
				(pYKPoint+ykcnt)->next = pTmp;
			}
		     }
		   ykcnt++;
		}
		else if((type==TYPE_YX)&&(yxcnt<yxnum))//ң��
		{
		 (pYXPoint+yxcnt)->ucType = type;
		 (pYXPoint+yxcnt)->uiAddr = addr-1;
		 (pYXPoint+yxcnt)->uiOffset = index-1;
		 (pYXPoint+yxcnt)->usFlag = flag;//bit[0]-SOEʹ��   bit[1]-COSʹ��
		    Write_To_Sharearea(index-1, TYPE_YX_ENABLE_BIT, 1);//д��ң��ʹ����
		 if(flag&0x01)
            Write_To_Sharearea(index-1, TYPE_YX_SOE_ENABLE_BIT, 1);//д��SOE��
		 if(flag&0x02)
            Write_To_Sharearea(index-1, TYPE_YX_COS_ENABLE_BIT, 1);//д��COS��
		 memcpy((pYXPoint+yxcnt)->ucName, name, 4);
		    if(yxcnt==0)
		    {
			mod_p->pYX_Addr_List_Head = pYXPoint+yxcnt;
			(pYXPoint+yxcnt)->next = NULL;
		    }
		    else
		    {
			pTmp = mod_p->pYX_Addr_List_Head;
			pTmp_Pre = pTmp;
			for(i=0; i<yxcnt; i++)
			{
				if(pTmp->uiAddr>addr)
					break;
				else
					pTmp_Pre = pTmp;
				pTmp = pTmp_Pre->next;
			}
			if(i==0)			//���뵽����ͷ
			{
			 	(pYXPoint+yxcnt)->next =  mod_p->pYX_Addr_List_Head;
				mod_p->pYX_Addr_List_Head = pYXPoint+yxcnt;
			}
			else
			{
				pTmp_Pre->next = pYXPoint+yxcnt;
				(pYXPoint+yxcnt)->next = pTmp;
			}
		     }
		  yxcnt++;
		}
		return ;
	}

	tmp_line=NULL;
	tmp_line=(char * )strstr((char *)line, "YC:");
	if(tmp_line!=NULL)
	{
		sscanf(tmp_line, "YC:%d YK:%d YX:%d",(int *) &ycnum, (int *)&yknum, (int *)&yxnum);
		if(ycnum>=1)//ң�����ռ�
		{
			mod_p->usYC_Num = ycnum;
			pYCPoint = malloc(sizeof(Point_Info)*mod_p->usYC_Num);
			memset(pYCPoint, 0 , sizeof(Point_Info)*mod_p->usYC_Num);
		}
		else
			mod_p->usYC_Num = 0 ;

		if(yknum>=1)//ң�ط���ռ�
		{
			mod_p->usYK_Num = yknum;
			pYKPoint = malloc(sizeof(Point_Info)*mod_p->usYK_Num);
			memset(pYKPoint, 0, sizeof(Point_Info)*mod_p->usYK_Num);
		}
		else
			mod_p->usYK_Num = 0;

		if(yxnum>=1)//ң�ŷ���ռ�
		{
			mod_p->usYX_Num = yxnum;
			pYXPoint = malloc(sizeof(Point_Info)*mod_p->usYX_Num);
			memset(pYXPoint, 0 ,sizeof(Point_Info)*mod_p->usYX_Num);
		}
		else
			mod_p->usYX_Num = 0;
		my_debug("YC:%d YK:%d YX:%d", ycnum, yknum, yxnum);
	}
}
