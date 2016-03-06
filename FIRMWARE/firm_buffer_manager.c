// File: firm_buffer_manager.c
// Date: 2014. 12. 03.
// Author: Jinsoo Yoo (jedisty@hanyang.ac.kr)
// Copyright(c)2014
// Hanyang University, Seoul, Korea
// Embedded Software Systems Laboratory. All right reserved

#include "firm_buffer_manager.h"
#include "ftl.h"
#include "ide.h"
#include <pthread.h>

static void die2(int err, const char *what)
{
    fprintf(stderr, "%s failed: %s\n", what, strerror(err));
    abort();
}

//#define FIRM_IO_BUF_DEBUG
//#define SSD_THREAD_DEBUG

void INIT_IO_BUFFER(IDEState *s)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);
    int nb_rb_frame = ssdconf->read_buffer_frame_nb;
    int nb_wb_frame = ssdconf->write_buffer_frame_nb;
    int sectsz = ssdconf->sector_size;


    /* Allocate event queue structure */
    ssd->e_queue = qemu_mallocz(sizeof(event_queue));
    ssd->e_queue->entry_nb = 0;
    ssd->e_queue->head = NULL;
    ssd->e_queue->tail = NULL;

    /* Initialize valid array of event queue */
    INIT_WB_VALID_ARRAY(s);

    /* Allocate completed event queue structure */
    ssd->c_e_queue = qemu_mallocz(sizeof(event_queue));
    ssd->c_e_queue->entry_nb = 0;
    ssd->c_e_queue->head = NULL;
    ssd->c_e_queue->tail = NULL;

    /* Allocate Write Buffer in DRAM */
    ssd->write_buffer = qemu_mallocz(nb_wb_frame*sectsz);
    ssd->write_buffer_end = ssd->write_buffer + nb_wb_frame * sectsz;	

    /* Allocate Read Buffer in DRAM */
    ssd->read_buffer = qemu_mallocz(nb_rb_frame*sectsz);
    ssd->read_buffer_end = ssd->read_buffer + nb_rb_frame * sectsz;	

    assert(ssd->e_queue && ssd->c_e_queue); 
    assert(ssd->write_buffer && ssd->read_buffer);

    /* Initialization Buffer Pointers */
    ssd->ftl_write_ptr = ssd->write_buffer;
    ssd->sata_write_ptr = ssd->write_buffer;
    ssd->write_limit_ptr = ssd->write_buffer;

    ssd->ftl_read_ptr = ssd->read_buffer;
    ssd->sata_read_ptr = ssd->read_buffer;
    ssd->read_limit_ptr = ssd->read_buffer;
    ssd->last_read_entry = NULL;

    /* Initialization Other Global Variable */
    ssd->empty_read_buffer_frame = ssdconf->read_buffer_frame_nb; 
    ssd->empty_write_buffer_frame = ssdconf->write_buffer_frame_nb;

#ifdef SSD_THREAD
    ssd->r_queue_full = 0;
    ssd->w_queue_full = 0;
    pthread_cond_init(&ssd->eq_ready, NULL);
    pthread_mutex_init(&ssd->eq_lock, NULL);
    pthread_mutex_init(&ssd->cq_lock, NULL);

    int ret = pthread_create(&ssd->ssd_thread_id, NULL, SSD_THREAD_MAIN_LOOP, s);
    if (ret) die2(ret, "pthread_create");
#endif
}

void TERM_IO_BUFFER(IDEState *s)
{
    SSDState *ssd = &(s->ssd);

    /* Flush all event in event queue */
    FLUSH_EVENT_QUEUE_UNTIL(s, ssd->e_queue->tail);

    /* Deallocate Buffer & Event queue */
    qemu_free(ssd->write_buffer);
    qemu_free(ssd->read_buffer);
    qemu_free(ssd->e_queue);
    qemu_free(ssd->c_e_queue);
}

void INIT_WB_VALID_ARRAY(IDEState *s)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);
    int nb_wb_frame = ssdconf->write_buffer_frame_nb;

    ssd->wb_valid_array = qemu_mallocz(sizeof(char)*nb_wb_frame);
    memset(ssd->wb_valid_array, '0', nb_wb_frame);
}

#ifdef SSD_THREAD
/* Coperd: host loop to handle all the events in the event_queue */
void *SSD_THREAD_MAIN_LOOP(void *s)
{
    IDEState *ide = (IDEState *)s;
    SSDState *ssd = &(ide->ssd);

    while (1) {
        pthread_mutex_lock(&ssd->eq_lock);

#if defined SSD_THREAD_MODE_1
        while (ssd->e_queue->entry_nb == 0) {
#ifdef SSD_THREAD_DEBUG
            printf("[%s, %s] wait signal..\n", ssd->name, __func__);
#endif
            pthread_cond_wait(&ssd->eq_ready, &ssd->eq_lock);
        }
#ifdef SSD_THREAD_DEBUG
        printf("[%s] Get up! \n", __func__);
#endif
        DEQUEUE_IO(s);

#elif defined SSD_THREAD_MODE_2
        while (ssd->r_queue_full == 0 && ssd->w_queue_full == 0) {
            pthread_cond_wait(&ssd->eq_ready, &ssd->eq_lock);
        }
        if (ssd->r_queue_full == 1) {
            SECURE_READ_BUFFER(s);
            ssd->r_queue_full = 0;
        } else if (ssd->w_queue_full == 1) {
            SECURE_WRITE_BUFFER(s);
            ssd->w_queue_full = 0;
        } else {
            printf("ERROR[%s] Wrong signal \n", __func__);
        }
#endif

        pthread_mutex_unlock(&ssd->eq_lock);
    }

}
#endif

void ENQUEUE_IO(IDEState *s, int io_type, int32_t sector_num, unsigned int length)
{
    SSDState *ssd = &(s->ssd);
    //SSDConf *ssdconf = &(ssd->param);

#ifdef FIRM_IO_BUF_DEBUG
    printf("[%s, %s] Start.\n", ssd->name, __func__);
#endif

#ifdef GET_FIRM_WORKLOAD
    FILE *fp_workload = fopen("./data/workload_firm.txt","a");
    struct timeval tv;
    struct tm *lt;
    double curr_time;
    gettimeofday(&tv, 0);
    lt = localtime(&(tv.tv_sec));
    curr_time = lt->tm_hour * 3600 + lt->tm_min * 60 + 
                lt->tm_sec + (double)tv.tv_usec / (double)1000000;
    if (io_type == READ) {
        fprintf(fp_workload, "%lf %d %u %x R\n", curr_time, sector_num, length, 1);
    } else if (io_type == WRITE) {
        fprintf(fp_workload, "%lf %d %u %x W\n", curr_time, sector_num, length, 0);
    }
    fclose(fp_workload);
#endif

    /* Check event queue depth */
#ifdef GET_QUEUE_DEPTH
    FILE *fp_workload = fopen("./data/queue_depth.txt","a");
    struct timeval tv;
    struct tm *lt;
    double curr_time;
    gettimeofday(&tv, 0);
    lt = localtime(&(tv.tv_sec));
    curr_time = lt->tm_hour * 3600 + lt->tm_min * 60 + 
                lt->tm_sec + (double)tv.tv_usec / (double)1000000;

    int n_read_event = COUNT_READ_EVENT(s);
    int n_write_event = ssd->e_queue->entry_nb - n_read_event;
    if (io_type == READ)
        fprintf(fp_workload, "%lf\tR\t%d\t%d\t%d\t%d\t%u\n", curr_time, 
                ssd->e_queue->entry_nb, n_read_event, n_write_event, sector_num, 
                length);
    else if (io_type == WRITE)
        fprintf(fp_workload, "%lf\tW\t%d\t%d\t%d\t%d\t%u\n", curr_time, 
                ssd->e_queue->entry_nb, n_read_event, n_write_event, sector_num, 
                length);

    fclose(fp_workload);
#endif

    if (io_type == READ) {
        ENQUEUE_READ(s, sector_num, length);
    } else if (io_type == WRITE) {
        ENQUEUE_WRITE(s, sector_num, length);
    } else {
        printf("ERROR[%s, %s] Wrong IO type.\n", ssd->name, __func__);
    }

#ifdef FIRM_IO_BUF_DEBUG
    printf("[%s, %s] End.\n", ssd->name, __func__);
#endif
}


int DEQUEUE_IO(IDEState *s)
{
    SSDState *ssd = &(s->ssd);
    int ret = FAIL;

#ifdef FIRM_IO_BUF_DEBUG
    printf("[%s, %s] Start.\n", ssd->name, __func__);
#endif
    assert(ssd->e_queue);
    if (ssd->e_queue->entry_nb == 0 || ssd->e_queue->head == NULL) {
        printf("ERROR[%s] There is no event. \n", __func__);
        return ret;
    }

    event_queue_entry *e_q_entry = ssd->e_queue->head;

    int io_type = e_q_entry->io_type;
    int valid = e_q_entry->valid;
    int32_t sector_num = e_q_entry->sector_num;
    unsigned int length = e_q_entry->length;
    void *buf = e_q_entry->buf;


    /* Deallocation event queue entry */
    ssd->e_queue->entry_nb--;
    if (ssd->e_queue->entry_nb == 0) {
        ssd->e_queue->head = NULL;
        ssd->e_queue->tail = NULL;
    } else {
        /* Coperd: we need to dequue the head entry */
        ssd->e_queue->head = e_q_entry->next; 
    }

    if (e_q_entry->io_type == WRITE) {
        qemu_free(e_q_entry);
    } else {
        if (e_q_entry == ssd->last_read_entry) {
            ssd->last_read_entry = NULL;
        }
    }

    if (valid == VALID) {	

        /* Call FTL Function */
        if (io_type == READ) {
            if (buf != NULL) {
                /* The data is already read from write buffer */
            } else {
                /* Allocate read pointer */
                e_q_entry->buf = ssd->ftl_read_ptr;

                // Coperd: unreadable here, set error
                if (sector_num >= 100 && sector_num <= 200) {
                    e_q_entry->has_error = 1;
                } 

                ret = FTL_READ(s, sector_num, length);
            }
        } else if (io_type == WRITE) {
            FTL_WRITE(s, sector_num, length);
        } else {
            printf("ERROR[%s, %s] Invalid IO type. \n", ssd->name, __func__);
        }
    }

#ifdef SSD_THREAD
    pthread_mutex_lock(&ssd->cq_lock);
#endif

    if (io_type == READ) { 
        /* Move event queue entry to completed event queue */
        e_q_entry->next = NULL;
        if (ssd->c_e_queue->entry_nb == 0) {
            ssd->c_e_queue->head = e_q_entry;
            ssd->c_e_queue->tail = e_q_entry;
        } else { /* Coperd: move the entry to the tail */
            ssd->c_e_queue->tail->next = e_q_entry;
            ssd->c_e_queue->tail = e_q_entry;
        }
        ssd->c_e_queue->entry_nb++;
    }

#ifdef SSD_THREAD
    pthread_mutex_unlock(&ssd->cq_lock);
#endif

#ifdef FIRM_IO_BUF_DEBUG
    printf("[%s] End.\n", __func__);
#endif

    return ret;
}

void DEQUEUE_COMPLETED_READ(IDEState *s)
{
    SSDState *ssd = &(s->ssd);
    //SSDConf *ssdconf = &(ssd->param);

#ifdef FIRM_IO_BUF_DEBUG
	printf("[%s, %s] Start.\n", ssd->name, __func__);
#endif

    assert(ssd->c_e_queue);
	
	if (ssd->c_e_queue->entry_nb == 0 || ssd->c_e_queue->head == NULL) {
#ifdef FIRM_IO_BUF_DEBUG
		printf("[%s] There is no completed read event. \n", __func__);
#endif
		return;
	}

	event_queue_entry *c_e_q_entry = ssd->c_e_queue->head;
	event_queue_entry *temp_c_e_q_entry = NULL;

#ifdef FIRM_IO_BUF_DEBUG
	printf("[%s, %s] entry number %d\n", ssd->name, __func__, 
                                         ssd->c_e_queue->entry_nb);
#endif

	while (c_e_q_entry != NULL) {

	    /* Read data from buffer to host */
        READ_DATA_FROM_BUFFER_TO_HOST(s, c_e_q_entry);

	    /* Remove completed read IO from queue */
	    temp_c_e_q_entry = c_e_q_entry;
	    c_e_q_entry = c_e_q_entry->next;

	    /* Update completed event queue data */
	    ssd->c_e_queue->entry_nb--;

	    /* Deallication completed read IO */
	    qemu_free(temp_c_e_q_entry);
	}

    /* Coperd: by now, c_e_queue should be empty */
    assert(ssd->c_e_queue->entry_nb == 0);

    ssd->c_e_queue->head = NULL;
    ssd->c_e_queue->tail = NULL;


#ifdef FIRM_IO_BUF_DEBUG
	printf("[%s, %s] End.\n", ssd->name, __func__);
#endif
}


void ENQUEUE_READ(IDEState *s, int32_t sector_num, unsigned int length)
{
    SSDState *ssd = &(s->ssd);
    //SSDConf *ssdconf = &(ssd->param);

#ifdef FIRM_IO_BUF_DEBUG
    printf("[%s, %s] Start.\n", ssd->name, __func__);
#endif
    /* Coperd: actually, e_queue_entry->buf stores nothing in SSD emualtion */
    void *p_buf = NULL; 
    event_queue_entry *ret_e_q_entry = NULL;
    event_queue_entry *new_e_q_entry = NULL;
    event_queue_entry *temp_e_q_entry = qemu_mallocz(sizeof(event_queue_entry));

    /* Make New Read Event */
    new_e_q_entry = ALLOC_NEW_EVENT(READ, sector_num, length, p_buf);

    /* Coperd: add new I/O event to e_queue */
    if (ssd->e_queue->entry_nb == 0) {
        ssd->e_queue->head = new_e_q_entry;
        ssd->e_queue->tail = new_e_q_entry;
        ssd->last_read_entry = new_e_q_entry;
    } else {
        ret_e_q_entry = CHECK_IO_DEPENDENCY_FOR_READ(s, sector_num, length);

        if (ret_e_q_entry != NULL) {

            temp_e_q_entry->sector_num = ret_e_q_entry->sector_num;
            temp_e_q_entry->length = ret_e_q_entry->length;
            temp_e_q_entry->buf = ret_e_q_entry->buf;

            /* 
             * Coperd: Since the new IO request is overlapped with the existing 
             * ones, we "should" directly read data from buffer, no need to 
             * execute it??? Why should we flush all the events before 
             * ret_e_q_entry??? 
             */
            FLUSH_EVENT_QUEUE_UNTIL(s, ret_e_q_entry);

            if (temp_e_q_entry->sector_num <= sector_num && (sector_num + length) 
                    <= (temp_e_q_entry->sector_num + temp_e_q_entry->length)) {

                new_e_q_entry->buf = ssd->ftl_read_ptr;
                COPY_DATA_TO_READ_BUFFER(s, new_e_q_entry, temp_e_q_entry);
            }

            ret_e_q_entry = NULL;
        }	

        /* If there is no read event */
        if (ssd->last_read_entry == NULL) {
            if (ssd->e_queue->entry_nb == 0) {
                ssd->e_queue->head = new_e_q_entry;
                ssd->e_queue->tail = new_e_q_entry;
            } else {
                new_e_q_entry->next = ssd->e_queue->head;
                ssd->e_queue->head = new_e_q_entry;
            }
        } else {
            if (ssd->last_read_entry == ssd->e_queue->tail) {
                ssd->e_queue->tail->next = new_e_q_entry;
                ssd->e_queue->tail = new_e_q_entry;
            } else {
                new_e_q_entry->next = ssd->last_read_entry->next;
                ssd->last_read_entry->next = new_e_q_entry;
            }
        }
        ssd->last_read_entry = new_e_q_entry;
    }

    ssd->e_queue->entry_nb++;

    /* Update empty read buffer frame number */
    ssd->empty_read_buffer_frame -= length;

    qemu_free(temp_e_q_entry);

#ifdef FIRM_IO_BUF_DEBUG
    printf("[%s, %s] End.\n", ssd->name, __func__);
#endif
}

void ENQUEUE_WRITE(IDEState *s, int32_t sector_nb, unsigned int length)
{
    SSDState *ssd = &(s->ssd);
    //SSDConf *ssdconf = &(ssd->param);

#ifdef FIRM_IO_BUF_DEBUG
    printf("[%s, %s] Start.\n", ssd->name, __func__);
#endif
    event_queue_entry *e_q_entry;
    event_queue_entry *new_e_q_entry = NULL;

    void *p_buf = NULL;
    int invalid_len;

    int flag_allocated = 0;

    /* Write SATA data to write buffer */
    p_buf = ssd->sata_write_ptr;
    WRITE_DATA_TO_BUFFER(s, length);

    if (ssd->last_read_entry != NULL)
        e_q_entry = ssd->last_read_entry->next;
    else
        e_q_entry = ssd->e_queue->head;

    /* Check pending write event */
    while (e_q_entry != NULL) {

        /* Check if there is overwrited event */
        if (e_q_entry->valid == VALID && 
                CHECK_OVERWRITE(s, e_q_entry, sector_nb, length) == SUCCESS) {

            /* Update event entry validity */
            e_q_entry->valid = INVALID;

            /* Update write buffer valid array */
            UPDATE_WB_VALID_ARRAY(s, e_q_entry, 'I');
        }

        e_q_entry = e_q_entry->next;
    }

    /* Check if the event is prior sequential event */
    if (CHECK_SEQUENTIALITY(ssd->e_queue->tail, sector_nb) == SUCCESS) {
        /* Update the last write event */
        ssd->e_queue->tail->length += length;

        /* Do not need to allocate new event */
        flag_allocated = 1;
    } else if (CHECK_IO_DEPENDENCY_FOR_WRITE(s, ssd->e_queue->tail, sector_nb, 
                length) == SUCCESS) {

        /* Calculate Overlapped length */
        invalid_len = ssd->e_queue->tail->sector_num + ssd->e_queue->tail->length
                      - sector_nb;

        /* Invalidate the corresponding write buffer frame */
        UPDATE_WB_VALID_ARRAY_PARTIAL(s, ssd->e_queue->tail, 'I', invalid_len, 1);

        /* Update the last write event */
        ssd->e_queue->tail->length += (length - invalid_len);			

        /* Do not need to allocate new event */
        flag_allocated = 1;
    }

    /* If need to allocate new event */
    if (flag_allocated == 0) {
        /* Allocate new event at the tail of the event queue */
        new_e_q_entry = ALLOC_NEW_EVENT(WRITE, sector_nb, length, p_buf);	

        /* Add New IO event entry to event queue */
        if (ssd->e_queue->entry_nb == 0) {
            ssd->e_queue->head = new_e_q_entry;
            ssd->e_queue->tail = new_e_q_entry;
        } else {
            ssd->e_queue->tail->next = new_e_q_entry;
            ssd->e_queue->tail = new_e_q_entry;
        }
        ssd->e_queue->entry_nb++;
    }

#ifdef FIRM_IO_BUF_DEBUG
    printf("[%s, %s] End.\n", ssd->name, __func__);
#endif
}

event_queue_entry *ALLOC_NEW_EVENT(int io_type, int32_t sector_num, 
                                   unsigned int length, void *buf)
{
#ifdef FIRM_IO_BUF_DEBUG
	printf("[%s] Start.\n", __func__);
#endif

    event_queue_entry *new_e_q_entry = qemu_mallocz(sizeof(event_queue_entry));

	new_e_q_entry->io_type = io_type;
	new_e_q_entry->valid = VALID;
	new_e_q_entry->sector_num = sector_num;
	new_e_q_entry->length = length;
    new_e_q_entry->has_error = 0;
	new_e_q_entry->buf = buf;
	new_e_q_entry->next = NULL;

#ifdef FIRM_IO_BUF_DEBUG
	printf("[%s] End.\n",  __func__);
#endif
	return new_e_q_entry;
}

void WRITE_DATA_TO_BUFFER(IDEState *s, unsigned int length)
{
#ifdef FIRM_IO_BUF_DEBUG
	printf("[%s] Start.\n", __func__);
#endif

	/* Write Data to Write Buffer Frame */
	INCREASE_WB_SATA_POINTER(s, length);

#ifdef FIRM_IO_BUF_DEBUG
	printf("[%s] End.\n", __func__);
#endif
}

void READ_DATA_FROM_BUFFER_TO_HOST(IDEState *s, event_queue_entry *c_e_q_entry)
{
    SSDState *ssd = &(s->ssd);
    //SSDConf *ssdconf = &(ssd->param);

#ifdef FIRM_IO_BUF_DEBUG
	printf("[%s, %s] Start.\n", ssd->name, __func__);
#endif
	if (ssd->sata_read_ptr != c_e_q_entry->buf) {
		printf("ERROR[%s, %s] sata pointer is different from entry pointer.\n", 
                ssd->name, __func__);
	}

	/* Read the buffer data and increase SATA pointer */
	INCREASE_RB_SATA_POINTER(s, c_e_q_entry->length);

#ifdef FIRM_IO_BUF_DEBUG
	printf("[%s] End.\n", __func__);
#endif
}

void COPY_DATA_TO_READ_BUFFER(IDEState *s, event_queue_entry *dst_entry, 
                              event_queue_entry *src_entry)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

#ifdef FIRM_IO_BUF_DEBUG
	printf("[%s] Start.\n", __func__);
#endif
	if (dst_entry == NULL || src_entry == NULL) {
		printf("[%s] Null pointer error.\n", __func__);
		return;
	}

	int count = 0;
	int offset;
	void *dst_buf;	// new read entry
	void *src_buf;  // write entry

	int32_t dst_sector_nb = dst_entry->sector_num;
	int32_t src_sector_nb = src_entry->sector_num;
	unsigned int dst_length = dst_entry->length;

	/* Update read entry buffer pointer */	
	dst_buf = dst_entry->buf; 

	/* Calculate write buffer frame address */
	src_buf = src_entry->buf;
	offset = dst_sector_nb - src_sector_nb;


	while (count != offset) {

		if (GET_WB_VALID_ARRAY_ENTRY(s, src_buf)!='I') {
			count++;
		}

		src_buf = src_buf + ssdconf->sector_size;
		if (src_buf == ssd->write_buffer_end) {
			src_buf = ssd->write_buffer;
		}
	}

	count = 0;
	while (count != dst_length) {
		if (GET_WB_VALID_ARRAY_ENTRY(s, src_buf) == 'I') {
			src_buf = src_buf + ssdconf->sector_size;
			if (src_buf == ssd->write_buffer_end) {
				src_buf = ssd->write_buffer;
			}
			continue;
		}

		/* Copy Write Buffer Data to Read Buffer */
		memcpy(dst_buf, src_buf, ssdconf->sector_size);

		/* Increase offset */
		dst_buf = dst_buf + ssdconf->sector_size;
		src_buf = src_buf + ssdconf->sector_size;

		ssd->ftl_read_ptr = ssd->ftl_read_ptr + ssdconf->sector_size;

		if (dst_buf == ssd->read_buffer_end) {
			dst_buf = ssd->read_buffer;
			ssd->ftl_read_ptr = ssd->read_buffer;
		}
		if (src_buf == ssd->write_buffer_end) {
			src_buf = ssd->write_buffer;
		}
		count++;
	}

	INCREASE_RB_LIMIT_POINTER(s);

#ifdef FIRM_IO_BUF_DEBUG
	printf("[%s] End.\n", __func__);
#endif

}

/*
 * Coperd: flush the events [head, e_q_entry]
 */
void FLUSH_EVENT_QUEUE_UNTIL(IDEState *s, event_queue_entry *e_q_entry)
{
    SSDState *ssd = &(s->ssd);

#ifdef FIRM_IO_BUF_DEBUG
	printf("[%s, %s] Start.\n", ssd->name, __func__);
#endif
	int i;
	int count = 1;
	event_queue_entry *temp_e_q_entry = ssd->e_queue->head;

    assert(e_q_entry && temp_e_q_entry);
	
	if (e_q_entry == NULL || temp_e_q_entry == NULL) {
		printf("ERROR[%s, %s] Invalid event pointer\n", ssd->name, __func__);
		return;
	}

	/* Count how many event should be flushed */
	if (e_q_entry == ssd->e_queue->tail) {
		count = ssd->e_queue->entry_nb;
	} else {
		while (temp_e_q_entry != e_q_entry) {
			count++;
			temp_e_q_entry = temp_e_q_entry->next;
		}
	}

	/* Dequeue event */
	for (i = 0; i < count; i++) {
		DEQUEUE_IO(s);
	}

#ifdef FIRM_IO_BUF_DEBUG
	printf("[%s, %s] End.\n", ssd->name, __func__);
#endif
}

int CHECK_OVERWRITE(IDEState *s, event_queue_entry *e_q_entry, 
                    int32_t sector_nb, unsigned int length)
{
	int ret = FAIL;
#ifdef FIRM_IO_BUF_DEBUG
	printf("[%s, %s] Start.\n", ssd->name, __func__);
#endif
	int32_t temp_sector_nb = e_q_entry->sector_num;
	unsigned int temp_length = e_q_entry->length;

	if (e_q_entry->io_type == WRITE) {
		if (sector_nb <= temp_sector_nb && \
			(sector_nb + length) >= (temp_sector_nb + temp_length)) {
				
			ret = SUCCESS;
		}
	}
#ifdef FIRM_IO_BUF_DEBUG
	printf("[%s, %s] End.\n", ssd->name, __func__);
#endif

    return ret;
}

int CHECK_SEQUENTIALITY(event_queue_entry *e_q_entry, int32_t sector_num)
{
	int ret = FAIL;
#ifdef FIRM_IO_BUF_DEBUG
	printf("[%s] Start.\n", __func__);
#endif
	if (e_q_entry == NULL) {
		return ret;
	}

	int32_t temp_sector_nb = e_q_entry->sector_num;
	unsigned int temp_length = e_q_entry->length;

	if ((e_q_entry->io_type == WRITE) && \
			(e_q_entry->valid == VALID) && \
			(temp_sector_nb + temp_length == sector_num)) {
		ret = SUCCESS;
	}

#ifdef FIRM_IO_BUF_DEBUG
	printf("[%s] End.\n", __func__);
#endif

    return ret;
}

/* Coperd: still don't fully understand this part */
event_queue_entry *CHECK_IO_DEPENDENCY_FOR_READ(IDEState *s, int32_t sector_num, 
                                                unsigned int length)
{
    SSDState *ssd = &(s->ssd);
    //SSDConf *ssdconf = &(ssd->param);

#ifdef FIRM_IO_BUF_DEBUG
	printf("[%s, %s] Start.\n", ssd->name, __func__);
#endif
	int32_t last_sector_num = sector_num + length - 1;
	int32_t temp_sector_num;
	int32_t temp_last_sector_num;

	event_queue_entry *ret_e_q_entry = NULL;
	event_queue_entry *e_q_entry = NULL;

	if (ssd->last_read_entry == NULL) {
		e_q_entry = ssd->e_queue->head;
	} else {
		e_q_entry = ssd->last_read_entry->next;
	}

	while (e_q_entry != NULL) {
		if (e_q_entry->valid == VALID) {
			temp_sector_num = e_q_entry->sector_num;
			temp_last_sector_num = temp_sector_num + e_q_entry->length - 1; 

			/* Find the last IO event which has dependency */		
            /* Coperd: the two I/O read requests have some overlapping */
			if (temp_sector_num <= last_sector_num && \
				sector_num <= temp_last_sector_num) {
				
				ret_e_q_entry = e_q_entry;
			}
		}
		e_q_entry = e_q_entry->next;
	}

#ifdef FIRM_IO_BUF_DEBUG
	printf("[%s, %s] End.\n", ssd->name, __func__);
#endif
	return ret_e_q_entry;
}

int CHECK_IO_DEPENDENCY_FOR_WRITE(IDEState *s, event_queue_entry *e_q_entry, 
                                  int32_t sector_nb, unsigned int length)
{
    int ret = FAIL;
#ifdef FIRM_IO_BUF_DEBUG
	printf("[%s, %s] Start.\n", ssd->name, __func__);
#endif
	if (e_q_entry == NULL) {
		return ret;
	}

	int32_t last_sector_nb = sector_nb + length - 1;
	int32_t temp_sector_nb = e_q_entry->sector_num;
	int32_t temp_last_sector_nb = temp_sector_nb + e_q_entry->length - 1;

	if (e_q_entry->io_type == WRITE && e_q_entry->valid == VALID) {

		/* Find the last IO event which has dependency */		
		if (temp_sector_nb < sector_nb && \
			sector_nb < temp_last_sector_nb && \
			temp_last_sector_nb < last_sector_nb ) {
			
			ret = SUCCESS;
		}
	}

#ifdef FIRM_IO_BUF_DEBUG
	printf("[%s, %s] End.\n", ssd->name, __func__);
#endif

    return ret;
}

int EVENT_QUEUE_IS_FULL(IDEState *s, int io_type, unsigned int length)
{
    SSDState *ssd = &(s->ssd);
	int ret = FAIL;

	if (io_type == WRITE) {	
		if (ssd->empty_write_buffer_frame < length)
			ret = SUCCESS;
	} else if (io_type == READ) {
		if (ssd->empty_read_buffer_frame < length)
			ret = SUCCESS;
	}

	return ret;
}


void SECURE_WRITE_BUFFER(IDEState *s)
{
    SSDState *ssd = &(s->ssd);
	FLUSH_EVENT_QUEUE_UNTIL(s, ssd->e_queue->tail);
}

void SECURE_READ_BUFFER(IDEState *s)
{
    SSDState *ssd = &(s->ssd);

	if (ssd->c_e_queue->entry_nb != 0) {
		DEQUEUE_COMPLETED_READ(s);
	}

	if (ssd->last_read_entry != NULL) {
		FLUSH_EVENT_QUEUE_UNTIL(s, ssd->last_read_entry);
		DEQUEUE_COMPLETED_READ(s);
	}
}

char GET_WB_VALID_ARRAY_ENTRY(IDEState *s, void *buffer_pointer)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

	/* Calculate index of write buffer valid array */
	int index = (int)(buffer_pointer - ssd->write_buffer)/ssdconf->sector_size;

#ifdef FIRM_IO_BUF_DEBUG
	printf("[%s, %s] index: %d\n", ssd->name, __func__, index);
#endif
	
	/* Update write buffer valid array */
	return ssd->wb_valid_array[index];
}

void UPDATE_WB_VALID_ARRAY(IDEState *s, event_queue_entry *e_q_entry, 
                           char new_value)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

#ifdef FIRM_IO_BUF_DEBUG
	printf("[%s, %s] Start. \n", ssd->name, __func__);
#endif
	void *p_buf = e_q_entry->buf;
	if (p_buf == NULL) {
		printf("ERROR[%s, %s] Null pointer!\n", ssd->name, __func__);
		return;
	}

	int index = (int)(p_buf - ssd->write_buffer)/ssdconf->sector_size;
	int count = 0;
	int length = e_q_entry->length;

	while (count != length) {
		if (GET_WB_VALID_ARRAY_ENTRY(s, p_buf) == 'V') {
			ssd->wb_valid_array[index] = new_value;	
			count++;
		}

		/* Increase index and buffer pointer */
		p_buf = p_buf + ssdconf->sector_size;
		index++;
		if (index == ssdconf->write_buffer_frame_nb) {
			p_buf = ssd->write_buffer;
			index = 0;
		} 
	}

#ifdef FIRM_IO_BUF_DEBUG
	printf("[%s, %s] End. \n", ssd->name, __func__);
#endif
}

void UPDATE_WB_VALID_ARRAY_ENTRY(IDEState *s, void *buf, char new_value)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

	/* Calculate index of write buffer valid array */
	int index = (int)(buf - ssd->write_buffer)/ssdconf->sector_size;
	if (index >= ssdconf->write_buffer_frame_nb) {
		printf("ERROR[%s, %s] Invlald index. \n", ssd->name, __func__);
		return;
	}
	
	/* Update write buffer valid array */
	ssd->wb_valid_array[index] = new_value;
}

void UPDATE_WB_VALID_ARRAY_PARTIAL(IDEState *s, event_queue_entry *e_q_entry, 
                                   char new_value, int length, int mode)
{
	// mode 0: change valid value of the front array
	// mode 1: change valid value of the rear array
    
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

	int count = 0;
	int offset = 0;
	void *p_buf = e_q_entry->buf;

	if (mode == 1) {
		offset = e_q_entry->length - length;

		while (count != offset) {
			if (GET_WB_VALID_ARRAY_ENTRY(s, p_buf) != 'I') {
				count++;
			}
			p_buf = p_buf + ssdconf->sector_size;
			if (p_buf == ssd->write_buffer_end) {
				p_buf = ssd->write_buffer;
			}
		}
	}

	count = 0;
	while (count != length) {
		if (GET_WB_VALID_ARRAY_ENTRY(s, p_buf) != 'I') {
			UPDATE_WB_VALID_ARRAY_ENTRY(s, p_buf, new_value);
			count++;
		}

		/* Increase index and buffer pointer */
		p_buf = p_buf + ssdconf->sector_size;
		if (p_buf == ssd->write_buffer_end) {
			p_buf = ssd->write_buffer;
		}
	}
}

void INCREASE_WB_SATA_POINTER(IDEState *s, int entry_nb)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);
	int i;

#ifdef FIRM_IO_BUF_DEBUG
	int index = (int)(ssd->sata_write_ptr - ssd->write_buffer)/ssdconf->sector_size;
	printf("[%s, %s] Start: %d -> ", ssd->name, __func__, index);
#endif
	for (i = 0; i < entry_nb; i++) {
		/* Decrease the # of empty write buffer frame */
		ssd->empty_write_buffer_frame--;

		/* Update write buffer valid array */
		UPDATE_WB_VALID_ARRAY_ENTRY(s, ssd->sata_write_ptr, 'V');

		/* Increase sata write pointer */
		ssd->sata_write_ptr = ssd->sata_write_ptr + ssdconf->sector_size;
		
		if (ssd->sata_write_ptr == ssd->write_buffer_end) {
			ssd->sata_write_ptr = ssd->write_buffer;
		}
	}
#ifdef FIRM_IO_BUF_DEBUG
	index = (int)(ssd->sata_write_ptr - ssd->write_buffer)/ssdconf->sector_size;
	printf("%d End.\n", index);
#endif
}

void INCREASE_RB_SATA_POINTER(IDEState *s, int entry_nb)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);
	int i;

#ifdef FIRM_IO_BUF_DEBUG
	int index = (int)(ssd->sata_read_ptr - ssd->read_buffer)/ssdconf->sector_size;
	printf("[%s, %s] Start: %d -> ", ssd->name, __func__, index);
#endif

	for (i = 0; i < entry_nb; i++) {
		ssd->empty_read_buffer_frame++;

		ssd->sata_read_ptr = ssd->sata_read_ptr + ssdconf->sector_size;

		if (ssd->sata_read_ptr == ssd->read_buffer_end) {
			ssd->sata_read_ptr = ssd->read_buffer;
		}
	}
#ifdef FIRM_IO_BUF_DEBUG
	index = (int)(ssd->sata_read_ptr - ssd->read_buffer)/ssdconf->sector_size;
	printf("%s, %d End.\n", ssd->name, index);
#endif
}

void INCREASE_WB_FTL_POINTER(IDEState *s, int entry_nb)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

#ifdef FIRM_IO_BUF_DEBUG
	int index = (int)(ssd->ftl_write_ptr - ssd->write_buffer)/ssdconf->sector_size;
	printf("[%s] Start: %d -> ", __func__, index);
#endif
	int count = 0;
	char validity;

	while (count != entry_nb) {
		/* Get write buffer frame status */
		validity = GET_WB_VALID_ARRAY_ENTRY(s, ssd->ftl_write_ptr);

		if (validity == 'V') {
			/* Update write buffer valid array */
			UPDATE_WB_VALID_ARRAY_ENTRY(s, ssd->ftl_write_ptr, 'F');

			count++;
		}

		/* Increase ftl pointer by ssdconf->sector_size */
		ssd->ftl_write_ptr = ssd->ftl_write_ptr + ssdconf->sector_size;
		if (ssd->ftl_write_ptr == ssd->write_buffer_end) {
			ssd->ftl_write_ptr = ssd->write_buffer;
		}
	}
#ifdef FIRM_IO_BUF_DEBUG
	index = (int)(ssd->ftl_write_ptr - ssd->write_buffer)/ssdconf->sector_size;
	printf("%d End.\n",index);
#endif
}

void INCREASE_RB_FTL_POINTER(IDEState *s, int entry_nb)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

#ifdef FIRM_IO_BUF_DEBUG
	int index = (int)(ssd->ftl_read_ptr - ssd->read_buffer)/ssdconf->sector_size;
	printf("[%s, %s] Start: %d -> ", ssd->name, __func__, index);
#endif
	int i;

	for (i = 0; i < entry_nb; i++) {

		/* Increase ftl read pointer by ssdconf->sector_size */
		ssd->ftl_read_ptr = ssd->ftl_read_ptr + ssdconf->sector_size;
		if (ssd->ftl_read_ptr == ssd->read_buffer_end) {
			ssd->ftl_read_ptr = ssd->read_buffer;
		}
	}
#ifdef FIRM_IO_BUF_DEBUG
	index = (int)(ssd->ftl_read_ptr - ssd->read_buffer)/ssdconf->sector_size;
	printf("%d End.\n", index);
#endif
}

void DECREASE_RB_FTL_POINTER(IDEState *s, int entry_nb)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

#ifdef FIRM_IO_BUF_DEBUG
	int index = (int)(ssd->ftl_read_ptr - ssd->read_buffer)/ssdconf->sector_size;
	printf("[%s, %s] Start: %d -> ", ssd->name, __func__, index);
#endif
	int i;

	for (i = 0; i < entry_nb; i++) {

		/* Increase ftl read pointer by ssdconf->sector_size */
		ssd->ftl_read_ptr = ssd->ftl_read_ptr - ssdconf->sector_size;
		if (ssd->ftl_read_ptr == ssd->read_buffer) {
			ssd->ftl_read_ptr = ssd->read_buffer_end;
		}
	}
#ifdef FIRM_IO_BUF_DEBUG
	index = (int)(ssd->ftl_read_ptr - ssd->read_buffer)/ssdconf->sector_size;
	printf("%d End.\n", index);
#endif
}

void INCREASE_WB_LIMIT_POINTER(IDEState *s)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

#ifdef FIRM_IO_BUF_DEBUG
	int index = (int)(ssd->write_limit_ptr - ssd->write_buffer)/ssdconf->sector_size;
	printf("[%s] Start: %d -> ", __func__, index);
#endif
	/* Increase write limit pointer until ftl write pointer */
	do {
		/* Update write buffer valid array */
		UPDATE_WB_VALID_ARRAY_ENTRY(s, ssd->write_limit_ptr, '0');

		/* Incrase write limit pointer by ssdconf->sector_size */
        ssd->write_limit_ptr = ssd->write_limit_ptr + ssdconf->sector_size;
		if (ssd->write_limit_ptr == ssd->write_buffer_end) {
			ssd->write_limit_ptr = ssd->write_buffer;
		}

		ssd->empty_write_buffer_frame++;
	
	} while (ssd->write_limit_ptr != ssd->ftl_write_ptr);

#ifdef FIRM_IO_BUF_DEBUG
	index = (int)(ssd->write_limit_ptr - ssd->write_buffer)/ssdconf->sector_size;
	printf("%d. End.\n", index);
#endif
}

void INCREASE_RB_LIMIT_POINTER(IDEState *s)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

#ifdef FIRM_IO_BUF_DEBUG
	int index = (int)(ssd->read_limit_ptr - ssd->read_buffer)/ssdconf->sector_size;
	printf("[%s, %s] Start: %d -> ", ssd->name, __func__, index);
#endif
	/* Increase read limit pointer until ftl read pointer */
	do {

		/* Increase read lmit pointer by ssdconf->sector_size */
		ssd->read_limit_ptr = ssd->read_limit_ptr + ssdconf->sector_size;
		if (ssd->read_limit_ptr == ssd->read_buffer_end) {
			ssd->read_limit_ptr = ssd->read_buffer;
		}
	} while (ssd->read_limit_ptr != ssd->ftl_read_ptr);
#ifdef FIRM_IO_BUF_DEBUG
	index = (int)(ssd->read_limit_ptr - ssd->read_buffer)/ssdconf->sector_size;
	printf("%d End.\n", index);
#endif
}

int COUNT_READ_EVENT(IDEState *s)
{
    SSDState *ssd = &(s->ssd);

	int count = 1;
	event_queue_entry *e_q_entry = NULL;

	if (ssd->last_read_entry == NULL) {
		return 0;
	} else {
		e_q_entry = ssd->e_queue->head;
		while (e_q_entry != ssd->last_read_entry) {
			count++;

			e_q_entry = e_q_entry->next;
		}
	}
	return count;
}
