#include "child.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>

void child_process() {
    int cpu_burst = rand() % CPU_BURST_MAX + 1;
    int io_burst = rand() % IO_BURST_MAX + 1;

    const key_t key = ftok(".", 'a');
    int msgid = msgget(key, 0666);

    struct my_msgbuf msg;
    msg.mtype = getpid();
    msg.pid = getpid();

    while (1) {
        msgrcv(msgid, &msg, sizeof(msg) - sizeof(long), getpid(), 0);
        cpu_burst--;

        if (cpu_burst <= 0) {
            msg.io_time = io_burst;
            msgsnd(msgid, &msg, sizeof(msg), IPC_NOWAIT);
            cpu_burst = rand() % CPU_BURST_MAX + 1;
        }
    }
}
