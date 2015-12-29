/*************************************************************************/
// arm_main.c                                       Version 1.00
//
// 本文件是DTU2.0通讯网关装置的主程序
//实现功能: 4块遥测板、每个板上3U9I、最大128个遥信量
//、128个遥控量实现这些资源的配置和保护
// 编写人:shaoyi       email:shaoyi1110@126.com
//  日	   期:2015.04.17
//  注	   释:
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
Point_Info   *pYCPoint;			//遥测槽板点号相应信息
Point_Info   *pYXPoint;			//遥信槽板点号相应信息
Point_Info   *pYKPoint;			//遥控槽板点号相应信息
/*******************local function***************************************/
static void Free_All_Mem( void );
static void Model_Read_Conf(char * fd,   LOCAL_Module  *  mod);
static void Model_Get_Dianbiao(char *  line,  LOCAL_Module  *  mod_p);
void Read_Faconfig_Conf(char * fd,  struct  _FAPRMETER_  *  fa);
//void Read_Sysconfig_Conf(char * fd,  struct  _SYSPARAME_ *  sys);


/***************************************************************************/
//函数: void ipc_init( void )
//说明:用于创建IPC
//输入:
//输出:
//编辑:shaoyi1110@126.com
//时间:2015.4.17
/***************************************************************************/
UInt16 Ipc_Init( void )
{
  Int 	    status=0;

   SysLink_setup();   				    /* SysLink建立初始化*/

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
//函数:void ipc_exit( void )
//说明:用于destory IPC
//输入: 信号中断ID
//输出:
//编辑:shaoyi1110@126.com
//时间:2015.4.17
/***************************************************************************/
void Ipc_Exit( void )
{
	Ipc_control(sys_mod.remoteProcId, Ipc_CONTROLCMD_STOPCALLBACK, NULL);
	SysLink_destroy();				    /* SysLink finalization */
}

/***************************************************************************/
//函数说明: 用于释放掉所有的资源
//
//输入: 无
//输出:
//编辑:shaoyi1110@126.com
//时间:2015.4.23
/***************************************************************************/
static void Free_All_Mem( void )
{
	IEC104_Free_Buf();
	pthread_cancel(network_id);//关掉线程
	pthread_join(network_id, NULL);
	my_debug("network_id exit\n ");

	pthread_cancel(iec104_id);//关掉线程
	pthread_join(iec104_id, NULL);
	my_debug("iec104_id exit\n ");

	Sharearea_Destory(&sys_mod);
	pthread_cancel(message_id);//关掉线程
	pthread_join(message_id, NULL);
	my_debug("message_id exit\n ");


	IEC101_Free_Buf();
	pthread_cancel(iec101_id);//关掉线程
	pthread_join(iec101_id, NULL);
	my_debug("iec101_Thread exit\n ");

 	pthread_cancel(pc_id);//关掉线程
	pthread_join(pc_id, NULL);

	Ipc_Exit();
	my_debug("Main_Thread3 exit\n ");
	pthread_exit("Main_Thread exit\n");
}
/***************************************************************************/
//函数说明: 用于反应信号处理函数
//
//输入: 信号中断ID
//输出:
//编辑:shaoyi1110@126.com
//时间:2015.4.17
/***************************************************************************/
static void Process_Signal(int type) /* 信号处理例程，其中dunno将会得到信号的值 */
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
        case 14://闹钟用于计时3S钟
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
//函数说明: 系统总函数入口
//输入:   argc   argv[]
//输出:
//编辑:
//时间:2015.8.17
/***************************************************************************/
Int main(Int argc, Char* argv[])
{
    Int status  = 0;

    SharedRegion_SRPtr sharearea_base;
    
    //创建用户重新启动的信号
    signal(SIGINT, Process_Signal);
    signal(SIGALRM, Process_Signal);
    alarm(2);

    sys_mod.reset = ENABLE;

    //创建IEC104需要的缓冲
    status = IEC104_Init_Buf();
    if(status <0)
    {
		my_debug("Main_Thread exit\n");
		pthread_exit("Main_Thread exit\n");
    }
    //创建IEC101需要的缓冲
    status = IEC101_Init_Buf();
    if(status <0)
    {
    	IEC104_Free_Buf();
		my_debug("Main_Thread exit\n");
		pthread_exit("Main_Thread exit\n");
    }

    //创建和DSP通讯的基础
    status = Ipc_Init();
    if(status <0)
	 goto leave;
    sharearea_base = Sharearea_Creat(&sys_mod);
    Model_Read_Conf(MODELCONFFILE,  &sys_mod);
    Read_Faconfig_Conf(MODELFAFILE, &pFAPARAM);
    Read_Sysconfig_Conf(MODELSYSFILE, &pSYSPARAM);
    Nand_Write_Config(MODELNANDFILE);
   //messageq接收线程
    pthread_create (&message_id,  NULL,  Message_Thread,  &sys_mod);
   //网络接收线程
    pthread_create (&network_id,  NULL,  Network_Thread, &sys_mod);
   //iec104处理线程
    pthread_create (&iec104_id, NULL,  Iec104_Thread, &sys_mod);
   //iec101处理线程
    pthread_create (&iec101_id, NULL,  Iec101_Thread, &sys_mod);
   //上位机处理线程
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
//函数: static void Model_Read_Conf(char * fd,  struct LOCAL_Module  *  mod)
//说明:读取点表
//输入:无
//输出:无
//编辑:shaoyi1110@126.com
//时间:2015.4.23
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
	 fseek(fp, 0, SEEK_SET); 			//重新定义到开始位置
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
			mod->Continuity |= YC_NOTCONTINUE;    //不连续地址标记
		}
	 }

	  pTmp = mod->pYK_Addr_List_Head;
	 for(i=0; i<mod->usYK_Num-1; i++)
	 {
	 	addr_tmp = pTmp->uiAddr;
		//my_debug("type:%d addr:%d index:%d flag:0x%x name:%s", pTmp->ucType, pTmp->uiAddr, pTmp->uiOffset, pTmp->usFlag, pTmp->ucName);
	 	pTmp = pTmp->next;
		if(pTmp->uiAddr !=addr_tmp+1)
			mod->Continuity |= YK_NOTCONTINUE;    //不连续地址标记
	 }


	  pTmp = mod->pYX_Addr_List_Head;
	 for(i=0; i<mod->usYX_Num-1; i++)
	 {
	 	addr_tmp = pTmp->uiAddr;
		//my_debug("type:%d addr:%d index:%d flag:0x%x name:%s", pTmp->ucType, pTmp->uiAddr, pTmp->uiOffset, pTmp->usFlag, pTmp->ucName);
	 	pTmp = pTmp->next;
		if(pTmp->uiAddr !=addr_tmp+1)
			mod->Continuity |= YX_NOTCONTINUE;    //不连续地址标记
	 }
	 fclose(fp);
}
/***************************************************************************/
//函数: void Model_Get_Dianbiao(char *  line, struct LOCAL_Module  *  mod_p)
//说明:读取点表的每一行
//输入:mod_p
//输出:无
//编辑:shaoyi1110@126.com
//时间:2015.4.23
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
	    if((type>=TYPE_UAB)&&(type<=TYPE_OAB)&&(yccnt<ycnum))//遥测
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
			if(i==0)			//插入到链表头
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
	      else if((type==TYPE_YK)&&(ykcnt<yknum))//遥控
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
			if(i==0)			//插入到链表头
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
		else if((type==TYPE_YX)&&(yxcnt<yxnum))//遥信
		{
		 (pYXPoint+yxcnt)->ucType = type;
		 (pYXPoint+yxcnt)->uiAddr = addr-1;
		 (pYXPoint+yxcnt)->uiOffset = index-1;
		 (pYXPoint+yxcnt)->usFlag = flag;//bit[0]-SOE使能   bit[1]-COS使能
		    Write_To_Sharearea(index-1, TYPE_YX_ENABLE_BIT, 1);//写入遥信使能区
		 if(flag&0x01)
            Write_To_Sharearea(index-1, TYPE_YX_SOE_ENABLE_BIT, 1);//写入SOE区
		 if(flag&0x02)
            Write_To_Sharearea(index-1, TYPE_YX_COS_ENABLE_BIT, 1);//写入COS区
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
			if(i==0)			//插入到链表头
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
		if(ycnum>=1)//遥测分配空间
		{
			mod_p->usYC_Num = ycnum;
			pYCPoint = malloc(sizeof(Point_Info)*mod_p->usYC_Num);
			memset(pYCPoint, 0 , sizeof(Point_Info)*mod_p->usYC_Num);
		}
		else
			mod_p->usYC_Num = 0 ;

		if(yknum>=1)//遥控分配空间
		{
			mod_p->usYK_Num = yknum;
			pYKPoint = malloc(sizeof(Point_Info)*mod_p->usYK_Num);
			memset(pYKPoint, 0, sizeof(Point_Info)*mod_p->usYK_Num);
		}
		else
			mod_p->usYK_Num = 0;

		if(yxnum>=1)//遥信分配空间
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
