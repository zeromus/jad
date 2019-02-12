#include <stdio.h>
#include <stdlib.h>

#include "jadstd.h"

static int jadstd_StdioRead(void* buffer, size_t bytes, jadStream* stream)
{
	FILE* f = (FILE*)stream->opaque;
	return fread(buffer,1,bytes,f);
}

static int jadstd_StdioWrite(const void* buffer, size_t bytes, jadStream* stream)
{
	FILE* f = (FILE*)stream->opaque;
	return fwrite(buffer,1,bytes,f);
}

static long jadstd_StdioSeek(jadStream* stream, long offset, int origin)
{
	FILE* f = (FILE*)stream->opaque;
	int ret = fseek(f,offset,origin);
	if(ret<0) return ret;
	offset = ftell(f);
	return offset;
}

static int jadstd_StdioGet(jadStream* stream)
{
	int ret = fgetc((FILE*)stream->opaque);
	if(ret == EOF)
		return JAD_EOF;
	else return ret;
}

static int jadstd_StdioPut(jadStream* stream, uint8_t value)
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
	stream->get = &jadstd_StdioGet;
	stream->put = &jadstd_StdioPut;
	return JAD_OK;
}

jadError jadstd_CloseStdio(jadStream* stream)
{
	if(stream->opaque == NULL)
		return JAD_ERROR;
	return fclose((FILE*)stream->opaque) != EOF;
}

//callbacks for jadAllocator operations
static void* jadstd_StdlibAlloc(jadAllocator* allocator, size_t amt)
{
	return malloc(amt);
}

static void* jadstd_StdlibRealloc(jadAllocator* allocator,  void* ptr, size_t amt)
{
	return realloc(ptr, amt);
}

static void jadstd_StdlibFree(jadAllocator* allocator, void* ptr)
{
	free(ptr);
}

jadError jadstd_OpenAllocator(jadAllocator* allocator)
{
	allocator->opaque = NULL;
	allocator->alloc = &jadstd_StdlibAlloc;
	allocator->realloc = &jadstd_StdlibRealloc;
	allocator->free = &jadstd_StdlibFree;
	return JAD_OK;
}

jadError jadstd_CloseAllocator(jadAllocator* allocator)
{
	return JAD_OK;
}
