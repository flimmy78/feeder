/*************************************************************************/
// sharearea.c                                 Version 1.00
// 描述
//
// 本文件是DTU通讯网关装置的CPU之间的共享内存
// 编写人:shaoyi
//
//  email		  :shaoyi1110@126.com
//  日	   期:2014.9.16
//  注	   释:
/*************************************************************************/
/* host header files */
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
/*Linux header files*/
#include <sys/ioctl.h>
#include <net/if.h>
#include <pthread.h>
#include   <sys/time.h>
#include <signal.h>
#include <semaphore.h>
/* package header files */
#include <ti/syslink/Std.h>     /* must be first */
#include <ti/syslink/utils/Memory.h>
#include <ti/syslink/IpcHost.h>
#include <ti/syslink/SysLink.h>
#include <ti/ipc/Notify.h>
#include <ti/ipc/SharedRegion.h>
#include <ti/ipc/MultiProc.h>
#include <ti/ipc/HeapBufMP.h>
#include <ti/ipc/MessageQ.h>
/* local header files */
#include "common.h"
#include "sharearea.h"
#include "iec104.h"
#include "iec101.h"
#include "readconfig.h"
#include "readsysconfig.h"
#define CHECKPARMFILE               "/config/checkparm.txt"
static  int Check_Write_Param(char * file);
static void Check_Read_Param(int file);
static void Write_Check_Sharearea(UInt8 flag, UInt32  *data);

UInt32            INDEX=0;
unsigned int checkfd;
/*******************extern various***************************************/

/*******************local struct***************************************/
typedef struct {
    MessageQ_MsgHeader  reserved;
    UInt32              cmd;
    UInt32              buf;
    UInt32              data;
} App_Msg;

typedef struct
{
    UInt32             queue[QUEUESIZE];   /* command queue */
    UInt                head;               	    /*queue head pointer */
    UInt                tail;               		    /* queue tail pointer */
    UInt32             error;              	    /* error flag */
    sem_t              semH;               	   /* handle to object above  */
} Event_Queue;

typedef struct {
    Event_Queue     eventQueue;
    UInt16          lineId;     /* notify line id */
    UInt32          eventId;    /* notify event id */
    Char*           bufferPtr;  /* string buffer */
} App_Module;


typedef struct {
    sem_t                 semH;
    UInt32                eventQue[MESSAGE_QUEUESIZE];
    UInt                    head;       // queue head pointer
    UInt                    tail;       // queue tail pointer
    UInt32                  error;
    HeapBufMP_Handle        msgHeap;    // created locally
    MessageQ_Handle         hostQue;    // created locally
    MessageQ_QueueId        videoQue;   // opened remotely
    UInt16                  heapId;     // MessageQ heapId
    UInt32                  msgSize;
} Message_Module;


/*******************local various***************************************/
static Message_Module message_mod;
static App_Module Module;
/*******************local function***************************************/
static UInt32 App_waitForEvent(Event_Queue* eventQueue);
static Void App_NotifyCB( UInt16 procId, UInt16 lineId, UInt32 eventId, UArg arg, UInt32 payload);

/***************************************************************************/
//函数说明:创建共享区
//输入:
//输出:共享区基地址
//编辑:shaoyi1110@126.com
//时间:2015.4.17
/***************************************************************************/
SharedRegion_SRPtr Sharearea_Creat(  LOCAL_Module * mod_p)
{
    int 	retStatus;
    Int     status = 0;
    IHeap_Handle heap;
    SharedRegion_SRPtr  sharedBufferPtr = 0;
    UInt32              command  = 0;
    UInt32              event           = 0;

    //为NOTIFY做准备
    Module.eventQueue.head      = 0;
    Module.eventQueue.tail      = 0;
    Module.eventQueue.error     = 0;
    Module.lineId                 = SystemCfg_LineId;
    Module.eventId              = SystemCfg_EventId;

    //初始化NOTIFY接收信号
    retStatus = sem_init(&Module.eventQueue.semH, 0, 0);
    if(retStatus == -1) {
        my_debug("App_create: Could not initialize a semaphore\n");
        status = -1;
        goto leave2;
    }
    //注册NOTIFY中断处理事件
    status = Notify_registerEvent(mod_p->remoteProcId , Module.lineId,  Module.eventId, App_NotifyCB, (UArg) &Module.eventQueue);

    if (status < 0) {
        my_debug("App_create: Host failed to register event\n");
        goto leave3;
    }
    //等待DSP也注册了NOTIFY
    do {
        status = Notify_sendEvent(mod_p->remoteProcId, Module.lineId, Module.eventId, APP_CMD_NOP, TRUE);

        if (status == Notify_E_EVTNOTREGISTERED) {
            usleep(100);
        }

    } while (status == Notify_E_EVTNOTREGISTERED);
    if (status != Notify_S_SUCCESS) {
        my_debug("App_create: Waiting for remote core to register has failed\n");
        goto leave3;
    }

    //从SHARED_REGION_1获得堆
    heap = (IHeap_Handle) SharedRegion_getHeap(SHARED_REGION_1);
    if (heap == NULL) {
        my_debug("App_create: Shared region heap does not exist\n");
        goto leave3;
    }

    //从heap中分配的空间，如果heap是共享区，那这个就是分配共享区
    Module.bufferPtr = (Char *) Memory_calloc(heap, SHAREAREA_SIZE, 0, NULL);

    if(Module.bufferPtr == NULL) {
        my_debug("App_create: Failed to create buffer from shared region 1 "
                "heap\n");
        goto leave4;
    }
    sprintf(Module.bufferPtr,"%s", "mytest");
    sharedBufferPtr = SharedRegion_getSRPtr(Module.bufferPtr,  SHARED_REGION_1);
    my_debug("sharedBufferPtr:0x%8x\n", sharedBufferPtr);

    //发送共享地址低16位到DSP
   command = (sharedBufferPtr & 0xFFFF);
   command = APP_SPTR_LADDR | command;
   status = Notify_sendEvent(mod_p->remoteProcId, Module.lineId, Module.eventId, command, TRUE);
    if (status < 0 )  {
        my_debug("App_exec: Error sending shared region pointer address lower two bytes\n");
        goto leave4;
    }
    //等待收到命令
    event = App_waitForEvent(&Module.eventQueue);
    if (event >= APP_E_FAILURE) {
        my_debug("App_exec: Received queue error: %d\n",event);
        goto leave4;
    }

    //发送共享地址高16位到DSP
    command = ((sharedBufferPtr >> 16) & 0xFFFF);
    command = APP_SPTR_HADDR | command;
    status = Notify_sendEvent(mod_p->remoteProcId, Module.lineId, Module.eventId, command, TRUE);
    if (status < 0 )  {
        my_debug("App_exec: Error sending shared region pointer address upper two bytes\n");
        goto leave4;
    }

    //等待收到命令
    event = App_waitForEvent(&Module.eventQueue);
    if (event >= APP_E_FAILURE) {
        my_debug("App_exec: Received queue error: %d\n", event);
        goto leave4;
    }
    //等待操作完成命令
    event = App_waitForEvent(&Module.eventQueue);

    if (event >= APP_E_FAILURE) {
        my_debug("App_exec: Received queue error: %d\n",event);
        goto leave4;
    }
    my_debug("App_exec: Transformed string: %s\n",  Module.bufferPtr);
    memset((char *)Module.bufferPtr,0, 3400);//初始化遥测区和遥信区
    //Write_To_Sharearea(0, TYPE_YC, 100);
    //Write_To_Sharearea(1, TYPE_YC, -710);
    //Write_To_Sharearea(2, TYPE_YC, 40);
    //Write_To_Sharearea(3, TYPE_YC, 45);
    //Write_To_Sharearea(4, TYPE_YC, 41);
    //Write_To_Sharearea(5, TYPE_YC, -43);
    return sharedBufferPtr;

 leave4:
	    Memory_free(heap, Module.bufferPtr, SHAREAREA_SIZE);
 leave3:
 	     Notify_unregisterEvent(mod_p->remoteProcId, Module.lineId,  Module.eventId, App_NotifyCB, (UArg) &Module.eventQueue);
 leave2:
 	 	sem_destroy(&Module.eventQueue.semH);
   return -1;
}

/***************************************************************************/
//函数说明:从共享区读取值
//输入:   base:YC YK YX都有自己独立的base， offset共享区的偏移地址
//输出:对于遥测量每次读取一个UInt32，对于遥信量每次读取一个UInt8
//编辑:shaoyi1110@126.com
//时间:2015.4.20
/***************************************************************************/
float Read_From_Sharearea( UInt16 index, UInt8 flag )
{

	//int ret=0;
	float ret=0;
    float test;

	if(flag == TYPE_YC)//每个遥测量暂用8个字节，前4个字节表示数据，后4个字节预留
	{
	    //ret = *((int *)(Module.bufferPtr+YC_BASE+(index<<3)));
        ret = *((float *)(Module.bufferPtr+YC_BASE+(index<<3)));
		//my_debug("ret is :%d",ret);
		return ret;
	}
    else if(flag == TYPE_YC_B)
    {
        //ret = *((int *)(Module.bufferPtr+YC_BASE+4+(index<<3)));
        ret = *((float *)(Module.bufferPtr+YC_BASE+4+(index<<3)));
		return ret;
    }
	else if(flag == TYPE_YX_STATUS_BIT)//每个遥信量暂用一个bit，每次读取一个字节
	{
		return (Module.bufferPtr+YX_STATUS_BASE)[index>>3];
	}
    else if(flag ==TYPE_YC_CHECK_VALUE )//存储校准值，每通道1字节
        //ret = *(Module.bufferPtr+YC_CHECK_VALUE_BASE+index);
		ret = *((float *)(Module.bufferPtr+YC_CHECK_VALUE_BASE+index));
    else if(flag ==TYPE_YC_CHECK_PAR )//存储校准参数，每通道4字节
    {
        //memcpy((UInt8 *)&ret, (UInt8 *)(Module.bufferPtr+YC_CHECK_PAR_BASE+(index<<2)), 4 );
        test = *((float *)(Module.bufferPtr+YC_CHECK_PAR_BASE+(index<<2)));
        my_debug("test:%.2f",test);
        memcpy((UInt8 *)&ret,(UInt8 *)&test, 4);
    }
       
	return ret;
}
/***************************************************************************/
//函数说明:将数据写入共享区
//输入:   base:YC YK YX都有自己独立的base， index共享区的偏移地址
//		     data将要写入的数据
//输出:
//编辑:shaoyi1110@126.com
//时间:2015.4.20
/***************************************************************************/
void   Write_To_Sharearea(UInt16 index, UInt8 flag, UInt32  data)
{
	int ret=0;
    UInt16 base=0;
    float test;
    if(flag == TYPE_YC)
    {
        *((UInt32 *)(Module.bufferPtr+YC_BASE+(index<<3)))= data;
    }
    else if(flag == TYPE_YC_B)
    {
        *((UInt32 *)(Module.bufferPtr+YC_BASE+4+(index<<3)))= data;
    }
    else if(flag ==TYPE_YC_CHECK_VALUE )//存储校准值，每通道1字节
    {
        
        //*(Module.bufferPtr+YC_CHECK_VALUE_BASE+index) = data;
        *((float *)(Module.bufferPtr+YC_CHECK_VALUE_BASE+index)) = (float)data;
        
    }
    else if(flag ==TYPE_YC_CHECK_PAR )//存储校准参数，每通道4字节
    {
        //*((UInt32 *)(Module.bufferPtr+YC_CHECK_PAR_BASE+(index<<2)))= data;
        memcpy((UInt8 *)&test,(UInt8 *)&data, 4);
        *((float *)(Module.bufferPtr+YC_CHECK_PAR_BASE+(index<<2)))= test;
        my_debug("write data to shareration:%.2f",test);
        my_debug("INDEX:%d",index);
    }
    else
    {
        if(flag == TYPE_YX_STATUS_BIT)//向共享区写入遥信状态
                base = YX_STATUS_BASE;
        else if(flag == TYPE_YX_ENABLE_BIT)//向共享区写入遥信使能
                base = YX_ENABLE_BASE;
        else if(flag == TYPE_YX_COS_ENABLE_BIT)//向共享区写入COS使能
                base = YX_COS_BASE;
        else if(flag == TYPE_YX_SOE_ENABLE_BIT)//向共享区写入SOE使能
                base = YX_SOE_BASE;
        else if(flag == TYPE_YX_RETURN_BIT)//向共享区写入反转使能
                base = YX_REVERSE_BASE;

        if(data !=0)
            (Module.bufferPtr+base)[index>>3] |= (1<<(index&0x7));
        else
            (Module.bufferPtr+base)[index>>3] &=~(1<<(index&0x7));
    }
}


/***************************************************************************/
//函数说明:将数据写入共享区
//输入:   base:YC YK YX FA都有自己独立的base， 
//		     data将要写入的数据
//输出:
//编辑:work
//时间:2015.8.20
/**************************************************************************/

void   Write_Fa_Sharearea(struct _FAPRMETER_ * item)
{
    *((FAPARAM *)(Module.bufferPtr+FA_CONFIG_BASE))= *item;
}

/***************************************************************************/
//函数说明:将数据写入共享区
//输入:   base:SYS都有自己独立的base， 
//		     data将要写入的数据
//输出:
//编辑:work
//时间:2015.8.20
/***************************************************************************/

void Write_Sys_Sharearea(struct _SYSPARAME_ * item)
{
    *((SYSDSPREAD *)(Module.bufferPtr+SYS_CONFIG_BASE))= item->DspReadData;
}

/***************************************************************************/
//函数说明:销毁共享区
//输入:
//输出:
//编辑:shaoyi1110@126.com
//时间:2015.4.17
/***************************************************************************/
void Sharearea_Destory( LOCAL_Module * mod_p )
{
     IHeap_Handle heap;
     int status = -1;
     UInt32 event = 0;

    // sending shutdown command
    status = Notify_sendEvent(mod_p->remoteProcId, Module.lineId,  Module.eventId,  APP_CMD_SHUTDOWN, TRUE);
    if (status < 0 )  {
        my_debug("App_delete: Error sending shutdown command\n");
    }

    // wait for shutdown acknowledge command
    event = App_waitForEvent(&Module.eventQueue);

    if (event >= APP_E_FAILURE) {
        my_debug("App_delete: Received queue error: %d\n", event);
    }

    MessageQ_delete(&message_mod.hostQue);
    MessageQ_unregisterHeap(App_MsgHeapId);
    HeapBufMP_delete(&message_mod.msgHeap);

    Notify_unregisterEvent(mod_p->remoteProcId,  Module.lineId,  Module.eventId, App_NotifyCB, (UArg) &Module.eventQueue);

    heap = (IHeap_Handle) SharedRegion_getHeap(SHARED_REGION_1);

    Memory_free(heap, Module.bufferPtr, SHAREAREA_SIZE);

    sem_destroy(&Module.eventQueue.semH);
}

/***************************************************************************/
//函数:Void App_notifyCB(UInt16 procId, UInt16 lineId, UInt32 eventId, UArg arg,  UInt32 payload)
//说明:NOTIFY中断处理函数
//输入:
//输出:共享区基地址
//编辑:shaoyi1110@126.com
//时间:2015.4.17
/***************************************************************************/
static Void App_NotifyCB(UInt16 procId, UInt16 lineId, UInt32 eventId, UArg arg,  UInt32 payload)
{
    UInt            next;
    Event_Queue*    eventQueue = (Event_Queue *) arg;

    //忽略空事件
    if (payload == APP_CMD_NOP) {
        return;
    }

    //计算队列中下一个slot
    next = (eventQueue->head + 1) % QUEUESIZE;

    if (next == eventQueue->tail) {
        //队列如果满了，置溢出位
        eventQueue->error = APP_E_OVERFLOW;
    }
    else {
        //将消息放进队列
        eventQueue->queue[eventQueue->head] = payload;
        eventQueue->head = next;

        //告诉等待消息的线程
        sem_post(&eventQueue->semH);
    }

    return;
}


/***************************************************************************/
//函数:static UInt32 App_waitForEvent(Event_Queue* eventQueue)
//说明:NOTIFY真正处理数据的地方
//输入:
//输出:共享区基地址
//编辑:shaoyi1110@126.com
//时间:2015.4.17
/***************************************************************************/

static UInt32 App_waitForEvent(Event_Queue* eventQueue)
{
    UInt32 event;

    if (eventQueue->error >= APP_E_FAILURE) {
        event = eventQueue->error;
    }
    else {
        /* use counting semaphore to wait for next event */
        sem_wait(&eventQueue->semH);

        /* remove next command from queue */
        event = eventQueue->queue[eventQueue->tail];

        /* queue tail is only written to within this function */
        eventQueue->tail = (eventQueue->tail + 1) % QUEUESIZE;
    }
    return (event);
}

/***************************************************************************/
//函数: void *message_thread(void * arg)
//说明:IPC通讯的线程，多用于接收
//输入:
//输出:
//编辑:R&N
//时间:2015.10.15
/***************************************************************************/
void *Message_Thread(void * arg)
{
    float                 status    =0;
    UInt32              event     =0;
    HeapBufMP_Params    heapParams;
    MessageQ_Params     msgqParams;
    UInt8  * sendbuf_p;
    float               data_test;
    App_Msg *   msg;
    LOCAL_Module *  mod_p= (LOCAL_Module *)arg;
    checkfd = Check_Write_Param(CHECKPARMFILE);
    Check_Read_Param(checkfd);
    //分配堆空间64*10
    HeapBufMP_Params_init(&heapParams);
    heapParams.name = App_MsgHeapName;
    heapParams.regionId = App_MsgHeapSrId;
    heapParams.blockSize = 64;
    heapParams.numBlocks = 10;
    message_mod.msgHeap = HeapBufMP_create(&heapParams);
    if (message_mod.msgHeap == NULL) {
        my_debug("App_create: Failed to create a HeapBufMP\n");
        goto leave1;
    }
    //先分配heap，然后在heap中分配msg，通过App_MsgHeapId来找到heap
    status = MessageQ_registerHeap((Ptr)(message_mod.msgHeap), App_MsgHeapId);
    if (status < 0) {
        my_debug("App_create: Failed to register HeapBufMP with MessageQ\n");
        goto leave2;
	}

    //创建ARM端消息队列
    MessageQ_Params_init(&msgqParams);
    message_mod.hostQue = MessageQ_create(App_HostMsgQueName, &msgqParams);//返回消息队列ID(name，参数)
    if (message_mod.hostQue == NULL) {
        my_debug("App_create: Failed creating MessageQ\n");
        goto leave3;
    }
    //发送ready事件
    status = Notify_sendEvent(mod_p->remoteProcId, Module.lineId, Module.eventId, APP_CMD_RESRDY, TRUE);

    if (status < 0) {
        my_debug("App_create: Failed to send event\n");
        goto leave3;
    }

    //等待DSP端ready事件
    do {
        event = App_waitForEvent(&Module.eventQueue);
        if (event >= APP_E_FAILURE) {
            my_debug("App_create: Failed waiting for event\n");
            goto leave3;
        }
    } while (event != APP_CMD_RESRDY);

    //打开DSP端消息队列
    status = MessageQ_open(App_VideoMsgQueName, &message_mod.videoQue);
    if (status < 0) {
        my_debug("App_create: Failed opening MessageQ\n");
        goto leave3;
    }

    //发送打开成功标记
    status = Notify_sendEvent(mod_p->remoteProcId, Module.lineId, Module.eventId, APP_CMD_READY, TRUE);
    my_debug("Module.eventId:%d",Module.eventId);
    if (status < 0) {
        my_debug("App_create: Failed to send event\n");
        goto leave3;
    }

    //等待DSP端的READY标记
    do {
        event = App_waitForEvent(&Module.eventQueue);
        if (event >= APP_E_FAILURE) {
            my_debug("App_create: Failed waiting for event\n");
            goto leave3;
        }
    } while (event != APP_CMD_READY);
    
while(1)
{
//    Send_Event_By_Message(MSG_PRINTF,0, 0);
        status = MessageQ_get(message_mod.hostQue, (MessageQ_Msg *)&msg, MessageQ_FOREVER);//从确定的消息队列中取数据
	if(status < 0)
		pthread_exit("Message_Thread exit\n");

    switch ((msg->cmd)&0xFFFF)
    {
        case MSG_EXIT://接收到DSP断电命令
            MessageQ_free((MessageQ_Msg)msg);
            pthread_exit("Message_Thread exit\n");
            break;
        case MSG_PRINTF://接收到DSP打印命令，用于DSP调试
    		my_debug("DSP:%s",  Module.bufferPtr+PRINTF_BASE);
            break;
        case MSG_COS:
        case MSG_SOE:
            //my_debug("Receive SOE or COS from Dsp");
            IEC101_Process_Message(msg->cmd, msg->buf, msg->data);
            IEC104_Process_Message(msg->cmd, msg->buf, msg->data);
            break;
        case MSG_JIAOZHUN://校准回复命令 msg-->buf放置index  msg-->data放置校准值
            sendbuf_p = malloc(8);
            sendbuf_p[0] = (msg->buf)&0xFF;
            sendbuf_p[1] = (msg->buf>>8)&0xFF;
            sendbuf_p[2] =  msg->data&0xFF;   //校准值
            //sendbuf_p[7] =  '\n';   //校准值
            my_debug("sendbuf_p[2]:%d",sendbuf_p[2]);
            status = Read_From_Sharearea(msg->buf, TYPE_YC_CHECK_PAR);//校准参数
            INDEX = msg->buf;
            my_debug("status:%d INDEX:%d",(Int)status,INDEX);
            memcpy((UInt8 *)&data_test, (UInt8 *)&status, 4);
            my_debug("status:%.2f",data_test);
            //memcpy(&sendbuf_p[3], (char *)&status, 4);
            memcpy(&sendbuf_p[3], (UInt8 *)&data_test, 4);
            PC_Send_Data(MSG_CHECK_U, sendbuf_p, 7);
            lseek(checkfd,0,SEEK_SET);
            write(checkfd,(UInt8 *)(Module.bufferPtr+YC_CHECK_PAR_BASE),80);
            free(sendbuf_p);
        default:
            break;
    }
	MessageQ_free((MessageQ_Msg)msg);
}
    //跳出线程，线程结束
leave3:
	MessageQ_delete(&message_mod.hostQue);
leave2:
	MessageQ_unregisterHeap(App_MsgHeapId);
leave1:
	HeapBufMP_delete(&message_mod.msgHeap);
	pthread_exit("Message_Thread exit\n");
}
/***************************************************************************/
//函数: void Nand_Write_Config(char * file)
//说明:write data to nandfile.txt
//输入:
//输出:
//编辑:R&N
//时间:2015.9.16
/***************************************************************************/
static  int Check_Write_Param(char * file)
{
    int fp = -1;
 // fp= open(file,  O_CREAT | O_TRUNC | O_RDWR, 11666);
    fp= open(file,  O_RDWR, 11666);
    if (fp < 0) 
    {
        my_debug("open %s err\n", file);
        return -1;
    }
     lseek(fp,0,SEEK_SET);
     return fp;
}
/***************************************************************************/
//函数: static void Check_Read_Param(int file)
//说明:read data from checkparm.txt
//输入:
//输出:
//编辑:R&N
//时间:2015.9.16
/***************************************************************************/

static void Check_Read_Param(int file)
{
    lseek(file,0,SEEK_SET);
    read(file, (UInt8 *)(Module.bufferPtr+YC_CHECK_PAR_BASE), 80 );
}

/***************************************************************************/
//函数: void  send_event_by_notify(void * arg)
//说明:供其他线程通过notify发送消息
//输入:
//输出:
//编辑:shaoyi1110@126.com
//时间:2015.4.17
/***************************************************************************/
void  Send_Event_By_Notify( UInt32  command, void * arg)
{
    LOCAL_Module *  mod_p= (LOCAL_Module *)arg;

    Module.lineId     = SystemCfg_LineId;
    Module.eventId  = SystemCfg_EventId;

    Notify_sendEvent(mod_p->remoteProcId,  Module.lineId,  Module.eventId, command, TRUE);
}
/***************************************************************************/
//函数: void  send_event_by_message(void * arg)
//说明:供其他线程通过message发送消息
//输入:
//输出: 0---成功  -1--失败
//编辑:shaoyi1110@126.com
//时间:2015.4.17
/***************************************************************************/
Int  Send_Event_By_Message( UInt32   cmd,  UInt32  buf, UInt32 data)
{
    App_Msg *   msg;
    Int         status;
    message_mod.heapId          = App_MsgHeapId;
    message_mod.msgSize         = sizeof(App_Msg);

    msg = (App_Msg *)MessageQ_alloc(message_mod.heapId, message_mod.msgSize);
     if (msg == NULL) {
	 	status = -1;
            goto leave;
       }
    MessageQ_setReplyQueue(message_mod.hostQue, (MessageQ_Msg)msg);
    msg->cmd = cmd;
    msg->buf = buf;
    msg->data = data;
    MessageQ_put(message_mod.videoQue, (MessageQ_Msg)msg);
	my_debug("DSP: MSG Data cmd 0x%x,buff %x, data %x", msg->cmd,msg->buf, msg->data);
    status = 0;
             return status;
    leave:
		return status;
}
