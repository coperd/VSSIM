#ifndef __IDE_H
#define __IDE_H

#include "hw.h"
#include "pc.h"
#include "pci.h"
#include "scsi-disk.h"
#include "pcmcia.h"
#include "block.h"
#include "block_int.h"
#include "qemu-timer.h"
#include "sysemu.h"
#include "ppc_mac.h"
#include "mac_dbdma.h"
#include "sh.h"
#include "dma.h"
#include "qemu-thread.h"

/* Coperd: old common.h include */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <stdint.h>
#include <sys/time.h>
#include <pthread.h>


#define SSD_EMULATION
//#define RANDOM_GC
/* Enable EBUSY mechanism here */
//#define DEBUG_ERR

/* FTL */
/* VSSIM Function */
//#define MONITOR_ON /* comment out this sentence to disable monitor */


/* Page Mapping FTL */
#define PAGE_MAP

#ifdef PAGE_MAP
	#define GC_ON			/* Garbage Collection for PAGE MAP */
	#define GC_TRIGGER_OVERALL
	//#define GC_VICTIM_OVERALL
	//#define WRITE_NOPARAL
	//#define FTL_MAP_CACHE		/* FTL MAP Cache for PAGE MAP */
#endif

/* Block Mapping FTL */
#ifdef BLOCK_MAP
	#define GC_ON
	#define GC_TRIGGER_OVERALL
#endif

/* Hybrid Mapping FTL */
#ifdef FAST_FTL
	//#define FTL_FAST1
	//#define LAST_SEQ_COND
	//#define SSD_SEQ_W_VALIDATION
#endif

#ifdef LAST_FTL
	//#define FAST_SEQ_COND
#endif

/* Disaggregated Mapping */
#ifdef DA_MAP
	#define BM_START_SECTOR_NB 8388608 // 8388608: 4GByte for PM, Remains for BM
	#define GC_ON
	#define GC_TRIGGER_OVERALL
#endif

//#define REMAIN_IO_DELAY	/* Remain delay for validation */
//#define O_DIRECT_VSSIM	/* O_DIRECT IO mode */

/* VSSIM Benchmark*/
#ifndef VSSIM_BENCH
  //#define DEL_QEMU_OVERHEAD
  //#define FIRM_IO_BUFFER	/* SSD Read/Write Buffer ON */
  //#define SSD_THREAD		/* Enable SSD thread & SSD Read/Write Buffer */
  //#define SSD_THREAD_MODE_1
  //#define SSD_THREAD_MODE_2	/* Pending WB Flush until WB is full */
#endif


/*************************
  Define Global Variables 
 *************************/

/* Function Return */
#define SUCCESS		            1
#define FAIL		            0

/* Write Type */
#if defined FAST_FTL || defined LAST_FTL
#define NEW_SEQ_WRITE           20
#define CONT_SEQ_WRITE          21
#define NEW_RAN_WRITE           22
#define HOT_RAN_WRITE	        23
#define COLD_RAN_WRITE	        24
#endif

/* Block Type */
#define EMPTY_BLOCK             30
#define SEQ_BLOCK               31
#define EMPTY_SEQ_BLOCK         32
#define RAN_BLOCK               33
#define EMPTY_RAN_BLOCK         34
#define RAN_COLD_BLOCK		    35
#define EMPTY_RAN_COLD_BLOCK	36
#define	RAN_HOT_BLOCK		    37
#define	EMPTY_RAN_HOT_BLOCK	    38
#define DATA_BLOCK              39
#define EMPTY_DATA_BLOCK        40

/* GC Copy Valid Page Type */
#define VICTIM_OVERALL	        41
#define VICTIM_INCHIP	        42
#define VICTIM_NOPARAL	        43

/* Page Type */
#define VALID		            50
#define INVALID		            51

/* Caller Type for Hybrid FTL */
#if defined FAST_FTL || defined LAST_FTL
#define INIT                    60
#define SEQ_MERGE               61
#define RAN_MERGE               62
#endif

/* Random Log Block Type */
#ifdef LAST_FTL
#define COLD_RAN                70
#define HOT_RAN                 71
#endif

/* Perf Checker Calloc Type */
#define CH_OP		            80
#define REG_OP		            81
#define LATENCY_OP	            82

#define CHANNEL_IS_EMPTY        700
#define CHANNEL_IS_WRITE        701
#define CHANNEL_IS_READ         702
#define CHANNEL_IS_ERASE        703

#define REG_IS_EMPTY 		    705
#define REG_IS_WRITE 		    706
#define REG_IS_READ  		    707
#define REG_IS_ERASE 		    708

#define NOOP			        800
#define READ  			        801
#define WRITE			        802
#define ERASE 			        803
#define GC_READ			        804
#define GC_WRITE		        805
#define SEQ_WRITE		        806
#define RAN_WRITE		        807
#define RAN_COLD_WRITE		    808
#define RAN_HOT_WRITE		    809
#define SEQ_MERGE_READ		    810
#define RAN_MERGE_READ		    811
#define SEQ_MERGE_WRITE		    812
#define RAN_MERGE_WRITE		    813
#define RAN_COLD_MERGE_READ	    814
#define RAN_HOT_MERGE_READ	    815
#define RAN_COLD_MERGE_WRITE    816
#define RAN_HOT_MERGE_WRITE	    817
#define MAP_READ		        818
#define MAP_WRITE		        819

#define UPDATE_START_TIME	    900
#define UPDATE_END_TIME		    901
#define UPDATE_GC_START_TIME	902
#define UPDATE_GC_END_TIME	    903

//#define EBUSY (-1024)

/* VSSIM Function Debug */
#define MNT_DEBUG			// MONITOR Debugging
//#define WRITE_BUFFER_DEBUG		// WRITE BUFFER Debugging 
//#define FTL_CACHE_DEBUG		// CACHE Debugging
//#define SSD_THREAD_DEBUG
//#define IO_LATENCY_DEBUG		// ??

/* Workload */
//#define GET_FTL_WORKLOAD
//#define GET_FIRM_WORKLOAD
//#define FTL_GET_WRITE_WORKLOAD

/* FTL Debugging */
//#define FTL_DEBUG
//#define FTL_PERF_DEBUG
//#define FTL_IO_LATENCY

/* FTL Perf Debug */
//#define PERF_DEBUG1
//#define PERF_DEBUG2
//#define PERF_DEBUG4

/* SSD Debugging */
#define SSD_DEBUG
//#define SSD_SYNC

/* FIRMWARE Debugging */
//#define FIRM_IO_BUF_DEBUG
//#define GET_QUEUE_DEPTH


/* Coperd: move from ftl_cache.h */
#define CACHE_MAP_SIZE	(PASE_SIZE * CACHE_IDX_SIZE)

#define MAP	        0
#define INV_MAP	    1


typedef struct map_data
{
	uint32_t ppn;
	void* data;
	struct map_data* prev;
	struct map_data* next;
}map_data;

typedef struct cache_idx_entry
{
	uint32_t map_num	:29;
	uint32_t clock_bit	:1;
	uint32_t map_type	:1;	// map (0), inv_map(1)
	uint32_t update_bit	:1;
	void* data;
}cache_idx_entry;

typedef struct map_state_entry
{
	int32_t ppn;
	uint32_t is_cached; // cached (1) not cached(0)
	cache_idx_entry* cache_entry;
}map_state_entry;


/* Coperd: move from firm_buffer_manager.h */
typedef struct event_queue_entry
{
	int io_type;
	int valid;
	int32_t sector_num;
	unsigned int length;
	void *buf;
    int has_error;
	struct event_queue_entry *next;
}event_queue_entry;

typedef struct event_queue
{
	int entry_nb;
	event_queue_entry *head;
	event_queue_entry *tail;
}event_queue;


/* 
 * Coperd: this structure is used to record the status of each io request,
 * specifically the start_time and end_time arrays maintains each page operations'
 * timing info 
 */
typedef struct io_request
{
	unsigned int request_nb;       /* Coperd: request NO. */
	int	request_type	: 16;      /* Coperd: request type: READ/WRITE */
	int	request_size	: 16;      /* Coperd: request size in number of pages */
	int	start_count	: 16;          /* Coperd: number of NAND requests that have started */
	int	end_count	: 16;          /* Coperd: number of NAND requests that have finished */
	int64_t	*start_time;           /* Coperd: record the start time of each page operation included in this io request */
	int64_t	*end_time;             /* Coperd: similar to start_time, record the ending time */
	struct io_request* next;
}io_request;


typedef struct block_state_entry
{
	int valid_page_nb;
	int type;
	unsigned int erase_count;
	char *valid_array;

}block_state_entry;

typedef struct empty_block_root
{
	struct empty_block_entry *head;
	struct empty_block_entry *tail;
	unsigned int empty_block_nb;
}empty_block_root;

typedef struct empty_block_entry
{
	unsigned int phy_flash_nb;
	unsigned int phy_block_nb;
	unsigned int curr_phy_page_nb;
	struct empty_block_entry *next;

}empty_block_entry;

typedef struct victim_block_root
{
	struct victim_block_entry *head;
	struct victim_block_entry *tail;
	unsigned int victim_block_nb;
}victim_block_root;

typedef struct victim_block_entry
{
	unsigned int phy_flash_num;
	unsigned int phy_block_num;
	struct victim_block_entry *prev;
	struct victim_block_entry *next;
}victim_block_entry;

typedef struct SSDPerf
{
    /* Average IO Time */
    double avg_write_delay;
    double total_write_count;
    double total_write_delay;

    double avg_read_delay;
    double total_read_count;
    double total_read_delay;

    double avg_gc_write_delay;
    double total_gc_write_count;
    double total_gc_write_delay;

    double avg_gc_read_delay;
    double total_gc_read_count;
    double total_gc_read_delay;

    double avg_seq_write_delay;
    double total_seq_write_count;
    double total_seq_write_delay;

    double avg_ran_write_delay;
    double total_ran_write_count;
    double total_ran_write_delay;

    double avg_ran_cold_write_delay;
    double total_ran_cold_write_count;
    double total_ran_cold_write_delay;

    double avg_ran_hot_write_delay;
    double total_ran_hot_write_count;
    double total_ran_hot_write_delay;

    double avg_seq_merge_read_delay;
    double total_seq_merge_read_count;
    double total_seq_merge_read_delay;

    double avg_ran_merge_read_delay;
    double total_ran_merge_read_count;
    double total_ran_merge_read_delay;

    double avg_seq_merge_write_delay;
    double total_seq_merge_write_count;
    double total_seq_merge_write_delay;

    double avg_ran_merge_write_delay;
    double total_ran_merge_write_count;
    double total_ran_merge_write_delay;

    double avg_ran_cold_merge_write_delay;
    double total_ran_cold_merge_write_count;
    double total_ran_cold_merge_write_delay;

    double avg_ran_hot_merge_write_delay;
    double total_ran_hot_merge_write_count;
    double total_ran_hot_merge_write_delay;

    double read_latency_count;
    double write_latency_count;

    double avg_read_latency;
    double avg_write_latency;

    int64_t written_page_nb;

}SSDPerf;

struct IDEState;
struct SSDState;

typedef void EndTransferFunc(struct IDEState *);

typedef struct SSDConf
{
    int sector_size;
    int page_size;
    int64_t sector_nb;
    int page_nb;
    int flash_nb;
    int block_nb;
    int channel_nb;
    int planes_per_flash;
    int sectors_per_page;
    int pages_per_flash;
    int64_t pages_in_ssd;

    int way_nb;
    int ovp;

    int data_block_nb;
    int64_t block_mapping_entry_nb;

#ifdef PAGE_MAP
    int64_t page_mapping_entry_nb;
#endif

#if defined PAGE_MAP || defined BLOCK_MAP
    int64_t each_empty_table_entry_nb;
    int empty_table_entry_nb;
    int victim_table_entry_nb;
#endif

#if defined FAST_FTL || defined LAST_FTL
    int log_rand_block_nb;
    int log_sqe_block_nb;
    int64_t data_mapping_entry_nb;
    int64_t ran_mapping_entry_nb;
    int64_t seq_mapping_entry_nb;
    int64_t ran_log_mapping_entry_nb;
    int64_t each_empty_block_entry_nb;
    int empty_block_table_nb;
#endif

#ifdef DA_MAP
    int64_t da_page_mapping_entry_nb;
    int64_t da_block_mapping_entry_nb;
    int64_t each_empty_table_entry_nb;
    int empty_table_entry_nb;
    int victim_table_entry_nb;
#endif

    /* NAND Flash Delay */
    int reg_write_delay;
    int cell_read_delay;
    int cell_program_delay;
    int reg_read_delay;
    int block_erase_delay;
    int channel_switch_delay_w;
    int channel_switch_delay_r;

    int dsm_trim_enable;
    int io_parallelism;

    /* Garbage Collection */
#if defined PAGE_MAP || defined BLOCK_MAP || defined DA_MAP
    double gc_threshold;
    double gc_threshold_hard;
    int gc_threshold_block_nb;
    int gc_threshold_block_nb_hard;
    int gc_threshold_block_nb_each;
    int gc_victim_nb;
#endif

    /* Write Buffer */
    uint32_t write_buffer_frame_nb;
    uint32_t read_buffer_frame_nb;

    /* Map Cache */
#if defined FTL_MAP_CACHE || defined Polymorphic_FTL
    int cache_idx_size;
#endif

#ifdef FTL_MAP_CACHE
    uint32_t map_entry_size;
    uint32_t map_entries_per_page;
    uint32_t map_entry_nb;
#endif

    /* Host Event Queue */
#ifdef HOST_QUEUE
    uint32_t host_queue_entry_nb;
#endif

    /* Rand Log Block Write Policy */
#if defined FAST_FTL || defined LAST_FTL
    int paral_degreee;
    int paral_count;
#endif

    /* LAST FTL */
#ifdef LAST_FTL
    int hot_page_nb_threshold;
    int seq_threshold;
#endif

    /* Polymorphic FTL */
#ifdef Polymorphic_FTL
    int phy_spare_size;
    int64_t nr_phy_blocks;
    int64_t nr_phy_pages;
    int64_t nr_phy_sectors;

    int nr_reserved_phy_super_blocks;
    int nr_reserved_phy_blocks;
    int nr_reserved_phy_pages;
#endif

}SSDConf;

typedef struct SSDState
{
    int gc_cnt;
    int64_t gc_time;
    int gc_fail_cnt;
    int read_cnt;
    int write_cnt;
    int nwarmup;

    //IDEState *s;
    char *name; 

#ifdef FTL_GET_WRITE_WORKLOAD
    FILE* fp_write_workload;
#endif
#ifdef FTL_IO_LATENCY
    FILE* fp_ftl_w;
    FILE* fp_ftl_r;
#endif

    int g_init;

    char ssd_version[8];
    char ssd_date[16];

    int64_t io_update_overhead;
    int64_t init_diff_reg;
    int64_t io_alloc_overhead;


    struct SSDConf param;
    char conf_filename[1024];

    struct SSDPerf perf;

    block_state_entry *block_state_table;
    char bst_filename[1024];

    empty_block_root *empty_block_list;
    char ebl_filename[1024];
    int64_t total_empty_block_nb;
    unsigned int empty_block_table_index;

    int32_t *inverse_mapping_table;
    char im_filename[1024];

    int32_t *mapping_table;
    char mt_filename[1024];

    char *valid_array;
    char va_filename[1024];

    victim_block_root *victim_block_list;
    char vbl_filename[1024];
    int64_t total_victim_block_nb;

    char cache_map_data_filename[1024];
    struct map_data *cache_map_data_start;
    struct map_data *cache_map_data_end;
    uint32_t cache_map_data_nb;
    cache_idx_entry *cache_idx_table;
    map_state_entry *map_state_table;
    uint32_t clock_hand;

    void *write_buffer;
    void *write_buffer_end;
    void *read_buffer;
    void *read_buffer_end;

    char *wb_valid_array;

    void *ftl_write_ptr;
    void *sata_write_ptr;
    void *write_limit_ptr;

    /* Coperd: pointers for read buffer */
    void *ftl_read_ptr;
    void *sata_read_ptr;
    void *read_limit_ptr;

    event_queue *e_queue;
    event_queue *c_e_queue;
    event_queue_entry *last_read_entry;

    int empty_write_buffer_frame;
    int empty_read_buffer_frame;
#ifdef SSD_THREAD
    int r_queue_full;
    int w_queue_full;
    pthread_t ssd_thread_id;
    pthread_cond_t eq_ready;
    pthread_mutex_t eq_lock;
    pthread_mutex_t cq_lock;
#endif

    /* some NAND running info */
    unsigned int io_request_nb;
    unsigned int io_request_seq_nb;
    struct io_request *io_request_start;
    struct io_request *io_request_end;

    int *reg_io_cmd;
    int *reg_io_type;


    int64_t *reg_io_time;
    int64_t *cell_io_time;

    int **access_nb;
    int64_t *io_overhead;

    int old_channel_num;
    int old_channel_cmd;
    int64_t old_channel_time;

    double ssd_util;

}SSDState;

/* NOTE: IDEState represents in fact one drive */
typedef struct IDEState {
    int nb_gc_eios;

#ifdef DEBUG_ERR
    int nerr_dma;
    int nerr_pio;
    int nsuc_dma;
    int nsuc_pio;
#endif

    /* ide config */
    int is_cdrom;
    int is_cf;
    int cylinders, heads, sectors;
    int64_t nb_sectors;
    int mult_sectors;
    int identify_set;
    uint16_t identify_data[256];
    qemu_irq irq;
    PCIDevice *pci_dev;
    struct BMDMAState *bmdma;
    int drive_serial;
    char drive_serial_str[21];
    /* ide regs */
    uint8_t feature;  /* Coperd: feature register */
    uint8_t error;    /* Coperd: error register */
    uint32_t nsector; 
    uint8_t sector;   /* Coperd: sector count register, serve as one parameter of command */
    uint8_t lcyl;     /* Coperd: LBA Low register, one parameter of command */
    uint8_t hcyl;     /* Coperd: LBA High register, one parameter of command */

    int hob;
    /* other part of tf for lba48 support */
    uint8_t hob_feature;
    uint8_t hob_nsector;
    uint8_t hob_sector;
    uint8_t hob_lcyl;
    uint8_t hob_hcyl;

    uint8_t select; /* Coperd: this is used to select which drive (master or slave) this cmd is for */
    uint8_t status; /* Coperd: status register */

    /* 0x3f6 command, only meaningful for drive 0 */
    uint8_t cmd;
    /* set for lba48 access */
    uint8_t lba48;
    /* depends on bit 4 in select, only meaningful for drive 0 */
    struct IDEState *cur_drive;
    BlockDriverState *bs;
    /* ATAPI specific */
    uint8_t sense_key;
    uint8_t asc;
    uint8_t cdrom_changed;
    int packet_transfer_size;
    int elementary_transfer_size;
    int io_buffer_index;
    int lba;
    int cd_sector_size;
    int atapi_dma; /* true if dma is requested for the packet cmd */
    /* ATA DMA state */
    int io_buffer_size;
    QEMUSGList sg;
    /* PIO transfer handling */
    int req_nb_sectors; /* number of sectors per interrupt */
    EndTransferFunc *end_transfer_func;
    uint8_t *data_ptr;
    uint8_t *data_end;
    uint8_t *io_buffer;
    QEMUTimer *sector_write_timer; /* only used for win2k install hack */
    uint32_t irq_count; /* counts IRQs when using win2k install hack */
    /* CF-ATA extended error */
    uint8_t ext_error;
    /* CF-ATA metadata storage */
    uint32_t mdata_size;
    uint8_t *mdata_storage;
    int media_changed; /* Coperd: set after r/w using DMA, why??? */
    /* for pmac */
    int is_read;

    /* Coperd: SSD structure put here */
    SSDState ssd;
} IDEState;


void ide_rw_error(struct IDEState *s);

extern IDEState *vm_ide[4];
extern int ide_idx;

#endif
