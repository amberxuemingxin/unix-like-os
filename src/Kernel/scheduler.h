#ifndef SCHEDULER_HEADER
#define SCHEDULER_HEADER

#include "queue.h"

#define CENTISECOND 10000 /* 10 millisconds */
#define SUCCESS 0
#define FAILURE -1

/*Keeps all queues
 Ready queue: three
 Zombie queue: one*/

void init_scheduler();

void set_timer();

void set_alarm_handler();

pcb_t *search_in_scheduler(pid_t pid);

pcb_t *search_in_zombies(pid_t pid);

void schedule();

void add_to_scheduler(pcb_t *p);

int remove_from_scheduler(pcb_t *p);

void ready_to_block(pcb_t *p);

void block_to_ready(pcb_t *process);

void exit_scheduler();

void print_all_process();

 #endif