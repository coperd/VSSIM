// File: ftl.h
// Date: 2014. 12. 03.
// Author: Jinsoo Yoo (jedisty@hanyang.ac.kr)
// Copyright(c)2014
// Hanyang University, Seoul, Korea
// Embedded Software Systems Laboratory. All right reserved

#ifndef _FTL_H_
#define _FTL_H_

#include "ide.h"

void FTL_INIT(IDEState *s);
void FTL_TERM(IDEState *s);

int FTL_READ(IDEState *s, int32_t sector_nb, unsigned int length);
void FTL_WRITE(IDEState *s, int32_t sector_nb, unsigned int length);

int _FTL_READ(IDEState *s, int32_t sector_nb, unsigned int length);
int _FTL_WRITE(IDEState *s, int32_t sector_nb, unsigned int length);

#endif
