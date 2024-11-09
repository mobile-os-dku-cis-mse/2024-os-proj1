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
	my_sigaction(SIGUSR1, null_handler);

	int cpu_burst, io_burst;

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
		my_msgsnd(msqid, getppid(), io_burst);

		printf("process[%d]: finished cpu job\n", getpid());
		pause();
	}
}

