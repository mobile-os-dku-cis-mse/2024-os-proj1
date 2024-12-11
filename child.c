#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <time.h>

struct my_msgbuf {
    long mtype;
    int pid;
    int io_time;
};
int main() {
    int key = 0x12345;
    int msgq = msgget(key, IPC_CREAT | 0666);
    if (msgq == -1) {
        perror("msgget failed");
        exit(1);
    }
    int cpu_burst = rand() % 10 + 1;
    int io_burst = rand() % 5 + 1;
    while (1) {
        struct my_msgbuf msg;
        if (msgrcv(msgq, &msg, sizeof(msg) - sizeof(long), 0, 0) != -1) {
            cpu_burst--;
            if (cpu_burst <= 0) {
                msg.io_time = io_burst;
                msgsnd(msgq, &msg, sizeof(msg) - sizeof(long), 0);
                sleep(io_burst); 
                cpu_burst = rand() % 10 + 1; 
            }
        }
    }
    return 0;
}
