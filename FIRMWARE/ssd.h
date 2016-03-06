// File: ssd.h
// Date: 2014. 12. 03.
// Author: Jinsoo Yoo (jedisty@hanyang.ac.kr)
// Copyright(c)2014
// Hanyang University, Seoul, Korea
// Embedded Software Systems Laboratory. All right reserved

#ifndef _SSD_H_
#define _SSD_H_

#include "hw.h"
#include "ide.h"


void SSD_INIT(IDEState *s);
void SSD_TERM(IDEState *s);

void SSD_WRITE(IDEState *s, int32_t sector_num, unsigned int length);
int SSD_READ(IDEState *s, int32_t sector_num, unsigned int length);
void SSD_DSM_TRIM(IDEState *s, unsigned int length, void *trim_data);
int SSD_IS_SUPPORT_TRIM(void);

#endif
