// File: ftl_mapping_manager.c
// Date: 2014. 12. 03.
// Author: Jinsoo Yoo (jedisty@hanyang.ac.kr)
// Copyright(c)2014
// Hanyang University, Seoul, Korea
// Embedded Software Systems Laboratory. All right reserved

#include "ftl_mapping_manager.h"
#include "ftl_inverse_mapping_manager.h"
#include "ssd_util.h"

//#define DEBUG_FTL

//int32_t* mapping_table;
void* block_table_start;

void INIT_MAPPING_TABLE(IDEState *s)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

#ifdef DEBUG_FTL
    printf("[%s] Mapping table initialization func starts...\n", __FUNCTION__);
#endif
    /* Allocation Memory for Mapping Table */
    ssd->mapping_table = qemu_mallocz(ssdconf->page_mapping_entry_nb*sizeof(int32_t));

    /* Initialization Mapping Table */
    set_ssd_struct_filename(s, ssd->mt_filename, "_mapping_table.dat");	
    /* If mapping_table.dat file exists */
    FILE *fp = fopen(ssd->mt_filename, "r");
    if (fp != NULL) {
#ifdef DEBUG_FTL
        printf("loading mapping table from SSD...\n");
#endif
        fread(ssd->mapping_table, sizeof(int32_t), ssdconf->page_mapping_entry_nb, fp);
    } else {	
        int i;	
        for (i = 0; i < ssdconf->page_mapping_entry_nb; i++) {
            ssd->mapping_table[i] = -1;
        }
    }
}

void TERM_MAPPING_TABLE(IDEState *s)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);


    FILE *fp = fopen(ssd->mt_filename, "w");
    if (fp == NULL) {
        printf("ERROR[%s] File(%s) open fail\n", __func__, ssd->mt_filename);
        return;
    }

#ifdef DEBUG_FTL
    printf("[%s] saving mapping table to SSD...\n", __FUNCTION__);
#endif
    /* Write the mapping table to file */
    fwrite(ssd->mapping_table, sizeof(int32_t), ssdconf->page_mapping_entry_nb, fp);

    /* Free memory for mapping table */
    free(ssd->mapping_table);
}

int32_t GET_MAPPING_INFO(IDEState *s, int32_t lpn)
{
    SSDState *ssd = &(s->ssd);
	int32_t ppn = ssd->mapping_table[lpn];

	return ppn;
}

int GET_NEW_PAGE(IDEState *s, int mode, int mapping_index, int32_t *ppn)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    empty_block_entry *curr_empty_block;

    curr_empty_block = GET_EMPTY_BLOCK(s, mode, mapping_index);

    /* If the flash memory has no empty block,
       Get empty block from the other flash memories */
    if (mode == VICTIM_INCHIP && curr_empty_block == NULL) {
        /* Try again */
        curr_empty_block = GET_EMPTY_BLOCK(s, VICTIM_OVERALL, mapping_index);
    }

    if (curr_empty_block == NULL) {
        printf("ERROR[%s] fail\n", __FUNCTION__);
        return FAIL;
    }

    *ppn = curr_empty_block->phy_flash_nb * ssdconf->block_nb * ssdconf->page_nb \
           + curr_empty_block->phy_block_nb * ssdconf->page_nb \
           + curr_empty_block->curr_phy_page_nb;

    curr_empty_block->curr_phy_page_nb += 1;

    return SUCCESS;
}

int UPDATE_OLD_PAGE_MAPPING(IDEState *s, int32_t lpn)
{
    int32_t old_ppn;

#ifdef FTL_MAP_CACHE
    old_ppn = CACHE_GET_PPN(s, lpn);
#else
    old_ppn = GET_MAPPING_INFO(s, lpn);
#endif

    if (old_ppn == -1) {
#ifdef FTL_DEBUG
        printf("[%s] New page \n", __FUNCTION__);
#endif
        return SUCCESS;
    } else {
        UPDATE_BLOCK_STATE_ENTRY(s, CALC_FLASH(s, old_ppn), 
                CALC_BLOCK(s, old_ppn), CALC_PAGE(s, old_ppn), INVALID);
        UPDATE_INVERSE_MAPPING(s, old_ppn, -1);
    }

    return SUCCESS;
}

int UPDATE_NEW_PAGE_MAPPING(IDEState *s, int32_t lpn, int32_t ppn)
{
    SSDState *ssd = &(s->ssd);

    /* Update Page Mapping Table */
#ifdef FTL_MAP_CACHE
    CACHE_UPDATE_PPN(s, lpn, ppn);
#else
    ssd->mapping_table[lpn] = ppn;
#endif

    /* Update Inverse Page Mapping Table */
    UPDATE_BLOCK_STATE_ENTRY(s, CALC_FLASH(s, ppn), CALC_BLOCK(s, ppn), 
            CALC_PAGE(s, ppn), VALID);
    UPDATE_BLOCK_STATE(s, CALC_FLASH(s, ppn), CALC_BLOCK(s, ppn), DATA_BLOCK);
    UPDATE_INVERSE_MAPPING(s, ppn, lpn);

    return SUCCESS;
}

unsigned int CALC_FLASH(IDEState *s, int32_t ppn)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    unsigned int flash_nb = (ppn/ssdconf->page_nb)/ssdconf->block_nb;

    if (flash_nb >= ssdconf->flash_nb) {
        printf("ERROR[%s] flash_nb %u\n", __FUNCTION__, flash_nb);
    }
    return flash_nb;
}

unsigned int CALC_BLOCK(IDEState *s, int32_t ppn)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    unsigned int block_nb = (ppn/ssdconf->page_nb) % ssdconf->block_nb;

    if (block_nb >= ssdconf->block_nb) {
        printf("ERROR[%s] block_nb %u\n", __FUNCTION__, block_nb);
    }
    return block_nb;
}

unsigned int CALC_PAGE(IDEState *s, int32_t ppn)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

	unsigned int page_nb = ppn % ssdconf->page_nb;

	return page_nb;
}
