#ifndef Server__include
#define Server__include
#if defined (__cplusplus)
extern "C" {
#endif
#include <ti/ipc/HeapBufMP.h>
#include <ti/ipc/MessageQ.h>
#include <ti/ipc/MultiProc.h>
#include <ti/ipc/Notify.h>
#include <ti/ipc/SharedRegion.h>

/****************************** macro define ************************************/
#define Status_Err  -1    /* err status flag */

/****************************** type define ************************************/
typedef enum
{
	MSG_GET_DB,
	MSG_GET_DB_OK,
	MSG_GET_DB_FAIL,
	MSG_EVENT,
	MSG_COS,
	MSG_SOE,
	MSG_JIAOSHI,
	MSG_JIAOZHUN,
	MSG_CURRENT,
	MSG_VOLTAGE,
	MSG_RECLOSE,
	MSG_YK,
	MSG_PRINTF,
	MSG_EXIT
}MsgCmd;

typedef struct 
{
    HeapBufMP_Handle   msgHeap;    				// created locally
    MessageQ_Handle    videoQue;       			// opened remotely
    MessageQ_QueueId   hostQueid;   			// created locally
    UInt16             heapId;     				// MessageQ heapId
    UInt32             msgSize;	
	HeapBufMP_Handle   HeapBufMP_remoteHandle; /* handle to remote HeapBufMP */
}Mes_Module;
/* msg struct */
struct _APP_MSG_
{
	MessageQ_MsgHeader  reserved;
	UInt32              cmd;
	UInt32				channel;
	UInt32				data;
};
typedef struct _APP_MSG_ App_Msg;
/******************************* declar data ***********************************/
extern Mes_Module Module_Mes;

/******************************* functions **************************************/
Void Server_init(Void);
Void Server_exit(Void);

Int ShareRegion_create(UInt16 remoteID);
Int ShareRegion_delete(Void);
Int Creat_Message(Void);
Void Message_Task(UArg arg0, UArg arg1);
Void Notify_del(Void);
Void Message_del(Void);
Int Message_Send(UInt32 cmd, UInt32 channel, UInt32 data);
Int ShareRegion_exec(UChar** shareptr);
Int Syslink_Init(UInt16 *remoteID, UChar **ptr);
Int Stop_IPC(UInt16 remoteID);

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
#endif /* Server__include */

