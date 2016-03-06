
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
extern int64_t GC_timestamp;

int64_t get_timestamp(void);

#define log(fmt, ...) \
    do { \
            fprintf(stderr, "[%" PRId64 "] %d:%s(): " fmt, get_timestamp(), \
                    __LINE__, __func__, __VA_ARGS__); \
    } while (0)


#endif
