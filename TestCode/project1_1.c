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
#include <time.h>

struct my_msgbuf 
{
    long mtype;
    int pid;
    int io_time;
};

int msgq_id;
double time_quantum = 0.02;
int current_pid = -1;
int tick_count = 0;
int max_ticks = 10000;
FILE *log_file;
clock_t start_time;
pid_t child_pids[10];

// Round-Robin 스케줄링 함수
void round_robin_scheduling() 
{
    current_pid = (current_pid + 1) % 10;
    char buffer[100];
    sprintf(buffer, "Running process with PID: %d\n", current_pid);
    fputs(buffer, log_file);
    fflush(log_file);
}

// 자원 정리 함수
void cleanup() {
    msgctl(msgq_id, IPC_RMID, NULL);
    if (log_file) fclose(log_file);
    printf("Resources cleaned up.\n");
}

// SIGALRM 핸들러
void signal_handler(int signo) 
{
    struct my_msgbuf msg;
    char buffer[100];

    // I/O 요청 수신
    while (msgrcv(msgq_id, &msg, sizeof(msg) - sizeof(long), 0, IPC_NOWAIT) != -1) 
    {
        sprintf(buffer, "Received I/O request from PID %d with I/O time %d\n", msg.pid, msg.io_time);
        fputs(buffer, log_file);
        fflush(log_file);
    }

    // 스케줄링 및 프로세스 전환
    round_robin_scheduling();

    tick_count++;
    if (tick_count % 100 == 0) 
    {
        printf("Current tick count: %d\n", tick_count);
    }

    if (tick_count >= max_ticks) 
    {
        printf("Max ticks reached. Terminating...\n");

        for (int i = 0; i < 10; i++) 
        {
            kill(child_pids[i], SIGTERM);
            wait(NULL);
        }
        
        clock_t end_time = clock();
        double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
        fprintf(log_file, "Total execution time: %.2f seconds\n", elapsed_time);
        printf("Total execution time: %.2f seconds\n", elapsed_time);  // 터미널 출력

        fprintf(log_file, "Resources cleaned up.\n");  // 로그 파일에 기록
        printf("Resources cleaned up.\n");  // 터미널 출력
        cleanup();
        exit(0);
    }
}

// 자식 프로세스 함수
void child_process(int pid) 
{
    struct my_msgbuf msg;
    msg.mtype = 1;
    msg.pid = pid;
    msg.io_time = rand() % 5 + 1;

    while (1) 
    {
        usleep(time_quantum * 1e6);

        if (msgsnd(msgq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) 
        {
            perror("msgsnd failed");
            exit(1);
        }
    }
}

// main 함수
int main() 
{
    key_t key = 0x12345;
    msgq_id = msgget(key, IPC_CREAT | 0666);
    if (msgq_id == -1) 
    {
        perror("msgget failed");
        exit(1);
    }

    log_file = fopen("schedule_dump.txt", "w");
    if (log_file == NULL) 
    {
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
    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) 
    {
        perror("setitimer failed");
        cleanup();
        exit(1);
    }

    start_time = clock();

    for (int i = 0; i < 10; i++) 
    {
        pid_t pid = fork();
        if (pid == 0) 
        {
            child_process(i);
            exit(0);
        } else if (pid < 0) 
        {
            perror("fork failed");
            cleanup();
            exit(1);
        }
        child_pids[i] = pid;  // 자식 PID 저장
    }

    while (1) 
    {
        pause();
    }

    cleanup();
    return 0;
}
