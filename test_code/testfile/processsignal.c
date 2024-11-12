#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

void timer_handler(int signum) {
    printf("타이머 신호 받음 (signum = %d)\n", signum);
}

int main() {
    struct sigaction sa;
    struct itimerval timer;

    // 타이머 핸들러 설정
    sa.sa_handler = timer_handler;
    sa.sa_flags = SA_RESTART; // 타이머가 반복될 때 핸들러가 다시 설정됨
    sigaction(SIGALRM, &sa, NULL);

    // 타이머 설정 (1초 후 시작, 이후 2초 간격으로 반복)
    timer.it_value.tv_sec = 1;  // 1초 후 처음 타이머 시작
    timer.it_value.tv_usec = 0; // 마이크로초 설정

    timer.it_interval.tv_sec = 2;  // 2초마다 반복
    timer.it_interval.tv_usec = 0; // 마이크로초 설정

    // setitimer를 사용하여 타이머 설정
    setitimer(ITIMER_REAL, &timer, NULL);

    // 메인 프로세스는 계속해서 실행되며 타이머 신호를 기다림
    while (1) {
        // 실제 작업을 하는 곳
        pause(); // 신호를 기다림
    }

    return 0;
}
