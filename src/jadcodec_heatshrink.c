#include "jadcodec_heatshrink.h"

#include <stdio.h>

int jadCodec_HeatShrinkEncoder_Get(jadStream* stream)
{
	jadHeatshrinkEncoder *encoder = (jadHeatshrinkEncoder*)stream->opaque;
	uint8_t buf;
	size_t output_size;
	HSE_poll_res result = heatshrink_encoder_poll(&encoder->encoder, &buf, 1, &output_size);
	
	if(output_size == 1) return buf;
	else if(result == HSER_POLL_EMPTY) return JAD_EOF;
	else if(result == HSER_POLL_MORE) return JAD_EOF;
	else return JAD_ERROR;
}

int jadCodec_HeatShrinkEncoder_Put(jadStream* stream, uint8_t val)
{
	jadHeatshrinkEncoder *encoder = (jadHeatshrinkEncoder*)stream->opaque;
	size_t input_size;
	HSE_sink_res result = heatshrink_encoder_sink(&encoder->encoder,&val,1,&input_size);
	return (result == HSER_SINK_OK && input_size==1) ? JAD_OK : JAD_ERROR;
}

int jadCodec_HeatShrinkEncoder_Flush(jadStream* stream)
{
	jadHeatshrinkEncoder *encoder = (jadHeatshrinkEncoder*)stream->opaque;
	HSE_finish_res result = heatshrink_encoder_finish(&encoder->encoder);
	return JAD_OK; //this can't fail, apparently
}

jadError jadcodec_OpenHeatshrinkEncoder(jadHeatshrinkEncoder* encoder)
{
	heatshrink_encoder_reset(&encoder->encoder);
	
	encoder->stream.opaque = encoder;
	encoder->stream.get = &jadCodec_HeatShrinkEncoder_Get;
	encoder->stream.put = &jadCodec_HeatShrinkEncoder_Put;
	encoder->stream.flush = &jadCodec_HeatShrinkEncoder_Flush;
	return JAD_OK;
}

jadError jadcodec_CloseHeatshrinkEncoder(jadHeatshrinkEncoder* encoder)
{
	//error if it isnt flushed fully?
	return JAD_OK;
}

//-------------------------------------------------------------------------

int jadCodec_HeatShrinkDecoder_Get(jadStream* stream)
{
	jadHeatshrinkDecoder *decoder = (jadHeatshrinkDecoder*)stream->opaque;
	uint8_t buf;
	size_t output_size;
	HSD_poll_res result = heatshrink_decoder_poll(&decoder->decoder, &buf, 1, &output_size);
	if(output_size == 1) return buf;
	else if(result == HSER_POLL_EMPTY) return JAD_EOF;
	else if(result == HSER_POLL_MORE) return JAD_EOF;
	else return JAD_ERROR;
}

int jadCodec_HeatShrinkDecoder_Put(jadStream* stream, uint8_t val)
{
	jadHeatshrinkDecoder *decoder = (jadHeatshrinkDecoder*)stream->opaque;
	size_t input_size;
	HSD_sink_res result = heatshrink_decoder_sink(&decoder->decoder,&val,1,&input_size);
	return (result == HSER_SINK_OK && input_size==1) ? JAD_OK : JAD_ERROR;
}

int jadCodec_HeatShrinkDecoder_Flush(jadStream* stream)
{
	jadHeatshrinkDecoder *decoder = (jadHeatshrinkDecoder*)stream->opaque;
	HSD_finish_res result = heatshrink_decoder_finish(&decoder->decoder);
	return JAD_OK; //this can't fail, apparently
}

jadError jadcodec_OpenHeatshrinkDecoder(jadHeatshrinkDecoder* decoder)
{
	heatshrink_decoder_reset(&decoder->decoder);
	
	decoder->stream.opaque = decoder;
	decoder->stream.get = &jadCodec_HeatShrinkDecoder_Get;
	decoder->stream.put = &jadCodec_HeatShrinkDecoder_Put;
	decoder->stream.flush = &jadCodec_HeatShrinkDecoder_Flush;

	return JAD_OK;
}

jadError jadcodec_CloseHeatshrinkDecoder(jadHeatshrinkDecoder* decoder)
{
	//error if it isnt flushed fully?
	return JAD_OK;
}