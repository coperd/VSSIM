// File: vssim_config_manager.c
// Date: 2014. 12. 03.
// Author: Jinsoo Yoo (jedisty@hanyang.ac.kr)
// Copyright(c)2014
// Hanyang University, Seoul, Korea
// Embedded Software Systems Laboratory. All right reserved

//#define DEBUG_CONF

#include "vssim_config_manager.h"
#include "ssd_util.h"
#include "mytrace.h"

extern int NB_CHANNEL, NB_CHIP;

void INIT_VSSIM_CONFIG(IDEState *s)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);
    //SSDPerf *ssdperf = &(ssd->perf);

    set_ssd_struct_filename(s, ssd->conf_filename, ".conf");
#ifdef DEBUG_CONF
    printf("CONF file name is [%s]\n", ssd->conf_filename);
#endif

    FILE* pfData = fopen(ssd->conf_filename, "r");

    char* szCommand = NULL;

    szCommand = (char *)malloc((sizeof(char) * 1024));
    memset(szCommand, 0x00, 1024);
    if (pfData != NULL) {
        while (fscanf(pfData, "%s", szCommand) != EOF) {
            if (strcmp(szCommand, "PAGE_SIZE") == 0) {
                fscanf(pfData, "%d", &ssdconf->page_size);
            } else if (strcmp(szCommand, "PAGE_NB") == 0) {
                fscanf(pfData, "%d", &ssdconf->page_nb);
            } else if (strcmp(szCommand, "SECTOR_SIZE") == 0) {
                fscanf(pfData, "%d", &ssdconf->sector_size);
            } else if (strcmp(szCommand, "FLASH_NB") == 0) {
                fscanf(pfData, "%d", &ssdconf->flash_nb);
            } else if (strcmp(szCommand, "BLOCK_NB") == 0) {
                fscanf(pfData, "%d", &ssdconf->block_nb);
            } else if (strcmp(szCommand, "PLANES_PER_FLASH") == 0) {
                fscanf(pfData, "%d", &ssdconf->planes_per_flash);
            } else if (strcmp(szCommand, "REG_WRITE_DELAY") == 0) {
                fscanf(pfData, "%d", &ssdconf->reg_write_delay);
            } else if (strcmp(szCommand, "CELL_PROGRAM_DELAY") == 0) {
                fscanf(pfData, "%d", &ssdconf->cell_program_delay);
            } else if (strcmp(szCommand, "REG_READ_DELAY") == 0) {
                fscanf(pfData, "%d", &ssdconf->reg_read_delay);
            } else if (strcmp(szCommand, "CELL_READ_DELAY") == 0) {
                fscanf(pfData, "%d", &ssdconf->cell_read_delay);
            } else if (strcmp(szCommand, "BLOCK_ERASE_DELAY") == 0) {
                fscanf(pfData, "%d", &ssdconf->block_erase_delay);
            } else if (strcmp(szCommand, "CHANNEL_SWITCH_DELAY_R") == 0) {
                fscanf(pfData, "%d", &ssdconf->channel_switch_delay_r);
            } else if (strcmp(szCommand, "CHANNEL_SWITCH_DELAY_W") == 0) {
                fscanf(pfData, "%d", &ssdconf->channel_switch_delay_w);
            } else if (strcmp(szCommand, "DSM_TRIM_ENABLE") == 0) {
                fscanf(pfData, "%d", &ssdconf->dsm_trim_enable);
            } else if (strcmp(szCommand, "IO_PARALLELISM") == 0) {
                fscanf(pfData, "%d", &ssdconf->io_parallelism);
            } else if (strcmp(szCommand, "CHANNEL_NB") == 0) {
                fscanf(pfData, "%d", &ssdconf->channel_nb);
                NB_CHANNEL = ssdconf->channel_nb;
            } else if (strcmp(szCommand, "OVP") == 0) {
                fscanf(pfData, "%d", &ssdconf->ovp);
            } else if (strcmp(szCommand, "GC_THRESHOLD") == 0) {
                fscanf(pfData, "%lf", &ssdconf->gc_threshold);
            } else if (strcmp(szCommand, "GC_THRESHOLD_HARD") == 0) {
                fscanf(pfData, "%lf", &ssdconf->gc_threshold_hard);
            } else if (strcmp(szCommand, "WARMUP") == 0) {
                fscanf(pfData, "%d", &ssd->nwarmup);
            } else if (strcmp(szCommand, "GC_TIME") == 0) {
                fscanf(pfData, "%ld", &ssd->gc_time);
            } else if (strcmp(szCommand, "GC_MODE") == 0) {
                fscanf(pfData, "%d", &ssd->gc_mode);
            } else if (strcmp(szCommand, "INTERFACE") == 0) {
                fscanf(pfData, "%d", &ssd->interface_type);
            }

#if defined FTL_MAP_CACHE || defined Polymorphic_FTL
            else if (strcmp(szCommand, "CACHE_IDX_SIZE") == 0) {
                fscanf(pfData, "%d", &ssdconf->cache_idx_size);
            }
#endif

#ifdef FIRM_IO_BUFFER
            else if (strcmp(szCommand, "WRITE_BUFFER_FRAME_NB") == 0) {
                fscanf(pfData, "%u", &ssdconf->write_buffer_frame_nb);
            } else if (strcmp(szCommand, "READ_BUFFER_FRAME_NB") == 0) {
                fscanf(pfData, "%u", &ssdconf->read_buffer_frame_nb);
            }
#endif
#ifdef HOST_QUEUE
            else if (strcmp(szCommand, "HOST_QUEUE_ENTRY_NB") == 0) {
                fscanf(pfData, "%u", &ssdconf->host_queue_entry_nb);
            }
#endif
#if defined FAST_FTL || defined LAST_FTL
            else if (strcmp(szCommand, "LOG_RAND_BLOCK_NB") == 0) {
                fscanf(pfData, "%d", &ssdconf->log_rand_block_nb);
            } else if (strcmp(szCommand, "LOG_SEQ_BLOCK_NB") == 0) {
                fscanf(pfData, "%d", &ssdconf->log_seq_block_nb);
            }	
#endif
            memset(szCommand, 0x00, 1024);
        }	
        fclose(pfData);

    } else {
        printf("SSD configuration file open failed!!!\n");
        exit(1);
    }

    /* Exception Handler */
    if (ssdconf->flash_nb < ssdconf->channel_nb) {
        printf("ERROR[%s] Wrong CHANNEL_NB %d\n", __FUNCTION__, ssdconf->channel_nb);
        return;
    }
    if (ssdconf->planes_per_flash != 1 && ssdconf->planes_per_flash % 2 != 0) {
        printf("ERROR[%s] Wrong PLANAES_PER_FLASH %d\n", __FUNCTION__, ssdconf->planes_per_flash);
        return;
    }
#ifdef FIRM_IO_BUFFER
    if (ssdconf->write_buffer_frame_nb == 0 || ssdconf->read_buffer_frame_nb == 0) {
        printf("ERROR[%s] Wrong parameter for SSD_IO_BUFFER", __FUNCTION__);
        return;
    }
#endif

    /* SSD Configuration */
    ssdconf->sectors_per_page = ssdconf->page_size / ssdconf->sector_size;
    ssdconf->pages_per_flash = ssdconf->page_nb * ssdconf->block_nb;
    ssdconf->sector_nb = (int64_t)ssdconf->sectors_per_page * (int64_t)ssdconf->page_nb * (int64_t)ssdconf->block_nb * (int64_t)ssdconf->flash_nb;
#ifndef Polymorphic_FTL
    ssdconf->way_nb = ssdconf->flash_nb / ssdconf->channel_nb;
#endif

    /* Mapping Table */
    ssdconf->block_mapping_entry_nb = (int64_t)ssdconf->block_nb * (int64_t)ssdconf->flash_nb;
    ssdconf->pages_in_ssd = (int64_t)ssdconf->page_nb * (int64_t)ssdconf->block_nb * (int64_t)ssdconf->flash_nb;

#ifdef PAGE_MAP
    ssdconf->page_mapping_entry_nb = (int64_t)ssdconf->page_nb * (int64_t)ssdconf->block_nb * (int64_t)ssdconf->flash_nb;
#endif

#if defined PAGE_MAP || defined BLOCK_MAP
    ssdconf->each_empty_table_entry_nb = (int64_t)ssdconf->block_nb / (int64_t)ssdconf->planes_per_flash;
    ssdconf->empty_table_entry_nb = ssdconf->flash_nb * ssdconf->planes_per_flash;
    ssdconf->victim_table_entry_nb = ssdconf->flash_nb * ssdconf->planes_per_flash;

    ssdconf->data_block_nb = ssdconf->block_nb;
#endif

#if defined FAST_FTL || defined LAST_FTL
    ssdconf->data_block_nb = ssdconf->block_nb - ssdconf->log_rand_block_nb - ssdconf->log_seq_block_nb;
    ssdconf->data_mapping_entry_nb = (int64_t)ssdconf->flash_nb * (int64_t)ssdconf->data_block_nb;

    ssdconf->each_empty_block_entry_nb = (int64_t)ssdconf->block_nb / (int64_t)ssdconf->planes_per_flash;
    ssdconf->empty_block_table_nb = ssdconf->flash_nb * ssdconf->planes_per_flash;
#endif

#ifdef DA_MAP
    if (ssdconf->bm_start_sector_nb < 0 || ssdconf->bm_start_sector_nb >= ssdconf->sector_nb) {
        printf("ERROR[%s] BM_START_SECTOR_NB %d \n", __FUNCTION__, ssdconf->bm_start_sector_nb);
    }

    ssdconf->da_page_mapping_entry_nb = CALC_DA_PM_ENTRY_NB(); 
    ssdconf->da_block_mapping_entry_nb = CALC_DA_BM_ENTRY_NB();
    ssdconf->each_empty_table_entry_nb = (int64_t)ssdconf->block_nb / (int64_t)ssdconf->planes_per_flash;
    ssdconf->empty_table_entry_nb = ssdconf->flash_nb * ssdconf->planes_per_flash;
    ssdconf->victim_table_entry_nb = ssdconf->flash_nb * ssdconf->planes_per_flash;
#endif

    /* FAST Performance Test */
#ifdef FAST_FTL
    ssdconf->seq_mapping_entry_nb = (int64_t)ssdconf->flash_nb * (int64_t)ssdconf->log_seq_block_nb;
    ssdconf->ran_mapping_entry_nb = (int64_t)ssdconf->flash_nb * (int64_t)ssdconf->log_rand_block_nb;
    ssdconf->ran_log_mapping_entry_nb = (int64_t)ssdconf->flash_nb * (int64_t)ssdconf->log_rand_block_nb * (int64_t)ssdconf->page_nb;

    // PARAL_DEGREE = 4;
    ssdconf->paral_degree = ssdconf->ran_mapping_entry_nb;
    if (ssdconf->paral_degree > ssdconf->ran_mapping_entry_nb) {
        printf("[INIT_SSD_CONFIG] ERROR PARAL_DEGREE \n");
        return;
    }
    ssdconf->paral_count = ssdconf->paral_degree * ssdconf->page_nb;
#endif

#ifdef LAST_FTL
    ssdconf->seq_mapping_entry_nb = (int64_t)ssdconf->flash_nb * (int64_t)ssdconf->log_seq_block_nb;
    ssdconf->ran_mapping_entry_nb = (int64_t)ssdconf->flash_nb * ((int64_t)ssdconf->log_rand_block_nb/2);
    ssdconf->ran_log_mapping_entry_nb = (int64_t)ssdconf->flash_nb * ((int64_t)ssdconf->log_rand_block_nb/2) * (int64_t)ssdconf->page_nb;

    ssdconf->seq_threshold = ssdconf->sectors_per_page * 4;
    ssdconf->hot_page_nb_threshold = ssdconf->page_nb;

    ssdconf->paral_degree = ssdconf->ran_mapping_entry_nb;
    if (ssdconf->paral_degree > ssdcon->ran_mapping_entry_nb) {
        printf("[INIT_SSD_CONFIG] ERROR PARAL_DEGREE \n");
        return;
    }
    ssdconf->paral_count = ssdconf->paral_degree * ssdconf->page_nb;
#endif

    /* Garbage Collection */
#if defined PAGE_MAP || defined BLOCK_MAP || defined DA_MAP
    //ssdconf->gc_threshold = 0.25; // 0.7 for 70%, 0.9 for 90%
    //ssdconf->gc_threshold_hard = 0.28;
    ssdconf->gc_threshold_block_nb = (int)((1-ssdconf->gc_threshold) * (double)ssdconf->block_mapping_entry_nb);
    ssdconf->gc_threshold_block_nb_hard = (int)((1-ssdconf->gc_threshold_hard) * (double)ssdconf->block_mapping_entry_nb);
    ssdconf->gc_threshold_block_nb_each = (int)((1-ssdconf->gc_threshold) * (double)ssdconf->each_empty_table_entry_nb);
    if (ssdconf->ovp != 0) {
        ssdconf->gc_victim_nb = ssdconf->flash_nb * ssdconf->block_nb * ssdconf->ovp / 100 / 2;
    } else {
        ssdconf->gc_victim_nb = 1; /* Coperd: for each time, we only clean one GC */
    }
#endif

    /* Map Cache */
#ifdef FTL_MAP_CACHE
    ssdconf->map_entry_size = sizeof(int32_t);
    ssdconf->map_entries_per_page = ssdconf->page_size / ssdconf->map_entry_size;
    ssdconf->map_entry_nb = ssdconf->page_mapping_entry_nb / ssdconf->map_entries_per_page;	

    NB_CHIP = ssdconf->flash_nb;
#endif

    /* Polymorphic FTL */
#ifdef Polymorphic_FTL
    ssdconf->way_nb = 1;

    ssdconf->phy_spare_size = 436;
    ssdconf->nr_phy_blocks = (int64_t)ssdconf->flash_nb * (int64_t)ssdconf->way_nb * (int64_t)ssdconf->block_nb;
    ssdconf->nr_phy_pages = ssdconf->nr_phy_blocks * (int64_t)ssdconf->page_nb;
    ssdconf->nr_phy_sectors = ssdconf->nr_phy_pages * (int64_t)ssdconf->sectors_per_page;

    ssdconf->nr_reserved_phy_super_blocks = 4;
    ssdconf->nr_reserved_phy_blocks = ssdconf->flash_nb * ssdconf->way_nb * ssdconf->nr_reserved_phy_super_blocks;
    ssdconf->nr_reserved_phy_pages = ssdconf->nr_reserved_phy_blocks * ssdconf->page_nb;
#endif
    free(szCommand);

}

void INIT_SSD_CONFIG(IDEState *s)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);
    /* SSDState structure initlization */
    ssd->interface_type = DEFAULT_INTERFACE; /* DEFAULT_INTERFACE as default */
    ssd->gc_mode = CHANNEL_BLOCKING; /* by default, use channel blocking GC */
    ssd->gc_cnt = 0;
    /* by default, each GC takes 10ms (only useful when real GC time is not used */
    ssd->gc_time = 10000; 
    ssd->gc_fail_cnt = 0;
    ssd->read_cnt = 0;
    ssd->write_cnt = 0;
    ssd->nwarmup = 0;

    INIT_VSSIM_CONFIG(s);
}

#ifdef DA_MAP
int64_t CALC_DA_PM_ENTRY_NB(IDEState *s)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

	int64_t ret_page_nb = (int64_t)ssdconf->bm_start_sector_nb / ssdconf->sectors_per_page;
	if ((ssdconf->bm_start_sector_nb % ssdconf->sectors_per_page) != 0)
		ret_page_nb += 1;

	int64_t block_nb = ret_page_nb / ssdconf->page_nb;
	if ((ret_page_nb % ssdconf->page_nb) != 0)
		block_nb += 1;

	ret_page_nb = block_nb * ssdconf->page_nb;
	ssdconf->bm_start_sect_nb = ret_page_nb * ssdconf->sectors_per_page;

	return ret_page_nb;
}

int64_t CALC_DA_BM_ENTRY_NB(IDEState *s)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

	int64_t total_page_nb = (int64_t)ssdconf->page_nb * ssdconf->block_nb * ssdconf->flash_nb;
	int64_t ret_block_nb = ((int64_t)ssdconf->total_page_nb - ssdconf->da_page_mapping_entry_nb)/(int64_t)ssdconf->page_nb;

	int64_t temp_total_page_nb = ret_block_nb * ssdconf->page_nb + ssdconf->da_page_mapping_entry_nb;
	if (temp_total_page_nb != total_page_nb) {
		printf("ERROR[%s] %ld != %ld\n", __FUNCTION__, total_page_nb, temp_total_page_nb);
	}

	return ret_block_nb;
}
#endif
