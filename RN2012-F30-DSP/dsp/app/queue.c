/*************************************************************************/
/*
 * Copyright (c) 2015, 郑州瑞能电气 Runner
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
//函数: Queue *queue_new(Void)
//说明: 创建queue
//输入: 无
//输出: 队列指针
//编辑:
//时间:2015.4.28
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
//函数: void queue_destroy(Queue *queue) 
//说明: 删除queue
//输入: queue队列指针
//输出: 
//编辑:
//时间:2015.4.28
/***************************************************************************/
void queue_destroy(Queue *queue) 
{
    if (! queue)
        return;

    Queue_delete(&queue->handle);
    Semaphore_delete(&queue->sem);
	//释放内存
    free(queue);
}
/***************************************************************************/
//函数: void queue_push(Queue *queue, Queue_Elem *elm) 
//说明: 出队列
//输入: queue队列指针
//输出: 
//编辑:
//时间:2015.4.28
/***************************************************************************/
void queue_push(Queue *queue, Queue_Elem *elm) 
{
	/* 断言queue是否有效 */
//	ASSERT(queue);
    
    Queue_put(queue->handle, elm);  
    Semaphore_post((Semaphore_Object *)queue->sem); 
}
/***************************************************************************/
//函数: void queue_push(Queue *queue, Queue_Elem *elm) 
//说明: 出队列
//输入: queue队列指针
//输出: 
//编辑:
//时间:2015.4.28
/***************************************************************************/
Queue_Elem *queue_pop(Queue *queue) 
{
//    ASSERT(queue);
    
    while (Queue_empty(queue->handle))
        Semaphore_pend(queue->sem, BIOS_WAIT_FOREVER);

    return Queue_get(queue->handle);
}

