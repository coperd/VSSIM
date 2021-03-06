// File: ftl_mapping_manager.h
// Date: 2014. 12. 03.
// Author: Jinsoo Yoo (jedisty@hanyang.ac.kr)
// Copyright(c)2014
// Hanyang University, Seoul, Korea
// Embedded Software Systems Laboratory. All right reserved

#ifndef _MAPPING_MANAGER_H_
#define _MAPPING_MANAGER_H_

#include "ide.h"

//extern int32_t* mapping_table;
//extern void* block_table_start;

//extern unsigned int flash_index;
//extern unsigned int* plane_index;
//extern unsigned int* block_index;

void INIT_MAPPING_TABLE(IDEState *s);
void TERM_MAPPING_TABLE(IDEState *s);

int32_t GET_MAPPING_INFO(IDEState *s, int32_t lpn);
int GET_NEW_PAGE(IDEState *s, int mode, int mapping_index, int32_t* ppn);

int UPDATE_OLD_PAGE_MAPPING(IDEState *s, int32_t lpn);
int UPDATE_NEW_PAGE_MAPPING(IDEState *s, int32_t lpn, int32_t ppn);

unsigned int CALC_FLASH(IDEState *s, int32_t ppn);
unsigned int CALC_BLOCK(IDEState *s, int32_t ppn);
unsigned int CALC_PAGE(IDEState *s, int32_t ppn);

#endif
