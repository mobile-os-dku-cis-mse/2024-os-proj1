#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "util/process.h"

int main(int argc, char * arg[])
{
	int n_process = 10;
	pcb_queue* pcb_queue_o = malloc(sizeof(pcb_queue) * n_process);

	launch_scheduler_and_worker(pcb_queue_o, n_process);

	return 0;
}
