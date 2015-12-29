#ifndef _QUEUE_H_
#define _QUEUE_H_
#if defined (__cplusplus)
extern "C" {
#endif

/* sysbios queue */
#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/knl/Semaphore.h>

typedef struct _Queue_ 
{
    Queue_Handle handle; 
    Semaphore_Handle sem;
}Queue;

Queue  * queue_new();
void   queue_destroy(Queue *queue);
void   queue_push(Queue *queue, Queue_Elem *elm);

Queue_Elem * queue_pop(Queue *queue);

#if defined (__cplusplus)
}
#endif /* _QUEUE_H_ */
#endif /* _QUEUE_H_ */

