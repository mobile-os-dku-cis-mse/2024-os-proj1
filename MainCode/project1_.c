#define _POSIX_C_SOURCE 199309L     // sigaction 구조체 오류 해결

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

#define NUM_CHILDREN 10     // 자식 프로세스 수 정의
#define TIME_QUANTUM 1      // 프로세스별 할당 타임 퀀텀
#define MAX_TICKS 1200      // 최대 실행 횟수

struct my_msgbuf 
{
    long mtype;             // 메시지 타입
    int pid;                // 자식 프로세스의 PID(식별 ID)
    int io_time;            // I/O burst time
    int cpu_burst;          // CPU 작업에 할당할 시간
};

int run_queue[NUM_CHILDREN];        // 실행 대기중인 프로세스 관리 큐
int remaining_burst[NUM_CHILDREN];  // 잔여 CPU_burst 시간을 저장
int io_burst[NUM_CHILDREN];         // 잔여 I/O_burst 시간을 저장
int tick_count = 0;                 // 진행 timer tick 수
int current_process = 0;            // 실행 중인 프로세스의 인덱스값
int msgq_id;                        // 메세지 큐 ID
FILE *log_file;                     // 파일 포인터(txt)

// 프로세스별 통계 추적 변수
int cpu_usage_time[NUM_CHILDREN] = {0}; // 총 CPU 사용 시간 기록
int wait_time[NUM_CHILDREN] = {0};      // 총 대기시간 기록
int io_wait_time[NUM_CHILDREN] = {0};   // 총 I/O 대기시간 기록

// 함수 정의 선언
void signal_handler(int signo);     // 신호 핸들러 함수
void setup_timer();                 // 타이머 설정 함수
void create_children();             // 자식 프로세스 생성 함수
void RR_scheduler();                // RR 스케줄러 함수
void handle_io();                   // I/O 처리 함수

// main 함수
int main()
{
    printf("Program started. Creating child processes and initializing scheduling.\n");

    log_file = fopen("schedule_dump.txt", "w");
    if (log_file == NULL)   // 오류처리
    {
        perror("fopen");
        exit(1);
    }
    // 프로그램 시작 시간 기록
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);  

    // 메시지 큐 생성
    msgq_id = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
    if (msgq_id == -1)      // 오류처리
    {
        perror("msgget");
        exit(1);
    }

    // 자식 프로세스 생성
    create_children();

    // 타이머 설정
    setup_timer();

    // 메인 스케줄링 루프
    while (tick_count < MAX_TICKS) 
    {
        pause();
    }

    // 모든 자식 프로세스가 종료될 때까지 대기
    for (int i = 0; i < NUM_CHILDREN; i++) 
    {
        wait(NULL);
    }

    // 모든 자식이 종료된 후에 메시지 큐 제거
    msgctl(msgq_id, IPC_RMID, NULL);
    fclose(log_file);

    // 프로그램 종료 시간 기록
    clock_gettime(CLOCK_MONOTONIC, &end);  
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    // CPU 사용률 계산
    double cpu_utilization = 0;
    for (int i = 0; i < NUM_CHILDREN; i++) 
    {
        cpu_utilization += cpu_usage_time[i];
    }
    cpu_utilization = (cpu_utilization / (MAX_TICKS * NUM_CHILDREN)) * 100;

    // 터미널에 프로세스별 통계 출력
    printf("\nProcess Statistics:\n");
    for (int i = 0; i < NUM_CHILDREN; i++) 
    {   
        printf("Process %d: CPU Usage Time = %d, Wait Time = %d, I/O Wait Time = %d\n", 
               run_queue[i], cpu_usage_time[i], wait_time[i], io_wait_time[i]);
    }
        // 총 실행시간 & CPU 사용률 & 자원 해제
    printf("\nTotal Execution Time: %f seconds\n", elapsed);
    printf("CPU Utilization: %f%%\n", cpu_utilization);
    printf("All processes have been terminated. Resources cleaned up.\n");

    return 0;
}

void create_children() 
{
    for (int i = 0; i < NUM_CHILDREN; i++) 
    {
        pid_t pid = fork();
        if (pid == 0) {
            srand(getpid());
            int cpu_burst = rand() % 10 + 1;
            int io_burst = rand() % 5 + 1;
            struct my_msgbuf msg;

            while (1) 
            {
                // 메시지 수신
                if (msgrcv(msgq_id, &msg, sizeof(msg) - sizeof(long), getpid(), 0) == -1) 
                {   // 오류처리
                    perror("msgrcv");
                    exit(1);
                }

                // CPU-burst 감소
                cpu_burst -= TIME_QUANTUM;
                if (cpu_burst <= 0) 
                {
                    msg.mtype = 1;
                    msg.pid = getpid();
                    msg.cpu_burst = io_burst;
                    if (msgsnd(msgq_id, &msg, sizeof(msg) - sizeof(long), IPC_NOWAIT) == -1) 
                    {   // 오류처리
                        perror("msgsnd");
                        exit(1);
                    }
                }
            }
            exit(0);
        } 
        else 
        {
            run_queue[i] = pid;
            remaining_burst[i] = rand() % 10 + 1;
            io_burst[i] = rand() % 5 + 1;
        }
    }
}

void setup_timer() 
{
    struct itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 50000;
    timer.it_interval = timer.it_value;

    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) 
    {   // 오류처리
        perror("setitimer");
        exit(1);
    }

    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGALRM, &sa, NULL) == -1) 
    {   // 오류처리
        perror("sigaction");
        exit(1);
    }
}

void signal_handler(int signo) 
{
    tick_count++;
    // 진행상황 확인용 tick 알림
    if (tick_count % 100 == 0) 
    {   
        printf("Current Timer Tick: %d\n", tick_count); // 100 tick마다 터미널에 진행 상황 출력
    }

    // 각 프로세스의 대기시간 증가
    for (int i = 0; i < NUM_CHILDREN; i++) 
    {
        if (i != current_process && remaining_burst[i] > 0) 
        {
            wait_time[i] += 1;
        }
        if (io_burst[i] > 0) 
        {
            io_wait_time[i] += 1;
        }
    }

    RR_scheduler();

    // 로그 기록
    fprintf(log_file, "Tick %d:\n", tick_count);
    fprintf(log_file, "Run-queue: ");
    for (int i = 0; i < NUM_CHILDREN; i++) 
    {
        fprintf(log_file, "[%d:%d] ", run_queue[i], remaining_burst[i]);
    }
    fprintf(log_file, "\nWait-queue: ");
    for (int i = 0; i < NUM_CHILDREN; i++) 
    {
        if (io_burst[i] > 0) 
        {
            fprintf(log_file, "[%d:%d] ", run_queue[i], io_burst[i]);
        }
    }
    fprintf(log_file, "\n");
    fflush(log_file);
}

void RR_scheduler() 
{
    if (remaining_burst[current_process] > 0) 
    {
        struct my_msgbuf msg;
        msg.mtype = run_queue[current_process];
        msg.cpu_burst = TIME_QUANTUM;

        if (msgsnd(msgq_id, &msg, sizeof(msg) - sizeof(long), IPC_NOWAIT) == -1) 
        {   // 오류처리
            perror("msgsnd");
        }

        // CPU 할당 상황을 파일에 기록
        fprintf(log_file, "at time %d, process %d gets cpu time, remaining cpu-burst: %d\n",
                tick_count, run_queue[current_process], remaining_burst[current_process]);

        remaining_burst[current_process] -= TIME_QUANTUM;
        cpu_usage_time[current_process] += TIME_QUANTUM;

        if (remaining_burst[current_process] <= 0) 
        {
            handle_io();
        }
    }
    current_process = (current_process + 1) % NUM_CHILDREN;
}

void handle_io() 
{
    for (int i = 0; i < NUM_CHILDREN; i++) 
    {
        if (io_burst[i] > 0) 
        {
            io_burst[i]--;
            if (io_burst[i] == 0) 
            {
                remaining_burst[i] = rand() % 10 + 1;
                io_burst[i] = rand() % 5 + 1;  // 새로운 io_burst 값 랜덤으로 설정
                fprintf(log_file, "Process %d completed I/O, returning to Run-queue\n", run_queue[i]);
            }
        }
    }
}
