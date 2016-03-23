// File: ssd_io_manager.c
// Date: 2014. 12. 03.
// Author: Jinsoo Yoo (jedisty@hanyang.ac.kr)
// Copyright(c)2014
// Hanyang University, Seoul, Korea
// Embedded Software Systems Laboratory. All right reserved

#include "ssd_io_manager.h"
#include "ftl_perf_manager.h"
#include "ssd_util.h"

#ifndef VSSIM_BENCH
#include "qemu-kvm.h"
#endif

/* Coperd: get current timestamp in microseconds */
int64_t get_usec(void)
{
	int64_t t = 0;
	struct timeval tv;
	struct timezone tz;

	gettimeofday(&tv, &tz);
	t = tv.tv_sec;
	t *= 1000000;
	t += tv.tv_usec;

	return t;
}

static inline int get_ssd_channel(IDEState *s, int flash_num)
{
    int channel;
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    channel = flash_num % ssdconf->channel_nb;

    return channel;
}

static inline int get_ssd_reg(IDEState *s, int flash_num, int block_num)
{
    int reg;
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    reg = flash_num * ssdconf->planes_per_flash + 
        block_num % ssdconf->planes_per_flash; 

    return reg;
}

int SSD_IO_INIT(IDEState *s)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);
    int nb_regs = ssdconf->flash_nb * ssdconf->planes_per_flash;

	int i = 0;

    strcpy(ssd->ssd_version, "1.0");
    strcpy(ssd->ssd_date, "13.04.11");

    ssd->io_update_overhead = 0;
    ssd->io_alloc_overhead = 0;
    ssd->init_diff_reg = 0;

	/* Print SSD version */
#ifdef DEBUG_SSD_IO
	printf("[%s] SSD Version: %s ver. (%s)\n", __FUNCTION__, ssd->ssd_version, 
            ssd->ssd_date);
#endif 

	/* Init Variable for Channel Switch Delay */
	ssd->old_channel_num = ssdconf->channel_nb;
	ssd->old_channel_cmd = NOOP;
	ssd->old_channel_time = 0;

	/* Init Variable for Time-stamp */

	/* Init Command and Command type */
    ssd->reg_io_cmd = qemu_malloc(sizeof(int) * nb_regs);
    ssd->reg_io_type = qemu_malloc(sizeof(int) * nb_regs);
    ssd->reg_io_time = qemu_malloc(sizeof(int64_t) * nb_regs);
    ssd->cell_io_time = qemu_malloc(sizeof(int64_t) * nb_regs);

	/* Init IO Overhead */
    ssd->io_overhead = qemu_mallocz(sizeof(int64_t) * nb_regs);

    for (i = 0; i < nb_regs; i++) {
        ssd->reg_io_cmd[i] = NOOP;
        ssd->reg_io_cmd[i] = NOOP;
        ssd->reg_io_time[i] = -1;
        ssd->cell_io_time[i] = -1;
        ssd->io_overhead[i] = 0;
    }

	/* Init Access sequence_nb */
	ssd->access_nb = (int **)qemu_malloc(sizeof(int *) * nb_regs);
	for (i = 0; i< nb_regs; i++) {
        ssd->access_nb[i] = qemu_malloc(sizeof(int) * 2);
        memset(ssd->access_nb[i], -1, sizeof(int) * 2);
	}

    /* ssd_io_init complete */
#ifdef DEBUG_SSD_IO
	printf("[%s] complete\n", __FUNCTION__);
#endif

	return SUCCESS;
}

int SSD_PAGE_WRITE(IDEState *s, unsigned int flash_num, unsigned int block_num, 
        unsigned int page_num, int offset, int type, int io_page_num)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    int ret = FAIL;
    int delay_ret;

    int channel = get_ssd_channel(s, flash_num);
    int reg = get_ssd_reg(s, flash_num, block_num);

    /* Delay Operation */
    SSD_CH_ENABLE(s, channel);	// channel enable	

    if (ssdconf->io_parallelism == 0) {
        delay_ret = SSD_FLASH_ACCESS(s, flash_num, reg);
    } else {
        delay_ret = SSD_REG_ACCESS(s, reg);
    }	

    /* Check Channel Operation */
    /* Coperd: polling on channel */
    while (ret == FAIL) {
        ret = SSD_CH_ACCESS(s, channel);
    }

    /* Record Time Stamp */
    SSD_CH_RECORD(s, channel, WRITE, offset, delay_ret);
    SSD_REG_RECORD(s, reg, WRITE, type, offset, channel);
    SSD_CELL_RECORD(s, reg, WRITE);

#ifdef O_DIRECT_VSSIM
    if (offset == io_page_num-1) {
        SSD_REMAIN_IO_DELAY(s, reg);
    }
#endif

    return SUCCESS;
}

int SSD_PAGE_PARTIAL_WRITE(IDEState *s, unsigned int old_flash_num, 
        unsigned int old_block_num, unsigned int old_page_num, 
        unsigned int new_flash_num, unsigned int new_block_num, 
        unsigned int new_page_num, int offset, int type, int io_page_nb)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    int ret = FAIL;
    int delay_ret;

    /* Calculate ch & reg */
    int channel = get_ssd_channel(s, old_flash_num);
    int reg = get_ssd_reg(s, old_flash_num, old_block_num);

    /* Delay Operation */
    SSD_CH_ENABLE(s, channel);	// channel enable	

    if (ssdconf->io_parallelism == 0) {
        delay_ret = SSD_FLASH_ACCESS(s, old_flash_num, reg);
    } else {
        delay_ret = SSD_REG_ACCESS(s, reg);
    }	

    /* Check Channel Operation */
    while (ret == FAIL) {
        ret = SSD_CH_ACCESS(s, channel);
    }

    /* Record Time Stamp */
    SSD_CH_RECORD(s, channel, READ, offset, delay_ret);
    SSD_REG_RECORD(s, reg, READ, type, offset, channel);
    SSD_CELL_RECORD(s, reg, READ);

    SSD_REMAIN_IO_DELAY(s, reg);

    /* Write 1 Page */

    /* Calculate ch & reg */
    channel = get_ssd_channel(s, new_flash_num);
    reg = get_ssd_reg(s, new_flash_num, new_block_num);

    /* Delay Operation */
    SSD_CH_ENABLE(s, channel);	// channel enable	

    if (ssdconf->io_parallelism == 0) {
        delay_ret = SSD_FLASH_ACCESS(s, new_flash_num, reg);
    } else {
        delay_ret = SSD_REG_ACCESS(s, reg);
    }	

    /* Check Channel Operation */
    while (ret == FAIL) {
        ret = SSD_CH_ACCESS(s, channel);
    }

    /* Record Time Stamp */
    SSD_CH_RECORD(s, channel, WRITE, offset, delay_ret);
    SSD_REG_RECORD(s, reg, WRITE, type, offset, channel);
    SSD_CELL_RECORD(s, reg, WRITE);

#ifdef O_DIRECT_VSSIM
    if (offset == io_page_nb-1) {
        SSD_REMAIN_IO_DELAY(s, reg);
    }
#endif

    return SUCCESS;	
}

/* 
 * Coperd: flash_num means flash NO., block_num means block_num within a flash 
 * chip, page_num means the page ID within one block
 * This function is responsible for reading one specific page at certain offset
 */
int SSD_PAGE_READ(IDEState *s, unsigned int flash_num, unsigned int block_num, 
                  unsigned int page_num, int offset, int type, int io_page_num)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    int delay_ret;

    /* Calculate ch & reg */
    int channel = get_ssd_channel(s, flash_num);
    int reg = get_ssd_reg(s, flash_num, block_num);

    /* 
     * Coperd: in VSSIM model, one read operation include (data transfer between 
     * controller and page register) and (read/write/erase between page register 
     * and cells), what they do is to add some latency for the previous parts 
     */


    /* Delay Operation (1) channel enable first */
    SSD_CH_ENABLE(s, channel);	

    /* Access Register (2) access the register for data second */
    /* Coperd: ssdconf->io_parallelism ??? need to figure out */
    if (ssdconf->io_parallelism == 0) {
        delay_ret = SSD_FLASH_ACCESS(s, flash_num, reg);
    } else {
        delay_ret = SSD_REG_ACCESS(s, reg);
    }

    /* Record Time Stamp */
    SSD_CH_RECORD(s, channel, READ, offset, delay_ret);
    SSD_CELL_RECORD(s, reg, READ);
    SSD_REG_RECORD(s, reg, READ, type, offset, channel);

#ifdef O_DIRECT_VSSIM
    if (offset == io_page_num - 1) {
        SSD_REMAIN_IO_DELAY(s, reg);
    }
#endif

    return SUCCESS;
}

int SSD_BLOCK_ERASE(IDEState *s, unsigned int flash_num, unsigned int block_num)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    /* Calculate ch & reg */
    int channel = get_ssd_channel(s, flash_num);
    int reg = get_ssd_reg(s, flash_num, block_num);

    /* Delay Operation */
    if (ssdconf->io_parallelism == 0) {
        SSD_FLASH_ACCESS(s, flash_num, reg);
    } else {
        SSD_REG_ACCESS(s, reg);
    }

    /* Record Time Stamp */
    SSD_REG_RECORD(s, reg, ERASE, ERASE, -1, channel);
    SSD_CELL_RECORD(s, reg, ERASE);

    return SUCCESS;
}

int SSD_FLASH_ACCESS(IDEState *s, unsigned int flash_num, int reg)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    int i;
    int r_num = flash_num * ssdconf->planes_per_flash;
    int ret = 0;

    for (i = 0; i < ssdconf->planes_per_flash; i++) {
        /* Coperd: what does this part mean??? */
        if (r_num != reg && ssd->access_nb[r_num][0] == ssd->io_request_seq_nb) {
            /* That's OK */
        } else{
            ret = SSD_REG_ACCESS(s, r_num);
        }

        r_num++;
    }	

    return ret;
}

/* 
 * Coperd: this step is to introduce latency to Page Register Access, 
 * including cell r/w/e operation and register operation 
 */
int SSD_REG_ACCESS(IDEState *s, int reg)
{
    SSDState *ssd = &(s->ssd);
    //SSDConf *ssdconf = &(ssd->param);

    int reg_cmd = ssd->reg_io_cmd[reg];
    int ret = 0;

    if (reg_cmd == NOOP) {
        /* That's OK */
    } else if (reg_cmd == READ) {
        ret = SSD_CELL_READ_DELAY(s, reg);
        ret = SSD_REG_READ_DELAY(s, reg);
    } else if (reg_cmd == WRITE) {
        ret = SSD_REG_WRITE_DELAY(s, reg);
        ret = SSD_CELL_WRITE_DELAY(s, reg);
    } else if (reg_cmd == ERASE) {
        ret = SSD_BLOCK_ERASE_DELAY(s, reg);
    } else {
        printf("ERROR[%s] Command Error! %d\n", __FUNCTION__, ssd->reg_io_cmd[reg]);
    }

    return ret;
}

int SSD_CH_ENABLE(IDEState *s, int channel)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    if (ssdconf->channel_switch_delay_r == 0 && ssdconf->channel_switch_delay_w == 0)
        return SUCCESS;

    /* 
     * Coperd: if we are accessing the same channel as the last cmd, then we 
     * don't need to introduce latency here. channel access info is updated at 
     * the frequency of each read/write page operation, e.g. SSD_READ_PAGE()
     */

    /* 
     * Coperd: actually this is checking if the channel is busy! If not, no 
     * delay is introduced here and the incoming NAND request should wait here 
     * until time passed by the latency 
     */
    if (ssd->old_channel_num != channel) {
        SSD_CH_SWITCH_DELAY(s, channel); 
    }

    return SUCCESS;
}

int SSD_CH_RECORD(IDEState *s, int channel, int cmd, int offset, int ret)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    ssd->old_channel_num = channel; 
    ssd->old_channel_cmd = cmd;

    if (cmd == READ && offset != 0 && ret == 0) {
        ssd->old_channel_time += ssdconf->channel_switch_delay_r;
    } else if (cmd == WRITE && offset != 0 && ret == 0) {
        ssd->old_channel_time += ssdconf->channel_switch_delay_w;
    } else { /* Coperd: GC_READ comes here */
        ssd->old_channel_time = get_usec();
    }

    return SUCCESS;
}

int SSD_REG_RECORD(IDEState *s, int reg, int cmd, 
        int type, int offset, int channel)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    ssd->reg_io_cmd[reg] = cmd;
    ssd->reg_io_type[reg] = type;

    int64_t old_ch_time = ssd->old_channel_time;
    int ch_s_delay_w = ssdconf->channel_switch_delay_w;

    if (cmd == WRITE) {
        ssd->reg_io_time[reg] = old_ch_time + ch_s_delay_w; 
        SSD_UPDATE_CH_ACCESS_TIME(s, channel, ssd->reg_io_time[reg]);

        /* Update SATA request Info */
        if (type == WRITE || type == SEQ_WRITE || type == RAN_WRITE || 
                type == RAN_COLD_WRITE || type == RAN_HOT_WRITE) {
            ssd->access_nb[reg][0] = ssd->io_request_seq_nb;
            ssd->access_nb[reg][1] = offset;
            ssd->io_update_overhead = UPDATE_IO_REQUEST(s, ssd->io_request_seq_nb, 
                    offset, ssd->old_channel_time, UPDATE_START_TIME);
            SSD_UPDATE_IO_OVERHEAD(s, reg, ssd->io_update_overhead);
        } else {
            ssd->access_nb[reg][0] = -1;
            ssd->access_nb[reg][1] = -1;
        }
    } else if (cmd == READ) {
        ssd->reg_io_time[reg] = SSD_GET_CH_ACCESS_TIME_FOR_READ(s, channel, reg);

        /* Update SATA request Info */
        if (type == READ) {
            ssd->access_nb[reg][0] = ssd->io_request_seq_nb;
            ssd->access_nb[reg][1] = offset;
            ssd->io_update_overhead = UPDATE_IO_REQUEST(s, ssd->io_request_seq_nb, 
                    offset, ssd->old_channel_time, UPDATE_START_TIME);
            SSD_UPDATE_IO_OVERHEAD(s, reg, ssd->io_update_overhead);
        } else { /* Coperd: e.g. GC_READ */
            ssd->access_nb[reg][0] = -1;
            ssd->access_nb[reg][1] = -1;
        }
    } else if (cmd == ERASE) {
        /* Update SATA request Info */
        ssd->access_nb[reg][0] = -1;
        ssd->access_nb[reg][1] = -1;
    }	

    return SUCCESS;
}

int SSD_CELL_RECORD(IDEState *s, int reg, int cmd)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    int reg_w_delay = ssdconf->reg_write_delay;
    int ch_s_delay_r = ssdconf->channel_switch_delay_r;

    if (cmd == WRITE) {
        ssd->cell_io_time[reg] = ssd->reg_io_time[reg] + reg_w_delay;
    } else if (cmd == READ) {
        ssd->cell_io_time[reg] = ssd->old_channel_time + ch_s_delay_r;
    } else if (cmd == ERASE) {
        ssd->cell_io_time[reg] = get_usec();
    }

    return SUCCESS;
}

int SSD_CH_ACCESS(IDEState *s, int channel)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    int i, j;
    int ret = SUCCESS;
    int r_num;

    for (i = 0; i < ssdconf->way_nb; i++) {
        r_num = channel * ssdconf->planes_per_flash + 
            i * ssdconf->channel_nb * ssdconf->planes_per_flash; 
        for (j = 0; j < ssdconf->planes_per_flash; j++) {
            if (ssd->reg_io_time[r_num] <= get_usec() && 
                    ssd->reg_io_time[r_num] != -1) {
                if (ssd->reg_io_cmd[r_num] == READ) {
                    SSD_CELL_READ_DELAY(s, r_num);
                    SSD_REG_READ_DELAY(s, r_num);
                    ret = FAIL;
                } else if (ssd->reg_io_cmd[r_num] == WRITE) {
                    SSD_REG_WRITE_DELAY(s, r_num);
                    ret = FAIL;
                }
            }
            r_num++;	
        }
    }

    return ret;
}

void SSD_UPDATE_IO_OVERHEAD(IDEState *s, int reg, int64_t overhead_time)
{
    SSDState *ssd = &(s->ssd);
    //SSDConf *ssdconf = &(ssd->param);

    ssd->io_overhead[reg] += overhead_time;
    ssd->io_alloc_overhead = 0;
    ssd->io_update_overhead = 0;
}

int64_t SSD_CH_SWITCH_DELAY(IDEState *s, int channel)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    int64_t start = 0;
    int64_t	end = 0;
    int64_t diff = 0;

    int64_t switch_delay = 0;

    if (ssd->old_channel_cmd == READ) {
        switch_delay = ssdconf->channel_switch_delay_r;
    } else if (ssd->old_channel_cmd == WRITE) {
        switch_delay = ssdconf->channel_switch_delay_w;
    } else {
        return 0;
    }

    start = get_usec();
    diff = start - ssd->old_channel_time;

#ifndef VSSIM_BENCH
#ifdef DEL_QEMU_OVERHEAD
    if (diff < switch_delay) {
        SSD_UPDATE_QEMU_OVERHEAD(s, switch_delay-diff);
    }
    diff = start - ssd->old_channel_time;
#endif
#endif

#if 0
    if (diff < switch_delay) {
        while (diff < switch_delay) {
            diff = get_usec() - ssd->old_channel_time;
        }
    }
#endif
    end = get_usec();

    return end-start;
}

int SSD_REG_WRITE_DELAY(IDEState *s, int reg)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);
    int ret = FAIL;

    int64_t start = 0;
    int64_t	end = 0;
    int64_t diff = 0;
    int64_t time_stamp = ssd->reg_io_time[reg];

    if (time_stamp == -1)
        return ret;

    /* Reg Write Delay */
    start = get_usec();
    diff = start - time_stamp;

#ifndef VSSIM_BENCH
#ifdef DEL_QEMU_OVERHEAD
    if (diff < ssdconf->reg_write_delay) { /* Coperd: last ops is not finished yet */
        SSD_UPDATE_QEMU_OVERHEAD(s, ssdconf->reg_write_delay-diff);
    }
    diff = start - ssd->reg_io_time[reg];
#endif
#endif

#if 0
    if (diff < ssdconf->reg_write_delay) {
        while (diff < ssdconf->reg_write_delay) {
            diff = get_usec() - time_stamp;
        }
        ret = SUCCESS;
    }
#endif
    ret = SUCCESS;
    end = get_usec();

    /* Send Delay Info To Perf Checker */
    SEND_TO_PERF_CHECKER(s, ssd->reg_io_type[reg], end-start, CH_OP);

    /* Update Time Stamp Struct */
    ssd->reg_io_time[reg] = -1;

    return ret;
}

int SSD_REG_READ_DELAY(IDEState *s, int reg)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

	int ret = 0;
	int64_t start = 0;
	int64_t end = 0;
	int64_t diff = 0;
	int64_t time_stamp = ssd->reg_io_time[reg];

	if (time_stamp == -1)
		return 0;

	/* Reg Read Delay */
	start = get_usec();
    /* Coperd: we should wait <diff> time here */
	diff = start - time_stamp;

#ifndef VSSIM_BENCH
  #ifdef DEL_QEMU_OVERHEAD
	if (diff < ssdconf->reg_read_delay) {
		SSD_UPDATE_QEMU_OVERHEAD(s, ssdconf->reg_read_delay-diff);
	}
	diff = start - ssd->reg_io_time[reg];
  #endif
#endif

#if 0
	if (diff < ssdconf->reg_read_delay) {
		while (diff < ssdconf->reg_read_delay) {
			diff = get_usec() - time_stamp;
		}
		ret = SUCCESS;
	}
#endif
    ret = SUCCESS;
	end = get_usec();


	/* Send Delay Info To Perf Checker */
	SEND_TO_PERF_CHECKER(s, ssd->reg_io_type[reg], end-start, CH_OP);

    /* Coperd: this sub-read (NAND) operation has finished, update io_request info */
	SSD_UPDATE_IO_REQUEST(s, reg);
	
	/* Update Time Stamp Struct */
	ssd->reg_io_time[reg] = -1;
	ssd->reg_io_cmd[reg] = NOOP;
	ssd->reg_io_type[reg] = NOOP;

	return ret;
}

int SSD_CELL_WRITE_DELAY(IDEState *s, int reg)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    int ret = 0;
    int64_t start = 0;
    int64_t end = 0;
    int64_t diff = 0;
    int64_t time_stamp = ssd->cell_io_time[reg];

    if (time_stamp == -1)
        return 0;

    /* Cell Write Delay */
    start = get_usec();
    diff = start - time_stamp + ssd->io_overhead[reg];

#ifndef VSSIM_BENCH
#ifdef DEL_QEMU_OVERHEAD
    if (diff < ssdconf->cell_program_delay) {
        SSD_UPDATE_QEMU_OVERHEAD(s, ssdconf->cell_program_delay-diff);
    }
    diff = start - ssd->cell_io_time[reg] + ssd->io_overhead[reg];
#endif
#endif


    if (diff < ssdconf->cell_program_delay) {
        ssd->init_diff_reg = diff;
#if 0
        while (diff < ssdconf->cell_program_delay) {
            diff = get_usec() - time_stamp + ssd->io_overhead[reg];
        }
#endif
        ret = 1;
    }
    end = get_usec();

    /* Send Delay Info To Perf Checker */
    SEND_TO_PERF_CHECKER(s, ssd->reg_io_type[reg], end-start, REG_OP);
    SSD_UPDATE_IO_REQUEST(s, reg);

    /* Update Time Stamp Struct */
    ssd->cell_io_time[reg] = -1;
    ssd->reg_io_cmd[reg] = NOOP;
    ssd->reg_io_type[reg] = NOOP;

    /* Update IO Overhead */
    ssd->io_overhead[reg] = 0;

    return ret;
}

int SSD_CELL_READ_DELAY(IDEState *s, int reg)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    int ret = FAIL;
    int64_t start = 0;
    int64_t end = 0;
    int64_t diff = 0;
    int64_t time_stamp = ssd->cell_io_time[reg];

    int64_t REG_DELAY = ssdconf->cell_read_delay;

    if (time_stamp == -1)
        return ret;

    /* Cell Read Delay */
    start = get_usec();
    diff = start - time_stamp + ssd->io_overhead[reg];

#ifndef VSSIM_BENCH
#ifdef DEL_QEMU_OVERHEAD
    if (diff < REG_DELAY) {
        SSD_UPDATE_QEMU_OVERHEAD(s, REG_DELAY-diff);
    }
    diff = start - ssd->cell_io_time[reg] + ssd->io_overhead[reg];
#endif
#endif

    if (diff < REG_DELAY) {
        ssd->init_diff_reg = diff;
#if 0
        while (diff < REG_DELAY) {
            diff = get_usec() - time_stamp + ssd->io_overhead[reg];
        }
#endif
        ret = SUCCESS;

    }
    end = get_usec();

    /* Send Delay Info To Perf Checker */
    SEND_TO_PERF_CHECKER(s, ssd->reg_io_type[reg], end-start, REG_OP);

    /* Update Time Stamp Struct */
    ssd->cell_io_time[reg] = -1;

    /* Update IO Overhead */
    ssd->io_overhead[reg] = 0;

    return ret;
}

int SSD_BLOCK_ERASE_DELAY(IDEState *s, int reg)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    int ret = FAIL;
    int64_t start = 0;
    int64_t end = 0;
    int64_t diff;
    int64_t time_stamp = ssd->cell_io_time[reg];

    if (time_stamp == -1)
        return ret;

    /* Block Erase Delay */
    start = get_usec();
    diff = get_usec() - ssd->cell_io_time[reg];
#if 0
    if (diff < ssdconf->block_erase_delay) {
        while (diff < ssdconf->block_erase_delay) {
            diff = get_usec() - time_stamp;
        }
        ret = SUCCESS;
    }
#endif
    ret = SUCCESS;
    end = get_usec();

    /* Send Delay Info to Perf Checker */
    SEND_TO_PERF_CHECKER(s, ssd->reg_io_type[reg], end-start, REG_OP);

    /* Update IO Overhead */
    ssd->cell_io_time[reg] = -1;
    ssd->reg_io_cmd[reg] = NOOP;
    ssd->reg_io_type[reg] = NOOP;

    return ret;
}

int64_t SSD_GET_CH_ACCESS_TIME_FOR_READ(IDEState *s, int channel, int reg)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    int i, j;
    int r_num;
    int64_t latest_time = ssd->cell_io_time[reg] + ssdconf->cell_read_delay;
    int64_t temp_time = 0;

    for (i = 0; i < ssdconf->way_nb; i++) {
        r_num = channel * ssdconf->planes_per_flash + 
            i * ssdconf->channel_nb * ssdconf->planes_per_flash; 

        for (j = 0; j < ssdconf->planes_per_flash; j++) {
            temp_time = 0;

            if (ssd->reg_io_cmd[r_num] == READ) {
                temp_time = ssd->reg_io_time[r_num] + ssdconf->reg_read_delay;	
            } else if (ssd->reg_io_cmd[r_num] == WRITE) {
                temp_time = ssd->reg_io_time[r_num] + ssdconf->reg_write_delay;
            }

            if (temp_time > latest_time) {
                latest_time = temp_time;
            }
            r_num++;
        }
    }

    return latest_time;
}

void SSD_UPDATE_CH_ACCESS_TIME(IDEState *s, int channel, int64_t currtime)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    int i, j;
    int r_num;

    for (i = 0; i < ssdconf->way_nb; i++) {
        r_num = channel * ssdconf->planes_per_flash + 
            i * ssdconf->channel_nb * ssdconf->planes_per_flash; 
        for (j = 0; j < ssdconf->planes_per_flash; j++) {
            if (ssd->reg_io_cmd[r_num] == READ && 
                    ssd->reg_io_time[r_num] > currtime ) {
                ssd->reg_io_time[r_num] += ssdconf->reg_write_delay;
            }
            r_num++;	
        }
    }
}

void SSD_UPDATE_IO_REQUEST(IDEState *s, int reg)
{
    SSDState *ssd = &(s->ssd);
    //SSDConf *ssdconf = &(ssd->param);

    int64_t currtime = get_usec();
    if (ssd->init_diff_reg != 0) {
        ssd->io_update_overhead = UPDATE_IO_REQUEST(s, ssd->access_nb[reg][0], 
                ssd->access_nb[reg][1], currtime, UPDATE_END_TIME);
        SSD_UPDATE_IO_OVERHEAD(s, reg, ssd->io_update_overhead);
        ssd->access_nb[reg][0] = -1;
    } else {
        ssd->io_update_overhead = UPDATE_IO_REQUEST(s, ssd->access_nb[reg][0], 
                ssd->access_nb[reg][1], 0, UPDATE_END_TIME);
        SSD_UPDATE_IO_OVERHEAD(s, reg, ssd->io_update_overhead);
        ssd->access_nb[reg][0] = -1;
    }
}

void SSD_REMAIN_IO_DELAY(IDEState *s, int reg)
{
	SSD_REG_ACCESS(s, reg);
}

#ifndef VSSIM_BENCH
void SSD_UPDATE_QEMU_OVERHEAD(IDEState *s, int64_t delay)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);
    int i;
    int p_num = ssdconf->flash_nb * ssdconf->planes_per_flash;
    int64_t diff = delay;

    if (qemu_overhead == 0) {
        return;
    } else {
        if (diff > qemu_overhead) {
            diff = qemu_overhead;
        }
    }

    ssd->old_channel_time -= diff;
    for (i = 0; i < p_num; i++) {
        ssd->cell_io_time[i] -= diff;
        ssd->reg_io_time[i] -= diff;
    }
    qemu_overhead -= diff;
}
#endif

void SSD_PRINT_STAMP(IDEState *s)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    int i, j, k;
    int op;
    int r_num;

    //	FILE* fp_temp = fopen("./data/stamp.txt","a");

    r_num = 0;
    for (i = 0; i < ssdconf->channel_nb; i++) {
        for (j = 0; j < ssdconf->way_nb; j++) {
            r_num = i * ssdconf->planes_per_flash + 
                j * ssdconf->channel_nb * ssdconf->planes_per_flash; 
            for (k = 0; k < ssdconf->planes_per_flash; k++) {

                op = ssd->reg_io_type[r_num];
                if (op == NOOP)
                    printf("[      ]");
                else if (op == READ)
                    printf("[READ%2d]", ssd->access_nb[r_num][1]);
                else if (op == WRITE)
                    printf("[PROG%2d]", ssd->access_nb[r_num][1]);
                else if (op == SEQ_WRITE)
                    printf("[SEQW%2d]", ssd->access_nb[r_num][1]);
                else if (op == RAN_WRITE)
                    printf("[RNDW%2d]", ssd->access_nb[r_num][1]);
                else if (op == RAN_COLD_WRITE)
                    printf("[RNCW%2d]", ssd->access_nb[r_num][1]);
                else if (op == RAN_HOT_WRITE)
                    printf("[RNHW%2d]", ssd->access_nb[r_num][1]);
                else if (op == SEQ_MERGE_WRITE)
                    printf("[SMGW%2d]", ssd->access_nb[r_num][1]);
                else if (op == RAN_MERGE_WRITE)
                    printf("[RMGW%2d]", ssd->access_nb[r_num][1]);
                else if (op == SEQ_MERGE_READ)
                    printf("[SMGR%2d]", ssd->access_nb[r_num][1]);
                else if (op == RAN_MERGE_READ)
                    printf("[RMGR%2d]", ssd->access_nb[r_num][1]);
                else
                    printf("[ %d ]",op);

                r_num ++;
            }
        }
        printf("\t");
    }
    printf("\n\n");

    //	fclose(fp_temp);
}
