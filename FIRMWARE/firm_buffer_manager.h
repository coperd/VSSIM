// File: firm_buffer_manager.h
// Date: 2014. 12. 03.
// Author: Jinsoo Yoo (jedisty@hanyang.ac.kr)
// Copyright(c)2014
// Hanyang University, Seoul, Korea
// Embedded Software Systems Laboratory. All right reserved

#ifndef _SSD_BUFFER_MANAGER_H_
#define _SSD_BUFFER_MANAGER_H_

#include "ide.h"

void INIT_IO_BUFFER(IDEState *s);
void TERM_IO_BUFFER(IDEState *s);
void INIT_WB_VALID_ARRAY(IDEState *s);

void *SSD_THREAD_MAIN_LOOP(void *s);
void ENQUEUE_IO(IDEState *s, int io_type, int32_t sector_nb, unsigned int length);
void ENQUEUE_READ(IDEState *s, int32_t sector_nb, unsigned int length);
void ENQUEUE_WRITE(IDEState *s, int32_t sector_nb, unsigned int length);

int DEQUEUE_IO(IDEState *s);
void DEQUEUE_COMPLETED_READ(IDEState *s);

event_queue_entry *ALLOC_NEW_EVENT(int io_type, int32_t sector_nb, 
                                   unsigned int length, void *buf);

void WRITE_DATA_TO_BUFFER(IDEState *s, unsigned int length);
void READ_DATA_FROM_BUFFER_TO_HOST(IDEState *s, event_queue_entry *c_e_q_entry);

void COPY_DATA_TO_READ_BUFFER(IDEState *s, event_queue_entry *dst_entry, 
                              event_queue_entry *src_entry);

void FLUSH_EVENT_QUEUE_UNTIL(IDEState *s, event_queue_entry *e_q_entry);

int EVENT_QUEUE_IS_FULL(IDEState *s, int io_type, unsigned int length);
void SECURE_WRITE_BUFFER(IDEState *s);
void SECURE_READ_BUFFER(IDEState *s);

/* Check Event */
int CHECK_OVERWRITE(IDEState *s, event_queue_entry *e_q_entry, 
                    int32_t sector_nb, unsigned int length);

int CHECK_SEQUENTIALITY(event_queue_entry *e_q_entry, int32_t sector_nb);

event_queue_entry *CHECK_IO_DEPENDENCY_FOR_READ(IDEState *s, int32_t sector_nb, 
                                                unsigned int length);

int CHECK_IO_DEPENDENCY_FOR_WRITE(IDEState *s, event_queue_entry *e_q_entry, 
                                  int32_t sector_nb, unsigned int length);

/* Manipulate Write Buffer Valid Array */
char GET_WB_VALID_ARRAY_ENTRY(IDEState *s, void *buf);
void UPDATE_WB_VALID_ARRAY(IDEState *s, event_queue_entry *e_q_entry, char new);
void UPDATE_WB_VALID_ARRAY_ENTRY(IDEState *s, void *buffer_pointer, char new);

void UPDATE_WB_VALID_ARRAY_PARTIAL(IDEState *s, event_queue_entry *e_q_entry, 
                                   char new_value, int length, int mode);

/* Move Buffer Frame Pointer */
void INCREASE_WB_SATA_POINTER(IDEState *s, int entry_nb);
void INCREASE_RB_SATA_POINTER(IDEState *s, int entry_nb);
void INCREASE_WB_FTL_POINTER(IDEState *s, int entry_nb);
void INCREASE_RB_FTL_POINTER(IDEState *s, int entry_nb);
void DECREASE_RB_FTL_POINTER(IDEState *s, int entry_nb);
void INCREASE_WB_LIMIT_POINTER(IDEState *s);
void INCREASE_RB_LIMIT_POINTER(IDEState *s);

/* Test IO BUFFER */
int COUNT_READ_EVENT(IDEState *s);

#endif
