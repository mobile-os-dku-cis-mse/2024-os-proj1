#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "proc.h"
#include "wrap/wrap.h"
#include "ds/pidq.h"
#include "ds/iopq.h"

#define TIME_QUANTUM 10

int msqid;
int ticks;
int time_slot = TIME_QUANTUM;
pidq running_q;
iopq waiting_q;

void alarm_handler(int)
{
	ticks++;
}

static void enable_ticks()
{
	struct itimerval timer = {{0, 10000}, {1, 0}};
	setitimer(ITIMER_REAL, &timer, NULL);
}

static void disable_ticks()
{
	struct itimerval ntimer = {{0, 0}, {0, 0}};
	setitimer(ITIMER_REAL, &ntimer, NULL);
}

void schedule_running()
{
	int status;
	int io_rq; // io request value.

	status = my_msgrcv(msqid, &io_rq);

	// process has finished its job. push to waiting queue.
	if (status != -1)
	{
		iopq_pair io_entry = (iopq_pair) {io_rq, pidq_pop(&running_q)};
		iopq_push(&waiting_q, io_entry);
		time_slot = TIME_QUANTUM;
	}

	// decrease the remaining time and signal the process.
	else
	{
		// process hasn't finished yet, but its time slot is exhausted.
		if (!time_slot)
		{
			pidq_push(&running_q, pidq_pop(&running_q));
			printf("scheduled out! time for process[%d]\n", pidq_peek(&running_q));
			time_slot = TIME_QUANTUM;
		}

		time_slot--;
		kill(pidq_peek(&running_q), SIGUSR1);
	}
		
}

void schedule_waiting()
{
	// decrease every burst value in the waiting queue.
	for (int i = 0; i < waiting_q.sz; i++)
		waiting_q.mem[i].burst--;

	// if there are finished io jobs, push the process back into the running queue.
	while (!iopq_empty(&waiting_q) && iopq_peek(&waiting_q).burst == 0)
	{
		pid_t done = iopq_pop(&waiting_q).pid;
		pidq_push(&running_q, done);
		kill(done, SIGUSR1);
	}
}

void schedule()
{
	if (!pidq_empty(&running_q))
		schedule_running();

	schedule_waiting();
}

int main()
{
	msqid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
	pidq_init(&running_q, 10);
	iopq_init(&waiting_q, 10);

	// spawn 10 processes.
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

	my_sigaction(SIGALRM, alarm_handler);
	enable_ticks();
	while (ticks < 10000)
	{
		pause();
		schedule();
	}
	disable_ticks();

	pidq_destroy(&running_q);
	iopq_destroy(&waiting_q);
	kill(0, SIGTERM);
	exit(0);
}
