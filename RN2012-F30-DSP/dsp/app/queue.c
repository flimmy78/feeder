/*************************************************************************/
/*
 * Copyright (c) 2015, ֣�����ܵ��� Runner
 * All rights reserved.
 * file       : fft.c
 * Design     : ZLB
 * Data       : 2015-4-27
 * Version    : V1.0
 * Change     :
 */
/*************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <xdc/std.h>
#include <xdc/runtime/Diags.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/Log.h>
#include <xdc/runtime/System.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>

#include "queue.h"
#include "log.h"

/****************************** type define ************************************/

/***************************************************************************/
//����: Queue *queue_new(Void)
//˵��: ����queue
//����: ��
//���: ����ָ��
//�༭:
//ʱ��:2015.4.28
/***************************************************************************/
Queue *queue_new(Void) 
{
	// creat queue
	Queue *queue = (Queue *)malloc(sizeof(Queue));
	
    memset(queue, 0, sizeof(Queue));
    queue->handle = Queue_create(NULL, NULL);
    if (! queue->handle)
        LOG_INFO("failed to create queue");

    /* creat sem */
    queue->sem = Semaphore_create(0, NULL, NULL);
    if (! queue->sem)
        LOG_INFO("failed to create semaphore");

    return queue;
}
/***************************************************************************/
//����: void queue_destroy(Queue *queue) 
//˵��: ɾ��queue
//����: queue����ָ��
//���: 
//�༭:
//ʱ��:2015.4.28
/***************************************************************************/
void queue_destroy(Queue *queue) 
{
    if (! queue)
        return;

    Queue_delete(&queue->handle);
    Semaphore_delete(&queue->sem);
	//�ͷ��ڴ�
    free(queue);
}
/***************************************************************************/
//����: void queue_push(Queue *queue, Queue_Elem *elm) 
//˵��: ������
//����: queue����ָ��
//���: 
//�༭:
//ʱ��:2015.4.28
/***************************************************************************/
void queue_push(Queue *queue, Queue_Elem *elm) 
{
	/* ����queue�Ƿ���Ч */
//	ASSERT(queue);
    
    Queue_put(queue->handle, elm);  
    Semaphore_post((Semaphore_Object *)queue->sem); 
}
/***************************************************************************/
//����: void queue_push(Queue *queue, Queue_Elem *elm) 
//˵��: ������
//����: queue����ָ��
//���: 
//�༭:
//ʱ��:2015.4.28
/***************************************************************************/
Queue_Elem *queue_pop(Queue *queue) 
{
//    ASSERT(queue);
    
    while (Queue_empty(queue->handle))
        Semaphore_pend(queue->sem, BIOS_WAIT_FOREVER);

    return Queue_get(queue->handle);
}

