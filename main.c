#include "util/proc.h"
#include "proc_one.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <scheduling_algorithm>\n", argv[0]);
        printf("1 = FCFS, 2 = SJF, 3 = RR, 4 = SRTS\n");
        return 1;
    }

    int scheduling_algorithm = atoi(argv[1]);
    if (scheduling_algorithm < 1 || scheduling_algorithm > 4)
    {
        printf("Invalid scheduling algorithm. Choose 1 = FCFS, 2 = SJF, 3 = RR, 4 = SRTS\n");
        return 1;
    }

    fp = fopen("os_scheculer.txt","w");
    setvbuf(fp, NULL, _IONBF, 0);

    
    srand(time(NULL));

    signal(SIGALRM, alrm_handler); // 타이머 알람 신호 핸들러 설정
    //setup_timer();                 // 타이머 설정

    initQueues();
    initMessageQueue(); // 메시지 큐 초기화

    // 자식 프로세스 생성
    // printf("1\n");
    createChildProcesses(scheduling_algorithm);

    // printf("2\n");
    //  선택한 스케줄링 알고리즘에 따라 프로세스 스케줄링 관리
    if (scheduling_algorithm < 3)
    {
        scheduleProcesses(scheduling_algorithm);
    }
    else{
        scheduleProcesses(scheduling_algorithm);
    }

    // printf("3\n");
    //  모든 자식 프로세스 종료
    // terminateChildProcesses();
    fclose(fp);

    return 0;
}
