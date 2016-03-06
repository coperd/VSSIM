// File: ftl_inverse_mapping_manager.c
// Date: 2014. 12. 03.
// Author: Jinsoo Yoo (jedisty@hanyang.ac.kr)
// Copyright(c)2014
// Hanyang University, Seoul, Korea
// Embedded Software Systems Laboratory. All right reserved

#include "ftl_inverse_mapping_manager.h"
#include "ssd_util.h"

void INIT_INVERSE_MAPPING_TABLE(IDEState *s)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    /* Allocation Memory for Inverse Page Mapping Table */
    ssd->inverse_mapping_table = qemu_mallocz(sizeof(int32_t) * ssdconf->page_mapping_entry_nb);

    set_ssd_struct_filename(s, ssd->im_filename, "_inverse_mapping.dat");
    /* Initialization Inverse Page Mapping Table */
    FILE *fp = fopen(ssd->im_filename, "r");
    if (fp != NULL) {
        fread(ssd->inverse_mapping_table, sizeof(int32_t), ssdconf->page_mapping_entry_nb, fp);
    } else {
        int i;
        for (i = 0; i < ssdconf->page_mapping_entry_nb; i++) {
            ssd->inverse_mapping_table[i] = -1;
        }
    }
}


void INIT_BLOCK_STATE_TABLE(IDEState *s)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);
    char *bst_filename = ssd->bst_filename;

    /* Allocation Memory for Inverse Block Mapping Table */
    ssd->block_state_table = qemu_mallocz(sizeof(block_state_entry) * ssdconf->block_mapping_entry_nb);

    /* Initialization Inverse Block Mapping Table */
    set_ssd_struct_filename(s, bst_filename, "_block_state_table.dat");
    FILE *fp = fopen(bst_filename, "r");

    if (fp != NULL) {
        fread(ssd->block_state_table, sizeof(block_state_entry), ssdconf->block_mapping_entry_nb, fp);
    } else {
        int i;
        block_state_entry *curr_b_s_entry = ssd->block_state_table;

        for (i = 0; i < ssdconf->block_mapping_entry_nb; i++) {
            curr_b_s_entry->type		    = EMPTY_BLOCK;
            curr_b_s_entry->valid_page_nb	= 0; /* Coperd: shouldn't we set this to PAGE_NB */
            curr_b_s_entry->erase_count		= 0;
            curr_b_s_entry                 += 1; 
        }
    }
}

/* Coperd: In this function, valid array is LOCAL */
void INIT_VALID_ARRAY(IDEState *s)
{
	int i;
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    //void *block_state_table = ssd->block_state_table;
    char *valid_array;
	block_state_entry *curr_b_s_entry = ssd->block_state_table;

    set_ssd_struct_filename(s, ssd->va_filename, "_valid_array.dat");

	FILE *fp = fopen(ssd->va_filename, "r");
	if (fp != NULL) {
		for (i = 0; i < ssdconf->block_mapping_entry_nb; i++) {
			valid_array = (char *)calloc(ssdconf->page_nb, sizeof(char));
			fread(valid_array, sizeof(char), ssdconf->page_nb, fp);
			curr_b_s_entry->valid_array = valid_array;

			curr_b_s_entry += 1;
		}
	} else {
		for (i = 0; i < ssdconf->block_mapping_entry_nb; i++) {
            valid_array = qemu_mallocz(sizeof(char) * ssdconf->page_nb);
			curr_b_s_entry->valid_array = valid_array;

			curr_b_s_entry += 1;
		}
	}
}


void INIT_EMPTY_BLOCK_LIST(IDEState *s)
{
    int i, j, k;
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);
    int nb_regs = ssdconf->planes_per_flash * ssdconf->flash_nb;

    empty_block_entry *curr_entry;
    empty_block_root *curr_root;

    /* Coperd: allocate memory for empty_block_root */
    ssd->empty_block_list = qemu_mallocz(sizeof(empty_block_root)*nb_regs);

    set_ssd_struct_filename(s, ssd->ebl_filename, "_empty_block_list.dat");

    FILE *fp = fopen(ssd->ebl_filename, "r");
    if (fp != NULL) {
        ssd->total_empty_block_nb = 0;
        fread(ssd->empty_block_list, sizeof(empty_block_root), ssdconf->planes_per_flash*ssdconf->flash_nb, fp);
        curr_root = ssd->empty_block_list;

        for (i = 0; i < ssdconf->planes_per_flash; i++) {

            for (j = 0; j < ssdconf->flash_nb ; j++) {

                ssd->total_empty_block_nb += curr_root->empty_block_nb;
                k = curr_root->empty_block_nb;

                while (k > 0) {
                    curr_entry = (empty_block_entry *)calloc(1, sizeof(empty_block_entry));
                    if (curr_entry == NULL) {
                        printf("ERROR[%s] Calloc fail\n", __FUNCTION__);
                        break;
                    }

                    fread(curr_entry, sizeof(empty_block_entry), 1, fp);
                    curr_entry->next = NULL;

                    if (k == curr_root->empty_block_nb) {
                        curr_root->head = curr_entry;
                        curr_root->tail = curr_entry;
                    } else{
                        curr_root->tail->next = curr_entry;
                        curr_root->tail = curr_entry;
                    }
                    k--;
                }
                curr_root += 1;
            }
        }
        ssd->empty_block_table_index = 0;
    } else {
        curr_root = ssd->empty_block_list;		

        for (i = 0; i < ssdconf->planes_per_flash; i++) {

            for (j = 0; j < ssdconf->flash_nb; j++) {

                for (k = i; k < ssdconf->block_nb; k += ssdconf->planes_per_flash) {

                    curr_entry = (empty_block_entry *)calloc(1, sizeof(empty_block_entry));	
                    if (curr_entry == NULL) {
                        printf("ERROR[%s] Calloc fail\n", __FUNCTION__);
                        break;
                    }

                    if (k == i) {
                        curr_root->head = curr_entry;
                        curr_root->tail = curr_entry;

                        curr_root->tail->phy_flash_nb = j;
                        curr_root->tail->phy_block_nb = k;
                        curr_root->tail->curr_phy_page_nb = 0;
                    } else {
                        curr_root->tail->next = curr_entry;
                        curr_root->tail = curr_entry;

                        curr_root->tail->phy_flash_nb = j;
                        curr_root->tail->phy_block_nb = k;
                        curr_root->tail->curr_phy_page_nb = 0;
                    }

                    UPDATE_BLOCK_STATE(s, j, k, EMPTY_BLOCK);
                }

                curr_root->empty_block_nb = (unsigned int)ssdconf->each_empty_table_entry_nb;
                curr_root += 1;
            }
        }

        ssd->total_empty_block_nb = (int64_t)ssdconf->block_mapping_entry_nb;
        ssd->empty_block_table_index = 0;
    }
}

void INIT_VICTIM_BLOCK_LIST(IDEState *s)
{
    int i, j, k;
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);
    int nb_regs = ssdconf->planes_per_flash * ssdconf->flash_nb;

    victim_block_entry *curr_entry;
    victim_block_root *curr_root;

    ssd->victim_block_list = qemu_mallocz(sizeof(victim_block_root) * nb_regs);

    set_ssd_struct_filename(s, ssd->vbl_filename, "_victim_block_list.dat");
    FILE *fp = fopen(ssd->vbl_filename, "r");
    if (fp != NULL) {
        ssd->total_victim_block_nb = 0;
        fread(ssd->victim_block_list, sizeof(victim_block_root), ssdconf->planes_per_flash*ssdconf->flash_nb, fp);
        curr_root = ssd->victim_block_list;

        for (i = 0; i < ssdconf->planes_per_flash; i++) {

            for (j = 0; j < ssdconf->flash_nb; j++) {

                ssd->total_victim_block_nb += curr_root->victim_block_nb;
                k = curr_root->victim_block_nb;
                while (k > 0) {
                    curr_entry = (victim_block_entry *)calloc(1, sizeof(victim_block_entry));
                    if (curr_entry == NULL) {
                        printf("ERROR[%s] Calloc fail\n", __FUNCTION__);
                        break;
                    }

                    fread(curr_entry, sizeof(victim_block_entry), 1, fp);
                    curr_entry->next = NULL;
                    curr_entry->prev = NULL;

                    if (k == curr_root->victim_block_nb) {
                        curr_root->head = curr_entry;
                        curr_root->tail = curr_entry;
                    } else {
                        curr_root->tail->next = curr_entry;
                        curr_entry->prev = curr_root->tail;
                        curr_root->tail = curr_entry;
                    }
                    k--;
                }
                curr_root += 1;
            }
        }
    } else {
        curr_root = ssd->victim_block_list;		

        for (i = 0; i < nb_regs; i++) {

            curr_root->head = NULL;
            curr_root->tail = NULL;
            curr_root->victim_block_nb = 0;

            curr_root += 1;
        }
        ssd->total_victim_block_nb = 0;
    }
}

void TERM_INVERSE_MAPPING_TABLE(IDEState *s)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

	FILE *fp = fopen(ssd->im_filename, "w");
	if (fp == NULL) {
		printf("ERROR[%s] File open fail\n", __FUNCTION__);
		return;
	}

	/* Write The inverse page table to file */
	fwrite(ssd->inverse_mapping_table, sizeof(int32_t), ssdconf->page_mapping_entry_nb, fp);

	/* Free the inverse page table memory */
	qemu_free(ssd->inverse_mapping_table);
}

void TERM_BLOCK_STATE_TABLE(IDEState *s)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    void *block_state_table = s->ssd.block_state_table;
	FILE *fp = fopen(s->ssd.bst_filename, "w");
	if (fp == NULL) {
		printf("ERROR[%s] File open fail\n", __FUNCTION__);
		return;
	}

	/* Write The inverse block table to file */
	fwrite(block_state_table, sizeof(block_state_entry), ssdconf->block_mapping_entry_nb, fp);

	/* Free The inverse block table memory */
	qemu_free(block_state_table);
}

void TERM_VALID_ARRAY(IDEState *s)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    int i;
    block_state_entry *curr_b_s_entry = ssd->block_state_table;
    char *valid_array;

    FILE *fp = fopen(ssd->va_filename, "w");
    if (fp == NULL) {
        printf("ERROR[%s] File open fail\n", __FUNCTION__);
        return;
    }

    for (i = 0; i < ssdconf->block_mapping_entry_nb; i++) {
        valid_array = curr_b_s_entry->valid_array;
        fwrite(valid_array, sizeof(char), ssdconf->page_nb, fp);
        curr_b_s_entry += 1;
    }
}

void TERM_EMPTY_BLOCK_LIST(IDEState *s)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    int i, j, k;

    empty_block_entry *curr_entry;
    empty_block_root *curr_root;

    FILE *fp = fopen(ssd->ebl_filename, "w");
    if (fp == NULL) {
        printf("ERROR[%s] File open fail\n", __FUNCTION__);
    }

    fwrite(ssd->empty_block_list, sizeof(empty_block_root), ssdconf->planes_per_flash * ssdconf->flash_nb, fp);

    curr_root = (empty_block_root *)ssd->empty_block_list;
    for (i = 0; i < ssdconf->planes_per_flash; i++) {

        for (j = 0; j < ssdconf->flash_nb; j++) {

            k = curr_root->empty_block_nb;
            if (k != 0) {
                curr_entry = (empty_block_entry *)curr_root->head;
            }
            while (k > 0) {

                fwrite(curr_entry, sizeof(empty_block_entry), 1, fp);

                if (k != 1) {
                    curr_entry = curr_entry->next;
                }
                k--;
            }
            curr_root += 1;
        }
    }
}

void TERM_VICTIM_BLOCK_LIST(IDEState *s)
{
    int i, j, k;
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    victim_block_entry *curr_entry;
    victim_block_root *curr_root;

    FILE *fp = fopen(ssd->vbl_filename, "w");
    if (fp == NULL) {
        printf("ERROR[%s] File open fail\n", __FUNCTION__);
    }

    fwrite(ssd->victim_block_list, sizeof(victim_block_root), ssdconf->planes_per_flash*ssdconf->flash_nb, fp);

    curr_root = ssd->victim_block_list;
    for (i = 0; i < ssdconf->planes_per_flash; i++) {

        for (j = 0; j < ssdconf->flash_nb; j++) {

            k = curr_root->victim_block_nb;
            if (k != 0) {
                curr_entry = curr_root->head;
            }
            while (k > 0) {

                fwrite(curr_entry, sizeof(victim_block_entry), 1, fp);

                if (k != 1) {
                    curr_entry = curr_entry->next;
                }
                k--;
            }
            curr_root += 1;
        }
    }
}

empty_block_entry *GET_EMPTY_BLOCK(IDEState *s, int mode, int mapping_index)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    if (ssd->total_empty_block_nb == 0) {
        printf("ERROR[%s] There is no empty block\n", __FUNCTION__);
        return NULL;
    }

    int input_mapping_index = mapping_index;

    empty_block_entry *curr_empty_block;
    empty_block_root *curr_root_entry;

    while (ssd->total_empty_block_nb != 0) {

        if (mode == VICTIM_OVERALL) {
            curr_root_entry = ssd->empty_block_list + ssd->empty_block_table_index;

            if (curr_root_entry->empty_block_nb == 0) {
                ssd->empty_block_table_index++;
                if (ssd->empty_block_table_index == ssdconf->empty_table_entry_nb) {
                    ssd->empty_block_table_index = 0;
                }
                continue;
            } else {
                curr_empty_block = curr_root_entry->head;
                if (curr_empty_block->curr_phy_page_nb == ssdconf->page_nb) {

                    /* Update Empty Block List */
                    if (curr_root_entry->empty_block_nb == 1) {
                        curr_root_entry->head = NULL;
                        curr_root_entry->empty_block_nb = 0;
                    } else {
                        curr_root_entry->head = curr_empty_block->next;
                        curr_root_entry->empty_block_nb -= 1;
                    }

                    /* Eject Empty Block from the list */
                    INSERT_VICTIM_BLOCK(s, curr_empty_block);

                    /* Update The total number of empty block */
                    ssd->total_empty_block_nb--;

                    ssd->empty_block_table_index++;
                    if (ssd->empty_block_table_index == ssdconf->empty_table_entry_nb) {
                        ssd->empty_block_table_index = 0;
                    }
                    continue;
                }
                ssd->empty_block_table_index++;
                if (ssd->empty_block_table_index == ssdconf->empty_table_entry_nb) {
                    ssd->empty_block_table_index = 0;
                }
                return curr_empty_block;
            }
        } else if (mode == VICTIM_INCHIP) {
            curr_root_entry = ssd->empty_block_list + mapping_index;
            if (curr_root_entry->empty_block_nb == 0) {

                /* If the flash memory has multiple planes, move index */
                if (ssdconf->planes_per_flash != 1) {
                    mapping_index++;
                    if (mapping_index % ssdconf->planes_per_flash == 0) {
                        mapping_index = mapping_index - (ssdconf->planes_per_flash-1);
                    }
                    if (mapping_index == input_mapping_index) {
                        printf("ERROR[%s] There is no empty block\n",__FUNCTION__);
                        return NULL;
                    }
                } else {
                    /* If there is no empty block in the flash memory, return fail */
#ifdef DEBUG_FTL
                    printf("ERROR[%s]-INCHIP There is no empty block\n", __FUNCTION__);
#endif
                    return NULL;
                }

                continue;
            } else {
                curr_empty_block = curr_root_entry->head;
                if (curr_empty_block->curr_phy_page_nb == ssdconf->page_nb) {

                    /* Update Empty Block List */
                    if (curr_root_entry->empty_block_nb == 1) {
                        curr_root_entry->head = NULL;
                        curr_root_entry->empty_block_nb = 0;
                    } else {
                        curr_root_entry->head = curr_empty_block->next;
                        curr_root_entry->empty_block_nb -= 1;
                    }

                    /* Eject Empty Block from the list */
                    INSERT_VICTIM_BLOCK(s, curr_empty_block);

                    /* Update The total number of empty block */
                    ssd->total_empty_block_nb--;

                    continue;
                } else {
                    curr_empty_block = curr_root_entry->head;
                }

                return curr_empty_block;
            }	
        } else if (mode == VICTIM_NOPARAL) {
            curr_root_entry = ssd->empty_block_list + mapping_index;
            if (curr_root_entry->empty_block_nb == 0) {

                mapping_index++;
                ssd->empty_block_table_index++;
                if (mapping_index == ssdconf->empty_table_entry_nb) {
                    mapping_index = 0;
                    ssd->empty_block_table_index = 0;
                }
                continue;
            } else {
                curr_empty_block = curr_root_entry->head;
                if (curr_empty_block->curr_phy_page_nb == ssdconf->page_nb) {

                    /* Update Empty Block List */
                    if (curr_root_entry->empty_block_nb == 1) {
                        curr_root_entry->head = NULL;
                        curr_root_entry->empty_block_nb = 0;
                    } else {
                        curr_root_entry->head = curr_empty_block->next;
                        curr_root_entry->empty_block_nb -= 1;
                    }

                    /* Eject Empty Block from the list */
                    INSERT_VICTIM_BLOCK(s, curr_empty_block);

                    /* Update The total number of empty block */
                    ssd->total_empty_block_nb--;

                    continue;
                } else {
                    curr_empty_block = curr_root_entry->head;
                }

                return curr_empty_block;
            }	
        }
    }

    printf("ERROR[%s] There is no empty block\n", __FUNCTION__);
    return NULL;
}

int INSERT_EMPTY_BLOCK(IDEState *s, unsigned int phy_flash_num, 
        unsigned int phy_block_num)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);
    int mapping_index;
    int plane_num;

    empty_block_root *curr_root_entry = NULL;
    empty_block_entry *new_empty_block = NULL;

    new_empty_block = qemu_mallocz(sizeof(empty_block_entry));

    /* Init New empty block */
    new_empty_block->phy_flash_nb = phy_flash_num;
    new_empty_block->phy_block_nb = phy_block_num;
    new_empty_block->curr_phy_page_nb = 0;
    new_empty_block->next = NULL;

    plane_num = phy_block_num % ssdconf->planes_per_flash;
    mapping_index = plane_num * ssdconf->flash_nb + phy_flash_num;

    curr_root_entry = ssd->empty_block_list + mapping_index;

    if (curr_root_entry->empty_block_nb == 0) {
        curr_root_entry->head = new_empty_block;
        curr_root_entry->tail = new_empty_block;
        curr_root_entry->empty_block_nb = 1;
    } else {
        curr_root_entry->tail->next = new_empty_block;
        curr_root_entry->tail = new_empty_block;
        curr_root_entry->empty_block_nb++;
    }
    ssd->total_empty_block_nb++;

    return SUCCESS;
}


int INSERT_VICTIM_BLOCK(IDEState *s, empty_block_entry *full_block)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    int mapping_index;
    int plane_num;

    victim_block_root *curr_v_b_root;
    victim_block_entry *new_v_b_entry;

    /* Alloc New victim block entry */
    new_v_b_entry = qemu_mallocz(sizeof(victim_block_entry));

    /* Copy the full block address */
    new_v_b_entry->phy_flash_num = full_block->phy_flash_nb;
    new_v_b_entry->phy_block_num = full_block->phy_block_nb;
    new_v_b_entry->prev = NULL;
    new_v_b_entry->next = NULL;

    plane_num = full_block->phy_block_nb % ssdconf->planes_per_flash;
    mapping_index = plane_num * ssdconf->flash_nb + full_block->phy_flash_nb;

    curr_v_b_root = ssd->victim_block_list + mapping_index;

    /* Update victim block list */
    if (curr_v_b_root->victim_block_nb == 0) {
        curr_v_b_root->head = new_v_b_entry;
        curr_v_b_root->tail = new_v_b_entry;
        curr_v_b_root->victim_block_nb = 1;
    } else {
        curr_v_b_root->tail->next = new_v_b_entry;
        new_v_b_entry->prev = curr_v_b_root->tail;
        curr_v_b_root->tail = new_v_b_entry;
        curr_v_b_root->victim_block_nb++;
    }

    /* Free the full empty block entry */
    qemu_free(full_block);

    /* Update the total number of victim block */
    ssd->total_victim_block_nb++;

    return SUCCESS;
}

int EJECT_VICTIM_BLOCK(IDEState *s, victim_block_entry *victim_block)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

	int mapping_index;
	int plane_num;

	victim_block_root *curr_v_b_root = NULL;

	plane_num = victim_block->phy_block_num % ssdconf->planes_per_flash;
    /* Coperd: this mapping_index is to choose the corresponding plane */
	mapping_index = victim_block->phy_flash_num + plane_num * ssdconf->flash_nb;

	curr_v_b_root = ssd->victim_block_list + mapping_index;

	/* Update victim block list */
	if (victim_block == curr_v_b_root->head) {
		if (curr_v_b_root->victim_block_nb == 1) {
			curr_v_b_root->head = NULL;
			curr_v_b_root->tail = NULL;
		} else {
			curr_v_b_root->head = victim_block->next;
			curr_v_b_root->head->prev = NULL;
		}
	} else if (victim_block == curr_v_b_root->tail) {
		curr_v_b_root->tail = victim_block->prev;
		curr_v_b_root->tail->next = NULL;
	} else {
		victim_block->prev->next = victim_block->next;
		victim_block->next->prev = victim_block->prev;
	}

	curr_v_b_root->victim_block_nb--;
	ssd->total_victim_block_nb--;

	/* Free the victim block */
	qemu_free(victim_block);

	return SUCCESS;
}

block_state_entry *GET_BLOCK_STATE_ENTRY(IDEState *s, unsigned int phy_flash_num, 
        unsigned int phy_block_num)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    block_state_entry *block_state_table = ssd->block_state_table;
    int64_t mapping_index = phy_flash_num * ssdconf->block_nb + phy_block_num;

    block_state_entry *mapping_entry = block_state_table + mapping_index;

    return mapping_entry;
}

int32_t GET_INVERSE_MAPPING_INFO(IDEState *s, int32_t ppn)
{
    SSDState *ssd = &(s->ssd);

	int32_t lpn = ssd->inverse_mapping_table[ppn];

	return lpn;
}

// NEED MODIFY
int UPDATE_INVERSE_MAPPING(IDEState *s, int32_t ppn,  int32_t lpn)
{
    SSDState *ssd = &(s->ssd);

#ifdef FTL_MAP_CACHE
	CACHE_UPDATE_LPN(lpn, ppn);
#else
	ssd->inverse_mapping_table[ppn] = lpn;
#endif

	return SUCCESS;
}

int UPDATE_BLOCK_STATE(IDEState *s, unsigned int phy_flash_nb, 
        unsigned int phy_block_nb, int type)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    int i;
    block_state_entry *b_s_entry = 
        GET_BLOCK_STATE_ENTRY(s, phy_flash_nb, phy_block_nb);

    b_s_entry->type = type;

    if (type == EMPTY_BLOCK) {
        for (i = 0; i < ssdconf->page_nb; i++) {
            UPDATE_BLOCK_STATE_ENTRY(s, phy_flash_nb, phy_block_nb, i, 0);
        }
    }

    return SUCCESS;
}

int UPDATE_BLOCK_STATE_ENTRY(IDEState *s, unsigned int phy_flash_nb, 
        unsigned int phy_block_nb, unsigned int phy_page_nb, int valid)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    if (phy_flash_nb >= ssdconf->flash_nb || phy_block_nb >= ssdconf->block_nb 
            || phy_page_nb >= ssdconf->page_nb) {
        printf("ERROR[%s] Wrong physical address\n", __FUNCTION__);
        return FAIL;
    }

    int i;
    int valid_count = 0;
    block_state_entry *b_s_entry = 
        GET_BLOCK_STATE_ENTRY(s, phy_flash_nb, phy_block_nb);

    char *valid_array = b_s_entry->valid_array;

    if (valid == VALID) {
        valid_array[phy_page_nb] = 'V';
    } else if (valid == INVALID) {
        valid_array[phy_page_nb] = 'I';
    } else if (valid == 0) {
        valid_array[phy_page_nb] = '0';
    } else {
        printf("ERROR[%s] Wrong valid value\n", __FUNCTION__);
    }

    /* Update valid_page_nb */
    for (i = 0; i < ssdconf->page_nb; i++) {
        if (valid_array[i] == 'V') {
            valid_count++;
        }
    }
    b_s_entry->valid_page_nb = valid_count;

    return SUCCESS;
}

void PRINT_VALID_ARRAY(IDEState *s, unsigned int phy_flash_nb, 
        unsigned int phy_block_nb)
{
    SSDState *ssd = &(s->ssd);
    SSDConf *ssdconf = &(ssd->param);

    int i;
    int cnt = 0;
    block_state_entry *b_s_entry = 
        GET_BLOCK_STATE_ENTRY(s, phy_flash_nb, phy_block_nb);

    printf("Type %d [%d][%d]valid array:\n", b_s_entry->type,  phy_flash_nb, 
            phy_block_nb);
    for (i = 0; i < ssdconf->page_nb; i++) {
        printf("%c ", b_s_entry->valid_array[i]);
        cnt++;
        if (cnt == 10) {
            printf("\n");
        }
    }
    printf("\n");
}
