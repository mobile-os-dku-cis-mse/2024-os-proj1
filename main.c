#include "src/scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/msg.h>

int main() {
    srand(time(NULL));

    struct Scheduler scheduler;
    global_scheduler = &scheduler;

    scheduler.run_queue_size = NUM_CHILDREN;
    scheduler.wait_queue_size = 0;
    scheduler.current_process = 0;
    scheduler.time_ticks = 0;

    scheduler.fd = open("schedule_dump.txt", O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (scheduler.fd == -1) {
        perror("Failed to open schedule_dump.txt");
        exit(1);
    }

    if ((scheduler.msgid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT)) == -1) {
        perror("msgget failed");
        exit(1);
    }

    create_child_processes(&scheduler);
    initialize_timer();

    while (1) {
        struct my_msgbuf msg;
        if (msgrcv(scheduler.msgid, &msg, sizeof(msg) - sizeof(long), 0, IPC_NOWAIT) != -1) {
            for (int i = 0; i < scheduler.run_queue_size; i++) {
                if (scheduler.processes[scheduler.run_queue[i]].pid == msg.pid) {
                    scheduler.processes[scheduler.run_queue[i]].in_io = 1;
                    scheduler.processes[scheduler.run_queue[i]].io_burst = msg.io_time;

                    scheduler.wait_queue[scheduler.wait_queue_size++] = scheduler.run_queue[i];
                    for (int j = i; j < scheduler.run_queue_size - 1; j++) {
                        scheduler.run_queue[j] = scheduler.run_queue[j + 1];
                    }
                    scheduler.run_queue_size--;
                    break;
                }
            }
        }
        pause();
    }

    close(scheduler.fd);
    msgctl(scheduler.msgid, IPC_RMID, NULL);
    return 0;
}
