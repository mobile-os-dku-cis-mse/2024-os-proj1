//<제출용 최종본>
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
#include <time.h>

#define MAX_CPROC 10
#define TIME_TICK 100000//0.1초
#define RUN_TIME 6000//6000//--> 프로세스 동작 시간 평균 1분 ==> 총 10분 동안 동작
#define MSG_KEY 0x12345 * 2 // 동일한 키를 사용 --> 여러 자식 프로세스에서 동일한 메시지 큐를 이용하는 방식

FILE *file;
int run_time = 0;
int time_quantum;
int count = 0;
Queue runq;
Queue waitq;
Process* cur_proc;

void signal_handler(int signo);
//void parent_handler(int signo);
void waitq_burst(void);

int main(int argc, char* argv[]){

    //main_0) 명령줄 인수 에러 처리
    if (argc < 2) {
        printf("Please provide an integer argument _ quantum size\n");
        return 1;
    }


    //main_1) 명령줄 인수로 타임 퀀텀 설정
    time_quantum = atoi(argv[1]);
    printf("입력한 타임 퀀텀 값 : %d\n\n", time_quantum);
    count = time_quantum;


    //main_2) 로그 파일 생성
    file = fopen("schedule_dump.txt", "w");
    if (file == NULL) {
        perror("Failed to open file");
        return 1;
    }


    //main_3) 타이머 설정 : 간격-0.1초, 1초 뒤 시작
    struct itimerval new_itimer, old_itimer;
    new_itimer.it_interval.tv_sec = 0;
    new_itimer.it_interval.tv_usec = TIME_TICK;
    new_itimer.it_value.tv_sec = 1;
    new_itimer.it_value.tv_usec = 0;


    //main_4) Signal Handler 등록(SIGALRM)
    struct sigaction old_sa;
    struct sigaction new_sa;
    memset(&new_sa, 0, sizeof(new_sa));
    new_sa.sa_handler = &signal_handler;
    sigaction(SIGALRM, &new_sa, &old_sa);

    
    //main_5) run queue, wait queue 초기화
    initQueue(&runq);
    initQueue(&waitq);


    //main_6) 자식 프로세스들이 사용할 burst를 랜덤하게 생성
    srand((unsigned int)time(NULL));
    int cpu_burst[MAX_CPROC];
    int io_burst[MAX_CPROC];
    for(int j=0; j<MAX_CPROC; j++){//버스트는 시뮬레이션을 관찰하기 적절한 값으로 임의 설정 하였음
        cpu_burst[j] = (rand() % 50) + 1;//(최소 : 1 ~ 최대 : 50 -> 타임 퀀텀은 50이하에서 유의미)
        io_burst[j] = (rand() % 200) + 1;//(최소 : 1 ~ 최대 : 200 -> 타임 퀀텀은 200이하에서 유의미)
    }


    //main_7) IPC(메시지 큐 방식 사용) : 여러 프로세스가 동일한 메시지 큐를 사용
    int msgq = msgget(MSG_KEY, IPC_CREAT | 0666);
    if(msgq == -1){
        perror("msgq failed");
    }

    //main_8) fork
    for(int i=0; i<MAX_CPROC; i++){
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        } 
        else if (pid == 0) {  // 자식 프로세스
            printf("[%d] %d번 자식 프로세스 생성 성공 || cpu: %d, io: %d\n", 
                   getpid(), i, cpu_burst[i], io_burst[i]);
            
            struct msgbuf snd_msg;
            snd_msg.mtype = i+1;
            snd_msg.idx = i;
            snd_msg.pid = getpid();
            snd_msg.cpu_burst = cpu_burst[i];
            snd_msg.io_burst = io_burst[i];
            
            if (msgsnd(msgq, &snd_msg, sizeof(snd_msg) - sizeof(long), 0) == -1) {
                perror("msgsnd failed");
                exit(1);
            }

            //자식 프로세스 : 부모 프로세스로부터 SIGCONT신호를 받아 재개
            while(1) {
                kill(getpid(),SIGSTOP);
                printf("*[%d] 자식 프로세스 실행중*\n",getpid());
            }
        } 
        else {  // 부모 프로세스
            printf("[%d번] 자식 프로세스 생성 실행\n", pid);
            
            struct msgbuf rcv_msg;
            if (msgrcv(msgq, &rcv_msg, sizeof(rcv_msg) - sizeof(long), i+1, 0) == -1) {
                perror("msgrcv failed");
                exit(1);
            }
            printf("[ IPC ]   msg qid : %d, index : %d , cpu burst : %d , io burst : %d \n\n", 
                   msgq, rcv_msg.idx, rcv_msg.cpu_burst, rcv_msg.io_burst);

            //자식 프로세스로부터 받은 메시지를 이용하여 대응하는 노드 생성
            Process* new_proc = malloc(sizeof(Process));
            new_proc->pid = rcv_msg.pid;
            new_proc->idx = rcv_msg.idx;
            new_proc->io_burst = rcv_msg.io_burst;
            new_proc->remain_io_burst = rcv_msg.io_burst;
            new_proc->cpu_burst = rcv_msg.cpu_burst;
            new_proc->remain_cpu_burst = rcv_msg.cpu_burst;

            //새로 만든 노드 실행 큐에 삽입
            if (!isFull(&runq)) {
                enqueue(&runq, new_proc);
            } else {
                printf("Run queue is full. Cannot add new process.\n\n");
                free(new_proc);
            }
        }
    }

    printf("----------------------------------------------------------------------\n");
    printf("              모든 자식 프로세스 실행 준비 큐에 삽입 완료\n\n\n");
    printf("---------------------------------------------------------------------------------------------------------------------------------------------------\n");
    printf("                                                                     스케줄링 시작                                                               \n");
    printf("---------------------------------------------------------------------------------------------------------------------------------------------------\n\n");
    printf("[Information]\n\n");
    printf("- 스케줄링 방식 : Round-Robin\n");
    printf("- Total Run Time : %d\n", RUN_TIME);
    printf("- Time_quantum : %d\n\n", time_quantum);

    fprintf(file, "-------------------------------------------\n");
    fprintf(file, "            자식 프로세스 생성\n\n");
    for(int i=0; i<MAX_CPROC; i++){
        fprintf(file, "process[%d] cpu_burst: %d, io_burst: %d\n", i, cpu_burst[i], io_burst[i]);
    }
    fprintf(file, "\n-------------------------------------------\n");

    // 실행 큐의 front에 SIGCONT 신호 전송
    if (!isEmpty(&runq)) {
        cur_proc = &runq.processes[runq.front];
        kill(cur_proc->pid, SIGCONT);
    }
    //---> 현재 cur_proc 프로세스는 pause해제 상태, 
    // 타이머 시작
    setitimer(ITIMER_REAL, &new_itimer, &old_itimer);

    
    while (1) {
        pause();
    }

    return 0;
}

//SIGALRM 신호마다 호출 : 기본적으로 cpu버스트를 관리하고, 추가적으로 waitq_burst()함수를 호출하여 io버스트를 관리
//라운드 로빈 방식으로 설계한 스케줄러 함수
void signal_handler(int signo) {
    run_time++;
    count--;

    // 현재 실행 중인 프로세스가 있으면 중지
    if (cur_proc != NULL) {
        kill(cur_proc->pid, SIGSTOP);
    }

    print_qstate(run_time, &runq, &waitq);
    writeToFile(file, run_time, count, &runq, &waitq);
    printf("[time_quantum : %d] -------> count: %d\n", time_quantum, count);

    // waitq의 프로세스들 처리
    waitq_burst();

    // 타임 퀀텀 만료 또는 CPU 버스트 완료 체크
    if (count == 0 || (cur_proc && cur_proc->remain_cpu_burst == 0)) {
        if (!isEmpty(&runq)) {
            Process* proc = dequeue(&runq);
            
            if (proc->remain_cpu_burst > 0) {
                enqueue(&runq, proc);
            } else {
                enqueue(&waitq, proc);
            }
            count = time_quantum;
        }
    }

    // 현재 다루고 있는 프로세스의 cpu버스트 1 감소
    if (!isEmpty(&runq)) {
        cur_proc = &runq.processes[runq.front];
        cur_proc->remain_cpu_burst--;
        kill(cur_proc->pid, SIGCONT);
    }

    //실제 실행 시간이 목표 실행시간에 도달할 시 종료(6000초 == 10분, 프로세스당 평균 1분씩 작동)
    if (run_time == RUN_TIME) {
        printf("\n\n\n\n---------------------------------------------------------------------------------------------------------------------------------------------------\n");
        printf("                                                                   Program finished                                                               \n");
        printf("---------------------------------------------------------------------------------------------------------------------------------------------------\n\n\n");
        exit(0);
    }
}


//Wait 큐에 들어있는 모든 프로세스의 I/O 버스트 1 감소
void waitq_burst() {
    int initial_size = waitq.size;
    
    while (initial_size > 0) {
        Process* proc = dequeue(&waitq);
        initial_size--;
        
        proc->remain_io_burst--;
        
        if (proc->remain_io_burst == 0) { // 버스트 모두 소진 시 재할당(초기화)
            proc->remain_cpu_burst = proc->cpu_burst;
            proc->remain_io_burst = proc->io_burst;
            enqueue(&runq, proc);
        } else {
            enqueue(&waitq, proc);
        }
    }
}