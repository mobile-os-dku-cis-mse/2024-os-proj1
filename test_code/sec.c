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

#define MAX_CPROC 10
#define TIME_TICK 10000
#define RUN_TIME 100
#define MSG_KEY 0x12345 * 2


// int count = 0; // process가 몇 번 도는가?
FILE *file;
int run_time = 0;
int time_quantum;
int count = 0;
Queue runq;
Queue waitq;
Process* cur_proc; // 전역 변수를 통해 시그널 핸들러에서 접근


void signal_handler(int signo);
void parent_handler(int signo);

int main(int* argc, char* argv[]){
    if (argc < 2) {
        printf("Please provide an integer argument.\n");
        return 1;
    }

    time_quantum = atoi(argv[1]); // 문자열을 정수로 변환
    printf("The number you entered is: %d\n", time_quantum);
    count = time_quantum;

    // 파일 포인터 선언 및 파일 열기
    file = fopen("schedule_dump.txt", "w");
    if (file == NULL) {
        perror("Failed to open file");
        return 1;
    }
    fprintf(file, "--------------------------------\n");
    fprintf(file, "%5s|%20s|%14s\n", "시간 t", "프로세스 pid가 CPU 사용", "남은 cpu-burst");


    // 부모 프로세스 시그널 함수 만들기
    struct sigaction old_sa;
	struct sigaction new_sa;
	memset(&new_sa, 0, sizeof(new_sa));
	new_sa.sa_handler = &signal_handler;
	sigaction(SIGALRM, &new_sa, &old_sa);

	struct itimerval new_itimer, old_itimer;
	new_itimer.it_interval.tv_sec = 0;
	new_itimer.it_interval.tv_usec = TIME_TICK;
	new_itimer.it_value.tv_sec = 1;
	new_itimer.it_value.tv_usec = 0;

    // 자식 프로세스 시그널
    struct sigaction sa_p;
    // SIGUSR1 시그널에 대한 핸들러 설정
    sa_p.sa_handler = &parent_handler;  // 시그널을 받을 때 실행할 함수 지정
    sa_p.sa_flags = 0;  // 플래그 설정 (기본값으로 설정)
    sigemptyset(&sa_p.sa_mask);  // 시그널을 받을 때 블록할 시그널이 없다면 초기화
    if (sigaction(SIGUSR1, &sa_p, NULL) == -1) {
        perror("sigaction error");
        exit(1);
    }
    
    initQueue(&runq);
    initQueue(&waitq);

    int cpu_burst[MAX_CPROC];
    int io_burst[MAX_CPROC];
    for(int j=0;j<MAX_CPROC;j++){
        cpu_burst[j] = (rand() % 10) + 1; // 1부터 10 사이의 랜덤 값 생성
        io_burst[j] = (rand() % 10) + 1;
    }

    int msgq = msgget(MSG_KEY, IPC_CREAT | 0666);
    if(msgq == -1){
        perror("msgq failed");
    }
    for(int i=0;i<MAX_CPROC;i++){
        pid_t pid = fork();  // fork를 호출하여 새로운 프로세스 생성
        if (pid < 0) {  // fork가 실패한 경우
            perror("Fork failed");
            exit(1);
        } 
        else if (pid == 0) {  // pid가 0이면 자식 프로세스
            printf("This is the child process. PID: %d, cpu: %d, io: %d\n", getpid(), cpu_burst[i], io_burst[i]);
            
            // 시그널로 부모 프로세스로 넘어감
            struct msgbuf snd_msg;
            snd_msg.mtype = i+1;
            snd_msg.idx = i;
            snd_msg.pid = getpid();
            snd_msg.cpu_burst = cpu_burst[i];
            snd_msg.io_burst = io_burst[i];
            
            // Send the message
            if (msgsnd(msgq, &snd_msg, sizeof(snd_msg) - sizeof(long), 0) == -1) {
                perror("msgsnd failed");
                exit(1);
            }
            raise(SIGSTOP); // 부모 프로세스에게 CPU 반환
            exit(0);  // Exit the child process after receiving the stop signal
            
        } 
        else {  // pid가 양수면 부모 프로세스
            printf("This is the parent process. Child PID: %d\n", pid);
            /* 부모 프로세스: 큐 관리(+ 시간 할당량 확인, IPC 보냄) */
            
            struct msgbuf rcv_msg;
            if (msgrcv(msgq, &rcv_msg, sizeof(rcv_msg) - sizeof(long), i+1, 0) == -1) {
                perror("msgrcv failed");
                exit(1);
            }
            printf("rcv id %d>> %d: %d\n", msgq, rcv_msg.idx, rcv_msg.cpu_burst);

            Process* new_proc = malloc(sizeof(Process));
            new_proc->pid = rcv_msg.pid;
            new_proc->idx = rcv_msg.idx;
            new_proc->io_burst = rcv_msg.io_burst;
            new_proc->remain_io_burst = rcv_msg.io_burst;
            new_proc->cpu_burst = rcv_msg.cpu_burst;
            new_proc->remain_cpu_burst = rcv_msg.cpu_burst;

            if (!isFull(&runq)) {
                enqueue(&runq, new_proc); // 포인터를 큐에 삽입
            } else {
                printf("Run queue is full. Cannot add new process.\n");
                free(new_proc); // 큐가 꽉 찼다면 메모리 해제
            }
        }
    }
    printf("All process is in runq\n");
    printf("----------------------------------------------\n");

    for(int i=0;i<MAX_CPROC;i++){
        fprintf(file, "process[%d] cpu_burst: %d, io_burst: %d\n", i, cpu_burst[i], io_burst[i]);
    }

	setitimer(ITIMER_REAL, &new_itimer, &old_itimer);
    int ipc_msgq = msgget(MSG_KEY * 2, IPC_CREAT | 0666);
    while (1) {
        printf("waitq > f %d, r %d\t", waitq.front, waitq.rear);
        printf("runq > f %d, r %d\n", runq.front, runq.rear);
        /*
        부모
            io 요청 받으면, 자식을 run -> wait으로 이동
            매 시간마다 io_busrt 값 감소
            io_burst == 0 -> 자식을 런 큐로            
        자식
            msg를 받음 -> signal로 부모 프로세스로 요청
            cpu_burst == 0 -> 부모 프로세스는 IPC로 io_busrt 값을 전송
        */
        cur_proc = &runq.processes[runq.front];
        printf("cproc->cpu_busrt: %d, front: %d\n", runq.processes[runq.front].remain_cpu_burst, runq.front);
        
        // pause();
        kill(getpid(), SIGALRM);
        cur_proc->remain_cpu_burst--;
        struct ipcbuf ipc_msg;
        if (msgrcv(ipc_msgq, &ipc_msg, sizeof(ipc_msg) - sizeof(long), 11, IPC_NOWAIT) == -1) {
            perror("msgrcv failed");
            exit(1);
        }
        
        // printf("\t\t\t\t\tcount: %d, cur_proc->remain_cpu_b: %d\n", count, cur_proc->remain_cpu_burst);
        if(count == 0 || cur_proc->remain_cpu_burst == 0){
            printf("cur_proc->pid: %d\n", cur_proc->pid);
            kill(getpid(), SIGUSR1); // 시그널 받고 부모 프로세스 요청
        }
    }

    if (msgctl(ipc_msgq, IPC_RMID, NULL) == -1) {
        perror("msgctl(IPC_RMID) failed");
    }

    // 작업 완료 후 메시지 큐 삭제

    // 파일 닫기
    fprintf(file, "--------------------------------\n");

    fclose(file);
    printf("Data has been written to schedule_dump.txt\n");

    return 0;
}

void parent_handler(int signo){ // 부모는 받기만 한대!!
    if(signo == SIGUSR1){
        // printf("why an dwe???????\n");
        Process* proc;
        if(!isEmpty(&runq)){
            proc = dequeue(&runq);
            // printf("proc cpu_b : %d\n", proc->remain_cpu_burst);
            if(count == 0){
                if(proc->remain_cpu_burst <= 0){
                    enqueue(&runq, proc);   // 남은 CPU 버스트가 있으면 다시 runq로 이동
                    count = time_quantum;
                } else {
                    enqueue(&waitq, proc);  // CPU 버스트가 끝나면 waitq로 이동
                    count = time_quantum;
                }
            }
            else {
                if(proc->remain_cpu_burst == 0){
                    enqueue(&waitq, proc);
                    count = time_quantum;
                }
            }
        } 
        kill(getpid(), SIGCONT);
    }
}


// runq의 모든 process 처리 함수
void signal_handler(int signo) {
    run_time++;
    count--;
    // printf("Run time left: %d\n", RUN_TIME - run_time);
    print_qstate(run_time, &runq, &waitq);
    writeToFile(file, run_time, count, &runq, &waitq);
    printf("\t-------> count: %d\n", count);

    int ipc_msgq = msgget(MSG_KEY * 2, IPC_CREAT | 0666);
    // 시그널로 부모 프로세스로 넘어감
    struct ipcbuf ipc_msg;
    ipc_msg.mtype = 11;
    ipc_msg.count = count - 1;
    if (msgsnd(ipc_msgq, &ipc_msg, sizeof(ipc_msg) - sizeof(long), 0) == -1) {
        perror("msgsnd failed");
        exit(1);
    }
    waitq_burst();
    // else printf("empty runq!!\n");

    if (run_time == RUN_TIME) {
        printf("Program finished.\n");
        exit(0);
    }
    kill(getpid(), SIGCONT);
}


// io_burst 감소시키는 함수 (시간 틱마다 I/O 관리)
void waitq_burst(){
    if(!isEmpty(&waitq)){
        for(int j=0;j<waitq.size;j++){
            Process* proc = dequeue(&waitq);
            proc->remain_io_burst--;
            if(proc->remain_io_burst == 0){
                proc->remain_cpu_burst = proc->cpu_burst; // cpu_burst 재할당
                proc->remain_io_burst = proc->io_burst;
                enqueue(&runq, proc); // runq로 ㄱㄱ
            } else enqueue(&waitq, proc); // 아직 0 아님. 다시 waitq
        }
    }
}

int msggets(int msgid, struct msgbuf *msg) {
    if (msgsnd(msgid, msg, sizeof(*msg) - sizeof(long), 0) == -1) {
        perror("msgsnd failed");
        return -1;
    }
    return 0;
}

int msgrcvs(int msgid, struct msgbuf *msg, long msgtype) {
    if (msgrcv(msgid, msg, sizeof(*msg) - sizeof(long), msgtype, 0) == -1) {
        perror("msgrcv failed");
        return -1;
    }
    return 0;
}




// int msgrcvs(struct msgbuf* msg, int msgq) {
// 	// int key = 0x12345 * 2;
// 	// int msgq = initMsgQueue(key);
//     if (msgq == -1) {
//         perror("msgget failed");
//         return -1;
//     }

//     // 메시지 수신
//     int q_id = msgrcv(msgq, msg, sizeof(struct msgbuf) - sizeof(long), 0, 0); // IPC_NOWAIT
//     // printf("[in msg] idx: %d\n", msg->idx);

//     // 수신된 메시지 내용 출력
//     printf("Received message: idx=%d, pid=%d, cpu_burst=%d, io_burst=%d\n",
//            msg->idx, msg->pid, msg->cpu_burst, msg->io_burst);
//     if (q_id == -1) {
//         perror("msgrcv failed");
//         return -1;
//     }

//     return q_id;
// }