#ifndef _JADSTD_H
#define _JADSTD_H

#include <stdio.h>

#include "jad.h"

#ifdef  __cplusplus
extern "C" {
#endif

JADEXPORT jadError jadstd_OpenStdio(jadStream* stream, const char* fname, const char* mode);
JADEXPORT jadError jadstd_CloseStdio(jadStream* stream);

JADEXPORT jadError jadstd_OpenAllocator(jadAllocator* allocator);
JADEXPORT jadError jadstd_CloseAllocator(jadAllocator* allocator);

#ifdef  __cplusplus
}
#endif

#endif //_JADSTD_H
