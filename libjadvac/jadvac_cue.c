#include <stdio.h>
#include <ctype.h>

#include "jadvac.h"
#include "../libjad/jad.h"

typedef struct StringBuffer
{
	int length, capacity;
	char* buf;
} StringBuffer;


typedef struct CueLexer
{
	int lastc, unget;
	int line, col;
	jadStream* stream;
	jadAllocator* allocator;
	StringBuffer buffer;
} CueLexer;

static void sb_init(CueLexer* L, StringBuffer* sb)
{
	sb->buf = NULL;
	sb->length = sb->capacity = 0;
}

static void sb_finalize(CueLexer* L, StringBuffer* sb)
{
	L->allocator->free(L->allocator,sb->buf);
}

static void sb_add(CueLexer* L, StringBuffer* sb, char c)
{
	if(sb->length == sb->capacity)
	{
		sb->capacity = sb->capacity?sb->capacity*2:128;
		sb->buf = L->allocator->realloc(L->allocator, sb->buf, sb->capacity);
	}
	sb->buf[sb->length++] = c;
}

static void sb_nullterminate(CueLexer* L, StringBuffer* sb)
{
	sb_add(L,sb,0);
	sb->length--; //length is now logical length, not physical length of buffer
}

static void sb_reset(CueLexer* L, StringBuffer* sb)
{
	sb->length = 0;
}

static int read(CueLexer* L)
{
	for (;;)
	{
		int c;
		if(L->lastc == '\n') { L->line++; L->col = 0; }
		if(L->unget) { c = L->unget; L->unget = 0; }
		else { c = L->stream->get(L->stream); L->col++; }

		if (c == '\r')
		{
			c = L->stream->get(L->stream);
			if (c == '\n')
			{
				// \r\n was received; eat and return \n
			}
			else
			{
				// \r ? was received. return \n and unget ?
				// \r\r was received; return \n and unget \r
				L->unget = c;
				c = '\n';
			}
		}
		else
		{
			// \n was received; eat and return \n
			// ? was received; eat and return ?
		}

		//for convenience in the lexer, we transform EOF to \0
		if (c == JAD_EOF) c = 0;

		return L->lastc = c;
	}
}

static int checktoken(StringBuffer* sb, const char* target)
{
	char* cp = sb->buf;
	char *cp_end = cp + sb->length;
	for(;;)
	{
		//if we made it to the end of both strings, then we had a match
		if(cp == cp_end && *target == 0) return 1;
		//if we made it to the end of one of the strings, we dont have a match
		if(cp == cp_end) return 0;
		if(*target == 0) return 0;

		int a = tolower(*cp);
		int b = tolower(*target);
		if(a != b) return 0;

		cp++;
		target++;
	}
}

//TODO - not sure about some of these character classes
//could provide better invalid character detection in tokens and strings instead of just truncating them
#define READ() { c = read(L); }
//placeholder for future diagnostic functionality
#define BOMB(x, ...) { return JAD_ERROR; }
#define SKIPWHITESPACE() while(c == ' ' || c == '\t' ||  c == '\n') READ();
#define _READTOKEN() \
	sb_reset(L,&L->buffer); \
	while(isalnum(c)) { sb_add(L,&L->buffer,c); READ(); } \
	sb_nullterminate(L,&L->buffer);
#define _READSTRING_QUOTED() \
	sb_reset(L,&L->buffer); \
	while(c != '\"' && c!= JAD_EOF) { sb_add(L,&L->buffer,c); READ(); } \
	sb_nullterminate(L,&L->buffer);
#define _READSTRING() \
	sb_reset(L,&L->buffer); \
	while(c != ' ' && c != '\n' && c != '\t' && c != JAD_EOF) { sb_add(L,&L->buffer,c); READ(); } \
	sb_nullterminate(L,&L->buffer);
#define READTOKEN() _READTOKEN()
#define EXPECT(X) if(c != X) { BOMB("Expected " X); }
#define READSTRING() \
	if(c == '\"') { \
		READ(); \
		_READSTRING_QUOTED(); \
		EXPECT('\"'); \
		READ(); \
	} else _READSTRING();


//opens a jadContext, which gets its data from the provided stream (containing a cue file) and allocator
//TODO: pass in other things, like a subfile resolver
int jadvacOpenFile_cue(struct jadvacContext* ctx)
{
	int c, ret;
	jadStream currFile = {0};
	CueLexer _L = {0}, *L = &_L;
	L->allocator = ctx->allocator;
	L->lastc = '\n';
	L->stream = ctx->stream;
	sb_init(L,&L->buffer);

	READ();
	SKIPWHITESPACE();
	READTOKEN();

	if(checktoken(&L->buffer,"FILE"))
	{
		//process file command
		SKIPWHITESPACE();
		READSTRING();
		printf("%s\n",L->buffer.buf);
		ret = ctx->fs->open(ctx->fs,&currFile,L->buffer.buf);
		if(ret) return ret;
	}

	return JAD_OK;
}
