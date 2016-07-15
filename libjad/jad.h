#ifndef _JAD_H
#define _JAD_H

//TODO - decide what's for internal use only and take out of the public header
//TODO - decide what we even need in the first place
//TODO - maybe split all the jadUtil into another file, or maybe theyre all for jadTool, or maybe they should be named jadTool etc.
//TODO - jadContext API for reading out the TOC? for handling redundancies and boiling down to 100? we dont wnat to forcibly store the whole thing in memory
//TODO - divide codecs from streams? i thought maybe by combining them we could support both streamed and buffered scenarios... but i dont know.

#include <stdint.h>

#ifdef  __cplusplus
extern "C" {
#endif

//error types
typedef int jadError;

#define JAD_OK 0
#define JAD_EOF -1
#define JAD_ERROR -2
#define JAD_ERROR_FILE_NOT_FOUND -3

//type forward declarations
typedef struct jadSubchannelQ jadSubchannelQ;
typedef struct jadStream jadStream;
typedef struct jadContext jadContext;
typedef struct jadAllocator jadAllocator;
typedef uint8_t jadBCD2;

typedef enum
{
	jadEnumControlQ_None = 0,

	jadEnumControlQ_StereoNoPreEmph = 0,
	jadEnumControlQ_StereoPreEmph = 1,
	jadEnumControlQ_MonoNoPreemph = 8,
	jadEnumControlQ_MonoPreEmph = 9,
	jadEnumControlQ_DataUninterrupted = 4,
	jadEnumControlQ_DataIncremental = 5,

	jadEnumControlQ_CopyProhibitedMask = 0,
	jadEnumControlQ_CopyPermittedMask = 2,
} jadEnumControlQ;

enum jadEncodeSettings
{
	jadEncodeSettings_None = 0,

	jadEncodeSettings_DirectCopy = 1,
};

enum jadFlags
{
	jadFlags_None = 0,

	//whether the file is JAC subformat (compressed)
	jadFlags_JAC = 1,

	//whether hash fields are present/filled-in
	jadFlags_IsHashed = 2,
};

//the read callback for a jadStream
typedef int (*jadStreamRead)(void* buffer, size_t bytes, jadStream* stream);

//the write callback for a jadStream
typedef int (*jadStreamWrite)(const void* buffer, size_t bytes, jadStream* stream);

//the seek callback for a jadStream. like fseek, but returns ftell. (0,SEEK_CUR) should work as ftell and work even on fundamentally non-seekable streams.
typedef long (*jadStreamSeek)(jadStream* stream, long offset, int origin);

//the codec get callback for a jadStream, like getc, but no provision for ferror is provided
typedef int (*jadStreamGet)(jadStream* stream);

//the codec put callback for a jadStream, like putc, but no provision for ferror is provided
typedef int (*jadStreamPut)(jadStream* stream, uint8_t val);

//just used for codec flushing
typedef int (*jadStreamFlush)(jadStream* stream);

//callbacks for jadAllocator operations
typedef void* (*jadAllocatorAlloc)(jadAllocator* allocator, size_t amt);
typedef void* (*jadAllocatorRealloc)(jadAllocator* allocator, void* ptr, size_t amt);
typedef void (*jadAllocatorFree)(jadAllocator* allocator, void* ptr);

//a stdio-like stream to be used for random access. fill it with function pointers and pass to jadOpen
struct jadStream
{
	void* opaque;
	jadStreamRead read;
	jadStreamWrite write;
	jadStreamSeek seek;
	jadStreamGet get;
	jadStreamPut put;
	jadStreamFlush flush;
};

//a simple memory allocator interface
typedef struct jadAllocator
{
	void* opaque;
	jadAllocatorAlloc alloc;
	jadAllocatorRealloc realloc;
	jadAllocatorFree free;
} jadAllocator;

//a timestamp like the format stored on a disc
typedef struct jadTimestamp
{
	jadBCD2 MIN, SEC, FRAC;
	uint8_t _padding;
} jadTimestamp;

//one sector of data in the 2448 byte raw JAD format
struct jadSector
{
	//nonstandard anonymous struct syntax. does anyone actually have a compiler that can't handle it?
	union {
		struct {
			uint8_t data[2352];
			uint8_t subcode[96];
		};
		uint8_t entire[2448];
	};
};

//a single TOC entry (essentially the contents of the Q subchannel)
typedef uint8_t jadTOCEntry[12];

//clears a jadTOCEntry to zeroes
void jadTOCEntry_Clear(jadTOCEntry* tocentry);

//the callback used by libjad to fetch sector raw data when creating a jad/jac file
//set a pointer to the sector and subcode buffer. subcode should be de-interleaved.
//these pointers should stay stable until the next call to jadCreateCallback or until common sense tells you theyre done being used by libjad
typedef int (*jadCreateReadCallback)(void* opaque, int sectorNumber, void** sectorBuffer, void **subcodeBuffer);

//the params struct used for jadCreate
//do not modify this while the context created by jadCreate is still open. moreover, the tocEntries struct must remain intact.
//TODO: this is really more of a 'read-write mounted disc' structure. It's what gets used when mounting a disc through libjadvac. so, rename it?
typedef struct jadCreationParams
{
	void* opaque;
	int numTocEntries;
	jadTOCEntry* tocEntries;
	jadAllocator* allocator;
	int numSectors;
	jadCreateReadCallback callback;
} jadCreationParams;


//the context for JAD-io operations.
//consider it as opaque (full declaration provided for easier static allocation)
//create with jadOpen, etc.
struct jadContext
{
	struct jadStream* stream;
	struct jadAllocator* allocator;
	uint32_t flags, numSectors, numTocEntries;
	struct jadCreationParams* createParams;
};


//performs necessary static initialization
int jadStaticInit();

//opens a jadContext, which gets its data from the provided stream (containing a jad/jac file) and allocator
int jadOpen(jadContext* jad, jadStream* stream, jadAllocator* allocator);

//creates a jadContext based on the provided jadCreationParams, which describe the disc
int jadCreate(jadContext* jad, jadCreationParams* jcp, jadAllocator* allocator);

//dumps the given jad to the output stream as a JAD or JAC file
int jadDump(jadContext* jad, jadStream* stream, int JACIT);

//closes a jadContext opened with jadOpen or jadCreate. do not call on unopened jadContexts.
int jadClose(jadContext* jad);

void jadTimestamp_increment(jadTimestamp* ts);

//general portability utilities for libjad and friends
size_t jadutil_strlcpy(char *dst, const char *src, size_t size);
size_t jadutil_strlcat(char *dst, const char *src, size_t size);
const char *jadutil_strrchr(const char* src, size_t len, char c);

#ifdef  __cplusplus
}
#endif

#endif //_JAD_H
