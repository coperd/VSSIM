// File: ftl_gc_manager.h
// Date: 2014. 12. 03.
// Author: Jinsoo Yoo (jedisty@hanyang.ac.kr)
// Copyright(c)2014
// Hanyang University, Seoul, Korea
// Embedded Software Systems Laboratory. All right reserved

#ifndef _GC_MANAGER_H_
#define _GC_MANAGER_H_

#include "ide.h"


void GC_CHECK(IDEState *s, unsigned int phy_flash_nb, unsigned int phy_block_nb);

int GARBAGE_COLLECTION(IDEState *s);
int SELECT_VICTIM_BLOCK(IDEState *s, unsigned int* phy_flash_nb, unsigned int* phy_block_nb);

#endif
