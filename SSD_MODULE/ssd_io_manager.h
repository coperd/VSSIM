// File: ssd_io_manager.h
// Date: 2014. 12. 03.
// Author: Jinsoo Yoo (jedisty@hanyang.ac.kr)
// Copyright(c)2014
// Hanyang University, Seoul, Korea
// Embedded Software Systems Laboratory. All right reserved

#ifndef _SSD_IO_MANAGER_H
#define _SSD_IO_MANAGER_H

#ifndef VSSIM_BENCH
#include "ssd_util.h"
#endif

//extern int old_channel_nb;
//extern int64_t io_alloc_overhead;
//extern int64_t io_update_overhead;

/* Get Current time in micro second */
int64_t get_usec(void);

/* Initialize SSD Module */
int SSD_IO_INIT(IDEState *s);

/* GET IO from FTL */
int SSD_PAGE_READ(IDEState *s, unsigned int flash_nb, unsigned int block_nb, unsigned int page_nb, int offset, int type, int io_page_nb);
int SSD_PAGE_WRITE(IDEState *s, unsigned int flash_nb, unsigned int block_nb, unsigned int page_nb, int offset, int type, int io_page_nb);
int SSD_BLOCK_ERASE(IDEState *s, unsigned int flash_nb, unsigned int block_nb);
int SSD_PAGE_PARTIAL_WRITE(IDEState *s, unsigned int old_flash_nb, unsigned int old_block_nb, unsigned int old_page_nb, unsigned new_flash_nb, unsigned int new_block_nb, unsigned int new_page_nb, int offset, int type, int io_page_nb);

/* Channel Access Delay */
int SSD_CH_ENABLE(IDEState *s, int channel);

/* Flash or Register Access */
int SSD_FLASH_ACCESS(IDEState *s, unsigned int flash_nb, int reg);
int SSD_REG_ACCESS(IDEState *s, int reg);

/* Channel Delay */
int64_t SSD_CH_SWITCH_DELAY(IDEState *s, int channel);

/* Register Delay */
int SSD_REG_WRITE_DELAY(IDEState *s, int reg);
int SSD_REG_READ_DELAY(IDEState *s, int reg);

/* Cell Delay */
int SSD_CELL_WRITE_DELAY(IDEState *s, int reg);
int SSD_CELL_READ_DELAY(IDEState *s, int reg);

/* Erase Delay */
int SSD_BLOCK_ERASE_DELAY(IDEState *s, int reg);

/* Mark Time Stamp */
int SSD_CH_RECORD(IDEState *s, int channel, int cmd, int offset, int ret);
int SSD_REG_RECORD(IDEState *s, int reg, int cmd, int type, int offset, int channel);
int SSD_CELL_RECORD(IDEState *s, int reg, int cmd);

/* Check Read Operation in the Same Channel  */
int SSD_CH_ACCESS(IDEState *s, int channel);
int64_t SSD_GET_CH_ACCESS_TIME_FOR_READ(IDEState *s, int channel, int reg);
void SSD_UPDATE_CH_ACCESS_TIME(IDEState *s, int channel, int64_t current_time);

/* Correction Delay */
void SSD_UPDATE_IO_REQUEST(IDEState *s, int reg);
void SSD_UPDATE_IO_OVERHEAD(IDEState *s, int reg, int64_t overhead_time);
void SSD_REMAIN_IO_DELAY(IDEState *s, int reg);
void SSD_UPDATE_QEMU_OVERHEAD(IDEState *s, int64_t delay);

/* SSD Module Debugging */
void SSD_PRINT_STAMP(IDEState *s);

#endif
