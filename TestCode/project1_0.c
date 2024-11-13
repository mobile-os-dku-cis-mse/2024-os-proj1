#define _POSIX_C_SOURCE 199309L


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

struct my_msgbuf 
{
    long mtype;
    int pid;
    int io_time;
};

int msgq_id;
double time_quantum = 0.01;
int current_pid = -1;
int tick_count = 0;
int max_ticks = 10000;
FILE *log_file;

void cleanup() {
    msgctl(msgq_id, IPC_RMID, NULL);
    fclose(log_file);
    printf("Resources cleaned up.\n");
}

void signal_handler(int signo) {
    struct my_msgbuf msg;
    char buffer[100];

    // I/O 요청 수신
    if (msgrcv(msgq_id, &msg, sizeof(msg) - sizeof(long), 0, IPC_NOWAIT) != -1) {
        sprintf(buffer, "Received I/O request from PID %d with I/O time %d\n", msg.pid, msg.io_time);
        fputs(buffer, log_file);
    }

    // 다음 프로세스로 전환
    current_pid = (current_pid + 1) % 10;
    sprintf(buffer, "Running process with PID: %d%%\n", current_pid);
    fputs(buffer, log_file);

    tick_count++;
    if (tick_count % 100 == 0) {
        printf("Current tick count: %d%%\n", tick_count/100);
    }

    if (tick_count >= max_ticks) {
        printf("Max ticks reached. Terminating...\n");
        for (int i = 0; i < 10; i++) {
            wait(NULL);
        }
        cleanup();
        exit(0);
    }
}

void child_process(int pid) {
    struct my_msgbuf msg;
    msg.mtype = 1;
    msg.pid = pid;
    msg.io_time = rand() % 5 + 1;

    while (1) {
        usleep(time_quantum * 1e6);
        printf("PID %d: Finished CPU burst, requesting I/O with I/O time %d\n", pid, msg.io_time);
        if (msgsnd(msgq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
            perror("msgsnd failed");
            exit(1);
        }
    }
}

int main() {
    key_t key = 0x12345;
    msgq_id = msgget(key, IPC_CREAT | 0666);
    if (msgq_id == -1) {
        perror("msgget failed");
        exit(1);
    }

    log_file = fopen("schedule_dump.txt", "w");
    if (log_file == NULL) {
        perror("Failed to open log file");
        cleanup();
        exit(1);
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigaction(SIGALRM, &sa, NULL);

    struct itimerval timer;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = (int)(time_quantum * 1e6);
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = (int)(time_quantum * 1e6);
    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("setitimer failed");
        cleanup();
        exit(1);
    }

    for (int i = 0; i < 10; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            child_process(i);
            exit(0);
        } else if (pid < 0) {
            perror("fork failed");
            cleanup();
            exit(1);
        }
    }

    while (1) {
        pause();
    }

    cleanup();
    return 0;
}
