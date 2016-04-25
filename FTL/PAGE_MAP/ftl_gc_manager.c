// File: ftl_gc_manager.c
// Date: 2014. 12. 03.
// Author: Jinsoo Yoo (jedisty@hanyang.ac.kr)
// Copyright(c)2014
// Hanyang University, Seoul, Korea
// Embedded Software Systems Laboratory. All right reserved

#include "ftl_gc_manager.h"
#include "ssd_io_manager.h"
#include "ftl_mapping_manager.h"
#include "ftl_inverse_mapping_manager.h"
#include "ssd_util.h"
#include "mytrace.h"

#define DEBUG_GC

void GC_CHECK(IDEState *s, unsigned int phy_flash_num, 
        unsigned int phy_block_num)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);
    int i, ret;

#ifdef GC_TRIGGER_OVERALL
    if (ssd->total_empty_block_nb < ssdconf->gc_threshold_block_nb) {
        //if (ssd->total_empty_block_nb <= ssdconf->flash_nb * ssdconf->planes_per_flash) {
        s->bs->gc_whole_endtime = get_timestamp();
        for (i = 0; i < ssdconf->gc_victim_nb; i++) {
            ret = GARBAGE_COLLECTION(s);
            if (ret == FAIL) {
                ssd->gc_fail_cnt++;
#ifdef DEBUG_GC
                mylog("%s: [[GC failed]] [%d]\n", get_ssd_name(s), ssd->gc_fail_cnt);
#endif
                break;
            } else {
                //s->bs->gc_whole_endtime += ssd->gc_time; // 100ms
                ssd->gc_cnt++;
#ifdef DEBUG_GC
                mylog("%s: GC[%d], blocking to %" PRId64 "\n", get_ssd_name(s), 
                        ssd->gc_cnt, s->bs->gc_whole_endtime);
#endif
            }


        }
    }
#else
#ifdef DEBUG_GC
    printf("[%s] start INCHIP GC ....\n", get_ssd_name(s));
#endif
    int plane_nb = phy_block_num % ssdconf->planes_per_flash;
    int mapping_index = plane_nb * ssdconf->flash_nb + phy_flash_num;
    empty_block_root *curr_root_entry = 
        (empty_block_root *)ssd->empty_block_list + mapping_index;

    if (curr_root_entry->empty_block_nb < ssdconf->gc_threshold_block_nb_each) {
        for (i = 0; i < ssdconf->gc_victim_nb; i++) {
            ret = GARBAGE_COLLECTION(s);
            if (ret == FAIL) {
                break;
            } else {
                //GC_timestamp += 12;
            }
        }
    }
#endif
    }

int GARBAGE_COLLECTION(IDEState *s)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    int i;
    int ret;
    int32_t lpn;
    int32_t old_ppn;
    int32_t new_ppn;

    unsigned int victim_phy_flash_num = ssdconf->flash_nb;
    unsigned int victim_phy_block_num = 0;

    char *valid_array;
    int copy_page_nb = 0;

    block_state_entry *b_s_entry;

    /* Coperd: by calling this function, we already know which block to do GC */
    ret = SELECT_VICTIM_BLOCK(s, &victim_phy_flash_num, &victim_phy_block_num);
    if (ret == FAIL) {
#ifdef DEBUG_GC
        printf("[%s] There is no available victim block\n", get_ssd_name(s));
#endif
        return FAIL;
    }

    int plane_nb = victim_phy_block_num % ssdconf->planes_per_flash;
    //int reg = victim_phy_flash_num * ssdconf->planes_per_flash + plane_nb;
    int mapping_index = plane_nb * ssdconf->flash_nb + victim_phy_flash_num;

    /* Coperd: actually real GC work starts here */
    b_s_entry = GET_BLOCK_STATE_ENTRY(s, victim_phy_flash_num, 
            victim_phy_block_num);
    valid_array = b_s_entry->valid_array;

    /* Coperd: copy valid pages out and update mapping table */
    /* Coperd: each valid_array's size is PAGE_NB */
    for (i = 0; i < ssdconf->page_nb; i++) { 
        if (valid_array[i] == 'V') {
#ifdef GC_VICTIM_OVERALL
            ret = GET_NEW_PAGE(s, VICTIM_OVERALL, EMPTY_TABLE_ENTRY_NB, &new_ppn);
#else
            ret = GET_NEW_PAGE(s, VICTIM_INCHIP, mapping_index, &new_ppn);
#endif
            if (ret == FAIL) {
#ifdef DEBUG_GC
                printf("[%s] get new page fail\n", get_ssd_name(s));
#endif
                return FAIL;
            }

            SSD_PAGE_READ(s, victim_phy_flash_num, 
                    victim_phy_block_num, i, i, GC_READ, -1);

            SSD_PAGE_WRITE(s, CALC_FLASH(s, new_ppn), CALC_BLOCK(s, new_ppn), 
                    CALC_PAGE(s, new_ppn), i, GC_WRITE, -1);

            old_ppn = victim_phy_flash_num * ssdconf->pages_per_flash + 
                victim_phy_block_num * ssdconf->page_nb + i;

            //			lpn = inverse_page_mapping_table[old_ppn];
#ifdef FTL_MAP_CACHE
            lpn = CACHE_GET_LPN(s, old_ppn);
#else
            lpn = GET_INVERSE_MAPPING_INFO(s, old_ppn);
#endif
            UPDATE_NEW_PAGE_MAPPING(s, lpn, new_ppn);

            copy_page_nb++;
        }
    }

    if (copy_page_nb != b_s_entry->valid_page_nb) {
        printf("ERROR[%s] The number of valid page is not correct\n", 
                __FUNCTION__);
        return FAIL;
    }

    SSD_BLOCK_ERASE(s, victim_phy_flash_num, victim_phy_block_num);
    UPDATE_BLOCK_STATE(s, victim_phy_flash_num, victim_phy_block_num, EMPTY_BLOCK);
    INSERT_EMPTY_BLOCK(s, victim_phy_flash_num, victim_phy_block_num);

    /* Coperd: use estimated GC delay here */
    int64_t GC_TIME = copy_page_nb * (ssdconf->cell_read_delay + 
            ssdconf->reg_write_delay + ssdconf->cell_program_delay + 
            ssdconf->reg_read_delay) + ssdconf->block_erase_delay;

    s->bs->gc_whole_endtime += GC_TIME;

    mylog("GC_TIME=%"PRId64", copy_page_nb=%d\n", GC_TIME, copy_page_nb);

#ifdef MONITOR_ON
    char szTemp[1024];
    sprintf(szTemp, "GC ");
    WRITE_LOG(szTemp);
    sprintf(szTemp, "WB AMP %d", copy_page_nb);
    WRITE_LOG(szTemp);
#endif

#ifdef FTL_DEBUG
    printf("[%s] Complete\n", __FUNCTION__);
#endif
    return SUCCESS;
}

/* Greedy Garbage Collection Algorithm */
int SELECT_VICTIM_BLOCK(IDEState *s, 
        unsigned int *phy_flash_num, unsigned int *phy_block_num)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    int i, j;
    int entry_nb = 0;

    victim_block_root *curr_v_b_root;
    victim_block_entry *curr_v_b_entry;
    victim_block_entry *victim_block = NULL;

    block_state_entry *b_s_entry;
    int curr_valid_page_nb = 0;

    if (ssd->total_victim_block_nb == 0) {
        printf("ERROR[%s, %s] There is no victim block\n", ssd->name, __func__);
        return FAIL;
    }

    /* if GC_TRIGGER_OVERALL is defined, then */
#ifdef GC_TRIGGER_OVERALL
    curr_v_b_root = ssd->victim_block_list;

    /* 
     * Coperd: find the block with most valid pages
     * victim_table_entry_nb == number of planes
     */
    for (i = 0; i < ssdconf->victim_table_entry_nb; i++) {

        /* Coperd: find a victim block */
        if (curr_v_b_root->victim_block_nb != 0) {

            entry_nb = curr_v_b_root->victim_block_nb;
            curr_v_b_entry = curr_v_b_root->head;
            if (victim_block == NULL) {
                victim_block = curr_v_b_root->head;

                b_s_entry = GET_BLOCK_STATE_ENTRY(s, victim_block->phy_flash_num, 
                        victim_block->phy_block_num);

                curr_valid_page_nb = b_s_entry->valid_page_nb;
            }
        } else {
            entry_nb = 0;
        }

        for (j = 0; j < entry_nb; j++) {

            b_s_entry = GET_BLOCK_STATE_ENTRY(s, curr_v_b_entry->phy_flash_num, 
                    curr_v_b_entry->phy_block_num);

            if (curr_valid_page_nb > b_s_entry->valid_page_nb) {
                victim_block = curr_v_b_entry;
                curr_valid_page_nb = b_s_entry->valid_page_nb;
            }
            curr_v_b_entry = curr_v_b_entry->next;
        }
        curr_v_b_root += 1;
    }
#else
    /* if GC_TREGGER_OVERALL is not defined, then */

    /* Coperd: NEED TO FIX HERE */
    int plane_nb = *phy_block_nb % ssdconf->planes_per_flash;
    int mapping_index = plane_nb + ssdconf->flash_nb;
    curr_v_b_root = (victim_block_root *)ssd->victim_block_list + mapping_index;

    if (curr_v_b_root->victim_block_nb != 0) {
        entry_nb = curr_v_b_root->victim_block_nb;
        curr_v_b_entry = curr_v_b_root->head;
        if (victim_block == NULL) {
            victim_block = curr_v_b_root->head;

            b_s_entry = GET_BLOCK_STATE_ENTRY(s, curr_v_b_entry->phy_flash_nb, 
                    curr_v_b_entry->phy_block_nb);
            curr_valid_page_nb = b_s_entry->valid_page_nb;
        }
    } else {
        printf("ERROR[%s] There is no victim entry\n", __FUNCTION__);
    }

    for (i = 0;i < entry_nb; i++) {

        b_s_entry = GET_BLOCK_STATE_ENTRY(s, curr_v_b_entry->phy_flash_nb, 
                curr_v_b_entry->phy_block_nb);

        if (curr_valid_page_nb > b_s_entry->valid_page_nb) {
            victim_block = curr_v_b_entry;
            curr_valid_page_nb = b_s_entry->valid_page_nb;
        }
        curr_v_b_entry = curr_v_b_entry->next;
    }
#endif

    if (curr_valid_page_nb == ssdconf->page_nb) {
        return FAIL;
    }

    *phy_flash_num = victim_block->phy_flash_num;
    *phy_block_num = victim_block->phy_block_num;
    EJECT_VICTIM_BLOCK(s, victim_block);

    return SUCCESS;
}
