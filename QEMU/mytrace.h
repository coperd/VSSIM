
#ifndef __MYTRACE_H
#define __MYTRACE_H

#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <dlfcn.h>
#include <pthread.h>

//#define MYTRACE 1

#define MYTRACE2 1
//#define SLEEP

extern pthread_mutex_t gc_lock;
extern int GC_is_ON;
extern int64_t GC_WHOLE_ENDTIME;
extern int NB_CHANNEL, NB_CHIP;

int64_t get_timestamp(void);

#define mylog(fmt, ...) \
    do { \
            fprintf(stderr, "[%" PRId64 "] %s(): " fmt, get_timestamp(), \
                    __func__, ## __VA_ARGS__); \
    } while (0)

#endif
