#ifndef _JADCODEC_HEATSHRINK_H
#define _JADCODEC_HEATSHRINK_H

#include "jad.h"

#include "heatshrink/heatshrink_encoder.h"
#include "heatshrink/heatshrink_decoder.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct jadHeatshrinkEncoder
{
	heatshrink_encoder encoder;
	jadStream stream;
} jadHeatshrinkEncoder;

typedef struct jadHeatshrinkDecoder
{
	heatshrink_decoder decoder;
	jadStream stream;
} jadHeatshrinkDecoder;

jadError jadcodec_OpenHeatshrinkEncoder(jadHeatshrinkEncoder* encoder);
jadError jadcodec_CloseHeatshrinkEncoder(jadHeatshrinkEncoder* encoder);

jadError jadcodec_OpenHeatshrinkDecoder(jadHeatshrinkDecoder* decoder);
jadError jadcodec_CloseHeatshrinkDecoder(jadHeatshrinkDecoder* decoder);

#ifdef  __cplusplus
}
#endif


#endif //_JADCODEC_HEATSHRINK_H
