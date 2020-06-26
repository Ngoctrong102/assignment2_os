#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t *q)
{
	return (q->size == 0);
}

// struct queue_t {
// 	struct pcb_t * proc[MAX_QUEUE_SIZE];
// 	int size;
// };
void enqueue(struct queue_t *q, struct pcb_t *proc)
{
	/* TODO: put a new process to queue [q] */
	if (q->size < MAX_QUEUE_SIZE)
	{
		int pos_temp = q->tail;
		while (pos_temp != q->head){
			if (proc->priority > q->proc[pos_temp]->priority){
				q->proc[(pos_temp-1+MAX_QUEUE_SIZE)%MAX_QUEUE_SIZE] = q->proc[pos_temp];
				pos_temp = (pos_temp+1)%MAX_QUEUE_SIZE;
			}
			else {
				q->proc[(pos_temp-1+MAX_QUEUE_SIZE)%MAX_QUEUE_SIZE] = proc;
				q->tail = (pos_temp-1+MAX_QUEUE_SIZE)%MAX_QUEUE_SIZE;
				q->size++;
				return;
			}
		}
		if (q->size == 0){
			q->proc[0] = proc;
			q->size = 1;
			q->head = 0;
			q->tail = 0;
		}
		else {
			if (proc->priority > q->proc[q->head]->priority){
				q->proc[(q->head-1+MAX_QUEUE_SIZE)%MAX_QUEUE_SIZE] = q->proc[q->head];
				q->proc[q->head] = proc;
			}
			else {
				q->proc[(q->head-1+MAX_QUEUE_SIZE)%MAX_QUEUE_SIZE] = proc;
			}
			q->size++;
			q->tail = (q->tail-1+MAX_QUEUE_SIZE)%MAX_QUEUE_SIZE;
		}
	}
}

struct pcb_t *dequeue(struct queue_t *q)
{
	/* TODO: return a pcb whose prioprity is the highest
	 * in the queue [q] and remember to remove it from q
	 * */
	if (q->size > 1){
		q->size--;
		struct pcb_t * proc = q->proc[q->head];
		q->head = (q->head-1+MAX_QUEUE_SIZE)%MAX_QUEUE_SIZE;
		return proc;
	}
	if (q->size == 1){
		q->size--;
		return q->proc[q->head];
	}
	return NULL;
}
