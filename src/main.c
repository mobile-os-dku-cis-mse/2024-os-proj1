#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "util/process.h"

int main(int argc, char * arg[])
{
	int n_process = 10;
	pid_t* pid_arr = malloc(sizeof(pid_t) * n_process);

	launch_scheduler_and_worker(pid_arr, n_process);

	return 0;
}
