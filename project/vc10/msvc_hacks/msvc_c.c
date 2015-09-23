
//#include "stdint.h" //why doesnt this work?

//have to do this instead
typedef __int32 int32_t;

//and also this for why? ?
typedef unsigned __int32 u_int32_t;

#include <math.h>

#define INFINITY   ((float)(_HUGE_ENUF * _HUGE_ENUF))  /* causes warning C4756: overflow in constant arithmetic (by design) */
#define NAN        ((float)(INFINITY * 0.0F))
#define FP_INFINITE  1
#define FP_NAN       2
#define FP_NORMAL    (-1)
#define FP_SUBNORMAL (-2)
#define FP_ZERO      0

#define IEEE_LITTLE_ENDIAN 1
#define JADTOOL_CUSTOM_DECLARATION 1
#include "strtod.c"