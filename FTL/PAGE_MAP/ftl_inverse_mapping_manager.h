// File: ftl_inverse_mapping_manager.h
// Date: 2014. 12. 03.
// Author: Jinsoo Yoo (jedisty@hanyang.ac.kr)
// Copyright(c)2014
// Hanyang University, Seoul, Korea
// Embedded Software Systems Laboratory. All right reserved

#ifndef _INVERSE_MAPPING_MANAGER_H_
#define _INVERSE_MAPPING_MANAGER_H_

#include "ide.h"

void INIT_INVERSE_MAPPING_TABLE(IDEState *s);
void INIT_BLOCK_STATE_TABLE(IDEState *s);
void INIT_EMPTY_BLOCK_LIST(IDEState *s);
void INIT_VICTIM_BLOCK_LIST(IDEState *s);
void INIT_VALID_ARRAY(IDEState *s);

void TERM_INVERSE_MAPPING_TABLE(IDEState *s);
void TERM_BLOCK_STATE_TABLE(IDEState *s);
void TERM_EMPTY_BLOCK_LIST(IDEState *s);
void TERM_VICTIM_BLOCK_LIST(IDEState *s);
void TERM_VALID_ARRAY(IDEState *s);

empty_block_entry *GET_EMPTY_BLOCK(IDEState *s, int mode, int mapping_index);

int INSERT_EMPTY_BLOCK(IDEState *s, unsigned int phy_flash_num, 
        unsigned int phy_block_num);

int INSERT_VICTIM_BLOCK(IDEState *s, empty_block_entry *full_block);

int EJECT_VICTIM_BLOCK(IDEState *s, victim_block_entry *victim_block);

block_state_entry *GET_BLOCK_STATE_ENTRY(IDEState *s, unsigned int phy_flash_num, 
        unsigned int phy_block_num);

int32_t GET_INVERSE_MAPPING_INFO(IDEState *s, int32_t lpn);

int UPDATE_INVERSE_MAPPING(IDEState *s, int32_t ppn, int32_t lpn);

int UPDATE_BLOCK_STATE(IDEState *s, unsigned int phy_flash_num, 
        unsigned int phy_block_num, int type);

int UPDATE_BLOCK_STATE_ENTRY(IDEState *s, unsigned int phy_flash_num, 
        unsigned int phy_block_num, unsigned int phy_page_num, int valid);

void PRINT_VALID_ARRAY(IDEState *s, unsigned int phy_flash_num, 
        unsigned int phy_block_num);

#endif
