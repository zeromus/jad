#ifndef _JADSTD_H
#define _JADSTD_H

#include <stdio.h>

#include "jad.h"

#ifdef  __cplusplus
extern "C" {
#endif

jadError jadstd_OpenStdio(struct jadStream* stream, const char* fname, const char* mode);
jadError jadstd_CloseStdio(struct jadStream* stream);

#ifdef  __cplusplus
}
#endif

#endif //_JADSTD_H
