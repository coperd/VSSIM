// File: ftl_perf_manager.c
// Date: 2014. 12. 03.
// Author: Jinsoo Yoo (jedisty@hanyang.ac.kr)
// Copyright(c)2014
// Hanyang University, Seoul, Korea
// Embedded Software Systems Laboratory. All right reserved

#include "ftl_perf_manager.h"
#include "ssd_io_manager.h"
#include <assert.h>

/* Average IO Time */
//double avg_write_delay;
//double total_write_count;
//double total_write_delay;
//
//double avg_read_delay;
//double total_read_count;
//double total_read_delay;
//
//double avg_gc_write_delay;
//double total_gc_write_count;
//double total_gc_write_delay;
//
//double avg_gc_read_delay;
//double total_gc_read_count;
//double total_gc_read_delay;
//
//double avg_seq_write_delay;
//double total_seq_write_count;
//double total_seq_write_delay;
//
//double avg_ran_write_delay;
//double total_ran_write_count;
//double total_ran_write_delay;
//
//double avg_ran_cold_write_delay;
//double total_ran_cold_write_count;
//double total_ran_cold_write_delay;
//
//double avg_ran_hot_write_delay;
//double total_ran_hot_write_count;
//double total_ran_hot_write_delay;
//
//double avg_seq_merge_read_delay;
//double total_seq_merge_read_count;
//double total_seq_merge_read_delay;
//
//double avg_ran_merge_read_delay;
//double total_ran_merge_read_count;
//double total_ran_merge_read_delay;
//
//double avg_seq_merge_write_delay;
//double total_seq_merge_write_count;
//double total_seq_merge_write_delay;
//
//double avg_ran_merge_write_delay;
//double total_ran_merge_write_count;
//double total_ran_merge_write_delay;
//
//double avg_ran_cold_merge_write_delay;
//double total_ran_cold_merge_write_count;
//double total_ran_cold_merge_write_delay;
//
//double avg_ran_hot_merge_write_delay;
//double total_ran_hot_merge_write_count;
//double total_ran_hot_merge_write_delay;

/* IO Latency */
//unsigned int io_request_nb;                /* Coperd: total number of io_request */
//unsigned int io_request_seq_nb;            /* Coperd: current io request number ??? */

/* Coperd: in FTL_READ/WRITE, each uplevel request will be allocated an io_request structure, denoting an io request, VSSIM maintains a list of these io requests in a single linked list, starting from io_request_start, ending with io_request_end */
//struct io_request *io_request_start;
//struct io_request *io_request_end;

/* Calculate IO Latency */
//double read_latency_count;
//double write_latency_count;

//double avg_read_latency;
//double avg_write_latency;

/* SSD Util */
//double ssd_util;
//int64_t written_page_nb;

//extern double ssd_util;

#ifdef PERF_DEBUG1
FILE *fp_perf1_w;
#endif
#ifdef PERF_DEBUG2
FILE *fp_perf2_r;
FILE *fp_perf2_w;
#endif
#ifdef PERF_DEBUG3
FILE *fp_perf3_up;
FILE *fp_perf3_al;
#endif

void INIT_PERF_CHECKER(IDEState *s)
{
    SSDState *ssd = &(s->ssd);
    //SSDConf *ssdconf = &(ssd->param);
    SSDPerf *ssdperf = &(ssd->perf);

	/* Average IO Time */
	ssdperf->avg_write_delay = 0;
	ssdperf->total_write_count = 0;
	ssdperf->total_write_delay = 0;

	ssdperf->avg_read_delay = 0;
	ssdperf->total_read_count = 0;
	ssdperf->total_read_delay = 0;

	ssdperf->avg_gc_write_delay = 0;
	ssdperf->total_gc_write_count = 0;
	ssdperf->total_gc_write_delay = 0;

	ssdperf->avg_gc_read_delay = 0;
	ssdperf->total_gc_read_count = 0;
	ssdperf->total_gc_read_delay = 0;

	ssdperf->avg_seq_write_delay = 0;
	ssdperf->total_seq_write_count = 0;
	ssdperf->total_seq_write_delay = 0;

	ssdperf->avg_ran_write_delay = 0;
	ssdperf->total_ran_write_count = 0;
	ssdperf->total_ran_write_delay = 0;

	ssdperf->avg_ran_cold_write_delay = 0;
	ssdperf->total_ran_cold_write_count = 0;
	ssdperf->total_ran_cold_write_delay = 0;

	ssdperf->avg_ran_hot_write_delay = 0;
	ssdperf->total_ran_hot_write_count = 0;
	ssdperf->total_ran_hot_write_delay = 0;

	ssdperf->avg_seq_merge_read_delay = 0;
	ssdperf->total_seq_merge_read_count = 0;
	ssdperf->total_seq_merge_read_delay = 0;

	ssdperf->avg_ran_merge_read_delay = 0;
	ssdperf->total_ran_merge_read_count = 0;
	ssdperf->total_ran_merge_read_delay = 0;

	ssdperf->avg_seq_merge_write_delay = 0;
	ssdperf->total_seq_merge_write_count = 0;
	ssdperf->total_seq_merge_write_delay = 0;

	ssdperf->avg_ran_merge_write_delay = 0;
	ssdperf->total_ran_merge_write_count = 0;
	ssdperf->total_ran_merge_write_delay = 0;

	ssdperf->avg_ran_cold_merge_write_delay = 0;
	ssdperf->total_ran_cold_merge_write_count = 0;
	ssdperf->total_ran_cold_merge_write_delay = 0;

	ssdperf->avg_ran_hot_merge_write_delay = 0;
	ssdperf->total_ran_hot_merge_write_count = 0;
	ssdperf->total_ran_hot_merge_write_delay = 0;

	/* IO Latency */
	ssd->io_request_nb = 0;
	ssd->io_request_seq_nb = 0;

	ssd->io_request_start = NULL;
	ssd->io_request_end = NULL;

	ssdperf->read_latency_count = 0;
	ssdperf->write_latency_count = 0;

	ssdperf->avg_read_latency = 0;
	ssdperf->avg_write_latency = 0;

	ssd->ssd_util = 0;
	ssdperf->written_page_nb = 0;

#ifdef PERF_DEBUG1
	fp_perf1_w = fopen("./data/p_perf1_w.txt","a");
#endif
#ifdef PERF_DEBUG2
	fp_perf2_r = fopen("./data/p_perf2_r.txt","a");
	fp_perf2_w = fopen("./data/p_perf2_w.txt","a");
#endif
#ifdef PERF_DEBUG3
	fp_perf3_al = fopen("./data/p_perf3_al.txt","a");
	fp_perf3_up = fopen("./data/p_perf3_up.txt","a");
#endif

}

void TERM_PERF_CHECKER(IDEState *s)
{
    SSDState *ssd = &(s->ssd);
    SSDPerf *ssdperf = &(ssd->perf);

    printf("Average Read Latency	%.3lf us\n", ssdperf->avg_read_latency);
    printf("Average Write Latency	%.3lf us\n", ssdperf->avg_write_latency);

#ifdef PERF_DEBUG1
    fclose(fp_perf1_w);
#endif
#ifdef PERF_DEBUG2
    fclose(fp_perf2_w);
#endif
#ifdef PERF_DEBUG3
    fclose(fp_perf3_al);
    fclose(fp_perf3_up);
#endif
#ifdef PERF_DEBUG4
    printf("Total Seq Write 		%lf\n", ssdperf->total_seq_write_delay);
    printf("Total Ran Write 		%lf\n", ssdperf->total_ran_write_delay);
    printf("Total Ran Cold Write		%lf\n", ssdperf->total_ran_cold_write_delay);
    printf("Total Ran Hot Write 		%lf\n", ssdperf->total_ran_hot_write_delay);
    printf("Total Seq Merge Write 		%lf\n", ssdperf->total_seq_merge_write_delay);
    printf("Total Ran Merge Write 		%lf\n", ssdperf->total_ran_merge_write_delay);
    printf("Total Ran Cold Merge Write	%lf\n", ssdperf->total_ran_cold_merge_write_delay);
    printf("Total Ran Hot Merge Write 	%lf\n", ssdperf->total_ran_hot_merge_write_delay);
#endif
}

void SEND_TO_PERF_CHECKER(IDEState *s, int op_type, int64_t op_delay, int type)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);
    SSDPerf *ssdperf = &(ssd->perf);

    double delay = (double)op_delay;
#ifdef MONITOR_ON
    char szTemp[1024];
    char szUtil[1024];
#endif

    if (type == CH_OP) {
        switch (op_type) {
            case READ:
                ssdperf->total_read_delay += delay;
                ssdperf->total_read_count++;
                ssdperf->avg_read_delay = ssdperf->total_read_delay / ssdperf->total_read_count;
                break;

            case WRITE:
                ssdperf->total_write_delay += delay;
                break;

            case ERASE:
                break;

            case GC_READ:
                ssdperf->total_gc_read_delay += delay;
                break;

            case GC_WRITE:
                ssdperf->total_gc_write_delay += delay;
                break;

            case SEQ_WRITE:
                ssdperf->total_seq_write_delay += delay;
                break;

            case RAN_WRITE:
                ssdperf->total_ran_write_delay += delay;
                break;

            case RAN_COLD_WRITE:
                ssdperf->total_ran_cold_write_delay += delay;
                break;

            case RAN_HOT_WRITE:
                ssdperf->total_ran_hot_write_delay += delay;
                break;

            case SEQ_MERGE_READ:
                ssdperf->total_seq_merge_read_delay += delay;
                break;

            case RAN_MERGE_READ:
                ssdperf->total_ran_merge_read_delay += delay;
                break;

            case SEQ_MERGE_WRITE:
                ssdperf->total_seq_merge_write_delay += delay;
                break;

            case RAN_MERGE_WRITE:
                ssdperf->total_ran_merge_write_delay += delay;
                break;

            case RAN_COLD_MERGE_WRITE:
                ssdperf->total_ran_cold_merge_write_delay += delay;
                break;

            case RAN_HOT_MERGE_WRITE:
                ssdperf->total_ran_hot_merge_write_delay += delay;
                break;

            case MAP_READ:
                break;

            case MAP_WRITE:
                break;

            default:
                break;
        }
    } else if (type == REG_OP) {
        switch (op_type) {
            case READ:
                ssdperf->total_read_delay += delay;
                break;

            case WRITE:
                ssdperf->total_write_delay += delay;
                ssdperf->total_write_count++;
                ssdperf->avg_write_delay = ssdperf->total_write_delay / ssdperf->total_write_count;

                /* Calc SSD Util */
                ssdperf->written_page_nb++;
#ifdef PERF_DEBUG1
                fprintf(fp_perf1_w, "%lf\t%lf\n", total_write_count, avg_write_delay);
#endif
                break;

            case ERASE:
                ssdperf->written_page_nb -= ssdconf->page_nb;
#ifdef MONITOR_ON
                sprintf(szTemp, "ERASE %d", 1);
                WRITE_LOG(szTemp);
#endif
                break;

            case GC_READ:
                ssdperf->total_gc_read_delay += delay;
                ssdperf->total_gc_read_count++;
                ssdperf->avg_gc_read_delay = ssdperf->total_gc_read_delay / ssdperf->total_gc_read_count;
                break;

            case GC_WRITE:
                ssdperf->total_gc_write_delay += delay;
                ssdperf->total_gc_write_count++;
                ssdperf->avg_gc_write_delay = ssdperf->total_gc_write_delay / ssdperf->total_gc_write_count;

                /* Calc SSD Util */
                ssdperf->written_page_nb++;
                break;

            case SEQ_WRITE:
                ssdperf->total_seq_write_delay += delay;
                ssdperf->total_seq_write_count++;
                ssdperf->avg_seq_write_delay = ssdperf->total_seq_write_delay / ssdperf->total_seq_write_count;

                /* Calc Monitor Write BW */
                ssdperf->total_write_delay += delay;
                ssdperf->total_write_count++;
                ssdperf->avg_write_delay = ssdperf->total_write_delay / ssdperf->total_write_count;

                /* Calc SSD Util */
                ssdperf->written_page_nb++;
                break;

            case RAN_WRITE:
                ssdperf->total_ran_write_delay += delay;
                ssdperf->total_ran_write_count++;
                ssdperf->avg_ran_write_delay = ssdperf->total_ran_write_delay / ssdperf->total_ran_write_count;

                /* Calc Monitor Write BW */
                ssdperf->total_write_delay += delay;
                ssdperf->total_write_count++;
                ssdperf->avg_write_delay = ssdperf->total_write_delay / ssdperf->total_write_count;

                /* Calc SSD Util */
                ssdperf->written_page_nb++;
                break;

            case RAN_COLD_WRITE:
                ssdperf->total_ran_cold_write_delay += delay;
                ssdperf->total_ran_cold_write_count++;
                ssdperf->avg_ran_cold_write_delay = ssdperf->total_ran_cold_write_delay / ssdperf->total_ran_cold_write_count;

                /* Calc Monitor Write BW */
                ssdperf->total_write_delay += delay;
                ssdperf->total_write_count++;
                ssdperf->avg_write_delay = ssdperf->total_write_delay / ssdperf->total_write_count;

                /* Calc SSD Util */
                ssdperf->written_page_nb++;
                break;

            case RAN_HOT_WRITE:
                ssdperf->total_ran_hot_write_delay += delay;
                ssdperf->total_ran_hot_write_count++;
                ssdperf->avg_ran_hot_write_delay = ssdperf->total_ran_hot_write_delay / ssdperf->total_ran_hot_write_count;

                /* Calc Monitor Write BW */
                ssdperf->total_write_delay += delay;
                ssdperf->total_write_count++;
                ssdperf->avg_write_delay = ssdperf->total_write_delay / ssdperf->total_write_count;

                /* Calc SSD Util */
                ssdperf->written_page_nb++;
                break;

            case SEQ_MERGE_READ:
                ssdperf->total_seq_merge_read_delay += delay;
                ssdperf->total_seq_merge_read_count++;
                ssdperf->avg_seq_merge_read_delay = ssdperf->total_seq_merge_read_delay / ssdperf->total_seq_merge_read_count;
                break;

            case RAN_MERGE_READ:
                ssdperf->total_ran_merge_read_delay += delay;
                ssdperf->total_ran_merge_read_count++;
                ssdperf->avg_ran_merge_read_delay = ssdperf->total_ran_merge_read_delay / ssdperf->total_ran_merge_read_count;
                break;

            case SEQ_MERGE_WRITE:
                ssdperf->total_seq_merge_write_delay += delay;
                ssdperf->total_seq_merge_write_count++;
                ssdperf->avg_seq_merge_write_delay = ssdperf->total_seq_merge_write_delay / ssdperf->total_seq_merge_write_count;

                /* Calc SSD Util */
                ssdperf->written_page_nb++;
                break;

            case RAN_MERGE_WRITE:
                ssdperf->total_ran_merge_write_delay += delay;
                ssdperf->total_ran_merge_write_count++;
                ssdperf->avg_ran_merge_write_delay = ssdperf->total_ran_merge_write_delay / ssdperf->total_ran_merge_write_count;

                /* Calc SSD Util */
                ssdperf->written_page_nb++;
                break;

            case RAN_COLD_MERGE_WRITE:
                ssdperf->total_ran_cold_merge_write_delay += delay;
                ssdperf->total_ran_cold_merge_write_count++;
                ssdperf->avg_ran_cold_merge_write_delay = ssdperf->total_ran_cold_merge_write_delay / ssdperf->total_ran_cold_merge_write_count;

                /* Calc SSD Util */
                ssdperf->written_page_nb++;
                break;

            case RAN_HOT_MERGE_WRITE:
                ssdperf->total_ran_hot_merge_write_delay += delay;
                ssdperf->total_ran_hot_merge_write_count++;
                ssdperf->avg_ran_hot_merge_write_delay = ssdperf->total_ran_hot_merge_write_delay / ssdperf->total_ran_hot_merge_write_count;

                /* Calc SSD Util */
                ssdperf->written_page_nb++;
                break;

            case MAP_READ:
                break;

            case MAP_WRITE:
                break;

            default:
                break;
        }
#if defined BLOCK_MAP || defined DA_MAP
        ssd->ssd_util = (double)(((double)ssdconf->BLOCK_MAPPING_ENTRY_NB - total_empty_block_nb)/BLOCK_MAPPING_ENTRY_NB)*100;
#else
        ssd->ssd_util = (double)((double)ssdperf->written_page_nb / ssdconf->pages_in_ssd)*100;
#endif
    } else if (type == LATENCY_OP) {
        switch (op_type){
            case READ:
#ifdef PERF_DEBUG2
                fprintf(fp_perf2_r, "%lf\t%lf\t%lf\n", ssdperf->read_latency_count, delay, ssd->ssd_util);
#endif
                ssdperf->avg_read_latency = (ssdperf->avg_read_latency * ssdperf->read_latency_count + delay)/(ssdperf->read_latency_count + 1);

                ssdperf->read_latency_count++;
#ifdef MONITOR_ON
                sprintf(szTemp, "READ BW %lf ", GET_IO_BANDWIDTH(avg_read_delay));
                WRITE_LOG(szTemp);
#endif
                break;
            case WRITE:
#ifdef PERF_DEBUG2
                fprintf(fp_perf2_w, "%lf\t%lf\t%lf\n", ssdperf->wwrite_latency_count, delay, ssd->ssd_util);
#endif

                ssdperf->avg_write_latency = (ssdperf->avg_write_latency * ssdperf->write_latency_count + delay)/(ssdperf->write_latency_count + 1);
                ssdperf->write_latency_count++;
#ifdef MONITOR_ON
                sprintf(szTemp, "WRITE BW %lf ", GET_IO_BANDWIDTH(avg_write_delay));
                WRITE_LOG(szTemp);

                sprintf(szUtil, "UTIL %lf ", ssd->ssd_util);
                WRITE_LOG(szUtil);
#endif
                break;
            default:
                break;
        }
    }
}

double GET_IO_BANDWIDTH(IDEState *s, double delay)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

	double bw;

	if (delay != 0)
		bw = ((double)ssdconf->page_size*1000000)/(delay*1024*1024);
	else
		bw = 0;

	return bw;

}

/* Coperd: (sector_nb, length) --> calculate the value of page_nb */
int64_t ALLOC_IO_REQUEST(IDEState *s, int32_t sector_nb, unsigned int length, int io_type, int* page_nb)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

	int64_t start = get_usec();
	int io_page_nb = 0;
	unsigned int remain = length;
	unsigned int left_skip = sector_nb % ssdconf->sectors_per_page;
	unsigned int right_skip;
	unsigned int sects;

	io_request *curr_io_request = (io_request *)calloc(1, sizeof(io_request));
	if (curr_io_request == NULL) {
		printf("ERROR[ALLOC_IO_REQUEST] Calloc io_request fail\n");
		return 0;
	}

	while (remain > 0) {
		if (remain > ssdconf->sectors_per_page - left_skip) {
			right_skip = 0;
		} else {
			right_skip = ssdconf->sectors_per_page - left_skip - remain;
		}
		sects = ssdconf->sectors_per_page - left_skip - right_skip;

		remain -= sects;
		left_skip = 0;
		io_page_nb++; /* Coperd: record the number of pages to read */
	}

    /* Coperd: finally page_nb is set to the value of total pages to read for this io request */
	*page_nb = io_page_nb; 

    /* Coperd: for each page read operation, maintain the time info */
	int64_t* start_time_arr = (int64_t*)calloc(io_page_nb, sizeof(int64_t));
	int64_t* end_time_arr = (int64_t*)calloc(io_page_nb, sizeof(int64_t));

	if (start_time_arr == NULL || end_time_arr == NULL) {
		printf("ERROR[ALLOC_IO_REQUEST] Calloc time array fail\n");
		return 0;
	} else {
		memset(start_time_arr, 0, io_page_nb);
		memset(end_time_arr, 0, io_page_nb);
	}

	curr_io_request->request_nb = ssd->io_request_seq_nb;
	
	curr_io_request->request_type = io_type;
	curr_io_request->request_size = io_page_nb;
	curr_io_request->start_count = 0;
	curr_io_request->end_count = 0;
	curr_io_request->start_time = start_time_arr;
	curr_io_request->end_time = end_time_arr;
	curr_io_request->next = NULL;

    /* Coperd: add this io_request to the request single linked list */
	if (ssd->io_request_start == NULL && ssd->io_request_nb == 0) {
		ssd->io_request_start = curr_io_request;
		ssd->io_request_end = curr_io_request;
	} else {
		ssd->io_request_end->next = curr_io_request;
		ssd->io_request_end = curr_io_request;
	}
	ssd->io_request_nb++; /* Coperd: now we have one more io request */
	
	int64_t end = get_usec();

#ifdef PERF_DEBUG3
	fprintf(fp_perf3_al,"%ld\n", end-start);
#endif
	return (end - start); /* Coperd: return the execution time of this function */
}

void FREE_DUMMY_IO_REQUEST(IDEState *s, int type)
{
    SSDState *ssd = &(s->ssd);
    //SSDConf *ssdconf = &(ssd->param);

    int i;
    int success = 0;
    io_request *prev_request = ssd->io_request_start;

    io_request *request = LOOKUP_IO_REQUEST(s, ssd->io_request_seq_nb, type);


    if (ssd->io_request_nb == 1) {
        ssd->io_request_start = NULL;
        ssd->io_request_end = NULL;
        success = 1;
    } else if (prev_request == request) {
        ssd->io_request_start = request->next;
        success = 1;
    } else {
        for (i = 0; i < (ssd->io_request_nb-1); i++) {
            if (prev_request->next == request && request == ssd->io_request_end) {
                prev_request->next = NULL;
                ssd->io_request_end = prev_request;
                success = 1;
                break;
            } else if (prev_request->next == request) {
                prev_request->next = request->next;
                success = 1;
                break;
            } else {
                prev_request = prev_request->next;
            }
        }
    }


    if (success == 0) {
        printf("ERROR[FREE_DUMMY_IO_REQUEST] There is no such io request\n");
        return;
    }

    free(request->start_time);
    free(request->end_time);
    free(request);

    ssd->io_request_nb--;
}

void FREE_IO_REQUEST(IDEState *s, io_request* request)
{
    SSDState *ssd = &(s->ssd);
    //SSDConf *ssdconf = &(ssd->param);

    int i;
    int success = 0;
    io_request* prev_request = ssd->io_request_start;

#ifdef FTL_PERF_DEBUG
    printf("\t\t[FREE_IO_REQUEST] %d\n", request->request_nb);
#endif	

    if (ssd->io_request_nb == 1) {
        ssd->io_request_start = NULL;
        ssd->io_request_end = NULL;
        success = 1;
    } else if (prev_request == request) {
        ssd->io_request_start = request->next;
        success = 1;
    } else {
        for (i = 0; i < (ssd->io_request_nb-1); i++) {
            if (prev_request->next == request && request == ssd->io_request_end) {
                prev_request->next = NULL;
                ssd->io_request_end = prev_request;
                success = 1;
                break;
            } else if (prev_request->next == request) {
                prev_request->next = request->next;
                success = 1;
                break;
            } else {
                prev_request = prev_request->next;
            }
        }
    }

    if (success == 0) {
        printf("ERROR[FREE_IO_REQUEST] There is no such io request\n");
        return;
    }

    free(request->start_time);
    free(request->end_time);
    free(request);

    ssd->io_request_nb--;
}


int64_t UPDATE_IO_REQUEST(IDEState *s, int request_nb, int offset, int64_t time, int type)
{
    int64_t start = get_usec();

    int io_type;
    int64_t latency=0;

    if (request_nb == -1)
        return 0;

    io_request *curr_request = LOOKUP_IO_REQUEST(s, request_nb, type);
    if (curr_request == NULL) {
        printf("ERROR[UPDATE_IO_REQUEST] No such io request, nb %d\n", request_nb);
        return 0;
    }

    if (type == UPDATE_START_TIME) {
        curr_request->start_time[offset] = time; /* Coperd: set to old_channel_time */
        curr_request->start_count++;             /* Coperd: one more NAND request has started execution */
    } else if (type == UPDATE_END_TIME) {
        curr_request->end_time[offset] = time;
        curr_request->end_count++;
    }

    /* Coperd: all NAND request have finished ==> the IO request is finished in the low level */
    if (curr_request->start_count == curr_request->request_size && curr_request->end_count == curr_request->request_size) {
        latency = CALC_IO_LATENCY(s, curr_request);
        io_type = curr_request->request_type;

        SEND_TO_PERF_CHECKER(s, io_type, latency, LATENCY_OP);

        FREE_IO_REQUEST(s, curr_request);
    }
    int64_t end = get_usec();

    return (end - start);
}

void INCREASE_IO_REQUEST_SEQ_NB(IDEState *s)
{
    SSDState *ssd = &(s->ssd);

    if (ssd->io_request_seq_nb == 0xffffffff) {
        ssd->io_request_seq_nb = 0;
    } else {
        ssd->io_request_seq_nb++;
    }
    //	printf("%d\n", ssd->io_request_nb);
}

/* Coperd: find the corresponding io_request to this NAND cmd */
io_request *LOOKUP_IO_REQUEST(IDEState *s, int request_nb, int type)
{
    SSDState *ssd = &(s->ssd);

    int i;
    int total_request=0;
    io_request *curr_request = NULL;

    //	PRINT_ALL_IO_REQUEST();

    if (ssd->io_request_start != NULL) {
        curr_request = ssd->io_request_start;
        total_request = ssd->io_request_nb;
    } else {
        printf("ERROR[LOOKUP_IO_REQUEST] There is no request\n");
        return NULL;
    }

    for (i = 0; i < total_request; i++) {
        if (curr_request->request_nb == request_nb) {
#ifdef FTL_PERF_DEBUG
            printf("[LOOKUP_IO_REQUEST] hit! %d %d\n", curr_request->request_nb, request_nb);
#endif
            return curr_request;
        }

        if (curr_request->next != NULL) {
            curr_request = curr_request->next;
        } else {
#ifdef FTL_PERF_DEBUG
            printf("[LOOKUP_IO_REQUEST] end\n");
#endif

            return NULL;
        }	
    }

    return NULL;
}

int64_t CALC_IO_LATENCY(IDEState *s, io_request *request)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    int64_t latency;
    int64_t *start_time_arr = request->start_time;
    int64_t *end_time_arr = request->end_time;

    int64_t min_start_time;
    int64_t max_end_time;

    int i;
    int type = request->request_type;
    int size = request->request_size;

    for (i = 0; i < size; i++) {
        if (end_time_arr[i] == 0) {
            if (type == READ) {
                end_time_arr[i] = start_time_arr[i] + ssdconf->reg_read_delay + ssdconf->cell_read_delay;
            } else if (type == WRITE) {
                end_time_arr[i] = start_time_arr[i] + ssdconf->reg_write_delay + ssdconf->cell_program_delay;
            }
        }
    }

    min_start_time = start_time_arr[0];
    max_end_time = end_time_arr[0];

    if (size > 1) {
        for (i = 1; i < size; i++) {
            if (min_start_time > start_time_arr[i]) {
                min_start_time = start_time_arr[i];
            }
            if (max_end_time < end_time_arr[i]) {
                max_end_time = end_time_arr[i];
            }
        }
    }

    assert(request->request_size > 0);
    latency = (max_end_time - min_start_time)/(request->request_size);

    return latency;
}

void PRINT_IO_REQUEST(IDEState *s, io_request *request)
{
    int i;

    printf("\t----- io request -----\n");
    printf("\trequeset_nb 	: %d\n", request->request_nb);
    printf("\trequest_type 	: ");
    if (request->request_type == READ)
        printf("READ\n");
    else if (request->request_type == WRITE)
        printf("WRITE\n");
    printf("\trequest_size	: %d\n", request->request_size);
    printf("\tstart_count	: %d\n", request->start_count);
    printf("\tend_count	: %d\n", request->end_count);
    printf("\tstart_time	: \n");
    for (i = 0; i < request->request_size; i++) {
        printf("\t[%d][%ld]\n", i, request->start_time[i]);
    }
    printf("\tend_time	: \n");
    for(i=0; i<request->request_size; i++){
        printf("\t[%d][%ld]\n", i, request->end_time[i]);
    }

    return;
}

void PRINT_ALL_IO_REQUEST(IDEState *s)
{
    SSDState *ssd = &(s->ssd);
    int i;

    io_request *curr_io_request = ssd->io_request_start;
    for (i = 0; i < ssd->io_request_nb; i++) {
        PRINT_IO_REQUEST(s, curr_io_request);
        curr_io_request = curr_io_request->next;
    }

    return;
}
