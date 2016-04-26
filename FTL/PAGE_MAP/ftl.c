// File: ftl.c
// Date: 2014. 12. 03.
// Author: Jinsoo Yoo (jedisty@hanyang.ac.kr)
// Copyright(c)2014
// Hanyang University, Seoul, Korea
// Embedded Software Systems Laboratory. All right reserved

#include "ftl.h"
#include "ftl_mapping_manager.h"
#include "vssim_config_manager.h"
#include "ftl_inverse_mapping_manager.h"
#include "ftl_perf_manager.h"
#include "ssd_io_manager.h"
#include "ftl_gc_manager.h"
#include "firm_buffer_manager.h"
#include "mytrace.h"

//#define DEBUG_LATENCY

#ifndef VSSIM_BENCH
#include "qemu-kvm.h"
#endif

void FTL_INIT(IDEState *s)
{
    SSDState *ssd = &(s->ssd);

    if (ssd->g_init == 0) {

        INIT_SSD_CONFIG(s);

        INIT_MAPPING_TABLE(s);            
        INIT_INVERSE_MAPPING_TABLE(s);    
        INIT_BLOCK_STATE_TABLE(s);        
        INIT_VALID_ARRAY(s);              
        INIT_EMPTY_BLOCK_LIST(s);         
        INIT_VICTIM_BLOCK_LIST(s);        
        INIT_PERF_CHECKER(s);

#ifdef FTL_MAP_CACHE
        INIT_CACHE(s);     /* Coperd: on-going */ 
#endif
#ifdef FIRM_IO_BUFFER
        INIT_IO_BUFFER(s);  
#endif
#ifdef MONITOR_ON
        INIT_LOG_MANAGER();
#endif
        ssd->g_init = 1;
#ifdef FTL_GET_WRITE_WORKLOAD
        ssd->fp_write_workload = fopen("./data/p_write_workload.txt","a");
#endif
#ifdef FTL_IO_LATENCY
        ssd->fp_ftl_w = fopen("./data/p_ftl_w.txt","a");
        ssd->fp_ftl_r = fopen("./data/p_ftl_r.txt","a");
#endif
        SSD_IO_INIT(s);

    }
}

void FTL_TERM(IDEState *s)
{
	printf("[%s] start\n", __FUNCTION__);

#ifdef FIRM_IO_BUFFER
	TERM_IO_BUFFER(s);
#endif
	TERM_MAPPING_TABLE(s);
	TERM_INVERSE_MAPPING_TABLE(s);
	TERM_VALID_ARRAY(s);
	TERM_BLOCK_STATE_TABLE(s);
	TERM_EMPTY_BLOCK_LIST(s);
	TERM_VICTIM_BLOCK_LIST(s);
	TERM_PERF_CHECKER(s);

#ifdef MONITOR_ON
	TERM_LOG_MANAGER(s);
#endif

#ifdef FTL_IO_LATENCY
	fclose(ssd->fp_ftl_w);
	fclose(ssd->fp_ftl_r);
#endif
}

int FTL_READ(IDEState *s, int32_t sector_num, unsigned int length)
{
	int ret = SUCCESS;

#ifdef GET_FTL_WORKLOAD
    /* Coperd: this is used to record all the IO requests */
	FILE *fp_workload = fopen("./data/workload_ftl.txt","a");
	struct timeval tv;
	struct tm *lt;
	double curr_time;
	gettimeofday(&tv, 0);
	lt = localtime(&(tv.tv_sec));
	curr_time = lt->tm_hour*3600 + lt->tm_min*60 + lt->tm_sec + 
        (double)tv.tv_usec/(double)1000000;
	fprintf(fp_workload,"%lf %d %u %x\n", curr_time, sector_num, length, 1);
	fclose(fp_workload);
#endif
#ifdef FTL_IO_LATENCY
	int64_t start_ftl_r, end_ftl_r;
	start_ftl_r = get_usec();
#endif

    /* Coperd: real FTL read finished below */
	ret = _FTL_READ(s, sector_num, length);

#ifdef FTL_IO_LATENCY
	end_ftl_r = get_usec();
	if (length >= 128) /* Coperd: why 128 here */
		fprintf(ssd->fp_ftl_r,"%ld\t%u\n", end_ftl_r - start_ftl_r, length);
#endif

    return ret;
}

void FTL_WRITE(IDEState *s, int32_t sector_num, unsigned int length)
{

#ifdef GET_FTL_WORKLOAD
    FILE* fp_workload = fopen("./data/workload_ftl.txt","a");
    struct timeval tv;
    struct tm *lt;
    double currtime;
    gettimeofday(&tv, 0);
    lt = localtime(&(tv.tv_sec));
    currtime = lt->tm_hour*3600 + lt->tm_min*60 + lt->tm_sec + 
        (double)tv.tv_usec/(double)1000000;
    fprintf(fp_workload,"%lf %d %u %x\n",currtime, sector_nb, length, 0);
    fclose(fp_workload);
#endif
#ifdef FTL_IO_LATENCY
    int64_t start_ftl_w, end_ftl_w;
    start_ftl_w = get_usec();
#endif
    _FTL_WRITE(s, sector_num, length);
#ifdef FTL_IO_LATENCY
    end_ftl_w = get_usec();
    if (length >= 128)
        fprintf(ssd->fp_ftl_w,"%ld\t%u\n", end_ftl_w - start_ftl_w, length);
#endif
}

int _FTL_READ(IDEState *s, int32_t sector_num, unsigned int length)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

#ifdef FTL_DEBUG
    printf("[%s] Start\n", __FUNCTION__);
#endif

    if (sector_num + length > ssdconf->sector_nb) {
        printf("Error[%s] Exceed Sector number\n", __FUNCTION__); 
        return FAIL;	
    }

    int nb_planes = ssdconf->flash_nb * ssdconf->planes_per_flash;

    int32_t lpn;
    int32_t ppn;
    int32_t lba = sector_num; /* Coperd: sector_nb means the starting sector */
    unsigned int remain = length;
    unsigned int left_skip = sector_num % ssdconf->sectors_per_page;
    unsigned int right_skip;
    /* 
     * Coperd: uplevel read will be decomposed of several NAND read here, 
     * read_sects means the number of sectors for each NAND read 
     */
    unsigned int read_sects; 

    unsigned int ret = FAIL;
    int read_page_nb = 0;
    int io_page_nb;

#ifdef FIRM_IO_BUFFER
    INCREASE_RB_FTL_POINTER(s, length);
#endif

    while (remain > 0) {

        if (remain > ssdconf->sectors_per_page - left_skip) {
            right_skip = 0;
        } else {
            right_skip = ssdconf->sectors_per_page - left_skip - remain;
        }
        read_sects = ssdconf->sectors_per_page - left_skip - right_skip;

        lpn = lba / (int32_t)ssdconf->sectors_per_page;
        ppn = GET_MAPPING_INFO(s, lpn);

        if (ppn == -1) {
#ifdef DEBUG_LATENCY
            mylog("[%s] no mapping\n", get_ssd_name(s));
#endif 
#ifdef FIRM_IO_BUFFER
            INCREASE_RB_LIMIT_POINTER(s);
#endif
            return ret;
        }

        lba += read_sects;
        remain -= read_sects;
        left_skip = 0;
    }

    ALLOC_IO_REQUEST(s, sector_num, length, READ, &io_page_nb);

    /* 
     * Coperd: record I/O's channel/chip information
     * if this I/O will access the [channel/chip], the corresponding bit is 1,
     * otherwise it's 0, in AIO layer, we check this bit to determin how long
     * this I/O will be blocked
     */
    int slot = 0;
    if (ssd->gc_mode == WHOLE_BLOCKING) {
        slot = 1;
    } else if (ssd->gc_mode == CHANNEL_BLOCKING) {
        slot = ssdconf->channel_nb;
    } else if (ssd->gc_mode == CHIP_BLOCKING) {
        slot = nb_planes;
    }
    s->bs->io_stat = qemu_mallocz(sizeof(int)*slot);

    remain = length;
    lba = sector_num;
    left_skip = sector_num % ssdconf->sectors_per_page;
    int64_t flash_num = 0;
    int64_t block_num = 0;
    int64_t page_num = 0;
    int pos = 0;
    /* Coperd: max_gc_endtime is per-IO variable, updated upon each SSD_READ */
    s->bs->max_gc_endtime = 0;

    while (remain > 0) {

        if (remain > ssdconf->sectors_per_page - left_skip) {
            right_skip = 0;
        } else {
            right_skip = ssdconf->sectors_per_page - left_skip - remain;
        }
        read_sects = ssdconf->sectors_per_page - left_skip - right_skip;

        lpn = lba / (int32_t)ssdconf->sectors_per_page;

#ifdef FTL_MAP_CACHE
        ppn = CACHE_GET_PPN(s, lpn);
#else
        ppn = GET_MAPPING_INFO(s, lpn);
#endif

        if (ppn == -1) {
            printf("[%s, %" PRId32 "] no mapping info\n", get_ssd_name(s), lba);
            return ret;
        }

        flash_num = CALC_FLASH(s, ppn);
        block_num = CALC_BLOCK(s, ppn);
        page_num = CALC_PAGE(s, ppn);

        if (ssd->gc_mode == WHOLE_BLOCKING) {
            /* Coperd: for whole-blocking mode, GC affects all I/Os */
            pos = 0;
        } else if (ssd->gc_mode == CHANNEL_BLOCKING) {
            pos = flash_num % ssdconf->channel_nb;
        } else if (ssd->gc_mode == CHIP_BLOCKING) {
            pos = flash_num * ssdconf->planes_per_flash + 
                block_num % ssdconf->planes_per_flash;
        }

        if (pos < 0 || pos >= s->bs->gc_slots) {
            mylog("PAGE cannot be located Unit at %d\n", pos);
        }
        s->bs->io_stat[pos]  = 1;
        if (s->bs->gc_endtime[pos] > s->bs->max_gc_endtime) {
            s->bs->max_gc_endtime = s->bs->gc_endtime[pos];
        }

        ret = SSD_PAGE_READ(s, flash_num, block_num, page_num, read_page_nb, 
                READ, io_page_nb);  

#ifdef FTL_DEBUG
        if (ret == SUCCESS) {
            printf("\t read complete [%u]\n",ppn);
        } else if (ret == FAIL) {
            printf("ERROR[%s] %u page read fail \n", __FUNCTION__, ppn);
        }
#endif
        read_page_nb++;

        lba += read_sects;
        remain -= read_sects;
        left_skip = 0;
    }

    /* Coperd: update io_request_seq_nb (+1) */
    INCREASE_IO_REQUEST_SEQ_NB(s);

#ifdef MONITOR_ON
    char szTemp[1024];
    sprintf(szTemp, "READ PAGE %d ", length);
    WRITE_LOG(szTemp);
#endif

#ifdef FTL_DEBUG
    printf("[%s] Complete\n", __FUNCTION__);
#endif

    return ret;
}

int _FTL_WRITE(IDEState *s, int32_t sector_num, unsigned int length)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

#ifdef FTL_DEBUG
    printf("[%s] Start\n", __FUNCTION__);
#endif

#ifdef FTL_GET_WRITE_WORKLOAD
    fprintf(ssd->fp_write_workload,"%d\t%u\n", sector_nb, length);
#endif

    int io_page_nb;

    if (sector_num + length > ssdconf->sector_nb) {
        printf("[%s] Exceed Sector number\n", get_ssd_name(s));
        return FAIL;
    } else {
        ALLOC_IO_REQUEST(s, sector_num, length, WRITE, &io_page_nb);
    }

    int32_t lba = sector_num;
    int32_t lpn;
    int32_t new_ppn;
    int32_t old_ppn;

    unsigned int remain = length;
    unsigned int left_skip = sector_num % ssdconf->sectors_per_page;
    unsigned int right_skip;
    unsigned int write_sects;

    unsigned int ret = FAIL;
    int write_page_nb = 0;

    while (remain > 0) {

        if (remain > ssdconf->sectors_per_page - left_skip) {
            right_skip = 0;
        } else {
            right_skip = ssdconf->sectors_per_page - left_skip - remain;
        }

        write_sects = ssdconf->sectors_per_page - left_skip - right_skip;

#ifdef FIRM_IO_BUFFER
        INCREASE_WB_FTL_POINTER(s, write_sects);
#endif

#ifdef WRITE_NOPARAL
        ret = GET_NEW_PAGE(s, VICTIM_NOPARAL, empty_block_table_index, &new_ppn);
#else
        ret = GET_NEW_PAGE(s, VICTIM_OVERALL, 
                           ssdconf->empty_table_entry_nb, &new_ppn);
#endif
        if (ret == FAIL) {
            printf("[%s] Get new page fail \n", get_ssd_name(s));
            return FAIL;
        }

        lpn = lba / (int32_t)ssdconf->sectors_per_page;
        old_ppn = GET_MAPPING_INFO(s, lpn);

        if ((left_skip || right_skip) && (old_ppn != -1)) {
            ret = SSD_PAGE_PARTIAL_WRITE(s, 
                                         CALC_FLASH(s, old_ppn), 
                                         CALC_BLOCK(s, old_ppn), 
                                         CALC_PAGE(s, old_ppn),
                                         CALC_FLASH(s, new_ppn), 
                                         CALC_BLOCK(s, new_ppn), 
                                         CALC_PAGE(s, new_ppn),
                                         write_page_nb, 
                                         WRITE, 
                                         io_page_nb);
        } else {
            ret = SSD_PAGE_WRITE(s, 
                                 CALC_FLASH(s, new_ppn), 
                                 CALC_BLOCK(s, new_ppn), 
                                 CALC_PAGE(s, new_ppn), 
                                 write_page_nb, 
                                 WRITE, 
                                 io_page_nb);
        }

        write_page_nb++;

        UPDATE_OLD_PAGE_MAPPING(s, lpn);
        UPDATE_NEW_PAGE_MAPPING(s, lpn, new_ppn);

        char *ssdname = get_ssd_name(s);
        if (ret == FAIL) {
            printf("[%s] write to page %" PRId32 " failed\n", ssdname, new_ppn);
        }

#ifdef FTL_DEBUG
        if (ret == SUCCESS) {
            printf("\twrite complete [%d, %d, %d]\n", CALC_FLASH(s, new_ppn), 
                    CALC_BLOCK(s, new_ppn), CALC_PAGE(s, new_ppn));
        } else if (ret == FAIL) {
            printf("ERROR[%s] %d page write fail \n", __FUNCTION__, new_ppn);
        }
#endif
        lba += write_sects;
        remain -= write_sects;
        left_skip = 0;

    }

    INCREASE_IO_REQUEST_SEQ_NB(s);

#ifdef GC_ON
    GC_CHECK(s, CALC_FLASH(s, new_ppn), CALC_BLOCK(s, new_ppn));
#endif

#ifdef FIRM_IO_BUFFER
    INCREASE_WB_LIMIT_POINTER(s);
#endif

#ifdef MONITOR_ON
    char szTemp[1024];
    sprintf(szTemp, "WRITE PAGE %d ", length);
    WRITE_LOG(szTemp);
    sprintf(szTemp, "WB CORRECT %d", write_page_nb);
    WRITE_LOG(szTemp);
#endif

#ifdef FTL_DEBUG
    printf("[%s] End\n", __FUNCTION__);
#endif
    return ret;
}
