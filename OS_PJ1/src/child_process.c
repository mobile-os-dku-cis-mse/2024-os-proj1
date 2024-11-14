#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <time.h>

#include "child_process.h"
#include "message_queue.h"
#include "scheduler.h"

void create_children(int num_children, int msgid)
{
    for (int i = 0; i < num_children; i++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            srand(time(NULL) ^ (getpid() << 16));
            int cpu_burst = rand() % 5 + 1;
            int io_burst = rand() % 5 + 1;
            while (1)
            {
                struct message msg;
                if (msgrcv(msgid, &msg, sizeof(msg.cpu_burst), getpid(), IPC_NOWAIT) != -1)
                {
                    cpu_burst -= msg.cpu_burst;
                    printf("Child %d received time slice, CPU burst remaining: %d\n", getpid(), cpu_burst);
                    if (cpu_burst <= 0)
                    {
                        cpu_burst = rand() % 5 + 1;
                        printf("Child %d switching to IO burst of %d seconds\n", getpid(), io_burst);
                        sleep(io_burst);
                        io_burst = rand() % 5 + 1;
                    }
                }
            }
            exit(0);
        }
        else
        {
            child_pids[i] = pid;
            remaining_quantum[i] = TIME_QUANTUM;
        }
    }
}

void switch_to_next_child()
{
    if (remaining_quantum[current_child] <= 0)
    {
        current_child = (current_child + 1) % NUM_CHILDREN;
    }
    struct message msg;
    msg.mtype = child_pids[current_child];
    msg.cpu_burst = TIME_QUANTUM;
    msgsnd(msgid, &msg, sizeof(msg.cpu_burst), IPC_NOWAIT);
    remaining_quantum[current_child] -= TIME_QUANTUM;
}