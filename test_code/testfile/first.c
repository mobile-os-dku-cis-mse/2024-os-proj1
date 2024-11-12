#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <fcntl.h>
#include "msg.h"

/*
1. 10개 자식 프로세스 생성
2. 부모는 주기적으로 ALARM을 받아 시간틱을 얻고 그에 따라 자식 프로세스 스케줄링
	by setitimer
3. 큐(런 큐, 대기 큐)
4. 스케줄링
	부모: 자식프로세스의 남은 시간 확인하고, CPU 시간 사용할 수 있도록 IPC로 메세지 보냄
		  자식의 대기 시간 기록
--------- --------- --------- ---------- ---------- ----------
1. ready, wait queue --> process 구조체(pid_t, int cpu_burst/io_burst --> 시간마다 burst 감소)
2. node 생성 후, 구조체 입력
*/

#define QUEUE_SIZE 10
#define QUANTUM 10

void signal_handler(int signo);
int elapsed_time = 0;

void child_process_logic(int cpu_burst){
    while(1){
        // CPU 작업 수행
        printf("Child process %d: Executing CPU burst for %d seconds.\n", getpid(), cpu_burst);
        sleep(cpu_burst); // CPU 버스트 동안 대기

        // I/O 요청 전송
        msgbuf message;
        strcpy(message.mtext, "I/O request");
        message.mtype = 1;
        msgsnd(msgid, &message, sizeof(message), 0);
        printf("Child process %d: Sent I/O request\n", getpid());

        // I/O 버스트 수행
        int io_burst = rand() % 3 + 1; // 1에서 3 사이의 랜덤 숫자 생성
        printf("Child process %d: Executing I/O burst for %d seconds.\n", getpid(), io_burst);
        sleep(io_burst); // I/O 버스트 동안 대기
    }
}

// 큐 초기화 함수
void initializeQueue(Run_Queue* readyq, Wait_Queue* waitq) {
    readyq->front = -1;
    readyq->rear = -1;
    waitq->front = -1;
    waitq->rear = -1;
}

// 큐에 프로세스 추가하는 함수
int enqueue(Run_Queue* runq, pid_t pid, int cpu_burst) {
    // 큐가 가득 찼는지 확인
    if ((runq->rear + 1) % QUEUE_SIZE == runq->front) {
        printf("Queue is full!\n");
        return -1; // 실패
    }
    
    // 큐가 비어있다면 front와 rear를 0으로 설정
    if (runq->front == -1) {
        runq->front = 0;
        runq->rear = 0;
    } else {
        runq->rear = (runq->rear + 1) % QUEUE_SIZE; // 원형 큐를 위한 래핑
    }

    // 프로세스를 큐에 추가
    runq->proc[runq->rear].pid = pid;
    runq->proc[runq->rear].cpu_burst = cpu_burst; // 랜덤 값 넣기
    runq->proc[runq->rear].io_burst = 0;

    printf("Process %d(%d) enqueued in Run Queue.\n", pid, cpu_burst);
    return 0; // 성공
}

int main() {
    // SIGALRM 핸들러 설정
    struct sigaction old_sa;
	struct sigaction new_sa;
	memset(&new_sa, 0, sizeof(new_sa));
	new_sa.sa_handler = &signal_handler;
	sigaction(SIGALRM, &new_sa, &old_sa);

	struct itimerval new_itimer, old_itimer;
	new_itimer.it_interval.tv_sec = 1;
	new_itimer.it_interval.tv_usec = 0;
	new_itimer.it_value.tv_sec = 1;
	new_itimer.it_value.tv_usec = 0;


    // 메세지 생성
    key_t key = ftok("msgfile", 65); // 메시지 큐 키 생성
    int msgid = msgget(key, 0666 | IPC_CREAT); // 메시지 큐 생성

    pid_t fd = open("test.txt", O_RDONLY); // 읽기 전용 파일 열기
    
    Run_Queue runq;  // Run 큐 선언
    Wait_Queue waitq;    // Wait 큐 선언
    
    // 큐 초기화
    initializeQueue(&runq, &waitq);

    for (int i = 0; i < 10; i++) {
        pid_t pid = fork();

        if (pid < 0) {  // fork 실패 시
            perror("Fork failed");
            exit(1);
        } else if (pid == 0) {  // 자식 프로세스
            printf("Child process %d created. PID: %d\n", i + 1, getpid());
            int cpu_burst = rand() % 10 + 1; // 1에서 10 사이의 랜덤 숫자 생성
            int io_burst = rand() % 10 + 1;
            child_process_logic(cpu_burst);
            exit(0);
        } else {
            /* 부모 프로세스: 큐 관리(+ 시간 할당량 확인, IPC 보냄) */
            enqueue(&runq, pid, runq.cpu_burst); // 대기큐에 자식 프로세스 넣기
        }
    }

    while (1) {
        setitimer(ITIMER_REAL, &timer, NULL);   // timer 시작
        
        // 시간 틱마다 run_queue의 모든 자식 프로세스의 io_burst를 감소시킴
        // io_burst == 0 -> 런 큐로 옮기고 다음에 cpu를 받을 수 있도록 함
    }

    return 0;
}

void signal_handler(int signo) {
	printf("signaled! %d \n", signo);
	elapsed_time++;

    while(1){
        if(runq.proc[i].cpu_burst > 0){
            runq.proc[i].cpu_burst--;
            printf("Process %d: CPU burst decreased to %d\n", runq.proc[i].pid, runq.proc[i].cpu_burst);

            if(runq.proc[i].cpu_burst == 0){
                waitq.proc[waitq.rear] = runq.proc[i];
                waitq.rear = (waitq.rear + 1) % QUEUE_SIZE;

                // 런 큐에서 프로세스 제거
                runq.front = (runq.front + 1) % QUEUE_SIZE; 
                printf("Process %d moved to wait queue\n", runq.proc[i].pid);

            }
        }
        i++;
    }

    // 특정 조건에 따라 종료하지 않도록 변경
    if (elapsed_time >= QUANTUM) {
        printf("Elapsed time reached quantum limit. Exiting.\n");
        exit(0);
    }
}
