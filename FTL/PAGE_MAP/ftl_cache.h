// File: ftl_cache.h
// Date: 2014. 12. 03.
// Author: Jinsoo Yoo (jedisty@hanyang.ac.kr)
// Copyright(c)2014
// Hanyang University, Seoul, Korea
// Embedded Software Systems Laboratory. All right reserved

#ifndef _VSSIM_CACHE_H_
#define _VSSIM_CACHE_H_

#include "ide.h"


//extern struct map_data* cache_map_data_start;
//extern struct map_data* cache_map_data_end;
//extern uint32_t cache_map_data_nb; 


void INIT_CACHE(IDEState *s);

int32_t CACHE_GET_PPN(IDEState *s, int32_t lpn);
int32_t CACHE_GET_LPN(IDEState *s, int32_t ppn);
int CACHE_UPDATE_PPN(IDEState *s, int32_t lpn, int32_t ppn);
int CACHE_UPDATE_LPN(IDEState *s, int32_t lpn, int32_t ppn);

cache_idx_entry* CACHE_GET_MAP(IDEState *s, uint32_t map_index, uint32_t map_type, int32_t *map_data);
cache_idx_entry* LOOKUP_CACHE(uint32_t map_index, uint32_t map_type);
cache_idx_entry* CACHE_INSERT_MAP(IDEState *s, uint32_t map_index, uint32_t map_type, uint32_t victim_index);
uint32_t CACHE_EVICT_MAP(IDEState *s); 
uint32_t CACHE_SELECT_VICTIM(IDEState *s);

int WRITE_MAP(IDEState *s, uint32_t page_nb, void *buf);
void* READ_MAP(IDEState *s, uint32_t page_nb);
map_data* LOOKUP_MAP_DATA_ENTRY(IDEState *s, uint32_t page_nb);
int REARRANGE_MAP_DATA_ENTRY(IDEState *s, struct map_data *new_entry);

#endif
