#include <stdio.h>
#include "jadstd.h"

int jadstd_StdioRead(void* buffer, size_t bytes, jadStream* stream)
{
	FILE* f = (FILE*)stream->opaque;
	return fread(buffer,1,bytes,f);
}

int jadstd_StdioWrite(const void* buffer, size_t bytes, jadStream* stream)
{
	FILE* f = (FILE*)stream->opaque;
	return fwrite(buffer,1,bytes,f);
}

long jadstd_StdioSeek(jadStream* stream, long offset, int origin)
{
	FILE* f = (FILE*)stream->opaque;
	int ret = fseek(f,offset,origin);
	if(ret<0) return ret;
	offset = ftell(f);
	return offset;
}

int jadstd_StdioGet(jadStream* stream)
{
	int ret = fgetc((FILE*)stream->opaque);
	if(ret == EOF)
		return JAD_EOF;
	else return ret;
}

int jadstd_StdioPut(jadStream* stream, uint8_t value)
{
	int ret = fputc(value, (FILE*)stream->opaque);
	if(ret == EOF)
		return JAD_EOF;
	else return ret;
}

jadError jadstd_OpenStdio(jadStream* stream, const char* fname, const char* mode)
{
	FILE* f = fopen(fname,mode);
	if(f == NULL)
	{
		return JAD_ERROR;
	}
	stream->opaque = f;
	stream->read = &jadstd_StdioRead;
	stream->write = &jadstd_StdioWrite;
	stream->seek = &jadstd_StdioSeek;
	return JAD_OK;
}

jadError jadstd_CloseStdio(jadStream* stream)
{
	if(stream->opaque == NULL)
		return JAD_ERROR;
	return fclose((FILE*)stream->opaque) != EOF;
}