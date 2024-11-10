#include "proc.h"


int TIME_QUANTUM = 5;
int current_time = 0; // 타이머 틱
int msgid;
FILE* fp;
int activeProcesses = NUM_PROCESSES; // 전체 프로세스 수
pid_t child_pids[NUM_PROCESSES];
Queue *readyQueue = NULL;
Queue *waitQueue = NULL;


// 준비 큐와 대기 큐를 초기화하는 함수
void initQueues()
{
    readyQueue = createQueue(); // 준비 큐 초기화
    waitQueue = createQueue();  // 대기 큐 초기화
}

// 메시지 큐 생성
void initMessageQueue()
{
    msgid = createMessageQueue(); // 새로운 메시지 큐 생성
}

// SIGCONT 신호를 처리하는 핸들러
void handle_sigcont(int sig)
{
    // 단순히 신호 처리를 위한 빈 핸들러
}

// SIGUSR1 신호를 처리하는 핸들러 (CPU 작업 완료)
void handle_sigusr1(int sig)
{
    // CPU 작업 완료 신호 처리
}

// SIGUSR2 신호를 처리하는 핸들러 (I/O 작업 완료)
void handle_sigusr2(int sig)
{
    // I/O 작업 완료 신호 처리
}


void setup_timer() {
    struct sigaction sa;
    struct itimerval timer;

    // 타이머가 실행될 때마다 호출될 신호 핸들러 설정
    sa.sa_handler = alrm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // 타이머를 자동 재설정
    sigaction(SIGALRM, &sa, NULL);

    // 타이머 설정: TIME_QUANTUM마다 SIGALRM 발생
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = TIME_QUANTUM * 1000; // TIME_QUANTUM을 마이크로초로 변환
    timer.it_interval = timer.it_value; // 반복 간격 설정
    setitimer(ITIMER_REAL, &timer, NULL); // 실제 타이머 설정
}


// 타이머 설정 함수
/*
void setup_timer() {
    struct itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = TIME_QUANTUM * 1000;
    timer.it_interval = timer.it_value;
    setitimer(ITIMER_REAL, &timer, NULL);
}
*/

void alrm_handler(int signum) {
    current_time++;
    //fprintf(fp,"Time tick : %d\n", current_time);  // 확인용
    fflush(stdout);  // 즉시 출력
}


// 자식 프로세스 생성 - 대기 상태로만 생성
void createChildProcesses(int scheduling_algorithm)
{
    for (int i = 0; i < NUM_PROCESSES; i++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {                                           // 자식 프로세스
            childProcessLoop(scheduling_algorithm); // 자식 프로세스는 대기 상태로 작업 루프에 들어감
            exit(0);                                // 작업 루프에서 종료 신호 수신 시 종료
        }
        else if (pid > 0)
        { // 부모 프로세스
            child_pids[i] = pid;
            PCB *pcb = (PCB *)malloc(sizeof(PCB));
            pcb->pid = pid;
            pcb->exe_time = rand() % 10000 + 1;  // cpu burst
            pcb->wait_time = rand() % 10000 + 1; // io burst
            fprintf(fp,"New PID: %d, exe_time: %d, wait_time: %d\n", pcb->pid, pcb->exe_time, pcb->wait_time);
            fflush(stdout);  // 즉시 출력
            if (scheduling_algorithm == 2 || scheduling_algorithm == 4)
            {
                enqueueSortedByExeTime(readyQueue, pcb);
            }
            else
            {
                enqueue(readyQueue, pcb); // 준비 큐에 추가
            }

            //free(pcb);
        }
        else
        {
            perror("fork failed");
            exit(1);
        }
    }
}

void childProcessLoop(int scheduling_algorithm)
{
    // SIGCONT 신호를 처리하는 핸들러 설정
    signal(SIGCONT, handle_sigcont);

    // 스케줄링 알고리즘 설정
    switch (scheduling_algorithm)
    {
    case 1:
        TIME_QUANTUM = 2000000000; // FCFS : 대략 20억 최대치로
        break;
    case 2:
        TIME_QUANTUM = 2000000000; // SJF : 대략 20억 최대치로
        break;
    case 3:
        TIME_QUANTUM = 5; // RR
        break;
    case 4:
        TIME_QUANTUM = 5; // SRTS
        break;    
    default:
        fprintf(fp,"Invalid scheduling algorithm selected.\n");
        fflush(stdout);  // 즉시 출력
        return;
    }


    while (1)
    {
        pause(); // SIGCONT 신호 수신 대기

        Message *msg = (Message *)malloc(sizeof(Message));

        // 부모로부터 메시지 수신
        receiveMessage(msgid, msg, getpid());
        // fprintf(fp,"%d\n", getpid());

        // 메시지를 통해 작업을 구분하여 수행
        if (msg->exe_time > 0 && msg->cpu_io == 1)
        { // CPU 작업이 있는 경우
            int initial_burst = msg->exe_time;
            msg->exe_time = (msg->exe_time < TIME_QUANTUM) ? 0 : msg->exe_time - TIME_QUANTUM;
            fprintf(fp,"[CPU Process] (PID: %d) \nInitial CPU burst:                          %d ms\nRemaining CPU burst after execution:        %d ms\n", msg->pid, initial_burst, msg->exe_time);
            fflush(stdout);  // 즉시 출력
            

            // 부모에게 CPU 작업 업데이트 전송
            Message update_msg = {msg->type, msg->pid, msg->exe_time, msg->wait_time, 0}; // I/O 상태는 이전 상태 유지
            sendMessage(msgid, &update_msg);
            kill(getppid(), SIGUSR1); // 부모에게 신호 전송
        }

        if (msg->wait_time > 0 && msg->cpu_io == 2)
        { // I/O 작업이 있는 경우
            int initial_io_burst = msg->wait_time;
            msg->wait_time = (msg->wait_time < TIME_QUANTUM) ? 0 : msg->wait_time - TIME_QUANTUM;
            fprintf(fp,"[io Process] (PID: %d) \nInitial I/O burst:                          %d ms\nRemaining I/O burst after execution:        %d ms\n\n", msg->pid, initial_io_burst, msg->wait_time);
            fflush(stdout);  // 즉시 출력

            // 부모에게 I/O 작업 업데이트 전송
            Message update_msg = {msg->type, msg->pid, msg->exe_time, msg->wait_time, 0}; // CPU 상태는 이전 상태 유지
            sendMessage(msgid, &update_msg);
            kill(getppid(), SIGUSR2); // 부모에게 신호 전송
        }

        // 모든 작업이 완료되면 종료
        if (msg->exe_time == 0 && msg->wait_time == 0)
        {
            // fprintf(fp,"Child (PID: %d) completed all tasks. Exiting...\n", msg->pid);
            //  activeProcesses--;
            exit(0); // 자식 프로세스 종료
            // break;
        }
        free(msg);

        // 작업 후 일시 정지 상태로 전환
        // kill(msg->pid, SIGSTOP);
    }
}

void scheduleProcesses(int scheduling_algorithm)
{
    signal(SIGALRM, alrm_handler); // SIGALRM 신호를 타이머 핸들러에 연결
    signal(SIGUSR1, handle_sigusr1); // SIGUSR1 신호를 처리할 핸들러 설정
    signal(SIGUSR2, handle_sigusr2);
    

    fprintf(fp,"Starting scheduling...\n");
    fflush(stdout);  // 즉시 출력

    // 타이머 설정
    setup_timer();

    activeProcesses = 10;

    while (activeProcesses != 0)
    {
        pause();  // SIGALRM 발생 대기
        fprintf(fp,"\n\n===================================================\n");
        fflush(stdout);  // 즉시 출력
        fprintf(fp,"Time tick : %d\n\n", current_time);
        fflush(stdout);  // 즉시 출력

        if (readyQueue->count == 0 && waitQueue->count == 0)
        {
            activeProcesses = 0;
        }

        PCB *io_pcb = dequeue(waitQueue); // I/O 작업을 위한 대기 큐

        if (io_pcb != NULL)
        {
            if (io_pcb->wait_time > 0)
            {
                Message io_msg = {io_pcb->pid, io_pcb->pid, io_pcb->exe_time, io_pcb->wait_time, 2};
                sendMessage(msgid, &io_msg);
                kill(io_pcb->pid, SIGCONT);
                pause(); // 자식 프로세스가 SIGUSR1 신호를 보낼 때까지 대기

                // 짧은 시간 동안 대기하여 자식 프로세스가 메시지를 처리할 시간을 줌
                // struct timespec ts;
                // ts.tv_ms = 0;
                // ts.tv_nms = 10000000; // 100 millimsonds
                // nanosleep(&ts, NULL);   // 0.1초 대기

                Message updated_msg;
                if (msgrcv(msgid, &updated_msg, sizeof(Message) - sizeof(long), io_pcb->pid, 0) != -1)
                {
                    // 받은 업데이트 정보로 pcb를 업데이트
                    io_pcb->exe_time = updated_msg.exe_time;
                    io_pcb->wait_time = updated_msg.wait_time;

                    // fprintf(fp,"(Parent) Received updated info from PID %d: exe_time = %d, wait_time = %d\n",
                    //       updated_msg.pid, updated_msg.exe_time, updated_msg.wait_time);
                }
                else
                {
                    perror("Failed to receive updated message");
                }

                if (io_pcb->exe_time == 0 && io_pcb->wait_time == 0)
                {
                    // fprintf(fp,"finish\n");
                    //  activeProcesses--; // 모든 작업 완료
                }
                else if (io_pcb->exe_time > 0)
                {
                    if (scheduling_algorithm == 2 || scheduling_algorithm == 4)
                        enqueueSortedByExeTime(readyQueue, io_pcb);
                    else
                        enqueue(readyQueue, io_pcb);
                }
                else
                {
                    if (scheduling_algorithm == 2 || scheduling_algorithm == 4)
                        enqueueSortedByExeTime(waitQueue, io_pcb);
                    else
                        enqueue(waitQueue, io_pcb);
                }
            }
        }
        else{
            fprintf(fp,"[IO Process] no IO burst \n\n");
            fflush(stdout);  // 즉시 출력
        }

        PCB *cpu_pcb = dequeue(readyQueue); // CPU 작업을 위한 준비 큐
        if (cpu_pcb != NULL)
        {
            // CPU 작업 처리
            if (cpu_pcb->exe_time > 0)
            {
                Message cpu_msg = {cpu_pcb->pid, cpu_pcb->pid, cpu_pcb->exe_time, cpu_pcb->wait_time, 1};
                sendMessage(msgid, &cpu_msg);
                kill(cpu_msg.pid, SIGCONT);
                pause(); // 자식 프로세스가 SIGUSR1 신호를 보낼 때까지 대기

                // 짧은 시간 동안 대기하여 자식 프로세스가 메시지를 처리할 시간을 줌
                // struct timespec ts;
                // ts.tv_ms = 0;
                // ts.tv_nms = 10000000; // 100 millimsonds
                // nanosleep(&ts, NULL);   // 0.1초 대기

                // SIGUSR1 신호를 대기
                // int sig;
                // sigwait(SIGUSR1, &sig);  // SIGUSR1 신호 대기

                Message updated_msg;
                if (msgrcv(msgid, &updated_msg, sizeof(Message) - sizeof(long), cpu_pcb->pid, 0) != -1)
                {
                    // 받은 업데이트 정보로 pcb를 업데이트
                    cpu_pcb->exe_time = updated_msg.exe_time;
                    cpu_pcb->wait_time = updated_msg.wait_time;

                    // fprintf(fp,"(Parent) Received updated info from PID %d: exe_time = %d, wait_time = %d\n",
                    //        updated_msg.pid, updated_msg.exe_time, updated_msg.wait_time);
                }
                else
                {
                    perror("Failed to receive updated message");
                }

                if (cpu_pcb->exe_time == 0 && cpu_pcb->wait_time == 0)
                {
                    // fprintf(fp,"finish\n");
                    //  activeProcesses--; // 모든 작업 완료
                }
                else if (cpu_pcb->exe_time > 0)
                {
                    if (scheduling_algorithm == 2 || scheduling_algorithm == 4)
                        enqueueSortedByExeTime(readyQueue, cpu_pcb);
                    else
                        enqueue(readyQueue, cpu_pcb);
                }
                else
                {
                    if (scheduling_algorithm == 2 || scheduling_algorithm == 4)
                        enqueueSortedByExeTime(waitQueue, cpu_pcb);
                    else
                        enqueue(waitQueue, cpu_pcb);
                }
            }
        }
        else{
            fprintf(fp,"[CPU Process] no CPU burst \n\n");
            fflush(stdout);  // 즉시 출력
        }

        // sleep(1);
    }
    fprintf(fp,"All processes have completed. Scheduling ended.\n\n===================================================\n");
}