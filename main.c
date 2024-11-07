#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "pidq.h"
#include "iopq.h"

int ticks;
int msqid;
pidq running_q;
iopq waiting_q;

struct msgbuf
{
	long mtype;
	char buf[128];
};

void run()
{
	// get message from parent.
}

void alarm_handler(int)
{
	ticks++;
}

static void enable_ticks()
{
	struct itimerval timer = {{1, 0}, {1, 0}};
	setitimer(ITIMER_REAL, &timer, NULL);

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = alarm_handler;
	sigaction(SIGALRM, &sa, NULL);
}

static void disable_ticks()
{
	struct itimerval ntimer = {{0, 0}, {0, 0}};
	setitimer(ITIMER_REAL, &ntimer, NULL);
}

void schedule()
{
	puts("scheduler invoked!");
	/*
	1. if child has finished job, pop and push to waiting queue.
	2. if child has used all ticks, pop and push to back.
	3. if queue is not empty, signal the child at front.
	4. decrease io time for every process in waiting queue.
	5. if some processes have completed their io jobs, push them back to the running queue.
	*/
}

int main()
{
	msqid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
	pidq_init(&running_q, 10);
	iopq_init(&waiting_q, 10);

	// spawning 10 child processes.
	for (int i = 0; i < 10; i++)
	{
		pid_t pid = fork();

		if (pid)
			pidq_push(&running_q, pid);
		else
		{
			run();
			exit(0);
		}
	}

	enable_ticks();

	while (ticks < 10)
	{
		pause();
		schedule();
	}

	disable_ticks();
	
	pidq_destroy(&running_q);
	iopq_destroy(&waiting_q);
	exit(0);
}
