#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <fcntl.h>

#include "proc.h"
#include "wrap/wrap.h"
#include "ds/pidq.h"
#include "ds/iopq.h"

#define TIME_QUANTUM 10

int dump_fd;
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
	int rq_val; // request value.

	if (pidq_empty(&running_q))
		return;

	my_msgsnd(msqid, pidq_peek(&running_q), 1);
	my_msgrcv(msqid, &rq_val, 0);
	time_slot--;

	// log active process.
	dprintf(dump_fd, "process[%d] gets cpu time, %d remaining\n", pidq_peek(&running_q), rq_val);

	// check if the process has finished its job.
	if (!rq_val)
	{
		my_msgrcv(msqid, &rq_val, 0);
		iopq_push(&waiting_q, (iopq_pair) {rq_val, pidq_pop(&running_q)});
		time_slot = TIME_QUANTUM;

		if (pidq_empty(&running_q))
			return;
	}

	// process hasn't finished yet, but its time slot is exhausted.
	if (!time_slot)
	{
		pidq_push(&running_q, pidq_pop(&running_q));
		time_slot = TIME_QUANTUM;
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
		kill(done, SIGALRM);

		dprintf(dump_fd, "process[%d] resumes to running queue\n", done);
	}
}

void dump_running()
{
	dprintf(dump_fd, "running queue dump\n");

	for (int i = 0; i < running_q.sz; i++)
		dprintf(dump_fd, "\tprocess[%d]\n", pidq_at(&running_q, i));
}

void dump_waiting()
{
	dprintf(dump_fd, "waiting queue dump\n");

	for (int i = 0; i < waiting_q.sz; i++)
		dprintf(dump_fd, "\tprocess[%d]:%d\n", waiting_q.mem[i].pid, waiting_q.mem[i].burst);
}

void schedule()
{
	printf("scheduler invoked, tick %d\n", ticks);
	dprintf(dump_fd, "log message at tick %d\n", ticks);

	schedule_running();
	schedule_waiting();
	dump_running();
	dump_waiting();

	dprintf(dump_fd, "\n\n");
}

int main()
{
	dump_fd = creat("schedule_dump.txt", 0664);
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
	close(dump_fd);

	puts("log messages dumped to schedule_dump.txt");
	kill(0, SIGTERM);
	exit(0);
}
