#include "queue.h"
#include "stdio.h" // debug purpose

// initialize a single linked list for processes
queue *init_queue()
{
    queue *q = (queue *)malloc(sizeof(queue));
    q->head = NULL;
    q->length = 0;

    return q;
}

// add a process to the back of the queue
void add_process(queue *q, pcb_t *p)
{
    pcb_t *prev = q->head;
    while (prev && prev->next)
    {
        prev = prev->next;
    }

    if (prev == NULL)
    {
        q->head = p;
    }
    else
    {
        prev->next = p;
    }

    // p->next = NULL;
    q->length++;
}

// remove a process from the queue, providing the queue and the process to remove
pcb_t *remove_process(queue *q, pcb_t *p)
{
    pcb_t *prev = NULL;
    pcb_t *tmp = q->head;

    while (tmp)
    {
        if (tmp == p)
        {
            if (prev)
            {
                prev->next = p->next;
            }
            else
            {
                q->head = p->next;
            }
            // p->next = NULL;
            q->length--;
            p->next = NULL;
            return p;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    return NULL;
}