// File: ftl_perf_manager.h
// Date: 2014. 12. 03.
// Author: Jinsoo Yoo (jedisty@hanyang.ac.kr)
// Copyright(c)2014
// Hanyang University, Seoul, Korea
// Embedded Software Systems Laboratory. All right reserved

#ifndef _PERF_MANAGER_H_
#define _PERF_MANAGER_H_

#include "ide.h"

/* IO Latency */


/* IO Latency */
//extern unsigned int io_request_nb;
//extern unsigned int io_request_seq_nb;

//extern struct io_request* io_request_start;
//extern struct io_request* io_request_end;

/* GC Latency */
//extern unsigned int gc_request_nb;
//extern unsigned int gc_request_seq_nb;

//extern struct io_request* gc_request_start;
//extern struct io_request* gc_request_end;

//extern int64_t written_page_nb;

double GET_IO_BANDWIDTH(IDEState *s, double delay);

void INIT_PERF_CHECKER(IDEState *s);
void TERM_PERF_CHECKER(IDEState *s);

void SEND_TO_PERF_CHECKER(IDEState *s, int op_type, int64_t op_delay, int type);

int64_t ALLOC_IO_REQUEST(IDEState *s, int32_t sector_nb, unsigned int length, int io_type, int* page_nb);
void FREE_DUMMY_IO_REQUEST(IDEState *s, int type);
void FREE_IO_REQUEST(IDEState *s, io_request* request);
int64_t UPDATE_IO_REQUEST(IDEState *s, int request_nb, int offset, int64_t time, int type);
void INCREASE_IO_REQUEST_SEQ_NB(IDEState *s);
io_request* LOOKUP_IO_REQUEST(IDEState *s, int request_nb, int type);
int64_t CALC_IO_LATENCY(IDEState *s, io_request* request);
void PRINT_IO_REQUEST(IDEState *s, io_request* request);
void PRINT_ALL_IO_REQUEST(IDEState *s);

#endif
