#include "mytrace.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>


pthread_mutex_t gc_lock = PTHREAD_MUTEX_INITIALIZER;

int GC_is_ON = 0;

int NB_CHANNEL, NB_CHIP;

#if 0
int64_t get_timestamp(void)
{
    int64_t tt = 0;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    tt = tv.tv_sec;
    tt *= 1000000;
    tt += tv.tv_usec;

    return tt;
}
#endif

int64_t get_timestamp(void)
{
    int64_t r = 0;
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);

    r = ts.tv_sec;
    r *= 1000000;
    r += ts.tv_nsec/1000;

    return r;
}
