#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "pidq.h"
#include "iopq.h"

#define TIME_QUANTUM 10

int ticks;
int msqid;
pidq running_q;
iopq waiting_q;

struct msgbuf
{
	long mtype;
	int value;
};

void child_handler(int) {}

void run()
{
	int cpu_burst, io_burst;

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = child_handler;
	sigaction(SIGUSR1, &sa, NULL);

	while (1)
	{
		cpu_burst = rand() % 100 + 1;
		io_burst = rand() % 100 + 1;

		while (cpu_burst)
		{
			pause();
			cpu_burst--;
			printf("process[%d]: %d left\n", getpid(), cpu_burst);
		}
		
		// send the io-burst value to parent.
		struct msgbuf msg;
		memset(&msg, 0, sizeof(msg));
		msg.mtype = getppid();
		msg.value = io_burst;
		msgsnd(msqid, &msg, sizeof(msg)-sizeof(long), 0);

		printf("process[%d]: finished cpu job\n", getpid());
		pause();
	}
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

int remaining = TIME_QUANTUM;

void schedule()
{
	int status;
	struct msgbuf msg;

	if (pidq_empty(&running_q))
		goto io_handling;

cpu_handling:
	memset(&msg, 0, sizeof(msg));
	status = msgrcv(msqid, &msg, sizeof(msg)-sizeof(long), getpid(), IPC_NOWAIT);

	// child process has finished its cpu job. push to waiting queue.
	if (status != -1)
	{
		iopq_pair io_entry = (iopq_pair) {msg.value, pidq_pop(&running_q)};
		iopq_push(&waiting_q, io_entry);
		remaining = TIME_QUANTUM;
	}

	// decrease the remaining time and signal the child.
	else
	{
		// child process hasn't finished yet, but its time slot is exhausted.
		if (!remaining)
		{
			pidq_push(&running_q, pidq_pop(&running_q));
			printf("scheduled out! time for process[%d]\n", pidq_peek(&running_q));
			remaining = TIME_QUANTUM;
		}

		remaining--;
		kill(pidq_peek(&running_q), SIGUSR1);
	}
		
io_handling:
	// decrease every burst value in the waiting queue.
	for (int i = 0; i < waiting_q.sz; i++)
		waiting_q.mem[i].burst--;

	// if there are finished io jobs, push the child back into the running queue.
	while (!iopq_empty(&waiting_q) && iopq_peek(&waiting_q).burst == 0)
	{
		pid_t fin = iopq_pop(&waiting_q).pid;
		pidq_push(&running_q, fin);
		kill(fin, SIGUSR1);
	}
}

int main()
{
	// seed the random generator.
	srand(time(0));

	msqid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
	pidq_init(&running_q, 10);
	iopq_init(&waiting_q, 10);

	// spawn 10 child processes.
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

	while (ticks < 100)
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
