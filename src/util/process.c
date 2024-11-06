//
// Created by hochacha on 24. 11. 6.
//

#include "process.h"



void generate_child_process(pid_t* pid, int n_process) {
    for(int i = 0; i < n_process; i++) {
        pid[i] = fork();
        if(pid[i] == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } if (pid[i] == 0) {
            do_work_childs();
        } else {
            // parent process
            scheduler_run();
        }
    }
}
