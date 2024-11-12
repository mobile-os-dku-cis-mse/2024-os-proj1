/* signal test */
/* sigaction */
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

void signal_handler(int signo);
int count = 0;

/*
시간 틱마다 대기Q의 모든 자식 프로세스의 io_burst값을 감소시킴
io_busrt == 0 -> 해당 프로세스를 다시 런Q로 옮겨서 다음 스케줄링 때 CPU 받을 수 있도록 함.
*/


int main()
{
	struct sigaction old_sa;
	struct sigaction new_sa;
	memset(&new_sa, 0, sizeof(new_sa));
	new_sa.sa_handler = &signal_handler;
	sigaction(SIGALRM, &new_sa, &old_sa);

	struct itimerval new_itimer, old_itimer;
	new_itimer.it_interval.tv_sec = 1;
	new_itimer.it_interval.tv_usec = 0;
	new_itimer.it_value.tv_sec = 1;
	new_itimer.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &new_itimer, &old_itimer);
	while (1){
		printf("a");
		pause();
	}
	return 0;
}

void signal_handler(int signo)
{
	printf("signaled! %d \n", signo);
	count++;

	if (count == 3) exit(0);
}












