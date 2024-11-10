#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

#include "wrap/wrap.h"

extern int msqid;

void null_handler(int) {}

void run()
{
	// each process needs to be seeded differently; we use a pid here.
	srand(getpid());
	my_sigaction(SIGALRM, null_handler);

	int cpu_burst, io_burst;
	int slice;
	int sched_pid = getppid();

	while (1)
	{
		cpu_burst = rand() % 50 + 1;
		io_burst = rand() % 200 + 1;

		while (cpu_burst)
		{
			my_msgrcv(msqid, &slice, 0);
			my_msgsnd(msqid, sched_pid, --cpu_burst);
		}
		
		// send the io-burst value to parent.
		my_msgsnd(msqid, sched_pid, io_burst);
		pause();
	}
}

