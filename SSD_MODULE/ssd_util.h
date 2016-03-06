// File: ssd_util.h
// Date: 2014. 12. 03.
// Author: Jinsoo Yoo (jedisty@hanyang.ac.kr)
// Copyright(c)2014
// Hanyang University, Seoul, Korea
// Embedded Software Systems Laboratory. All right reserved

#ifndef _SSD_UTIL_H_
#define _SSD_UTIL_H_

#include <string.h>
#include "ide.h"

#ifndef _UTEST_
	#include "hw.h"
#else

	typedef signed long long int64_t;
	#include <stdio.h>
	#include <malloc.h>
	#include <memory.h>

#endif

char *truncate_filename(char *src);
void set_ssd_name(IDEState *s);
char *get_ssd_name(IDEState *s);
void set_ssd_struct_filename(IDEState *s, char *src, const char *ssd_struct_name);

#endif

