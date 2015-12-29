/*************************************************************************/
/*
 * Copyright (c) 2015, ֣�����ܵ��� Runner
 * All rights reserved.
 * file       : IPCServer.c
 * Design  	  : ZLB
 * Data       : 2015-4-20
 * Version    : V1.0
 * Change     :
 */
/*************************************************************************/
/* this define must precede inclusion of any xdc header file */
#define Registry_CURDESC Test__Desc
#define MODULE_NAME "Server"

/* xdctools header files */
#include <xdc/std.h>
#include <xdc/runtime/Assert.h>
#include <xdc/runtime/Diags.h>
#include <xdc/runtime/Log.h>
#include <xdc/runtime/Registry.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/knl/Cache.h>

/* package header files */
#include <ti/ipc/Ipc.h>
#include <ti/ipc/HeapBufMP.h>
#include <ti/ipc/MessageQ.h>
#include <ti/ipc/MultiProc.h>
#include <ti/ipc/Notify.h>
#include <ti/ipc/SharedRegion.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Task.h>

/* local header files */
#include "../shared/AppCommon.h"

/* module header file */
#include "sys.h"
#include "communicate.h"
#include "log.h"

/****************************** macro define ************************************/
#define QUEUESIZE   8   
/****************************** module structure *********************************/
typedef struct 
{
    UInt16              	remoteProcId;          	// host processor id
    UInt16              	lineId;               		// notify line id
    UInt32              	eventId;              		// notify event id
    Semaphore_Struct    	semObj;               		// semaphore object
    Semaphore_Handle   		semH;                 		// handle to object above
    UInt32              	eventQue[QUEUESIZE];  		// command queue
    UInt                	head;                	 	// queue head pointer
    UInt                	tail;                 		// queue tail pointer
    UInt32              	error;
}Server_Module;
/******************************* functions **************************************/
Void Server_notifyCB(UInt16 procId, UInt16 lineId, UInt32 eventId,
                           UArg arg, UInt32 payload);
static UInt32 Server_waitForEvent(Void);
/******************************* private data *********************************/
Registry_Desc      Registry_CURDESC;
static Int            Module_curInit = 0;
static Server_Module  Module;
Mes_Module Module_Mes;

/***************************************************************************/
//����:Int Init_IPC(UInt16 remoteID)
//˵��:Init_IPC ()��ʼ��IPC����	
//����: remoteID Զ��������ID
//���: ����״̬status ,Status_Err��ʾ���Ӵ���
//�༭:
//ʱ��:2015.4.20
/***************************************************************************/
Int Init_IPC(UInt16 *remoteID)
{
	Int status = 0;
	
	do		/* only one thread must call start */
	{
		status = Ipc_start();
	} while (status == Ipc_E_NOTREADY);

	if (status < 0) 
	{
		Log_error0("Init_IPC: Ipc_start() failed");
		return status;
	}

    *remoteID = MultiProc_getId("HOST");   /* attach to the remote processor */

    /* connect to remote processor */
	do 
	{
		status = Ipc_attach(*remoteID);

		if (status == Ipc_E_NOTREADY) 
		{
		    Task_sleep(100);
		}

	} while (status == Ipc_E_NOTREADY);

    if (status < 0) 
	{
		Log_error0("Init_IPC: Ipc_attach() failed");
		return status;
	}
	return status;
}
/***************************************************************************/
//����:Int Stop_IPC(UInt16 remoteID)
//˵��:Stop_IPC ()ֹͣIPC����	
//����: remoteID Զ��������ID
//���: ����״̬status ,Status_Err��ʾֹͣIPC���Ӵ���
//�༭:
//ʱ��:2015.4.20
/***************************************************************************/
Int Stop_IPC(UInt16 remoteID)
{
	Int status;

	/* END server phase */

    	/* disconnect from remote processor */
	do 
	{
		status = Ipc_detach(remoteID);
		if (status == Ipc_E_NOTREADY) 
		{
		    Task_sleep(100);
		}
		
	} while (status == Ipc_E_NOTREADY);

	if (status < 0) 
	{
	    Log_error0("smain: Failed to disconnect from remote process");
	    return Status_Err;
	}

     /* only one thread must call stop */
	 Ipc_stop();
	 
	 return status;
}

/***************************************************************************/
//����:Void Server_init(Void)
//˵��:��ʼ��ģ��			
//����: ��
//���: 
//�༭:
//ʱ��:2015.4.20
/***************************************************************************/
Void Server_init(Void)
{
    	Registry_Result result;

    	if (Module_curInit++ != 0) 
	{
	    return;  /* already initialized */
	}
	
	result = Registry_addModule(&Registry_CURDESC, MODULE_NAME);
	Assert_isTrue(result == Registry_SUCCESS, (Assert_Id)NULL);
	
}
/***************************************************************************/
//����:Void Server_exit(Void)
//˵��:�˳�ģ��		
//����: ��
//���: 
//�༭:
//ʱ��:2015.4.20
/***************************************************************************/
Void Server_exit(Void)
{

	if (Module_curInit-- != 1) 
	{
	    return;  /* object still being used */
	}
	/*
	 * Note that there isn't a Registry_removeModule() yet:
	 *     https://bugs.eclipse.org/bugs/show_bug.cgi?id=315448
	 *
	 * ... but this is where we'd call it.
	 */
}
/***************************************************************************/
//����:Int Creat_Message(Void)
//˵��:����Message ���в���ARM ����message����
//����: ��
//���: �Ƿ񴴽���ȷ
//�༭:
//ʱ��:2015.4.20
/***************************************************************************/
Int Creat_Message(Void)
{
	Int  status = 0;
	UInt32  event;
	MessageQ_Params    msgqParams;

	Module_Mes.msgHeap = NULL;
	Module_Mes.hostQueid  = MessageQ_INVALIDMESSAGEQ;
	Module_Mes.videoQue = NULL;
	Module_Mes.heapId	=  App_MsgHeapSrId;
	Module_Mes.msgSize   = sizeof(App_Msg);

	/* 1. ������Ϣ���� (inbound messages) */
	MessageQ_Params_init(&msgqParams);
	//����APPʹ�õ���Ϣ����
	Module_Mes.videoQue = MessageQ_create(App_VideoMsgQueName, &msgqParams);

	if (Module_Mes.videoQue == NULL) 
	{
		status = -1;
		goto leave1;
	}

	/* 2. ����׼�������¼� */
	status = Notify_sendEvent(Module.remoteProcId, Module.lineId, Module.eventId,
        						App_CMD_RESRDY, TRUE);

	if (status < 0) 
	{
		goto leave2;
	}

	 /* 3. �ȴ�Զ����Դ׼�������¼� */
	do 
	{
		event = Server_waitForEvent();

		if (event >= App_E_FAILURE) 
		{
			status = -1;
			goto leave2;
		}
	} while (event != App_CMD_RESRDY);

	/* 4. ��Զ��HeapBufMP ���� ARM �����Ĺ���Heap */
    status = HeapBufMP_open(App_MsgHeapName, &Module_Mes.HeapBufMP_remoteHandle);

    if(status < 0)
	{
        goto leave2;
    }
	
	/* 5. ע�� message */
	status = MessageQ_registerHeap((Ptr)(Module_Mes.HeapBufMP_remoteHandle), App_MsgHeapSrId);

	if (status < 0) 
	{
	    goto leave3;
	}


	/* 5. ��Զ����Դ */
	//��ARM �˴�������Ϣ����
    status = MessageQ_open(App_HostMsgQueName, &Module_Mes.hostQueid);

    if (status < 0) 
	{
        goto leave4;
    }
	
	/* 6. ���ͳ���׼�����¼� */
	status = Notify_sendEvent(Module.remoteProcId, Module.lineId, Module.eventId,
	    						App_CMD_READY, TRUE);

	if (status < 0) 
	{
	 	goto leave5;
	}

	/* 7. �ȴ�Զ����������¼����� */
	do 
	{
		event = Server_waitForEvent();

		if (event >= App_E_FAILURE)
		{
			status = -1;
			goto leave5;
		}
	} while (event != App_CMD_READY);

//	LOG_INFO("Creat Message is ok;\n");
	return (status);

leave5:
	Log_print0(Diags_INFO,"MessageQ_close: close open messageQ ");
	MessageQ_close(&Module_Mes.hostQueid);
leave4:
	Log_print0(Diags_INFO,"MessageQ_unregisterHeap: unregister heap ");
	MessageQ_unregisterHeap(App_MsgHeapSrId);
leave3:
	Log_print0(Diags_INFO,"HeapBufMP_close: close heapbufmp ");
	HeapBufMP_close(&Module_Mes.HeapBufMP_remoteHandle);
leave2:
	Log_print0(Diags_INFO,"MessageQ_delete: delete messageQ");
	MessageQ_delete(&Module_Mes.videoQue);
leave1:
	Log_print1(Diags_EXIT, "<-- Creat_Message: %d", (IArg)status);
	return (status);
}
/***************************************************************************/
//����:Void Message_del(Void)
//˵��:ɾ���Ѵ�����message��Դ
//����: ��
//���: ��
//�༭:
//ʱ��:2015.4.23
/***************************************************************************/
Void Message_del(Void)
{
	Log_print0(Diags_ENTRY, "--> Resource_delete:");

	MessageQ_close(&Module_Mes.hostQueid);
	MessageQ_unregisterHeap(App_MsgHeapSrId);
	HeapBufMP_close(&Module_Mes.HeapBufMP_remoteHandle);
	MessageQ_delete(&Module_Mes.videoQue);
}
/***************************************************************************/
//����:Int ShareRegion_create(UInt16 remoteID)
//˵��:�����ڴ�����������
//����: remoteID Զ������ID  Char* shareptr ��������ָ��
//���: status ����״̬
//�༭:
//ʱ��:2015.4.20
/***************************************************************************/
Int ShareRegion_create(UInt16 remoteID)
{
	Int    status;
	Semaphore_Params    semParams;

	Log_print0(Diags_ENTRY, "--> ShareRegion_create:");

	/* ����Ĭ�ϲ��� */
	Module.head = 0;
	Module.tail   = 0;
	Module.error  = 0;
	Module.semH = NULL;
	Module.lineId = SystemCfg_LineId;
	Module.eventId = SystemCfg_EventId;
	Module.remoteProcId = remoteID;
    
	/* enable some log events */
	Diags_setMask(MODULE_NAME"+EXF");

	 /* 1. ����ͬ������ */
	Semaphore_Params_init(&semParams);
	semParams.mode = Semaphore_Mode_COUNTING;
	Semaphore_construct(&Module.semObj, 0, &semParams);

	Module.semH = Semaphore_handle(&Module.semObj);

	/* 2. ע��notify �ص����� */
	status = Notify_registerEvent(Module.remoteProcId, Module.lineId, 
	        Module.eventId, Server_notifyCB,(UArg)  &Module.eventQue);

	if (status < 0) 
	{
	    Log_error0("ShareRegion_create: Device failed to register notify event");
	    goto leave1;
	}

	/* 3. �ȴ�Զ������ע��notify �ص����� */
	do 
	{
		status = Notify_sendEvent(Module.remoteProcId,Module.lineId,
		        Module.eventId, App_CMD_NOP, TRUE);

		if (status == Notify_E_EVTNOTREGISTERED) 
		{
		    Task_sleep(100);
		}

	} while (status == Notify_E_EVTNOTREGISTERED);

	/* ���ش�����Ϣ��������ʧ�� */
	if (status < 0 ) 
	{
		Log_error0("ShareRegion_create: Host failed to register callback");
		goto leave2;
	}
	
	Log_print0(Diags_INFO, "ShareRegion_create: Slave is ready");

	return (status);
leave2:
	Notify_unregisterEvent(Module.remoteProcId, Module.lineId, 
	        Module.eventId, Server_notifyCB,(UArg)  &Module.eventQue);
leave1:
	Semaphore_destruct(&Module.semObj);		//delete semaphore
	Log_print1(Diags_EXIT, "<-- ShareRegion_create: %d", (IArg)status);
	return (status);	
}
/***************************************************************************/
//����:Int ShareRegion_exec(Void)
//˵��:ִ�й����ڴ�����������
//����: Char** shareptr ��������ָ��
//���: statusִ��״̬
//�༭:
//ʱ��:2015.4.20
/***************************************************************************/
Int ShareRegion_exec(UChar** shareptr)
{
	Int  status = 0;
	SharedRegion_SRPtr  sharedBufferPtr = 0;
	UInt32  event;
	UChar* bufferPtr;

	Log_print0(Diags_ENTRY | Diags_INFO, "--> ShareRegion_exec:");

	/* 1. �ȴ�ARM���͹����ڴ����ĵ����ֽڵ�ַ����*/
	event = Server_waitForEvent();

	if (event >= App_E_FAILURE) 
	{
		Log_error1("ShareRegion_exec: Received queue error: %d",event); 
		status = -1;       
		goto leave;
	} 

	/* �����payload ������Ĺ����ڴ��������ֽڵ�ַ */
	sharedBufferPtr = event & App_SPTR_MASK;
    
	/* ���ͽ��յ���ַ��ȷ���ź� */
	status = Notify_sendEvent(Module.remoteProcId, Module.lineId, 
	        Module.eventId, App_SPTR_ADDR_ACK, TRUE);

	/* �������ʧ�������־��Ϣ */
	if (status < 0 )  
	{
	    Log_print0(Diags_INFO,"ShareRegion_exec: Error sending shared region pointer"
	            "address acknowledge");
	    goto leave;
	} 

	/* 3. �ȴ������ڴ����ĸ����ֽڵ�ַ*/
	event = Server_waitForEvent();

	if (event >= App_E_FAILURE) 
	{
	    Log_error1("ShareRegion_exec: Received queue error: %d",event); 
	    status = -1;       
	    goto leave;
	} 

	/* 4. ���յ��ĸߵ͵�ַ����������� */
	sharedBufferPtr = ((event & App_SPTR_MASK) << 16) | sharedBufferPtr;

	/* �����յ���ַ��ȷ���ź� */
	status = Notify_sendEvent(Module.remoteProcId, Module.lineId, 
	    Module.eventId, App_SPTR_ADDR_ACK, TRUE);

	if (status < 0 )  
	{
		Log_print0(Diags_INFO,"ShareRegion_exec: Error sending shared region pointer"
		        "address acknowledge");
		goto leave;
	}  

	/* 5. �������ڴ�����ַת��ΪDSP ���ص�ַ */
	bufferPtr = SharedRegion_getPtr(sharedBufferPtr);
	
	LOG_INFO("Syslink_Init: ShareRegionPtr string: %s\n",  bufferPtr);
	*shareptr = bufferPtr;

//	sprintf((char *)(bufferPtr + PRINTF_BASE), "hello L&M, this is a test");
	/* 6. ���������ڴ�������--�򵥵�Ӧ�� */
	while (*bufferPtr != 0) 
	{
		if (*bufferPtr >= 0x61 && *bufferPtr <= 0x7A) 
		{
		    *bufferPtr = *bufferPtr - 0x20;
		}
		bufferPtr++;
	}

	/* 7. ������ɷ���notify ��Ϣ */
	status = Notify_sendEvent(Module.remoteProcId, Module.lineId, 
	        Module.eventId, App_CMD_OP_COMPLETE, TRUE);

	if (status < 0 ) 
	{
	    Log_print0(Diags_INFO,"ShareRegion_exec: Error sending operation complete "
	            "command");
	    goto leave;
	}  

//	LOG_INFO("Creat ShareRegion is ok;");
	
	Log_print0(Diags_INFO, "ShareRegion_exec: Completed lowercase to uppercase "
            "string operation");    
leave:
    Log_print1(Diags_EXIT, "<-- ShareRegion_exec: %d", (IArg)status);
    return (status);	
}
/***************************************************************************/
//����:Int ShareRegion_delete(Void)
//˵��:ɾ�������ڴ�����������
//����: ��
//���: statusִ��״̬
//�༭:
//ʱ��:2015.4.20
/***************************************************************************/
Int ShareRegion_delete(Void)
{
	Int     status = 0; 
	UInt32  event;

	Log_print0(Diags_ENTRY, "--> Server_delete:");

	/* 1. �ȴ�ɾ������ */
	event = Server_waitForEvent();

	if (event >= App_E_FAILURE) 
	{
		Log_error1("Server_delete: Received queue error: %d",event); 
		status = -1;       
		goto leave;
	} 

	/* ����ɾ������Ӧ�� */
	status = Notify_sendEvent(Module.remoteProcId, Module.lineId, 
	    Module.eventId, App_CMD_SHUTDOWN_ACK, TRUE);

	if (status < 0 )  
	{
		Log_error0("Server_delete: Error sending shutdown command acknowledge");
		goto leave;
	}

	/* 2. ȡ��notify �Ļص����� */
	status = Notify_unregisterEvent(Module.remoteProcId, Module.lineId, 
	    Module.eventId, Server_notifyCB,(UArg) &Module.eventQue);

	if (status < 0) 
	{
		Log_error0("Server_delete: Unregistering event has failed");
		goto leave;
	}

	/* 3. ɾ��ͬ��ģ�� */
	Semaphore_destruct(&Module.semObj);

	Log_print0(Diags_INFO,"Server_delete: Cleanup complete");

leave:
	Log_print1(Diags_EXIT, "<-- Server_delete: %d", (IArg)status);
	return (status);
}
/***************************************************************************/
//����:Void Notify_del(Void)
//˵��:ɾ����������Դ
//����: ��
//���: ��
//�༭:
//ʱ��:2015.4.23
/***************************************************************************/
Void Notify_del(Void)
{
	Log_print0(Diags_ENTRY, "--> Resource_delete:");

	/* 1. ȡ��notify �Ļص����� */
	Notify_unregisterEvent(Module.remoteProcId, Module.lineId, 
	    Module.eventId, Server_notifyCB,(UArg) &Module.eventQue);

	/* 2. ɾ��ͬ��ģ�� */
	Semaphore_destruct(&Module.semObj);

	Log_print0(Diags_INFO,"Resource_delete: Cleanup complete");

	return ;
}
/***************************************************************************/
//����:UInt32 Server_waitForEvent(Void)
//˵��:�ȴ��¼�����,δ�õ��ź�ʱ��������
//����: ��
//���: event �¼���Ϣ
//�༭:
//ʱ��:2015.4.20
/***************************************************************************/
static UInt32 Server_waitForEvent(Void)
{
	UInt32 event;

	if (Module.error >= App_E_FAILURE) 
	{
	    event = Module.error;
	    goto leave;
	}

	/* �ü����ź����ȴ��´��¼� */
	Semaphore_pend(Module.semH, BIOS_WAIT_FOREVER);

	/* �Ӷ����а������ */
	event = Module.eventQue[Module.tail];
	Module.tail = (Module.tail + 1) % QUEUESIZE;

leave:
	return(event);
}
/***************************************************************************/
//����:UInt32 Server_waitForEvent(Void)
//˵��:�ȴ��¼�����,δ�õ��ź�ʱ��������
//����: ��
//���: event �¼���Ϣ
//�༭:
//ʱ��:2015.4.20
/***************************************************************************/
Void Server_notifyCB( UInt16 procId, UInt16 lineId, UInt32  eventId, UArg arg,
        						UInt32 payload)
{
	UInt  next;

	/* ����nop ���� */
	if (payload == App_CMD_NOP)
	{
		return;
	}

	//֪ͨARM��������
	if(payload ==App_CMD_SHUTDOWN)			
	{
		LOG_INFO("App_CMD_SHUTDOWN shutdown app;");
		
		Message_Send(MSG_EXIT,0,0);
		
		/* ����ɾ������Ӧ�� */
		Notify_sendEvent(Module.remoteProcId, Module.lineId,  Module.eventId, App_CMD_SHUTDOWN_ACK, TRUE);

		//ֹͣ��ʱ��
		if((ad7606Str.flagfreq >> 3) & 0x01)
		{
			Timer_stop(ad7606Str.clock);
			Timer_delete(&ad7606Str.clock);
			LOG_INFO("Timer_delete timer2 ;");
		}
		/* ɾ��messageģ��*/
		Message_del();
		/* ɾ��notify */
		Notify_del();
		/* stop ipcͨ�� */
		Stop_IPC(Module.remoteProcId);
		Server_exit();
		return;
	}

	/* ������һ���¼���λ�� */
	next = (Module.head + 1) % QUEUESIZE;

	if (next == Module.tail ) 
	{
		/* ������,��������,�ô����־ */
		Module.error = App_E_OVERFLOW;
	}
	else 
	{
		Module.eventQue[Module.head] = payload;

		/* queue head is only written to within this function */
		Module.head = next;

		/* signal semaphore (counting) that new event is in queue */
		Semaphore_post(Module.semH);
	}

	return;
}
/***************************************************************************/
//����:Int Message_Task(Void)
//˵��:Message����	
//����: ��
//���: ��������״̬
//�༭:
//ʱ��:2015.4.20
/***************************************************************************/
Void Message_Task(UArg arg0, UArg arg1)
{
	App_Msg   sendmsg;
	App_Msg  * msg;
	UInt16 ipcID = (UInt16)arg0;

	LOG_INFO("--> Message_Task:");

	while(1)
	{
		MessageQ_get(Module_Mes.videoQue, (MessageQ_Msg *)&msg, MessageQ_FOREVER);
		/* process the message */
		LOG_INFO("Message_Task: processed cmd=0x%x", msg->cmd);
		
		//process message
		switch(msg->cmd)
		{
			case MSG_EXIT:
				MessageQ_free((MessageQ_Msg )msg);
				sendmsg.cmd = MSG_EXIT;
//				Message_Send(&sendmsg);
				goto level;
			case MSG_YK:
				break;
			default:
				break;
		}
		MessageQ_free((MessageQ_Msg)msg);
	}
	
level:
	LOG_INFO("Resource_delete: delete creat Message resource");
	Message_del();
	LOG_INFO("Resource_delete: delete creat Notify resource");
	Notify_del();		
	LOG_INFO("Stop_IPC: stop IPC connect ");
	Stop_IPC(ipcID);
	/* finalize modules */
	Server_exit();
	return;
}
/***************************************************************************/
//����:Int Message_Task(Void)
//˵��:Message����	
//����: ��
//���: ��������״̬
//�༭:
//ʱ��:2015.4.20
/***************************************************************************/
//Int Message_Send(App_Msg *sendmsg)
//{
//	Int status = 0;
//	App_Msg*  msg;
//	msg = (App_Msg *)MessageQ_alloc(Module_Mes.heapId, Module_Mes.msgSize);

//	if (msg == NULL) 
//	{
//		status = -1;
//		goto leave;
//	}

//	msg->cmd = sendmsg->cmd;
//	msg->channel = sendmsg->channel;
//	msg->data = sendmsg->data;
//	MessageQ_put(Module_Mes.hostQueid, (MessageQ_Msg)msg);  //��������
//	
//leave:
//	LOG_INFO("send message: channel:%x data: %x", msg->channel, msg->data);
//	Log_print1(Diags_EXIT, "send message: %d", (IArg)status);
//	return status;
//}
/***************************************************************************/
//����:Int Message_Send(UInt32 cmd, UInt32 channel, UInt32 data)
//˵��:Message ���ͺ���
//����: cmd ���� channel ͨ���� data ����
//���: �����Ƿ�ɹ� -1 ʧ�� 0 �ɹ�
//�༭:
//ʱ��:2015.9.20
/***************************************************************************/
Int Message_Send(UInt32 cmd, UInt32 channel, UInt32 data)
{
	Int status = 0;
	App_Msg  *msg;
	msg = (App_Msg *)MessageQ_alloc(Module_Mes.heapId, Module_Mes.msgSize);

	if (msg == NULL) 
	{
		status = -1;
		goto leave;
	}

	msg->cmd = cmd;
	msg->channel = channel;
	msg->data = data;
	MessageQ_put(Module_Mes.hostQueid, (MessageQ_Msg)msg);  //��������
	
leave:
	LOG_INFO("send message: channel:%x data: %x", msg->channel, msg->data);
	Log_print1(Diags_EXIT, "send message: %d", (IArg)status);
	return status;
}
/***************************************************************************/
//����:Int Syslink_Init(UInt16 *remoteID, UChar *ptr)
//˵��:˫��ͨ�ų�ʼ��(�����ڴ�����Message)
//����: remoteID Զ���ں�ID ptr ��������ַ
//���: ����״̬ -1 ʧ�� 0 �ɹ�
//�༭:
//ʱ��:2015.4.20
/***************************************************************************/
Int Syslink_Init(UInt16 *remoteID, UChar **ptr)
{
	Int status = 0;
	UChar *shareaddr = NULL;
	/* Initialize modules */
	Server_init();

	//��ʼ��IPC ����
	status = Init_IPC(remoteID);		
	if(status < 0)
	{
		goto leave1;
	}

	/* server setup phase */
    status= ShareRegion_create(remoteID[0]);
	if(status < 0)
	{
		goto leave2;
	}

	/* sharerigion get prt */
    status= ShareRegion_exec(&shareaddr);
	if(status < 0)
	{
		goto leave3;
	}
	*ptr = shareaddr;
	/* creat message */
	status = Creat_Message();
    if (status < 0) 
	{
        goto leave3;
    }
	//no err
	return 0;

leave3:
	LOG_INFO("Resource_delete: delete creat Notify resource");
	Notify_del();
leave2:		
	LOG_INFO("Stop_IPC: stop IPC connect ");
	Stop_IPC(remoteID[0]);
leave1:
	/* finalize modules */
	Server_exit();
    LOG_INFO("<-- Init_Conf: %d", (IArg)status);
	//err
	return -1;	
}


