
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

#define NO_BLOCKING         -1
#define WHOLE_BLOCKING      0
#define CHANNEL_BLOCKING    1
#define CHIP_BLOCKING       2

#define DEFAULT_INTERFACE   0
#define EBUSY_INTERFACE     4
#define GCT_INTERFACE       5

extern pthread_mutex_t gc_lock;
extern int GC_is_ON;
extern int NB_CHANNEL, NB_CHIP;

int64_t get_timestamp(void);

#define mylog(fmt, ...) \
    do { \
            fprintf(stderr, "[%" PRId64 "] %s(): " fmt, get_timestamp(), \
                    __func__, ## __VA_ARGS__); \
    } while (0)

#endif
