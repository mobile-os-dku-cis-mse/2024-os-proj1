#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>

int ticks;

void alarm_handler(int)
{
	ticks++;

	// printf is non re-entrant; should not be called in a signal handler.
	printf("timer invoked\n");
}

int main()
{
	// itimer setup.
	struct itimerval timer = {{1, 0}, {1, 0}};
	setitimer(ITIMER_REAL, &timer, NULL);

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = alarm_handler;
	sigaction(SIGALRM, &sa, NULL);

	pid_t children[10];
	pid_t ppid = getpid();

	for (int i = 0; i < 10; i++)
	{
		pid_t pid = fork();

		if (pid < 0)
		{
			// kill all children.
			// ...
			exit(EXIT_FAILURE);
		}

		if (pid)
		{
			children[i] = pid;
			continue;
		}
		else
		{
			printf("child process[%08X]\n", getpid());
			exit(EXIT_SUCCESS);
		}
	}

	// wait until every child process terminates.
	while (wait(NULL) > 0);

	while (ticks < 10)
		sleep(1);
	
	struct itimerval ntimer = {{0, 0}, {0, 0}};
	setitimer(ITIMER_REAL, &ntimer, NULL);

	printf("parent process[%08X]\n", getpid());
	return 0;
}
