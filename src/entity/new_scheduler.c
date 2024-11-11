//
// Created by hochacha on 24. 11. 11.
//

#include "new_scheduler.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <bits/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>

#include "src/util/message_queue.h"
#include "src/util/pid_queue.h"
#include "src/util/timer.h"

unsigned int current_time = 0;
pcb_queue* ready_queue_schd = NULL;
pcb_queue* waiting_queue_schd = NULL;
pcb_t* current_process = NULL;

void sigint_handler(int sig) {
  if (sig == SIGINT) {
    pcb_t* process;
    while ((process = dequeue_pcb(ready_queue_schd)) != NULL) {
      kill(process->pid, SIGTERM);
      waitpid(process->pid, NULL, 0);
    }
    while ((process = dequeue_pcb(waiting_queue_schd)) != NULL) {
      kill(process->pid, SIGTERM);
      waitpid(process->pid, NULL, 0);
    }
    if (current_process != NULL) {
      kill(current_process->pid, SIGTERM);
      waitpid(current_process->pid, NULL, 0);
    }
    msgctl(msg_queue_id, IPC_RMID, NULL);  // 메시지 큐 삭제
    exit(0);
  }
}


void alarm_handler(int sig) {
  current_time++;
  if(current_process->remaining_time > 0) {
    // 이미 실행 중인 프로세스가 존재할 때

  }else {
    // 실행 중인 프로세스가 없을 때
    if (is_queue_empty(ready_queue_schd) != 1) {
      // 준비 큐에서 프로세스 팝

    }else {
      // 준비 큐도 비어서
      if (is_queue_empty(waiting_queue_schd) != 1) {
        // 대기 큐에 프로세스들이 있으면 대기
      }else {
        // 대기 큐에도 없으면 자식 프로세스 전부 죽었으므로 종료
      }
    }
  }

  // io 처리 (다같이 1 감소)
}




void io_schedule_handler(int sig) {

}

void scheduler_init(pcb_queue* ready_queue) {
  // wait! for scheduler, SIGUSR2 is work for I/O scheduler
  if(signal(SIGUSR2, io_schedule_handler) == SIG_ERR) {
    perror("signal");
    sigint_handler(SIGINT);
    exit(1);
  }

  if(signal(SIGUSR1, sigint_handler) == SIG_ERR) {
    perror("signal");
    sigint_handler(SIGINT);
    exit(1);
  }

  if(init_msg_queue() == -1) {
    perror("init_msg_queue");
    sigint_handler(SIGINT);
    exit(1);
  }

  if(signal(SIGALRM, alarm_handler) == SIG_ERR) {
    perror("signal");
    sigint_handler(SIGINT);
    exit(1);
  }

  ready_queue_schd = ready_queue;
  waiting_queue_schd = create_pcb_queue();
  setup_timer();
}

void scheduler_run(pcb_queue* ready_queue, int n_process) {
  scheduler_init(ready_queue);
  while(1);
}