//
// Created by hochacha on 24. 11. 6.
//

#include "process.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "pid_queue.h"
#include "../entity/child.h"
#include "../entity/scheduler.h"

// process.c concern about launching the scheduler (parent) and
// worker (child) process.


// launching process
void launch_scheduler_and_worker(pcb_queue* ready_queue, int n_process) {
    for(int i = 0; i < n_process; i++) {
        pid_t pid = fork();

        if(pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {  // Child process
            do_work_child();
            exit(EXIT_SUCCESS);  // Ensure child exits after work is done
        } else {
            // create pcb
            pcb_t* child_pcb = (pcb_t*)calloc(1, sizeof(pcb_t));
            child_pcb->pid = pid;
            child_pcb->state = PROCESS_READY;

            enqueue_pcb(ready_queue, pid);
        }
    }

    // start schedule
    scheduler_run(ready_queue, n_process);
}