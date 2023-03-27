#ifndef PCB_HEADER
#define PCB_HEADER

#include <sys/types.h>
#include "queue.h"
#include "parser.h"

#define RUNNING_J 0
#define STOPPED_J 1

typedef struct jobs
{
    pid_t *pid;
    pid_t pgid;

    char *cmd;
    bool background;
    int (*pipes)[2];
    int fd0;
    int fd1;

    int jid;
    int status; /* RUNNING or STOPPED or FINISHED */
} job;

typedef struct job_queue {
    queue *queue_running;
    queue *queue_stopped;
    int max_jid;
} job_list;

job *init_job(struct parsed_command *cmd, int jid, pid_t pid, pid_t pgid, int fd0, int fd1);

job_list *init_job_list();

void free_job(job *j);

void free_all_jobs(job_list *list);

void print_all_jobs(job_list *list);

#endif
