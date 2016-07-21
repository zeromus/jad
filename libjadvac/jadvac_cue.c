#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include "jadvac.h"
#include "../libjad/jad.h"

//TODO: mednafen can skip a UTF-8 BOM
//TODO: mednafen can handle // comments and maybe also # too

typedef struct StringBuffer
{
	char* buf;
	int capacity; //this is always the buffer capacity
	int length; //this may be logical or physical length depending on whether it's terminated
	int terminated; //whether it's terminated
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
	sb->terminated = 0;
}

static void sb_finalize(CueLexer* L, StringBuffer* sb)
{
	L->allocator->free(L->allocator,sb->buf);
}

static void sb_clone(CueLexer* L, StringBuffer* sb, StringBuffer* sbOut)
{
	//clean up the target StringBuffer
	sb_finalize(L,sbOut);

	//if source StringBuffer is empty, we have an easy job
	if(!sb->buf)
	{
		*sbOut = *sb;
		return;
	}

	//we'll create this null terminated
	sbOut->capacity = sb->length + 1;
	sbOut->length = sb->length;
	sbOut->buf = L->allocator->alloc(L->allocator, sbOut->capacity);
	sbOut->terminated = 1;
	memcpy(sbOut->buf,sb->buf,sbOut->length);
	sbOut->buf[sbOut->length] = 0;
}

static void sb_add(CueLexer* L, StringBuffer* sb, char c)
{
	//if we're terminated, clobber the null terminator
	if(sb->terminated)
	{
		sb->buf[sb->length++] = c;
		sb->terminated = 0;
		return;
	}

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
	sb->terminated = 1;
}

static void sb_reset(CueLexer* L, StringBuffer* sb)
{
	sb->length = 0;
	sb->terminated = 0;
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
#define READLINE() while(c != '\n') READ();
#define READDIGITS() \
	READTOKEN(); \
	digits = 0; \
	if(L->buffer.length==0) BOMB("Expected a number; got %s", L->buffer.buf);  \
	for(int i=0;i<L->buffer.length;i++) {  \
		if(isdigit(L->buffer.buf[i])) { digits *= 10; digits += L->buffer.buf[i] - '0'; } \
		else BOMB("Expected a number; got %s", L->buffer.buf); \
	}


//opens a jadContext, which gets its data from the provided stream (containing a cue file) and allocator
//TODO: pass in other things, like a subfile resolver
int jadvacOpenFile_cue(struct jadvacContext* ctx)
{
	int c, ret, digits;
	jadStream currFile = {0};
	CueLexer _L = {0}, *L = &_L;
	struct {
		StringBuffer performer, songwriter, title, catalog, isrc, cdtextfile;
	} meta;

	L->allocator = ctx->allocator;
	L->lastc = '\n';
	L->stream = ctx->stream;
	sb_init(L,&L->buffer);

	//init meta
	sb_init(L,&meta.performer);
	sb_init(L,&meta.songwriter);
	sb_init(L,&meta.title);
	sb_init(L,&meta.catalog);
	sb_init(L,&meta.isrc);
	sb_init(L,&meta.cdtextfile);

LOOP:
	READ();
	SKIPWHITESPACE();
	READTOKEN();
	SKIPWHITESPACE();

	//commands that dont matter much
	if(checktoken(&L->buffer,"PERFORMER")) { READSTRING(); sb_clone(L, &L->buffer, &meta.performer); goto ENDLINE; }
	if(checktoken(&L->buffer,"SONGWRITER")) { READSTRING(); sb_clone(L, &L->buffer, &meta.songwriter); goto ENDLINE; }
	if(checktoken(&L->buffer,"TITLE")) { READSTRING(); sb_clone(L, &L->buffer, &meta.title); goto ENDLINE; }
	if(checktoken(&L->buffer,"CATALOG")) { READSTRING(); sb_clone(L, &L->buffer, &meta.catalog); goto ENDLINE; }
	if(checktoken(&L->buffer,"ISRC")) { READSTRING(); sb_clone(L, &L->buffer, &meta.isrc); goto ENDLINE; }
	if(checktoken(&L->buffer,"CDTEXTFILE")) { READSTRING(); sb_clone(L, &L->buffer, &meta.cdtextfile); goto ENDLINE; }
	if(checktoken(&L->buffer,"REM")) { READLINE(); goto LOOP; }

	if(checktoken(&L->buffer,"FILE"))
	{
		READSTRING();
		ret = ctx->fs->open(ctx->fs,&currFile,L->buffer.buf);
		if(ret) return ret;
		READTOKEN(); //file type
		goto ENDLINE;
	}

	if(checktoken(&L->buffer,"TRACK"))
	{
		READDIGITS();
		goto ENDLINE;
	}

ENDLINE:
	READLINE();
	goto LOOP;

	return JAD_OK;
}
