#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "entity/scheduler.h"
#include "util/process.h"
FILE* IO_result;
int main(int argc, char * arg[])
{
	int n_process = 10;
	pcb_queue* ready_queue_o = malloc(sizeof(pcb_queue) * n_process);
	IO_result = freopen("scheduler_dump.txt", "w", stdout);
	launch_scheduler_and_worker(ready_queue_o, n_process);

	scheduler_run(ready_queue_o, 10);
	return 0;
}
