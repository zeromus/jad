#ifndef _JADTEST_H
#define _JADTEST_H

#include <stdio.h>

#include "jad.h"

#ifdef  __cplusplus
extern "C" {
#endif

//compares two files for identity
int jadTest_CompareFiles(const char* inpath, const char* inpath2);

#ifdef  __cplusplus
}
#endif

#endif //_JADSTD_H
