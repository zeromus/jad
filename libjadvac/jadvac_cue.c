#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include "jadvac.h"
#include "../libjad/jad.h"

//TODO: mednafen can skip a UTF-8 BOM
//TODO: mednafen can handle // comments and maybe also # too
//TODO: disordered tracks should be illegal

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
#define BOMB(x, ...) { goto ERROR; }
#define SKIPWHITESPACE() while(c == ' ' || c == '\t') READ();

#define _READTOKEN() { \
	SKIPWHITESPACE(); \
	sb_reset(L,&L->buffer); \
	while(isalnum(c)) { sb_add(L,&L->buffer,c); READ(); } \
	sb_nullterminate(L,&L->buffer); \
	SKIPWHITESPACE(); \
	}

#define _READSTRING_QUOTED() { \
	SKIPWHITESPACE(); \
	sb_reset(L,&L->buffer); \
	while(c != '\"' && c!= JAD_EOF) { sb_add(L,&L->buffer,c); READ(); } \
	sb_nullterminate(L,&L->buffer); \
	SKIPWHITESPACE(); \
	}

#define _READSTRING() { \
	SKIPWHITESPACE(); \
	sb_reset(L,&L->buffer); \
	while(c != ' ' && c != '\n' && c != '\t' && c != JAD_EOF) { sb_add(L,&L->buffer,c); READ(); } \
	sb_nullterminate(L,&L->buffer); \
	SKIPWHITESPACE(); \
	}

#define READTOKEN() { _READTOKEN(); }
#define EXPECT(X) if(c != X) { BOMB("Expected " X); }

#define READSTRING() { \
	if(c == '\"') { \
		READ(); \
		_READSTRING_QUOTED(); \
		EXPECT('\"'); \
		READ(); \
	} else _READSTRING(); \
	}

#define READLINE() { while(c != '\n') READ(); }

#define READDIGITS() { \
	READTOKEN(); \
	digits = 0; \
	if(L->buffer.length==0) BOMB("Expected a 2 digit number; got %s", L->buffer.buf);  \
	for(int i=0;i<L->buffer.length;i++) {  \
		if(isdigit(L->buffer.buf[i])) { digits *= 10; digits += L->buffer.buf[i] - '0'; } \
		else BOMB("Expected a 2 digit number; got %s", L->buffer.buf); \
	} \
	}

#define READMSF() { \
	READDIGITS(); m = digits; \
	EXPECT(':'); READ(); READDIGITS(); s = digits; \
	EXPECT(':'); READ(); READDIGITS(); f = digits; \
	}
#define CHECKTOKEN(x) checktoken(&L->buffer,x)


enum TrackType
{
	TrackType_None, //used to flag unused tracks
	TrackType_Audio,
	TrackType_Mode1_2048,
	TrackType_Mode1_2352,
};

typedef enum FileType
{
	FileType_None,
	FileType_Binary
} FileType;

typedef struct Blob
{
	FileType type;
	jadStream stream;
} Blob;

typedef struct IndexInfo
{
	int exists;
	int lba;
} IndexInfo;

typedef struct TrackInfo
{
	int lba; //within file
	int pregap, postgap;
	enum TrackType type;
	struct IndexInfo indexes[100];

	//if this is the first track in a blob, that's recorded here
	struct Blob* newBlob;
} TrackInfo;

static Blob* blob_alloc(jadAllocator* allocator)
{
	return (Blob*)allocator->alloc(allocator,sizeof(Blob));
}

static void process(jadvacContext* ctx, TrackInfo* tracks)
{
	int firstTrack = -1, lastTrack = -1;
	Blob* currBlob = NULL;
	int lba = 0;
	//jadCreationParams _jcp; //eh should this have come in here, i dont know
	//jadCreationParams* jcp = &_jcp;
	//jcp->numTocEntries = 0;
	//jcp->tocEntries = NULL;
	//jcp->numSectors = length;
	//jcp->callback = _jadCreateCallback;

	//identify first and last tracks
	for(int i=1;i<100;i++)
	{
		if(tracks[i].type != TrackType_None)
		{
			if(firstTrack == -1) firstTrack = i;
			lastTrack = i;
		}
	}

	//
}

int jadvacOpenFile_cue(jadvacContext* ctx)
{
	Blob* currBlob = NULL;
	int retval = JAD_ERROR;
	int c = 0, ret, digits;
	int pregap_lba = -1;
	CueLexer _L = {0}, *L = &_L;
	struct {
		StringBuffer performer, songwriter, title, catalog, isrc, cdtextfile;
	} meta;

	//this might be getting a little large for a stack so...
	TrackInfo* tracks = ctx->allocator->alloc(ctx->allocator,sizeof(TrackInfo)*100);
	int tracknum = -1;

	L->allocator = ctx->allocator;
	L->lastc = '\n';
	L->stream = ctx->stream;
	sb_init(L,&L->buffer);

	sb_init(L,&meta.performer);
	sb_init(L,&meta.songwriter);
	sb_init(L,&meta.title);
	sb_init(L,&meta.catalog);
	sb_init(L,&meta.isrc);
	sb_init(L,&meta.cdtextfile);

	memset(tracks,0,sizeof(struct TrackInfo)*100);


LOOP:
	READ();
	READTOKEN();

	//commands that dont matter much
	if(CHECKTOKEN("PERFORMER")) {
		READSTRING();
		sb_clone(L, &L->buffer, &meta.performer);
		goto ENDLINE;
	}
	if(CHECKTOKEN("SONGWRITER")) { READSTRING(); sb_clone(L, &L->buffer, &meta.songwriter); goto ENDLINE; }
	if(CHECKTOKEN("TITLE")) { READSTRING(); sb_clone(L, &L->buffer, &meta.title); goto ENDLINE; }
	if(CHECKTOKEN("CATALOG")) { READSTRING(); sb_clone(L, &L->buffer, &meta.catalog); goto ENDLINE; }
	if(CHECKTOKEN("ISRC")) { READSTRING(); sb_clone(L, &L->buffer, &meta.isrc); goto ENDLINE; }
	if(CHECKTOKEN("CDTEXTFILE")) { READSTRING(); sb_clone(L, &L->buffer, &meta.cdtextfile); goto ENDLINE; }
	if(CHECKTOKEN("REM")) { READLINE(); goto LOOP; }

	if(CHECKTOKEN("FILE"))
	{
		jadStream stream;

		READSTRING();
		ret = ctx->fs->open(ctx->fs,&stream,L->buffer.buf);
		if(ret) { retval = ret; goto EXIT; }
		READTOKEN(); //file type
		if(CHECKTOKEN("BINARY")) {}
		else BOMB("Invalid file type");

		//currently only binary files are allowed, since we don't have the audio decoding framework in

		//ok, startup the new file
		currBlob = blob_alloc(ctx->allocator);
		currBlob->stream = stream;
		currBlob->type = FileType_Binary;

		goto ENDLINE;
	}

	if(CHECKTOKEN("PREGAP"))
	{
		int m, s, f;
		if(tracknum == -1) BOMB("Invalid INDEX command\n");
		READMSF();
		tracks[tracknum].pregap = m*60*75+s*75+f;
	}
	if(CHECKTOKEN("POSTGAP"))
	{
		int m, s, f;
		if(tracknum == -1) BOMB("Invalid INDEX command\n");
		READMSF();
		tracks[tracknum].postgap = m*60*75+s*75+f;
	}

	if(CHECKTOKEN("INDEX"))
	{
		int indexnum, m, s, f;
		if(tracknum == -1) BOMB("Invalid INDEX command\n");
		READDIGITS();
		indexnum = digits;
		READMSF();
		tracks[tracknum].indexes[indexnum].exists = 1;
		tracks[tracknum].lba = m*60*75+s*75+f;
	}

	if(CHECKTOKEN("TRACK"))
	{
		READDIGITS();
		tracknum = digits;
		READTOKEN(); //track type
		if(CHECKTOKEN("AUDIO")) tracks[tracknum].type = TrackType_Audio;
		else if(CHECKTOKEN("MODE1/2352")) tracks[tracknum].type = TrackType_Mode1_2352;
		else BOMB("Invalid track type");

		//if we have an unassociated blob, now's the time to associte it
		tracks[tracknum].newBlob = currBlob;
		currBlob = NULL;

		goto ENDLINE;
	}

ENDLINE:
	READLINE();
	goto LOOP;

	retval = JAD_OK;

EXIT:
	//todo: unassociated blob
	sb_finalize(L,&meta.performer);
	sb_finalize(L,&meta.songwriter);
	sb_finalize(L,&meta.title);
	sb_finalize(L,&meta.catalog);
	sb_finalize(L,&meta.isrc);
	sb_finalize(L,&meta.cdtextfile);
	sb_finalize(L,&L->buffer);
	ctx->allocator->free(ctx->allocator,tracks);
	return retval;

ERROR:
	retval = JAD_ERROR;
	goto EXIT;
}
