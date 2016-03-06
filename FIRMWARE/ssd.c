// File: ssd.c
// Date: 2014. 12. 03.
// Author: Jinsoo Yoo (jedisty@hanyang.ac.kr)
// Copyright(c)2014
// Hanyang University, Seoul, Korea
// Embedded Software Systems Laboratory. All right reserved

#include "ssd.h"
#include "ftl.h"
#include "ssd_util.h"
#include "firm_buffer_manager.h"
#include "mytrace.h"

#ifdef GET_WORKLOAD
#include <time.h>
#include <sys/time.h>
#endif

//#define DEBUG_LATENCY

void SSD_INIT(IDEState *s)
{
    set_ssd_name(s);
	FTL_INIT(s);
}

void SSD_TERM(IDEState *s)
{	
	FTL_TERM(s);
}

void SSD_WRITE(IDEState *s, int32_t sector_num, unsigned int length)
{
#ifdef DEBUG_LATENCY
    int64_t tstart = get_timestamp();
#endif

    SSDState *ssd = &(s->ssd);
#if defined SSD_THREAD	
    pthread_mutex_lock(&ssd->cq_lock);
    DEQUEUE_COMPLETED_READ(s); 
    pthread_mutex_unlock(&ssd->cq_lock);

    if (EVENT_QUEUE_IS_FULL(s, WRITE, length)) {
        ssd->w_queue_full = 1;
        while (ssd->w_queue_full == 1) {
            pthread_cond_signal(&ssd->eq_ready);
        }
    }

    pthread_mutex_lock(&ssd->eq_lock);
    ENQUEUE_IO(s, WRITE, sector_num, length);
    pthread_mutex_unlock(&ssd->eq_lock);

#ifdef SSD_THREAD_MODE_1
    pthread_cond_signal(&ssd->eq_ready);
#endif

#elif defined FIRM_IO_BUFFER
    DEQUEUE_COMPLETED_READ(s);
    if (EVENT_QUEUE_IS_FULL(s, WRITE, length)) {
        SECURE_WRITE_BUFFER(s);
    }
    ENQUEUE_IO(s, WRITE, sector_num, length);
#else
    FTL_WRITE(s, sector_num, length);
#endif

#ifdef DEBUG_LATENCY
    int64_t tend = get_timestamp();

    printf("[%s] write (%" PRId32 ", %d): %" PRId64 " us\n", get_ssd_name(s), 
            sector_num, length, tend - tstart);
#endif
}


int SSD_READ(IDEState *s, int32_t sector_num, unsigned int length)
{
#ifdef DEBUG_LATENCY
    int64_t tstart = get_timestamp();
#endif
    SSDState *ssd = &(s->ssd);
    int ret = SUCCESS;

#if defined SSD_THREAD

    pthread_mutex_lock(&ssd->cq_lock);
    DEQUEUE_COMPLETED_READ(s);
    pthread_mutex_unlock(&ssd->cq_lock);

    if (EVENT_QUEUE_IS_FULL(s, READ, length)) {
        ssd->r_queue_full = 1;
        while (ssd->r_queue_full == 1) {
            pthread_cond_signal(&ssd->eq_ready);
        }
    }

    pthread_mutex_lock(&ssd->eq_lock);
    ENQUEUE_IO(s, READ, sector_num, length);
    pthread_mutex_unlock(&ssd->eq_lock);

#ifdef SSD_THREAD_MODE_1
    pthread_cond_signal(&ssd->eq_ready);
#endif

#elif defined FIRM_IO_BUFFER
    DEQUEUE_COMPLETED_READ(s);
    if (EVENT_QUEUE_IS_FULL(s, READ, length)) {
        SECURE_READ_BUFFER(s);
    }

    ENQUEUE_IO(s, READ, sector_num, length);

#else
    ret = FTL_READ(s, sector_num, length);

#endif

#ifdef DEBUG_LATENCY
    int64_t tend = get_timestamp();

    printf("[%s] read (%" PRId32 ", %d): %" PRId64 " us\n", get_ssd_name(s), 
            sector_num, length, tend - tstart);
#endif

    return ret;
}

void SSD_DSM_TRIM(IDEState *s, unsigned int length, void *trim_data)
{
}

int SSD_IS_SUPPORT_TRIM(void)
{
	return 1; /* Coperd: cheat OS about the TRIM support */
}

