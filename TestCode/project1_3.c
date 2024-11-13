#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <time.h>

#define NUM_CHILDREN 10
#define TIME_QUANTUM 1
#define MAX_TICKS 1000

struct my_msgbuf {
    long mtype;
    int pid;
    int cpu_burst;
};

int run_queue[NUM_CHILDREN];
int remaining_burst[NUM_CHILDREN];
int io_burst[NUM_CHILDREN];
int tick_count = 0;
int current_process = 0;
int msgq_id;
FILE *log_file;

// 프로세스별 통계 추적 변수
int cpu_usage_time[NUM_CHILDREN] = {0};
int wait_time[NUM_CHILDREN] = {0};
int io_wait_time[NUM_CHILDREN] = {0};

void signal_handler(int signo);
void setup_timer();
void create_children();
void RR_scheduler();
void handle_io();

int main() {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);  // 프로그램 시작 시간 기록

    printf("Program started. Creating child processes and initializing scheduling.\n");

    log_file = fopen("schedule_dump.txt", "w");
    if (log_file == NULL) {
        perror("fopen");
        exit(1);
    }

    // 메시지 큐 생성
    msgq_id = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
    if (msgq_id == -1) {
        perror("msgget");
        exit(1);
    }

    // 자식 프로세스 생성
    create_children();

    // 타이머 설정
    setup_timer();

    // 메인 스케줄링 루프
    while (tick_count < MAX_TICKS) {
        pause();
    }

    // 모든 자식 프로세스가 종료될 때까지 대기
    for (int i = 0; i < NUM_CHILDREN; i++) {
        wait(NULL);
    }

    // 종료 처리
    msgctl(msgq_id, IPC_RMID, NULL);
    fclose(log_file);

    clock_gettime(CLOCK_MONOTONIC, &end);  // 프로그램 종료 시간 기록
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    // CPU 사용률 계산
    double cpu_utilization = 0;
    for (int i = 0; i < NUM_CHILDREN; i++) {
        cpu_utilization += cpu_usage_time[i];
    }
    cpu_utilization = (cpu_utilization / (MAX_TICKS * NUM_CHILDREN)) * 100;

    // 터미널에 프로세스별 통계 출력
    printf("\nProcess Statistics:\n");
    for (int i = 0; i < NUM_CHILDREN; i++) {
        printf("Process %d: CPU Usage Time = %d, Wait Time = %d, I/O Wait Time = %d\n", 
               run_queue[i], cpu_usage_time[i], wait_time[i], io_wait_time[i]);
    }

    printf("\nTotal Execution Time: %.2f seconds\n", elapsed);
    printf("CPU Utilization: %.2f%%\n", cpu_utilization);
    printf("All processes have been terminated. Resources cleaned up.\n");

    return 0;
}

void create_children() {
    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            srand(getpid());
            int cpu_burst = rand() % 10 + 1;
            int io_burst = rand() % 5 + 1;
            struct my_msgbuf msg;

            while (1) {
                // 메시지 수신
                if (msgrcv(msgq_id, &msg, sizeof(msg) - sizeof(long), getpid(), 0) == -1) {
                    perror("msgrcv");
                    exit(1);
                }

                // CPU-burst 감소
                cpu_burst -= TIME_QUANTUM;
                if (cpu_burst <= 0) {
                    msg.mtype = 1;
                    msg.pid = getpid();
                    msg.cpu_burst = io_burst;
                    if (msgsnd(msgq_id, &msg, sizeof(msg) - sizeof(long), IPC_NOWAIT) == -1) {
                        perror("msgsnd");
                        exit(1);
                    }
                }
            }
            exit(0);
        } else {
            run_queue[i] = pid;
            remaining_burst[i] = rand() % 10 + 1;
            io_burst[i] = rand() % 5 + 1;
        }
    }
}

void setup_timer() {
    struct itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 50000;
    timer.it_interval = timer.it_value;

    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("setitimer");
        exit(1);
    }

    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
}

void signal_handler(int signo) {
    tick_count++;

    if (tick_count % 100 == 0) {
        printf("Current Timer Tick: %d\n", tick_count); // 100 tick마다 터미널에 진행 상황 출력
    }

    // 각 프로세스의 대기 시간 증가
    for (int i = 0; i < NUM_CHILDREN; i++) {
        if (i != current_process && remaining_burst[i] > 0) {
            wait_time[i] += 1;
        }
        if (io_burst[i] > 0) {
            io_wait_time[i] += 1;
        }
    }

    RR_scheduler();

    // 로그 기록
    fprintf(log_file, "Tick %d:\n", tick_count);
    fprintf(log_file, "Run-queue: ");
    for (int i = 0; i < NUM_CHILDREN; i++) {
        fprintf(log_file, "[%d:%d] ", run_queue[i], remaining_burst[i]);
    }
    fprintf(log_file, "\nWait-queue: ");
    for (int i = 0; i < NUM_CHILDREN; i++) {
        if (io_burst[i] > 0) {
            fprintf(log_file, "[%d:%d] ", run_queue[i], io_burst[i]);
        }
    }
    fprintf(log_file, "\n");
    fflush(log_file);
}

void RR_scheduler() {
    if (remaining_burst[current_process] > 0) {
        struct my_msgbuf msg;
        msg.mtype = run_queue[current_process];
        msg.cpu_burst = TIME_QUANTUM;

        if (msgsnd(msgq_id, &msg, sizeof(msg) - sizeof(long), IPC_NOWAIT) == -1) {
            perror("msgsnd");
        }

        // CPU 할당 상황을 파일에 기록
        fprintf(log_file, "at time %d, process %d gets cpu time, remaining cpu-burst: %d\n",
                tick_count, run_queue[current_process], remaining_burst[current_process]);

        remaining_burst[current_process] -= TIME_QUANTUM;
        cpu_usage_time[current_process] += TIME_QUANTUM;

        if (remaining_burst[current_process] <= 0) {
            handle_io();
        }
    }

    current_process = (current_process + 1) % NUM_CHILDREN;
}

void handle_io() {
    for (int i = 0; i < NUM_CHILDREN; i++) {
        if (io_burst[i] > 0) {
            io_burst[i]--;
            if (io_burst[i] == 0) {
                remaining_burst[i] = rand() % 10 + 1;
                io_burst[i] = rand() % 5 + 1;  // 새로운 io_burst 값 설정
                fprintf(log_file, "Process %d completed I/O, returning to Run-queue\n", run_queue[i]);
            }
        }
    }
}
