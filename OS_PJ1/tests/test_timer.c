#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include "timer.h"

void test_timer()
{
    setup_timer();
    // Trigger the alarm handler manually
    alarm_handler(SIGALRM);
    // Additional assertions based on expected behavior can be added here
}
#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include "timer.h"

void test_timer()
{
    setup_timer();
    // Trigger the alarm handler manually
    alarm_handler(SIGALRM);
    // Additional assertions based on expected behavior can be added here
}
